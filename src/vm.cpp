#include "vm.h"
#include <iostream>
#include <stdexcept>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
VM::VM(CompiledProgram program, bool debug)
    : prog(std::move(program)), debug(debug), ip(0) {}

// ─────────────────────────────────────────────
//  Byte-reading helpers
// ─────────────────────────────────────────────

uint8_t VM::readByte() {
    if (ip >= (int)prog.code.size())
        throw std::runtime_error("VM: instruction pointer past end of bytecode");
    return prog.code[ip++];
}

// Read a 4-byte little-endian signed integer.
int32_t VM::readInt32() {
    if (ip + 3 >= (int)prog.code.size())
        throw std::runtime_error("VM: truncated INT32 operand");
    int32_t val =
         (int32_t)prog.code[ip]         |
        ((int32_t)prog.code[ip + 1] <<  8) |
        ((int32_t)prog.code[ip + 2] << 16) |
        ((int32_t)prog.code[ip + 3] << 24);
    ip += 4;
    return val;
}

// Read a 2-byte little-endian unsigned integer.
uint16_t VM::readUInt16() {
    if (ip + 1 >= (int)prog.code.size())
        throw std::runtime_error("VM: truncated UINT16 operand");
    uint16_t val =
         (uint16_t)prog.code[ip]          |
        ((uint16_t)prog.code[ip + 1] << 8);
    ip += 2;
    return val;
}

// ─────────────────────────────────────────────
//  Stack helpers
// ─────────────────────────────────────────────

void VM::push(Value v) {
    stack.push_back(v);
}

Value VM::pop() {
    if (stack.empty())
        throw std::runtime_error("VM: stack underflow");
    Value v = stack.back();
    stack.pop_back();
    return v;
}

Value VM::stackTop() const {
    if (stack.empty())
        throw std::runtime_error("VM: stack is empty");
    return stack.back();
}

// ─────────────────────────────────────────────
//  Debug helpers
// ─────────────────────────────────────────────

std::string VM::opcodeName(uint8_t op) const {
    switch (static_cast<Opcode>(op)) {
        case Opcode::PUSH_INT:      return "PUSH_INT";
        case Opcode::PUSH_BOOL:     return "PUSH_BOOL";
        case Opcode::POP:           return "POP";
        case Opcode::ADD:           return "ADD";
        case Opcode::SUB:           return "SUB";
        case Opcode::MUL:           return "MUL";
        case Opcode::DIV:           return "DIV";
        case Opcode::EQ:            return "EQ";
        case Opcode::NEQ:           return "NEQ";
        case Opcode::LT:            return "LT";
        case Opcode::LEQ:           return "LEQ";
        case Opcode::GT:            return "GT";
        case Opcode::GEQ:           return "GEQ";
        case Opcode::NEGATE:        return "NEGATE";
        case Opcode::LOAD:          return "LOAD";
        case Opcode::STORE:         return "STORE";
        case Opcode::JUMP:          return "JUMP";
        case Opcode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case Opcode::PRINT:         return "PRINT";
        case Opcode::INPUT:         return "INPUT";
        case Opcode::HALT:          return "HALT";
        default:                    return "UNKNOWN";
    }
}

void VM::printStack() const {
    std::cout << "  stack [";
    for (int i = 0; i < (int)stack.size(); i++) {
        if (i) std::cout << ", ";
        std::cout << stack[i].toString();
    }
    std::cout << "]\n";
}

// ─────────────────────────────────────────────
//  Main execution loop
// ─────────────────────────────────────────────

