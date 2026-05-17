# CVM++ — Custom Compiler & Stack-Based Virtual Machine

A lightweight scripting language built entirely from scratch in C++.
Source code is lexed, parsed into an AST, compiled to custom bytecode,
and executed on a hand-written stack-based virtual machine.

This project replicates the core architecture used by Python (CPython),
Java (JVM), and Lua — with zero external dependencies.

---

## How It Works

```
Source Code (.cvm)
      │
      ▼  Lexer           converts raw text into tokens
      │
      ▼  Parser          arranges tokens into an Abstract Syntax Tree
      │
      ▼  Compiler        flattens the AST into bytecode instructions
      │
      ▼  Virtual Machine executes bytecode on a stack-based engine
      │
    Output
```

---

## Language Features

```
// Variables
let x = 10;
let flag = true;

// Arithmetic & comparison
let result = (x + 5) * 2;

// If / else-if / else
if (result > 20) {
    print result;
} else {
    print 0;
}

// While loop
while (x < 100) {
    x = x * 2;
}

// Built-in I/O
print x;
input x;   // reads integer from stdin
```

Supported operators: `+` `-` `*` `/` `==` `!=` `<` `<=` `>` `>=`  
Supported types: Integer, Boolean

---

## Build & Run

**Using g++ directly:**
```bash
g++ -std=c++17 -o cvm src/main.cpp src/lexer.cpp src/parser.cpp \
    src/compiler.cpp src/vm.cpp -Isrc
```

**Using CMake:**
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..   # Windows
cmake -G "Unix Makefiles" ..    # Linux / Mac
cmake --build .
```

---

## Usage

```bash
# Run a script
./cvm tests/test1_arithmetic.cvm

# Debug mode — shows Tokens, AST, Bytecode, and Execution trace
./cvm --debug tests/test1_arithmetic.cvm

# Interactive REPL
./cvm
```

**REPL example:**
```
cvm>> let x = 10;
cvm>> print x * 3;
30
cvm>> exit
```

**Debug mode output:**
```
=== TOKENS ===
  [let] [x] [=] [10] [;]

=== AST ===
Block
  LetStatement(x)
    NumberLiteral(10)

=== BYTECODE ===
  0: PUSH_INT  10
  5: STORE  'x'
  8: HALT

=== EXECUTION ===
[VM:0] PUSH_INT 10   stack: [] → [10]
[VM:5] STORE 'x'     stack: [10] → []
[VM:8] HALT
```

---

## Test Scripts

| File | What it tests | Key result |
|---|---|---|
| `test1_arithmetic.cvm` | All operators, precedence | `2+3*4 = 14` not `20` |
| `test2_variables.cvm` | `let`, assignment, booleans | Variables hold correct state |
| `test3_conditionals.cvm` | `if / else-if / else`, nested | Correct branch always taken |
| `test4_loops.cvm` | `while`, nested loops | sum=55, factorial=720 |
| `test5_fizzbuzz.cvm` | All features combined | FizzBuzz 1–20 correct |
| `test6_input.cvm` | `input`, interactive I/O | Correct math on user input |

---

## Project Structure

```
cvm/
├── src/
│   ├── lexer.h / lexer.cpp        # Tokenizer
│   ├── ast.h                      # AST node definitions
│   ├── parser.h / parser.cpp      # Recursive descent parser
│   ├── compiler.h / compiler.cpp  # Bytecode compiler + backpatching
│   ├── vm.h / vm.cpp              # Stack-based virtual machine
│   └── main.cpp                   # Entry point, REPL, debug tools
├── tests/                         # Sample .cvm scripts
├── CMakeLists.txt
└── README.md
```

---

## Key Concepts Used

- **Lexical Analysis** — character-by-character tokenization
- **Recursive Descent Parsing** — operator precedence enforced by call hierarchy
- **Abstract Syntax Trees** — structured in-memory program representation
- **Bytecode Compilation** — custom ISA with 20 opcodes
- **Jump Backpatching** — resolving forward jump addresses for if/while
- **Stack-Based VM** — fetch → decode → execute loop with operand stack
- **Little-Endian Encoding** — consistent multi-byte operand layout

---

## Why I Built This

Most developers use Python or Java without ever questioning what happens 
between writing code and the CPU executing it. This project forced me to 
answer that question from scratch — no libraries, no shortcuts.

Building CVM++ gave me a ground-up understanding of how real language 
runtimes like CPython and the JVM actually work. Every bug I fixed in the 
lexer, parser, or VM taught me something that reading a textbook never could.

*Built by Nimit Jain*
