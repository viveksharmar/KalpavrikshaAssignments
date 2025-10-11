#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define File_Name "users.txt"

struct userData {
    int id;
    char name[20];
    int age;
};

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

FILE* fileOpener(const char* filename, const char* mode) {
    FILE *file = fopen(filename, mode);
    if (!file && (strcmp(mode, "r") == 0 || strcmp(mode, "r+") == 0)) {
        file = fopen(filename, "w+");
    }

    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    return file;
}

int getValidatedID(const char *prompt) {
    int id;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &id) != 1) {
            printf("Invalid input. ID must be a positive integer.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        if (id <= 0) {
            printf("ID must be greater than 0.\n");
            continue;
        }
        return id;
    }
}

void getValidatedName(char *name, const char *prompt) {
    char input[100];
    while (1) {
        printf("%s", prompt);
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Error reading input.\n");
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        bool valid = true;
        for (int i = 0; input[i]; i++) {
            if (!isalpha(input[i]) && input[i] != ' ') {
                valid = false;
                break;
            }
        }

        if (!valid || strlen(input) == 0) {
            printf("Name must contain only alphabets and spaces.\n");
            continue;
        }

        strncpy(name, input, 20);
        name[19] = '\0';
        return;
    }
}

int getValidatedAge(const char *prompt) {
    int age;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &age) != 1) {
            printf("Invalid input. Age must be a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        if (age < 18 || age > 65) {
            printf("Age must be between 18 and 65.\n");
            continue;
        }
        return age;
    }
}

void createNewUser() {
    FILE *userFile = fileOpener(File_Name, "a+");
    struct userData actualData;
    struct userData tempData;
    int uniqueFlag = 0;

    actualData.id = getValidatedID("Enter Unique ID: ");

    rewind(userFile);
    while (fscanf(userFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age) != EOF) {
        if (actualData.id == tempData.id) {
            uniqueFlag = 1;
            break;
        }
    }

    if (uniqueFlag) {
        printf("User ID %d already exists. Try another.\n", actualData.id);
    } else {
        getValidatedName(actualData.name, "Enter Name: ");
        actualData.age = getValidatedAge("Enter Age: ");
        fprintf(userFile, "%d,%s,%d\n", actualData.id, actualData.name, actualData.age);
        printf("User created successfully.\n");
    }

    fclose(userFile);
}

void readAllData() {
    FILE *userFile = fileOpener(File_Name, "r");
    struct userData tempData;
    int found = 0;

    printf("\nList of Users:\n");
    while (fscanf(userFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age) != EOF) {
        printf("ID: %d | Name: %s | Age: %d\n", tempData.id, tempData.name, tempData.age);
        found = 1;
    }

    if (!found) {
        printf("No user records found.\n");
    }

    fclose(userFile);
}

void updateData() {
    int id = getValidatedID("Enter User ID to update: ");
    FILE *userRead = fileOpener(File_Name, "r");
    FILE *tempFile = fileOpener("temp.txt", "w");

    struct userData tempData;
    int found = 0;

    while (fscanf(userRead, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age) != EOF) {
        if (tempData.id == id) {
            found = 1;
            getValidatedName(tempData.name, "Enter New Name: ");
            tempData.age = getValidatedAge("Enter New Age: ");
        }
        fprintf(tempFile, "%d,%s,%d\n", tempData.id, tempData.name, tempData.age);
    }

    fclose(userRead);
    fclose(tempFile);

    if (found) {
        remove(File_Name);
        rename("temp.txt", File_Name);
        printf("User data updated successfully.\n");
    } else {
        remove("temp.txt");
        printf("User with ID %d not found.\n", id);
    }
}

void deleteData() {
    int id = getValidatedID("Enter User ID to delete: ");
    FILE *userRead = fileOpener(File_Name, "r");
    FILE *tempFile = fileOpener("temp.txt", "w");

    struct userData tempData;
    int found = 0;

    while (fscanf(userRead, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age) != EOF) {
        if (tempData.id == id) {
            found = 1;
            continue;
        }
        fprintf(tempFile, "%d,%s,%d\n", tempData.id, tempData.name, tempData.age);
    }

    fclose(userRead);
    fclose(tempFile);

    if (found) {
        remove(File_Name);
        rename("temp.txt", File_Name);
        printf("User deleted successfully.\n");
    } else {
        remove("temp.txt");
        printf("User with ID %d not found.\n", id);
    }
}

int main() {
    FILE *userFile = fileOpener(File_Name, "a+");
    fclose(userFile);

    int choice;
    while (true) {
        printf("\nMenu:\n");
        printf("1. Create new user\n");
        printf("2. Read all users\n");
        printf("3. Update user\n");
        printf("4. Delete user\n");
        printf("5. Exit\n");

        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: createNewUser(); break;
            case 2: readAllData(); break;
            case 3: updateData(); break;
            case 4: deleteData(); break;
            case 5:
                printf("Exiting program.\n");
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }

    return 0;
}
