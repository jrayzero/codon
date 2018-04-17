#include "seq/basestage.h"
#include "seq/call.h"
#include "seq/exc.h"
#include "seq/func.h"

using namespace seq;
using namespace llvm;

BaseFunc::BaseFunc() :
    compilationContext(), module(nullptr), initBlock(nullptr),
    preambleBlock(nullptr), initFunc(nullptr), func(nullptr)
{
}

LLVMContext& BaseFunc::getContext()
{
	return module->getContext();
}

BasicBlock *BaseFunc::getInit() const
{
	if (!initBlock)
		throw exc::SeqException("cannot request initialization block before code generation");

	return initBlock;
}

BasicBlock *BaseFunc::getPreamble() const
{
	if (!preambleBlock)
		throw exc::SeqException("cannot request preamble before code generation");

	return preambleBlock;
}

types::Type *BaseFunc::getInType() const
{
	return types::VoidType::get();
}

types::Type *BaseFunc::getOutType() const
{
	return types::VoidType::get();
}

Func::Func(types::Type& inType,
           types::Type& outType,
           std::string name,
           void *rawFunc) :
    BaseFunc(), inType(&inType), outType(&outType),
    pipelines(), outs(new std::map<SeqData, Value *>),
    name(std::move(name)), rawFunc(rawFunc)
{
}

Func::Func(types::Type& inType, types::Type& outType) :
    Func(inType, outType, "", nullptr)
{
}

void BaseFunc::codegenInit(Module *module)
{
	static int idx = 1;

	if (initFunc)
		return;

	LLVMContext& context = module->getContext();

	initFunc = cast<Function>(
	             module->getOrInsertFunction(
	               "init" + std::to_string(idx++),
	               Type::getVoidTy(context)));

	BasicBlock *entryBlock = BasicBlock::Create(context, "entry", initFunc);
	initBlock = BasicBlock::Create(context, "init", initFunc);
	BasicBlock *exitBlock = BasicBlock::Create(context, "exit", initFunc);

	GlobalVariable *init = new GlobalVariable(*module,
	                                          IntegerType::getInt1Ty(context),
	                                          false,
	                                          GlobalValue::PrivateLinkage,
	                                          nullptr,
	                                          "init");

	init->setInitializer(ConstantInt::get(IntegerType::getInt1Ty(context), 0));

	IRBuilder<> builder(entryBlock);
	Value *initVal = builder.CreateLoad(init);
	builder.CreateCondBr(initVal, exitBlock, initBlock);

	builder.SetInsertPoint(initBlock);
	builder.CreateStore(ConstantInt::get(IntegerType::getInt1Ty(context), 1), init);

	builder.SetInsertPoint(exitBlock);
	builder.CreateRetVoid();
}

void BaseFunc::finalizeInit(Module *module)
{
	IRBuilder<> builder(initBlock);
	builder.CreateRetVoid();
}

void Func::codegen(Module *module)
{
	if (func)
		return;

	func = inType->makeFuncOf(module, outType);

	if (rawFunc) {
		func->setName(name);
		return;
	}

	if (pipelines.empty())
		throw exc::SeqException("function has no pipelines");

	compilationContext.reset();
	LLVMContext& context = module->getContext();

	preambleBlock = BasicBlock::Create(context, "preamble", func);
	IRBuilder<> builder(preambleBlock);

	codegenInit(module);
	builder.CreateCall(initFunc);
	inType->setFuncArgs(func, outs, preambleBlock);

	BasicBlock *entry = BasicBlock::Create(context, "entry", func);
	builder.SetInsertPoint(entry);
	BasicBlock *block;

	compilationContext.inFunc = true;
	compilationContext.inMain = true;
	for (auto &pipeline : pipelines) {
		pipeline.validate();
		builder.SetInsertPoint(&func->getBasicBlockList().back());
		block = BasicBlock::Create(context, "pipeline", func);
		builder.CreateBr(block);

		auto *begin = dynamic_cast<BaseStage *>(pipeline.getHead());
		assert(begin);
		begin->setBase(pipeline.getHead()->getBase());
		begin->block = block;
		pipeline.getHead()->codegen(module);
	}
	compilationContext.inMain = false;

	BasicBlock *exitBlock = &func->getBasicBlockList().back();
	builder.SetInsertPoint(exitBlock);

	if (outType->isChildOf(types::VoidType::get())) {
		builder.CreateRetVoid();
	} else {
		Stage *tail = pipelines.back().getHead();
		while (!tail->getNext().empty())
			tail = tail->getNext().back();

		if (!tail->getOutType()->isChildOf(outType))
			throw exc::SeqException("function does not output type '" + outType->getName() + "'");

		ValMap tailOuts = tail->outs;
		Value *result = outType->pack(this, tailOuts, exitBlock);
		builder.CreateRet(result);
	}

	builder.SetInsertPoint(preambleBlock);
	builder.CreateBr(entry);

	finalizeInit(module);
}

