//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <iostream>
#include <string>
#include <random>
#include <sstream>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <climits>
//
#include "gl_frontEnd.h"

using namespace std;


//==================================================================================
//	Function prototypes
//==================================================================================
void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = NUM_DIRECTIONS);
TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);
unsigned int getPartitionFromLocation(unsigned int travelerRow, unsigned int travelerCol);
bool movePartition(SlidingPartition* part, unsigned int tRow, unsigned int tCol);
void generateTravelerThreads(int count);
void* findExit(void*);
// Direction randomizeDir (void);
vector<Direction> getFreeSpaces (const TravelerSegment& head);
void cleanGrid(void);
void joinThreads(void);
void quit(void);

//==================================================================================
//	Application-level global variables
//==================================================================================

#define ON 1
#define OFF 2

#define DEBUG_OUTPUT ON
#define WALLS ON
#define PARTITIONS ON

// OFF

// typedef struct TravelerThread
// {
// 	pthread_t id;
// 	int index;
// 	unsigned int startRow, endRow;
//
// } ThreadInfo;

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;	//	height of the grid
unsigned int numCols = 0;	//	width
unsigned int numTravelers = 0;	//	initial number
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;		//	the number of live traveler threads
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;
GridPosition	exitPos;	//	location of the exit

int growthRate;

//	travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;
int sleepTime = 500000;

//	Random generators:  For uniform distributions
// const unsigned int INT_MAX = 1;
const unsigned int MAX_NUM_INITIAL_SEGMENTS = 6;
random_device randDev;
default_random_engine engine(randDev());
uniform_int_distribution<unsigned int> unsignedNumberGenerator(0, numeric_limits<unsigned int>::max());
uniform_int_distribution<unsigned int> segmentNumberGenerator(0, MAX_NUM_INITIAL_SEGMENTS);
uniform_int_distribution<unsigned int> segmentDirectionGenerator(0, NUM_DIRECTIONS-1);
uniform_int_distribution<unsigned int> headsOrTails(0, 1);
uniform_int_distribution<unsigned int> rowGenerator;
uniform_int_distribution<unsigned int> colGenerator;


// VERSION03
// mutex lock for grid data structure
pthread_mutex_t gridLock;
pthread_mutex_t outLock;
vector<pthread_mutex_t> travelerLocks;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

void drawTravelers(void)
{
	//-----------------------------
	//	You may have to sychronize things here
	//-----------------------------
	// TODO
	for (unsigned int i = 0; i < travelerList.size(); i++)
	{
		pthread_mutex_lock(&travelerLocks[i]);
		if (travelerList[i].draw)
			drawTraveler(travelerList[i]);
		pthread_mutex_unlock(&travelerLocks[i]);
	}
}

void updateMessages(void)
{
	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	unsigned int numMessages = 4;
	sprintf(message[0], "We created %d travelers", numTravelers);
	sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
	sprintf(message[2], "Cache $$Mula$$ BBY");
	sprintf(message[3], "Simulation run time is %ld", time(NULL)-launchTime);

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	pthread_mutex_lock(&gridLock);
	drawMessages(numMessages, message);
	pthread_mutex_unlock(&gridLock);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;

	switch (c)
	{
		//	'esc' to quit
		case 27:
			quit();
			break;

		//	slowdown
		case ',':
			slowdownTravelers();
			ok = 1;
			break;

		//	speedup
		case '.':
			speedupTravelers();
			ok = 1;
			break;

		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		// TODO
	}
}


//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * travelerSleepTime) / 10;

	if (newSleepTime > MIN_SLEEP_TIME)
	{
		travelerSleepTime = newSleepTime;
	}
}

void slowdownTravelers(void)
{
	//	increase sleep time by 20%
	travelerSleepTime = (12 * travelerSleepTime) / 10;
}




