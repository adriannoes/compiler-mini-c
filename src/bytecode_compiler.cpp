#include "bytecode_compiler.hpp"

#include "type_checker.hpp"

#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace minic {
namespace {

OpCode binary_opcode(TokenKind token) {
  switch (token) {
    case TokenKind::Plus: return OpCode::Add;
    case TokenKind::Minus: return OpCode::Subtract;
    case TokenKind::Star: return OpCode::Multiply;
    case TokenKind::Slash: return OpCode::Divide;
    case TokenKind::Percent: return OpCode::Remainder;
    case TokenKind::EqualEqual: return OpCode::Equal;
    case TokenKind::BangEqual: return OpCode::NotEqual;
    case TokenKind::Less: return OpCode::Less;
    case TokenKind::LessEqual: return OpCode::LessEqual;
    case TokenKind::Greater: return OpCode::Greater;
    case TokenKind::GreaterEqual: return OpCode::GreaterEqual;
    default: throw std::logic_error("The typed binary operator has no bytecode instruction.");
  }
}

class BytecodeCompiler {
 public:
  explicit BytecodeCompiler(const Program& program) : program_(program), analysis_(check_types(program)) {
    const auto& symbols = analysis_.resolution().table().symbols();
    for (std::size_t id = 0; id < symbols.size(); ++id) {
      if (const auto* variable = std::get_if<const VarDecl*>(&symbols[id].declaration)) {
        declarations_.emplace(*variable, id);
      } else if (const auto* parameter = std::get_if<const Parameter*>(&symbols[id].declaration)) {
        parameters_.emplace(*parameter, id);
      }
    }
    result_.functions.reserve(program.functions.size());
    for (const auto& function : program.functions) {
      functions_.emplace(&function, result_.functions.size());
      result_.functions.push_back({function.name, detail::syntax_type(function.return_type),
                                   function.parameters.size(), function.location, {}, {}, {}});
    }
  }

  BytecodeProgram compile() {
    for (std::size_t index = 0; index < program_.functions.size(); ++index) {
      const auto& source = program_.functions[index];
      current_ = &result_.functions[index];
      slots_.clear();
      for (const auto& parameter : source.parameters) {
        local(parameters_.at(&parameter), parameter.name, parameter.type, parameter.location);
      }
      emit(OpCode::Step, source.body.location);
      block_contents(source.body);
      emit(current_->return_type == Type::Void ? OpCode::ReturnVoid : OpCode::Unreachable, source.body.location);
    }
    return std::move(result_);
  }

 private:
  std::size_t emit(OpCode opcode, SourceLocation location, std::size_t operand = 0) {
    const auto offset = current_->code.size();
    current_->code.push_back({opcode, operand, location});
    return offset;
  }

  void patch(std::size_t offset) { current_->code.at(offset).operand = current_->code.size(); }

  std::size_t local(std::size_t symbol, const std::string& name, const TypeSyntax& type, SourceLocation location) {
    const auto index = current_->locals.size();
    slots_.emplace(symbol, index);
    current_->locals.push_back({name, detail::value_type(type), location});
    return index;
  }

  std::size_t slot(const Expr& source) const {
    return slots_.at(analysis_.resolution().binding(source)->symbol.index);
  }

  void constant(BytecodeConstant value, SourceLocation location) {
    const auto index = current_->constants.size();
    current_->constants.push_back(std::move(value));
    emit(OpCode::LoadConstant, location, index);
  }

  void expression(const Expr& source) {
    emit(OpCode::Step, source.location);
    std::visit([&](const auto& node) { compile(node, source); }, source.node);
  }

  void compile(const IntegerExpr& node, const Expr& source) {
    constant(IntegerConstant{node.text, false}, source.location);
  }

  void compile(const BooleanExpr& node, const Expr& source) { constant(node.value, source.location); }

  void compile(const NameExpr&, const Expr& source) { emit(OpCode::LoadLocal, source.location, slot(source)); }

  void compile(const IndexExpr& node, const Expr& source) {
    expression(*node.index);
    emit(OpCode::LoadElement, source.location, slot(source));
  }

