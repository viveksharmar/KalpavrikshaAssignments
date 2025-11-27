#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 1024
#define MAX_NAME_LEN 50
#define MAX_CMD_LEN 2048

typedef struct FreeBlockNode {
    int blockIndex;
    struct FreeBlockNode *prev;
    struct FreeBlockNode *next;
} FreeBlockNode;

typedef struct VfsNode {
    char name[MAX_NAME_LEN + 1];
    int isDirectory;
    struct VfsNode *parent;
    struct VfsNode *firstChild;
    struct VfsNode *nextSibling;
    struct VfsNode *prevSibling;
    int *allocatedBlocks;
    int allocatedBlockCount;
    int fileSize;
} VfsNode;

unsigned char *virtualDisk = NULL;
FreeBlockNode *freeBlockHead = NULL;
FreeBlockNode *freeBlockTail = NULL;
int usedBlockCount = 0;

VfsNode *rootDirectory = NULL;
VfsNode *currentDirectory = NULL;

void exitWithError(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

FreeBlockNode* createFreeBlockNode(int blockIndex) {
    FreeBlockNode *node = malloc(sizeof(FreeBlockNode));
    if (!node) exitWithError("Out of memory");
    node->blockIndex = blockIndex;
    node->prev = node->next = NULL;
    return node;
}

void initializeFreeBlockList(int totalBlocks) {
    freeBlockHead = freeBlockTail = NULL;
    for (int i = 0; i < totalBlocks; ++i) {
        FreeBlockNode *node = createFreeBlockNode(i);
        if (!freeBlockHead) {
            freeBlockHead = freeBlockTail = node;
        } else {
            freeBlockTail->next = node;
            node->prev = freeBlockTail;
            freeBlockTail = node;
        }
    }
}

int allocateFreeBlock() {
    if (!freeBlockHead) return -1;
    FreeBlockNode *node = freeBlockHead;
    int blockIndex = node->blockIndex;
    freeBlockHead = node->next;
    if (freeBlockHead) {
        freeBlockHead->prev = NULL;
    } else {
        freeBlockTail = NULL;
    }
    free(node);
    usedBlockCount++;
    return blockIndex;
}

void releaseFreeBlock(int blockIndex) {
    FreeBlockNode *node = createFreeBlockNode(blockIndex);
    if (!freeBlockTail) {
        freeBlockHead = freeBlockTail = node;
    } else {
        freeBlockTail->next = node;
        node->prev = freeBlockTail;
        freeBlockTail = node;
    }
    usedBlockCount--;
}

int getFreeBlockCount() {
    return TOTAL_BLOCKS - usedBlockCount;
}

VfsNode* createVfsNode(const char *name, int isDirectory, VfsNode *parent) {
    if (strlen(name) > MAX_NAME_LEN) return NULL;
    VfsNode *node = malloc(sizeof(VfsNode));
    if (!node) exitWithError("Out of memory");
    strncpy(node->name, name, MAX_NAME_LEN);
    node->name[MAX_NAME_LEN] = '\0';
    node->isDirectory = isDirectory;
    node->parent = parent;
    node->firstChild = NULL;
    node->nextSibling = node->prevSibling = NULL;
    node->allocatedBlocks = NULL;
    node->allocatedBlockCount = 0;
    node->fileSize = 0;
    return node;
}

int attachChildNode(VfsNode *parent, VfsNode *child) {
    if (!parent || !parent->isDirectory) return -1;
    if (!parent->firstChild) {
        parent->firstChild = child;
        child->nextSibling = child->prevSibling = child;
    } else {
        VfsNode *first = parent->firstChild;
        VfsNode *last = first->prevSibling;
        last->nextSibling = child;
        child->prevSibling = last;
        child->nextSibling = first;
        first->prevSibling = child;
    }
    child->parent = parent;
    return 0;
}

int detachChildNode(VfsNode *parent, VfsNode *child) {
    if (!parent || !parent->isDirectory || !parent->firstChild || !child) return -1;
    if (parent->firstChild == child && child->nextSibling == child) {
        parent->firstChild = NULL;
    } else {
        if (parent->firstChild == child) parent->firstChild = child->nextSibling;
        child->prevSibling->nextSibling = child->nextSibling;
        child->nextSibling->prevSibling = child->prevSibling;
    }
    child->nextSibling = child->prevSibling = NULL;
    child->parent = NULL;
    return 0;
}

VfsNode* findChildNode(VfsNode *parent, const char *name) {
    if (!parent || !parent->isDirectory) return NULL;
    VfsNode *head = parent->firstChild;
    if (!head) return NULL;
    VfsNode *node = head;
    do {
        if (strcmp(node->name, name) == 0) return node;
        node = node->nextSibling;
    } while (node != head);
    return NULL;
}

void freeVfsNode(VfsNode *node) {
    if (!node) return;
    if (node->allocatedBlocks) {
        free(node->allocatedBlocks);
        node->allocatedBlocks = NULL;
    }
    free(node);
}

int writeFileContent(VfsNode *fileNode, const unsigned char *content, int size) {
    if (!fileNode || fileNode->isDirectory) return -1;
    if (size < 0) size = 0;
    int requiredBlocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (requiredBlocks > getFreeBlockCount()) return -2;
    if (fileNode->allocatedBlockCount > 0 && fileNode->allocatedBlocks) {
        for (int i = 0; i < fileNode->allocatedBlockCount; ++i) {
            releaseFreeBlock(fileNode->allocatedBlocks[i]);
        }
        free(fileNode->allocatedBlocks);
        fileNode->allocatedBlocks = NULL;
        fileNode->allocatedBlockCount = 0;
        fileNode->fileSize = 0;
    }
    if (requiredBlocks == 0) {
        fileNode->allocatedBlocks = NULL;
        fileNode->allocatedBlockCount = 0;
        fileNode->fileSize = 0;
        return 0;
    }
    fileNode->allocatedBlocks = malloc(sizeof(int) * requiredBlocks);
    if (!fileNode->allocatedBlocks) exitWithError("Out of memory");
    for (int i = 0; i < requiredBlocks; ++i) {
        int blockIndex = allocateFreeBlock();
        if (blockIndex < 0) {
            for (int j = 0; j < i; ++j) {
                releaseFreeBlock(fileNode->allocatedBlocks[j]);
            }
            free(fileNode->allocatedBlocks);
            fileNode->allocatedBlocks = NULL;
            fileNode->allocatedBlockCount = 0;
            return -2;
        }
        fileNode->allocatedBlocks[i] = blockIndex;
        unsigned char *blockStart = virtualDisk + ((size_t)blockIndex * BLOCK_SIZE);
        int copyOffset = i * BLOCK_SIZE;
        int bytesToCopy = BLOCK_SIZE;
        if (copyOffset + bytesToCopy > size) bytesToCopy = size - copyOffset;
        if (bytesToCopy > 0) memcpy(blockStart, content + copyOffset, bytesToCopy);
        if (bytesToCopy < BLOCK_SIZE) memset(blockStart + bytesToCopy, 0, BLOCK_SIZE - bytesToCopy);
    }
    fileNode->allocatedBlockCount = requiredBlocks;
    fileNode->fileSize = size;
    return 0;
}

int readFileContent(VfsNode *fileNode) {
    if (!fileNode || fileNode->isDirectory) return -1;
    if (fileNode->allocatedBlockCount == 0 || fileNode->allocatedBlocks == NULL || fileNode->fileSize == 0) return 0;
    int remainingBytes = fileNode->fileSize;
    for (int i = 0; i < fileNode->allocatedBlockCount; ++i) {
        int blockIndex = fileNode->allocatedBlocks[i];
        unsigned char *blockStart = virtualDisk + ((size_t)blockIndex * BLOCK_SIZE);
        int bytesToPrint = (remainingBytes > BLOCK_SIZE) ? BLOCK_SIZE : remainingBytes;
        fwrite(blockStart, 1, bytesToPrint, stdout);
        remainingBytes -= bytesToPrint;
        if (remainingBytes <= 0) break;
    }
    return 0;
}

int deleteFileNode(VfsNode *fileNode) {
    if (!fileNode || fileNode->isDirectory) return -1;
    if (fileNode->allocatedBlockCount > 0 && fileNode->allocatedBlocks) {
        for (int i = 0; i < fileNode->allocatedBlockCount; ++i) {
            releaseFreeBlock(fileNode->allocatedBlocks[i]);
        }
        free(fileNode->allocatedBlocks);
        fileNode->allocatedBlocks = NULL;
        fileNode->allocatedBlockCount = 0;
        fileNode->fileSize = 0;
    }
    if (fileNode->parent) detachChildNode(fileNode->parent, fileNode);
    freeVfsNode(fileNode);
    return 0;
}

int removeDirectoryNode(VfsNode *directoryNode) {
    if (!directoryNode || !directoryNode->isDirectory) return -1;
    if (directoryNode->firstChild != NULL) return -2;
    if (directoryNode->parent) detachChildNode(directoryNode->parent, directoryNode);
    freeVfsNode(directoryNode);
    return 0;
}

int handleMakeDirectory(const char *name) {
    if (!name || strlen(name) == 0) { printf("Usage: mkdir <name>\n"); return -1; }
    if (strlen(name) > MAX_NAME_LEN) { printf("Error: name too long\n"); return -1; }
    if (strchr(name, '/')) { printf("Error: name cannot contain '/'\n"); return -1; }
    if (findChildNode(currentDirectory, name)) { printf("Error: directory '%s' already exists\n", name); return -1; }
    VfsNode *directoryNode = createVfsNode(name, 1, currentDirectory);
    attachChildNode(currentDirectory, directoryNode);
    printf("Directory '%s' created\n", name);
    return 0;
}

int handleCreateFile(const char *name) {
    if (!name || strlen(name) == 0) { printf("Usage: create <filename>\n"); return -1; }
    if (strlen(name) > MAX_NAME_LEN) { printf("Error: name too long\n"); return -1; }
    if (strchr(name, '/')) { printf("Error: name cannot contain '/'\n"); return -1; }
    if (findChildNode(currentDirectory, name)) { printf("Error: file '%s' already exists\n", name); return -1; }
    VfsNode *fileNode = createVfsNode(name, 0, currentDirectory);
    attachChildNode(currentDirectory, fileNode);
    printf("File '%s' created\n", name);
    return 0;
}

void buildAbsolutePath(VfsNode *node, char *outputBuffer, int bufferSize) {
    if (!node || node == rootDirectory) {
        strncpy(outputBuffer, "/", bufferSize - 1);
        outputBuffer[bufferSize - 1] = '\0';
        return;
    }
    char nameStack[256][MAX_NAME_LEN + 1];
    int depth = 0;
    VfsNode *cursor = node;
    while (cursor && cursor != rootDirectory) {
        strncpy(nameStack[depth], cursor->name, MAX_NAME_LEN);
        nameStack[depth][MAX_NAME_LEN] = '\0';
        depth++;
        cursor = cursor->parent;
    }
    outputBuffer[0] = '\0';
    for (int i = depth - 1; i >= 0; --i) {
        strncat(outputBuffer, "/", bufferSize - strlen(outputBuffer) - 1);
        strncat(outputBuffer, nameStack[i], bufferSize - strlen(outputBuffer) - 1);
    }
    if (strlen(outputBuffer) == 0) {
        strncpy(outputBuffer, "/", bufferSize - 1);
    }
    outputBuffer[bufferSize - 1] = '\0';
}

int changeDirectory(const char *path) {
    if (!path || strlen(path) == 0) { printf("Usage: cd <path>\n"); return -1; }
    VfsNode *startNode = currentDirectory;
    if (path[0] == '/') startNode = rootDirectory;
    char pathCopy[MAX_CMD_LEN];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';
    char *token = strtok(pathCopy, "/");
    VfsNode *cursor = startNode;
    while (token) {
        if (strcmp(token, "") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        if (strcmp(token, "..") == 0) {
            if (cursor->parent) cursor = cursor->parent;
            token = strtok(NULL, "/");
            continue;
        }
        VfsNode *nextNode = findChildNode(cursor, token);
        if (!nextNode) {
            printf("Error: path component '%s' not found\n", token);
            return -1;
        }
        if (!nextNode->isDirectory) {
            printf("Error: '%s' is not a directory\n", token);
            return -1;
        }
        cursor = nextNode;
        token = strtok(NULL, "/");
    }
    currentDirectory = cursor;
    char fullPath[1024];
    buildAbsolutePath(currentDirectory, fullPath, sizeof(fullPath));
    printf("Moved to %s\n", fullPath);
    return 0;
}

void handleListDirectory() {
    if (!currentDirectory->firstChild) {
        printf("(empty)\n");
        return;
    }
    VfsNode *head = currentDirectory->firstChild;
    VfsNode *node = head;
    do {
        printf("%s%s\n", node->name, node->isDirectory ? "/" : "");
        node = node->nextSibling;
    } while (node != head);
}

void handleDiskUsage() {
    int freeBlocks = getFreeBlockCount();
    double usedPercent = (double)usedBlockCount / (double)TOTAL_BLOCKS * 100.0;
    printf("Total Blocks: %d\nUsed Blocks: %d\nFree Blocks: %d\nDisk Usage: %.2f%%\n", TOTAL_BLOCKS, usedBlockCount, freeBlocks, usedPercent);
}

void parseWriteArguments(const char *argumentLine, char *fileNameOut, char **contentOut) {
    fileNameOut[0] = '\0';
    *contentOut = NULL;
    if (!argumentLine) return;
    const char *cursor = argumentLine;
    while (*cursor && isspace((unsigned char)*cursor)) cursor++;
    if (!*cursor) return;
    int i = 0;
    while (*cursor && !isspace((unsigned char)*cursor) && i < MAX_NAME_LEN) fileNameOut[i++] = *cursor++;
    fileNameOut[i] = '\0';
    while (*cursor && isspace((unsigned char)*cursor)) cursor++;
    if (!*cursor) return;
    if (*cursor == '"' || *cursor == '\'') {
        char quoteChar = *cursor++;
        const char *start = cursor;
        while (*cursor && *cursor != quoteChar) cursor++;
        size_t length = cursor - start;
        *contentOut = malloc(length + 1);
        if (!*contentOut) exitWithError("Out of memory");
        memcpy(*contentOut, start, length);
        (*contentOut)[length] = '\0';
    } else {
        size_t length = strlen(cursor);
        while (length > 0 && (cursor[length - 1] == '\n' || cursor[length - 1] == '\r')) length--;
        *contentOut = malloc(length + 1);
        if (!*contentOut) exitWithError("Out of memory");
        memcpy(*contentOut, cursor, length);
        (*contentOut)[length] = '\0';
    }
}

void freeVfsTreeRecursive(VfsNode *directoryNode) {
    if (!directoryNode) return;
    while (directoryNode->firstChild) {
        VfsNode *child = directoryNode->firstChild;
        detachChildNode(directoryNode, child);
        if (child->isDirectory) {
            freeVfsTreeRecursive(child);
        } else {
            if (child->allocatedBlockCount > 0 && child->allocatedBlocks) {
                for (int i = 0; i < child->allocatedBlockCount; ++i) {
                    releaseFreeBlock(child->allocatedBlocks[i]);
                }
                free(child->allocatedBlocks);
                child->allocatedBlocks = NULL;
            }
            freeVfsNode(child);
        }
    }
    freeVfsNode(directoryNode);
}

void cleanupVfs() {
    if (rootDirectory) {
        freeVfsTreeRecursive(rootDirectory);
        rootDirectory = currentDirectory = NULL;
    }
    FreeBlockNode *cursor = freeBlockHead;
    while (cursor) {
        FreeBlockNode *next = cursor->next;
        free(cursor);
        cursor = next;
    }
    freeBlockHead = freeBlockTail = NULL;
    if (virtualDisk) free(virtualDisk);
    virtualDisk = NULL;
}

void runShellLoop() {
    printf("Compact VFS ready. Type 'exit' to quit.\n");
    char inputLine[MAX_CMD_LEN];
    while (1) {
        char currentPath[1024];
        buildAbsolutePath(currentDirectory, currentPath, sizeof(currentPath));
        printf("%s > ", currentPath);
        if (!fgets(inputLine, sizeof(inputLine), stdin)) {
            printf("\n");
            break;
        }
        char *cursor = inputLine;
        while (*cursor && (*cursor == '\n' || *cursor == '\r')) cursor++;
        char command[64] = {0};
        int cmdLen = 0;
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        while (*cursor && !isspace((unsigned char)*cursor) && cmdLen < (int)sizeof(command) - 1) {
            command[cmdLen++] = *cursor++;
        }
        command[cmdLen] = '\0';
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        char arguments[MAX_CMD_LEN] = {0};
        if (*cursor) {
            strncpy(arguments, cursor, sizeof(arguments) - 1);
            arguments[sizeof(arguments) - 1] = '\0';
            int argLen = strlen(arguments);
            while (argLen > 0 && (arguments[argLen - 1] == '\n' || arguments[argLen - 1] == '\r')) {
                arguments[--argLen] = '\0';
            }
        }
        if (strcmp(command, "mkdir") == 0) {
            handleMakeDirectory(arguments);
        } else if (strcmp(command, "create") == 0) {
            handleCreateFile(arguments);
        } else if (strcmp(command, "ls") == 0) {
            handleListDirectory();
        } else if (strcmp(command, "pwd") == 0) {
            char pathBuffer[1024];
            buildAbsolutePath(currentDirectory, pathBuffer, sizeof(pathBuffer));
            printf("%s\n", pathBuffer);
        } else if (strcmp(command, "df") == 0) {
            handleDiskUsage();
        } else if (strcmp(command, "cd") == 0) {
            changeDirectory(arguments);
        } else if (strcmp(command, "rmdir") == 0) {
            if (!arguments || strlen(arguments) == 0) {
                printf("Usage: rmdir <dirname>\n");
            } else {
                VfsNode *dirNode = findChildNode(currentDirectory, arguments);
                if (!dirNode) {
                    printf("Error: directory '%s' not found\n", arguments);
                } else if (!dirNode->isDirectory) {
                    printf("Error: '%s' is not a directory\n", arguments);
                } else if (dirNode->firstChild != NULL) {
                    printf("Error: directory not empty\n");
                } else {
                    removeDirectoryNode(dirNode);
                    printf("Directory '%s' removed\n", arguments);
                }
            }
        } else if (strcmp(command, "write") == 0) {
            if (!arguments || strlen(arguments) == 0) {
                printf("Usage: write <filename> \"content\"\n");
                continue;
            }
            char fileName[MAX_NAME_LEN + 1];
            char *content = NULL;
            parseWriteArguments(arguments, fileName, &content);
            if (strlen(fileName) == 0) {
                printf("Error: missing filename\n");
                if (content) free(content);
                continue;
            }
            VfsNode *fileNode = findChildNode(currentDirectory, fileName);
            if (!fileNode) {
                printf("Error: file '%s' not found\n", fileName);
                if (content) free(content);
                continue;
            }
            if (fileNode->isDirectory) {
                printf("Error: '%s' is a directory\n", fileName);
                if (content) free(content);
                continue;
            }
            int size = content ? (int)strlen(content) : 0;
            int result = writeFileContent(fileNode, (const unsigned char *)(content ? content : ""), size);
            if (content) free(content);
            if (result == 0) {
                printf("Data written (%d bytes) to %s/%s\n", size, currentPath, fileName);
            } else if (result == -2) {
                printf("Error: not enough disk space\n");
            } else {
                printf("Error: write failed\n");
            }
        } else if (strcmp(command, "read") == 0) {
            if (!arguments || strlen(arguments) == 0) {
                printf("Usage: read <filename>\n");
                continue;
            }
            VfsNode *fileNode = findChildNode(currentDirectory, arguments);
            if (!fileNode) {
                printf("Error: file '%s' not found\n", arguments);
                continue;
            }
            if (fileNode->isDirectory) {
                printf("Error: '%s' is a directory\n", arguments);
                continue;
            }
            readFileContent(fileNode);
            printf("\n");
        } else if (strcmp(command, "delete") == 0) {
            if (!arguments || strlen(arguments) == 0) {
                printf("Usage: delete <filename>\n");
                continue;
            }
            VfsNode *fileNode = findChildNode(currentDirectory, arguments);
            if (!fileNode) {
                printf("Error: file '%s' not found\n", arguments);
                continue;
            }
            if (fileNode->isDirectory) {
                printf("Error: '%s' is a directory. Use rmdir\n", arguments);
                continue;
            }
            deleteFileNode(fileNode);
            printf("File '%s' deleted\n", arguments);
        } else if (strcmp(command, "exit") == 0) {
            cleanupVfs();
            printf("Memory released. Exiting...\n");
            break;
        } else if (strlen(command) > 0) {
            printf("Unknown command: %s\n", command);
        }
    }
}

void initializeVfs() {
    virtualDisk = malloc((size_t)TOTAL_BLOCKS * BLOCK_SIZE);
    if (!virtualDisk) exitWithError("Cannot allocate virtual disk");
    memset(virtualDisk, 0, (size_t)TOTAL_BLOCKS * BLOCK_SIZE);
    initializeFreeBlockList(TOTAL_BLOCKS);
    rootDirectory = createVfsNode("/", 1, NULL);
    currentDirectory = rootDirectory;
    usedBlockCount = 0;
}

int main() {
    initializeVfs();
    runShellLoop();
    return 0;
}
