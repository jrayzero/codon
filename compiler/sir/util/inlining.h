#pragma once

#include "sir/sir.h"

namespace seq {
namespace ir {
namespace util {

/// Result of an inlining operation.
struct InlineResult {
  /// the result, either a SeriesFlow or FlowInstr
  Value *result;
  /// variables added by the inlining
  std::vector<Var *> newVars;

  operator bool() const { return bool(result); }
};

/// Inline the given function with the supplied arguments.
/// @param func the function
/// @param args the arguments
/// @param callInfo the call information
/// @param aggressive true if should inline complex functions
/// @return the inlined result, nullptr if unsuccessful
InlineResult inlineFunction(Func *func, std::vector<Value *> args,
                            bool aggressive = false, seq::SrcInfo callInfo = {});

/// Inline the given call.
/// @param v the instruction
/// @param aggressive true if should inline complex functions
/// @return the inlined result, nullptr if unsuccessful
InlineResult inlineCall(CallInstr *v, bool aggressive = false);

} // namespace util
} // namespace ir
} // namespace seq
