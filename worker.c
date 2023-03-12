#include <stdio.h>
#include <string.h> //remedy exit warning
#include <stdlib.h> //EXIT_FAILURE
#include <sys/shm.h> //Shared memory
#include <sys/msg.h> //message queues
#include "oss.h"

//for message queue
struct msgqueue {
    long mtype;
    char mtext[200];
}msq;

int main(int argc, char *argv[]){
    int termTimeS;
    int termTimeNano;
    int sysClockS;
    int sysClockNano;
    int checkSec = 0;

    // //grab sh_key from oss for shared memory
    int sh_key = atoi(argv[1]);

    //Grab same key oss.c grabbed for message queue
    key_t msgkey;
    if((msgkey = ftok("oss.h", 'a')) == (key_t) -1){
        perror("IPC error: ftok");
        exit(1);
    }

    //connect to the queue
    int msqid;
    if ((msqid = msgget(msgkey, PERMS)) == -1) {
      perror("msgget");
      exit(1);
   }

    //recieve the message
    msgrcv(msqid, &msq, sizeof(msq), 1, 0);

    // initialization for string loop
    int seperate = 0;
    int sec;
    int nanosec;

    //seperate the message by white space and assign it to seconds and nanoseconds respectively
    char * text = strtok(msq.mtext, " ");
        while( text != NULL ) {
            seperate++;
            if(seperate == 1){
                sec = atoi(text); //assign second as an integer
                text = strtok(NULL, " ");
            }
            if(seperate == 2){
                nanosec = atoi(text); //assign nanosecond as an integer
                text = strtok(NULL, " ");
                break;
            }
    }

    //get shared memory
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
    termTimeNano = readFromMem.nano + nanosec;

    //Read time from memory
    sysClockS = readFromMem.sec;
    sysClockNano = readFromMem.nano;

    //for printing every second in the loop for each child that starts at different times
    checkSec = sysClockS;

    printf("WORKER PID: %ld PPID: %ld Received message from oss: SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n--Received message\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

    //loop child until termination time is passed 
    while(1){
        readFromMem = *shm_ptr;
        sysClockS = readFromMem.sec;
        sysClockNano = readFromMem.nano;

        //to terminate at the right time
        if(sysClockS > termTimeS || (sysClockS == termTimeS && sysClockNano > termTimeNano)){
            break;
        }
        //printins out every second
        if(checkSec == sysClockS){
            printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --%i seconds has passed\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano, checkSec);
            checkSec++;
        }
    }
    //print when child has finished loop and terminating
    printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --Terminating\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

    return 0;
}