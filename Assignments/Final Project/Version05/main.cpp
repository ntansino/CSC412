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
pthread_mutex_t outLock;
vector<pthread_mutex_t> gridLocks;
vector<pthread_mutex_t> travelerLocks;

// VERSION 4 PART 9
unsigned int completedTravelers = 0;

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
	drawMessages(numMessages, message);
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
		pthread_mutex_init(&outLock, NULL);
		for (unsigned int i = 0; i < numTravelers; i++)
		{
			pthread_mutex_t this_traveler;
			pthread_mutex_init(&this_traveler, NULL);
			travelerLocks.push_back(this_traveler);
		}
		for (unsigned int i = 0; i < numRows * numCols; i++)
		{
			pthread_mutex_t thisSquare;
			pthread_mutex_init(&thisSquare, NULL);
			gridLocks.push_back(thisSquare);
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

	for (unsigned int k=0; k<numTravelers; k++)
	{
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
			if (traveler->segmentList.size() < 1 && traveler->searching == false)
			{
				traveler->draw = false;
				run = false;
			}
		}
		else
		{
			if (moveAttempts++ > 10)
			{
				traveler->searching = false;
				run = false;
			}
		}
		usleep(sleepTime);
	}

	if (moveAttempts <= 10)
	{
		numTravelersDone++;
	}


	return NULL;
}


