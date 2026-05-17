#pragma once
#include <string>
#include <vector>
#include <memory>

// ============================================================
//  ABSTRACT SYNTAX TREE  (AST)
//
//  Every construct in the language maps to an ASTNode subclass.
//  We use std::unique_ptr for automatic, exception-safe memory
//  management so the caller never manually calls delete.
// ============================================================

struct ASTNode {
    virtual ~ASTNode() = default;
    // Human-readable type tag – used by the AST printer.
    virtual std::string nodeType() const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ────────────────────────────────────────────────────────────
//  EXPRESSIONS
// ────────────────────────────────────────────────────────────

// An integer literal:  42
struct NumberLiteral : ASTNode {
    int value;
    explicit NumberLiteral(int v) : value(v) {}
    std::string nodeType() const override {
        return "NumberLiteral(" + std::to_string(value) + ")";
    }
};

// A boolean literal:  true  /  false
struct BoolLiteral : ASTNode {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
    std::string nodeType() const override {
        return std::string("BoolLiteral(") + (value ? "true" : "false") + ")";
    }
};

// A variable reference:  myVar
struct Identifier : ASTNode {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
    std::string nodeType() const override { return "Identifier(" + name + ")"; }
};

// A binary expression:  left OP right
//   op is one of: + - * / == != < <= > >=
struct BinaryExpr : ASTNode {
    std::string op;
    ASTNodePtr  left, right;

    BinaryExpr(std::string op, ASTNodePtr l, ASTNodePtr r)
        : op(std::move(op)), left(std::move(l)), right(std::move(r)) {}

    std::string nodeType() const override { return "BinaryExpr(" + op + ")"; }
};

// A unary expression:  -expr
struct UnaryExpr : ASTNode {
    std::string op;       // currently only "-"
    ASTNodePtr  operand;

    UnaryExpr(std::string op, ASTNodePtr operand)
        : op(std::move(op)), operand(std::move(operand)) {}

    std::string nodeType() const override { return "UnaryExpr(" + op + ")"; }
};

// ────────────────────────────────────────────────────────────
//  STATEMENTS
// ────────────────────────────────────────────────────────────

// A list of statements enclosed in { }  (or the top-level program)
struct Block : ASTNode {
    std::vector<ASTNodePtr> statements;
    std::string nodeType() const override { return "Block"; }
};

// Variable declaration:  let x = expr;
struct LetStatement : ASTNode {
    std::string name;
    ASTNodePtr  initializer;

    LetStatement(std::string n, ASTNodePtr init)
        : name(std::move(n)), initializer(std::move(init)) {}

    std::string nodeType() const override { return "LetStatement(" + name + ")"; }
};

// Variable assignment:  x = expr;
//  (requires variable to already exist via let)
struct AssignStatement : ASTNode {
    std::string name;
    ASTNodePtr  value;

    AssignStatement(std::string n, ASTNodePtr v)
        : name(std::move(n)), value(std::move(v)) {}

    std::string nodeType() const override { return "AssignStatement(" + name + ")"; }
};

// Conditional:  if (cond) { then } else { else }
//  elseBranch may be nullptr when there is no else clause.
struct IfStatement : ASTNode {
    ASTNodePtr condition;
    ASTNodePtr thenBranch;
    ASTNodePtr elseBranch;   // nullptr if no else

    IfStatement(ASTNodePtr cond, ASTNodePtr then_, ASTNodePtr else_)
        : condition(std::move(cond)),
          thenBranch(std::move(then_)),
          elseBranch(std::move(else_)) {}

    std::string nodeType() const override { return "IfStatement"; }
};

// Loop:  while (cond) { body }
struct WhileStatement : ASTNode {
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStatement(ASTNodePtr cond, ASTNodePtr body)
        : condition(std::move(cond)), body(std::move(body)) {}

    std::string nodeType() const override { return "WhileStatement"; }
};

// Built-in output:  print expr;
struct PrintStatement : ASTNode {
    ASTNodePtr expr;
    explicit PrintStatement(ASTNodePtr e) : expr(std::move(e)) {}
    std::string nodeType() const override { return "PrintStatement"; }
};

// Built-in input:  input varName;
//  Reads an integer from stdin and stores it in varName.
struct InputStatement : ASTNode {
    std::string varName;
    explicit InputStatement(std::string n) : varName(std::move(n)) {}
    std::string nodeType() const override { return "InputStatement(" + varName + ")"; }
};