#pragma once

//Can include any other headers as needed
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

//Adding for uint64_t
#include <stdint.h>
#include <time.h>
#include <errno.h>

#define MAX_ARGS 2048
#define MAX_CMD_SIZE 2048
#define MAX_CMDS 100

typedef struct {
    char *command;
    bool finished;
    bool error;    
    uint64_t start_time;
    uint64_t completion_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    uint64_t burst_time;
    uint64_t entry_time;
    bool started; 
    int process_id;

} Process;

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

typedef struct {
    char *command;
    int count;
    int burst_time;
    Queue* q;
} Command;

// Function prototypes
void ShortestJobFirst();
// void ShortestRemainingTimeFirst();
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime);
void splitString(char* cmd, char **token_list);
void clearMemory(char **token_list);
int inList(char* cmd, Command list[MAX_CMDS], int num_commands);
int minTimeCommand(Command list[MAX_CMDS], int num_commands);

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

void executeCommand(char **token_list, int *error){
    pid_t pid = fork();
    if(pid<0){
        // fprintf(stderr, "Error\n");
        exit(1);
    }
    else if(pid==0){
        int status = execvp(token_list[0],token_list);
        if(status<0){
            // fprintf(stderr, "Error\n");
            exit(1);
        }
    }
    else{
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {  // Check if child exited normally
            int exit_status = WEXITSTATUS(status);  // Get child's exit status
            if (exit_status != 0) 
                *error = 1;
        }
    }
}

int inList(char* cmd, Command list[MAX_CMDS], int num_commands){
    for(int i=0; i<num_commands; i++){
        if(strcmp(list[i].command,cmd)==0) return i;
    }
    return -1;
}

int minTimeCommand(Command list[MAX_CMDS], int num_commands){
    int mn = -1;;
    for(int i=0; i<num_commands; i++){
        if(list[i].count>0){
            mn = i;
            break;
        }
    }
    if(mn==-1) return -1;
    for(int i=0; i<num_commands; i++){
        if(list[i].count>0 && list[i].burst_time<list[mn].burst_time){
            mn = i;
        }
    }
    if(list[mn].count==0) return -1;
    return mn;
}

void set_nonblocking_mode(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        exit(EXIT_FAILURE);
    }
}

uint64_t currentTimeMs(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}





