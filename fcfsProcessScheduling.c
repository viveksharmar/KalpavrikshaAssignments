#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 200
#define MAX_PROCESS_NAME 50

typedef enum {
    processReady,
    processRunning,
    processWaiting,
    processTerminated,
    processKilled
} processExecutionState;

typedef struct processControlBlock {
    int processId;
    char processName[MAX_PROCESS_NAME];

    int cpuBurstTotal;
    int cpuBurstRemaining;

    int ioTriggerAfterCpu;
    int ioBurstTotal;
    int ioBurstRemaining;

    int cpuTimeConsumed;
    int processArrivalTime;
    int processCompletionTime;

    int turnaroundDuration;
    int waitingDuration;

    processExecutionState executionState;
    struct processControlBlock *next;
} processControlBlock;

typedef struct {
    processControlBlock *front;
    processControlBlock *rear;
} processQueue;

typedef struct terminationEvent {
    int targetProcessId;
    int terminationTick;
    struct terminationEvent *next;
} terminationEvent;

processQueue readyProcessQueue;
processQueue waitingProcessQueue;
processQueue finishedProcessQueue;

processControlBlock *activeCpuProcess = NULL;
terminationEvent *terminationEventList = NULL;

int globalSystemTick = 0;

int isValidNumber(const char *value) {
    if (!value || *value == '\0') return 0;
    for (int i = 0; value[i]; i++)
        if (!isdigit((unsigned char)value[i])) return 0;
    return 1;
}

void initializeProcessQueue(processQueue *queue) {
    queue->front = queue->rear = NULL;
}

int isProcessQueueEmpty(processQueue *queue) {
    return queue->front == NULL;
}

void enqueue(processQueue *queue, processControlBlock *process) {
    process->next = NULL;
    if (!queue->rear) {
        queue->front = queue->rear = process;
    } else {
        queue->rear->next = process;
        queue->rear = process;
    }
}

processControlBlock* dequeue(processQueue *queue) {
    if (isProcessQueueEmpty(queue)) return NULL;
    processControlBlock *process = queue->front;
    queue->front = process->next;
    if (!queue->front) queue->rear = NULL;
    process->next = NULL;
    return process;
}

processControlBlock* allocateNewProcess(
        char *name,
        int pid,
        int cpuBurst,
        int ioStart,
        int ioDuration
) {
    processControlBlock *process =
        (processControlBlock*)malloc(sizeof(processControlBlock));

    strcpy(process->processName, name);
    process->processId = pid;

    process->cpuBurstTotal = cpuBurst;
    process->cpuBurstRemaining = cpuBurst;

    process->ioTriggerAfterCpu = ioStart;
    process->ioBurstTotal = ioDuration;
    process->ioBurstRemaining = ioDuration;

    process->cpuTimeConsumed = 0;
    process->processArrivalTime = 0;
    process->processCompletionTime = -1;

    process->turnaroundDuration = 0;
    process->waitingDuration = 0;

    process->executionState = processReady;
    process->next = NULL;

    return process;
}

void registerTerminationEvent(int pid, int tick) {
    terminationEvent *event =
        (terminationEvent*)malloc(sizeof(terminationEvent));

    event->targetProcessId = pid;
    event->terminationTick = tick;
    event->next = terminationEventList;
    terminationEventList = event;
}

void handleTerminationEventsAtTickStart() {
    terminationEvent *event = terminationEventList;

    while (event) {
        if (event->terminationTick == globalSystemTick) {

            if (activeCpuProcess &&
                activeCpuProcess->processId == event->targetProcessId) {

                activeCpuProcess->executionState = processKilled;
                activeCpuProcess->processCompletionTime = globalSystemTick;
                enqueue(&finishedProcessQueue, activeCpuProcess);
                activeCpuProcess = NULL;
            }

            processQueue *queues[] = {
                &readyProcessQueue,
                &waitingProcessQueue
            };

            for (int i = 0; i < 2; i++) {
                processControlBlock *previous = NULL;
                processControlBlock *current = queues[i]->front;

                while (current) {
                    if (current->processId == event->targetProcessId) {
                        if (previous)
                            previous->next = current->next;
                        else
                            queues[i]->front = current->next;

                        if (current == queues[i]->rear)
                            queues[i]->rear = previous;

                        current->executionState = processKilled;
                        current->processCompletionTime = globalSystemTick;
                        enqueue(&finishedProcessQueue, current);
                        break;
                    }
                    previous = current;
                    current = current->next;
                }
            }
        }
        event = event->next;
    }
}

void progressIoOperationsForWaitingProcesses() {
    processControlBlock *previous = NULL;
    processControlBlock *current = waitingProcessQueue.front;

    while (current) {
        current->ioBurstRemaining--;

        if (current->ioBurstRemaining == 0) {
            processControlBlock *completedIoProcess = current;

            if (previous)
                previous->next = current->next;
            else
                waitingProcessQueue.front = current->next;

            if (completedIoProcess == waitingProcessQueue.rear)
                waitingProcessQueue.rear = previous;

            current = current->next;

            completedIoProcess->executionState = processReady;
            enqueue(&readyProcessQueue, completedIoProcess);
        } else {
            previous = current;
            current = current->next;
        }
    }
}

