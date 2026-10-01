#include "compiler/semantic_analyzer.h"

#include "core/assert.h"
#include "core/type.h"

namespace fell {

const ExpressionSemantics& SemanticModel::Get(
    const Expression& expression) const {
  const usize index{expression.id.value};

  FELL_ASSERT(index < expressions_.size());
  FELL_ASSERT(expressions_[index].type != Type::kInvalid);

  return expressions_[index];
}

void SemanticModel::Set(const Expression& expression,
                        ExpressionSemantics semantics) {
  const usize index{expression.id.value};

  if (expressions_.size() <= index) {
    expressions_.resize(index + 1);
  }

  expressions_[index] = semantics;
}

const StatementSemantics& SemanticModel::Get(const Statement& statement) const {
  const usize index{statement.id.value};

  FELL_ASSERT(index < statements_.size());
  FELL_ASSERT(statements_[index].type != Type::kInvalid);

  return statements_[index];
}

void SemanticModel::Set(const Statement& statement,
                        StatementSemantics semantics) {
  const usize index{statement.id.value};

  if (statements_.size() <= index) {
    statements_.resize(index + 1);
  }

  statements_[index] = semantics;
}

void SemanticAnalyzer::Reset() {
  symbols_.clear();
  functions_.clear();

  scope_depth_ = 0;
  loop_depth_ = 0;

  next_global_id_ = 0;
  next_local_id_ = 0;

  current_return_type_ = Type::kInvalid;
  inside_function_ = false;
}

void SemanticAnalyzer::DeclareFunctions(const CompilationUnit& unit,
                                        SemanticResult& result) {
  for (const Statement* statement : unit.statements) {
    if (statement->kind != StatementKind::kFunctionDeclaration) {
      continue;
    }

    const FunctionData& function{*statement->function_declaration.data};

    if (FindFunction(function.name) != nullptr ||
        FindNativeFunction(function.name) != nullptr) {
      result.diagnostics.push_back({
          .severity = DiagnosticSeverity::kError,
          .message = "function is already declared",
          .span = statement->span,
      });

      continue;
    }

    Vector<Type> parameter_types;
    parameter_types.reserve(function.parameters.size());

    for (const FunctionParameter& parameter : function.parameters) {
      parameter_types.push_back(parameter.type);
    }

    functions_.push_back({
        .name = function.name,
        .parameter_types = std::move(parameter_types),
        .parameter_local_slots = {},
        .return_type = function.return_type,
        .declaration = statement,
    });
  }
}

const SemanticAnalyzer::Symbol* SemanticAnalyzer::FindSymbol(
    StringView name) const {
  for (auto iterator{symbols_.rbegin()}; iterator != symbols_.rend();
       ++iterator) {
    if (iterator->name == name) {
      return &*iterator;
    }
  }

  return nullptr;
}

const SemanticAnalyzer::Symbol* SemanticAnalyzer::FindSymbolInCurrentScope(
    StringView name) const {
  for (auto iterator{symbols_.rbegin()}; iterator != symbols_.rend();
       ++iterator) {
    if (iterator->scope_depth < scope_depth_) {
      break;
    }

    if (iterator->name == name) {
      return &*iterator;
    }
  }

  return nullptr;
}

const FunctionSemantics* SemanticAnalyzer::FindFunction(StringView name,
                                                        FunctionId* id) const {
  for (usize index{0}; index < functions_.size(); ++index) {
    if (functions_[index].name == name) {
      if (id != nullptr) {
        *id = static_cast<FunctionId>(index);
      }

      return &functions_[index];
    }
  }

  return nullptr;
}

const NativeFunctionDescriptor* SemanticAnalyzer::FindNativeFunction(
    StringView name) const {
  for (const NativeFunctionDescriptor& function :
       GetNativeFunctionDescriptors()) {
    if (function.name == name) {
      return &function;
    }
  }

  return nullptr;
}

void SemanticAnalyzer::BeginScope() { ++scope_depth_; }

void SemanticAnalyzer::EndScope() {
  FELL_ASSERT(scope_depth_ > 0);

  while (!symbols_.empty() && symbols_.back().scope_depth == scope_depth_) {
    symbols_.pop_back();
  }

  --scope_depth_;
}

SemanticResult SemanticAnalyzer::Analyze(const CompilationUnit& unit) {
  Reset();

  SemanticResult result{};

  DeclareFunctions(unit, result);

  for (const Statement* statement : unit.statements) {
    AnalyzeStatement(*statement, result);
  }

  result.model.functions_ = functions_;
  result.model.global_count_ = next_global_id_;
  result.model.local_count_ = next_local_id_;

  return result;
}

Type SemanticAnalyzer::GetIntegerLiteralType(
    const IntegerLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    if (!CanRepresentInteger(literal.explicit_type, literal.value)) {
      return Type::kInvalid;
    }

    return literal.explicit_type;
  }