//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	cout << "-----------------------------------------" << endl;
    cout << "|\tFinal Project Fall 2020 \t|" << endl;
    cout << "|\tJohn O'Donnell,Sierra Obi,\t|" << endl;
	cout << "|\tand Nick Tansino\t\t|" << endl;
    cout << "-----------------------------------------" << endl;
    cout << "running..." << endl;
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of travelers, etc.
	//	So far, I hard code-some values

	if (argc == 4 || argc == 5)
	{
		numLiveThreads = 0;
		numTravelersDone = 0;
		if (argc != 5)
		{
			numRows = atoi(argv[1]);
			numCols = atoi(argv[2]);
			numTravelers = atoi(argv[3]);
			growthRate = INT_MAX;
		}
		else
		{
			numRows = atoi(argv[1]);
			numCols = atoi(argv[2]);
			numTravelers = atoi(argv[3]);
			growthRate = atoi(argv[4]);
		}

		// INITIALIZE MUTEX LOCKS
		pthread_mutex_init(&gridLock, NULL);
		pthread_mutex_init(&outLock, NULL);
		for (unsigned int i = 0; i < numTravelers; i++) {
			pthread_mutex_t this_traveler;
			pthread_mutex_init(&this_traveler, NULL);
			travelerLocks.push_back(this_traveler);
		}

		//	Even though we extracted the relevant information from the argument
		//	list, I still need to pass argc and argv to the front-end init
		//	function because that function passes them to glutInit, the required call
		//	to the initialization of the glut library.
		initializeFrontEnd(argc, argv);
		//	Now we can do application-level initialization
		initializeApplication();
		launchTime = time(NULL);
		generateTravelerThreads(numTravelers);

		//	Now we enter the main loop of the program and to a large extend
		//	"lose control" over its execution.  The callback functions that
		//	we set up earlier will be called when the corresponding event
		//	occurs
		glutMainLoop();
		cleanGrid();
	}
	else
	{
		pthread_mutex_lock(&outLock);
		cout << "Usage: " << argv[0] << " <number-of-rows> <number-of-cols> <number-of-travelers>" << endl;
		cout << "Usage: " << argv[0] << " <number-of-rows> <number-of-cols> <number-of-travelers> <segment-growth-rate>" << endl;
		pthread_mutex_unlock(&outLock);
		exit(1);
	}
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a function that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
	//	Initialize some random generators
	rowGenerator = uniform_int_distribution<unsigned int>(0, numRows-1);
	colGenerator = uniform_int_distribution<unsigned int>(0, numCols-1);

	//	Allocate the grid
	grid = new SquareType*[numRows];
	for (unsigned int i=0; i<numRows; i++)
	{
		grid[i] = new SquareType[numCols];
		for (unsigned int j=0; j< numCols; j++)
			grid[i][j] = FREE_SQUARE;

	}

	message = new char*[MAX_NUM_MESSAGES];
	for (unsigned int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = new char[MAX_LENGTH_MESSAGE+1];

	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	generate a random exit
	exitPos = getNewFreePosition();
	grid[exitPos.row][exitPos.col] = EXIT;

	//	Generate walls and partitions
	#if WALLS == ON
		generateWalls();
	#endif
	#if PARTITIONS == ON
		generatePartitions();
	#endif

	//	Initialize traveler info structs
	//	You will probably need to replace/complete this as you add thread-related data
	float** travelerColor = createTravelerColors(numTravelers);

	for (unsigned int k=0; k<numTravelers; k++) {
		GridPosition pos = getNewFreePosition();
		//	Note that treating an enum as a sort of integer is increasingly
		//	frowned upon, as C++ versions progress
		Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));

		TravelerSegment seg = {pos.row, pos.col, dir};
		Traveler traveler;
		traveler.segmentList.push_back(seg);

		grid[pos.row][pos.col] = TRAVELER;

		//	I add 0-n segments to my travelers
		unsigned int numAddSegments = segmentNumberGenerator(engine);
		TravelerSegment currSeg = traveler.segmentList[0];
		bool canAddSegment = true;

		pthread_mutex_lock(&outLock);
		cout << "  Traveler [" << k << "] at (" << pos.row << "," <<
			pos.col << "), dir: " << dirStr(dir) << ", with up to " << numAddSegments << " additional segments" << endl;
		cout << "\t";
		pthread_mutex_unlock(&outLock);

		for (unsigned int s=0; s<numAddSegments && canAddSegment; s++)
		{
			TravelerSegment newSeg = newTravelerSegment(currSeg, canAddSegment);
			if (canAddSegment)
			{
				traveler.segmentList.push_back(newSeg);
				currSeg = newSeg;
				cout << dirStr(newSeg.dir) << "  ";
			}
		}
		cout << endl;

		for (unsigned int c=0; c<4; c++)
			traveler.rgba[c] = travelerColor[k][c];

		travelerList.push_back(traveler);
	}

	//	free array of colors
	for (unsigned int k=0; k<numTravelers; k++)
		delete []travelerColor[k];
	delete []travelerColor;
}


