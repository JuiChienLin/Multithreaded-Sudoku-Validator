#!/bin/bash

set -u

# Build the Sudoku validator.
echo "Building Sudoku validator..."

if ! clang -Wall -Wextra -std=c11 -pthread \
    class_project.c -o sudoku_validator; then
    echo "[FAIL] Compilation failed"
    exit 1
fi

passed=0
failed=0

run_test() {
    local name="$1"
    local file="$2"
    local expected_exit="$3"
    local expected_text="$4"

    local output
    local actual_exit

    output=$(./sudoku_validator "$file")
    actual_exit=$?

    if [[ "$actual_exit" -eq "$expected_exit" &&
          "$output" == *"$expected_text"* ]]; then
        echo "[PASS] $name"
        ((passed += 1))
    else
        echo "[FAIL] $name"
        echo "  Expected exit: $expected_exit"
        echo "  Actual exit:   $actual_exit"
        echo "  Expected text: $expected_text"
        echo "  Actual output: $output"
        ((failed += 1))
    fi
}

run_test "Valid Sudoku" \
    "puzzles/valid.txt" 0 \
    "Sudoku solution is VALID"

run_test "Invalid Row" \
    "puzzles/invalid_row.txt" 1 \
    "Row check failed at row 8"

run_test "Invalid Column" \
    "puzzles/invalid_column.txt" 1 \
    "Column check failed at column 1"

run_test "Invalid Subgrid" \
    "puzzles/invalid_subgrid.txt" 1 \
    "Subgrid check failed at subgrid 1"

echo
echo "Test summary: PASS=$passed FAIL=$failed"

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi

exit 0
