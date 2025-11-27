#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_TABLE_SIZE 1009
#define MAX_STRING_LENGTH 200
#define MAX_INPUT_LENGTH 300

typedef struct queueNode {
    int key;
    char value[MAX_STRING_LENGTH];
    struct queueNode *previousNode;
    struct queueNode *nextNode;
} QueueNode;

typedef struct hashTableNode {
    int key;
    QueueNode *queueNodePointer;
    struct hashTableNode *nextHashTableNode;
} HashTableNode;

typedef struct lruCache {
    int cacheCapacity;
    int currentCacheSize;
    QueueNode *queueFrontNode;
    QueueNode *queueRearNode;
    HashTableNode *hashTable[HASH_TABLE_SIZE];
} LRUCache;

int isValidIntegerString(const char *stringValue) {
    if (stringValue == NULL || *stringValue == '\0') {
        return 0;
    }
    for (int characterIndex = 0; stringValue[characterIndex] != '\0'; characterIndex++) {
        if (!isdigit((unsigned char)stringValue[characterIndex])) {
            return 0;
        }
    }
    return 1;
}

int calculateHashIndex(int key) {
    return key % HASH_TABLE_SIZE;
}

QueueNode* createQueueNodeForCache(int key, const char *value) {
    QueueNode *newQueueNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newQueueNode == NULL) {
        printf("ERROR: Memory allocation failed.\n");
        exit(1);
    }
    newQueueNode->key = key;
    strncpy(newQueueNode->value, value, MAX_STRING_LENGTH);
    newQueueNode->value[MAX_STRING_LENGTH - 1] = '\0';
    newQueueNode->previousNode = NULL;
    newQueueNode->nextNode = NULL;
    return newQueueNode;
}

HashTableNode* createHashTableNode(int key, QueueNode *queueNodePointer) {
    HashTableNode *newHashTableNode = (HashTableNode*)malloc(sizeof(HashTableNode));
    if (newHashTableNode == NULL) {
        printf("ERROR: Memory allocation failed.\n");
        exit(1);
    }
    newHashTableNode->key = key;
    newHashTableNode->queueNodePointer = queueNodePointer;
    newHashTableNode->nextHashTableNode = NULL;
    return newHashTableNode;
}

void moveQueueNodeToFront(LRUCache *cachePointer, QueueNode *queueNodePointer) {
    if (cachePointer->queueFrontNode == queueNodePointer) {
        return;
    }
    if (queueNodePointer->previousNode != NULL) {
        queueNodePointer->previousNode->nextNode = queueNodePointer->nextNode;
    }
    if (queueNodePointer->nextNode != NULL) {
        queueNodePointer->nextNode->previousNode = queueNodePointer->previousNode;
    }
    if (cachePointer->queueRearNode == queueNodePointer) {
        cachePointer->queueRearNode = queueNodePointer->previousNode;
    }
    queueNodePointer->previousNode = NULL;
    queueNodePointer->nextNode = cachePointer->queueFrontNode;

    if (cachePointer->queueFrontNode != NULL) {
        cachePointer->queueFrontNode->previousNode = queueNodePointer;
    }

    cachePointer->queueFrontNode = queueNodePointer;

    if (cachePointer->queueRearNode == NULL) {
        cachePointer->queueRearNode = queueNodePointer;
    }
}

void insertQueueNodeAtFront(LRUCache *cachePointer, QueueNode *queueNodePointer) {
    queueNodePointer->previousNode = NULL;
    queueNodePointer->nextNode = cachePointer->queueFrontNode;

    if (cachePointer->queueFrontNode != NULL) {
        cachePointer->queueFrontNode->previousNode = queueNodePointer;
    }

    cachePointer->queueFrontNode = queueNodePointer;

    if (cachePointer->queueRearNode == NULL) {
        cachePointer->queueRearNode = queueNodePointer;
    }
}