//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void)
{
	GridPosition pos;

	bool noGoodPos = true;
	while (noGoodPos)
	{
		unsigned int row = rowGenerator(engine);
		unsigned int col = colGenerator(engine);
		if (grid[row][col] == FREE_SQUARE)
		{
			pos.row = row;
			pos.col = col;
			noGoodPos = false;

		}
	}
	return pos;
}

Direction newDirection(Direction forbiddenDir)
{
	bool noDir = true;

	Direction dir = NUM_DIRECTIONS;
	while (noDir)
	{
		dir = static_cast<Direction>(segmentDirectionGenerator(engine));
		noDir = (dir==forbiddenDir);
	}
	return dir;
}


TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd)
{
	TravelerSegment newSeg;
	switch (currentSeg.dir)
	{
		case NORTH:
			if (	currentSeg.row < numRows-1 &&
					grid[currentSeg.row+1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row+1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(SOUTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case SOUTH:
			if (	currentSeg.row > 0 &&
					grid[currentSeg.row-1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row-1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(NORTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case WEST:
			if (	currentSeg.col < numCols-1 &&
					grid[currentSeg.row][currentSeg.col+1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col+1;
				newSeg.dir = newDirection(EAST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case EAST:
			if (	currentSeg.col > 0 &&
					grid[currentSeg.row][currentSeg.col-1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col-1;
				newSeg.dir = newDirection(WEST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		default:
			canAdd = false;
	}

	return newSeg;
}

void generateWalls(void)
{
	const unsigned int NUM_WALLS = (numCols+numRows)/4;

	//	I decide that a wall length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_WALL_LENGTH = 3;
	const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodWall = true;

	//	Generate the vertical walls
	for (unsigned int w=0; w< NUM_WALLS; w++)
	{
		goodWall = false;

		//	Case of a vertical wall
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;

				//	select a column index
				unsigned int HSP = numCols/(NUM_WALLS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);

				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}

				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
		// case of a horizontal wall
		else
		{
			goodWall = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;

				//	select a column index
				unsigned int VSP = numRows/(NUM_WALLS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);

				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}

				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
	}
}


void generatePartitions(void)
{
	const unsigned int NUM_PARTS = (numCols+numRows)/4;

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_PARTITION_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodPart = true;

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		goodPart = false;

		//	Case of a vertical partition
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a column index
				unsigned int HSP = numCols/(NUM_PARTS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2))*HSP + HSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);

				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}

				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part);
				}
			}
		}
		// case of a horizontal partition
		else
		{
			goodPart = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a column index
				unsigned int VSP = numRows/(NUM_PARTS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2))*VSP + VSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);

				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}

				//	if the wall first, add it to the grid and build SlidingPartition object
				if (goodPart)
				{
					SlidingPartition part;
					part.isVertical = false;
					for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
					{
						grid[row][col] = HORIZONTAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part);
				}
			}
		}
	}
}

void generateTravelerThreads(int count)
{
	// create a thread for each of the n travelers from the user input
	for (int k = 0; k < count; k++)
	{
		pthread_mutex_lock(&travelerLocks[k]);
		travelerList[k].index = k;
		pthread_t* this_id = &(travelerList[k].id);
		pthread_mutex_unlock(&travelerLocks[k]);
		pthread_create(this_id,nullptr,findExit,&(travelerList[k]));
		numLiveThreads++;
	}
}

