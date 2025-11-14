#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Players_data.h"

#define MAX_TEAMS_LOCAL 10
#define MAX_NAME_LENGTH 51
#define MAX_PLAYERS_PER_TEAM 50

typedef enum { RoleUnknown = 0, RoleBatsman = 1, RoleBowler = 2, RoleAllRounder = 3 } RoleType;

typedef struct PlayerNode {
    int playerId;
    char playerName[MAX_NAME_LENGTH];
    char teamName[MAX_NAME_LENGTH];
    RoleType role;
    long totalRuns;
    double battingAverage;
    double strikeRate;
    int wickets;
    double economyRate;
    double performanceIndex;
    struct PlayerNode *teamNext;
    struct PlayerNode *roleNext;
} PlayerNode;

typedef struct TeamInformationStructure {
    int teamId;
    char teamName[MAX_NAME_LENGTH];
    int totalPlayers;
    double averageBattingStrikeRate;
    PlayerNode *playerListHead;
} TeamInformationStructure;

static TeamInformationStructure teamInformationArray[MAX_TEAMS_LOCAL];
static int totalNumberOfTeamsLocal = MAX_TEAMS_LOCAL;
static PlayerNode *roleListHeads[4] = { NULL, NULL, NULL, NULL };
static PlayerNode **allocatedPlayerPointers = NULL;
static size_t allocatedPlayerPointerCount = 0;
static size_t allocatedPlayerPointerCapacity = 0;

int readIntegerInRange(const char *prompt, int minValue, int maxValue) {
    char buffer[128];
    long val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) { printf("Input error, try again.\n"); continue; }
        if (sscanf(buffer, "%ld", &val) == 1) {
            if (val < minValue || val > maxValue) {
                printf("Wrong input: value must be between %d and %d. Try again.\n", minValue, maxValue);
                continue;
            }
            return (int)val;
        }
        printf("Wrong input: not an integer. Try again.\n");
    }
}

long readLongInRange(const char *prompt, long minValue, long maxValue) {
    char buffer[256];
    long val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) { printf("Input error, try again.\n"); continue; }
        if (sscanf(buffer, "%ld", &val) == 1) {
            if (val < minValue || val > maxValue) {
                printf("Wrong input: value must be between %ld and %ld. Try again.\n", minValue, maxValue);
                continue;
            }
            return val;
        }
        printf("Wrong input: not a valid number. Try again.\n");
    }
}

double readDoubleInRange(const char *prompt, double minValue, double maxValue) {
    char buffer[256];
    double val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) { printf("Input error, try again.\n"); continue; }
        if (sscanf(buffer, "%lf", &val) == 1) {
            if (!(val >= minValue && val <= maxValue)) {
                printf("Wrong input: value must be between %.2f and %.2f. Try again.\n", minValue, maxValue);
                continue;
            }
            return val;
        }
        printf("Wrong input: not a valid decimal. Try again.\n");
    }
}

void readStringLineStrict(const char *prompt, char *outputBuffer, int maxLength) {
    char buffer[512];
    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) { printf("Input error, try again.\n"); continue; }
        size_t n = strlen(buffer);
        if (n > 0 && buffer[n-1] == '\n') buffer[n-1] = '\0';
        if (strlen(buffer) < 1 || strlen(buffer) >= (size_t)maxLength) {
            printf("Wrong input: length must be 1..%d. Try again.\n", maxLength - 1);
            continue;
        }
        int ok = 1;
        for (size_t i = 0; i < strlen(buffer); ++i) {
            char c = buffer[i];
            if (!(isalpha((unsigned char)c) || isdigit((unsigned char)c) || isspace((unsigned char)c) || c == '.' || c == '-')) {
                ok = 0; break;
            }
        }
        if (!ok) { printf("Wrong input: invalid character found. Try again.\n"); continue; }
        strncpy(outputBuffer, buffer, maxLength - 1);
        outputBuffer[maxLength - 1] = '\0';
        return;
    }
}