  if (CanRepresentInteger(Type::kS32, literal.value)) {
    return Type::kS32;
  }

  if (CanRepresentInteger(Type::kS64, literal.value)) {
    return Type::kS64;
  }

  return Type::kInvalid;
}

Type SemanticAnalyzer::GetNegatedIntegerLiteralType(
    const IntegerLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    if (!IsSignedInteger(literal.explicit_type) ||
        !CanRepresentNegativeInteger(literal.explicit_type, literal.value)) {
      return Type::kInvalid;
    }

    return literal.explicit_type;
  }

  if (CanRepresentNegativeInteger(Type::kS32, literal.value)) {
    return Type::kS32;
  }

  if (CanRepresentNegativeInteger(Type::kS64, literal.value)) {
    return Type::kS64;
  }

  return Type::kInvalid;
}

Type SemanticAnalyzer::GetFloatLiteralType(
    const FloatLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    FELL_ASSERT(literal.explicit_type == Type::kF32 ||
                literal.explicit_type == Type::kF64);

    return literal.explicit_type;
  }

  return Type::kF64;
}

bool SemanticAnalyzer::TryApplyIntegerLiteralContext(
    const Expression& expression, Type expected_type, SemanticResult& result) {
  if (!IsSignedInteger(expected_type) && !IsUnsignedInteger(expected_type)) {
    return false;
  }

  if (expression.kind == ExpressionKind::kIntegerLiteral) {
    const auto& literal{expression.integer_literal};

    if (literal.explicit_type != Type::kInvalid ||
        !CanRepresentInteger(expected_type, literal.value)) {
      return false;
    }

    result.model.Set(expression, {
                                     .type = expected_type,
                                 });

    return true;
  }

  if (expression.kind != ExpressionKind::kUnary ||
      expression.unary.op != UnaryOperator::kNegate ||
      expression.unary.operand->kind != ExpressionKind::kIntegerLiteral ||
      !IsSignedInteger(expected_type)) {
    return false;
  }

  const Expression& operand{*expression.unary.operand};
  const auto& literal{operand.integer_literal};

  if (literal.explicit_type != Type::kInvalid ||
      !CanRepresentNegativeInteger(expected_type, literal.value)) {
    return false;
  }

  // The positive magnitude may not fit in the signed type even though the
  // corresponding negative value does, such as -128s8.
  result.model.Set(operand, {
                                .type = expected_type,
                            });

  result.model.Set(expression, {
                                   .type = expected_type,
                               });

  return true;
}

void SemanticAnalyzer::AnalyzeFunctionDeclaration(const Statement& statement,
                                                  SemanticResult& result) {
  const FunctionData& function{*statement.function_declaration.data};

  FunctionId function_id{kInvalidFunctionId};

  if (FindFunction(function.name, &function_id) == nullptr) {
    result.model.Set(statement, {
                                    .type = Type::kError,
                                });

    return;
  }

  const usize symbol_count{symbols_.size()};
  const u32 saved_scope_depth{scope_depth_};
  const u32 saved_loop_depth{loop_depth_};
  const Type saved_return_type{current_return_type_};
  const bool saved_inside_function{inside_function_};

  scope_depth_ = 1;
  loop_depth_ = 0;
  current_return_type_ = function.return_type;
  inside_function_ = true;

  bool error{false};

  for (const FunctionParameter& parameter : function.parameters) {
    if (FindSymbolInCurrentScope(parameter.name) != nullptr) {
      result.diagnostics.push_back({
          .severity = DiagnosticSeverity::kError,
          .message = "duplicate parameter name",
          .span = statement.span,
      });

      error = true;

      continue;
    }

    const VariableBinding binding{
        .storage = VariableStorage::kLocal,
        .slot = next_local_id_++,
    };

    functions_[function_id].parameter_local_slots.push_back(binding.slot);

    symbols_.push_back({
        .name = parameter.name,
        .type = parameter.type,
        .binding = binding,
        .scope_depth = scope_depth_,
        .is_mutable = false,
    });
  }

  AnalyzeExpression(*function.body, result);

  error = error || result.model.Get(*function.body).type == Type::kError;

  const CompilationUnit& body_unit{*function.body->block.body};

  if (body_unit.statements.empty() ||
      body_unit.statements.back()->kind != StatementKind::kReturn) {
    result.diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "function body must end with return",
        .span = function.body->span,
    });

    error = true;
  }

  symbols_.resize(symbol_count);

  scope_depth_ = saved_scope_depth;
  loop_depth_ = saved_loop_depth;
  current_return_type_ = saved_return_type;
  inside_function_ = saved_inside_function;

  result.model.Set(statement, {
                                  .type = error ? Type::kError : Type::kUnit,
                                  .function_id = function_id,
                              });
}

