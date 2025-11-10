#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 1024
#define MAX_NAME_LEN 50
#define MAX_CMD_LEN 2048

typedef struct BlockNode {
    int index;
    struct BlockNode *prev;
    struct BlockNode *next;
} BlockNode;

typedef struct FileNode {
    char name[MAX_NAME_LEN + 1];
    int isDir;
    struct FileNode *parent;
    struct FileNode *child;
    struct FileNode *next;
    struct FileNode *prev;
    int *blockRefs;
    int blockCount;
    int contentSize;
} FileNode;

unsigned char *diskMemory = NULL;
BlockNode *freeHead = NULL;
BlockNode *freeTail = NULL;
int usedBlocks = 0;

FileNode *rootDir = NULL;
FileNode *currentDir = NULL;

void fatal(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

BlockNode* createBlockNode(int index) {
    BlockNode *n = malloc(sizeof(BlockNode));
    if (!n) fatal("out of memory");
    n->index = index;
    n->prev = n->next = NULL;
    return n;
}

void initFreeBlockList(int total) {
    freeHead = freeTail = NULL;
    for (int i = 0; i < total; ++i) {
        BlockNode *n = createBlockNode(i);
        if (!freeHead) freeHead = freeTail = n;
        else {
            freeTail->next = n;
            n->prev = freeTail;
            freeTail = n;
        }
    }
}

int allocateBlock() {
    if (!freeHead) return -1;
    BlockNode *n = freeHead;
    int idx = n->index;
    freeHead = n->next;
    if (freeHead) freeHead->prev = NULL;
    else freeTail = NULL;
    free(n);
    usedBlocks++;
    return idx;
}

void releaseBlock(int index) {
    BlockNode *n = createBlockNode(index);
    if (!freeTail) freeHead = freeTail = n;
    else {
        freeTail->next = n;
        n->prev = freeTail;
        freeTail = n;
    }
    usedBlocks--;
}

int countFreeBlocks() {
    return TOTAL_BLOCKS - usedBlocks;
}

FileNode* createFileNode(const char *name, int isDir, FileNode *parent) {
    if (strlen(name) > MAX_NAME_LEN) return NULL;
    FileNode *f = malloc(sizeof(FileNode));
    if (!f) fatal("out of memory");
    strncpy(f->name, name, MAX_NAME_LEN);
    f->name[MAX_NAME_LEN] = '\0';
    f->isDir = isDir;
    f->parent = parent;
    f->child = NULL;
    f->next = f->prev = NULL;
    f->blockRefs = NULL;
    f->blockCount = 0;
    f->contentSize = 0;
    return f;
}

int addChild(FileNode *parent, FileNode *child) {
    if (!parent || !parent->isDir) return -1;
    if (!parent->child) {
        parent->child = child;
        child->next = child->prev = child;
    } else {
        FileNode *first = parent->child;
        FileNode *last = first->prev;
        last->next = child;
        child->prev = last;
        child->next = first;
        first->prev = child;
    }
    child->parent = parent;
    return 0;
}

int removeChild(FileNode *parent, FileNode *node) {
    if (!parent || !parent->isDir || !parent->child || !node) return -1;
    if (parent->child == node && node->next == node) {
        parent->child = NULL;
    } else {
        if (parent->child == node) parent->child = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    node->next = node->prev = NULL;
    node->parent = NULL;
    return 0;
}

FileNode* findChild(FileNode *parent, const char *name) {
    if (!parent || !parent->isDir) return NULL;
    FileNode *c = parent->child;
    if (!c) return NULL;
    FileNode *it = c;
    do {
        if (strcmp(it->name, name) == 0) return it;
        it = it->next;
    } while (it != c);
    return NULL;
}

void freeFileNode(FileNode *node) {
    if (!node) return;
    if (node->blockRefs) {
        free(node->blockRefs);
        node->blockRefs = NULL;
    }
    free(node);
}

int writeFile(FileNode *file, const unsigned char *content, int size) {
    if (!file || file->isDir) return -1;
    if (size < 0) size = 0;
    int required = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (required > countFreeBlocks()) return -2;
    if (file->blockCount > 0 && file->blockRefs) {
        for (int i = 0; i < file->blockCount; ++i) releaseBlock(file->blockRefs[i]);
        free(file->blockRefs);
        file->blockRefs = NULL;
        file->blockCount = 0;
        file->contentSize = 0;
    }
    if (required == 0) {
        file->blockRefs = NULL;
        file->blockCount = 0;
        file->contentSize = 0;
        return 0;
    }
    file->blockRefs = malloc(sizeof(int) * required);
    if (!file->blockRefs) fatal("out of memory");
    for (int i = 0; i < required; ++i) {
        int idx = allocateBlock();
        if (idx < 0) {
            for (int j = 0; j < i; ++j) releaseBlock(file->blockRefs[j]);
            free(file->blockRefs);
            file->blockRefs = NULL;
            file->blockCount = 0;
            return -2;
        }
        file->blockRefs[i] = idx;
        unsigned char *blk = diskMemory + ((size_t)idx * BLOCK_SIZE);
        int copyStart = i * BLOCK_SIZE;
        int toCopy = BLOCK_SIZE;
        if (copyStart + toCopy > size) toCopy = size - copyStart;
        if (toCopy > 0) memcpy(blk, content + copyStart, toCopy);
        if (toCopy < BLOCK_SIZE) memset(blk + toCopy, 0, BLOCK_SIZE - toCopy);
    }
    file->blockCount = required;
    file->contentSize = size;
    return 0;
}

int readFile(FileNode *file) {
    if (!file || file->isDir) return -1;
    if (file->blockCount == 0 || file->blockRefs == NULL || file->contentSize == 0) return 0;
    int remaining = file->contentSize;
    for (int i = 0; i < file->blockCount; ++i) {
        int idx = file->blockRefs[i];
        unsigned char *blk = diskMemory + ((size_t)idx * BLOCK_SIZE);
        int toPrint = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        fwrite(blk, 1, toPrint, stdout);
        remaining -= toPrint;
        if (remaining <= 0) break;
    }
    return 0;
}

int deleteFile(FileNode *file) {
    if (!file || file->isDir) return -1;
    if (file->blockCount > 0 && file->blockRefs) {
        for (int i = 0; i < file->blockCount; ++i) releaseBlock(file->blockRefs[i]);
        free(file->blockRefs);
        file->blockRefs = NULL;
        file->blockCount = 0;
        file->contentSize = 0;
    }
    if (file->parent) removeChild(file->parent, file);
    freeFileNode(file);
    return 0;
}

int removeDir(FileNode *dir) {
    if (!dir || !dir->isDir) return -1;
    if (dir->child != NULL) return -2;
    if (dir->parent) removeChild(dir->parent, dir);
    freeFileNode(dir);
    return 0;
}

int makeDirCmd(const char *name) {
    if (!name || strlen(name) == 0) { printf("Usage: mkdir <name>\n"); return -1; }
    if (strlen(name) > MAX_NAME_LEN) { printf("Error: name too long\n"); return -1; }
    if (strchr(name, '/')) { printf("Error: name cannot contain '/'\n"); return -1; }
    if (findChild(currentDir, name)) { printf("Error: directory '%s' already exists\n", name); return -1; }
    FileNode *d = createFileNode(name, 1, currentDir);
    addChild(currentDir, d);
    printf("Directory '%s' created\n", name);
    return 0;
}

int createFileCmd(const char *name) {
    if (!name || strlen(name) == 0) { printf("Usage: create <filename>\n"); return -1; }
    if (strlen(name) > MAX_NAME_LEN) { printf("Error: name too long\n"); return -1; }
    if (strchr(name, '/')) { printf("Error: name cannot contain '/'\n"); return -1; }
    if (findChild(currentDir, name)) { printf("Error: file '%s' already exists\n", name); return -1; }
    FileNode *f = createFileNode(name, 0, currentDir);
    addChild(currentDir, f);
    printf("File '%s' created\n", name);
    return 0;
}

void buildFullPath(FileNode *node, char *out, int outSize) {
    if (!node || node == rootDir) { strncpy(out, "/", outSize-1); out[outSize-1] = '\0'; return; }
    char stack[256][MAX_NAME_LEN + 1];
    int top = 0;
    FileNode *p = node;
    while (p && p != rootDir) {
        strncpy(stack[top++], p->name, MAX_NAME_LEN);
        stack[top-1][MAX_NAME_LEN] = '\0';
        p = p->parent;
    }
    out[0] = '\0';
    for (int i = top - 1; i >= 0; --i) {
        strncat(out, "/", outSize - strlen(out) - 1);
        strncat(out, stack[i], outSize - strlen(out) - 1);
    }
    if (strlen(out) == 0) strncpy(out, "/", outSize-1);
    out[outSize-1] = '\0';
}

int changeDirPath(const char *path) {
    if (!path || strlen(path) == 0) { printf("Usage: cd <path>\n"); return -1; }
    FileNode *start = currentDir;
    if (path[0] == '/') start = rootDir;
    char copy[MAX_CMD_LEN];
    strncpy(copy, path, sizeof(copy)-1);
    copy[sizeof(copy)-1] = '\0';
    char *token = strtok(copy, "/");
    FileNode *cur = start;
    while (token) {
        if (strcmp(token, "") == 0) { token = strtok(NULL, "/"); continue; }
        if (strcmp(token, ".") == 0) { token = strtok(NULL, "/"); continue; }
        if (strcmp(token, "..") == 0) { if (cur->parent) cur = cur->parent; token = strtok(NULL, "/"); continue; }
        FileNode *nx = findChild(cur, token);
        if (!nx) { printf("Error: path component '%s' not found\n", token); return -1; }
        if (!nx->isDir) { printf("Error: '%s' is not a directory\n", token); return -1; }
        cur = nx;
        token = strtok(NULL, "/");
    }
    currentDir = cur;
    char full[1024]; buildFullPath(currentDir, full, sizeof(full));
    printf("Moved to %s\n", full);
    return 0;
}

void listCmd() {
    if (!currentDir->child) { printf("(empty)\n"); return; }
    FileNode *c = currentDir->child;
    FileNode *it = c;
    do {
        printf("%s%s\n", it->name, it->isDir ? "/" : "");
        it = it->next;
    } while (it != c);
}

void dfCmd() {
    int freeB = countFreeBlocks();
    double usedPercent = (double)usedBlocks / (double)TOTAL_BLOCKS * 100.0;
    printf("Total Blocks: %d\nUsed Blocks: %d\nFree Blocks: %d\nDisk Usage: %.2f%%\n", TOTAL_BLOCKS, usedBlocks, freeB, usedPercent);
}

void parseWriteArgs(const char *argline, char *outName, char **outContent) {
    outName[0] = '\0';
    *outContent = NULL;
    if (!argline) return;
    const char *p = argline;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (!*p) return;
    int i = 0;
    while (*p && !isspace((unsigned char)*p) && i < MAX_NAME_LEN) outName[i++] = *p++;
    outName[i] = '\0';
    while (*p && isspace((unsigned char)*p)) ++p;
    if (!*p) return;
    if (*p == '"' || *p == '\'') {
        char q = *p++;
        const char *start = p;
        while (*p && *p != q) ++p;
        size_t len = p - start;
        *outContent = malloc(len + 1);
        if (!*outContent) fatal("out of memory");
        memcpy(*outContent, start, len);
        (*outContent)[len] = '\0';
    } else {
        size_t len = strlen(p);
        while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r')) len--;
        *outContent = malloc(len + 1);
        if (!*outContent) fatal("out of memory");
        memcpy(*outContent, p, len);
        (*outContent)[len] = '\0';
    }
}

void cleanupRecursive(FileNode *dir) {
    if (!dir) return;
    while (dir->child) {
        FileNode *child = dir->child;
        removeChild(dir, child);
        if (child->isDir) cleanupRecursive(child);
        else {
            if (child->blockCount > 0 && child->blockRefs) {
                for (int i = 0; i < child->blockCount; ++i) releaseBlock(child->blockRefs[i]);
                free(child->blockRefs);
                child->blockRefs = NULL;
            }
            freeFileNode(child);
        }
    }
    freeFileNode(dir);
}

void cleanupAll() {
    if (rootDir) {
        cleanupRecursive(rootDir);
        rootDir = currentDir = NULL;
    }
    BlockNode *it = freeHead;
    while (it) {
        BlockNode *nxt = it->next;
        free(it);
        it = nxt;
    }
    freeHead = freeTail = NULL;
    if (diskMemory) free(diskMemory);
    diskMemory = NULL;
}

void promptAndRun() {
    printf("Compact VFS ready. Type 'exit' to quit.\n");
    char line[MAX_CMD_LEN];
    while (1) {
        char path[1024];
        buildFullPath(currentDir, path, sizeof(path));
        printf("%s > ", path);
        if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }
        char *s = line;
        while (*s && (*s == '\n' || *s == '\r')) { s++; }
        char cmd[64] = {0};
        int pos = 0;
        while (*s && isspace((unsigned char)*s)) ++s;
        while (*s && !isspace((unsigned char)*s) && pos < (int)sizeof(cmd)-1) cmd[pos++] = *s++;
        cmd[pos] = '\0';
        while (*s && isspace((unsigned char)*s)) ++s;
        char args[MAX_CMD_LEN] = {0};
        if (*s) {
            strncpy(args, s, sizeof(args)-1);
            args[sizeof(args)-1] = '\0';
            int L = strlen(args);
            while (L > 0 && (args[L-1] == '\n' || args[L-1] == '\r')) args[--L] = '\0';
        }
        if (strcmp(cmd, "mkdir") == 0) makeDirCmd(args);
        else if (strcmp(cmd, "create") == 0) createFileCmd(args);
        else if (strcmp(cmd, "ls") == 0) listCmd();
        else if (strcmp(cmd, "pwd") == 0) { char pth[1024]; buildFullPath(currentDir, pth, sizeof(pth)); printf("%s\n", pth); }
        else if (strcmp(cmd, "df") == 0) dfCmd();
        else if (strcmp(cmd, "cd") == 0) changeDirPath(args);
        else if (strcmp(cmd, "rmdir") == 0) {
            if (!args || strlen(args)==0) { printf("Usage: rmdir <dirname>\n"); }
            else {
                FileNode *d = findChild(currentDir, args);
                if (!d) printf("Error: directory '%s' not found\n", args);
                else if (!d->isDir) printf("Error: '%s' is not a directory\n", args);
                else if (d->child != NULL) printf("Error: directory not empty\n");
                else { removeDir(d); printf("Directory '%s' removed\n", args); }
            }
        }
        else if (strcmp(cmd, "write") == 0) {
            if (!args || strlen(args)==0) { printf("Usage: write <filename> \"content\"\n"); continue; }
            char fname[MAX_NAME_LEN+1];
            char *content = NULL;
            parseWriteArgs(args, fname, &content);
            if (strlen(fname) == 0) { printf("Error: missing filename\n"); if (content) free(content); continue; }
            FileNode *f = findChild(currentDir, fname);
            if (!f) { printf("Error: file '%s' not found\n", fname); if (content) free(content); continue; }
            if (f->isDir) { printf("Error: '%s' is a directory\n", fname); if (content) free(content); continue; }
            int size = content ? (int)strlen(content) : 0;
            int r = writeFile(f, (const unsigned char*)(content?content:""), size);
            if (content) free(content);
            if (r == 0) printf("Data written (%d bytes) to %s/%s\n", size, path, fname);
            else if (r == -2) printf("Error: not enough disk space\n");
            else printf("Error: write failed\n");
        }
        else if (strcmp(cmd, "read") == 0) {
            if (!args || strlen(args)==0) { printf("Usage: read <filename>\n"); continue; }
            FileNode *f = findChild(currentDir, args);
            if (!f) { printf("Error: file '%s' not found\n", args); continue; }
            if (f->isDir) { printf("Error: '%s' is a directory\n", args); continue; }
            readFile(f); printf("\n");
        }
        else if (strcmp(cmd, "delete") == 0) {
            if (!args || strlen(args)==0) { printf("Usage: delete <filename>\n"); continue; }
            FileNode *f = findChild(currentDir, args);
            if (!f) { printf("Error: file '%s' not found\n", args); continue; }
            if (f->isDir) { printf("Error: '%s' is a directory. Use rmdir\n", args); continue; }
            deleteFile(f);
            printf("File '%s' deleted\n", args);
        }
        else if (strcmp(cmd, "exit") == 0) {
            cleanupAll();
            printf("Memory released. Exiting...\n");
            break;
        }
        else if (strlen(cmd) > 0) printf("Unknown command: %s\n", cmd);
    }
}

void initVfs() {
    diskMemory = malloc((size_t)TOTAL_BLOCKS * BLOCK_SIZE);
    if (!diskMemory) fatal("cannot allocate disk memory");
    memset(diskMemory, 0, (size_t)TOTAL_BLOCKS * BLOCK_SIZE);
    initFreeBlockList(TOTAL_BLOCKS);
    rootDir = createFileNode("/", 1, NULL);
    currentDir = rootDir;
    usedBlocks = 0;
}

int main() {
    initVfs();
    promptAndRun();
    return 0;
}
