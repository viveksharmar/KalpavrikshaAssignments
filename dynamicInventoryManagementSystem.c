#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_PRODUCT_NAME 50
#define MAX_TOTAL_PRODUCTS 100

typedef struct {
    int ID;
    char name[MAX_PRODUCT_NAME + 1];
    float price;
    int quantity;
} Product;

int getValidatedIntegerInput(const char *inputMessage, int minValue, int maxValue);
float getValidatedFloatInput(const char *inputMessage, float minValue, float maxValue);
void getValidatedProductName(const char *inputMessage, char *nameBuffer);
int isProductNameValid(const char *name);
char *findSubstringIgnoreCase(const char *mainText, const char *searchText);
int isProductIDUnique(Product *productList, int productCount, int ID);

void addProduct(Product **productList, int *productCount);
void displayAllProducts(Product *productList, int productCount);
void updateProductQuantity(Product *productList, int productCount);
int searchProductByID(Product *productList, int productCount, int searchID);
void searchProductByName(Product *productList, int productCount);
void searchProductsByPriceRange(Product *productList, int productCount);
void deleteProductByID(Product **productList, int *productCount);

int getValidatedIntegerInput(const char *inputMessage, int minValue, int maxValue) {
    char userInput[100];
    int intValue;
    char *endPtr;
    while (1) {
        if (strlen(inputMessage) > 0)
            printf("%s", inputMessage);
        else
            printf("Enter Integer Input: ");
        if (fgets(userInput, sizeof(userInput), stdin) == NULL) continue;
        userInput[strcspn(userInput, "\n")] = '\0';
        intValue = (int)strtol(userInput, &endPtr, 10);
        if (*endPtr == '\0' && intValue >= minValue && intValue <= maxValue)
            return intValue;
        printf("Invalid input! Enter a number between %d and %d.\n", minValue, maxValue);
    }
}

float getValidatedFloatInput(const char *inputMessage, float minValue, float maxValue) {
    char userInput[100];
    float floatValue;
    char *endPtr;
    while (1) {
        if (strlen(inputMessage) > 0)
            printf("%s", inputMessage);
        else
            printf("Enter Float Input: ");
        if (fgets(userInput, sizeof(userInput), stdin) == NULL) continue;
        userInput[strcspn(userInput, "\n")] = '\0';
        floatValue = strtof(userInput, &endPtr);
        if (*endPtr == '\0' && floatValue >= minValue && floatValue <= maxValue)
            return floatValue;
        printf("Invalid input! Enter a valid number between %.2f and %.2f.\n", minValue, maxValue);
    }
}

int isProductNameValid(const char *name) {
    int length = strlen(name);
    if (length < 1 || length > MAX_PRODUCT_NAME)
        return 0;
    for (int i = 0; i < length; i++) {
        if (!isalnum(name[i]) && name[i] != ' ' && name[i] != '-')
            return 0;
        if (name[i] == ' ' && (i == 0 || i == length - 1 || name[i - 1] == ' '))
            return 0;
    }
    return 1;
}

void getValidatedProductName(const char *inputMessage, char *nameBuffer) {
    char tempBuffer[200];
    while (1) {
        printf("%s", inputMessage);
        if (fgets(tempBuffer, sizeof(tempBuffer), stdin) == NULL) continue;
        tempBuffer[strcspn(tempBuffer, "\n")] = '\0';
        if (isProductNameValid(tempBuffer)) {
            strcpy(nameBuffer, tempBuffer);
            return;
        } else {
            printf("Invalid name! Only letters, digits, hyphen (-), and single spaces allowed.\n");
        }
    }
}

char *findSubstringIgnoreCase(const char *mainText, const char *searchText) {
    if (!mainText || !searchText) return NULL;
    size_t searchLength = strlen(searchText);
    if (searchLength == 0) return (char *)mainText;
    for (; *mainText; mainText++) {
        if (strncasecmp(mainText, searchText, searchLength) == 0)
            return (char *)mainText;
    }
    return NULL;
}

int searchProductByID(Product *productList, int productCount, int searchID) {
    for (int i = 0; i < productCount; i++) {
        if (productList[i].ID == searchID)
            return i;
    }
    return -1;
}

int isProductIDUnique(Product *productList, int productCount, int ID) {
    return searchProductByID(productList, productCount, ID) == -1;
}

void addProduct(Product **productList, int *productCount) {
    if (*productCount >= MAX_TOTAL_PRODUCTS) {
        printf("Cannot add more products (limit reached).\n");
        return;
    }
    *productList = realloc(*productList, (*productCount + 1) * sizeof(Product));
    if (*productList == NULL) {
        printf("\nMemory allocation failed!\n");
        exit(1);
    }
    printf("Enter new product details:\n");
    int newID;
    while (1) {
        newID = getValidatedIntegerInput("Product ID: ", 1, 10000);
        if (isProductIDUnique(*productList, *productCount, newID))
            break;
        printf("Product ID already exists! Enter a unique ID.\n");
    }
    (*productList)[*productCount].ID = newID;
    getValidatedProductName("Product Name: ", (*productList)[*productCount].name);
    (*productList)[*productCount].price = getValidatedFloatInput("Product Price: ", 0, 100000);
    (*productList)[*productCount].quantity = getValidatedIntegerInput("Product Quantity: ", 0, 1000000);
    (*productCount)++;
    printf("\nProduct added successfully!\n");
}

