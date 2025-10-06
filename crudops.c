#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#define File_Name "users.txt"

struct userData{
    int id;
    char name[20];
    int age;
};

void createNewUser(){
    FILE *userFile=fopen(File_Name, "r+");
    struct userData actualData;

    int uniqueFlag=0;
    printf("Enter Unique ID: ");
    scanf("%d", &actualData.id);

    struct userData tempData;
    
    while(fscanf(userFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age)!=EOF){
        if(actualData.id==tempData.id){
            uniqueFlag=1;
            break;
        }
    }
    if(uniqueFlag==1){
        printf("Entered User ID: %d, already exists. Please enter any other unique ID.\n", tempData.id);
    }
    else{
        getchar();
        printf("Enter Name: ");
        fgets(actualData.name, sizeof(actualData.name), stdin);
        actualData.name[strcspn(actualData.name, "\n")]='\0';
        printf("Enter Age: ");
        scanf("%d", &actualData.age);

        fprintf(userFile, "%d, %s, %d\n", actualData.id, actualData.name, actualData.age);
        printf("User Created Successfully!");
    }
    fclose(userFile);
}

void readAllData(){
    FILE *userFile=fopen(File_Name, "r");
    struct userData tempData;

    printf("\n\tList of All Users:\n");
    while(fscanf(userFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age)!=EOF){
        printf("\t\tID: %d, Name: %s, Age: %d\n", tempData.id, tempData.name, tempData.age);
    }
    fclose(userFile);
}

void updateData(){
    int id;
    printf("Enter User ID to update: ");
    scanf("%d", &id);

    FILE *userFile=fopen(File_Name, "r+");
    struct userData tempData;
    int findFlag=0;

    while(fscanf(userFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age)!=EOF){
        if(tempData.id==id){
            findFlag=1;
            printf("Enter New Name: ");
            getchar();
            fgets(tempData.name, sizeof(tempData.name), stdin);
            tempData.name[strcspn(tempData.name, "\n")]='\0';
            printf("Enter New Age: ");
            scanf("%d", &tempData.age);

            fseek(userFile, -((long)sizeof(tempData.id)+strlen(tempData.name)+sizeof(tempData.age)+5), SEEK_CUR);
            fprintf(userFile, "\n%d, %s, %d\n", tempData.id, tempData.name, tempData.age);
            printf("User Data Updated Successfully!");
            break;
        }
    }
    if(findFlag==0){
        printf("User with this ID: %d, does not exists\n", id);
    }
    fclose(userFile);
}

void deleteData(){
    int id;
    printf("Enter User ID to delete: ");
    scanf("%d", &id);

    FILE *userReadFile=fopen(File_Name, "r");
    FILE *userWriteFile=fopen("temp.txt", "w");
    struct userData tempData; 
    int findFlag=0;
    while(fscanf(userReadFile, "%d,%19[^,],%d\n", &tempData.id, tempData.name, &tempData.age)!=EOF){
        if(tempData.id==id){
            findFlag=1;
        }
        else{
            fprintf(userWriteFile, "%d, %s, %d\n", tempData.id, tempData.name, tempData.age);
        }
    }
    if(findFlag==1){
        printf("User with ID: %d, Deleted Successfully\n", id);
        fclose(userReadFile);
        fclose(userWriteFile);
        remove(File_Name);
        rename("temp.txt", File_Name);
    }
    else{
        printf("User with ID: %d, Not Found\n", id);
        fclose(userReadFile);
        fclose(userWriteFile);
        remove("temp.txt");
    }

}

int main()
{
    FILE *userFile=fopen(File_Name, "r");
    if(!userFile){
        userFile=fopen(File_Name, "w");
        printf("users.txt file created successfully\n");
    }
    else{
        printf("users.txt file found\n");
    }

    int choice;

    while(true){
        printf("\nOptions:\n");
        printf("\t1. Create new user data\n");
        printf("\t2. Read all the data records\n");
        printf("\t3. Update the data record of any user using ID\n");
        printf("\t4. Delete the data record of any user using ID\n");
        printf("\t5. Exit the program\n");
        printf("\nEnter/Select Your Option(1-5) : ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            createNewUser();
            break;
        case 2:
            readAllData();
            break;
        case 3:
            updateData();
            break;
        case 4:
            deleteData();
            break;
        case 5:
            printf("\nThank You!\n");
            return 0;
        default:
            printf("\nInvalid Choice Entered\n");
            break;
        }
    }
    return 0;
}
