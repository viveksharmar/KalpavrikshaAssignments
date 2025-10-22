#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10

int getSize() {
    int n;
    char line[100];

    while (1) {
        printf("Enter matrix size (2-10): ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("Input error. Try again.\n");
            continue;
        }

        char extra;
        if (sscanf(line, "%d %c", &n, &extra) != 1 || n < 2 || n > 10) {
            printf("Values could be integral only in range 2-10\n");
            continue;
        }

        return n;
    }
}

void print(unsigned char (*mat)[MAX], int n) {
    for (int i = 0; i < n; i++) {
        unsigned char* row = *(mat + i);
        for (int j = 0; j < n; j++) {
            printf("%4d", *(row + j));
        }
        printf("\n");
    }
}

void rotate(unsigned char (*mat)[MAX], int n) {
    for (int layer = 0; layer < n / 2; layer++) {
        int first = layer;
        int last = n - 1 - layer;
        for (int i = first; i < last; i++) {
            int offset = i - first;

            unsigned char* a = (*(mat + first)) + i;
            unsigned char* b = (*(mat + (last - offset))) + first;
            unsigned char* c = (*(mat + last)) + (last - offset);
            unsigned char* d = (*(mat + i)) + last;

            unsigned char temp = *a;
            *a = *b;
            *b = *c;
            *c = *d;
            *d = temp;
        }
    }
}

void smooth(unsigned char (*mat)[MAX], int n) {
    unsigned char currRow[MAX];
    unsigned char prevRow[MAX];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int sum = 0;
            int count = 0;

            for (int dx = -1; dx <= 1; dx++) {
                int ni = i + dx;
                if (ni < 0 || ni >= n) continue;

                for (int dy = -1; dy <= 1; dy++) {
                    int nj = j + dy;
                    if (nj < 0 || nj >= n) continue;

                    sum += *(*(mat + ni) + nj);
                    count++;
                }
            }

            *(currRow + j) = sum / count;
        }

        if (i > 0) {
            for (int j = 0; j < n; j++) {
                *(*(mat + i - 1) + j) = *(prevRow + j);
            }
        }

        for (int j = 0; j < n; j++) {
            *(prevRow + j) = *(currRow + j);
        }
    }

    for (int j = 0; j < n; j++) {
        *(*(mat + n - 1) + j) = *(prevRow + j);
    }
}

int main() {
    int n = getSize();
    unsigned char matrix[MAX][MAX];

    srand((unsigned int)time(NULL));

    for (int i = 0; i < n; i++) {
        unsigned char* row = *(matrix + i);
        for (int j = 0; j < n; j++) {
            *(row + j) = rand() % 256;
        }
    }

    printf("\nOriginal:\n");
    print(matrix, n);

    rotate(matrix, n);

    printf("\nRotated:\n");
    print(matrix, n);

    smooth(matrix, n);

    printf("\nFinal Output:\n");
    print(matrix, n);

    return 0;
}