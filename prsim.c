#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

typedef struct Job {
    char name[11];
    int runTime;
    int probBlocking;

    // for statkeeping
    int finishedTime;
    int timesGivenCPU;
    int timesBlockedForIO;
    int timeSpentForIO;
    int timeSpentForCPU;

    // for IO
    int timeLeftIO;
    int timeUntilBlock;

    // for RR and FCFS
    int timeLeftRunning;
    int timeLeftQuantum;
} Job;

#define MAX_JOBS 256

typedef struct JobQueue {
    Job *queue[MAX_JOBS];
    int head;
    int tail;
    int busyTime;
    int idleTime;
} JobQueue;

void push_queue(JobQueue *queue, Job *job) {
    // tail has looped around to head. Too many jobs!
    if ((queue->tail + 1) % MAX_JOBS == queue->head) {
        fprintf(stderr, "Max number of jobs exceeded!");
        exit(1);
    }

    queue->queue[queue->tail] = job;
    queue->tail = (queue->tail + 1) % MAX_JOBS;
}

Job* pop_queue(JobQueue *queue) {
    // queue is empty
    if (queue->head == queue->tail)
        return NULL;

    int oldHead = queue->head;
    queue->head = (queue->head + 1) % MAX_JOBS;
    return queue->queue[oldHead];
}

Job* peek_queue(JobQueue *queue) {
    return queue->queue[queue->head];
}

void push_all_to_ready_FCFS(JobQueue *queue, Job *jobs, int numJobs) {
    int i;
    for (i = 0; i < numJobs; i++) {
        jobs[i].timeLeftRunning = jobs[i].runTime;
        jobs[i].timeUntilBlock = -1;
        jobs[i].timesGivenCPU = 1;
        jobs[i].timeSpentForCPU = jobs[i].runTime;
        push_queue(queue, &jobs[i]);
    }
}

int min(int a, int b) {
    if (a < b)
        return a;
    return b;
}

#define QUANTUM_LEN 5

void push_all_to_ready_RR(JobQueue *queue, Job *jobs, int numJobs) {
    int i;
    for (i = 0; i < numJobs; i++) {
        jobs[i].timeLeftRunning = jobs[i].runTime;
        jobs[i].timeLeftQuantum = min(5, jobs[i].timeLeftRunning);
        jobs[i].timeUntilBlock = -1;
        jobs[i].timesGivenCPU = 0;
        jobs[i].timeSpentForCPU = jobs[i].runTime;
        push_queue(queue, &jobs[i]);
    }
}

int is_empty(JobQueue *queue) {
    return queue->tail == queue->head;
}

float get_random_to_1() {
    int r = random();
    float a = ((float)r / RAND_MAX);
    
    return a;
}

// end is noninclusive
int get_random_in_range(int start, int end) {
    // end -= 1;       // because we passed in everything end noninclusive
    int r = random();
    int a = (r % (end - start)) + start;
    
    return a;
    // return (random() % (end - start)) + start;
}

// 100 is noninclusive
int get_random_to_100() {
    return get_random_to_1() * 100;
}

void print_queue(JobQueue *queue) {
    int i;
    for (i = queue->head; i != queue->tail; i = (i + 1) % MAX_JOBS) {
        Job *job = queue->queue[i];
        printf("%s (%d, %d, %d, %d)\t", job->name, job->timeLeftRunning, job->timeLeftIO, job->timeUntilBlock, job->timeLeftQuantum);
    }
    printf("\n");
}


