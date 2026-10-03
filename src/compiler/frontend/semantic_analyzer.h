#pragma once

#include "compiler/frontend/ast.h"
#include "compiler/frontend/diagnostic.h"
#include "compiler/frontend/type.h"
#include "compiler/native_function.h"

namespace fell {

enum class VariableStorage : u8 {
  kInvalid,

  kGlobal,
  kLocal,
};

struct VariableBinding {
  VariableStorage storage{VariableStorage::kInvalid};
  u32 slot{kMaxValue<u32>};
};

using FunctionId = u32;
inline constexpr FunctionId kInvalidFunctionId{static_cast<FunctionId>(-1)};

enum class FunctionStorage : u8 {
  kInvalid,

  kFell,
  kNative,
};

struct FunctionBinding {
  FunctionStorage storage{FunctionStorage::kInvalid};
  u32 slot{kMaxValue<u32>};
};

struct FunctionSemantics {
  StringView name;
  Vector<Type> parameter_types;
  Vector<u32> parameter_local_slots;
  Type return_type{Type::kInvalid};
  const Statement* declaration{nullptr};
};

struct ExpressionSemantics {
  Type type{Type::kInvalid};
  Type operand_type{Type::kInvalid};
  VariableBinding binding{};
  FunctionBinding function{};
};

struct StatementSemantics {
  Type type{Type::kInvalid};
  VariableBinding binding{};
  FunctionId function_id{kInvalidFunctionId};
};

class SemanticModel {
 public:
  [[nodiscard]] const ExpressionSemantics& Get(
      const Expression& expression) const;
  [[nodiscard]] const StatementSemantics& Get(const Statement& statement) const;
  [[nodiscard]] u32 global_count() const { return global_count_; }
  [[nodiscard]] u32 local_count() const { return local_count_; }
  [[nodiscard]] const Vector<FunctionSemantics>& functions() const {
    return functions_;
  }

 private:
  friend class SemanticAnalyzer;

  void Set(const Expression& expression, ExpressionSemantics semantics);
  void Set(const Statement& statement, StatementSemantics semantics);

  Vector<ExpressionSemantics> expressions_;
  Vector<StatementSemantics> statements_;
  u32 global_count_{0};
  u32 local_count_{0};
  Vector<FunctionSemantics> functions_;
};

struct SemanticResult {
  SemanticModel model;
  Vector<Diagnostic> diagnostics;
};

class SemanticAnalyzer {
 public:
  SemanticResult Analyze(const CompilationUnit& unit);

 private:
  struct Symbol {
    StringView name;
    Type type;
    VariableBinding binding;
    u32 scope_depth{0};
    bool is_mutable{false};
  };

  static Type GetIntegerLiteralType(const IntegerLiteralExpression& literal);
  static Type GetNegatedIntegerLiteralType(
      const IntegerLiteralExpression& literal);
  static Type GetFloatLiteralType(const FloatLiteralExpression& literal);
  static bool TryApplyIntegerLiteralContext(const Expression& expression,
                                            Type expected_type,
                                            SemanticResult& result);

  void Reset();
  void DeclareFunctions(const CompilationUnit& unit, SemanticResult& result);

  [[nodiscard]] const Symbol* FindSymbol(StringView name) const;
  [[nodiscard]] const Symbol* FindSymbolInCurrentScope(StringView name) const;
  [[nodiscard]] const FunctionSemantics* FindFunction(
      StringView name, FunctionId* id = nullptr) const;
  [[nodiscard]] const NativeFunctionDescriptor* FindNativeFunction(
      StringView name) const;

  void BeginScope();
  void EndScope();

  void AnalyzeStatement(const Statement& statement, SemanticResult& result);
  void AnalyzeFunctionDeclaration(const Statement& statement,
                                  SemanticResult& result);
  void AnalyzeExpression(const Expression& expression, SemanticResult& result);
  void AnalyzeCall(const Expression& expression, SemanticResult& result);
  bool AnalyzeCallArguments(const CallData& call,
                            const Vector<Type>& parameter_types,
                            bool accepts_any_value, SemanticResult& result);

  Vector<Symbol> symbols_;
  u32 scope_depth_{0};
  u32 loop_depth_{0};
  u32 next_global_id_{0};
  u32 next_local_id_{0};
  Type current_return_type_{Type::kInvalid};
  bool inside_function_{false};
  Vector<FunctionSemantics> functions_;
};

}  // namespace fell