RoleType convertRoleStringToEnum(const char *roleString) {
    if (!roleString) return RoleUnknown;
    if (strcasecmp(roleString, "Batsman") == 0) return RoleBatsman;
    if (strcasecmp(roleString, "Bowler") == 0) return RoleBowler;
    if (strcasecmp(roleString, "All-rounder") == 0 || strcasecmp(roleString, "Allrounder") == 0 || strcasecmp(roleString, "All Rounder") == 0) return RoleAllRounder;
    return RoleUnknown;
}

double computePerformanceIndexForPlayer(RoleType role, double battingAverage, double strikeRate, int wickets, double economyRate) {
    if (role == RoleBatsman) return (battingAverage * strikeRate) / 100.0;
    if (role == RoleBowler) return (wickets * 2.0) + (100.0 - economyRate);
    if (role == RoleAllRounder) return ((battingAverage * strikeRate) / 100.0) + (wickets * 2.0);
    return 0.0;
}

void storeAllocatedPlayerPointer(PlayerNode *nodePointer) {
    if (allocatedPlayerPointerCount + 1 > allocatedPlayerPointerCapacity) {
        allocatedPlayerPointerCapacity = (allocatedPlayerPointerCapacity == 0) ? 128 : allocatedPlayerPointerCapacity * 2;
        allocatedPlayerPointers = realloc(allocatedPlayerPointers, allocatedPlayerPointerCapacity * sizeof(PlayerNode *));
        if (!allocatedPlayerPointers) { perror("realloc"); exit(1); }
    }
    allocatedPlayerPointers[allocatedPlayerPointerCount++] = nodePointer;
}

PlayerNode *createPlayerNode(int id, const char *name, const char *teamName, RoleType role, long runs, double avg, double sr, int wkts, double er) {
    PlayerNode *p = malloc(sizeof(PlayerNode));
    if (!p) { perror("malloc"); exit(1); }
    p->playerId = id;
    strncpy(p->playerName, name, MAX_NAME_LENGTH - 1); p->playerName[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(p->teamName, teamName, MAX_NAME_LENGTH - 1); p->teamName[MAX_NAME_LENGTH - 1] = '\0';
    p->role = role;
    p->totalRuns = runs;
    p->battingAverage = avg;
    p->strikeRate = sr;
    p->wickets = wkts;
    p->economyRate = er;
    p->performanceIndex = computePerformanceIndexForPlayer(role, avg, sr, wkts, er);
    p->teamNext = NULL;
    p->roleNext = NULL;
    storeAllocatedPlayerPointer(p);
    return p;
}

int isPlayerIdAlreadyPresent(int idToCheck) {
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) {
        PlayerNode *cur = teamInformationArray[i].playerListHead;
        while (cur) {
            if (cur->playerId == idToCheck) return 1;
            cur = cur->teamNext;
        }
    }
    return 0;
}

int findTeamIndexById(int teamId) {
    int left = 0, right = totalNumberOfTeamsLocal - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (teamInformationArray[mid].teamId == teamId) return mid;
        if (teamInformationArray[mid].teamId < teamId) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}

void insertPlayerIntoTeamAndRoleLists(PlayerNode *playerNode) {
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) {
        if (strcasecmp(teamInformationArray[i].teamName, playerNode->teamName) == 0) {
            playerNode->teamNext = teamInformationArray[i].playerListHead;
            teamInformationArray[i].playerListHead = playerNode;
            teamInformationArray[i].totalPlayers += 1;
            playerNode->roleNext = roleListHeads[playerNode->role];
            roleListHeads[playerNode->role] = playerNode;
            return;
        }
    }
}

void recomputeAverageStrikeRateForTeam(TeamInformationStructure *team) {
    if (!team) return;
    PlayerNode *cur = team->playerListHead;
    double sum = 0.0;
    int count = 0;
    while (cur) {
        if (cur->role == RoleBatsman || cur->role == RoleAllRounder) { sum += cur->strikeRate; count++; }
        cur = cur->teamNext;
    }
    team->averageBattingStrikeRate = (count == 0) ? 0.0 : (sum / count);
}

