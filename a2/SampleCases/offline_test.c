#include "offline_schedulers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define n_commands 3
#define quantum 1000
#define quantum0 500
#define quantum1 1000
#define quantum2 2000
#define burst 1000000

int main(int argc, char *argv[]){
    char* commands[n_commands] = {"./dummy_p 3", "./galat 2","./dummy_p 1"};

    Process p[n_commands];
    srand(time(NULL));

    for (int i = 0; i < n_commands; i++) {
        p[i].command = commands[i];
    }

    // FCFS(p, n_commands);
    RoundRobin(p, n_commands, quantum);
    // MultiLevelFeedbackQueue(p, n_commands, quantum0, quantum1, quantum2, burst);
    // for(int i=0;i<n_commands;i++)
    //     printf("Process %s started at %ld and completed at %ld and finished is %d\n", p[i].command, p[i].start_time, p[i].completion_time, p[i].finished);
    
    return 0;
}