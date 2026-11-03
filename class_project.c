#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define SIZE 9
#define SUBGRID_SIZE 3
#define NUM_THREADS 11
#define FIRST_SUBGRID_THREAD 2

/* The board is loaded before any worker thread is started. */
static int sudoku[SIZE][SIZE];

/* Each worker writes only to its own result slot. */
static int results[NUM_THREADS];
static int error_unit[NUM_THREADS];

typedef struct {
    int row;
    int column;
    int index;
} ThreadParams;

/* Read exactly SIZE * SIZE integers from the input file. */
static int load_sudoku(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror(filename);
        return -1;
    }

    int board[SIZE][SIZE];

    for (int row = 0; row < SIZE; row++) {
        for (int column = 0; column < SIZE; column++) {
            if (fscanf(file, "%d", &board[row][column]) != 1) {
                fprintf(stderr,
                        "Invalid Sudoku input at row %d, column %d\n",
                        row + 1, column + 1);
                fclose(file);
                return -1;
            }
        }
    }

    /* Reject any non-whitespace content after the board. */
    char extra;
    int scan_result = fscanf(file, " %c", &extra);
    if (scan_result == 1) {
        fprintf(stderr, "Invalid input: extra data after Sudoku board\n");
        fclose(file);
        return -1;
    }
    if (ferror(file)) {
        fprintf(stderr, "Error reading Sudoku file: %s\n", strerror(errno));
        fclose(file);
        return -1;
    }
    if (fclose(file) == EOF) {
        perror("fclose");
        return -1;
    }

    memcpy(sudoku, board, sizeof(board));
    return 0;
}

/* Validate all rows. */
static void *check_rows(void *argument)
{
    const ThreadParams *params = argument;

    for (int row = 0; row < SIZE; row++) {
        int used[SIZE] = {0};

        for (int column = 0; column < SIZE; column++) {
            int value = sudoku[row][column];
            if (value < 1 || value > SIZE || used[value - 1]) {
                results[params->index] = 0;
                error_unit[params->index] = row;
                return NULL;
            }
            used[value - 1] = 1;
        }
    }

    results[params->index] = 1;
    return NULL;
}

/* Validate all columns. */
static void *check_columns(void *argument)
{
    const ThreadParams *params = argument;

    for (int column = 0; column < SIZE; column++) {
        int used[SIZE] = {0};

        for (int row = 0; row < SIZE; row++) {
            int value = sudoku[row][column];
            if (value < 1 || value > SIZE || used[value - 1]) {
                results[params->index] = 0;
                error_unit[params->index] = column;
                return NULL;
            }
            used[value - 1] = 1;
        }
    }

    results[params->index] = 1;
    return NULL;
}

/* Validate one 3x3 subgrid. */
static void *check_subgrid(void *argument)
{
    const ThreadParams *params = argument;
    int used[SIZE] = {0};

    for (int row = 0; row < SUBGRID_SIZE; row++) {
        for (int column = 0; column < SUBGRID_SIZE; column++) {
            int value = sudoku[params->row + row][params->column + column];
            if (value < 1 || value > SIZE || used[value - 1]) {
                results[params->index] = 0;
                error_unit[params->index] = params->index - FIRST_SUBGRID_THREAD;
                return NULL;
            }
            used[value - 1] = 1;
        }
    }

    results[params->index] = 1;
    return NULL;
}

static int create_thread(pthread_t *thread,
                         void *(*worker)(void *),
                         void *argument)
{
    int error = pthread_create(thread, NULL, worker, argument);
    if (error != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(error));
    }
    return error;
}

static void print_failure(int index)
{
    puts("Sudoku solution is INVALID");

    if (index == 0) {
        printf("Row check failed at row %d\n", error_unit[index] + 1);
    } else if (index == 1) {
        printf("Column check failed at column %d\n", error_unit[index] + 1);
    } else {
        int grid = error_unit[index];
        int first_row = (grid / SUBGRID_SIZE) * SUBGRID_SIZE + 1;
        int first_column = (grid % SUBGRID_SIZE) * SUBGRID_SIZE + 1;

        printf("Subgrid check failed at subgrid %d "
               "(rows %d-%d, columns %d-%d)\n",
               grid + 1,
               first_row, first_row + SUBGRID_SIZE - 1,
               first_column, first_column + SUBGRID_SIZE - 1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sudoku-file>\n", argv[0]);
        return 1;
    }
    if (load_sudoku(argv[1]) != 0) {
        return 1;
    }

    pthread_t threads[NUM_THREADS];
    ThreadParams params[NUM_THREADS] = {0};
    int created_count = 0;
    int thread_failed = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        error_unit[i] = -1;
    }

    /* Thread 0 checks all rows. */
    params[created_count].index = created_count;
    if (create_thread(&threads[created_count], check_rows,
                      &params[created_count]) != 0) {
        thread_failed = 1;
        goto join_threads;
    }
    created_count++;

    /* Thread 1 checks all columns. */
    params[created_count].index = created_count;
    if (create_thread(&threads[created_count], check_columns,
                      &params[created_count]) != 0) {
        thread_failed = 1;
        goto join_threads;
    }
    created_count++;

    /* Threads 2 through 10 each check one subgrid. */
    for (int row = 0; row < SIZE; row += SUBGRID_SIZE) {
        for (int column = 0; column < SIZE; column += SUBGRID_SIZE) {
            params[created_count].row = row;
            params[created_count].column = column;
            params[created_count].index = created_count;

            if (create_thread(&threads[created_count], check_subgrid,
                              &params[created_count]) != 0) {
                thread_failed = 1;
                goto join_threads;
            }
            created_count++;
        }
    }

join_threads:
    /* Only join successfully created threads. */
    for (int i = 0; i < created_count; i++) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(error));
            thread_failed = 1;
        }
    }

    if (thread_failed) {
        fputs("Thread execution failed.\n", stderr);
        return 1;
    }

    /* Report the first failure in worker index order. */
    for (int i = 0; i < NUM_THREADS; i++) {
        if (results[i] == 0) {
            print_failure(i);
            return 1;
        }
    }

    puts("Sudoku solution is VALID");
    return 0;
}
