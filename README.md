
# crushing-four

This repository is one solution to an assignment that is part of the Computer Science study program at [Humboldt University of Berlin](https://www.hu-berlin.de/).

## Assignment

The requested C-program is basically a mixture of Connect Four and Candy Crush. On a 2D-playground of unkown width and height pieces get inserted at given X-coordinates. They stack up until a horizontal, vertical or diagonal line of 4 or more pieces of the same color (255 possible colors) gets recognized and removed. Dangling pieces fall down after all lines have been removed. The next piece is injected when no more lines could be found.

Input and output of the program happen through `stdin` and `stdout`. The format is strictly defined and invalid inputs should be handled gracefully. When the program exists (successfully or not) it should free up all the memory it requested through `malloc` or `realloc`. Valid submissions consist of a single C source file that compiles in a set environment and with set flags.

## Concept

In this solution the playground consists of a doubly-linked list of columns. Each column contains a dynamic array of piece colors. To efficiently store empty columns in a row a special 'padding column' is used.

## Development

### Build

Run build script to compile the C program to `./loesung`.

```bash
./bin/make.sh
```

### Test

Tests are defined in the `tests` folder with each of them having a `*.stdin` and `*.stdout` file.

To run all the tests use:

```bash
./bin/test.sh
```

Verify program memory management using `valgrind`:

```bash
valgrind --leak-check=full ./loesung < ./tests/109-example.stdin
```
