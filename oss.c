// Name: Gabrielle Hieken
// Class: 4760 Operating Systems
// Date: 2/15/2023

//To come back to: Reconfigure the help statement, and README, do I use the commented include?

#include <stdio.h>
#include <getopt.h> //Needed for optarg function
#include <stdlib.h> //EXIT_FAILURE
#include <unistd.h> //for pid_t and exec
#include <sys/types.h>
#include <time.h> // to create system time
#include <sys/shm.h> //Shared memory
#include <stdbool.h> //bool values
#include <sys/wait.h> //wait
#include <string.h> //remedy exit warning
#include "oss.h"



void printTable();

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

    //variables for our system clock
    struct timespec start, stop;
    double sec;
    double nano;

    //child process ID
    pid_t childpid = 0;

    //initialize the process table
    int j;
    for(j=0;j<20;j++){
        processTable[j].nano = (double)0;
        processTable[j].sec = (double)0;
        processTable[j].pid = 0;
        processTable[j].occupied = 0;
    }
    printTable();

    //Parse through command line options
	char opt;
    while((opt = getopt(argc, argv, "hn:s:t:")) != -1 )
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
                    printf("If you leave out a '-n', '-s', or '-t' in the command line prompt it will defualt to the value 1 for all except  -t, which will be 2\n\n");
                    printf("Have fun :)\n\n");

                    exit(0);
            break;
        case 'n':
            proc = atoi(optarg);
			//printf("proc,n: %i \n", proc);
            break;
        case 's':
            simul = atoi(optarg);
			//printf("simul,s: %i \n", simul);
            break;
        case 't':
            timelimit = atoi(optarg);
			//printf("timelimit,t: %i \n", timelimit);
            break;
        default:
            printf ("Invalid option %c \n", optopt);
            return (EXIT_FAILURE);
        }
    }

    //Create random second and nanosecond from user input
    srand(time(0));

    int seconds = randomNumberGenerator(timelimit);

    int nanoseconds = randomNumberGenerator(BILLION);

    //Create shared memory, key
    const int sh_key = 3147550;

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

    //Loop to check for terminated children
    while(1) {
        if (childpid != 0){
            //return_pid = waitpid(childpid, &status, WNOHANG); /* WNOHANG def'd in wait.h */
            return_pid = waitpid(-1, &status, WNOHANG);
            if (return_pid == -1) {
                perror("Failed to fork");
                return 1;
            } else if (return_pid == 0) {
                //Child is still running, do nothing
            } else if (return_pid > 0) {
                //Child(ren) have finished, start new chilren if needed, exit program if all chlriren have finished
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

        currentTime = sec + nano/BILLION;

        //Print the table ever half a second
        if(currentTime > (lastPrintTime + 0.5) || lastPrintTime == 0){
            lastPrintTime = currentTime;

            printf("OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);
            printf("Process Table:\n");
            printTable();
            printf("\n\n");
        }

        //Check if all children have been created, check if all children have finished or if time has surpassed 60 seconds
        if(((childrenToLaunch >= proc) && (allChildrenHaveFinished)) || currentTime >= 60){    
            printf("OSS PID: %ld SysClockS: %f SysclockNano: %f\n", (long)getpid(), sec, nano);
            printf("Process Table:\n");
            printTable();
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
        }
    
        //send shared memory key to worker for children to use 
        if (childpid == 0){ 
            char sh_key_string[50];
            char termSec_string[50];
            char termNano_string[50];

            snprintf(sh_key_string, sizeof(sh_key_string), "%i", sh_key);
            snprintf(termSec_string, sizeof(termSec_string), "%i", seconds);
            snprintf(termNano_string, sizeof(termNano_string), "%i", nanoseconds);

            //exec function to send children to worker
            char *args[] = {"worker", sh_key_string, termSec_string, termNano_string, NULL};
            execvp("./worker", args);
            return 1;
        }
    }   
   
    printf("deleting memory");
    shmdt( shm_ptr ); // Detach from the shared memory segment
    shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id 

    return 0;
}

//Print the process table
void printTable(){
    printf("Entry\tOccupied\tPID\t\tStartS\t\tStartN\n");
    int i;
    for(i=0;i<20;i++){
        if(processTable[i].pid == 0 ){
            break;
        }
        printf("%i\t%d\t\t%ld\t\t%f\t%f\n", i, processTable[i].occupied, (long)processTable[i].pid, processTable[i].sec, processTable[i].nano);
    }
}