void initializeTeamInformationArrayAlphabetical(void) {
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) {
        teamInformationArray[i].teamId = i + 1;
        strncpy(teamInformationArray[i].teamName, teams[i], MAX_NAME_LENGTH - 1);
        teamInformationArray[i].teamName[MAX_NAME_LENGTH - 1] = '\0';
        teamInformationArray[i].totalPlayers = 0;
        teamInformationArray[i].averageBattingStrikeRate = 0.0;
        teamInformationArray[i].playerListHead = NULL;
    }
}

void loadPlayersFromHeaderIntoStructures(void) {
    for (int i = 0; i < playerCount; ++i) {
        const Player *src = &players[i];
        RoleType r = convertRoleStringToEnum(src->role);
        PlayerNode *pn = createPlayerNode(src->id, src->name, src->team, r, src->totalRuns, src->battingAverage, src->strikeRate, src->wickets, src->economyRate);
        insertPlayerIntoTeamAndRoleLists(pn);
    }
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) recomputeAverageStrikeRateForTeam(&teamInformationArray[i]);
}

void displayPlayerFullLine(const PlayerNode *p) {
    const char *roleString = (p->role == RoleBatsman) ? "Batsman" : (p->role == RoleBowler) ? "Bowler" : "All-rounder";
    printf("%-6d %-20s %-12s %8ld %7.2f %6.1f %6d %6.1f %11.2f\n", p->playerId, p->playerName, roleString, p->totalRuns, p->battingAverage, p->strikeRate, p->wickets, p->economyRate, p->performanceIndex);
}

void menuAddNewPlayerToTeam(void) {
    printf("\n-- Add Player to Team --\n");
    int teamId = readIntegerInRange("Enter Team ID to add player (1..10): ", 1, totalNumberOfTeamsLocal);
    int teamIndex = findTeamIndexById(teamId);
    if (teamIndex == -1) { printf("Team ID %d not found.\n", teamId); return; }
    TeamInformationStructure *team = &teamInformationArray[teamIndex];
    if (team->totalPlayers >= MAX_PLAYERS_PER_TEAM) { printf("Team %s already has maximum allowed players (%d).\n", team->teamName, MAX_PLAYERS_PER_TEAM); return; }
    int playerId;
    while (1) {
        playerId = readIntegerInRange("Player ID (1..1500): ", 1, 1500);
        if (isPlayerIdAlreadyPresent(playerId)) { printf("ERROR: A player with ID %d already exists. Please enter a unique Player ID.\n", playerId); }
        else break;
    }
    char name[MAX_NAME_LENGTH];
    readStringLineStrict("Name: ", name, MAX_NAME_LENGTH);
    int roleChoice = readIntegerInRange("Role (1-Batsman, 2-Bowler, 3-All-rounder): ", 1, 3);
    RoleType role = (RoleType)roleChoice;
    long runs = readLongInRange("Total Runs (>=0): ", 0, 1000000000L);
    double avg = readDoubleInRange("Batting Average (>=0): ", 0.0, 10000.0);
    double sr = readDoubleInRange("Strike Rate (>=0): ", 0.0, 10000.0);
    int wkts = readIntegerInRange("Wickets (>=0): ", 0, 1000000);
    double er = readDoubleInRange("Economy Rate (>=0): ", 0.0, 1000.0);
    PlayerNode *pn = createPlayerNode(playerId, name, team->teamName, role, runs, avg, sr, wkts, er);
    insertPlayerIntoTeamAndRoleLists(pn);
    recomputeAverageStrikeRateForTeam(team);
    printf("Player added successfully to Team %s!\n", team->teamName);
}

void menuDisplayPlayersOfSpecificTeam(void) {
    printf("\n-- Display Players of a Specific Team --\n");
    int teamId = readIntegerInRange("Enter Team ID: ", 1, totalNumberOfTeamsLocal);
    int idx = findTeamIndexById(teamId);
    if (idx == -1) { printf("Team ID %d not found.\n", teamId); return; }
    TeamInformationStructure *team = &teamInformationArray[idx];
    printf("\nPlayers of Team %s:\n", team->teamName);
    printf("=============================================================================================\n");
    printf("%-6s %-20s %-12s %8s %7s %6s %6s %6s %11s\n", "ID", "Name", "Role", "Runs", "Avg", "SR", "Wkts", "ER", "Perf.Index");
    printf("=============================================================================================\n");
    PlayerNode *cur = team->playerListHead;
    if (!cur) printf("(No players found)\n");
    while (cur) {
        displayPlayerFullLine(cur);
        cur = cur->teamNext;
    }
    printf("---------------------------------------------------------------------------------------------\n");
    printf("Total Players: %d\n", team->totalPlayers);
    printf("Average Batting Strike Rate: %.2f\n", team->averageBattingStrikeRate);
}

