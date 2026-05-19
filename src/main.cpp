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
    std::cout << "\n=== TOKENS ===\n";
    for (const auto& t : tokens) {
        if (t.type == TokenType::EOF_TOK) continue;
        std::cout << "  line " << t.line
                  << "  [" << t.value << "]\n";
    }
    std::cout << "=====================================\n";
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
        std::cout << pad << "  +- condition:\n";
        printAST(n->condition.get(), indent + 2);
        std::cout << pad << "  +- then:\n";
        printAST(n->thenBranch.get(), indent + 2);
        if (n->elseBranch) {
            std::cout << pad << "  +- else:\n";
            printAST(n->elseBranch.get(), indent + 2);
        }
    }
    else if (auto n = dynamic_cast<WhileStatement*>(node)) {
        std::cout << pad << "  +- condition:\n";
        printAST(n->condition.get(), indent + 2);
        std::cout << pad << "  +- body:\n";
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
    std::cout << "\n=== BYTECODE DISASSEMBLY ===\n";

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
    std::cout << "=====================================\n";
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
            std::cout << "\n=== AST ===\n";
            printAST(ast.get());
            std::cout << "=====================================\n";
        }

        // 3. Compile
        Compiler compiler;
        auto prog = compiler.compile(ast.get());
        if (debug) printBytecode(prog);

        // 4. Execute
        if (debug)
            std::cout << "\n=== EXECUTION ===\n";
        VM vm(std::move(prog), debug);
        vm.run();
        if (debug)
            std::cout << "=====================================\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[CVM ERROR] " << e.what() << "\n";
    }
}

// ============================================================
//  REPL  – Read-Eval-Print Loop
// ============================================================
void runREPL(bool debug) {
    std::cout << "+---------------------------------------------+\n";
    std::cout << "|         CVM++  Interactive REPL             |\n";
    std::cout << "|  Commands:                                  |\n";
    std::cout << "|    exit          - quit                     |\n";
    std::cout << "|    debug on/off  - toggle debug mode        |\n";
    std::cout << "|  Terminate multi-line input with a blank    |\n";
    std::cout << "|  line, or just type a single statement.     |\n";
    std::cout << "+---------------------------------------------+\n\n";

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

        // Never auto-run. Always accumulate until the user
        // presses a blank line. This ensures all statements in a
        // session share one VM instance and variables persist.
        // (Blank line handling is above — it calls runSource then clears)
    }

    std::cout << "\nGoodbye!\n";
}


// ============================================================
//  BYTECODE FILE FORMAT  (.cvmb)
//
//  Header:   4 bytes magic  "CVMB"
//            4 bytes (uint32) number of name table entries
//            4 bytes (uint32) number of bytecode bytes
//  Names:    for each name: 4-byte length + raw chars (no null)
//  Code:     raw bytecode bytes
// ============================================================

// Save a CompiledProgram to a binary .cvmb file
bool saveProgram(const CompiledProgram& prog, const std::string& outPath) {
    std::ofstream f(outPath, std::ios::binary);
    if (!f) {
        std::cerr << "[CVM] Error: cannot write to '" << outPath << "'\n";
        return false;
    }

    // Magic header
    f.write("CVMB", 4);

    // Name table size
    uint32_t nameCount = static_cast<uint32_t>(prog.nameTable.size());
    f.write(reinterpret_cast<const char*>(&nameCount), 4);

    // Bytecode size
    uint32_t codeSize = static_cast<uint32_t>(prog.code.size());
    f.write(reinterpret_cast<const char*>(&codeSize), 4);

    // Name table entries: 4-byte length + chars
    for (const auto& name : prog.nameTable) {
        uint32_t len = static_cast<uint32_t>(name.size());
        f.write(reinterpret_cast<const char*>(&len), 4);
        f.write(name.data(), len);
    }

    // Raw bytecode
    f.write(reinterpret_cast<const char*>(prog.code.data()), codeSize);

    return true;
}

