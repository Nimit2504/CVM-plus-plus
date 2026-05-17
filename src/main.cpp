#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"

// ============================================================
//  DEBUG UTILITIES
// ============================================================

// ── Print all tokens ─────────────────────────────────────────
void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n╔══════════════════ TOKENS ══════════════════╗\n";
    for (const auto& t : tokens) {
        if (t.type == TokenType::EOF_TOK) continue;
        std::cout << "  line " << t.line
                  << "  [" << t.value << "]\n";
    }
    std::cout << "╚════════════════════════════════════════════╝\n";
}

// ── Recursively print the AST ─────────────────────────────────
void printAST(ASTNode* node, int indent = 0) {
    std::string pad(indent * 2, ' ');
    std::cout << pad << node->nodeType() << "\n";

    if (auto n = dynamic_cast<Block*>(node)) {
        for (auto& s : n->statements)
            printAST(s.get(), indent + 1);
    }
    else if (auto n = dynamic_cast<LetStatement*>(node)) {
        printAST(n->initializer.get(), indent + 1);
    }
    else if (auto n = dynamic_cast<AssignStatement*>(node)) {
        printAST(n->value.get(), indent + 1);
    }
    else if (auto n = dynamic_cast<BinaryExpr*>(node)) {
        printAST(n->left.get(), indent + 1);
        printAST(n->right.get(), indent + 1);
    }
    else if (auto n = dynamic_cast<UnaryExpr*>(node)) {
        printAST(n->operand.get(), indent + 1);
    }
    else if (auto n = dynamic_cast<IfStatement*>(node)) {
        std::cout << pad << "  ├─ condition:\n";
        printAST(n->condition.get(), indent + 2);
        std::cout << pad << "  ├─ then:\n";
        printAST(n->thenBranch.get(), indent + 2);
        if (n->elseBranch) {
            std::cout << pad << "  └─ else:\n";
            printAST(n->elseBranch.get(), indent + 2);
        }
    }
    else if (auto n = dynamic_cast<WhileStatement*>(node)) {
        std::cout << pad << "  ├─ condition:\n";
        printAST(n->condition.get(), indent + 2);
        std::cout << pad << "  └─ body:\n";
        printAST(n->body.get(), indent + 2);
    }
    else if (auto n = dynamic_cast<PrintStatement*>(node)) {
        printAST(n->expr.get(), indent + 1);
    }
    // leaf nodes (NumberLiteral, BoolLiteral, Identifier, InputStatement)
    // have no children to recurse into
}

// ── Disassemble compiled bytecode into human-readable form ────
void printBytecode(const CompiledProgram& prog) {
    std::cout << "\n╔══════════════ BYTECODE DISASSEMBLY ════════╗\n";

    std::cout << "  Name Table (variables):\n";
    for (int i = 0; i < (int)prog.nameTable.size(); i++)
        std::cout << "    [" << i << "] = " << prog.nameTable[i] << "\n";

    std::cout << "  Instructions:\n";
    const auto& code = prog.code;
    int i = 0;
    while (i < (int)code.size()) {
        std::cout << "    " << i << ": ";
        Opcode op = static_cast<Opcode>(code[i++]);

        switch (op) {
        case Opcode::PUSH_INT: {
            int32_t v = code[i] | (code[i+1]<<8) | (code[i+2]<<16) | (code[i+3]<<24);
            std::cout << "PUSH_INT  " << v; i += 4; break;
        }
        case Opcode::PUSH_BOOL:
            std::cout << "PUSH_BOOL " << (code[i++] ? "true" : "false"); break;
        case Opcode::POP:    std::cout << "POP";    break;
        case Opcode::ADD:    std::cout << "ADD";    break;
        case Opcode::SUB:    std::cout << "SUB";    break;
        case Opcode::MUL:    std::cout << "MUL";    break;
        case Opcode::DIV:    std::cout << "DIV";    break;
        case Opcode::EQ:     std::cout << "EQ";     break;
        case Opcode::NEQ:    std::cout << "NEQ";    break;
        case Opcode::LT:     std::cout << "LT";     break;
        case Opcode::LEQ:    std::cout << "LEQ";    break;
        case Opcode::GT:     std::cout << "GT";     break;
        case Opcode::GEQ:    std::cout << "GEQ";    break;
        case Opcode::NEGATE: std::cout << "NEGATE"; break;
        case Opcode::LOAD: {
            uint16_t idx = code[i] | (code[i+1]<<8);
            std::cout << "LOAD   '" << prog.nameTable[idx] << "'"; i += 2; break;
        }
        case Opcode::STORE: {
            uint16_t idx = code[i] | (code[i+1]<<8);
            std::cout << "STORE  '" << prog.nameTable[idx] << "'"; i += 2; break;
        }
        case Opcode::JUMP: {
            int32_t t = code[i]|(code[i+1]<<8)|(code[i+2]<<16)|(code[i+3]<<24);
            std::cout << "JUMP          -> " << t; i += 4; break;
        }
        case Opcode::JUMP_IF_FALSE: {
            int32_t t = code[i]|(code[i+1]<<8)|(code[i+2]<<16)|(code[i+3]<<24);
            std::cout << "JUMP_IF_FALSE -> " << t; i += 4; break;
        }
        case Opcode::PRINT: std::cout << "PRINT"; break;
        case Opcode::INPUT: {
            uint16_t idx = code[i] | (code[i+1]<<8);
            std::cout << "INPUT  '" << prog.nameTable[idx] << "'"; i += 2; break;
        }
        case Opcode::HALT:  std::cout << "HALT";  break;
        default:
            std::cout << "??? (0x"
                      << std::hex << (int)code[i-1] << std::dec << ")"; break;
        }
        std::cout << "\n";
    }
    std::cout << "╚════════════════════════════════════════════╝\n";
}

