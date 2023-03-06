#include <stdio.h>
#include <string.h> //remedy exit warning
#include <stdlib.h> //EXIT_FAILURE
#include <sys/shm.h> //Shared memory
#include <sys/msg.h> //message queues
#include "oss.h"

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

    // //grab sh_key, random second, and random nanosecond from oss
    int sh_key = atoi(argv[1]);
    // int sec = atoi(argv[2]);
    // int nano = atoi(argv[3]);

    //Grab same key oss.c grabbed for message queue
    key_t msgkey;
    if((msgkey = ftok("msgkey.txt", 'a')) == (key_t) -1){
        perror("IPC error: ftok");
        exit(1);
    }

    //connect to the queue
    int msqid;
    if ((msqid = msgget(msgkey, PERMS)) == -1) {
      perror("msgget");
      exit(1);
   }

    //int msgrcv(int msgid, const void *msgp, size_t msgsz, long msgtype, int msgflg)
    //msgid = recognized message queue
    //msgp = pointer to the message recieved the caller
    //msgsz = size of message recieved (ending w/ null character)
    //msgtype =
        //0 − Reads the first received message in the queue
        //+ve − Reads the first message in the queue of type msgtype (if msgtype is 10, then reads only the first message of type 10 even though other types may be in the queue at the beginning)
        //–ve − Reads the first message of lowest type less than or equal to the absolute value of message type (say, if msgtype is -5, then it reads first message of type less than 5 i.e., message type from 1 to 5)
    //msgflg = IPC_NOWAIT (returns immediately when no message is found in queue or MSG_NOERROR (truncates message text, if more than msgsz bytes)
    //recieve the message
    msgrcv(msqid, &msq, sizeof(msq), 1, 0);
    printf("Data Received is : %s \n", msq.mtext);

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
    termTimeS = readFromMem.sec + 1;
    termTimeNano = readFromMem.nano;
    double termTogether = termTimeS + termTimeNano/BILLION;


    sysClockS = readFromMem.sec;
    sysClockNano = readFromMem.nano;

    checkSec = sysClockS;

    double currentTime;

    printf("WORKER PID: %ld PPID: %ld Received message from oss: SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n--Received message\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

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