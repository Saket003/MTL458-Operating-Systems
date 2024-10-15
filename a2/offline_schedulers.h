#pragma once

//Can include any other headers as needed
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>

//Adding for uint64_t
#include <stdint.h>
#include <time.h>

#define MAX_ARGS 2048

//Reusing from a1


typedef struct {

    //This will be given by the tester function this is the process command to be scheduled
    char *command;

    //Temporary parameters for your usage can modify them as you wish
    bool finished;  //If the process is finished safely
    bool error;    //If an error occurs during execution
    uint64_t start_time;
    uint64_t completion_time;
    uint64_t burst_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    bool started; 
    int process_id;

} Process;

// Function prototypes
void FCFS(Process p[], int n);
void RoundRobin(Process p[], int n, int quantum);
void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime);
void splitString(char* cmd, char **token_list);
void clearMemory(char **token_list);
uint64_t currentTimeMs();

void splitString(char* cmd, char **token_list) {
    char *token = strtok(cmd, " ");
    int i = 0;

    while (token != NULL) {  
        token_list[i++] = token;  
        token = strtok(NULL, " ");  
    }
}

void clearMemory(char **token_list){
    for(int i=0; i<MAX_ARGS; i++){
        if(token_list[i]==NULL) break;
        token_list[i] = NULL;
    }
}

typedef struct Node {
    Process data;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* end;
    int size;
} Queue;

Node* createNode(Process data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        printf("Error\n");
        exit(0);
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) {
        printf("Error\n");
        exit(0);
    }
    q->front = q->end = NULL;
    q->size = 0;
    return q;
}

bool isEmpty(Queue* q) {
    return q->size == 0;
}

void push(Queue* q, Process data) {
    Node* newNode = createNode(data);
    if (q->end == NULL)
        q->front = q->end = newNode;
    else {
        q->end->next = newNode;
        q->end = newNode;
    }
    q->size++;
}

Process pop(Queue* q) {
    if (isEmpty(q)) {
        printf("Error\n");
        exit(0);
    }
    
    Node* temp = q->front;
    Process data = temp->data;
    q->front = q->front->next;
    
    if (q->front == NULL) {
        q->end = NULL;
    }

    free(temp);
    q->size--;
    return data;
}

void FCFS(Process p[], int n){
    char* token_list[MAX_ARGS];

    uint64_t global_start_time = currentTimeMs();

    for (int i=0; i<n; i++){
        p[i].start_time = currentTimeMs()-global_start_time;

        char* cmd = malloc(strlen(p[i].command) + 1);;
        strcpy(cmd, p[i].command);
        splitString(cmd, token_list); 

        p[i].process_id = fork(); 

        p[i].started = true;
        p[i].error = false;
        if(p[i].process_id < 0){
            // printf("Error in fork\n");
            // p[i].error = true;
            exit(1);
        }
        else if (p[i].process_id == 0){
            execvp(token_list[0], token_list);
            // printf("Error in execvp\n");
            // p[i].error = true;
            exit(1);
        }
        int status;
        waitpid(p[i].process_id, &status, 0);
        if (WIFEXITED(status)) {  // Check if child exited normally
            int exit_status = WEXITSTATUS(status);  // Get child's exit status
            if (exit_status != 0) 
                p[i].error = true;
        }

        p[i].finished = true;
        p[i].completion_time = currentTimeMs()-global_start_time;

        p[i].burst_time = p[i].completion_time-p[i].start_time;
        p[i].turnaround_time = p[i].completion_time-p[i].start_time;
        p[i].waiting_time = p[i].start_time;
        p[i].response_time = p[i].start_time;

        printf("%s|%ld|%ld\n", p[i].command, p[i].start_time, p[i].completion_time);

        free(cmd);
        clearMemory(token_list);
    }
    char * s = "result_offline_FCFS.csv";
    FILE *file = fopen(s, "w");

    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");

    for(int i=0;i<n;i++){
        char* finished = p[i].error ? "No" : "Yes";
        char* error = p[i].error ? "Yes" : "No";
        fprintf(file, "%s,%s,%s,%ld,%ld,%ld,%ld\n", p[i].command, finished, error, p[i].burst_time, p[i].turnaround_time, p[i].burst_time-p[i].turnaround_time, p[i].response_time);
    }
    fclose(file);
}


