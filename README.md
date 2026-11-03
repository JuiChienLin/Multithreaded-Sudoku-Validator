# Multithreaded Sudoku Validator

A 9×9 Sudoku solution validator implemented in C using POSIX threads (pthreads).

The program validates a completed Sudoku board by checking its rows, columns, and 3×3 subgrids concurrently.

## Features

- Uses 11 POSIX threads for concurrent validation.
- Validates all 9 rows, all 9 columns, and all 9 subgrids.
- Reads Sudoku boards from external text files.
- Detects duplicate or out-of-range numbers.
- Reports the location of a validation failure.
- Handles thread creation and joining errors.
- Includes an automated Bash test script.

## Thread Architecture

The program creates 11 worker threads:

| Thread | Responsibility |
|---|---|
| 1 | Validate all 9 rows |
| 2 | Validate all 9 columns |
| 3–11 | Validate one 3×3 subgrid per thread |

Each worker stores its validation result in a dedicated entry of a shared results array.

The main thread waits for the workers using `pthread_join()` before examining their results.

## Requirements

- A C11-compatible compiler, such as Clang or GCC
- POSIX threads support
- Bash for running automated tests

## Build

Compile the program:

```bash
clang -Wall -Wextra -std=c11 -pthread class_project.c -o sudoku_validator
```

## Usage

Run the validator with a Sudoku text file:

```bash
./sudoku_validator puzzles/valid.txt
```

Expected output:

```text
Sudoku solution is VALID ✅
```

For an invalid Sudoku:

```bash
./sudoku_validator puzzles/invalid_row.txt
```

Expected output:

```text
Sudoku solution is INVALID ❌
Row check failed at row 8
```

## Input Format

Each input file must contain a 9×9 Sudoku board with 81 integers separated by whitespace.

Example:

```text
5 3 4 6 7 8 9 1 2
6 7 2 1 9 5 3 4 8
1 9 8 3 4 2 5 6 7
8 5 9 7 6 1 4 2 3
4 2 6 8 5 3 7 9 1
7 1 3 9 2 4 8 5 6
9 6 1 5 3 7 2 8 4
2 8 7 4 1 9 6 3 5
3 4 5 2 8 6 1 7 9
```

## Automated Testing

Run all tests:

```bash
./run_tests.sh
```

The script compiles the program and checks four test cases:

| Test Case | Expected Result |
|---|---|
| Valid Sudoku | VALID |
| Invalid Row | Row 8 |
| Invalid Column | Column 1 |
| Invalid Subgrid | Subgrid 1 |

Expected summary:

```text
Test summary: PASS=4 FAIL=0
```

## Exit Codes

| Code | Meaning |
|---|---|
| 0 | Sudoku board is valid |
| 1 | Invalid board, input error, or thread operation failure |

## Implementation Notes

- The validator uses a shared 9×9 array for the Sudoku board.
- Each worker thread writes to a separate result slot.
- The main thread joins the workers before evaluating the results.
- When multiple violations exist, the program reports the first failure according to the result-array order.
- Multithreading demonstrates POSIX thread coordination; it does not necessarily improve performance for a small 9×9 board.

## Future Improvements

- Expand tests for malformed and incomplete input files.
- Add fault-injection tests for thread error handling.
- Improve separation between validation logic and user interface.

## Background

Originally developed as an operating systems course project and subsequently extended with file-based input, error handling, detailed validation reporting, and automated testing.