// procsim <-r|-f> <filename>
int main(int argc, char *argv[]) {
    
    // seed random
    srandom(12345);

    if (argc != 3 || ((strcmp(argv[1], "-r") != 0) && (strcmp(argv[1], "-f") != 0))){
        fprintf(stderr, "Usage: %s [-r | -f] file\n", argv[0]);
        return 1;
    }

    FILE* ptr;
    ptr = fopen(argv[2], "rb");         // rb = read from file and not write to it

    if(ptr == NULL){
        printf("Not a valid file");
        return 0;
    }

    // initialize array of jobs and set to nulls

    JobQueue ready;
    JobQueue IO;
    Job allJobs[256];

    // editor	    	5	    	0.87
    // compiler	    	40	    	0.53
    // adventure	    	30	    	0.72

    char line[256];
    int numJobs = 0;
    int lineNum = 0;

    char *tokens[3];
    
    //While loop to read from the file and put rows into arrays
    while (fgets(line, sizeof(line), ptr)) {  
        lineNum++;  
        line[strcspn(line, "\n")] = '\0';
        char *token = strtok(line, "\t");
        int offset = 0;

        while (token) {
            if (offset < 3)
                tokens[offset++] = token;
            
            token = strtok(NULL, "\t");
        }

        if (offset != 3) {
            fprintf(stderr, "Malformed line %s(%d)\n", argv[2], lineNum);
            return 1;
        }

        /////////// process tokens
        
        // name
        token = tokens[0];
        
        //check if name is valid
        if(strlen(token)>=10){
            fprintf(stderr, "name is too long %s(%d)\n", argv[2], lineNum);
            return 1;
        }
        strcpy(allJobs[numJobs].name, token);
        
        // run time
        token = tokens[1];
        
        //check if runtime is an integer
        for(int i=0; i<strlen(token);i++){
            if(!(isdigit(token[i]) || (i == 0 && token[i] == '-'))){
                fprintf(stderr, "Malformed line %s(%d)\n", argv[2], lineNum);
                return 1;
            }
        }

        //check if the integer is not negative (can be 0)
        if(atoi(token) < 0){
            fprintf(stderr, "runtime is not positive integer %s(%d)\n", argv[2], lineNum);
            return 1;
        }

        allJobs[numJobs].runTime = atoi(token);
        
        // probability of blocking
        token = tokens[2];
        // wrong float format
        // '\r'

        //for loop to check if the probability of blocking is an actual number (not malformed)
        for(int i = 0; i < strlen(token); i++){
            if(i >= 4 
                || (i != 1 && isdigit(token[i]) == FALSE)
                || (i == 1 && token[i] != '.')){
	            fprintf(stderr, "Malformed line %s(%d)\n", argv[2], lineNum);
                return 1;
            }
        }

        //check if the probability of blocking is between 0 and 1 inclusive.
        float prob = strtod(token, NULL);
        if (prob < 0 || prob > 1) {
            fprintf(stderr, "probability < 0 or > 1 %s(%d)\n", argv[2], lineNum);
            return 1;
        }

        allJobs[numJobs].probBlocking = prob * 100;

        numJobs++;
    }


    printf("Processes:\n\n");
	printf("   name     CPU time  when done  cpu disp  i/o disp  i/o time\n");


    int wallClock = 0;
    ready.busyTime = 0;
    ready.idleTime= 0;
    IO.busyTime = 0;
    IO.idleTime = 0;
    
    //If user chose -r (Round Robin)
    if(strcmp(argv[1], "-r") == 0){
        
        //Call Round Robin function
        

        // Put all jobs to ready queue
        // also sets running time left to runTime
        push_all_to_ready_RR(&ready, allJobs, numJobs);
        
        // while ready and IO queue have processes, process them
        while (is_empty(&ready) == FALSE || is_empty(&IO) == FALSE) {
            
            wallClock++;
            
            //get jobs from ready queue and I/O queue
            Job *readyHead = peek_queue(&ready), *IOHead = peek_queue(&IO);

            int runCPU = readyHead != NULL, runIO = IOHead != NULL;

              /////////
             /// CPU ///
              /////////

            
            if (runCPU) {

                // if timeUntilBlock == -2, DO NOT BLOCK AT ALL and run to completion
                // if timeUntilBlock == -1, We have not yet determined if it will block or not
                // if timeUntilBlock == 0, we have ran out of time, pop from ready, push to IO
                // if timeUntilBlock > 0, JUST DECREMENT

                //if timeUntilBlock == -1, we know this is the first time it has entered the CPU
                //therefore, we increment timesGivenCPU
                if ((readyHead->timeUntilBlock == -1)) {
                    readyHead->timesGivenCPU++;
                }

                // check if we will block
                if (readyHead->timeUntilBlock == -1 && readyHead->timeLeftRunning >= 2) {
                    // if we're not waiting for a block, get probability it will block
                    int r = get_random_to_100();
                    
                    int willBlock = r < readyHead->probBlocking;

                    //if blocks, see how long it runs until moved to IO queue
                    if (willBlock == TRUE) {
                        if (readyHead->timeLeftRunning >= 5) {
                            readyHead->timeUntilBlock = get_random_in_range(1, 6);
                        }
                        else {
                            readyHead->timeUntilBlock = get_random_in_range(1, readyHead->timeLeftRunning + 1);
                        }
                        
                    }
                    else {
                        // DO NOT BLOCK AT ALL
                        readyHead->timeUntilBlock = -2;
                    }
                }
                
                // it takes 1 tick to move done process off? weird...
                // if process is 0 time left, counts as "idle"
                // Here we decrement time left running and time left in the quantum
                if (readyHead->timeLeftRunning >= 0) {

                    if (readyHead->timeLeftRunning == 0)
                        ready.idleTime++;
                    else
                        ready.busyTime++;
                    readyHead->timeLeftRunning--;
                    readyHead->timeLeftQuantum--; 
                }

                //if process is going to block still, decrement time until process blocks
                // (do not decrement if timeUntilBlock is -1 or -2)
                if (readyHead->timeUntilBlock > 0)
                    readyHead->timeUntilBlock--;    

                // if we've gotten time until block to 0, BLOCK (pop from ready queue) and push to IO queue
                if (readyHead->timeUntilBlock == 0) {
                    readyHead->timeLeftIO = -1;
                    readyHead->timeUntilBlock = -1;
                    pop_queue(&ready);
                    push_queue(&IO, readyHead);
                    readyHead->timesBlockedForIO++;
                    readyHead->timeLeftQuantum = min(5, readyHead->timeLeftRunning);

                }                
                // Once quantum is finished, re-assign quantum time for the next time it is in CPU
                // if we're not blocking at the end of quantum, put to back of ready queue instead of IO queue (EVEN IF TIMERUNNING IS EMPTY (=-1)
                else if (readyHead->timeLeftQuantum == 0) {
                    readyHead->timeLeftQuantum = min(5, readyHead->timeLeftRunning);
                    if (readyHead->timeUntilBlock == -2) {
                        //reset timeUntilBlock for the next time it is in CPU, to check for blocking
                        readyHead->timeUntilBlock = -1;
                    }
                    pop_queue(&ready);
                    push_queue(&ready, readyHead);
                }
                
                // if we're not blocking at the end of process running, just pop from ready queue meaing job has finished
                else if (readyHead->timeLeftRunning == -1) {
                    
                    // PROCESS IS FINISHED COMPLETELY
                    readyHead->finishedTime = wallClock;
                    pop_queue(&ready);
                    printf("%-10s %6d     %6d    %6d    %6d    %6d\n", readyHead->name, readyHead->timeSpentForCPU, readyHead->finishedTime, readyHead->timesGivenCPU, readyHead->timesBlockedForIO, readyHead->timeSpentForIO);
                }
            }

            //if no jobs in the CPU/ready queue, the idle time for cpu increases
            else{
                ready.idleTime++;
            }

            ////////////
            ///  IO  
            ////////////

            if (runIO) {
                
                if (IOHead->timeLeftIO == -1) {
                    // if time left running is 0, just set IO block for 1 tick unconditionally
                    if (IOHead->timeLeftRunning == 0) {
                        IOHead->timeLeftIO = 1;
                        IOHead->timeSpentForIO += 1;
                        
                    }
                    // otherwise, generate rand number 1-30 inclusive
                    else {
                        int howLongIO = get_random_in_range(1, 31);
                        
                        IOHead->timeLeftIO = howLongIO;
                        IOHead->timeSpentForIO += howLongIO;
                    }
                }

                IOHead->timeLeftIO--;
                IO.busyTime++;
                
                if (IOHead->timeLeftIO <= 0) {
                    
                    pop_queue(&IO);
                    push_queue(&ready, IOHead);
                }
            }
            else{
                IO.idleTime++;
            }
        }

    }

    //If user chose -f (FCFS)
    else if(strcmp(argv[1], "-f") == 0) {

        // Put all jobs to ready queue
        // also sets running time left to runTime
        push_all_to_ready_FCFS(&ready, allJobs, numJobs);
        
        // while ready and IO queue have processes, process them
        while (is_empty(&ready) == FALSE || is_empty(&IO) == FALSE) {
            
            wallClock++;
            //get jobs from ready queue and I/O queue
            Job *readyHead = peek_queue(&ready), *IOHead = peek_queue(&IO);

            int runCPU = readyHead != NULL, runIO = IOHead != NULL;
            

              /////////
             /// CPU ///
              /////////

            
            if (runCPU) {           

                // if timeUntilBlock == -2, DO NOT BLOCK AT ALL and run to completion
                // if timeUntilBlock == -1, We have not yet determined if it will block or not
                // if timeUntilBlock == 0, we have ran out of time, pop from ready, push to IO
                // if timeUntilBlock > 0, JUST DECREMENT

                // check if we will block
                if (readyHead->timeUntilBlock == -1 && readyHead->timeLeftRunning >= 2) {
                    // if we're not waiting for a block, get probability it will block
                    int r = get_random_to_100();
                    
                    int willBlock = r < readyHead->probBlocking;

                    //if blocks, see how long it runs until moved to IO queue
                    if (willBlock == TRUE) {
                        readyHead->timeUntilBlock = get_random_in_range(1, readyHead->timeLeftRunning + 1);
                    }
                    else {
                        // DO NOT BLOCK AT ALL
                        readyHead->timeUntilBlock = -2;
                    }
                }
                
                // it takes 1 tick to move done process off? weird...
                // if process is 0 time left, counts as "idle"
                if (readyHead->timeLeftRunning >= 0) {
                    if (readyHead->timeLeftRunning == 0)
                        ready.idleTime++;
                    else
                        ready.busyTime++;
                    readyHead->timeLeftRunning--; 

                }

                //if process is going to block still, decrement time
                // (do not decrement if timeUntilBlock is -1 or -2)
                if (readyHead->timeUntilBlock > 0)
                    readyHead->timeUntilBlock--;      

                // if we've gotten time until block to 0, "BLOCK" (pop from ready queue) and push to IO queue
                if (readyHead->timeUntilBlock == 0) {
                    readyHead->timeLeftIO = -1;
                    readyHead->timeUntilBlock = -1;
                    pop_queue(&ready);
                    push_queue(&IO, readyHead);
                    
                    readyHead->timesBlockedForIO++;
                }
                // if we're not blocking at the end of process running, just pop from ready queue
                else if (readyHead->timeLeftRunning == -1) {
                    // PROCESS IS FINISHED COMPLETELY

                    readyHead->finishedTime = wallClock;
                    printf("%-10s %6d     %6d    %6d    %6d    %6d\n", readyHead->name, readyHead->timeSpentForCPU, readyHead->finishedTime, readyHead->timesGivenCPU, readyHead->timesBlockedForIO, readyHead->timeSpentForIO);
                    
                    pop_queue(&ready);
                }
            }
            else{
                ready.idleTime++;
            }

            /////////////
            // IO 
            /////////////

            if (runIO) {
                
                if (IOHead->timeLeftIO == -1) {
                    // if time left running is 0, just set IO block for 1 tick unconditionally
                    if (IOHead->timeLeftRunning == 0) {
                        IOHead->timeLeftIO = 1;
                        IOHead->timeSpentForIO += 1;
                        
                    }
                    // otherwise, generate rand number 1-30 inclusive
                    else {
                        int howLongIO = get_random_in_range(1, 31);
                        IOHead->timeLeftIO = howLongIO;
                        IOHead->timeSpentForIO += howLongIO;
                    }
                }

                IO.busyTime++;
                IOHead->timeLeftIO--;
                
                if (IOHead->timeLeftIO <= 0) {
                    pop_queue(&IO);
                    
                    IOHead->timesGivenCPU++;
                    push_queue(&ready, IOHead);
                }
            }
            else{
                IO.idleTime++;
            }
        }
    }
    

    //what happens if it is put into io queue when there is 0 run time left
    int i;
    int cpuDispatch = 0;
    int ioDispatch = 0;
    for (i = 0; i < numJobs; i++) {
        Job *job = &allJobs[i];
        cpuDispatch += job->timesGivenCPU;
        ioDispatch += job->timesBlockedForIO;
    }

    
    printf("\nSystem:\nThe wall clock time at which the simulation finished: %d\n", wallClock);

    //calculate utilization
    double cpuUtil = (double)ready.busyTime / (double)(ready.busyTime + ready.idleTime);
    double ioUtil = (double)IO.busyTime / (double)(IO.busyTime + IO.idleTime);

    //calculate throughput
    double cpuThrough = (double)numJobs/(double)(ready.busyTime + ready.idleTime);
    double ioThrough = (double)numJobs/(double)(IO.busyTime + IO.idleTime);

	/* print cpu statistics */
	printf("\nCPU:\n");
	printf("Total time spent busy: %d\n", ready.busyTime);
	printf("Total time spent idle: %d\n", ready.idleTime);
	printf("CPU utilization: %.2f\n", cpuUtil);
	printf("Number of dispatches: %d\n", cpuDispatch);
	printf("Overall throughput: %.2f\n", cpuThrough);

	/* print i/o statistics */
	printf("\nI/O device:\n");
	printf("Total time spent busy: %d\n", IO.busyTime);
	printf("Total time spent idle: %d\n", IO.idleTime);
	printf("I/O utilization: %.2f\n", ioUtil);
	printf("Number of dispatches: %d\n", ioDispatch);
	printf("Overall throughput: %.2f\n", ioThrough);
    return 0;
}