int compareTeamsByAverageDesc(const void *a, const void *b) {
    const TeamInformationStructure *ta = a;
    const TeamInformationStructure *tb = b;
    if (ta->averageBattingStrikeRate < tb->averageBattingStrikeRate) return 1;
    if (ta->averageBattingStrikeRate > tb->averageBattingStrikeRate) return -1;
    return 0;
}

void menuDisplayTeamsByAverageStrikeRate(void) {
    printf("\n-- Teams by Average Batting Strike Rate (desc) --\n");
    TeamInformationStructure copyArray[MAX_TEAMS_LOCAL];
    memcpy(copyArray, teamInformationArray, sizeof(teamInformationArray));
    qsort(copyArray, totalNumberOfTeamsLocal, sizeof(TeamInformationStructure), compareTeamsByAverageDesc);
    printf("=============================================================\n");
    printf("%-4s %-15s %-18s %-12s\n", "ID", "Team Name", "Avg Bat SR", "Total Players");
    printf("=============================================================\n");
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) {
        printf("%-4d %-15s %-18.2f %-12d\n", copyArray[i].teamId, copyArray[i].teamName, copyArray[i].averageBattingStrikeRate, copyArray[i].totalPlayers);
    }
}

PlayerNode **gatherPlayersOfTeamByRole(TeamInformationStructure *team, RoleType role, int *outCount) {
    int cap = 16;
    PlayerNode **arr = malloc(cap * sizeof(PlayerNode *));
    int cnt = 0;
    PlayerNode *cur = team->playerListHead;
    while (cur) {
        if (cur->role == role) {
            if (cnt >= cap) { cap *= 2; arr = realloc(arr, cap * sizeof(PlayerNode *)); }
            arr[cnt++] = cur;
        }
        cur = cur->teamNext;
    }
    *outCount = cnt;
    return arr;
}

int comparePlayersByPerformanceDesc(const void *a, const void *b) {
    const PlayerNode *pa = *(const PlayerNode **)a;
    const PlayerNode *pb = *(const PlayerNode **)b;
    if (pa->performanceIndex < pb->performanceIndex) return 1;
    if (pa->performanceIndex > pb->performanceIndex) return -1;
    return 0;
}

void menuDisplayTopKPlayersOfSpecificTeamByRole(void) {
    printf("\n-- Top K Players of a Specific Team by Role --\n");
    int teamId = readIntegerInRange("Enter Team ID: ", 1, totalNumberOfTeamsLocal);
    int idx = findTeamIndexById(teamId);
    if (idx == -1) { printf("Team ID %d not found.\n", teamId); return; }
    TeamInformationStructure *team = &teamInformationArray[idx];
    int roleChoice = readIntegerInRange("Enter Role (1-Batsman, 2-Bowler, 3-All-rounder): ", 1, 3);
    RoleType role = (RoleType)roleChoice;
    int k = readIntegerInRange("Enter number of players (K): ", 1, 1000);
    int count = 0;
    PlayerNode **arr = gatherPlayersOfTeamByRole(team, role, &count);
    if (count == 0) {
        printf("No players of that role in team %s.\n", team->teamName);
        free(arr);
        return;
    }
    qsort(arr, count, sizeof(PlayerNode *), comparePlayersByPerformanceDesc);
    if (k > count) k = count;
    printf("\nTop %d players of role %d in Team %s:\n", k, role, team->teamName);
    printf("=============================================================================================\n");
    printf("%-6s %-20s %-12s %8s %7s %6s %6s %6s %11s\n", "ID", "Name", "Role", "Runs", "Avg", "SR", "Wkts", "ER", "Perf.Index");
    printf("=============================================================================================\n");
    for (int i = 0; i < k; ++i) displayPlayerFullLine(arr[i]);
    free(arr);
}

