#pragma once
#include <string>
#include <vector>
#include <stdexcept>

// ============================================================
//  TOKEN TYPES
//  Every distinct piece of syntax gets its own token type.
// ============================================================
enum class TokenType {
    // --- Literals ---
    NUMBER,       // e.g.  42
    TRUE_LIT,     // true
    FALSE_LIT,    // false
    IDENTIFIER,   // e.g.  myVar

    // --- Arithmetic operators ---
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH,        // /

    // --- Comparison operators ---
    EQ_EQ,        // ==
    NOT_EQ,       // !=
    LESS,         // <
    LESS_EQ,      // <=
    GREATER,      // >
    GREATER_EQ,   // >=

    // --- Assignment ---
    ASSIGN,       // =

    // --- Delimiters ---
    SEMICOLON,    // ;
    LPAREN,       // (
    RPAREN,       // )
    LBRACE,       // {
    RBRACE,       // }

    // --- Keywords ---
    LET,          // let
    IF,           // if
    ELSE,         // else
    WHILE,        // while
    PRINT,        // print
    INPUT,        // input

    // --- Special ---
    EOF_TOK       // marks end of token stream
};

// ============================================================
//  TOKEN
//  A token bundles its type, raw text value, and source line.
// ============================================================
struct Token {
    TokenType   type;
    std::string value;   // raw text (e.g. "123", "myVar", "+")
    int         line;    // 1-based line number for error messages

    Token(TokenType t, std::string v, int l)
        : type(t), value(std::move(v)), line(l) {}
};

// ============================================================
//  LEXER
//  Scans a source string character-by-character and produces
//  a flat list of tokens.
// ============================================================
class Lexer {
public:
    explicit Lexer(std::string source);

    // Main entry point: returns all tokens including EOF_TOK.
    std::vector<Token> tokenize();

private:
    std::string source;
    int pos;    // current read position
    int line;   // current line number

    char   peek(int offset = 0) const;
    char   advance();
    bool   isAtEnd() const;

    void  skipWhitespaceAndComments();
    Token readNumber();
    Token readIdentifierOrKeyword();
};