void RoundRobin(Process p[], int n, int quantum){
    char* token_list[MAX_ARGS];
    int pending_flag = 0;

    uint64_t global_start_time = currentTimeMs();

    FILE *file = fopen("result_offline_RR.csv", "w");
    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");

    Queue* q = createQueue();

    for(int i=0;i<n;i++){
        p[i].started = false;
        p[i].finished = false;
        push(q, p[i]);
    }

    while(!isEmpty(q)){
        Process current_process = pop(q);

        uint64_t start_switch_time = currentTimeMs()-global_start_time;
        
        if(!current_process.started){
            current_process.burst_time = quantum;
            current_process.start_time = currentTimeMs()-global_start_time;
            current_process.response_time = current_process.start_time;

            char* cmd = malloc(strlen(current_process.command) + 1);;
            strcpy(cmd, current_process.command);
            splitString(cmd, token_list); 

            current_process.process_id = fork();

            current_process.started = true;
            if (current_process.process_id < 0){
                // printf("Error in fork\n");
                exit(1);
            }
            else if (current_process.process_id == 0){
                execvp(token_list[0], token_list);
                // printf("Error in execvp\n");
                exit(1);
            }
            else {
                usleep(quantum*1000);

                int status;
                pid_t result = waitpid(current_process.process_id, &status, WNOHANG);
                if (result == 0) {
                    kill(current_process.process_id, SIGSTOP);
                    current_process.finished = false;
                    push(q,current_process); // Continue doing later
                } 
                else if (result == current_process.process_id) {
                    current_process.finished = true;
                    waitpid(current_process.process_id, &status, 0);
                    if (WIFEXITED(status)) {  // Check if child exited normally
                        int exit_status = WEXITSTATUS(status);  // Get child's exit status
                        if (exit_status != 0) 
                            current_process.error = true;
                    }
                    current_process.completion_time = currentTimeMs()-global_start_time;
                    current_process.turnaround_time = current_process.completion_time-current_process.start_time;
                } 
                else {
                    // printf("Error");
                    current_process.error = true;
                }
            }
            free(cmd);
        }
        else{
            kill(current_process.process_id, SIGCONT);
            usleep(quantum*1000);
            current_process.burst_time += quantum;

            int status;
            pid_t result = waitpid(current_process.process_id, &status, WNOHANG);
            if (result == 0) {
                kill(current_process.process_id, SIGSTOP);
                current_process.finished = false;
                push(q,current_process); // Continue doing later
            } 
            else if (result == current_process.process_id) {
                current_process.finished = true;
                waitpid(current_process.process_id, &status, 0);
                    if (WIFEXITED(status)) {  // Check if child exited normally
                        int exit_status = WEXITSTATUS(status);  // Get child's exit status
                        if (exit_status != 0) 
                            current_process.error = true;
                    }
                current_process.completion_time = currentTimeMs()-global_start_time;
                current_process.turnaround_time = current_process.completion_time-current_process.start_time;
            } 
            else {
                current_process.error = true;
            }
        }

        

        uint64_t end_switch_time = currentTimeMs()-global_start_time;
        if(current_process.finished){
            char* finished = current_process.error ? "No" : "Yes";
            char* error = current_process.error ? "Yes" : "No";
            fprintf(file, "%s,%s,%s,%ld,%ld,%ld,%ld\n", current_process.command, finished, error, current_process.burst_time, current_process.turnaround_time, current_process.turnaround_time-current_process.burst_time, current_process.response_time);
        }
        printf("%s|%ld|%ld\n", current_process.command, start_switch_time, end_switch_time);

        clearMemory(token_list);
    }
    
    fclose(file);
}

