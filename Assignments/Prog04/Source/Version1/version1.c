#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

/**
	* Summary: The purpose of this function is to count the number of files in a given directory, and return that number as an integer
	* @param dir: The path to the desired directory in which the number of files will be counted
	* @return integer that stores the number of files in the given directory
*/
int count_files(char* dir) {

	char* rootPath = dir;
	DIR* directory = opendir(rootPath);

	struct dirent* entry;
	int counter = 0;

	while ((entry = readdir(directory)) != NULL) {

        char* name = entry->d_name;
	        
        if ((name[0] != '.') && (entry->d_type == DT_REG)) {
			counter++;
        }
    }

	closedir(directory);

	return counter;
}

/**
	* Summary: The purpose of this function is to store all desired file paths to a char** array
	* @param numFiles: The number of files in the given directory
	* @param dir: The path to the desired directory in which the char** array will add files from
	* @param filePath: The path to each text file in the given directory
	* @return char** array full of target file paths
*/
char** file_array(int numFiles, char* dir, char* filePath) {

	char** fileNames = (char**) malloc(numFiles*sizeof(char*));

	int i = 0;
	char* rootPath = dir;
	DIR* directory = opendir(rootPath);
	struct dirent* entry;
    
    while ((entry = readdir(directory)) != NULL) {
        
        char* name = entry->d_name;
        
        if ((name[0] != '.') && (entry->d_type == DT_REG)) {
			fileNames[i] = (char*) malloc(strlen(filePath) + strlen(name) + 1);
			sprintf(fileNames[i], "%s%s", filePath, name);
			i++;
        }
    }

	closedir(directory);

	return fileNames;
}

/**
	* Summary: The purpose of this function is to assure the directory/file path given by a user has a forward slash '/' at the end
	* @param dataPath: The path to the desired directory in which the forward slash will be added
	* @return char* array that holds the new directory/file path
*/
char* new_path(char* dataPath) {
	
	// remove forward slash from dataPath (if any)
	if (dataPath[strlen(dataPath) - 1] == '/') {
		dataPath[strlen(dataPath) - 1] = '\0';
	}

	// create new path (added '/' at the end)
	char slash = '/';
	char* newPath = strncat(dataPath, &slash, 1);

	return newPath;
}

/**
	* Summary: The purpose of this function is to write distributor information to a text file
	* @param index: The distributor index in which each file is being written
	* @param fileCount: The number of files in the given directory
	* @param numProcs: The number of distributors being utilized
	* @param sorterCount: A list of the number of files that go to each sorter
	* @param sorterList: A 2D list of file indices that go to each sorter
	* @return void
*/
void write_file(int index, int fileCount, int numProcs, int* sorterCount, int** sorterList) {
	
	// create output file
	FILE* outFile;
		
	// name of .txt file (w/ extension)
	char* prefix = "distr_";
	char* ext = ".txt";

	// allocate space for prefix + distrIndex + extension (distr_0.txt, distr_1.txt, etc.)
	char* fname = (char*) calloc(strlen(prefix) + strlen(ext) + 2, sizeof(char));
	sprintf(fname, "%s%d%s", prefix, index, ext);
		
	// write to respective .txt file(s)
	outFile = fopen(fname, "w");

	// output number of files for each sorter
	for (int i = 0; i < numProcs; i++) {
		fprintf(outFile, "%d ", sorterCount[i]);
	}

	fprintf(outFile, "\n");

	// output file indices that go to each sorter
	for (int i = 0; i < numProcs; i++) {
		for(int j = 0; j < (fileCount/numProcs); j++) {
			fprintf(outFile, "%d ", sorterList[i][j]);
		}
		
		fprintf(outFile, "\n");
	}
}

/**
	* Summary: The purpose of this function is to determine which files go to their respective sorters
	* @param index: The distributor index in which each file is being read
	* @param numProcs: The number of distributors being utilized
	* @param fileCount: The number of files in the given directory
	* @param dataPath: The directory path in which each file is being read
	* @param fileArray: A char** array that stores each file's path in the given directory
	* @param startIndex: The file index (in the directory) in which the distributor starts to read from
	* @param endIndex: The file index (in the directory) in which the distributor reads up to
	* @return void
*/
void distributor(int index, int numProcs, int fileCount, char* dataPath, char** fileArray, int startIndex, int endIndex) {

	int* sorterCount = (int*) calloc(numProcs, sizeof(int)); // number of files for each sorter
	int** sorterList = (int**) calloc(numProcs, sizeof(int*)); // file indices that go to each sorter

	// preallocate max possible space for arrays
	for (int i = 0; i < numProcs; i++) {
		sorterList[i] = (int*) calloc(endIndex-startIndex+1, sizeof(int));
	}

	// process index variable
	int procIndex;

	for (int k = startIndex; k <= endIndex; k++) {
		FILE *inFile = fopen(fileArray[k], "r");
		
		// scan in process index to variable
		fscanf(inFile, "%d", &procIndex);
		
		sorterList[procIndex][sorterCount[procIndex]] = k;
		sorterCount[procIndex]++;
	}

	write_file(index, fileCount, numProcs, sorterCount, sorterList);
}

/**
	* Summary: The purpose of this function is to sort the text files by line number, and output them to a text file (in order)
	* @param index: The sorter index in which each file is being read
	* @param numProcs: The number of distributors being utilized
	* @param fileCount: The number of files in the given directory
	* @param fileArray: A char** array that stores each file's path in the given directory
	* @return void
*/
void sorter(int index, int numProcs, int fileCount, char** fileArray) {
	return;
}

/**
	* Summary: This function runs all previous functions in their respective order
	* @param argc: The number of arguments given by the user
	* @param argv: The list of arguments given by the user
	* @return integer value designating the end of the program
*/
int main(int argc, char** argv) {

	// Data Validation
	if (argc == 3) {
		
		// take arguments in as variables
		char* dataPath = argv[1];
		int numProcs = atoi(argv[2]);
		dataPath = new_path(dataPath); // added slash to end of path

		// count number of files in directory
		int fileCount = count_files(dataPath);

		// create array of file paths
		char** fileArray = file_array(fileCount, dataPath, dataPath);

		int m = fileCount/numProcs;
		int remainder = fileCount%numProcs;

		int startIndex = 0;
		int endIndex = m - 1;

		for (int i = 0; i < numProcs; i++) {
			
			if (i < remainder) {
				endIndex++;
			}

			distributor(i, numProcs, fileCount, dataPath, fileArray, startIndex, endIndex);

			startIndex = endIndex + 1;
			endIndex = startIndex + m - 1;
		}

		for(int i = 0; i < numProcs; i++) {
			sorter(i, numProcs, fileCount, fileArray);
		}
	}

	else {
		printf("Wrong Number of Arguments: Please Try Again\n");
		return 0;
	}
}