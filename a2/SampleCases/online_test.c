#include "online_schedulers.h"
#include <string.h>

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


int main(int argc, char *argv[]){
    // ShortestJobFirst();
    MultiLevelFeedbackQueue(2,5,10,20000000);
    return 0;
}