#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10

int getMatrixSize() {
    int matrixSize;
    char inputLine[100];

    while (1) {
        printf("Enter matrix size (2-10): ");
        if (!fgets(inputLine, sizeof(inputLine), stdin)) {
            printf("Input error. Try again.\n");
            continue;
        }

        char extraChar;
        if (sscanf(inputLine, "%d %c", &matrixSize, &extraChar) != 1 || matrixSize < 2 || matrixSize > 10) {
            printf("Values could be integral only in range 2-10\n");
            continue;
        }

        return matrixSize;
    }
}

void printMatrix(unsigned char (*matrix)[MAX], int matrixSize) {
    for (int rowIndex = 0; rowIndex < matrixSize; rowIndex++) {
        unsigned char* currentRow = *(matrix + rowIndex);
        for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
            printf("%4d", *(currentRow + columnIndex));
        }
        printf("\n");
    }
}

void rotateMatrixClockwise(unsigned char (*matrix)[MAX], int matrixSize) {
    int firstIndex = 0;
    int lastIndex = 0;
    int offsetIndex = 0;
    for (int layerIndex = 0; layerIndex < matrixSize / 2; layerIndex++) {
        firstIndex = layerIndex;
        lastIndex = matrixSize - 1 - layerIndex;
        for (int currentIndex = firstIndex; currentIndex < lastIndex; currentIndex++) {
            offsetIndex = currentIndex - firstIndex;

            unsigned char* topElement = (*(matrix + firstIndex)) + currentIndex;
            unsigned char* leftElement = (*(matrix + (lastIndex - offsetIndex))) + firstIndex;
            unsigned char* bottomElement = (*(matrix + lastIndex)) + (lastIndex - offsetIndex);
            unsigned char* rightElement = (*(matrix + currentIndex)) + lastIndex;

            unsigned char temporaryValue = *topElement;
            *topElement = *leftElement;
            *leftElement = *bottomElement;
            *bottomElement = *rightElement;
            *rightElement = temporaryValue;
        }
    }
}

void applySmoothingFilter(unsigned char (*matrix)[MAX], int matrixSize) {
    unsigned char currentRowAverages[MAX];
    unsigned char previousRowAverages[MAX];

    int valueSum = 0;
    int neighborCount = 0;
    int neighborRow = 0;
    int neighborColumn = 0;


    for (int rowIndex = 0; rowIndex < matrixSize; rowIndex++) {
        for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
            valueSum = 0;
            neighborCount = 0;

            for (int rowOffset = -1; rowOffset <= 1; rowOffset++) {
                neighborRow = rowIndex + rowOffset;
                if (neighborRow < 0 || neighborRow >= matrixSize) continue;

                for (int columnOffset = -1; columnOffset <= 1; columnOffset++) {
                    neighborColumn = columnIndex + columnOffset;
                    if (neighborColumn < 0 || neighborColumn >= matrixSize) continue;

                    valueSum += *(*(matrix + neighborRow) + neighborColumn);
                    neighborCount++;
                }
            }

            *(currentRowAverages + columnIndex) = valueSum / neighborCount;
        }

        if (rowIndex > 0) {
            for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
                *(*(matrix + rowIndex - 1) + columnIndex) = *(previousRowAverages + columnIndex);
            }
        }

        for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
            *(previousRowAverages + columnIndex) = *(currentRowAverages + columnIndex);
        }
    }

    for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
        *(*(matrix + matrixSize - 1) + columnIndex) = *(previousRowAverages + columnIndex);
    }
}

int main() {
    int matrixSize = getMatrixSize();
    unsigned char matrix[MAX][MAX];

    srand((unsigned int)time(NULL));

    for (int rowIndex = 0; rowIndex < matrixSize; rowIndex++) {
        unsigned char* currentRow = *(matrix + rowIndex);
        for (int columnIndex = 0; columnIndex < matrixSize; columnIndex++) {
            *(currentRow + columnIndex) = rand() % 256;
        }
    }

    printf("\nOriginal:\n");
    printMatrix(matrix, matrixSize);

    rotateMatrixClockwise(matrix, matrixSize);

    printf("\nRotated:\n");
    printMatrix(matrix, matrixSize);

    applySmoothingFilter(matrix, matrixSize);

    printf("\nFinal Output:\n");
    printMatrix(matrix, matrixSize);

    return 0;
}