PlayerNode **gatherPlayersByRoleGlobal(RoleType role, int *outCount) {
    int cap = 64;
    PlayerNode **arr = malloc(cap * sizeof(PlayerNode *));
    int cnt = 0;
    PlayerNode *cur = roleListHeads[role];
    while (cur) {
        if (cnt >= cap) { cap *= 2; arr = realloc(arr, cap * sizeof(PlayerNode *)); }
        arr[cnt++] = cur;
        cur = cur->roleNext;
    }
    *outCount = cnt;
    return arr;
}

void menuDisplayAllPlayersOfSpecificRoleAcrossTeams(void) {
    printf("\n-- Display All Players of Specific Role Across All Teams --\n");
    int roleChoice = readIntegerInRange("Enter Role (1-Batsman, 2-Bowler, 3-All-rounder): ", 1, 3);
    RoleType role = (RoleType)roleChoice;
    int count = 0;
    PlayerNode **arr = gatherPlayersByRoleGlobal(role, &count);
    if (count == 0) { printf("No players of that role found across teams.\n"); free(arr); return; }
    qsort(arr, count, sizeof(PlayerNode *), comparePlayersByPerformanceDesc);
    printf("\nPlayers (Role %d) across all teams sorted by Performance Index (desc):\n", role);
    printf("=============================================================================================\n");
    printf("%-6s %-20s %-15s %-12s %8s %7s %6s %6s %11s\n", "ID", "Name", "Team", "Role", "Runs", "Avg", "SR", "Wkts", "Perf.Index");
    printf("=============================================================================================\n");
    for (int i = 0; i < count; ++i) {
        PlayerNode *p = arr[i];
        const char *roleString = (p->role == RoleBatsman) ? "Batsman" : (p->role == RoleBowler) ? "Bowler" : "All-rounder";
        printf("%-6d %-20s %-15s %-12s %8ld %7.2f %6.1f %6d %11.2f\n", p->playerId, p->playerName, p->teamName, roleString, p->totalRuns, p->battingAverage, p->strikeRate, p->wickets, p->performanceIndex);
    }
    free(arr);
}

void freeAllAllocatedMemoryAndExit(void) {
    for (size_t i = 0; i < allocatedPlayerPointerCount; ++i) free(allocatedPlayerPointers[i]);
    free(allocatedPlayerPointers);
    allocatedPlayerPointers = NULL;
    allocatedPlayerPointerCount = allocatedPlayerPointerCapacity = 0;
    for (int r = 1; r <= 3; ++r) roleListHeads[r] = NULL;
    for (int i = 0; i < totalNumberOfTeamsLocal; ++i) teamInformationArray[i].playerListHead = NULL;
}

int main(void) {
    initializeTeamInformationArrayAlphabetical();
    loadPlayersFromHeaderIntoStructures();
    while (1) {
        printf("\n=============================================================================\n");
        printf("ICC ODI Player Performance Analyzer\n");
        printf("=============================================================================\n");
        printf("1. Add Player to Team\n");
        printf("2. Display Players of a Specific Team\n");
        printf("3. Display Teams by Average Batting Strike Rate\n");
        printf("4. Display Top K Players of a Specific Team by Role\n");
        printf("5. Display all Players of specific role Across All Teams by performance index\n");
        printf("6. Exit\n");
        printf("=============================================================================\n");
        int choice = readIntegerInRange("Enter your choice: ", 1, 6);
        switch (choice) {
            case 1: menuAddNewPlayerToTeam(); break;
            case 2: menuDisplayPlayersOfSpecificTeam(); break;
            case 3: menuDisplayTeamsByAverageStrikeRate(); break;
            case 4: menuDisplayTopKPlayersOfSpecificTeamByRole(); break;
            case 5: menuDisplayAllPlayersOfSpecificRoleAcrossTeams(); break;
            case 6:
                printf("Exiting. Cleaning up memory.\n");
                freeAllAllocatedMemoryAndExit();
                return 0;
            default: printf("Invalid choice. Try again.\n");
        }
    }
    return 0;
}
