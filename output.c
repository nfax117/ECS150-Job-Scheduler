/*
 * this shows how output should look
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	/*
	 * printing process information
	 */
	printf("Program output (to stdout):\n------------------\n");
	/* header line */
	printf("Processes:\n\n");
	printf("   name     CPU time  when done  cpu disp  i/o disp  i/o time\n"
);
	/* for process information */
        printf("%-10s %6d     %6d    %6d    %6d    %6d\n", "program1", 25, 75, 5, 10, 50);

	/* print clock time at end */
	printf("\nSystem:\n");
	printf("The wall clock time at which the simulation finished: %d\n", 75);

	/* print cpu statistics */
	printf("\nCPU:\n");
	printf("Total time spent busy: %d\n", 25);
	printf("Total time spent idle: %d\n", 50);
	printf("CPU utilization: %.2f\n", 0.31);
	printf("Number of dispatches: %d\n", 5);
	printf("Overall throughput: %.2f\n", 0.01);

	/* print i/o statistics */
	printf("\nI/O device:\n");
	printf("Total time spent busy: %d\n", 50);
	printf("Total time spent idle: %d\n", 25);
	printf("I/O utilization: %.2f\n", 0.66666);
	printf("Number of dispatches: %d\n", 10);
	printf("Overall throughput: %.2f\n", 0.05);

	/*
	 * error messages
	 * the arguments for %s(%d) are file name and line number,
	 *     respectively
	 * all cause exit with status code 1 (ie, exit(1))
	 */
	printf("\n\n\nProgram error output (to stderr):\n------------------\n");
	/* bad option (not -f or -r) */
	fprintf(stderr, "Usage: %s [-r | -f] file\n", argv[0]);
	/* can't open file */
	errno = ENOENT;
	perror("filename");
	/* the line in the input file is malformed */
	fprintf(stderr, "Malformed line %s(%d)\n", "filename", 100);
	/* process name is to long (over 10 characters) */
	fprintf(stderr, "name is too long %s(%d)\n", "filename", 100);
	/* runtime is 0 or less */
	fprintf(stderr, "runtime is not positive integer %s(%d)\n", "filename", 100);
	/* probability is not between 0 and 1 */
	fprintf(stderr, "probability < 0 or > 1 %s(%d)\n", "filename", 100);

	exit(0);
}