void SemanticAnalyzer::AnalyzeStatement(const Statement& statement,
                                        SemanticResult& result) {
  switch (statement.kind) {
    case StatementKind::kInvalid:
      FELL_UNREACHABLE();

    case StatementKind::kExpression:
      AnalyzeExpression(*statement.expression.expression, result);

      result.model.Set(
          statement,
          {
              .type = result.model.Get(*statement.expression.expression).type,
          });

      return;

    case StatementKind::kVariableDeclaration: {
      const auto& declaration{statement.variable_declaration};

      if (FindSymbolInCurrentScope(declaration.name) != nullptr) {
        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "variable is already declared",
            .span = statement.span,
        });

        return;
      }

      if (declaration.explicit_type == Type::kInvalid ||
          !TryApplyIntegerLiteralContext(*declaration.initializer,
                                         declaration.explicit_type, result)) {
        AnalyzeExpression(*declaration.initializer, result);
      }

      const Type initializer_type{
          result.model.Get(*declaration.initializer).type};

      if (initializer_type == Type::kError) {
        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });

        return;
      }

      if (initializer_type == Type::kUnit) {
        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "variable initializer must produce a value",
            .span = statement.span,
        });

        return;
      }

      Type type{initializer_type};

      if (declaration.explicit_type != Type::kInvalid) {
        type = declaration.explicit_type;

        if (initializer_type != type &&
            (!IsNumericType(initializer_type) || !IsNumericType(type) ||
             !CanImplicitlyConvert(initializer_type, type))) {
          result.model.Set(statement, {
                                          .type = Type::kError,
                                      });

          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message =
                  "initializer is not implicitly convertible to declared type",
              .span = statement.span,
          });

          return;
        }
      }

      VariableBinding binding{};

      if (scope_depth_ == 0) {
        FELL_ASSERT(next_global_id_ < kMaxValue<u32>);

        binding = {
            .storage = VariableStorage::kGlobal,
            .slot = next_global_id_++,
        };
      } else {
        FELL_ASSERT(next_local_id_ < kMaxValue<u32>);

        binding = {
            .storage = VariableStorage::kLocal,
            .slot = next_local_id_++,
        };
      }

      symbols_.push_back({
          .name = declaration.name,
          .type = type,
          .binding = binding,
          .scope_depth = scope_depth_,
          .is_mutable = declaration.is_mutable,
      });

      result.model.Set(statement, {
                                      .type = type,
                                      .binding = binding,
                                  });

      return;
    }

    case StatementKind::kFunctionDeclaration:
      AnalyzeFunctionDeclaration(statement, result);
      return;

    case StatementKind::kReturn: {
      if (!inside_function_) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "'return' is only valid inside a function",
            .span = statement.span,
        });

        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });

        return;
      }

      if (!TryApplyIntegerLiteralContext(*statement.return_.value,
                                         current_return_type_, result)) {
        AnalyzeExpression(*statement.return_.value, result);
      }

      const Type value_type{result.model.Get(*statement.return_.value).type};

      if (value_type != current_return_type_ &&
          (!IsNumericType(value_type) || !IsNumericType(current_return_type_) ||
           !CanImplicitlyConvert(value_type, current_return_type_))) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "return value is not implicitly convertible to "
                       "function return type",
            .span = statement.span,
        });

        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });

        return;
      }

      result.model.Set(statement, {
                                      .type = current_return_type_,
                                  });

      return;
    }

    case StatementKind::kIf: {
      AnalyzeExpression(*statement.if_.condition, result);

      const Type condition_type{
          result.model.Get(*statement.if_.condition).type};

      if (condition_type != Type::kBool && condition_type != Type::kError) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "if condition must be bool",
            .span = statement.if_.condition->span,
        });
      }

      AnalyzeExpression(*statement.if_.then_block, result);

      if (statement.if_.else_block != nullptr) {
        AnalyzeExpression(*statement.if_.else_block, result);
      }

      const bool error{
          condition_type == Type::kError || condition_type != Type::kBool ||
          result.model.Get(*statement.if_.then_block).type == Type::kError ||
          (statement.if_.else_block != nullptr &&
           result.model.Get(*statement.if_.else_block).type == Type::kError)};

      result.model.Set(statement,
                       {
                           .type = error ? Type::kError : Type::kUnit,
                       });

      return;
    }

    case StatementKind::kWhile: {
      AnalyzeExpression(*statement.while_.condition, result);

      const Type condition_type{
          result.model.Get(*statement.while_.condition).type};

      if (condition_type != Type::kBool && condition_type != Type::kError) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "while condition must be bool",
            .span = statement.while_.condition->span,
        });
      }

      ++loop_depth_;

      AnalyzeExpression(*statement.while_.body, result);

      --loop_depth_;

      const bool error{
          condition_type == Type::kError || condition_type != Type::kBool ||
          result.model.Get(*statement.while_.body).type == Type::kError};

      result.model.Set(statement,
                       {
                           .type = error ? Type::kError : Type::kUnit,
                       });

      return;
    }

    case StatementKind::kFor: {
      BeginScope();

      bool error{false};

      if (statement.for_.initializer != nullptr) {
        AnalyzeStatement(*statement.for_.initializer, result);

        error =
            result.model.Get(*statement.for_.initializer).type == Type::kError;
      }

      if (statement.for_.condition != nullptr) {
        AnalyzeExpression(*statement.for_.condition, result);

        const Type condition_type{
            result.model.Get(*statement.for_.condition).type};

        if (condition_type != Type::kBool && condition_type != Type::kError) {
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "for condition must be bool",
              .span = statement.for_.condition->span,
          });

          error = true;
        } else if (condition_type == Type::kError) {
          error = true;
        }
      }

      if (statement.for_.increment != nullptr) {
        AnalyzeExpression(*statement.for_.increment, result);

        error = error || result.model.Get(*statement.for_.increment).type ==
                             Type::kError;
      }

      ++loop_depth_;

      AnalyzeExpression(*statement.for_.body, result);

      --loop_depth_;

      error =
          error || result.model.Get(*statement.for_.body).type == Type::kError;

      EndScope();

      result.model.Set(statement,
                       {
                           .type = error ? Type::kError : Type::kUnit,
                       });

      return;
    }

    case StatementKind::kBreak:
      if (loop_depth_ == 0) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "'break' is only valid inside a loop",
            .span = statement.span,
        });

        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });
      } else {
        result.model.Set(statement, {
                                        .type = Type::kUnit,
                                    });
      }

      return;

    case StatementKind::kContinue:
      if (loop_depth_ == 0) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "'continue' is only valid inside a loop",
            .span = statement.span,
        });

        result.model.Set(statement, {
                                        .type = Type::kError,
                                    });
      } else {
        result.model.Set(statement, {
                                        .type = Type::kUnit,
                                    });
      }

      return;

    case StatementKind::kSwitch: {
      const SwitchData& data{*statement.switch_.data};

      AnalyzeExpression(*data.value, result);

      const Type switch_type{result.model.Get(*data.value).type};
      bool error{switch_type == Type::kError || switch_type == Type::kUnit};

      for (const SwitchCase& case_ : data.cases) {
        if (!TryApplyIntegerLiteralContext(*case_.value, switch_type, result)) {
          AnalyzeExpression(*case_.value, result);
        }

        const Type case_type{result.model.Get(*case_.value).type};

        if (case_type != Type::kError && case_type != switch_type &&
            (!IsNumericType(case_type) || !IsNumericType(switch_type) ||
             !CanImplicitlyConvert(case_type, switch_type))) {
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "switch case is not implicitly convertible to switch "
                         "value type",
              .span = case_.value->span,
          });

          error = true;
        }

        error = error || case_type == Type::kError;

        AnalyzeExpression(*case_.body, result);

        error = error || result.model.Get(*case_.body).type == Type::kError;
      }

      if (data.default_body != nullptr) {
        AnalyzeExpression(*data.default_body, result);

        error =
            error || result.model.Get(*data.default_body).type == Type::kError;
      }

      result.model.Set(statement,
                       {
                           .type = error ? Type::kError : Type::kUnit,
                       });

      return;
    }
  }

  FELL_UNREACHABLE();
}

