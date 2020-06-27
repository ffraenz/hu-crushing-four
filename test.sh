#!/bin/bash

# Build target
./make.sh

# Temporary stdout write location
TEST_STDOUT=./test.stdout

# Iterate through tests
for CASE_STDIN_PATH in ./tests/*.stdin
do
  CASE_NAME="$(basename ${CASE_STDIN_PATH%.*})"
  CASE_STDOUT_PATH="${CASE_STDIN_PATH%.*}.stdout"

  # Run test
  cat $CASE_STDIN_PATH | time ./loesung | sort > $TEST_STDOUT

  # Diff result
  CASE_DIFF=$(diff <(sort $CASE_STDOUT_PATH) $TEST_STDOUT)

  # Check if diff empty
  if [ "$CASE_DIFF" != "" ]
  then
    DIFF_DELTA="$(wc -l <<< "$CASE_DIFF")"
    echo -e "\x1B[1;31m✕ Test $CASE_NAME\x1B[0m (Delta: $DIFF_DELTA)"
  else
    echo -e "\x1B[1;32m✓ Test $CASE_NAME\x1B[0m"
  fi
done

# Clean up
rm $TEST_STDOUT