QueueNode* removeQueueNodeFromRear(LRUCache *cachePointer) {
    QueueNode *rearNodePointer = cachePointer->queueRearNode;

    if (rearNodePointer->previousNode != NULL) {
        cachePointer->queueRearNode = rearNodePointer->previousNode;
        cachePointer->queueRearNode->nextNode = NULL;
    } else {
        cachePointer->queueFrontNode = NULL;
        cachePointer->queueRearNode = NULL;
    }

    rearNodePointer->previousNode = NULL;
    rearNodePointer->nextNode = NULL;
    return rearNodePointer;
}

QueueNode* searchQueueNodeInHashTable(LRUCache *cachePointer, int key) {
    int hashTableBucketIndex = calculateHashIndex(key);
    HashTableNode *currentHashTableNode = cachePointer->hashTable[hashTableBucketIndex];

    while (currentHashTableNode != NULL) {
        if (currentHashTableNode->key == key) {
            return currentHashTableNode->queueNodePointer;
        }
        currentHashTableNode = currentHashTableNode->nextHashTableNode;
    }
    return NULL;
}

void insertNodeInHashTable(LRUCache *cachePointer, int key, QueueNode *queueNodePointer) {
    int hashTableBucketIndex = calculateHashIndex(key);
    HashTableNode *newHashTableNode = createHashTableNode(key, queueNodePointer);
    newHashTableNode->nextHashTableNode = cachePointer->hashTable[hashTableBucketIndex];
    cachePointer->hashTable[hashTableBucketIndex] = newHashTableNode;
}

void deleteNodeFromHashTable(LRUCache *cachePointer, int key) {
    int hashTableBucketIndex = calculateHashIndex(key);
    HashTableNode *currentHashTableNode = cachePointer->hashTable[hashTableBucketIndex];
    HashTableNode *previousHashTableNode = NULL;

    while (currentHashTableNode != NULL) {
        if (currentHashTableNode->key == key) {
            if (previousHashTableNode != NULL) {
                previousHashTableNode->nextHashTableNode = currentHashTableNode->nextHashTableNode;
            } else {
                cachePointer->hashTable[hashTableBucketIndex] = currentHashTableNode->nextHashTableNode;
            }
            free(currentHashTableNode);
            return;
        }
        previousHashTableNode = currentHashTableNode;
        currentHashTableNode = currentHashTableNode->nextHashTableNode;
    }
}

LRUCache* createLruCache(int cacheCapacity) {
    if (cacheCapacity <= 0 || cacheCapacity > 1000) {
        printf("ERROR: Cache size must be between 1 and 1000.\n");
        return NULL;
    }

    LRUCache *newCachePointer = (LRUCache*)malloc(sizeof(LRUCache));
    if (newCachePointer == NULL) {
        printf("ERROR: Memory allocation failed.\n");
        exit(1);
    }

    newCachePointer->cacheCapacity = cacheCapacity;
    newCachePointer->currentCacheSize = 0;
    newCachePointer->queueFrontNode = NULL;
    newCachePointer->queueRearNode = NULL;

    for (int bucketIndex = 0; bucketIndex < HASH_TABLE_SIZE; bucketIndex++) {
        newCachePointer->hashTable[bucketIndex] = NULL;
    }

    printf("Cache created with capacity = %d\n", cacheCapacity);
    return newCachePointer;
}

char* getValueFromCache(LRUCache *cachePointer, int key) {
    QueueNode *foundQueueNode = searchQueueNodeInHashTable(cachePointer, key);
    if (foundQueueNode == NULL) {
        return NULL;
    }
    moveQueueNodeToFront(cachePointer, foundQueueNode);
    return foundQueueNode->value;
}

void putKeyValueInCache(LRUCache *cachePointer, int key, const char *value) {
    QueueNode *existingQueueNode = searchQueueNodeInHashTable(cachePointer, key);

    if (existingQueueNode != NULL) {
        strncpy(existingQueueNode->value, value, MAX_STRING_LENGTH);
        existingQueueNode->value[MAX_STRING_LENGTH - 1] = '\0';
        moveQueueNodeToFront(cachePointer, existingQueueNode);
        return;
    }

    if (cachePointer->currentCacheSize == cachePointer->cacheCapacity) {
        QueueNode *leastRecentlyUsedNode = removeQueueNodeFromRear(cachePointer);
        deleteNodeFromHashTable(cachePointer, leastRecentlyUsedNode->key);
        free(leastRecentlyUsedNode);
        cachePointer->currentCacheSize--;
    }

    QueueNode *newQueueNode = createQueueNodeForCache(key, value);
    insertQueueNodeAtFront(cachePointer, newQueueNode);
    insertNodeInHashTable(cachePointer, key, newQueueNode);
    cachePointer->currentCacheSize++;
}