void displayAllProducts(Product *productList, int productCount) {
    if (productCount == 0) {
        printf("\nNo products available.\n");
        return;
    }
    printf("\n========= PRODUCT LIST =========\n");
    for (int i = 0; i < productCount; i++) {
        printf("Product ID: %d | Name: %s | Price: %.2f | Quantity: %d\n", productList[i].ID, productList[i].name, productList[i].price, productList[i].quantity);
        if (i != productCount - 1)
            printf("\n");
    }
}

void updateProductQuantity(Product *productList, int productCount) {
    if (productCount == 0) {
        printf("\nNo products available.\n");
        return;
    }
    int targetID = getValidatedIntegerInput("Enter Product ID to update quantity: ", 1, 10000);
    int index = searchProductByID(productList, productCount, targetID);
    if (index != -1) {
        productList[index].quantity = getValidatedIntegerInput("Enter new Quantity: ", 0, 1000000);
        printf("\nQuantity updated successfully!\n");
    } else {
        printf("\nProduct not found!\n");
    }
}

void searchProductByName(Product *productList, int productCount) {
    if (productCount == 0) {
        printf("\nNo products available.\n");
        return;
    }
    char searchTerm[100];
    printf("Enter name to search (partial allowed): ");
    fgets(searchTerm, sizeof(searchTerm), stdin);
    searchTerm[strcspn(searchTerm, "\n")] = '\0';
    int foundFlag = 0;
    for (int i = 0; i < productCount; i++) {
        if (findSubstringIgnoreCase(productList[i].name, searchTerm)) {
            if (!foundFlag)
                printf("\nProducts Found:\n");
            printf("Product ID: %d | Name: %s | Price: %.2f | Quantity: %d\n", productList[i].ID, productList[i].name, productList[i].price, productList[i].quantity);
            foundFlag = 1;
        }
    }
    if (!foundFlag)
        printf("\nNo matching products found.\n");
}

void searchProductsByPriceRange(Product *productList, int productCount) {
    if (productCount == 0) {
        printf("\nNo products available.\n");
        return;
    }
    float minPrice = getValidatedFloatInput("Enter minimum price: ", 0, 100000);
    float maxPrice = getValidatedFloatInput("Enter maximum price: ", minPrice, 100000);
    int found = 0;
    printf("\nProducts in price range:\n");
    for (int i = 0; i < productCount; i++) {
        if (productList[i].price >= minPrice && productList[i].price <= maxPrice) {
            printf("Product ID: %d | Name: %s | Price: %.2f | Quantity: %d\n", productList[i].ID, productList[i].name, productList[i].price, productList[i].quantity);
            found = 1;
        }
    }
    if (!found)
        printf("\nNo products found in the given range.\n");
}

void deleteProductByID(Product **productList, int *productCount) {
    if (*productCount == 0) {
        printf("\nNo products to delete.\n");
        return;
    }
    int deleteID = getValidatedIntegerInput("Enter Product ID to delete: ", 1, 10000);
    int index = searchProductByID(*productList, *productCount, deleteID);
    if (index != -1) {
        for (int j = index; j < *productCount - 1; j++)
            (*productList)[j] = (*productList)[j + 1];
        (*productCount)--;
        *productList = realloc(*productList, (*productCount) * sizeof(Product));
        printf("\nProduct deleted successfully!\n");
    } else {
        printf("\nProduct not found!\n");
    }
}

int main() {
    Product *productList = NULL;
    int productCount = 0;
    int initialCount = getValidatedIntegerInput("Enter initial number of products: ", 1, 100);
    printf("\n");
    for (int i = 0; i < initialCount; i++) {
        printf("\nEnter details for product %d:\n", i + 1);
        addProduct(&productList, &productCount);
    }
    int userChoice;
    while (1) {
        printf("\n\n========= INVENTORY MENU =========\n\n");
        printf("1. Add New Product\n");
        printf("2. View All Products\n");
        printf("3. Update Quantity\n");
        printf("4. Search Product by ID\n");
        printf("5. Search Product by Name\n");
        printf("6. Search Product by Price Range\n");
        printf("7. Delete Product\n");
        printf("8. Exit\n");
        printf("Enter your choice: ");
        userChoice = getValidatedIntegerInput("", 1, 8);
        switch (userChoice) {
            case 1: addProduct(&productList, &productCount); break;
            case 2: displayAllProducts(productList, productCount); break;
            case 3: updateProductQuantity(productList, productCount); break;
            case 4:
                if (productCount == 0) {
                    printf("\nNo products available.\n");
                    break;
                }
                {
                    int searchID = getValidatedIntegerInput("Enter Product ID to search: ", 1, 10000);
                    int idx = searchProductByID(productList, productCount, searchID);
                    if (idx != -1)
                        printf("\nProduct Found: Product ID: %d | Name: %s | Price: %.2f | Quantity: %d\n", productList[idx].ID, productList[idx].name, productList[idx].price, productList[idx].quantity);
                    else
                        printf("\nProduct not found!\n");
                }
                break;
            case 5: searchProductByName(productList, productCount); break;
            case 6: searchProductsByPriceRange(productList, productCount); break;
            case 7: deleteProductByID(&productList, &productCount); break;
            case 8:
                free(productList);
                printf("\nMemory released successfully. Exiting program...\n");
                return 0;
        }
    }
}

