// 判斷一個 9x9 數獨棋盤是否為有效解答
// 使用多執行緒來同時檢查列、行及 3x3 子區塊
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define SIZE 9
#define NUM_THREADS 11   // 0: rows, 1: columns, 2~10: subgrids

/* ========= 9x9 數獨棋盤 ========= */
int sudoku[SIZE][SIZE] = {
    {6,2,4,5,3,9,1,8,7},
    {5,1,9,7,2,8,6,3,4},
    {8,3,7,6,1,4,2,9,5},
    {1,4,3,8,6,5,7,2,9},
    {9,5,8,2,4,7,3,6,1},
    {7,6,2,3,9,1,4,5,8},
    {3,7,1,9,5,6,8,4,2},  
    {4,9,6,1,8,2,5,4,3},
    {2,8,5,4,7,3,9,1,5}   
};/* Load a 9x9 Sudoku board from a text file. */
int load_sudoku(const char *filename) {
    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        perror(filename);
        return -1;
    }

    int board[SIZE][SIZE];

    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (fscanf(fp, "%d", &board[r][c]) != 1) {
                fprintf(stderr,
                        "Invalid Sudoku input at row %d, column %d\n",
                        r + 1, c + 1);
                fclose(fp);
                return -1;
            }
        }
    }

    /* Reject extra data after the 81 integers. */
    char extra;
    if (fscanf(fp, " %c", &extra) == 1) {
        fprintf(stderr, "Invalid input: extra data after Sudoku board\n");
        fclose(fp);
        return -1;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Error reading Sudoku file\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    memcpy(sudoku, board, sizeof(board));
    return 0;
}

/* ========= 各 thread 檢查結果 ========= */
int results[NUM_THREADS];      // 1 = OK, 0 = FAIL
int error_unit[NUM_THREADS];   // 出錯的 row 編號

/* ========= 傳遞給 thread 的參數 ========= */
typedef struct { 
    int row;
    int column;
    int index;   // results[] 和 error_unit[] 的索引
} parameters;

/* ========= 檢查所有 rows ========= */
void *check_rows(void *param) {
    parameters *data = (parameters *)param;

    for (int r = 0; r < SIZE; r++) {    // 檢查共九列
        int used[SIZE] = {0};
        for (int c = 0; c < SIZE; c++) {
            int num = sudoku[r][c];
            if (num < 1 || num > 9 || used[num - 1]) { // 重複或不在 1~9 範圍內
                results[data->index] = 0;
                error_unit[data->index] = r;   // 第 r 列出錯
                pthread_exit(NULL);   // 提早結束 thread
            }
            used[num - 1] = 1; //若沒問題，標記該數字已出現過
        }
    }

    results[data->index] = 1; // 全部九列皆通過檢查, results 設為 1
    pthread_exit(NULL);
}

/* ========= 檢查所有 columns ========= */
void *check_columns(void *param) {
    parameters *data = (parameters *)param;

    for (int c = 0; c < SIZE; c++) {
        int used[SIZE] = {0};
        for (int r = 0; r < SIZE; r++) {
            int num = sudoku[r][c];
           if (num < 1 || num > 9 || used[num - 1]) {
                results[data->index] = 0;
                error_unit[data->index] = c;
                pthread_exit(NULL);
            }
            used[num - 1] = 1;
        }
    }

    results[data->index] = 1;
    pthread_exit(NULL);
}

/* ========= 檢查單一 3x3 subgrid ========= */
void *check_subgrid(void *param) {
    parameters *data = (parameters *)param;
    int used[SIZE] = {0};

    for (int i = 0; i < 3; i++) {  
        for (int j = 0; j < 3; j++) {
            int num = sudoku[data->row + i][data->column + j];
            if (num < 1 || num > 9 || used[num - 1]) {
                results[data->index] = 0;
                error_unit[data->index] = data->index - 2;
                pthread_exit(NULL);
            }
            used[num - 1] = 1;
        }
    }

    results[data->index] = 1;
    pthread_exit(NULL);
}

int create_thread(pthread_t *thread,
                  void *(*function)(void *),
                  void *argument)
{
    int ret = pthread_create(thread, NULL, function, argument);

    if (ret != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
    }

    return ret;
}
/* ========= 主程式 ========= */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sudoku-file>\n", argv[0]);
        return 1;
    }

    if (load_sudoku(argv[1]) != 0) {
        return 1;
    }
    pthread_t threads[NUM_THREADS];
    parameters params[NUM_THREADS] = {0};
    int thread_id = 0;
    int failed = 0;

    // Initialize shared results.
    for (int i = 0; i < NUM_THREADS; i++) {
        results[i] = 1;
        error_unit[i] = -1;
    }

    /* --- rows --- */
    params[thread_id].index = thread_id;

    if (create_thread(&threads[thread_id],
                      check_rows,
                      &params[thread_id]) != 0) {
        failed = 1;
        goto join_threads;
    }
    thread_id++;

    /* --- columns --- */
    params[thread_id].index = thread_id;

    if (create_thread(&threads[thread_id],
                      check_columns,
                      &params[thread_id]) != 0) {
        failed = 1;
        goto join_threads;
    }
    thread_id++;

    /* --- subgrids --- */
    for (int r = 0; r < SIZE; r += 3) {
        for (int c = 0; c < SIZE; c += 3) {
            params[thread_id].row = r;
            params[thread_id].column = c;
            params[thread_id].index = thread_id;

            if (create_thread(&threads[thread_id],
                              check_subgrid,
                              &params[thread_id]) != 0) {
                failed = 1;
                goto join_threads;
            }
            thread_id++;
        }
    }

join_threads:
    // Join only successfully created threads.
    for (int i = 0; i < thread_id; i++) {
        int ret = pthread_join(threads[i], NULL);

        if (ret != 0) {
            fprintf(stderr, "pthread_join failed: %s\n",
                    strerror(ret));
            failed = 1;
        }
    }

    if (failed) {
        fprintf(stderr, "Thread execution failed.\n");
        return 1;
    }

    /* --- Check validation results --- */
    for (int i = 0; i < NUM_THREADS; i++) {
        if (results[i] == 0) {
            printf("Sudoku solution is INVALID ❌\n");

            if (i == 0) {
                printf("Row check failed at row %d\n",
                    error_unit[i] + 1);

            } else if (i == 1) {
                printf("Column check failed at column %d\n",
                    error_unit[i] + 1);

            } else {
                int grid = error_unit[i];

                int start_row = (grid / 3) * 3 + 1;
                int start_col = (grid % 3) * 3 + 1;

                printf("Subgrid check failed at subgrid %d "
                    "(rows %d-%d, columns %d-%d)\n",
                    grid + 1,
                    start_row, start_row + 2,
                    start_col, start_col + 2);
            }

            return 1;
        }
    }

    printf("Sudoku solution is VALID ✅\n");
    return 0;
}