void* findExit (void* arg)
{
	int count = 0;
	Traveler* traveler = static_cast<Traveler*>(arg);

	#if DEBUG_OUTPUT == ON
		pthread_mutex_lock(&outLock);
		std::cout << "\n + Traveler [" << traveler->index << "] created " <<
			" still searching: "<< traveler->searching << std::endl;
		for (unsigned long i=0; i<traveler->segmentList.size(); i++)
		{
			std::cout << "\n\t(row=" << traveler->segmentList[i].row << ", col=" <<
			traveler->segmentList[i].col << "), direction: " << dirStr(traveler->segmentList[i].dir) << std::endl;
		}
		std::cout << std::flush;
		pthread_mutex_unlock(&outLock);
	#endif


	bool run = true;
	int moveAttempts = 0;
	while(run)
	{
		// move the traveler until the exit is reached
		bool couldMove = traveler->move();
		if (couldMove)
		{
			moveAttempts = 0;
			TravelerSegment tail = traveler->getTail();
			count++;
			// TODO
			if (count % growthRate == 0)
				traveler->segmentList.push_back(tail);
			if (traveler->segmentList.size() < 1 && traveler->searching == false) {
				traveler->draw = false;
				run = false;
			}
		}
		else
		{
			if (moveAttempts++ > 20)
			{
				traveler->searching = false;
				run = false;

				for (unsigned int i = 0; i < traveler->segmentList.size(); i++) {
					unsigned int row = traveler->segmentList[i].row;
					unsigned int col = traveler->segmentList[i].col;
					grid[row][col] = FREE_SQUARE;
				}
			}
		}
		usleep(sleepTime);
	}

	if (moveAttempts <= 20) {
		numTravelersDone++;
	}


	return NULL;
}


bool Traveler::move(void)
{
	bool canMove = true;
	const TravelerSegment& head = getHead();
	const TravelerSegment& tail = getTail();

	// TODO - remove
	pthread_mutex_lock(&gridLock);

	vector<Direction> freeSpaces = getFreeSpaces(head);

	if (freeSpaces.empty())
	{
		canMove = false;
		#if DEBUG_OUTPUT == ON
			stringstream sstr;
			pthread_mutex_lock(&outLock);
			sstr << "\n\t\t  -= traveler " << this->index << " has no moves left\n";
			cout << sstr.str() << flush;
			pthread_mutex_unlock(&outLock);
		#endif
	}
	else
	{
		// TODO - remove
		pthread_mutex_lock(&travelerLocks[this->index]);
		if (!searching)
		{
			if (segmentList.size() > 1) {
				unsigned int row = segmentList[segmentList.size() - 1].row;
				unsigned int col = segmentList[segmentList.size() - 1].col;
				grid[row][col] = FREE_SQUARE;
			}
			segmentList.pop_back();
			// make sqaye free
		}
		else
		{
			Direction randDir = freeSpaces[unsignedNumberGenerator(engine) % freeSpaces.size()];

			unsigned long lastIndex = segmentList.size()-1;
			// move the every trailing segment of a traveler
			if (!this->addTravelerSegment)
			{
				grid[tail.row][tail.col] = FREE_SQUARE;
			}
			else
			{
				TravelerSegment newSeg = tail;
				segmentList.push_back(newSeg);
				lastIndex--;
			}
			for (unsigned long k = lastIndex; k >= 1; k--)
			{
				segmentList[k].row = segmentList[k-1].row;
				segmentList[k].col = segmentList[k-1].col;
				segmentList[k].dir = segmentList[k-1].dir;
			}
			// reset the head to the location of the direction that is chosen
			searching = resetHead(randDir);

			#if DEBUG_OUTPUT == ON
				pthread_mutex_lock(&outLock);
				std::cout << "\n   -+ Traveler [" << index << "] moved " << dirStr(randDir) <<
						" still searching: "<< searching << std::endl;
				for (unsigned long i=0; i<segmentList.size(); i++)
				{
					std::cout << "\t(row=" << segmentList[i].row << ", col=" <<
					segmentList[i].col << "), direction: " << dirStr(segmentList[i].dir) << std::endl;
				}
				std::cout << std::flush;
				pthread_mutex_unlock(&outLock);
			#endif
		}
		// TODO - remove
		pthread_mutex_unlock(&travelerLocks[this->index]);
	}
	// TODO - remove
	pthread_mutex_unlock(&gridLock);
	return canMove;
}

