#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <stdexcept>

// ============================================================
//  PARSER  – Recursive Descent
//
//  Turns a flat token stream into a hierarchical AST.
//  Each grammar rule maps to exactly one private method.
//
//  Grammar (informal):
//
//  program      → statement* EOF
//  statement    → letStmt | ifStmt | whileStmt
//               | printStmt | inputStmt | block
//               | assignOrExpr
//  block        → '{' statement* '}'
//  letStmt      → 'let' IDENTIFIER '=' expr ';'
//  assignOrExpr → IDENTIFIER '=' expr ';'   |   expr ';'
//  ifStmt       → 'if' '(' expr ')' block ( 'else' (ifStmt|block) )?
//  whileStmt    → 'while' '(' expr ')' block
//  printStmt    → 'print' expr ';'
//  inputStmt    → 'input' IDENTIFIER ';'
//
//  Expressions (from lowest to highest precedence):
//  expr         → equality
//  equality     → comparison ( ('=='|'!=') comparison )*
//  comparison   → additive   ( ('<'|'<='|'>'|'>=') additive )*
//  additive     → multiply   ( ('+'|'-') multiply )*
//  multiply     → unary      ( ('*'|'/') unary )*
//  unary        → '-' unary  |  primary
//  primary      → NUMBER | 'true' | 'false' | IDENTIFIER | '(' expr ')'
// ============================================================
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parses the entire token stream and returns the root Block node.
    ASTNodePtr parse();

private:
    std::vector<Token> tokens;
    int pos;

    // ── Token navigation ──────────────────────────────────────
    Token& peek(int offset = 0);
    Token& advance();
    bool   check(TokenType t);
    bool   match(TokenType t);          // advance if next matches t
    Token  expect(TokenType t, const std::string& msg);
    bool   isAtEnd();

    // ── Statement parsing ─────────────────────────────────────
    ASTNodePtr parseStatement();
    ASTNodePtr parseBlock();
    ASTNodePtr parseLetStatement();
    ASTNodePtr parseAssignOrExprStatement();
    ASTNodePtr parseIfStatement();
    ASTNodePtr parseWhileStatement();
    ASTNodePtr parsePrintStatement();
    ASTNodePtr parseInputStatement();

    // ── Expression parsing (precedence layers) ─────────────────
    ASTNodePtr parseExpression();
    ASTNodePtr parseEquality();
    ASTNodePtr parseComparison();
    ASTNodePtr parseAdditive();
    ASTNodePtr parseMultiplicative();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();
};