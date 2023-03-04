#include <stdio.h>
#include <sys/shm.h> //Shared memory
#include "oss.h"

int main(int argc, char *argv[]){
    int termTimeS;
    int termTimeNano;
    int sysClockS;
    int sysClockNano;
    int checkSec = 0;

    //grab sh_key, random second, and random nanosecond from oss
    int sh_key = atoi(argv[1]);
    int sec = atoi(argv[2]);
    int nano = atoi(argv[3]);

    int shm_id = shmget(sh_key, sizeof(struct PCB), 0666);
    if(shm_id <= 0) {
        fprintf(stderr,"CHILD ERROR: Failed to get shared memory, shared memory id = %i\n", shm_id);
        exit(1);
    }

    //attatch memory we allocated to our process and point pointer to it 
    struct PCB *shm_ptr = (struct PCB*) (shmat(shm_id, 0, 0));
    if (shm_ptr <= 0) {
        fprintf(stderr,"Child Shared memory attach failed\n");
        exit(1);
    }

    //read time from memory
    struct PCB readFromMem;
    readFromMem = *shm_ptr;

    //Figure out when to terminate
    termTimeS = readFromMem.sec + sec;
    termTimeNano = readFromMem.nano + nano;
    double termTogether = termTimeS + termTimeNano/BILLION;


    sysClockS = readFromMem.sec;
    sysClockNano = readFromMem.nano;

    checkSec = sysClockS;

    double currentTime;

    printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n--Just Starting\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

    //loop child until termination time is passed 
    while(1){
        readFromMem = *shm_ptr;
        sysClockS = readFromMem.sec;
        sysClockNano = readFromMem.nano;

        currentTime = sysClockS + sysClockNano/BILLION;

        if(currentTime > termTogether){
            break;
        }
        if(checkSec == sysClockS){
            printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --%i seconds has passed\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano, checkSec);
            checkSec++;
        }
    }
    //print when child has finished loop and terminating
    printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --Terminating\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

    return 0;
}