bool Traveler::move(void)
{
	bool canMove = true;
	const TravelerSegment& head = getHead();
	const TravelerSegment& tail = getTail();

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
			if (segmentList.size() > 1)
			{

				unsigned int row = segmentList[segmentList.size()-1].row;
				unsigned int col = segmentList[segmentList.size()-1].col;

				pthread_mutex_lock(&gridLocks[(row * col) + col]);
				grid[row][col] = FREE_SQUARE;
				pthread_mutex_unlock(&gridLocks[(row * col) + col]);

			}

			segmentList.pop_back();
		}
		else
		{
			Direction randDir = freeSpaces[unsignedNumberGenerator(engine) % freeSpaces.size()];

			unsigned long lastIndex = segmentList.size()-1;
			// move the every trailing segment of a traveler
			if (!this->addTravelerSegment)
			{
				pthread_mutex_lock(&gridLocks[(tail.row * tail.col) + tail.col]);
				grid[tail.row][tail.col] = FREE_SQUARE;
				pthread_mutex_unlock(&gridLocks[(tail.row * tail.col) + tail.col]);
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

	pthread_mutex_lock(&gridLocks[(head.row * head.col) + head.col]);
	if (grid[head.row][head.col] == FREE_SQUARE)
	{
		grid[head.row][head.col] = TRAVELER;
	}
	else
	{
		// the traveler has reached the exitPos
		exitNotFound = false;
	}
	pthread_mutex_unlock(&gridLocks[(head.row * head.col) + head.col]);

	return exitNotFound;
}


vector<Direction> getFreeSpaces(const TravelerSegment& head)
{
	unsigned int row = head.row, col = head.col;
	Direction dir = head.dir;
	// get valid moves for the current position
	vector<Direction> moves;
	bool partMoved = false;
	unsigned int partitionIdx;

	if((row > 0) && (dir != SOUTH) && ((grid[row-1][col] == HORIZONTAL_PARTITION) || (grid[row-1][col] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row-1, col);
		if (partitionIdx < partitionList.size()) {
			partMoved = partitionList[partitionIdx].move(row-1, col);
		}
	}
	if((row < numRows-1) && (dir != NORTH) && ((grid[row+1][col] == HORIZONTAL_PARTITION) || (grid[row+1][col] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row+1, col);
		if (partitionIdx < partitionList.size()) {
			partMoved = partitionList[partitionIdx].move(row+1, col);
		}
	}
	if((col < numCols-1) && (dir != WEST) && ((grid[row][col+1] == HORIZONTAL_PARTITION) || (grid[row][col+1] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row, col+1);
		if (partitionIdx < partitionList.size()) {
			partMoved = partitionList[partitionIdx].move(row, col+1);
		}
	}
	if((col > 0) && (dir != EAST) && ((grid[row][col-1] == HORIZONTAL_PARTITION) || (grid[row][col-1] == VERTICAL_PARTITION)))
	{
		partitionIdx = getPartitionFromLocation(row, col-1);
		if (partitionIdx < partitionList.size()) {
			partMoved = partitionList[partitionIdx].move(row, col-1);
		}
	}

	#if DEBUG_OUTPUT == ON
		if (partMoved == true)
		{
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

unsigned int getPartitionFromLocation(unsigned int travelerRow, unsigned int travelerCol)
{
	for (unsigned int i = 0; i < partitionList.size(); i++)
	{
		for (unsigned int j = 0; j < partitionList[i].blockList.size(); j++)
		{
			if (travelerRow == partitionList[i].blockList[j].row && travelerCol == partitionList[i].blockList[j].col)
			{
				return i;
			}
		}
	}
	return -1;
}


bool SlidingPartition::move(unsigned int tRow, unsigned int tCol)
{

	pthread_mutex_lock(&outLock);

	bool moved = false;
	vector<pthread_mutex_t*> lockedGrids;

	// lock where the partition is
	for (unsigned int i = 0; i < this->blockList.size(); i++)
	{
		lockedGrids.push_back(&gridLocks[(this->blockList[i].row * numCols) + this->blockList[i].col]);
		pthread_mutex_lock(&gridLocks[(this->blockList[i].row * numCols) + this->blockList[i].col]);
	}

	if (this->isVertical)
	{
		// if the partition is vertical...

		unsigned int idx;
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].row == tRow && this->blockList[i].col == tCol)
			{
				idx = i;
			}
		}

		unsigned int spacesToMoveDown = idx + 1;
		unsigned int spacesToMoveUp = this->blockList.size() - idx;
		(void) spacesToMoveDown;

		// lock up and down
		// lock where the partition will move up
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].row - spacesToMoveUp < this->blockList[0].row && this->blockList[i].row - spacesToMoveUp >= 0 && this->blockList[i].row - spacesToMoveUp < numRows)
			{
				lockedGrids.push_back(&gridLocks[((this->blockList[i].row - spacesToMoveUp) * numCols) + tCol]);
				pthread_mutex_lock(&gridLocks[((this->blockList[i].row - spacesToMoveUp) * numCols) + tCol]);
			}
		}
		// lock where the partition will move down
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].row + spacesToMoveDown > this->blockList[this->blockList.size() - 1].row && this->blockList[i].row + spacesToMoveDown < numRows)
			{
				lockedGrids.push_back(&gridLocks[((this->blockList[i].row + spacesToMoveDown) * numCols) + tCol]);
				pthread_mutex_lock(&gridLocks[((this->blockList[i].row + spacesToMoveDown) * numCols) + tCol]);
			}
		}

		// check if it will move upward
		unsigned int topRow = this->blockList[0].row;
		unsigned int topCol = this->blockList[0].col;
		bool canMoveUp = true;
		for (unsigned int i = 1; canMoveUp && i <= spacesToMoveUp; i++)
		{
			if (topRow-i > numRows)
			{
				canMoveUp = false;
			}
			else if (topRow-i >= 0)
			{
				if (grid[topRow-i][topCol] != FREE_SQUARE)
				{
					canMoveUp = false;
				}
			}
		}

		// if it can move without conflict, move and return
		if (canMoveUp && spacesToMoveUp > 0)
		{
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				this->blockList[i].row = this->blockList[i].row - spacesToMoveUp;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = VERTICAL_PARTITION;
			}
			moved = true;
		}

		// check if it can move downward
		unsigned int bottomRow= this->blockList[this->blockList.size() - 1].row;
		bool canMoveDown = true;
		for (unsigned int i = 1; canMoveDown && i <= spacesToMoveDown; i++)
		{
			if (bottomRow+i >= numRows)
			{
				canMoveDown = false;
			} else if (bottomRow+i < numRows)
			{
				if (grid[bottomRow+i][tCol] != FREE_SQUARE)
				{
					canMoveDown = false;
				}
			}
		}

		// if it can move down without conflict, move and return
		if (canMoveDown && spacesToMoveDown > 0)
		{
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				this->blockList[i].row = this->blockList[i].row + spacesToMoveDown;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = VERTICAL_PARTITION;
			}
			moved = true;
		}

	} else {
		// if the partition is horizontal...

		unsigned int idx;
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].row == tRow && this->blockList[i].col == tCol)
			{
				idx = i;
			}
		}

		unsigned int spacesToMoveLeft = this->blockList.size() - idx;
		unsigned int spacesToMoveRight = idx + 1;

		// lock left and right
		// lock where the partition will move left
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].col - spacesToMoveLeft < this->blockList[0].col && this->blockList[i].col - spacesToMoveLeft >= 0 && this->blockList[i].col - spacesToMoveLeft < numCols)
			{
				lockedGrids.push_back(&gridLocks[(tRow * numCols) + (this->blockList[i].col - spacesToMoveLeft)]);
				pthread_mutex_lock(&gridLocks[(tRow * numCols) + (this->blockList[i].col - spacesToMoveLeft)]);
			}
		}
		// lock where the partition will move down
		for (unsigned int i = 0; i < this->blockList.size(); i++)
		{
			if (this->blockList[i].col + spacesToMoveRight > this->blockList[this->blockList.size() - 1].col && this->blockList[i].col + spacesToMoveRight < numCols)
			{
				lockedGrids.push_back(&gridLocks[(tRow * numCols) + (this->blockList[i].col + spacesToMoveRight)]);
				pthread_mutex_lock(&gridLocks[(tRow * numCols) + (this->blockList[i].col + spacesToMoveRight)]);
			}
		}

		// check if can move left
		unsigned int leftRow = this->blockList[0].row;
		unsigned int leftCol = this->blockList[0].col;
		bool canMoveLeft = true;
		for (unsigned int i = 1; canMoveLeft && i <= spacesToMoveLeft; i++)
		{
			if (leftCol-i > numRows)
			{
				canMoveLeft = false;
			}
			else if (leftCol-i >= 0)
			{
				if (grid[leftRow][leftCol-i] != FREE_SQUARE)
				{
					canMoveLeft = false;
				}
			}
		}

		// if can move left, move and return
		if (canMoveLeft && spacesToMoveLeft > 0)
		{
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				this->blockList[i].col = this->blockList[i].col - spacesToMoveLeft;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = HORIZONTAL_PARTITION;
			}
			moved = true;
		}

		// check if can move right
		unsigned int rightCol = this->blockList[this->blockList.size() - 1].col;
		bool canMoveRight = true;
		for (unsigned int i = 1; canMoveRight && i <= spacesToMoveRight; i++)
		{
			if (rightCol+i >= numCols)
			{
				canMoveRight = false;
			} else if (rightCol+i < numCols)
			{
				if (grid[tRow][rightCol+i] != FREE_SQUARE)
				{
					canMoveRight = false;
				}
			}
		}

		// if it can move right without conflit, move and return
		if (canMoveRight && spacesToMoveRight > 0)
		{
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = FREE_SQUARE;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				this->blockList[i].col = this->blockList[i].col + spacesToMoveRight;
			}
			for (unsigned int i = 0; i < this->blockList.size(); i++)
			{
				grid[this->blockList[i].row][this->blockList[i].col] = HORIZONTAL_PARTITION;
			}
			moved = true;
		}

	}

	for (unsigned int i = 0; i < lockedGrids.size(); i++)
	{
		pthread_mutex_unlock(lockedGrids[i]);
	}
	// delete[] lockedGrids;

	pthread_mutex_unlock(&outLock);

	return moved;
}

void cleanGrid(void)
{
	for (unsigned int i=0; i< numRows; i++)
		delete []grid[i];
	delete []grid;
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		delete []message[k];
	delete []message;
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
		travelerList[k].searching = false;
	}
	joinThreads();
	for (unsigned int k = 0; k < numTravelers; k++)
	{
		pthread_mutex_destroy(&travelerLocks[k]);
	}
	cleanGrid();
	exit(0);
}
