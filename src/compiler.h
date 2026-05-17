#pragma once
#include "ast.h"
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

// ============================================================
//  INSTRUCTION SET ARCHITECTURE  (ISA)
//
//  Each opcode is one byte (uint8_t).
//  Some opcodes are followed by operand bytes:
//    PUSH_INT        → 4 bytes (little-endian int32)
//    PUSH_BOOL       → 1 byte  (0 = false, 1 = true)
//    LOAD / STORE    → 2 bytes (uint16 index into nameTable)
//    INPUT           → 2 bytes (uint16 index into nameTable)
//    JUMP            → 4 bytes (int32 absolute bytecode offset)
//    JUMP_IF_FALSE   → 4 bytes (int32 absolute bytecode offset)
// ============================================================
enum class Opcode : uint8_t {
    // ── Stack operations ──────────────────────────
    PUSH_INT        = 0x01,  // push integer constant onto stack
    PUSH_BOOL       = 0x02,  // push boolean constant onto stack
    POP             = 0x03,  // discard top-of-stack

    // ── Arithmetic ────────────────────────────────
    ADD             = 0x10,  // pop b, pop a → push a + b
    SUB             = 0x11,  // pop b, pop a → push a - b
    MUL             = 0x12,  // pop b, pop a → push a * b
    DIV             = 0x13,  // pop b, pop a → push a / b

    // ── Comparison (always produces a bool) ───────
    EQ              = 0x20,  // ==
    NEQ             = 0x21,  // !=
    LT              = 0x22,  // <
    LEQ             = 0x23,  // <=
    GT              = 0x24,  // >
    GEQ             = 0x25,  // >=

    // ── Unary ─────────────────────────────────────
    NEGATE          = 0x30,  // pop a → push -a

    // ── Variables ─────────────────────────────────
    LOAD            = 0x40,  // push value of named variable
    STORE           = 0x41,  // pop and store into named variable

    // ── Control flow ──────────────────────────────
    JUMP            = 0x50,  // unconditional jump
    JUMP_IF_FALSE   = 0x51,  // pop condition; jump if falsy

    // ── I/O ───────────────────────────────────────
    PRINT           = 0x60,  // pop and print to stdout
    INPUT           = 0x61,  // read int from stdin, store in variable

    // ── Terminal ──────────────────────────────────
    HALT            = 0xFF   // stop the VM
};

// Raw bytecode array
using Bytecode = std::vector<uint8_t>;

// ============================================================
//  COMPILED PROGRAM
//  Everything the VM needs to execute a CVM program.
// ============================================================
struct CompiledProgram {
    Bytecode             code;       // the raw instruction stream
    std::vector<std::string> nameTable;  // variable names, indexed by uint16
};

// ============================================================
//  COMPILER
//
//  Walks the AST and emits bytecode instructions.
//  Key technique: "backpatching" for jump addresses.
//    1. Emit JUMP/JUMP_IF_FALSE with a placeholder offset.
//    2. Compile the body whose size we don't know yet.
//    3. Patch the placeholder with the real offset.
// ============================================================
class Compiler {
public:
    // Compile a full program (root must be a Block node).
    // Returns the compiled program ready for the VM.
    CompiledProgram compile(ASTNode* root);

private:
    Bytecode             code;
    std::vector<std::string> nameTable;
    std::unordered_map<std::string, uint16_t> nameIndex;

    // ── Name table management ────────────────────────────────
    uint16_t getOrAddName(const std::string& name);

    // ── Raw emission helpers ──────────────────────────────────
    void emit(uint8_t byte);
    void emitOpcode(Opcode op);
    void emitInt32(int32_t val);    // 4-byte little-endian
    void emitUInt16(uint16_t val);  // 2-byte little-endian

    // ── Jump backpatching ────────────────────────────────────
    // Emits opcode + 4-byte placeholder; returns position of the
    // placeholder so patchJump() can fill it in later.
    int  emitJump(Opcode jumpOp);
    // Fills the 4-byte slot at jumpPos with the current code size
    // (i.e., the instruction immediately after the jump target body).
    void patchJump(int jumpPos);

    // ── Node dispatch ────────────────────────────────────────
    void compileNode(ASTNode* node);
    void compileBlock(Block* node);
    void compileLetStatement(LetStatement* node);
    void compileAssignStatement(AssignStatement* node);
    void compileIfStatement(IfStatement* node);
    void compileWhileStatement(WhileStatement* node);
    void compilePrintStatement(PrintStatement* node);
    void compileInputStatement(InputStatement* node);
    void compileBinaryExpr(BinaryExpr* node);
    void compileUnaryExpr(UnaryExpr* node);
};