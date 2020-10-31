#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	
	// too few arguments in terminal (returns error code 1)
	if (argc < 3) {
		printf("usage:\t%s <argument1> <argument2>\n", argv[0]);
		exit(1);
	}

	// too many arguments in terminal (returns error code 2)
	else if (argc > 3) {
		printf("usage:\t%s <argument1> <argument2>\n", argv[0]);
		exit(2);
	}

	// default to else if argc == 3
	else {
		printf("The executable %s was launched with two arguments:\n", argv[0]);
		printf("\tThe first argument is: %s,\n", argv[1]);
		printf("\tThe second argument is: %s.\n", argv[2]);
	}

	return 0;
}