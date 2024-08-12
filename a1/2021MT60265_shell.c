#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_CMD_SIZE 2048
#define MAX_ARGS 2048
#define MAX_HISTORY 1000

void addHistory(char *cmd, char **history_list, int offset){
    for(int i=0; i<MAX_HISTORY; i++){
        if(history_list[i]==NULL){
            history_list[i] = (char *)malloc(MAX_CMD_SIZE*sizeof(char));
            strcpy(history_list[i],cmd);
            return;
        }
    }

    // Rotating History
    free(history_list[offset]);
    history_list[offset] = (char *)malloc(MAX_CMD_SIZE*sizeof(char));
    strcpy(history_list[offset],cmd);
}

void printHistory(char **history_list, int offset){
    printf("History (Recent command last):\n");
    for(int i=0; i<MAX_HISTORY; i++){
        if(history_list[(i+offset)%MAX_HISTORY]!=NULL){
            printf("%s\n", history_list[(i+offset)%MAX_HISTORY]);
        }
    }
}

int pipeCount (char* cmd){
    int count = 0;
    for(int i=0; i<strlen(cmd); i++){
        if(cmd[i]=='|') count++;
    }
    return count;
}

void splitString(char* cmd, char **token_list){
    char * token = strtok(cmd, " ");
    int i = 0;
    while( token != NULL ) {
        token_list[i++] = token;
        token = strtok(NULL, " ");
    }
}

void executeCommand(char **token_list, char * old_pwd){
    pid_t pid = fork();
    if(pid<0){
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    else if(pid==0){
        if(strcmp(token_list[0],"cd")==0){
            char curr_pwd[MAX_CMD_SIZE];
            getcwd(curr_pwd, sizeof(curr_pwd));
            if(token_list[1]==NULL || strcmp(token_list[1],"~")==0){
                // chdir(getenv("HOME"));
                char *home = getenv("HOME");
                if(home==NULL || chdir(home)!=0){
                    fprintf(stderr, "Invalid Command\n");
                    exit(1);
                }
            }
            else if (strcmp(token_list[1],"-")==0){
                if(old_pwd==NULL){
                    fprintf(stderr, "Invalid Command\n");
                    exit(1);
                }
                // printf("%s\n", old_pwd);
                chdir(old_pwd);
            }
            else if(chdir(token_list[1])!=0){
                fprintf(stderr, "Invalid Command\n"); // If invalid directory
                exit(1);
            }
            strcpy(old_pwd,curr_pwd);
        }
        else {
            int status = execvp(token_list[0],token_list);
            if(status<0){
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
        }
    }
    else{
        wait(NULL);
    }
}

void clearMemory(char **token_list, char **token_list_spare){
    for(int i=0; i<MAX_ARGS; i++){
        if(token_list[i]==NULL) break;
        token_list[i] = NULL;
    }
    for(int i=0; i<MAX_ARGS; i++){
        if(token_list_spare[i]==NULL) break;
        token_list_spare[i] = NULL;
    }
}

void splitStringPiped(char* cmd, char **token_list, char **token_list_spare){
    char * token1 = strtok(cmd, "|");
    char * token2 = strtok(NULL, "|");
    splitString(token1, token_list);
    splitString(token2, token_list_spare);
}

int executeCommandPiped(char **token_list, char **token_list_spare, char ** history_list, int offset){
    // Initial Approach
    // Perform first pipe, Add output to token list, Execute second pipe as normal command

    // Instead, using pipe() in C, store output in a file descriptor and use it as input for second pipe
    int pipefd[2];
    int input_fd;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    if(strcmp(token_list[0],"exit")==0){
        return 0;
    }

    pid_t pid1 = fork();
    if(pid1<0){
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    else if(pid1==0){
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        // close(pipefd[1]);

        if(strcmp(token_list[0],"history")==0){
            printHistory(history_list,offset);
            exit(1);
        }
        else{
            int status = execvp(token_list[0],token_list);
            if(status<0){
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
        }
    }
    else{
        wait(NULL);
        close(pipefd[1]);
        input_fd = pipefd[0];
    }

    if(strcmp(token_list_spare[0],"exit")==0){
        return 0;
    }
    if(strcmp(token_list_spare[0],"history")==0){
        printHistory(history_list,offset);
        return 1;
    }

    pid_t pid2 = fork();
    if(pid2<0){
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    else if(pid2==0){
        dup2(input_fd, 0);
        // close(input_fd);
        int status = execvp(token_list_spare[0],token_list_spare);
        if(status<0){
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
    }
    else{
        wait(NULL);
        close(input_fd);
    }

    return 1;
}

int justBlanks(char *cmd){
    if(strlen(cmd)==0) return 1;
    for(int i=0; i<strlen(cmd); i++){
        if(cmd[i]!=' ' && cmd[i]!='\t') return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    char cmd[MAX_CMD_SIZE];
    char * token_list[MAX_ARGS];
    char * token_list_spare[MAX_ARGS];
    char * history_list[MAX_HISTORY];
    char old_pwd[MAX_CMD_SIZE];
    int pipe_count;
    int offset = 0;
    int exit_flag = 1;

    while(exit_flag){
        // Ready terminal and take input
        printf("MTL458 > ");
        fgets(cmd, MAX_CMD_SIZE, stdin);
        if(cmd[0]=='\n') continue; // If empty command
        cmd[strcspn(cmd, "\n")] = 0;

        // If blanks
        if (justBlanks(cmd)) continue;

        // Store to History
        addHistory(cmd,history_list,offset);
        offset = (offset+1)%MAX_HISTORY;

        // Split the command into tokens and execute command
        pipe_count = pipeCount(cmd);       
        if(pipe_count==0){
            if(strcmp(cmd,"exit")==0){
                break;
            }
            else if(strcmp(cmd,"history")==0){
                printHistory(history_list,offset);
                continue;
            }
            else{
                splitString(cmd, token_list);
                executeCommand(token_list, old_pwd);
            }
        }
        else if(pipe_count==1){
            splitStringPiped(cmd, token_list, token_list_spare);
            exit_flag =  executeCommandPiped(token_list, token_list_spare, history_list, offset);
        }
        else{
            fprintf(stderr, "Invalid Command\n");
        }

        // Resetting token_list
        clearMemory(token_list,token_list_spare);
    }
    return 0;
}