void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime){
    // Rule 1 and 2 - Ensured by 3 Queues, Treating as 3 bitmaps
    // Rule 3 - Highest Priority at start
    // Rule 4 - Uses up time allotment, same as using up quant for our implementation
    // Rule 5 - After burst time, move all up

    // Note: https://piazza.com/class/lyxe7peedfl188/post/10_f28
    // Hence treating up using up time allotment as using up quant for our implementation

    // Assumptions - (# of process)*Quantum is considerably lesser than burst time, so we will only run a check for burst time
    // after the entire set of processes have been navigated once, to prevent possible gamification by assigning an S which prevents
    // a process, say pj, from ever running in the first queue by setting burst time as (j-1)*quantum0

    char* token_list[MAX_ARGS];
    int pending_flag = 0;

    uint64_t global_start_time = currentTimeMs();

    Queue* q0 = createQueue();
    Queue* q1 = createQueue();
    Queue* q2 = createQueue();

    uint64_t last_boost_time = global_start_time;

    for(int i=0;i<n;i++){
        p[i].started = false;
        p[i].finished = false;
        push(q0, p[i]);
    }

    FILE *file = fopen("result_offline_MLFQ.csv", "w");
    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");

    while(true){
        if(currentTimeMs()-last_boost_time>=boostTime){
            last_boost_time = currentTimeMs();
            while(!isEmpty(q1)){ // Middle Queue boosted up first
                Process current_process = pop(q1);
                push(q0, current_process);
            }
            while(!isEmpty(q2)){ // Then Lowest Queue
                Process current_process = pop(q2);
                push(q0, current_process);
            }
        }

        if(isEmpty(q0) && isEmpty(q1) && isEmpty(q2)){
            break;
        }

        uint64_t start_switch_time = currentTimeMs()-global_start_time;

        int status = -1;
        if(!isEmpty(q0)) status = 0;
        else if (!isEmpty(q1)) status = 1;
        else if (!isEmpty(q2)) status = 2;

        Process current_process;
        int quantum;
        if(status==0){
            current_process = pop(q0);
            quantum = quantum0;
        }
        else if (status==1){
            current_process = pop(q1);
            quantum = quantum1;
        }
        else{
            current_process = pop(q2);
            quantum = quantum2;
        }

        if(!current_process.started){
            current_process.started = true;
            current_process.start_time = currentTimeMs()-global_start_time;
            current_process.burst_time = quantum;
            current_process.error = false;

            char* cmd = malloc(strlen(current_process.command) + 1);;
            strcpy(cmd, current_process.command);
            splitString(cmd, token_list);

            current_process.process_id = fork();
            if(current_process.process_id<0){
                // printf("Error in fork\n");
                exit(1);
            }
            else if(current_process.process_id==0){
                execvp(token_list[0], token_list);
                // printf("Error in execvp\n");
                exit(1);
            }
            else{
                usleep(quantum*1000);
                current_process.burst_time += quantum;

                int status;
                pid_t result = waitpid(current_process.process_id, &status, WNOHANG);
                if (result == 0) {
                    kill(current_process.process_id, SIGSTOP);
                    current_process.finished = false;
                } 
                else if (result == current_process.process_id) {
                    current_process.finished = true;
                    waitpid(current_process.process_id, &status, 0);
                    if (WIFEXITED(status)) {  // Check if child exited normally
                        int exit_status = WEXITSTATUS(status);  // Get child's exit status
                        if (exit_status != 0) 
                            current_process.error = true;
                    }
                } 
                else {
                    // printf("Error");
                    current_process.error = true;
                }
            }
        }
        else{
            kill(current_process.process_id, SIGCONT);
            usleep(quantum*1000);
            current_process.burst_time += quantum;

            int status;
            pid_t result = waitpid(current_process.process_id, &status, WNOHANG);
            if (result == 0) {
                kill(current_process.process_id, SIGSTOP);
                current_process.finished = false;
            } 
            else if (result == current_process.process_id) {
                current_process.finished = true;
                waitpid(current_process.process_id, &status, 0);
                if (WIFEXITED(status)) {  // Check if child exited normally
                    int exit_status = WEXITSTATUS(status);  // Get child's exit status
                    if (exit_status != 0) 
                        current_process.error = true;
                }
            } 
            else {
                // printf("Error");
                current_process.error = true;
            }
        }
        if(!current_process.finished){
            if(status==0){
                push(q1, current_process);
            }
            else if(status==1){
                push(q2, current_process);
            }
            else{
                push(q2, current_process);
            }
        }
        else{
            char* finished = current_process.error ? "No": "Yes";
            char* error = current_process.error ? "Yes" : "No";
            fprintf(file, "%s,%s,%s,%ld,%ld,%ld,%ld\n", current_process.command, finished, error, current_process.burst_time, currentTimeMs()-global_start_time-current_process.start_time, currentTimeMs()-global_start_time-current_process.start_time-current_process.burst_time,current_process.start_time);
        }
        uint64_t end_switch_time = currentTimeMs()-global_start_time;
        printf("%s|%ld|%ld\n", current_process.command, start_switch_time, end_switch_time);

        clearMemory(token_list);
    }

    fclose(file);
}

uint64_t currentTimeMs(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}