#include "seq/seq.h"

using namespace seq;
using namespace llvm;

static void ensureNonVoid(types::Type *type)
{
	if (type->is(types::Void))
		throw exc::SeqException("cannot load or store void variable");
}

Var::Var(types::Type *type) :
    type(type), ptr(nullptr), global(false)
{
}

void Var::allocaIfNeeded(BaseFunc *base)
{
	if (ptr)
		return;

	LLVMContext& context = base->getContext();
	if (global) {
		Type *llvmType = getType()->getLLVMType(context);
		ptr = new GlobalVariable(*base->getPreamble()->getModule(),
		                         llvmType,
		                         false,
		                         GlobalValue::PrivateLinkage,
		                         Constant::getNullValue(llvmType),
		                         "var");
	} else {
		ptr = makeAlloca(getType()->getLLVMType(context), base->getPreamble());
	}
}

bool Var::isGlobal()
{
	return global;
}

void Var::setGlobal()
{
	global = true;
}

Value *Var::load(BaseFunc *base, BasicBlock *block)
{
	ensureNonVoid(getType());
	allocaIfNeeded(base);
	IRBuilder<> builder(block);
	return builder.CreateLoad(ptr);
}

void Var::store(BaseFunc *base, Value *val, BasicBlock *block)
{
	ensureNonVoid(getType());
	allocaIfNeeded(base);
	IRBuilder<> builder(block);
	builder.CreateStore(val, ptr);
}

void Var::setType(types::Type *type)
{
	this->type = type;
}

types::Type *Var::getType()
{
	assert(type);
	return type;
}

Var *Var::clone(Generic *ref)
{
	if (ref->seenClone(this))
		return (Var *)ref->getClone(this);

	auto *x = new Var();
	ref->addClone(this, x);
	if (type) x->setType(type->clone(ref));
	return x;
}