Value *Func::codegenCallRaw(BaseFunc *base, ValMap ins, BasicBlock *block)
{
	module = block->getModule();
	codegen(module);
	return inType->callFuncOf(func, ins, block);
}

void Func::codegenCall(BaseFunc *base, ValMap ins, ValMap outs, BasicBlock *block)
{
	Value *result = codegenCallRaw(base, ins, block);
	outType->unpack(base, result, outs, block);
}

void Func::add(Pipeline pipeline)
{
	if (pipeline.isAdded())
		throw exc::MultiLinkException(*pipeline.getHead());

	pipelines.push_back(pipeline);
	pipeline.setAdded();
}

void Func::finalize(Module *module, ExecutionEngine *eng)
{
	if (rawFunc) {
		eng->addGlobalMapping(func, rawFunc);
	} else {
		for (auto &pipeline : pipelines) {
			pipeline.getHead()->finalize(module, eng);
		}
	}
}

types::Type *Func::getInType() const
{
	return inType;
}

types::Type *Func::getOutType() const
{
	return outType;
}

Pipeline Func::operator|(Pipeline to)
{
	if (rawFunc)
		throw exc::SeqException("cannot add pipelines to native function");

	to.getHead()->setBase(this);
	BaseStage& begin = BaseStage::make(types::VoidType::get(), inType, nullptr);
	begin.setBase(this);
	begin.outs = outs;

	Pipeline full = begin | to;
	add(full);

	return full;
}

Pipeline Func::operator|(PipelineList to)
{
	for (auto *node = to.head; node; node = node->next) {
		*this | node->p;
	}

	return {to.head->p.getHead(), to.tail->p.getTail()};
}

Pipeline Func::operator|(Var& to)
{
	if (rawFunc)
		throw exc::SeqException("cannot add pipelines to native function");

	if (!to.isAssigned())
		throw exc::SeqException("variable used before assigned");

	to.ensureConsistentBase(this);
	Stage *stage = to.getStage();
	BaseStage& begin = BaseStage::make(types::VoidType::get(), to.getType(stage), stage);
	begin.setBase(this);
	begin.outs = to.outs(&begin);
	add(begin);

	return begin;
}

Call& Func::operator()()
{
	return Call::make(*this);
}

FuncList::Node::Node(Func& f) :
    f(f), next(nullptr)
{
}

FuncList::FuncList(Func& f)
{
	head = tail = new Node(f);
}

FuncList& FuncList::operator,(Func& f)
{
	auto *n = new Node(f);
	tail->next = n;
	tail = n;
	return *this;
}

FuncList& seq::operator,(Func& f1, Func& f2)
{
	auto& l = *new FuncList(f1);
	l , f2;
	return l;
}

MultiCall& FuncList::operator()()
{
	std::vector<Func *> funcs;
	for (Node *n = head; n; n = n->next)
		funcs.push_back(&n->f);

	return MultiCall::make(funcs);
}
