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

CVM++ supports two execution modes:

**Direct mode** — compile and run in one step:
```
source.cvm  →  [Lexer → Parser → Compiler → VM]  →  output
```

**Separate mode** — compile to bytecode file, run later:
```
source.cvm  →  [Lexer → Parser → Compiler]  →  output.cvmb
output.cvmb →  [VM]  →  output
```

This is exactly how Java works: `.java` → `.class` → JVM.

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

## Build Instructions

**Requirements:**
- g++ with C++17 support (MinGW on Windows, g++ on Linux/Mac)
- CMake 3.10+ (optional)

**Using g++ directly:**
```bash
# Linux / Mac
g++ -std=c++17 -o cvm src/main.cpp src/lexer.cpp src/parser.cpp \
    src/compiler.cpp src/vm.cpp -Isrc

# Windows
mkdir build
g++ -std=c++17 -o build\cvm.exe src/main.cpp src/lexer.cpp src/parser.cpp \
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

### 1. Run a script directly
```bash
./cvm tests/fibonacci.cvm
```

### 2. Run with debug mode
Shows Tokens, AST, Bytecode disassembly, and Execution trace:
```bash
./cvm --debug tests/fibonacci.cvm
```

### 3. Compile to bytecode file (.cvmb)
Compiles source code and saves raw bytecode to a `.cvmb` file:
```bash
./cvm compile tests/fibonacci.cvm -o tests/fibonacci.cvmb
```
Output:
```
[CVM] Compiled 'tests/fibonacci.cvm'
[CVM] Bytecode saved to 'tests/fibonacci.cvmb'
[CVM] Bytecode size: 82 bytes
[CVM] Variables in name table: 4
```

### 4. Run a bytecode file on the VM
Loads the `.cvmb` file and executes it directly on the Virtual Machine:
```bash
./cvm run tests/fibonacci.cvmb
```
Output:
```
[CVM] Loaded bytecode from 'tests/fibonacci.cvmb'
[CVM] Bytecode size: 82 bytes
[CVM] Running...
0
1
1
2
3
5
8
13
21
34
[CVM] Execution complete.
```

### 5. Compile and run with debug
```bash
# See tokens, AST, and bytecode during compilation
./cvm compile --debug tests/fibonacci.cvm -o tests/fibonacci.cvmb

# See bytecode disassembly and execution trace when running
./cvm run --debug tests/fibonacci.cvmb
```

### 6. Interactive REPL
Type statements and press blank Enter to run:
```bash
./cvm
```
```
cvm>> let x = 10;
  ... let y = 20;
  ... print x + y;
  ...
30
cvm>> exit
```

---

## Command Reference

| Command | Description |
|---|---|
| `./cvm script.cvm` | Run source file directly |
| `./cvm --debug script.cvm` | Run source with full debug output |
| `./cvm compile script.cvm -o out.cvmb` | Compile source to bytecode file |
| `./cvm compile --debug script.cvm -o out.cvmb` | Compile with debug output |
| `./cvm run out.cvmb` | Run bytecode file on VM |
| `./cvm run --debug out.cvmb` | Run bytecode with debug output |
| `./cvm` | Start interactive REPL |

> On Windows replace `./cvm` with `.\build\cvm.exe` and `/` with `\`

---

## Bytecode File Format (.cvmb)

The `.cvmb` format is CVM++'s proprietary binary bytecode format:

```
Header:   4 bytes  magic "CVMB"
          4 bytes  number of name table entries
          4 bytes  number of bytecode bytes
Names:    for each variable name: 4-byte length + raw chars
Code:     raw bytecode instructions
```

This is analogous to Java's `.class` files or Python's `.pyc` files —
the source code is compiled once and the bytecode can be distributed
and run on any CVM++ VM without the original source.

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
| `fibonacci.cvm` | while, multi-variable state | 0 1 1 2 3 5 8 13 21 34 |
| `calculator.cvm` | if/else, input, arithmetic | Correct operation result |
| `truth_machine.cvm` | while, input, conditionals | 0 stops, 1 loops forever |

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
│   └── main.cpp                   # Entry point, REPL, debug tools,
│                                  # compile/run subcommands
├── tests/                         # Sample .cvm scripts
├── CMakeLists.txt
├── instructions.txt
└── README.md
```

---

## Key Concepts Used

- **Lexical Analysis** — character-by-character tokenization
- **Recursive Descent Parsing** — operator precedence enforced by call hierarchy
- **Abstract Syntax Trees** — structured in-memory program representation
- **Bytecode Compilation** — custom ISA with 21 opcodes
- **Jump Backpatching** — resolving forward jump addresses for if/while
- **Stack-Based VM** — fetch → decode → execute loop with operand stack
- **Bytecode Serialization** — save/load compiled programs as binary `.cvmb` files
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
