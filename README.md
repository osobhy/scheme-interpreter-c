# Scheme Interpreter in C

This project implements a minimalist (yet robust!) Scheme interpreter written in C. It parses Scheme source code into an abstract syntax tree and evaluates expressions using lexical scoping and a small set of primitive procedures.

## Features
- Tokenizer, parser and evaluator for a subset of Scheme
- Primitive arithmetic (`+`, `-`, `*`, `/`, `modulo`) and comparison operators
- List operations such as `cons`, `car`, `cdr`, and `append`
- Special forms: `if`, `let`, and `lambda`
- Memory management through a custom `talloc` allocator to simplify cleanup

## Why use this interpreter?
This is a single binary with no external runtime dependencies that is also memory safe through the custom `talloc` which tracks allocations and frees them automatically on exit. Great for running Scheme code on the go and easy to experiment with / add new features.

## Build
```
just build
./interpreter < your-program.scm
```

`just build` compiles the interpreter with `clang` and produces an executable named `interpreter`. The program reads Scheme code from standard input or a file redirect and prints evaluation results.

## Layout
- `tokenizer.c`: converts characters into lexical tokens
- `parser.c`: builds an abstract syntax tree from tokens
- `interpreter.c`: evaluates the syntax tree in nested frames
- `talloc.c`: simple garbage collector used across the project
- `linkedlist.c`: basic list implementation used for both tokens and AST nodes