// Load a .cvmb file back into a CompiledProgram
CompiledProgram loadProgram(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot open bytecode file '" + path + "'");

    // Check magic
    char magic[4];
    f.read(magic, 4);
    if (std::string(magic, 4) != "CVMB")
        throw std::runtime_error("Not a valid .cvmb file: '" + path + "'");

    // Read sizes
    uint32_t nameCount = 0, codeSize = 0;
    f.read(reinterpret_cast<char*>(&nameCount), 4);
    f.read(reinterpret_cast<char*>(&codeSize),  4);

    CompiledProgram prog;

    // Read name table
    for (uint32_t i = 0; i < nameCount; i++) {
        uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), 4);
        std::string name(len, '\0');
        f.read(&name[0], len);
        prog.nameTable.push_back(name);
    }

    // Read bytecode
    prog.code.resize(codeSize);
    f.read(reinterpret_cast<char*>(prog.code.data()), codeSize);

    return prog;
}

// Compile a .cvm source file and save bytecode to a .cvmb file
void compileToFile(const std::string& srcPath, const std::string& outPath, bool debug) {
    std::ifstream file(srcPath);
    if (!file) {
        std::cerr << "[CVM] Error: cannot open source file '" << srcPath << "'\n";
        return;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (debug) printTokens(tokens);

        Parser parser(tokens);
        auto ast = parser.parse();
        if (debug) {
            std::cout << "\n=== AST ===\n";
            printAST(ast.get());
            std::cout << "=====================================\n";
        }

        Compiler compiler;
        auto prog = compiler.compile(ast.get());
        if (debug) printBytecode(prog);

        if (saveProgram(prog, outPath)) {
            std::cout << "[CVM] Compiled '" << srcPath << "'\n";
            std::cout << "[CVM] Bytecode saved to '" << outPath << "'\n";
            std::cout << "[CVM] Bytecode size: " << prog.code.size() << " bytes\n";
            std::cout << "[CVM] Variables in name table: " << prog.nameTable.size() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "\n[CVM ERROR] " << e.what() << "\n";
    }
}

// Load a .cvmb bytecode file and execute it on the VM
void runFromFile(const std::string& path, bool debug) {
    try {
        CompiledProgram prog = loadProgram(path);
        std::cout << "[CVM] Loaded bytecode from '" << path << "'\n";
        std::cout << "[CVM] Bytecode size: " << prog.code.size() << " bytes\n";
        std::cout << "[CVM] Running...\n";
        std::cout << "-------------------------------------\n";
        if (debug) printBytecode(prog);
        VM vm(std::move(prog), debug);
        vm.run();
        std::cout << "-------------------------------------\n";
        std::cout << "[CVM] Execution complete.\n";
    } catch (const std::exception& e) {
        std::cerr << "\n[CVM ERROR] " << e.what() << "\n";
    }
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
    // Usage:
    //   cvm                              -> REPL
    //   cvm script.cvm                   -> run source file
    //   cvm --debug script.cvm           -> run with debug output
    //   cvm compile script.cvm -o out.cvmb  -> compile to bytecode file
    //   cvm run out.cvmb                 -> run bytecode file on VM
    //   cvm compile --debug script.cvm -o out.cvmb  -> compile with debug

    if (argc < 2) {
        runREPL(false);
        return 0;
    }

    std::string command = argv[1];
    bool debug = false;

    // ── compile subcommand ───────────────────────────────────
    // Usage: cvm compile [--debug] source.cvm -o output.cvmb
    if (command == "compile") {
        std::string srcPath, outPath;
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") debug = true;
            else if ((arg == "-o") && i + 1 < argc) outPath = argv[++i];
            else srcPath = arg;
        }
        if (srcPath.empty()) {
            std::cerr << "[CVM] Usage: cvm compile [--debug] source.cvm -o output.cvmb\n";
            return 1;
        }
        if (outPath.empty()) {
            // Default: replace .cvm with .cvmb
            outPath = srcPath;
            size_t dot = outPath.rfind('.');
            if (dot != std::string::npos) outPath = outPath.substr(0, dot);
            outPath += ".cvmb";
        }
        compileToFile(srcPath, outPath, debug);
        return 0;
    }

    // ── run subcommand ───────────────────────────────────────
    // Usage: cvm run [--debug] output.cvmb
    if (command == "run") {
        std::string bytecodeFile;
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") debug = true;
            else bytecodeFile = arg;
        }
        if (bytecodeFile.empty()) {
            std::cerr << "[CVM] Usage: cvm run [--debug] file.cvmb\n";
            return 1;
        }
        runFromFile(bytecodeFile, debug);
        return 0;
    }

    // ── default: run source file directly ───────────────────
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