bool Traveler::resetHead(Direction dir)
{
	bool exitNotFound = true;

	TravelerSegment& head = segmentList[0];
	head.dir = dir;

	if (dir == NORTH)
		head.row--;
	else if (dir == SOUTH)
		head.row++;
	else if (dir == EAST)
		head.col++;
	else if (dir == WEST)
		head.col--;
	if (grid[head.row][head.col] == FREE_SQUARE)
	{
		grid[head.row][head.col] = TRAVELER;
	}
	else
	{
		// the traveler has reached the exitPos
		exitNotFound = false;
	}

	return exitNotFound;
}


vector<Direction> getFreeSpaces(const TravelerSegment& head)
{
	unsigned int row = head.row, col = head.col;
	Direction dir = head.dir;
	// get valid moves for the current position
	vector<Direction> moves;
	bool partMoved;
	unsigned int partitionIdx;

	if((row > 0) && (dir != SOUTH) && ((grid[row-1][col] == HORIZONTAL_PARTITION) || (grid[row-1][col] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row-1, col);
		partMoved = movePartition(&partitionList[partitionIdx], row-1, col);
	}
	if((row < numRows-1) && (dir != NORTH) && ((grid[row+1][col] == HORIZONTAL_PARTITION) || (grid[row+1][col] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row+1, col);
		partMoved = movePartition(&partitionList[partitionIdx], row+1, col);
	}
	if((col < numCols-1) && (dir != WEST) && ((grid[row][col+1] == HORIZONTAL_PARTITION) || (grid[row][col+1] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row, col+1);
		partMoved = movePartition(&partitionList[partitionIdx], row, col+1);
	}
	if((col > 0) && (dir != EAST) && ((grid[row][col-1] == HORIZONTAL_PARTITION) || (grid[row][col-1] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row, col-1);
		partMoved = movePartition(&partitionList[partitionIdx], row, col-1);
	}

	#if DEBUG_OUTPUT == ON
		if (partMoved == true) {
			cout << "PARTITION " << partitionIdx << " MOVED" << endl;
		}
	#endif

	// add all valid moves to the vector
	if((row > 0) && (dir != SOUTH) && ((grid[row-1][col] == FREE_SQUARE) || (grid[row-1][col] == EXIT)))
	{
		moves.push_back(NORTH);
	}
	if((row < numRows-1) && (dir != NORTH) && ((grid[row+1][col] == FREE_SQUARE) || (grid[row+1][col] == EXIT)))
	{
		moves.push_back(SOUTH);
	}
	if((col < numCols-1) && (dir != WEST) && ((grid[row][col+1] == FREE_SQUARE) || (grid[row][col+1] == EXIT)))
	{
		moves.push_back(EAST);
	}
	if((col > 0) && (dir != EAST) && ((grid[row][col-1] == FREE_SQUARE) || (grid[row][col-1] == EXIT)))
	{
		moves.push_back(WEST);
	}

	return moves;
}

unsigned int getPartitionFromLocation(unsigned int travelerRow, unsigned int travelerCol) {
	for (unsigned int i = 0; i < partitionList.size(); i++) {
		for (unsigned int j = 0; j < partitionList[i].blockList.size(); j++) {
			if (travelerRow == partitionList[i].blockList[j].row && travelerCol == partitionList[i].blockList[j].col) {
				return i;
			}
		}
	}
	return -1;
}

bool movePartition(SlidingPartition* part, unsigned int tRow, unsigned int tCol) {
	if (part->isVertical) {
		// if the partition is vertical...

		unsigned int idx;
		for (unsigned int i = 0; i < part->blockList.size(); i++) {
			if (part->blockList[i].row == tRow && part->blockList[i].col == tCol) {
				idx = i;
			}
		}

		unsigned int to_move_down = idx + 1;
		unsigned int to_move_up = part->blockList.size() - idx;
		(void) to_move_down;

		// check if it will move upward
		unsigned int top_row = part->blockList[0].row;
		unsigned int top_col = part->blockList[0].col;
		bool can_move_up = true;
		for (unsigned int i = 1; can_move_up && i <= to_move_up; i++) {
			if (top_row-i > numRows) {
				can_move_up = false;
			} else if (top_row-i >= 0) {
				if (grid[top_row-i][top_col] != FREE_SQUARE) {
					can_move_up = false;
				}
			}
		}

		// if it can move without conflict, move and return
		if (can_move_up) {
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				part->blockList[i].row = part->blockList[i].row - to_move_up;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = VERTICAL_PARTITION;
			}
			return true;
		}

		// check if it can move downward
		unsigned int bottom_row = part->blockList[part->blockList.size() - 1].row;
		bool can_move_down = true;
		for (unsigned int i = 1; can_move_down && i <= to_move_down; i++) {
			if (bottom_row+i >= numRows) {
				can_move_down = false;
			} else if (bottom_row+i < numRows) {
				if (grid[bottom_row+i][tCol] != FREE_SQUARE) {
					can_move_down = false;
				}
			}
		}

		// if it can move down without conflict, move and return
		if (can_move_down) {
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				part->blockList[i].row = part->blockList[i].row + to_move_down;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = VERTICAL_PARTITION;
			}
			return true;
		}

	} else {
		// if the partition is horizontal...

		unsigned int idx;
		for (unsigned int i = 0; i < part->blockList.size(); i++) {
			if (part->blockList[i].row == tRow && part->blockList[i].col == tCol) {
				idx = i;
			}
		}

		unsigned int to_move_left = part->blockList.size() - idx;
		unsigned int to_move_right = idx + 1;

		// check if can move left
		unsigned int left_row = part->blockList[0].row;
		unsigned int left_col = part->blockList[0].col;
		bool can_move_left = true;
		for (unsigned int i = 1; can_move_left && i <= to_move_left; i++) {
			if (left_col-i > numRows) {
				can_move_left = false;
			} else if (left_col-i >= 0) {
				if (grid[left_row][left_col-i] != FREE_SQUARE) {
					can_move_left = false;
				}
			}
		}

		// if can move left, move and return
		if (can_move_left) {
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				part->blockList[i].col = part->blockList[i].col - to_move_left;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = HORIZONTAL_PARTITION;
			}
			return true;
		}

		// check if can move right
		unsigned int right_col = part->blockList[part->blockList.size() - 1].col;
		bool can_move_right = true;
		for (unsigned int i = 1; can_move_right && i <= to_move_right; i++) {
			if (right_col+i >= numCols) {
				can_move_right = false;
			} else if (right_col+i < numCols) {
				if (grid[tRow][right_col+i] != FREE_SQUARE) {
					can_move_right = false;
				}
			}
		}

		// if it can move right without conflit, move and return
		if (can_move_right) {
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				part->blockList[i].col = part->blockList[i].col + to_move_right;
			}
			for (unsigned int i = 0; i < part->blockList.size(); i++) {
				grid[part->blockList[i].row][part->blockList[i].col] = HORIZONTAL_PARTITION;
			}
			return true;
		}

	}

	return false;
}

