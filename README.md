C Program to simulate a job scheduler for a computer system's CPU. It models jobs being placed in a queue, and the execution of jobs using two scheduling algorithms: Round Robin (RR) and First-Come-First-Serve (FCFS). The simulation reads job information from a file specified as a command-line argument and outputs various statistics about the simulation.

1.) Job Struct and JobQueue Struct:
- The Job structure holds information about a job, including its name, runtime, and probability of blocking during execution.
- The JobQueue structure represents a queue of jobs, including pointers to the jobs, head, tail, and some statistics about the queue.

2.) Queue Operations (push_queue, pop_queue, peek_queue):
- Functions for basic queue operations, such as pushing a job onto the queue, popping a job from the queue, and peeking at the front of the queue.

3.) Initialization and Job Reading:
- The program reads job information from a file specified in the command line arguments.
- It validates and processes each line of the file, storing job details in the allJobs array.

4.) Job Scheduling Functions (push_all_to_ready_FCFS, push_all_to_ready_RR):
- These functions initialize the ready queue with jobs, setting their initial parameters based on the scheduling algorithm (FCFS or RR).

5.) Simulation Loop (main function):
- The main simulation loop runs until both the ready queue and I/O queue are empty.
- It simulates the execution of jobs in the CPU and I/O devices based on the chosen scheduling algorithm.
- Jobs can block for I/O operations, and the program randomly determines whether a job will block during execution.

6.) Statistics and Output:
- The program outputs various statistics, including CPU and I/O device utilization, number of dispatches, and overall throughput.
- It also prints individual job statistics, such as CPU time, completion time, CPU dispatch count, I/O dispatch count, and I/O time.

7.) Random Number Generation:
- The program uses the random() function for generating random numbers, such as determining if a job will block and for calculating I/O times.

8.) Command-Line Arguments Handling:
- The program expects two command-line arguments: the scheduling algorithm (-r for RR or -f for FCFS) and the input file containing job details.

Error Checking:
- The program performs various error checks, such as validating file existence, checking the format of the input file, and ensuring valid job parameters.


Overall, the program provides a basic simulation of a job scheduler and allows for the comparison of RR and FCFS scheduling algorithms based on simulated job executions.