void VM::run() {
    while (ip < (int)prog.code.size()) {

        int instrOffset = ip;          // save for debug output
        uint8_t opByte  = readByte();
        Opcode  op      = static_cast<Opcode>(opByte);

        // ── Debug: print instruction before executing ──────────
        if (debug) {
            std::cout << "[VM:" << instrOffset << "] "
                      << opcodeName(opByte) << "\n";
            printStack();
        }

        switch (op) {

        // ══════════════════════════════════════════════════════
        //  Stack literals
        // ══════════════════════════════════════════════════════

        case Opcode::PUSH_INT: {
            int32_t val = readInt32();
            push(Value::fromInt(val));
            break;
        }

        case Opcode::PUSH_BOOL: {
            bool val = (readByte() != 0);
            push(Value::fromBool(val));
            break;
        }

        case Opcode::POP: {
            pop();
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Arithmetic  (both operands must be INT)
        // ══════════════════════════════════════════════════════

        case Opcode::ADD: {
            Value b = pop(), a = pop();
            if (a.type != Value::Type::INT || b.type != Value::Type::INT)
                throw std::runtime_error("VM: ADD requires integer operands");
            push(Value::fromInt(a.ival + b.ival));
            break;
        }

        case Opcode::SUB: {
            Value b = pop(), a = pop();
            if (a.type != Value::Type::INT || b.type != Value::Type::INT)
                throw std::runtime_error("VM: SUB requires integer operands");
            push(Value::fromInt(a.ival - b.ival));
            break;
        }

        case Opcode::MUL: {
            Value b = pop(), a = pop();
            if (a.type != Value::Type::INT || b.type != Value::Type::INT)
                throw std::runtime_error("VM: MUL requires integer operands");
            push(Value::fromInt(a.ival * b.ival));
            break;
        }

        case Opcode::DIV: {
            Value b = pop(), a = pop();
            if (a.type != Value::Type::INT || b.type != Value::Type::INT)
                throw std::runtime_error("VM: DIV requires integer operands");
            if (b.ival == 0)
                throw std::runtime_error("VM: division by zero");
            push(Value::fromInt(a.ival / b.ival));
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Comparison  (result is always BOOL)
        // ══════════════════════════════════════════════════════

        case Opcode::EQ: {
            Value b = pop(), a = pop();
            bool result = (a.type == b.type) &&
                          (a.type == Value::Type::INT
                              ? a.ival == b.ival
                              : a.bval == b.bval);
            push(Value::fromBool(result));
            break;
        }

        case Opcode::NEQ: {
            Value b = pop(), a = pop();
            bool eq = (a.type == b.type) &&
                      (a.type == Value::Type::INT
                          ? a.ival == b.ival
                          : a.bval == b.bval);
            push(Value::fromBool(!eq));
            break;
        }

        case Opcode::LT: {
            Value b = pop(), a = pop();
            push(Value::fromBool(a.ival < b.ival));
            break;
        }

        case Opcode::LEQ: {
            Value b = pop(), a = pop();
            push(Value::fromBool(a.ival <= b.ival));
            break;
        }

        case Opcode::GT: {
            Value b = pop(), a = pop();
            push(Value::fromBool(a.ival > b.ival));
            break;
        }

        case Opcode::GEQ: {
            Value b = pop(), a = pop();
            push(Value::fromBool(a.ival >= b.ival));
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Unary
        // ══════════════════════════════════════════════════════

        case Opcode::NEGATE: {
            Value a = pop();
            if (a.type != Value::Type::INT)
                throw std::runtime_error("VM: NEGATE requires integer operand");
            push(Value::fromInt(-a.ival));
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Variable access
        // ══════════════════════════════════════════════════════

        case Opcode::LOAD: {
            uint16_t idx = readUInt16();
            const std::string& name = prog.nameTable[idx];
            auto it = variables.find(name);
            if (it == variables.end())
                throw std::runtime_error(
                    "VM: undefined variable '" + name + "'");
            push(it->second);
            break;
        }

        case Opcode::STORE: {
            uint16_t idx = readUInt16();
            const std::string& name = prog.nameTable[idx];
            variables[name] = pop();
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Control flow
        // ══════════════════════════════════════════════════════

        case Opcode::JUMP: {
            int32_t target = readInt32();
            ip = target;
            break;
        }

        case Opcode::JUMP_IF_FALSE: {
            int32_t target = readInt32();
            Value   cond   = pop();
            if (!cond.truthy()) ip = target;
            break;
        }

        // ══════════════════════════════════════════════════════
        //  I/O
        // ══════════════════════════════════════════════════════

        case Opcode::PRINT: {
            Value val = pop();
            std::cout << val.toString() << "\n";
            break;
        }

        case Opcode::INPUT: {
            uint16_t    idx  = readUInt16();
            const std::string& name = prog.nameTable[idx];
            std::cout << "Enter value for " << name << ": ";
            std::string inputStr;
            if (!std::getline(std::cin, inputStr)) inputStr = "0";
            int val = 0;
            try { val = std::stoi(inputStr); }
            catch (...) { /* non-numeric input → 0 */ }
            variables[name] = Value::fromInt(val);
            break;
        }

        // ══════════════════════════════════════════════════════
        //  Halt
        // ══════════════════════════════════════════════════════

        case Opcode::HALT:
            if (debug) std::cout << "[VM] Halted.\n";
            return;

        default:
            throw std::runtime_error(
                "VM: unknown opcode 0x" +
                [&]{ char buf[8]; snprintf(buf, sizeof(buf), "%02X", opByte); return std::string(buf); }());
        }
    }
    // Fell off the end of code without HALT — treat as normal exit.
}