#pragma once

#include "sir/sir.h"

namespace seq {
namespace ir {
namespace util {

/// Checks whether a function has a given attribute.
/// @param func the function
/// @param attribute the attribute name
/// @return true if the function has the given attribute
bool hasAttribute(const Func *func, const std::string &attribute);

/// Checks whether a function comes from the standard library, and
/// optionally a specific module therein.
/// @param func the function
/// @param submodule module name (e.g. "std::bio"), or empty if
///                  no module check is required
/// @return true if the function is from the standard library in
///         the given module
bool isStdlibFunc(const Func *func, const std::string &submodule = "");

/// Calls a function.
/// @param func the function
/// @param args vector of call arguments
/// @return call instruction with the given function and arguments
CallInstr *call(Func *func, const std::vector<Value *> &args);

/// Constructs a new tuple.
/// @param args vector of tuple contents
/// @param M the module
/// @return value represents a tuple with the given contents
Value *makeTuple(const std::vector<Value *> &args, Module *M);

/// Constructs and assigns a new variable.
/// @param x the value to assign to the new variable
/// @param flow series flow in which to assign the new variable
/// @param parent function to add the new variable to
/// @return value containing the new variable
VarValue *makeVar(Value *x, SeriesFlow *flow, BodiedFunc *parent);

/// Dynamically allocates memory for the given type with the given
/// number of elements.
/// @param type the type
/// @param count integer value representing the number of elements
/// @return value representing a pointer to the allocated memory
Value *alloc(types::Type *type, Value *count);

/// Dynamically allocates memory for the given type with the given
/// number of elements.
/// @param type the type
/// @param count the number of elements
/// @return value representing a pointer to the allocated memory
Value *alloc(types::Type *type, int64_t count);

/// Builds a new series flow with the given contents. Returns
/// null if no contents are provided.
/// @param args contents of the series flow
/// @return new series flow
template <typename... Args> SeriesFlow *series(Args... args) {
  std::vector<Value *> vals = {args...};
  if (vals.empty())
    return nullptr;
  auto *series = vals[0]->getModule()->Nr<SeriesFlow>();
  for (auto *val : vals) {
    series->push_back(val);
  }
  return series;
}

/// Checks whether the given value is a constant of the given
/// type. Note that standard "int" corresponds to the C type
/// "int64_t", which should be used here.
/// @param x the value to check
/// @return true if the value is constant
template <typename T> bool isConst(const Value *x) { return isA<TemplatedConst<T>>(x); }

/// Checks whether the given value is a constant of the given
/// type, and that is has a particular value. Note that standard
/// "int" corresponds to the C type "int64_t", which should be used here.
/// @param x the value to check
/// @param value constant value to compare to
/// @return true if the value is constant with the given value
template <typename T> bool isConst(const Value *x, const T &value) {
  if (auto *c = cast<TemplatedConst<T>>(x)) {
    return c->getVal() == value;
  }
  return false;
}

/// Returns the constant represented by a given value. Raises an assertion
/// error if the given value is not constant. Note that standard
/// "int" corresponds to the C type "int64_t", which should be used here.
/// @param x the (constant) value
/// @return the constant represented by the given value
template <typename T> T getConst(const Value *x) {
  auto *c = cast<TemplatedConst<T>>(x);
  assert(c);
  return c->getVal();
}

/// Gets a variable from a value.
/// @param x the value
/// @return the variable represented by the given value, or null if none
Var *getVar(Value *x);

/// Gets a variable from a value.
/// @param x the value
/// @return the variable represented by the given value, or null if none
const Var *getVar(const Value *x);

/// Gets a function from a value.
/// @param x the value
/// @return the function represented by the given value, or null if none
Func *getFunc(Value *x);

/// Gets a function from a value.
/// @param x the value
/// @return the function represented by the given value, or null if none
const Func *getFunc(const Value *x);

/// Gets a bodied standard library function from a value.
/// @param x the value
/// @param name name of the function
/// @param submodule optional module to check
/// @return the standard library function (with the given name, from the given
/// submodule) represented by the given value, or null if none
BodiedFunc *getStdlibFunc(Value *x, const std::string &name,
                          const std::string &submodule = "");

/// Gets a bodied standard library function from a value.
/// @param x the value
/// @param name name of the function
/// @param submodule optional module to check
/// @return the standard library function (with the given name, from the given
/// submodule) represented by the given value, or null if none
const BodiedFunc *getStdlibFunc(const Value *x, const std::string &name,
                                const std::string &submodule = "");

/// Gets the return type of a function.
/// @param func the function
/// @return the return type of the given function
types::Type *getReturnType(const Func *func);

/// Sets the return type of a function. Argument types remain unchanged.
/// @param func the function
/// @param rType the new return type
void setReturnType(Func *func, types::Type *rType);

} // namespace util
} // namespace ir
} // namespace seq
