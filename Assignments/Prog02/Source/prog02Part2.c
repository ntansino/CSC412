#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

bool validate(int num, int arg1, int arg2, int arg3, char* execName) {

	// correct # of arguments
	if ((num == 4)) {

		// checks for all positive values
		if (arg1 > 0 && arg2 > 0 && arg3 > 0) {

			// syntax check
			if (arg2 > arg1) {
				return true;
			}

			// F1 > F2
			else {
				printf("usage:\t%s [F1 F2 n]+, with F2>F1>0 and n>0\n", execName);
				return false;
			}
		}

		// detected negative values
		else {
			printf("usage:\t%s [F1 F2 n]+, with F2>F1>0 and n>0\n", execName);
			return false;
		}
	}

	// incorrect # of arguments
	else {
		printf("usage:\t%s [F1 F2 n]+, with F2>F1>0 and n>0\n", execName);
		return false;
	}
}

int fibonacci(int f1, int f2, int n, int init, int Fnum) {
	
	// base case
	if (n <= 0) {
		printf("\n");
		return 0;
	}

	// prints initial statement (only prints once)
	if (n == init) {
		printf("%d Terms of the Fibonacci series with F1=%d and F2=%d:\n\t", n, f1, f2);
	}

	// print series (w/ formatting)
	printf("F%d=%d", Fnum, f1);

	if (n >= 2) {
		printf(", ");
	}

	return fibonacci(f2, (f2+f1), n-1, init, Fnum + 1);
}

int main(int argc, char* argv[]) {

	int len = argc - 1;
	char* prog = argv[0];

	// check size of list (assures triplets)
	if ((len != 0) && ((len % 3) == 0)) {

		for (int i = 1; i+2 <= len; i += 3) {

			// convert ASCII to int
			int arg1 = atoi(argv[i]);
			int arg2 = atoi(argv[i+1]);
			int arg3 = atoi(argv[i+2]);

			// Data Validation
			if (validate(argc, arg1, arg2, arg3, prog)) {
				fibonacci(arg1, arg2, arg3, arg3, 1);
			}
		}

		printf("The values encountered in the sequences are\n\t{ }\n");
	}

	// incorrect input
	else {
		printf("usage:\t%s [F1 F2 n]+, with F2>F1>0 and n>0\n", prog);
		return 0;
	}
}