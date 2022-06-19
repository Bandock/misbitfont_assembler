# MisbitFont Assembler

A command-line based assembler that is designed to assemble valid MisbitFont files from compatible text files.  Any text editor can be used for this purpose, as long it can produce ANSI/UTF-8 compatible text files.

The manual provided is a good way to get started.

## Requirements for Building

- [CMake](https://www.cmake.org/download/) (at least 3.10)
- [fmt](https://github.com/fmtlib/fmt)
- [libmsbtfont](https://github.com/Bandock/libmsbtfont) (Compatible with 0.1, latest release is 0.2)
- C++ Compiler with C++20 Support (Primarily for UTF-8 support, though likely to work on slightly earlier C++ standards.)

## Documentation

- [MisbitFont Assembler Manual](docs/Manual.md)
