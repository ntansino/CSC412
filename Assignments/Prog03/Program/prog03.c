#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct Image {
	int numRows, numCols;
	char** data;
}Image;

Image read_image(char* patPath) {
	
	// create image for pattern
	Image pattern;

	// open pattern file from path
	FILE *inFile = fopen(patPath, "r");

	// read first line to get numRows/numCols
	fscanf(inFile, "%d%d\n", &(pattern.numCols), &(pattern.numRows));

	// allocate numRows
	pattern.data = (char**) calloc(pattern.numRows, sizeof(char*));

	// read pattern into image
	for(int i = 0; i < pattern.numRows; i++) {

		// allocate numCols
		pattern.data[i] = (char*) calloc(pattern.numCols + 1, sizeof(char));

		// store data in 2D array
		for(int j = 0; j < pattern.numCols; j++) {
			fscanf(inFile, "%c", &(pattern.data[i][j]));
		}
		
		// account for '\n'
		char c;
		fscanf(inFile, "%c", &c);
	}


	return pattern;
}

char* find_matches(Image img, Image pattern) {

	// counter for found matches
	int count = 0;

	// create array for found matches
	char* matchArray = (char*) calloc(1, sizeof(char));

	// search image for pattern
	for(int i = 0; i <= img.numRows - pattern.numRows; i++) {
		for(int j = 0; j <= img.numCols - pattern.numCols; j++) {
			
			// assume match
			int match = 1;

			for(int k = 0; k < pattern.numRows && match == 1; k++) {
				for(int l = 0; l < pattern.numCols && match == 1; l++) {

					// only reaches conditional if discrepancy is found
					if (pattern.data[k][l] != img.data[i+k][j+l]) {
						match = 0;
					}

				}
			}

			// instance of pattern found
			if (match == 1) {
				count++;
			}

		}
	}

	// add count to matchArray (at the end)
	matchArray = (char*) calloc(1, sizeof(char*));
	sprintf(matchArray, "%c", count);

	return matchArray;
}

bool check_dir(char* dir) {
	
	// create variable for path and directory
	char* rootPath = dir;
	DIR* directory = opendir(rootPath);
	    
	// no directory found at path
	if (directory == NULL) {
		printf("data folder %s not found\n", rootPath);
		exit(1);
	}

	closedir(directory);

	return true;
}

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

void print_array(int counter, char** fileArray) {

	for (int i = 0; i < counter; i++) {
		printf("\t%s\n", fileArray[i]);
	}

	return;
}

int main(int argc, char** argv) {

	// Data Validation
	if (argc == 4) {

		// create path variables (based on args)
		char* patPath = argv[1];
		char* imgPath = argv[2];
		char* outPath = argv[3];

		// remove forward slash from imgPath (if any)
		if (imgPath[strlen(imgPath) - 1] == '/') {
			imgPath[strlen(imgPath) - 1] = '\0';
		}

		// create new path (added '/' at the end)
		char slash = '/';
		char* newPath = strncat(imgPath, &slash, 1);

		// validate directory
		if (check_dir(imgPath)) {
			
			// count number of files
			int fileCount = count_files(imgPath);

			// create array of discovered files (with their path)
			char** fileArray = file_array(fileCount, imgPath, newPath);
			
			// read/store pattern as image
			Image pattern = read_image(patPath);

			// check for pattern in image directory
			for(int i = 0; i < fileCount; i++) {
					
				char* fname = fileArray[i];

				Image img = read_image(fname);

				char* matches = find_matches(img, pattern);
			}

		}
	}

	else {
		printf("Incorrect number of arguments, please try again.\n");
	}

	return 0;
}