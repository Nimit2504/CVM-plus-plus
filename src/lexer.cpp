#include "lexer.h"
#include <cctype>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Lexer::Lexer(std::string source)
    : source(std::move(source)), pos(0), line(1) {}

// ─────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────

char Lexer::peek(int offset) const {
    int idx = pos + offset;
    if (idx < 0 || idx >= (int)source.size()) return '\0';
    return source[idx];
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') ++line;   // track line numbers
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= (int)source.size();
}

// Skip spaces, tabs, newlines, and // line-comments.
void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // Consume until end of line
            while (!isAtEnd() && peek() != '\n') advance();
        } else {
            break;
        }
    }
}

// Called when we see a digit: consume all consecutive digits.
Token Lexer::readNumber() {
    int start = pos;
    while (!isAtEnd() && std::isdigit((unsigned char)peek())) advance();
    return Token(TokenType::NUMBER, source.substr(start, pos - start), line);
}

// Called when we see a letter/_: consume identifier, then check
// if it matches a reserved keyword.
Token Lexer::readIdentifierOrKeyword() {
    int start = pos;
    while (!isAtEnd() &&
           (std::isalnum((unsigned char)peek()) || peek() == '_'))
        advance();

    std::string word = source.substr(start, pos - start);

    // Keyword table
    if (word == "let")   return Token(TokenType::LET,       word, line);
    if (word == "if")    return Token(TokenType::IF,        word, line);
    if (word == "else")  return Token(TokenType::ELSE,      word, line);
    if (word == "while") return Token(TokenType::WHILE,     word, line);
    if (word == "print") return Token(TokenType::PRINT,     word, line);
    if (word == "input") return Token(TokenType::INPUT,     word, line);
    if (word == "true")  return Token(TokenType::TRUE_LIT,  word, line);
    if (word == "false") return Token(TokenType::FALSE_LIT, word, line);

    return Token(TokenType::IDENTIFIER, word, line);
}

// ─────────────────────────────────────────────
//  Main tokenize() loop
// ─────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();

        if (isAtEnd()) {
            tokens.emplace_back(TokenType::EOF_TOK, "<EOF>", line);
            break;
        }

        char c = peek();

        // ── Multi-char: numbers ────────────────
        if (std::isdigit((unsigned char)c)) {
            tokens.push_back(readNumber());
            continue;
        }

        // ── Multi-char: identifiers / keywords ─
        if (std::isalpha((unsigned char)c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // ── Single / double-char tokens ─────────
        advance(); // consume the leading character

        switch (c) {
            case '+': tokens.emplace_back(TokenType::PLUS,      "+",  line); break;
            case '-': tokens.emplace_back(TokenType::MINUS,     "-",  line); break;
            case '*': tokens.emplace_back(TokenType::STAR,      "*",  line); break;
            case '/': tokens.emplace_back(TokenType::SLASH,     "/",  line); break;
            case ';': tokens.emplace_back(TokenType::SEMICOLON, ";",  line); break;
            case '(': tokens.emplace_back(TokenType::LPAREN,    "(",  line); break;
            case ')': tokens.emplace_back(TokenType::RPAREN,    ")",  line); break;
            case '{': tokens.emplace_back(TokenType::LBRACE,    "{",  line); break;
            case '}': tokens.emplace_back(TokenType::RBRACE,    "}",  line); break;

            case '=':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::EQ_EQ,   "==", line); }
                else                             tokens.emplace_back(TokenType::ASSIGN,   "=",  line);
                break;

            case '!':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::NOT_EQ,  "!=", line); }
                else throw std::runtime_error(
                    "Unexpected character '!' at line " + std::to_string(line));
                break;

            case '<':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::LESS_EQ, "<=", line); }
                else                             tokens.emplace_back(TokenType::LESS,    "<",  line);
                break;

            case '>':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::GREATER_EQ, ">=", line); }
                else                             tokens.emplace_back(TokenType::GREATER,    ">",  line);
                break;

            default:
                throw std::runtime_error(
                    std::string("Unexpected character '") + c +
                    "' at line " + std::to_string(line));
        }
    }

    return tokens;
}