void cleanGrid(void)
{
	pthread_mutex_lock(&gridLock);
	for (unsigned int i=0; i< numRows; i++)
		free(grid[i]);
	free(grid);
	delete []grid;
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
	pthread_mutex_unlock(&gridLock);
}

void joinThreads (void)
{
	int count = 0;
	for (unsigned int k = 0; k < numTravelers; k++)
	{
		pthread_join(travelerList[k].id,nullptr);
		numLiveThreads--;
		count ++;
	}
	#if DEBUG_OUTPUT == ON
		stringstream sstr;
		// notice that the threads will continue to run forever
	    // join the threads once they have finished processing
		pthread_mutex_lock(&outLock);
		sstr << "  simulation terminated" << endl;
		sstr << "  " << count << " threads joined\n";
		cout << sstr.str() << flush;
		pthread_mutex_unlock(&outLock);
	#endif
}

void quit (void)
{

	for (unsigned int k = 0; k < numTravelers; k++)
	{
		// pthread_mutex_lock(&travelerLocks[k]);
		travelerList[k].searching = false;
		// pthread_mutex_unlock(&travelerLocks[k]);
	}
	joinThreads();
	for (unsigned int k = 0; k < numTravelers; k++)
	{
		pthread_mutex_destroy(&travelerLocks[k]);
	}
	cleanGrid();
	exit(0);
}