bool SemanticAnalyzer::AnalyzeCallArguments(const CallData& call,
                                            const Vector<Type>& parameter_types,
                                            bool accepts_any_value,
                                            SemanticResult& result) {
  if (call.arguments.size() != parameter_types.size()) {
    return false;
  }

  bool valid{true};

  for (usize index{0}; index < call.arguments.size(); ++index) {
    Expression& argument{*call.arguments[index]};
    const Type expected{parameter_types[index]};

    if (!accepts_any_value &&
        TryApplyIntegerLiteralContext(argument, expected, result)) {
      continue;
    }

    AnalyzeExpression(argument, result);

    const Type actual{result.model.Get(argument).type};

    if (actual == Type::kError) {
      valid = false;

      continue;
    }

    if (accepts_any_value) {
      if (actual == Type::kUnit) {
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "argument must produce a value",
            .span = argument.span,
        });

        valid = false;
      }

      continue;
    }

    if (actual != expected &&
        (!IsNumericType(actual) || !IsNumericType(expected) ||
         !CanImplicitlyConvert(actual, expected))) {
      result.diagnostics.push_back({
          .severity = DiagnosticSeverity::kError,
          .message = "argument is not implicitly convertible to parameter type",
          .span = argument.span,
      });

      valid = false;
    }
  }

  return valid;
}