void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime){
    char cmd[MAX_CMD_SIZE];
    char * token_list[MAX_ARGS];

    Queue* q0 = createQueue();
    Queue* q1 = createQueue();
    Queue* q2 = createQueue();

    Command list[MAX_CMDS] = {0};

    int i=0;

    set_nonblocking_mode(STDIN_FILENO);

    uint64_t global_start_time = currentTimeMs();
    uint64_t last_boost_time = global_start_time;

    FILE *file = fopen("result_online_MLFQ.csv", "w");
    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(file);

    while(true){
        // Note boost time updates are with respect to global time
        // Hence even when no process is running, boosting is occuring
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
        
        // Note it might not take all inputs even if preloaded in one go
        while(i<MAX_CMDS){
            // Note: To avoid multiple lookups, we assign the process to the queue as soon as it is received
            // For example, if two same processes arrive at the same instant, both will make use of the same memory, rather than the "second" one using the memory of the first one
            if(fgets(cmd, MAX_CMD_SIZE, stdin)==NULL) break;
            if(cmd[0]=='\n') continue;
            cmd[strcspn(cmd,"\n")] = '\0';
            int loc = inList(cmd,list,i);
            if(loc==-1){
                Command new_command;
                new_command.command = strdup(cmd);
                new_command.burst_time = -1000; // ms
                list[i] = new_command;
                i++;
            }
            Process new_process;
            new_process.command = strdup(cmd);
            new_process.started = false;
            new_process.finished = false;
            new_process.entry_time = currentTimeMs()-global_start_time;
            if(loc == -1 || list[loc].burst_time==-1000){
                push(q1, new_process); //New Processes defaulted to middle queue
            }
            else if(list[loc].burst_time<=quantum0){
                push(q0, new_process);
            }
            else if(list[loc].burst_time<=quantum1){
                push(q1, new_process);
            }
            else{
                push(q2, new_process);
            }
        }

        if(isEmpty(q0) && isEmpty(q1) && isEmpty(q2)){
            continue;
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
            current_process.burst_time = 0;

            char* cmd = malloc(strlen(current_process.command) + 1);;
            strcpy(cmd, current_process.command);
            splitString(cmd, token_list);

            current_process.process_id = fork();
            if(current_process.process_id<0){
                // printf("Error in fork\n");
                current_process.error = true;
                exit(1);
            }
            else if(current_process.process_id==0){
                execvp(token_list[0], token_list);
                // printf("Error in execvp\n");
                current_process.error = true;
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
            int loc = inList(current_process.command,list,i);
            if(current_process.error){
                // If error, do not include in historical data
                // Do Nothing
            }
            else if(list[loc].burst_time==-1000)
                list[loc].burst_time = current_process.burst_time;
            else
                list[loc].burst_time = (list[loc].burst_time + current_process.burst_time)/2;
            
            FILE *file = fopen("result_online_MLFQ.csv", "a");
            char* finished = current_process.error ? "No" : "Yes" ;
            char* error = current_process.error ? "Yes" : "No";
            fprintf(file, "%s,%s,%s,%ld,%ld,%ld,%ld\n", current_process.command, finished, error, current_process.burst_time, currentTimeMs()-global_start_time-current_process.start_time, currentTimeMs()-current_process.start_time-global_start_time-current_process.burst_time,current_process.start_time-current_process.entry_time);
            fclose(file);
        }
        uint64_t end_switch_time = currentTimeMs()-global_start_time;
        printf("%s|%ld|%ld\n", current_process.command, start_switch_time, end_switch_time);

        clearMemory(token_list);
    }
}

void ShortestJobFirst(){
    char cmd[MAX_CMD_SIZE];
    char * token_list[MAX_ARGS];
    Command list[MAX_CMDS] = {0};
    int i=0;

    set_nonblocking_mode(STDIN_FILENO);

    uint64_t global_start_time = currentTimeMs();

    FILE *file = fopen("result_online_SJF.csv", "w");
    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(file);

    while(true){
        while(i<MAX_CMDS){
            if(fgets(cmd, MAX_CMD_SIZE, stdin)==NULL) break;
            if(cmd[0]=='\n') continue;
            cmd[strcspn(cmd,"\n")] = '\0';
            int loc = inList(cmd,list,i);
            if(loc==-1){
                Command new_command;
                new_command.command = strdup(cmd);
                new_command.count = 1;
                new_command.burst_time = 1000; //ms
                new_command.q = createQueue();
                push(new_command.q, (Process){.entry_time = currentTimeMs()-global_start_time});
                list[i] = new_command;
                i++;
            }
            else{
                list[loc].count++;
                push(list[loc].q, (Process){.entry_time = currentTimeMs()-global_start_time});
            }
        }
        int index = minTimeCommand(list,i);
        if(index==-1) continue;
        list[index].count--;
        uint64_t entry_time = pop(list[index].q).entry_time;
        char *temp = strdup(list[index].command);
        splitString(temp, token_list);
        uint64_t start_switch_time = currentTimeMs()-global_start_time;
        int err = 0;
        executeCommand(token_list, &err);
        uint64_t end_switch_time = currentTimeMs()-global_start_time;

        // If error, do not consider that in history of burst times
        if(!err) list[index].burst_time = (list[index].burst_time + (end_switch_time-start_switch_time))/2;

        printf("%s|%ld|%ld\n", list[index].command, start_switch_time, end_switch_time);
        file = fopen("result_online_SJF.csv", "a");
        char* finished = err ? "No" : "Yes" ;
        char* error = err ? "Yes" : "No";
        fprintf(file, "%s,%s,%s,%ld,%ld,%ld,%ld\n", list[index].command, finished, error, end_switch_time-start_switch_time, end_switch_time-start_switch_time, (long int)0, start_switch_time-entry_time);
        fclose(file);
        clearMemory(token_list);
    }
    
}