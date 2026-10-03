#pragma once

#include "compiler/ir.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

class IrBuilder {
 public:
  IrProgram Build(const CompilationUnit& unit, const SemanticModel& semantics,
                  bool return_last_expression = false);

 private:
  struct LoopContext {
    IrBlockId continue_target{kInvalidIrBlockId};
    IrBlockId break_target{kInvalidIrBlockId};
  };

  void BuildStatement(const Statement& statement,
                      const SemanticModel& semantics, IrProgram& program,
                      IrProcedure& procedure);
  IrValueId BuildExpression(const Expression& expression,
                            const SemanticModel& semantics, IrProgram& program,
                            IrProcedure& procedure);

  IrValueId AllocateValue(IrProcedure& procedure, Type type);

  IrValueId ConvertIfNeeded(IrValueId source, Type destination_type,
                            IrProgram& program);

  IrBlockId CreateBlock(IrProcedure& procedure);
  IrBasicBlock& GetBlock(IrProcedure& procedure, IrBlockId block);
  void SetCurrentBlock(IrBlockId block);
  IrBasicBlock& GetCurrentBlock(IrProcedure& procedure);
  bool IsBlockTerminated(const IrBasicBlock& block);

  void EmitInstruction(IrProcedure& procedure,
                       const IrInstruction& instruction);
  void EmitBooleanConstant(IrProgram& program, IrValueId destination,
                           bool value);
  void EmitIntegerConstant(IrProgram& program, IrValueId destination, u64 value,
                           Type type);
  void EmitNegatedIntegerConstant(IrProgram& program, IrValueId destination,
                                  u64 magnitude, Type type);
  void EmitFloatConstant(IrProgram& program, IrValueId destination, f64 value,
                         Type type);
  void EmitStringConstant(IrProgram& program, IrValueId destination,
                          StringView value);
  void EmitJump(IrProcedure& procedure, IrBlockId target);
  void EmitBranch(IrProcedure& procedure, IrValueId condition,
                  IrBlockId true_target, IrBlockId false_target);
  void EmitReturn(IrProcedure& procedure, IrValueId value);
  void EmitExit(IrProcedure& procedure);

  Vector<LoopContext> loop_stack_;
  IrBlockId current_block_{kInvalidIrBlockId};
};

}  // namespace fell