// ============================================================
//  PIPELINE
//  Runs the full Lex → Parse → Compile → Execute pipeline on
//  a source string.
// ============================================================
void runSource(const std::string& source, bool debug) {
    try {
        // 1. Lex
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (debug) printTokens(tokens);

        // 2. Parse
        Parser parser(tokens);
        auto ast = parser.parse();
        if (debug) {
            std::cout << "\n╔══════════════════ AST ═════════════════════╗\n";
            printAST(ast.get());
            std::cout << "╚════════════════════════════════════════════╝\n";
        }

        // 3. Compile
        Compiler compiler;
        auto prog = compiler.compile(ast.get());
        if (debug) printBytecode(prog);

        // 4. Execute
        if (debug)
            std::cout << "\n╔═══════════════ EXECUTION ══════════════════╗\n";
        VM vm(std::move(prog), debug);
        vm.run();
        if (debug)
            std::cout << "╚════════════════════════════════════════════╝\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[CVM ERROR] " << e.what() << "\n";
    }
}

// ============================================================
//  REPL  – Read-Eval-Print Loop
// ============================================================
void runREPL(bool debug) {
    std::cout << "┌─────────────────────────────────────────────┐\n";
    std::cout << "│         CVM++  Interactive REPL             │\n";
    std::cout << "│  Commands:                                  │\n";
    std::cout << "│    exit          – quit                     │\n";
    std::cout << "│    debug on/off  – toggle debug mode        │\n";
    std::cout << "│  Terminate multi-line input with a blank    │\n";
    std::cout << "│  line, or just type a single statement.     │\n";
    std::cout << "└─────────────────────────────────────────────┘\n\n";

    std::string accumulated;

    while (true) {
        // Show a different prompt when in the middle of a multi-line block
        std::cout << (accumulated.empty() ? "cvm>> " : "  ... ");
        std::cout.flush();

        std::string line;
        if (!std::getline(std::cin, line)) break; // EOF (Ctrl-D)

        // ── Meta-commands ────────────────────────────────────
        if (line == "exit") break;
        if (line == "debug on")  { debug = true;  std::cout << "  [Debug ON]\n";  continue; }
        if (line == "debug off") { debug = false; std::cout << "  [Debug OFF]\n"; continue; }

        // ── Accumulate lines ──────────────────────────────────
        // An empty line flushes the buffer; otherwise keep collecting.
        if (line.empty()) {
            if (!accumulated.empty()) {
                runSource(accumulated, debug);
                accumulated.clear();
            }
            continue;
        }

        accumulated += line + "\n";

        // Auto-flush if the line ends with ';' and no open braces.
        // (Simple heuristic; good enough for single-statement REPL use.)
        int braceDepth = 0;
        for (char c : accumulated) {
            if (c == '{') ++braceDepth;
            if (c == '}') --braceDepth;
        }
        if (braceDepth == 0 && !accumulated.empty() &&
            accumulated.find(';') != std::string::npos)
        {
            runSource(accumulated, debug);
            accumulated.clear();
        }
    }

    std::cout << "\nGoodbye!\n";
}

// ============================================================
//  main()
//  Usage:
//    cvm                    → start REPL
//    cvm script.cvm         → run file
//    cvm --debug script.cvm → run file with debug output
//    cvm -d script.cvm      → same
// ============================================================
int main(int argc, char* argv[]) {
    bool        debug    = false;
    std::string filename;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") debug = true;
        else filename = arg;
    }

    if (filename.empty()) {
        runREPL(debug);
    } else {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "[CVM] Error: cannot open file '" << filename << "'\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string source = ss.str();

        std::cout << "=== CVM++ | Running: " << filename << " ===\n";
        runSource(source, debug);
    }

    return 0;
}