void freeEntireCache(LRUCache *cachePointer) {
    QueueNode *currentQueueNode = cachePointer->queueFrontNode;
    while (currentQueueNode != NULL) {
        QueueNode *nextQueueNode = currentQueueNode->nextNode;
        free(currentQueueNode);
        currentQueueNode = nextQueueNode;
    }

    for (int bucketIndex = 0; bucketIndex < HASH_TABLE_SIZE; bucketIndex++) {
        HashTableNode *currentHashTableNode = cachePointer->hashTable[bucketIndex];
        while (currentHashTableNode != NULL) {
            HashTableNode *nextHashTableNode = currentHashTableNode->nextHashTableNode;
            free(currentHashTableNode);
            currentHashTableNode = nextHashTableNode;
        }
    }

    free(cachePointer);
}

void printUsageInstructions() {
    printf("\n====================== LRU CACHE COMMANDS ======================\n");
    printf("  createCache <size>\n");
    printf("  put <key> <data>\n");
    printf("  get <key>\n");
    printf("  exit\n");
    printf("=================================================================\n\n");
}

int main() {
    char inputLine[MAX_INPUT_LENGTH];
    char commandString[50];
    char firstArgumentString[200];
    char secondArgumentString[200];

    LRUCache *cachePointer = NULL;

    printUsageInstructions();

    while (1) {
        printf(">> ");
        if (fgets(inputLine, sizeof(inputLine), stdin) == NULL) {
            break;
        }

        commandString[0] = '\0';
        firstArgumentString[0] = '\0';
        secondArgumentString[0] = '\0';

        int numberOfPartsScanned = sscanf(inputLine, "%s %s %s", commandString, firstArgumentString, secondArgumentString);

        if (numberOfPartsScanned <= 0) {
            continue;
        }

        if (strcmp(commandString, "createCache") == 0) {
            if (numberOfPartsScanned < 2 || !isValidIntegerString(firstArgumentString)) {
                printf("ERROR: Usage -> createCache <size>\n");
                continue;
            }
            int cacheSize = atoi(firstArgumentString);
            if (cachePointer != NULL) {
                freeEntireCache(cachePointer);
                cachePointer = NULL;
            }
            cachePointer = createLruCache(cacheSize);
        }

        else if (strcmp(commandString, "put") == 0) {
            if (cachePointer == NULL) {
                printf("ERROR: Cache not created yet.\n");
                continue;
            }
            if (numberOfPartsScanned < 3) {
                printf("ERROR: Usage -> put <key> <data>\n");
                continue;
            }
            if (!isValidIntegerString(firstArgumentString)) {
                printf("ERROR: Key must be a valid integer.\n");
                continue;
            }

            int key = atoi(firstArgumentString);
            putKeyValueInCache(cachePointer, key, secondArgumentString);
        }

        else if (strcmp(commandString, "get") == 0) {
            if (cachePointer == NULL) {
                printf("ERROR: Cache not created yet.\n");
                continue;
            }
            if (numberOfPartsScanned < 2 || !isValidIntegerString(firstArgumentString)) {
                printf("ERROR: Usage -> get <key>\n");
                continue;
            }

            int key = atoi(firstArgumentString);
            char *valueFromCache = getValueFromCache(cachePointer, key);

            if (valueFromCache != NULL) {
                printf("%s\n", valueFromCache);
            } else {
                printf("NULL\n");
            }
        }

        else if (strcmp(commandString, "exit") == 0) {
            break;
        }

        else {
            printf("ERROR: Unknown command.\n");
        }
    }

    if (cachePointer != NULL) {
        freeEntireCache(cachePointer);
    }

    return 0;
}