void selectNextProcessForCpuIfIdle() {
    if (!activeCpuProcess && !isProcessQueueEmpty(&readyProcessQueue)) {
        activeCpuProcess = dequeue(&readyProcessQueue);
        activeCpuProcess->executionState = processRunning;
    }
}

void executeCpuForSingleTick() {
    if (!activeCpuProcess) return;

    activeCpuProcess->cpuBurstRemaining--;
    activeCpuProcess->cpuTimeConsumed++;

    sleep(1);

    if (activeCpuProcess->ioBurstTotal > 0 &&
        activeCpuProcess->cpuTimeConsumed ==
        activeCpuProcess->ioTriggerAfterCpu &&
        activeCpuProcess->cpuBurstRemaining > 0) {

        activeCpuProcess->executionState = processWaiting;
        enqueue(&waitingProcessQueue, activeCpuProcess);
        activeCpuProcess = NULL;
        return;
    }

    if (activeCpuProcess &&
        activeCpuProcess->cpuBurstRemaining == 0) {

        activeCpuProcess->executionState = processTerminated;
        activeCpuProcess->processCompletionTime = globalSystemTick + 1;
        enqueue(&finishedProcessQueue, activeCpuProcess);
        activeCpuProcess = NULL;
    }
}

void computeFinalProcessStatistics() {
    processControlBlock *process = finishedProcessQueue.front;

    while (process) {
        if (process->executionState != processKilled) {
            process->turnaroundDuration =
                process->processCompletionTime -
                process->processArrivalTime;

            process->waitingDuration =
                process->turnaroundDuration -
                process->cpuBurstTotal;
        }
        process = process->next;
    }
}

void startFcfsSchedulerSimulation() {
    printf("\nStarting FCFS scheduling simulation...\n");

    while (1) {
        handleTerminationEventsAtTickStart();
        selectNextProcessForCpuIfIdle();
        executeCpuForSingleTick();
        progressIoOperationsForWaitingProcesses();

        globalSystemTick++;

        if (!activeCpuProcess &&
            isProcessQueueEmpty(&readyProcessQueue) &&
            isProcessQueueEmpty(&waitingProcessQueue))
            break;
    }

    printf("Simulation completed.\n");
}

int main() {
    char inputLine[MAX_LINE_LENGTH];

    initializeProcessQueue(&readyProcessQueue);
    initializeProcessQueue(&waitingProcessQueue);
    initializeProcessQueue(&finishedProcessQueue);

    printf("FCFS OS Process Scheduling Simulator\n\n");
    printf("Enter process details in the format:\n");
    printf("<process_name> <pid> <cpu_burst> <io_start> <io_duration>\n");
    printf("Use '-' for io_start and io_duration if no IO\n\n");
    printf("Enter KILL events as:\n");
    printf("KILL <pid> <kill_time>\n\n");
    printf("After entering all inputs, press CTRL+D (Linux/macOS)\n");
    printf("or CTRL+Z then ENTER (Windows) to start simulation.\n\n");

    while (fgets(inputLine, sizeof(inputLine), stdin)) {
        char firstWord[50];
        if (sscanf(inputLine, "%s", firstWord) != 1) continue;

        if (strcmp(firstWord, "KILL") == 0) {
            char pidValue[20], tickValue[20];
            if (sscanf(inputLine, "%*s %s %s", pidValue, tickValue) != 2)
                continue;
            if (!isValidNumber(pidValue) || !isValidNumber(tickValue))
                continue;

            registerTerminationEvent(
                atoi(pidValue),
                atoi(tickValue)
            );
        } else {
            char name[50], pidValue[20], cpuValue[20];
            char ioStartValue[20], ioDurationValue[20];

            if (sscanf(inputLine, "%s %s %s %s %s",
                       name, pidValue, cpuValue,
                       ioStartValue, ioDurationValue) != 5)
                continue;

            if (!isValidNumber(pidValue) || !isValidNumber(cpuValue))
                continue;

            int pid = atoi(pidValue);
            int cpu = atoi(cpuValue);
            int ioStart = -1;
            int ioDuration = 0;

            if (strcmp(ioStartValue, "-") != 0 &&
                strcmp(ioDurationValue, "-") != 0) {

                if (!isValidNumber(ioStartValue) ||
                    !isValidNumber(ioDurationValue))
                    continue;

                ioStart = atoi(ioStartValue);
                ioDuration = atoi(ioDurationValue);
            }

            enqueue(&readyProcessQueue,
                allocateNewProcess(
                    name, pid, cpu,
                    ioStart, ioDuration
                ));
        }
    }

    startFcfsSchedulerSimulation();
    computeFinalProcessStatistics();

    printf("\nPID\tName\tCPU\tIO\tStatus\t\tTurnaround\tWaiting\n");

    processControlBlock *process = finishedProcessQueue.front;
    while (process) {
        if (process->executionState == processKilled) {
            printf("%d\t%s\t%d\t%d\tKILLED at %d\t-\t\t-\n",
                   process->processId,
                   process->processName,
                   process->cpuBurstTotal,
                   process->ioBurstTotal,
                   process->processCompletionTime);
        } else {
            printf("%d\t%s\t%d\t%d\tOK\t\t%d\t\t%d\n",
                   process->processId,
                   process->processName,
                   process->cpuBurstTotal,
                   process->ioBurstTotal,
                   process->turnaroundDuration,
                   process->waitingDuration);
        }
        process = process->next;
    }

    return 0;
}