void SemanticAnalyzer::AnalyzeCall(const Expression& expression,
                                   SemanticResult& result) {
  const CallData& call{*expression.call.data};

  if (const NativeFunctionDescriptor* const function{
          FindNativeFunction(call.callee)};
      function != nullptr) {
    if (call.arguments.size() != function->parameter_types.size()) {
      result.model.Set(expression, {
                                       .type = Type::kError,
                                   });

      result.diagnostics.push_back({
          .severity = DiagnosticSeverity::kError,
          .message = "incorrect argument count",
          .span = expression.span,
      });

      return;
    }

    const bool valid{AnalyzeCallArguments(call, function->parameter_types,
                                          function->accepts_any_value, result)};

    result.model.Set(expression,
                     {
                         .type = valid ? function->return_type : Type::kError,
                         .function =
                             {
                                 .storage = FunctionStorage::kNative,
                                 .slot = function->id,
                             },
                     });

    return;
  }

  FunctionId function_id{kInvalidFunctionId};

  const FunctionSemantics* const function{
      FindFunction(call.callee, &function_id)};

  if (function == nullptr) {
    result.model.Set(expression, {
                                     .type = Type::kError,
                                 });

    result.diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "undefined function",
        .span = expression.span,
    });

    return;
  }

  if (call.arguments.size() != function->parameter_types.size()) {
    result.model.Set(expression, {
                                     .type = Type::kError,
                                 });

    result.diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "incorrect argument count",
        .span = expression.span,
    });

    return;
  }

  const bool valid{
      AnalyzeCallArguments(call, function->parameter_types, false, result)};

  result.model.Set(expression,
                   {
                       .type = valid ? function->return_type : Type::kError,
                       .function =
                           {
                               .storage = FunctionStorage::kFell,
                               .slot = function_id,
                           },
                   });
}

