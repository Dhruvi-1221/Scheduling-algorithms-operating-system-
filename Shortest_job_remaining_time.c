
#include <stdio.h>
#include <stdlib.h>

struct Process {
    int pid;
    int arrival;
    int burst;
    int remaining;
    int finish;
    int waiting;
    int turnaround;
    struct Process* next;
};

// Gantt chart node
struct GanttNode {
    int pid;        // 0 = idle
    int start;
    int end;
    struct GanttNode* next;
};

// ─── Gantt helpers ───────────────────────────────────────────

struct GanttNode* createGanttNode(int pid, int start) {
    struct GanttNode* g = (struct GanttNode*)malloc(sizeof(struct GanttNode));
    g->pid   = pid;
    g->start = start;
    g->end   = start;
    g->next  = NULL;
    return g;
}

void addToGantt(struct GanttNode** ghead, struct GanttNode** gtail,
                int pid, int time) {
    // Extend current block if same process continues
    if (*gtail != NULL && (*gtail)->pid == pid) {
        (*gtail)->end = time + 1;
        return;
    }
    // Otherwise start a new block
    struct GanttNode* g = createGanttNode(pid, time);
    g->end = time + 1;
    if (*ghead == NULL) {
        *ghead = *gtail = g;
    } else {
        (*gtail)->next = g;
        *gtail = g;
    }
}

void printGantt(struct GanttNode* ghead) {
    printf("\nGantt Chart:\n");

    // Top border
    printf(" ");
    struct GanttNode* temp = ghead;
    while (temp != NULL) {
        int width = temp->end - temp->start;
        for (int i = 0; i < width * 2 + 1; i++) printf("-");
        printf(" ");
        temp = temp->next;
    }
    printf("\n|");

    // Process labels
    temp = ghead;
    while (temp != NULL) {
        int width = temp->end - temp->start;
        int spaces = width * 2;
        int label_len = (temp->pid == 0) ? 4 : 2; // "IDLE" or "P#"
        int pad = (spaces - label_len) / 2;
        for (int i = 0; i < pad; i++) printf(" ");
        if (temp->pid == 0)
            printf("IDLE");
        else
            printf("P%d", temp->pid);
        for (int i = 0; i < spaces - pad - label_len; i++) printf(" ");
        printf("|");
        temp = temp->next;
    }
    printf("\n ");

    // Bottom border
    temp = ghead;
    while (temp != NULL) {
        int width = temp->end - temp->start;
        for (int i = 0; i < width * 2 + 1; i++) printf("-");
        printf(" ");
        temp = temp->next;
    }
    printf("\n");

    // Time markers
    temp = ghead;
    printf("%-2d", temp->start);
    while (temp != NULL) {
        int width = temp->end - temp->start;
        for (int i = 0; i < width * 2; i++) printf(" ");
        printf("%-2d", temp->end);
        temp = temp->next;
    }
    printf("\n");
}

void freeGantt(struct GanttNode* ghead) {
    struct GanttNode* temp;
    while (ghead != NULL) {
        temp  = ghead;
        ghead = ghead->next;
        free(temp);
    }
}

// ─── Process helpers ─────────────────────────────────────────

struct Process* createProcess(int pid, int arrival, int burst) {
    struct Process* p = (struct Process*)malloc(sizeof(struct Process));
    p->pid        = pid;
    p->arrival    = arrival;
    p->burst      = burst;
    p->remaining  = burst;
    p->finish     = 0;
    p->waiting    = 0;
    p->turnaround = 0;
    p->next       = NULL;
    return p;
}

void insertProcess(struct Process** head, int pid, int arrival, int burst) {
    struct Process* newNode = createProcess(pid, arrival, burst);
    if (*head == NULL) { *head = newNode; return; }
    struct Process* temp = *head;
    while (temp->next != NULL) temp = temp->next;
    temp->next = newNode;
}

struct Process* findShortest(struct Process* head, int time) {
    struct Process* shortest = NULL;
    struct Process* temp = head;
    while (temp != NULL) {
        if (temp->arrival <= time && temp->remaining > 0) {
            if (shortest == NULL || temp->remaining < shortest->remaining)
                shortest = temp;
        }
        temp = temp->next;
    }
    return shortest;
}

int countProcesses(struct Process* head) {
    int count = 0;
    struct Process* temp = head;
    while (temp != NULL) { count++; temp = temp->next; }
    return count;
}

// ─── SRT scheduler ───────────────────────────────────────────

void srt(struct Process* head,
         struct GanttNode** ghead, struct GanttNode** gtail) {
    int total     = countProcesses(head);
    int completed = 0, time = 0;

    while (completed < total) {
        struct Process* current = findShortest(head, time);

        if (current == NULL) {
            addToGantt(ghead, gtail, 0, time);  // CPU idle
            time++;
            continue;
        }

        addToGantt(ghead, gtail, current->pid, time);
        current->remaining--;
        time++;

        if (current->remaining == 0) {
            completed++;
            current->finish     = time;
            current->turnaround = current->finish - current->arrival;
            current->waiting    = current->turnaround - current->burst;
        }
    }
}

// ─── Print results ────────────────────────────────────────────

void printResults(struct Process* head) {
    printf("\n%-6s %-10s %-10s %-12s %-12s %-10s\n",
           "PID", "Arrival", "Burst", "Finish", "Turnaround", "Waiting");
    printf("--------------------------------------------------------------\n");

    float total_wait = 0, total_tat = 0;
    int count = 0;
    struct Process* temp = head;

    while (temp != NULL) {
        printf("P%-5d %-10d %-10d %-12d %-12d %-10d\n",
               temp->pid, temp->arrival, temp->burst,
               temp->finish, temp->turnaround, temp->waiting);
        total_wait += temp->waiting;
        total_tat  += temp->turnaround;
        count++;
        temp = temp->next;
    }

    printf("\nAverage Waiting Time    : %.2f\n", total_wait / count);
    printf("Average Turnaround Time : %.2f\n", total_tat  / count);
}

void freeList(struct Process* head) {
    struct Process* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

// ─── Main ─────────────────────────────────────────────────────

int main() {
    struct Process*  head  = NULL;
    struct GanttNode* ghead = NULL;
    struct GanttNode* gtail = NULL;
    int n;

    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        int arrival, burst;
        printf("\nProcess P%d:\n", i + 1);
        printf("  Arrival time: ");
        scanf("%d", &arrival);
        printf("  Burst time:   ");
        scanf("%d", &burst);
        insertProcess(&head, i + 1, arrival, burst);
    }

    srt(head, &ghead, &gtail);
    printResults(head);
    printGantt(ghead);

    freeList(head);
    freeGantt(ghead);

    return 0;
}
