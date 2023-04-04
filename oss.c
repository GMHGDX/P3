// Name: Gabrielle Hieken
// Class: 4760 Operating Systems
// Date: 2/15/2023

//To come back to: Reconfigure the help statement, and README, do I use the commented include?

#include <stdio.h>
#include <getopt.h> //Needed for optarg function
#include <stdlib.h> //EXIT_FAILURE
#include <unistd.h> //for pid_t and exec
#include <sys/types.h>
#include <time.h> //to create system time
#include <sys/shm.h> //Shared memory
#include <stdbool.h> //bool values
#include <sys/wait.h> //wait
#include <string.h> //remedy exit warning
#include <sys/msg.h> //message queues
#include "oss.h"

void printTable();

//My message queue initialization
struct msgqueue {
    long mtype;
    char mtext[200];
}msq;

//Create random second and nanosecond in bound of user input
int randomNumberGenerator(int limit)
{
    int sec;

    sec = (rand() % (limit)) + 1;

    return sec;
}

int main(int argc, char *argv[]){
	//number of total children to launch (n)
	int proc = 1;

	//how many children run at the same time (s)
	int simul = 1;

	//bound of time that a child process will be launched for (t)
	int timelimit= 2;

    //logfile declaration
    char* logFile = "logfile";

    //variables for our system clock
    struct timespec start, stop;
    double sec;
    double nano;

    //for the file 
    FILE *fileLogging;

    //child process ID
    pid_t childpid = 0;

    //Parse through command line options
	char opt;
    while((opt = getopt(argc, argv, "hn:s:t:f:")) != -1 )
    {
        switch (opt)
        {
        //help message
        case 'h':
			printf("To run this project: \n\n");
            printf("run the command: ./oss -n num -s num -t num\n\n");
                    printf("\tnum = can be replaced with any number below 18 | -n, -s, and -t can be typed in any order\n");
                    printf("\t-n = number of total children to launch\n");
                    printf("\t-s = how many children run at the same time\n");
                    printf("\t-t = bound of time that a child process will be launched for (will be picked randomly for sec and nano within given bound)\n\n");   
                    printf("\t-f = name of logfile you wish to write the process table in oss.c to\n\n"); 
                    printf("If you leave out a '-n', '-s', '-t', or '-f' in the command line prompt it will default to the value 1 for -n and -s, the value of 2 for -t, and 'logfile' for -f\n\n");
                    printf("Have fun :)\n\n");

                    exit(0);
            break;
        case 'n':
            proc = atoi(optarg);
            break;
        case 's':
            simul = atoi(optarg);
            break;
        case 't':
            timelimit = atoi(optarg);
            break;
        case 'f':
            logFile = optarg; 
            break;
        default:
            printf ("Invalid option %c \n", optopt);
            return (EXIT_FAILURE);
        }
    }

    //Open the log file before input begins 
    fileLogging = fopen(logFile, "w+");

    //initialize the process table
    int j;
    for(j=0;j<20;j++){
        processTable[j].nano = (double)0;
        processTable[j].sec = (double)0;
        processTable[j].pid = 0;
        processTable[j].occupied = 0;
    }

    //Create random second and nanosecond from user input
    srand(time(0));

    int seconds = randomNumberGenerator(timelimit);

    int nanoseconds = randomNumberGenerator(BILLION);

    //Create shared memory, key
    const int sh_key = 3147550;

    //Create key using ftok() for more uniqueness
    key_t msqkey;
    if((msqkey = ftok("oss.h", 'a')) == (key_t) -1){
        perror("IPC error: ftok");
        exit(1);
    }

    //open an existing message queue or create a new one
    int msqid;
    if ((msqid = msgget(msqkey, PERMS | IPC_CREAT)) == -1) {
      perror("Failed to create new private message queue");
      exit(1);
   }

    //create shared memory
    int shm_id = shmget(sh_key, sizeof(struct PCB), IPC_CREAT | 0666);
    if(shm_id <= 0) {
        fprintf(stderr,"ERROR: Failed to get shared memory, shared memory id = %i\n", shm_id);
        exit(1);
    }

    //attatch memory we allocated to our process and point pointer to it 
    struct PCB *shm_ptr = (struct PCB*) (shmat(shm_id, NULL, 0));
    if (shm_ptr <= 0) {
        fprintf(stderr,"Shared memory attach failed\n");
        exit(1);
    }

    //start the simulated system clock
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
      return EXIT_FAILURE;
    }


    //intialize values for use in while loop
    int childrenToLaunch = 0;
    int i = 0;
    int status;
    pid_t return_pid;
    bool allChildrenHaveFinished = false;
    int currentChildren=0;

    double currentTime, lastPrintTime=0;
    
    //Loop to handle our children processes and print the process table
    while(1) {
        if (childpid != 0){
            return_pid = waitpid(-1, &status, WNOHANG);
            if (return_pid == -1) {
                perror("Failed to fork");
                return 1;
            } else if (return_pid == 0) {
                //Child is still running, do nothing
            } else if (return_pid > 0) {
                //Child(ren) have finished, start new chilren if needed, exit program if all children have finished
                currentChildren--;
                for(i = 0; i < 20; i++){
                    if(processTable[i].pid == return_pid){
                        processTable[i].occupied = 0;
                        break;
                    }
                }
                for(i=0;i<20;i++){
                    allChildrenHaveFinished = true;
                    if(processTable[i].occupied == 1){
                        allChildrenHaveFinished = false;
                        break;
                    }
                }
            }
        }

        //stop simulated system clock
        if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
            perror( "clock gettime" );
            return EXIT_FAILURE;
        }
        
        sec = (stop.tv_sec - start.tv_sec); 
        nano = (double)( stop.tv_nsec - start.tv_nsec);

        //if start time nanosecond is greater than stop, carry the one to get positive nanosecond
        if(start.tv_nsec > stop.tv_nsec){
            sec = (stop.tv_sec - start.tv_sec) - 1;
            nano = (double)( stop.tv_nsec - start.tv_nsec) + ((double)(1)*BILLION);
        }

        //combine seconds and nanoseconds as one decimal
        currentTime = sec + nano/BILLION;

        //Print the table every half a second and write the same information to log file
        if(currentTime > (lastPrintTime + 0.5) || lastPrintTime == 0){
            lastPrintTime = currentTime;

            printf("OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);
            fprintf(fileLogging, "OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);
            

            printf("Process Table:\n");
            fprintf(fileLogging, "Process Table:\n");

            printTable(fileLogging);

            printf("\n\n");
            fprintf(fileLogging, "\n\n");
        }

        //Check if all children have been created and all children have finished, or check if time has surpassed 60 seconds. Print and log it if true
        if(((childrenToLaunch >= proc) && (allChildrenHaveFinished)) || currentTime >= 60){  
            printf("OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);  
            fprintf(fileLogging, "OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);

            printf("Process Table:\n");
            fprintf(fileLogging, "Process Table:\n");

            printTable(fileLogging);
            break; //program can end, all child processes are done
        }

        //Write the seconds and nanoseconds to memory for children to read
        struct PCB writeToMem;
        writeToMem.sec = sec;
        writeToMem.nano = nano;

        *shm_ptr = writeToMem;
        writeToMem = *shm_ptr;
    
        //fork child processes
        if (childrenToLaunch < proc && currentChildren < simul){
            childpid = fork();
            if (childpid == -1) {
                perror("Failed to fork");
                return 1;
            }
            //update all values in the table
            allChildrenHaveFinished = false;
            processTable[childrenToLaunch].nano = nano;
            processTable[childrenToLaunch].occupied = 1;
            processTable[childrenToLaunch].pid = childpid;
            processTable[childrenToLaunch].sec = sec;

            childrenToLaunch++;
            currentChildren++;

            if(childpid != 0 ){ 
                //initialize mtype to 1
                msq.mtype = 1;
                char sec_string[50];
                char nano_string[50];

                //convert integer to char string
                snprintf(sec_string, sizeof(sec_string), "%i", seconds);
                snprintf(nano_string, sizeof(nano_string), "%i", nanoseconds);

                //add seconds and nanoseconds together with a space in between to send as one message
                char *together;
                together = malloc(strlen(sec_string) + strlen(nano_string) + 1 + 1);
                strcpy(together, sec_string);
                strcat(together, " ");
                strcat(together, nano_string);

                //copy our new string into mtext
                strcpy(msq.mtext, together);

                //send our string to message queue
                msgsnd(msqid, &msq, sizeof(msq), 0);
            }
        }
    
        //send shared memory key to worker for children to use 
        if (childpid == 0){ 
            char sh_key_string[50];
            snprintf(sh_key_string, sizeof(sh_key_string), "%i", sh_key);

            char *args[] = {"worker", sh_key_string, NULL};
            //exec function to send children to worker alonk with our shared memeory key
            execvp("./worker", args);

            return 1;
        }
    }   
   
    ///printf("deleting memory");
    shmdt( shm_ptr ); // Detach from the shared memory segment
    shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id 

    //Removes the message queue immediately
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
            return EXIT_FAILURE;
    }

    //close the log file
    fclose(fileLogging);
    return 0;
}

//Print the process table
void printTable(FILE* fileLogging){
    printf("Entry\tOccupied\tPID\t\tStartS\t\tStartN\n");
    fprintf(fileLogging, "Entry\tOccupied\tPID\t\tStartS\t\tStartN\n");
    
    int i;
    for(i=0;i<20;i++){
        if(processTable[i].pid == 0 ){
            break;
        }
        
        printf("%i\t%d\t\t%ld\t\t%f\t%f\n", i, processTable[i].occupied, (long)processTable[i].pid, processTable[i].sec, processTable[i].nano);
        fprintf(fileLogging, "%i\t%d\t\t%ld\t\t%f\t%f\n", i, processTable[i].occupied, (long)processTable[i].pid, processTable[i].sec, processTable[i].nano);     
    }
}