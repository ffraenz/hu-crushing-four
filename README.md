# crushing-candy

## Build

Run build script:

```bash
./make.sh
```

## Tests

Tests are defined in the `tests` folder with each of them having a `*.stdin`
and `*.stdout` file.

Run all the tests:

```bash
./test.sh
```

Show the diff of a single test:

```bash
cat ./tests/109-example.stdin | ./loesung | sort | diff <(sort ./tests/109-example.stdout) -
```

Verify program memory management using `valgrind`:

```bash
valgrind --leak-check=full ./loesung < ./tests/109-example.stdin
```
