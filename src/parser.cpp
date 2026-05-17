#include "parser.h"

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)), pos(0) {}

// ─────────────────────────────────────────────
//  Token navigation helpers
// ─────────────────────────────────────────────

Token& Parser::peek(int offset) {
    int idx = pos + offset;
    // Clamp to the last token (which is always EOF_TOK)
    if (idx >= (int)tokens.size()) return tokens.back();
    return tokens[idx];
}

Token& Parser::advance() {
    if (!isAtEnd()) ++pos;
    return tokens[pos - 1];
}

bool Parser::check(TokenType t) {
    return peek().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

// Advance if the current token matches t; otherwise throw.
Token Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    throw std::runtime_error(
        msg + " at line " + std::to_string(peek().line) +
        " (got '" + peek().value + "')");
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::EOF_TOK;
}

// ─────────────────────────────────────────────
//  Top-level entry
// ─────────────────────────────────────────────

// parse() → program → statement* EOF
ASTNodePtr Parser::parse() {
    auto root = std::make_unique<Block>();
    while (!isAtEnd()) {
        root->statements.push_back(parseStatement());
    }
    return root;
}

// ─────────────────────────────────────────────
//  Statement parsing
// ─────────────────────────────────────────────

ASTNodePtr Parser::parseStatement() {
    if (check(TokenType::LET))    return parseLetStatement();
    if (check(TokenType::IF))     return parseIfStatement();
    if (check(TokenType::WHILE))  return parseWhileStatement();
    if (check(TokenType::PRINT))  return parsePrintStatement();
    if (check(TokenType::INPUT))  return parseInputStatement();
    if (check(TokenType::LBRACE)) return parseBlock();
    return parseAssignOrExprStatement();
}

// block → '{' statement* '}'
ASTNodePtr Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<Block>();
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    expect(TokenType::RBRACE, "Expected closing '}'");
    return block;
}

// letStmt → 'let' IDENTIFIER '=' expr ';'
ASTNodePtr Parser::parseLetStatement() {
    advance(); // consume 'let'
    Token name = expect(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    expect(TokenType::ASSIGN, "Expected '=' after variable name in let");
    auto init = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after let statement");
    return std::make_unique<LetStatement>(name.value, std::move(init));
}

// assignOrExpr →  IDENTIFIER '=' expr ';'
//              |  expr ';'
// Disambiguated by 1-token lookahead: IDENTIFIER followed by '=' is an
// assignment; anything else is an expression statement.
ASTNodePtr Parser::parseAssignOrExprStatement() {
    if (check(TokenType::IDENTIFIER) &&
        peek(1).type == TokenType::ASSIGN)
    {
        Token name = advance(); // IDENTIFIER
        advance();              // '='
        auto val = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after assignment");
        return std::make_unique<AssignStatement>(name.value, std::move(val));
    }

    // Plain expression statement (e.g. a bare function call in the future)
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after expression statement");
    return expr;
}

// ifStmt → 'if' '(' expr ')' block ( 'else' ( ifStmt | block ) )?
ASTNodePtr Parser::parseIfStatement() {
    advance(); // consume 'if'
    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after if-condition");
    auto thenBranch = parseBlock();

    ASTNodePtr elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        // else-if chain: reuse parseIfStatement recursively
        if (check(TokenType::IF)) elseBranch = parseIfStatement();
        else                      elseBranch = parseBlock();
    }

    return std::make_unique<IfStatement>(
        std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

// whileStmt → 'while' '(' expr ')' block
ASTNodePtr Parser::parseWhileStatement() {
    advance(); // consume 'while'
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after while-condition");
    auto body = parseBlock();
    return std::make_unique<WhileStatement>(std::move(cond), std::move(body));
}

// printStmt → 'print' expr ';'
ASTNodePtr Parser::parsePrintStatement() {
    advance(); // consume 'print'
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after print");
    return std::make_unique<PrintStatement>(std::move(expr));
}

// inputStmt → 'input' IDENTIFIER ';'
ASTNodePtr Parser::parseInputStatement() {
    advance(); // consume 'input'
    Token name = expect(TokenType::IDENTIFIER,
                        "Expected variable name after 'input'");
    expect(TokenType::SEMICOLON, "Expected ';' after input");
    return std::make_unique<InputStatement>(name.value);
}

// ─────────────────────────────────────────────
//  Expression parsing  (precedence climbing via
//  recursive-descent layers)
// ─────────────────────────────────────────────

// Each layer calls the next-higher-precedence layer and then
// loops while it sees operators at its own precedence level.
// This naturally gives correct precedence without any table.

ASTNodePtr Parser::parseExpression() {
    return parseEquality();
}

// equality → comparison ( ('==' | '!=') comparison )*
ASTNodePtr Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EQ_EQ) || check(TokenType::NOT_EQ)) {
        std::string op = advance().value;
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// comparison → additive ( ('<' | '<=' | '>' | '>=') additive )*
ASTNodePtr Parser::parseComparison() {
    auto left = parseAdditive();
    while (check(TokenType::LESS)    || check(TokenType::LESS_EQ) ||
           check(TokenType::GREATER) || check(TokenType::GREATER_EQ))
    {
        std::string op = advance().value;
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// additive → multiply ( ('+' | '-') multiply )*
ASTNodePtr Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = advance().value;
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// multiply → unary ( ('*' | '/') unary )*
ASTNodePtr Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        std::string op = advance().value;
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// unary → '-' unary  |  primary
ASTNodePtr Parser::parseUnary() {
    if (check(TokenType::MINUS)) {
        std::string op = advance().value;
        auto operand = parseUnary();   // right-associative
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    return parsePrimary();
}

// primary → NUMBER | 'true' | 'false' | IDENTIFIER | '(' expr ')'
ASTNodePtr Parser::parsePrimary() {
    if (check(TokenType::NUMBER)) {
        int val = std::stoi(advance().value);
        return std::make_unique<NumberLiteral>(val);
    }
    if (check(TokenType::TRUE_LIT)) {
        advance();
        return std::make_unique<BoolLiteral>(true);
    }
    if (check(TokenType::FALSE_LIT)) {
        advance();
        return std::make_unique<BoolLiteral>(false);
    }
    if (check(TokenType::IDENTIFIER)) {
        return std::make_unique<Identifier>(advance().value);
    }
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' to close grouped expression");
        return expr;
    }

    throw std::runtime_error(
        "Unexpected token '" + peek().value +
        "' at line " + std::to_string(peek().line));
}