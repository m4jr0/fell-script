# Fell tooling overlay

Generated for the tooling milestone.

Adds:

- `fell`, `fell repl`
- `fell run <file>`
- `fell compile <file> [-o <file>]`
- `fell exec <file>`
- `fell dump tokens <file>`
- `fell dump ast <file>`
- `fell dump ir <file>`
- `fell dump bytecode <file>`
- `fell -h`, `fell --help`
- `fell --version`
- explicit `.fellc` serialization with `FELL` magic and bytecode format version 1

## Suggested smoke test

Given `sample.fell`:

    1u32 + 2s64;

Run:

    fell dump tokens sample.fell
    fell dump ast sample.fell
    fell dump ir sample.fell
    fell dump bytecode sample.fell
    fell run sample.fell
    fell compile sample.fell
    fell exec sample.fellc

Both `run` and `exec` should print `3`.

## Bytecode validation

The deserializer validates magic, format version, opcode and value-type ranges,
instruction structure, trailing data, register count, and every register operand
before a module reaches the VM.
