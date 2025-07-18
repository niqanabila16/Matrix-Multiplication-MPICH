#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_SIZE 3000

int *allocate1DMatrix(int rows, int cols) {
    return (int *)malloc(rows * cols * sizeof(int));
}

float *allocate1DFloatMatrix(int rows, int cols) {
    return (float *)malloc(rows * cols * sizeof(float));
}

void readCSV(const char *filename, int *matrix, int *rows, int *cols, int maxCols) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Tidak bisa membuka file %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char line[65536];
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        int j = 0;
        char *token = strtok(line, ",");
        while (token && j < maxCols) {
            matrix[i * maxCols + j] = atoi(token);
            token = strtok(NULL, ",");
            j++;
        }
        i++;
        *cols = j;
    }
    *rows = i;
    fclose(file);
}

void saveCSV(const char *filename, float *matrix, int rows, int cols) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: Tidak bisa membuka file %s untuk ditulis\n", filename);
        return;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%.2f", matrix[i * cols + j]);
            if (j < cols - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    int rank, size;
    int rowsA = 0, colsA = 0, rowsB = 0, colsB = 0;

    int *matA = NULL;
    int *matB = NULL;
    float *result = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start_time, end_time;

    if (rank == 0) {
        matA = allocate1DMatrix(MAX_SIZE, MAX_SIZE);
        matB = allocate1DMatrix(MAX_SIZE, MAX_SIZE);
        result = allocate1DFloatMatrix(MAX_SIZE, MAX_SIZE);

        readCSV("file_1.csv", matA, &rowsA, &colsA, MAX_SIZE);
        readCSV("file_2.csv", matB, &rowsB, &colsB, MAX_SIZE);

        if (colsA != rowsB) {
            printf("Error: Kolom matriks A harus sama dengan baris matriks B.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&rowsA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&colsA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&colsB, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        matB = allocate1DMatrix(MAX_SIZE, MAX_SIZE);  // supaya semua punya matB
    }

    MPI_Bcast(matB, MAX_SIZE * MAX_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

    int rows_per_proc = rowsA / size;
    int rem = rowsA % size;
    int start_row = rank * rows_per_proc + (rank < rem ? rank : rem);
    int end_row = start_row + rows_per_proc + (rank < rem ? 1 : 0);
    int my_rows = end_row - start_row;

    int *my_matA = allocate1DMatrix(my_rows, MAX_SIZE);
    float *my_result = allocate1DFloatMatrix(my_rows, MAX_SIZE);

    if (rank == 0) {
        for (int r = 1; r < size; r++) {
            int s_row = r * rows_per_proc + (r < rem ? r : rem);
            int e_row = s_row + rows_per_proc + (r < rem ? 1 : 0);
            int n_rows = e_row - s_row;
            MPI_Send(&matA[s_row * MAX_SIZE], n_rows * MAX_SIZE, MPI_INT, r, 0, MPI_COMM_WORLD);
        }

        memcpy(my_matA, &matA[start_row * MAX_SIZE], my_rows * MAX_SIZE * sizeof(int));
    } else {
        MPI_Recv(my_matA, my_rows * MAX_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    for (int i = 0; i < my_rows; i++) {
        for (int j = 0; j < colsB; j++) {
            float sum = 0.0;
            for (int k = 0; k < colsA; k++) {
                sum += my_matA[i * MAX_SIZE + k] * matB[k * MAX_SIZE + j];
            }
            my_result[i * MAX_SIZE + j] = sum;
        }
    }

    if (rank == 0) {
        memcpy(&result[start_row * MAX_SIZE], my_result, my_rows * MAX_SIZE * sizeof(float));
        for (int r = 1; r < size; r++) {
            int s_row = r * rows_per_proc + (r < rem ? r : rem);
            int e_row = s_row + rows_per_proc + (r < rem ? 1 : 0);
            int n_rows = e_row - s_row;
            MPI_Recv(&result[s_row * MAX_SIZE], n_rows * MAX_SIZE, MPI_FLOAT, r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        printf("Waktu eksekusi: %.6f detik\n", MPI_Wtime() - start_time);
        saveCSV("output.csv", result, rowsA, colsB);
        printf("Hasil disimpan ke output.csv\n");
    } else {
        MPI_Send(my_result, my_rows * MAX_SIZE, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    free(my_matA);
    free(my_result);
    if (rank == 0) {
        free(matA);
        free(matB);
        free(result);
    } else {
        free(matB);
    }

    MPI_Finalize();
    return 0;
}
