#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 100

struct Student{
    int rollNumber;
    char name[100];
    double marks[3];
    double total;
    double average;
    char grade;
};

int isValidRollNumber(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) return 0;
    }
    int num = atoi(str);
    return num >= 1;
}

int isValidName(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    for (int i = 0; str[i]; i++) {
        if (!isalpha(str[i]) && str[i] != ' ') return 0;
    }
    return 1;
}

int isValidMark(const char *str, double *value) {
    char *end;
    *value = strtod(str, &end);
    if (*end != '\0') return 0;
    return *value >= 0 && *value <= 100;
}

char getGrade(double avg) {
    if (avg >= 85) return 'A';
    else if (avg >= 70) return 'B';
    else if (avg >= 50) return 'C';
    else if (avg >= 35) return 'D';
    else return 'F';
}

void printPerformance(char grade) {
    if (grade == 'A') printf("Performance: *****\n");
    else if (grade == 'B') printf("Performance: ****\n");
    else if (grade == 'C') printf("Performance: ***\n");
    else if (grade == 'D') printf("Performance: **\n");
    else return;
}

int isDuplicateRollNumber(int roll, int list[], int count) {
    for (int i = 0; i < count; i++) {
        if (list[i] == roll) return 1;
    }
    return 0;
}

void sortRollNumbers(int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

void printRollNumbers(int rolls[], int index, int size) {
    if (index == size) {
        printf("\n");
        return;
    }
    printf("%d ", rolls[index]);
    printRollsRecursive(rolls, index + 1, size);
}

int main() {
    int studentCount;
    struct Student students[MAX];
    int rolls[MAX];
    char input[256];
    char rollStr[20], mark1[20], mark2[20], mark3[20];
    char firstName[50], lastName[50];

    printf("\nEnter number of students: ");
    while (scanf("%d", &studentCount) != 1 || studentCount <= 0 || studentCount > MAX) {
        printf("Invalid number. Please enter a positive number (max %d): ", MAX);
        while (getchar() != '\n');
    }
    getchar();

    for (int i = 0; i < studentCount; i++) {
        while (1) {
            printf("\nFormat of Input--> Roll_No First_Name Last_Name Mark_1 Mark_2 Mark_3\n");
            printf("\nEnter data for student %d (e.g. format, 1 Vivek Sharma 75 80 95):\n", i + 1);

            if (!fgets(input, sizeof(input), stdin)) {
                printf("Invalid input. Try again.\n");
                continue;
            }

            input[strcspn(input, "\n")] = 0;

            int scanned = sscanf(input, "%19s %49s %49s %19s %19s %19s", rollStr, firstName, lastName, mark1, mark2, mark3);

            if (scanned != 6) {
                printf("Please enter exactly 6 values.\n");
                continue;
            }

            int roll = atoi(rollStr);

            if (!isValidRollNumber(rollStr)) {
                printf("Roll Number must be a positive integer.\n");
                continue;
            }

            if (isDuplicateRollNumber(roll, rolls, i)) {
                printf("Roll Number already exists. Please enter a unique roll number.\n");
                continue;
            }

            char fullName[100];
            snprintf(fullName, sizeof(fullName), "%s %s", firstName, lastName);

            if (!isValidName(fullName)) {
                printf("Name must contain only alphabetic characters and spaces.\n");
                continue;
            }

            double m1, m2, m3;

            if (!isValidMark(mark1, &m1) || !isValidMark(mark2, &m2) || !isValidMark(mark3, &m3)) {
                printf("Marks must be numeric values between 0 and 100.\n");
                continue;
            }

            students[i].rollNumber = roll;
            strcpy(students[i].name, fullName);
            students[i].marks[0] = m1;
            students[i].marks[1] = m2;
            students[i].marks[2] = m3;
            students[i].total = m1 + m2 + m3;
            students[i].average = students[i].total / 3.0;
            students[i].grade = getGrade(students[i].average);
            rolls[i] = roll;

            break;
        }
    }

    printf("\n===========================\n\n");

    for (int i = 0; i < studentCount; i++) {
        printf("Roll Number: %d\n", students[i].rollNumber);
        printf("Name: %s\n", students[i].name);
        printf("Total Marks: %.2f\n", students[i].total);
        printf("Average Marks: %.2f\n", students[i].average);
        printf("Grade: %c\n", students[i].grade);
        if (students[i].grade != 'F') {
            printPerformance(students[i].grade);
        }
        printf("\n");
    }

    sortRollNumbers(rolls, studentCount);

    printf("List of Roll Numbers: ");
    printRollNumbers(rolls, 0, studentCount);

    return 0;
}
