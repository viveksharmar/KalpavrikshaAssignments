#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define File_Name "users.txt"

struct userData {
    int id;
    char name[20];
    int age;
};

void createNewUser() {
    FILE *userFile = fopen(File_Name, "a+");
    struct userData actualData;
    struct userData tempData;
    int uniqueFlag = 0;

    printf("Enter Unique ID: ");
    scanf("%d", &actualData.id);

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
        getchar();
        printf("Enter Name: ");
        fgets(actualData.name, sizeof(actualData.name), stdin);
        actualData.name[strcspn(actualData.name, "\n")] = '\0';

        printf("Enter Age: ");
        scanf("%d", &actualData.age);

        fprintf(userFile, "%d,%s,%d\n", actualData.id, actualData.name, actualData.age);
        printf("User created successfully.\n");
    }

    fclose(userFile);
}

void readAllData() {
    FILE *userFile = fopen(File_Name, "r");
    if (!userFile) {
        printf("File not found.\n");
        return;
    }

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
    int id, found = 0;
    printf("Enter User ID to update: ");
    scanf("%d", &id);

    FILE *userRead = fopen(File_Name, "r");
    FILE *tempFile = fopen("temp.txt", "w");

    if (!userRead || !tempFile) {
        printf("Error opening files.\n");
        return;
    }

    struct userData tempData;
    while (fscanf(userRead, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age) != EOF) {
        if (tempData.id == id) {
            found = 1;
            getchar();
            printf("Enter New Name: ");
            fgets(tempData.name, sizeof(tempData.name), stdin);
            tempData.name[strcspn(tempData.name, "\n")] = '\0';

            printf("Enter New Age: ");
            scanf("%d", &tempData.age);
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
    int id, found = 0;
    printf("Enter User ID to delete: ");
    scanf("%d", &id);

    FILE *userRead = fopen(File_Name, "r");
    FILE *tempFile = fopen("temp.txt", "w");

    if (!userRead || !tempFile) {
        printf("Error opening files.\n");
        return;
    }

    struct userData tempData;
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
    FILE *userFile = fopen(File_Name, "r");
    if (!userFile) {
        userFile = fopen(File_Name, "w");
        printf("users.txt created.\n");
    } else {
        printf("users.txt found.\n");
    }
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
        scanf("%d", &choice);

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