void SemanticAnalyzer::AnalyzeExpression(const Expression& expression,
                                         SemanticResult& result) {
  switch (expression.kind) {
    case ExpressionKind::kInvalid:
      FELL_UNREACHABLE();

    case ExpressionKind::kBooleanLiteral:
      result.model.Set(expression, {
                                       .type = Type::kBool,
                                   });

      return;

    case ExpressionKind::kIntegerLiteral: {
      const Type type{GetIntegerLiteralType(expression.integer_literal)};

      if (type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "integer literal is out of range",
            .span = expression.span,
        });

        return;
      }

      result.model.Set(expression, {
                                       .type = type,
                                   });

      return;
    }

    case ExpressionKind::kFloatLiteral: {
      const Type type{GetFloatLiteralType(expression.float_literal)};

      if (type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "float literal is out of range",
            .span = expression.span,
        });

        return;
      }

      result.model.Set(expression, {
                                       .type = type,
                                   });

      return;
    }

    case ExpressionKind::kStringLiteral:
      result.model.Set(expression, {
                                       .type = Type::kString,
                                   });

      return;

    case ExpressionKind::kVariable: {
      const Symbol* const symbol{FindSymbol(expression.variable.name)};

      if (symbol == nullptr) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "undefined variable",
            .span = expression.span,
        });

        return;
      }

      result.model.Set(expression, {
                                       .type = symbol->type,
                                       .binding = symbol->binding,
                                   });

      return;
    }

    case ExpressionKind::kAssignment: {
      const Symbol* const symbol{FindSymbol(expression.assignment.name)};

      if (symbol == nullptr) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "undefined variable",
            .span = expression.span,
        });

        return;
      }

      if (!symbol->is_mutable) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "cannot assign to immutable variable",
            .span = expression.span,
        });

        return;
      }

      AnalyzeExpression(*expression.assignment.value, result);

      const Type value_type{
          result.model.Get(*expression.assignment.value).type};

      if (value_type == Type::kError) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        return;
      }

      if (value_type != symbol->type &&
          (!IsNumericType(value_type) || !IsNumericType(symbol->type) ||
           !CanImplicitlyConvert(value_type, symbol->type))) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message =
                "assigned value is not implicitly convertible to variable type",
            .span = expression.span,
        });

        return;
      }

      result.model.Set(expression, {
                                       .type = symbol->type,
                                       .operand_type = symbol->type,
                                       .binding = symbol->binding,
                                   });

      return;
    }

    case ExpressionKind::kCall:
      AnalyzeCall(expression, result);
      return;

    case ExpressionKind::kConditional: {
      const auto& conditional{expression.conditional};

      AnalyzeExpression(*conditional.condition, result);
      AnalyzeExpression(*conditional.then_expression, result);
      AnalyzeExpression(*conditional.else_expression, result);

      const Type condition_type{result.model.Get(*conditional.condition).type};
      Type then_type{result.model.Get(*conditional.then_expression).type};
      Type else_type{result.model.Get(*conditional.else_expression).type};

      if (condition_type != Type::kBool || then_type == Type::kError ||
          else_type == Type::kError || then_type == Type::kUnit ||
          else_type == Type::kUnit) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        if (condition_type != Type::kBool && condition_type != Type::kError) {
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "conditional condition must be bool",
              .span = conditional.condition->span,
          });
        } else if (then_type != Type::kError && else_type != Type::kError) {
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "conditional branches must produce values",
              .span = expression.span,
          });
        }

        return;
      }

      Type result_type{then_type};

      if (then_type != else_type) {
        if (!IsNumericType(then_type) || !IsNumericType(else_type) ||
            (result_type = FindCommonNumericType(then_type, else_type)) ==
                Type::kInvalid) {
          result.model.Set(expression, {
                                           .type = Type::kError,
                                       });

          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "conditional branches have incompatible types",
              .span = expression.span,
          });

          return;
        }
      }

      result.model.Set(expression, {
                                       .type = result_type,
                                       .operand_type = result_type,
                                   });

      return;
    }

    case ExpressionKind::kBlock: {
      BeginScope();

      bool has_error{false};

      for (const Statement* statement : expression.block.body->statements) {
        AnalyzeStatement(*statement, result);

        if (result.model.Get(*statement).type == Type::kError) {
          has_error = true;
        }
      }

      Type type{Type::kUnit};

      if (expression.block.trailing_expression != nullptr) {
        AnalyzeExpression(*expression.block.trailing_expression, result);

        type = result.model.Get(*expression.block.trailing_expression).type;
        has_error = has_error || type == Type::kError;
      }

      EndScope();

      result.model.Set(expression, {
                                       .type = has_error ? Type::kError : type,
                                   });

      return;
    }

    case ExpressionKind::kUnary: {
      const auto& unary{expression.unary};

      if (unary.op == UnaryOperator::kNegate &&
          unary.operand->kind == ExpressionKind::kIntegerLiteral &&
          (unary.operand->integer_literal.explicit_type == Type::kInvalid ||
           IsSignedInteger(unary.operand->integer_literal.explicit_type))) {
        const Type type{
            GetNegatedIntegerLiteralType(unary.operand->integer_literal),
        };

        if (type == Type::kInvalid) {
          result.model.Set(*unary.operand, {
                                               .type = Type::kError,
                                           });

          result.model.Set(expression, {
                                           .type = Type::kError,
                                       });

          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "integer literal is out of range",
              .span = expression.span,
          });

          return;
        }

        // The literal's type is contextual here: its positive magnitude may
        // not be representable by the type, but the negated value is.
        result.model.Set(*unary.operand, {
                                             .type = type,
                                         });

        result.model.Set(expression, {
                                         .type = type,
                                     });

        return;
      }

      AnalyzeExpression(*unary.operand, result);

      const auto& operand{result.model.Get(*unary.operand)};

      if (operand.type == Type::kError) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        return;
      }

      switch (unary.op) {
        case UnaryOperator::kInvalid:
          FELL_UNREACHABLE();

        case UnaryOperator::kNegate:
          if (!IsSignedInteger(operand.type) &&
              !IsFloatingPoint(operand.type)) {
            result.model.Set(expression, {
                                             .type = Type::kError,
                                         });

            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::kError,
                .message = "negation only applies to signed integers and "
                           "floating-point types",
                .span = expression.span,
            });

            return;
          }

          break;

        case UnaryOperator::kLogicalNot:
          if (operand.type != Type::kBool) {
            result.model.Set(expression, {
                                             .type = Type::kError,
                                         });

            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::kError,
                .message = "logical not only applies to bool",
                .span = expression.span,
            });

            return;
          }

          break;
      }

      result.model.Set(expression, {
                                       .type = operand.type,
                                   });

      return;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};

      AnalyzeExpression(*binary.left, result);
      AnalyzeExpression(*binary.right, result);

      const auto& left{result.model.Get(*binary.left)};
      const auto& right{result.model.Get(*binary.right)};

      if (left.type == Type::kError || right.type == Type::kError) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        return;
      }

      const bool is_logical{binary.op == BinaryOperator::kLogicalAnd ||
                            binary.op == BinaryOperator::kLogicalOr};

      if (is_logical) {
        if (left.type != Type::kBool || right.type != Type::kBool) {
          result.model.Set(expression, {
                                           .type = Type::kError,
                                       });

          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "logical operators require bool operands",
              .span = expression.span,
          });

          return;
        }

        result.model.Set(expression, {
                                         .type = Type::kBool,
                                         .operand_type = Type::kBool,
                                     });

        return;
      }

      const bool is_arithmetic{binary.op == BinaryOperator::kMultiply ||
                               binary.op == BinaryOperator::kDivide ||
                               binary.op == BinaryOperator::kAdd ||
                               binary.op == BinaryOperator::kSubtract};

      const bool is_equality{binary.op == BinaryOperator::kEqual ||
                             binary.op == BinaryOperator::kNotEqual};

      if (is_equality && left.type == Type::kBool &&
          right.type == Type::kBool) {
        result.model.Set(expression, {
                                         .type = Type::kBool,
                                         .operand_type = Type::kBool,
                                     });

        return;
      }

      if (binary.op == BinaryOperator::kAdd && left.type == Type::kString &&
          right.type == Type::kString) {
        result.model.Set(expression, {
                                         .type = Type::kString,
                                         .operand_type = Type::kString,
                                     });

        return;
      }

      if (is_equality && left.type == Type::kString &&
          right.type == Type::kString) {
        result.model.Set(expression, {
                                         .type = Type::kBool,
                                         .operand_type = Type::kString,
                                     });

        return;
      }

      if (!IsNumericType(left.type) || !IsNumericType(right.type)) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message =
                is_arithmetic
                    ? "arithmetic operators only apply to numeric types"
                : is_equality
                    ? "equality operands must both be bool, string, or numeric"
                    : "ordering operators only apply to numeric types",
            .span = expression.span,
        });

        return;
      }

      const Type common_type{FindCommonNumericType(left.type, right.type)};

      if (common_type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "numeric operands have no lossless common type",
            .span = expression.span,
        });

        return;
      }

      result.model.Set(expression,
                       {
                           .type = is_arithmetic ? common_type : Type::kBool,
                           .operand_type = common_type,
                       });

      return;
    }
  }

  FELL_UNREACHABLE();
}

}  // namespace fell