  void compile(const UnaryExpr& node, const Expr& source) {
    if (node.op == TokenKind::Minus) {
      if (const auto* literal = std::get_if<IntegerExpr>(&node.operand->node)) {
        emit(OpCode::Step, node.operand->location);
        constant(IntegerConstant{literal->text, true}, source.location);
        return;
      }
    }
    expression(*node.operand);
    emit(node.op == TokenKind::Minus ? OpCode::Negate : OpCode::Not, source.location);
  }

  void compile(const BinaryExpr& node, const Expr& source) {
    expression(*node.left);
    if (node.op == TokenKind::AndAnd || node.op == TokenKind::OrOr) {
      const bool disjunction = node.op == TokenKind::OrOr;
      const auto selected = emit(disjunction ? OpCode::JumpIfTrue : OpCode::JumpIfFalse, source.location);
      expression(*node.right);
      const auto end = emit(OpCode::Jump, source.location);
      patch(selected);
      constant(disjunction, source.location);
      patch(end);
      return;
    }
    expression(*node.right);
    emit(binary_opcode(node.op), source.location);
  }

  void compile(const AssignmentExpr& node, const Expr& source) {
    if (const auto* index = std::get_if<IndexExpr>(&node.target->node)) {
      emit(OpCode::Step, node.target->location);
      expression(*index->index);
      emit(OpCode::CheckIndex, node.target->location, slot(*node.target));
      expression(*node.value);
      emit(OpCode::StoreElement, node.target->location, slot(*node.target));
      return;
    }
    expression(*node.value);
    emit(OpCode::StoreLocal, source.location, slot(*node.target));
  }

  void compile(const CallExpr& node, const Expr& source) {
    const auto id = analysis_.resolution().binding(source)->symbol;
    const auto& symbol = analysis_.resolution().table().symbol(id);
    const auto function = functions_.at(std::get<const FunctionDecl*>(symbol.declaration));
    for (const auto& argument : node.arguments) {
      expression(*argument);
    }
    emit(OpCode::Call, source.location, function);
  }

  void statement(const Stmt& source) {
    std::visit([&](const auto& node) {
      emit(OpCode::Step, node.location);
      compile(node);
    }, source.node);
  }

  void block_contents(const BlockStmt& node) {
    for (const auto& child : node.statements) {
      statement(*child);
    }
  }

  void compile(const BlockStmt& node) { block_contents(node); }

  void compile(const VarDecl& node) {
    const auto index = local(declarations_.at(&node), node.name, node.type, node.location);
    current_->locals[index].array_length = analysis_.array_length(node).value_or(0);
    emit(OpCode::ResetLocal, node.location, index);
    if (node.initializer) {
      expression(*node.initializer);
      emit(OpCode::StoreLocal, node.location, index);
      emit(OpCode::Pop, node.location);
    }
  }

  void compile(const ExprStmt& node) {
    expression(*node.expression);
    if (analysis_.type_of(*node.expression).value() != Type::Void) {
      emit(OpCode::Pop, node.location);
    }
  }

  void compile(const EmptyStmt&) {}

  void compile(const ReturnStmt& node) {
    if (node.value) {
      expression(*node.value);
    }
    emit(node.value ? OpCode::Return : OpCode::ReturnVoid, node.location);
  }

  void compile(const IfStmt& node) {
    expression(*node.condition);
    const auto alternate = emit(OpCode::JumpIfFalse, node.location);
    statement(*node.then_branch);
    if (node.else_branch) {
      const auto end = emit(OpCode::Jump, node.location);
      patch(alternate);
      statement(*node.else_branch);
      patch(end);
    } else {
      patch(alternate);
    }
  }

  void compile(const WhileStmt& node) {
    const auto start = current_->code.size();
    expression(*node.condition);
    const auto end = emit(OpCode::JumpIfFalse, node.location);
    statement(*node.body);
    emit(OpCode::Jump, node.location, start);
    patch(end);
  }

  const Program& program_;
  TypeAnalysis analysis_;
  BytecodeProgram result_;
  BytecodeFunction* current_ = nullptr;
  std::unordered_map<const FunctionDecl*, std::size_t> functions_;
  std::unordered_map<const VarDecl*, std::size_t> declarations_;
  std::unordered_map<const Parameter*, std::size_t> parameters_;
  std::unordered_map<std::size_t, std::size_t> slots_;
};

}  // namespace

BytecodeProgram compile_bytecode(const Program& program) { return BytecodeCompiler(program).compile(); }

}  // namespace minic
