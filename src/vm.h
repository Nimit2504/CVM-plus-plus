#pragma once
#include "compiler.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>

// ============================================================
//  VALUE
//
//  The runtime value type.  CVM supports two types:
//    INT  – a 32-bit signed integer
//    BOOL – a boolean (true / false)
//
//  We use a plain union rather than std::variant to keep the
//  VM loop as fast as possible (no heap allocation, no RTTI).
// ============================================================
struct Value {
    enum class Type { INT, BOOL } type;
    union { int ival; bool bval; };

    // Named constructors (cleaner than using constructors with
    // an ambiguous bool/int implicit-conversion problem).
    static Value fromInt(int v) {
        Value val;
        val.type = Type::INT;
        val.ival = v;
        return val;
    }
    static Value fromBool(bool v) {
        Value val;
        val.type = Type::BOOL;
        val.bval = v;
        return val;
    }

    // Truthiness: 0 and false are falsy; everything else is truthy.
    bool truthy() const {
        return (type == Type::INT) ? (ival != 0) : bval;
    }

    // For PRINT and debug output.
    std::string toString() const {
        if (type == Type::INT)  return std::to_string(ival);
        return bval ? "true" : "false";
    }
};

// ============================================================
//  VM  – Stack-Based Virtual Machine
//
//  Execution model:
//    • ip   – instruction pointer (index into code[])
//    • stack – operand stack (std::vector used as a stack)
//    • variables – name → Value map for the global scope
//
//  The main loop: read opcode → execute → repeat until HALT.
// ============================================================
class VM {
public:
    // prog  – the compiled program to execute
    // debug – when true, print each instruction before executing it
    VM(CompiledProgram prog, bool debug = false);

    void run();

private:
    CompiledProgram prog;
    bool            debug;
    int             ip;                    // instruction pointer
    std::vector<Value>                  stack;
    std::unordered_map<std::string, Value> variables;

    // ── Byte-reading helpers ──────────────────────────────────
    uint8_t  readByte();
    int32_t  readInt32();
    uint16_t readUInt16();

    // ── Stack helpers ─────────────────────────────────────────
    void  push(Value v);
    Value pop();
    Value stackTop() const;

    // ── Debug helpers ─────────────────────────────────────────
    std::string opcodeName(uint8_t op) const;
    void        printStack() const;
};