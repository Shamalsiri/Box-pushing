//  John Stevenson
//  Shamal Siriwardana
//  Version: 2
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2018-12-05
//  Finished by John Stevenson and Shamal Siriwardana
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zconf.h>
#include <pthread.h>
//
#include "gl_frontEnd.h"
/**
 * Debugging print statement switches
 */
#define DEBUG_START 1
#define DEBUG_GRID 0
#define DEBUG_MOVE 0
/**
 * Boolean State values for robots
 */
#define TRUE 1
#define FALSE 0
/**
 * Constants for grid representation
 */
#define EMPTY 0
#define ROBOT 1
#define BOX 2
/**
 * Constants for movement clarification
 */
#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3
#define IN_PLACE 4

struct pushData {
    //number of spaces the box needs to travel still
    int numSpaces;
    //direction the box needs to travel in
    int direction;
    //the box/robot we are referring to
    int boxId;
    //the side of the robot that the box is against
    int boxSide;
};

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);

void displayStatePane(void);

void initializeApplication(void);

void move(struct pushData *);

void printGrid();

struct pushData *compBotnBox(int);

void push(struct pushData *);

struct pushData *compBoxnDoor(int);

void* startRobot(void*);

void end(int);

//==================================================================================
//	Application-level global variables
//==================================================================================


//	Don't touch
extern const int GRID_PANE, STATE_PANE;
extern int gMainWindow, gSubwindow[2];


//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
int **grid;
int numRows = -1;    //	height of the grid
int numCols = -1;    //	width
int numBoxes = -1;    //	also the number of robots
int numDoors = -1;    //	The number of doors.

/**
 * Global array holding robot location data
 */
int **robotLoc;
/**
 * Global array holding box location data
 */
int **boxLoc;
/**
 * Global array holding door assignments for robot indexes
 */
int *doorAssign;
/**
 * Global array holding door location data
 */
int **doorLoc;

/**
 * Global array to signal the robots are finished
 */
int *isDone;

/**
 * the number of live robot threads
 */
int numLiveThreads = 0;

//	robot sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int robotSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char **message;

//File that we output to
char *outputFile = "output.txt";

//file mutex lock
pthread_mutex_t fileLock;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

/**
 * Method that starts and runs a robot
 */
void* startRobot(void* botNum) {
    /**
     * Movement of the robot
     */
        //cast the incoming data
        int robotNum = *(int*)botNum;
        if(DEBUG_START)
        printf("Robot %d started.\n",robotNum);
        //initialize a data structure we can read the results of and assign
        //it as the return of our first initialize call
        while (isDone[robotNum] == FALSE) {
            struct pushData *tempData = compBotnBox(robotNum);
            //if the robot is not in place
            if (tempData->direction != IN_PLACE) {
                //we set the array to not done
                isDone[robotNum] = FALSE;
                //move based on our function call
                move(tempData);
                //free the function call data
                free(tempData);
            } else {
                //if the robot is in place but we aren't done yet
                if (isDone[robotNum] == FALSE) {
                    //call the method that gets our push information
                    tempData = compBoxnDoor(robotNum);
                    //push that information
                    push(tempData);
                    //if we are in place again
                    if (tempData->direction == IN_PLACE) {
                        end(robotNum);
                    }
                }
                //free function call data
                free(tempData);
            }
            usleep(robotSleepTime);
        }
        free(botNum);
        pthread_exit(&botNum);
}
/**
 * Clears the robot from the board when its tasks are complete
 * @param robotNum - robot number to clear
 */
void end(int robotNum){
    //we set our done state to true
    isDone[robotNum] = TRUE;
    //update the grid locations to have the locations removed
    grid[robotLoc[robotNum][0]][robotLoc[robotNum][1]] = EMPTY;
    grid[boxLoc[robotNum][0]][boxLoc[robotNum][1]] = EMPTY;
}

void displayGridPane(void) {
    //	This is OpenGL/glut magic.  Don't touch
    glutSetWindow(gSubwindow[GRID_PANE]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (int i = 0; i < numBoxes; i++) {
        //	here I would test if the robot thread is still live
        if (isDone[i] == FALSE) {
            drawRobotAndBox(i, robotLoc[i][1], robotLoc[i][0], boxLoc[i][1], boxLoc[i][0], doorAssign[i]);
        }
    }

    for (int i = 0; i < numDoors; i++) {
        //	here I would test if the robot thread is still alive
        drawDoor(i, doorLoc[i][1], doorLoc[i][0]);
    }

    //	This call does nothing important. It only draws lines
    //	There is nothing to synchronize here
    drawGrid();

    //	This is OpenGL/glut magic.  Don't touch
    glutSwapBuffers();

    glutSetWindow(gMainWindow);
}


void displayStatePane(void) {

    //	This is OpenGL/glut magic.  Don't touch
    glutSetWindow(gSubwindow[STATE_PANE]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //	Here I hard-code a few messages that I want to see displayed
    //	in my state pane.  The number of live robot threads will
    //	always get displayed.  No need to pass a message about it.
    int numMessages = 3;
    int counter = 0;
    for(int i = 0; i< numBoxes;i++){
        if(isDone[i] == TRUE){
            counter++;
        }
    }
    numLiveThreads = numBoxes-counter;
    sprintf(message[0], "Doors: %d", numDoors);
    sprintf(message[1], "Active Robots:  %d",numBoxes-counter);
    sprintf(message[2], "Robots Finished: %d", counter);

    //---------------------------------------------------------
    //	This is the call that makes OpenGL render information
    //	about the state of the simulation.
    //
    //	You *must* synchronize this call.
    //
    //---------------------------------------------------------
    drawState(numMessages, message);


    //	This is OpenGL/glut magic.  Don't touch
    glutSwapBuffers();

    glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupRobots(void) {
    //	decrease sleep time by 20%, but don't get too small
    int newSleepTime = (8 * robotSleepTime) / 10;

    if (newSleepTime > MIN_SLEEP_TIME) {
        robotSleepTime = newSleepTime;
    }
}

void slowdownRobots(void) {
    //	increase sleep time by 20%
    robotSleepTime = (12 * robotSleepTime) / 10;
}

int main(int argc, char **argv) {
    if (argc == 5) {
        numCols = atoi(argv[1]);
        numRows = atoi(argv[2]);
        numBoxes = atoi(argv[3]);
        numDoors = atoi(argv[4]);
        if ((numBoxes <= 0) || (numRows <= 0) || (numCols <= 0)) {
            printf("Illegal argument.\n");
            exit(4);
        }
        if (numDoors < 1 || numDoors > 3) {
            printf("Illegal argument. Number of doors must be between 1 and 3(inclusive)");
            exit(4);
        }
    } else {
        printf("Invalid argument count.\nProper Usage: ./robotsV1  <width>  <height>  <# of boxes/robots>  <# of doors>\n");
        exit(4);
    }


    //	Even though we extracted the relevant information from the argument
    //	list, I still need to pass argc and argv to the front-end init
    //	function because that function passes them to glutInit, the required call
    //	to the initialization of the glut library.
    initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);

    //	Now we can do application-level initialization
    initializeApplication();

    //	Now we enter the main loop of the program and to a large extend
    //	"lose control" over its execution.  The callback functions that
    //	we set up earlier will be called when the corresponding event
    //	occurs
    glutMainLoop();

    //	Free allocated resource before leaving (not absolutely needed, but
    //	just nicer.  Also, if you crash there, you know something is wrong
    //	in your code.
    for (int i = 0; i < numRows; i++)
        free(grid[i]);
    free(grid);
    for (int k = 0; k < MAX_NUM_MESSAGES; k++)
        free(message[k]);
    free(message);


    //	This will probably never be executed (the exit point will be in one of the
    //	call back functions).
    return 0;
}

/**
 * iterates through the available checkList that holds all the coordinates
 * for the system.  It checks these coordinates against a row/column combination
 * and returns 0 if there is no match and 1 if there is a match
 * @param row - the row coordinate we are checking for
 * @param column - the column coordinate we are checking for
 * @param checkList - the list of already allocated grid spaces
 * @param checkIndex - the length of the list so far, since this will be called with
 * 						an incomplete checkList
 * @return - 1 if match is found and 0 if no match is found
 */
int checkForDupe(int row, int column, int **checkList, int checkIndex) {
    for (int i = 0; i < checkIndex; i++) {
        if (checkList[i][0] == row && checkList[i][1] == column) {
            return 1;
        }
    }
    return 0;
}

/**
 * prints out the state grid for debugging purposes
 */
void printGrid() {
    if (DEBUG_GRID) {
        for (int i = numRows - 1; i >= 0; i--) {
            for (int j = 0; j < numCols; j++) {
                printf("%d ", grid[j][i]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

/**
 * Moves the selected robot in the desired direction if the space is empty
 * If the space is not empty, we do nothing and hope that the next pass
 * will be more fruitful
 * @param data - struct holding the robot we are moving and the direction we
 *                  are moving it
 */
void move(struct pushData *data) {
    /**
     * Local copies of struct data for ease of referencing and reading
     */

    //Robot and box id number
    int myRobot = data->boxId;

    pthread_mutex_lock(&fileLock);
    FILE *output = fopen(outputFile,"a");
    //row and column of robot that is moving
    int row, col;
    float itemLoc[2] = {robotLoc[myRobot][0], robotLoc[myRobot][1]};
    row = itemLoc[1];
    col = itemLoc[0];

    //Based on the direction data we move the robot in the direction we want
    //Updating the grid along the way
    switch (data->direction) {
        //Moving North
        case NORTH:
            if (DEBUG_MOVE)
                printf("North is free for robot %d. Moving North to (%d,%d).\n", data->boxId, col, row + 1);
            grid[col][(row + 1)] = ROBOT;
            robotLoc[myRobot][0] = col;
            robotLoc[myRobot][1] = row + 1;
            grid[col][row] = EMPTY;
            fprintf(output,"robot %d move N\n",myRobot);
            break;
            //Moving South
        case SOUTH:
            if (DEBUG_MOVE)
                printf("South is free for robot %d. Moving South to (%d,%d).\n", data->boxId, col, row - 1);
            grid[col][(row - 1)] = ROBOT;
            robotLoc[myRobot][0] = col;
            robotLoc[myRobot][1] = row - 1;
            grid[col][row] = EMPTY;
            fprintf(output,"robot %d move S\n",myRobot);
            break;
            //Moving East
        case EAST:
            if (DEBUG_MOVE)
                printf("East is free for robot %d. Moving East to (%d,%d).\n", data->boxId, col + 1, row);
            grid[col + 1][row] = ROBOT;
            robotLoc[myRobot][0] = col + 1;
            robotLoc[myRobot][1] = row;
            grid[col][row] = EMPTY;
            fprintf(output,"robot %d move E\n",myRobot);
            break;
            //Moving West
        case WEST:
            if (DEBUG_MOVE)
                printf("West is free for robot %d. Moving West to (%d,%d).\n", data->boxId, col - 1, row);
            grid[col - 1][row] = ROBOT;
            robotLoc[myRobot][0] = col - 1;
            robotLoc[myRobot][1] = row;
            grid[col][row] = EMPTY;
            fprintf(output,"robot %d move W\n",myRobot);
            break;
        default:
            //If we have something other than our expected values we print an error and exit out
            printf("Error in movement Integer.");
            exit(2);
            break;
    }

    //close the file
    fclose(output);

    pthread_mutex_unlock(&fileLock);
    //if the debug_grid switch is set to 1, this will print
    printGrid();
    return;
}

/**
 * Pushes the box of a specific robot in a specific direction
 * @param data - pushData struct with box/robot number, direction of the push, and location of box relative to robot
 */
void push(struct pushData *data) {
    //local copies of struct values for convenience and ease od reading
    int myRobot = data->boxId;
    int myDirection = data->direction;
    int myBoxSide = data->boxSide;
    float botLoc[2] = {robotLoc[myRobot][0], robotLoc[myRobot][1]};



    //struct to handle reposition calls to move
    struct pushData *rotateData = (struct pushData *) calloc(1, sizeof(struct pushData));
    rotateData->boxId = myRobot;
    rotateData->numSpaces = 1;

    //handle the different cases
    //critical areas
    int col = robotLoc[myRobot][0];
    int row = robotLoc[myRobot][1];
    switch (myDirection) {
        case NORTH:
            switch (myBoxSide) {
                //if we need to move north, we rotate appropriately for the case we have and then push
                case NORTH:
                    //and the box is on our north side
                    grid[col][row] = EMPTY;
                    grid[col][row + 2] = BOX;
                    grid[col][row + 1] = ROBOT;
                    robotLoc[myRobot][1] = row + 1;
                    boxLoc[myRobot][1] = row + 2;
                    //lock the file to write to and open it
                    FILE *output = fopen(outputFile,"a");
                    pthread_mutex_lock(&fileLock);
                    fprintf(output,"robot %d push N\n",myRobot);
                    fclose(output);
                    pthread_mutex_unlock(&fileLock);
                    break;
                case SOUTH:
                    //the box is on our south side
                    rotateData->direction = WEST;
                    move(rotateData);
                    rotateData->direction = SOUTH;
                    move(rotateData);
                    data->boxSide = WEST;
                    push(data);
                    break;

                case EAST:
                    //the box is on our east side
                    rotateData->direction = SOUTH;
                    move(rotateData);
                    rotateData->direction = EAST;
                    move(rotateData);
                    data->boxSide = NORTH;
                    push(data);
                    break;
                case WEST:
                    //the box is on our west side
                    rotateData->direction = SOUTH;
                    move(rotateData);
                    rotateData->direction = WEST;
                    move(rotateData);
                    data->boxSide = NORTH;
                    push(data);
                    break;
            }
            break;
        case SOUTH:
            //if we need to move south, we rotate appropriately for the case we have and then push
            switch (myBoxSide) {
                case NORTH:
                    //and the box is on our north side
                    rotateData->direction = WEST;
                    move(rotateData);
                    rotateData->direction = NORTH;
                    move(rotateData);
                    data->boxSide = EAST;
                    push(data);
                    break;
                case SOUTH:
                    //and the box is on our south side
                    grid[col][row] = EMPTY;
                    grid[col][row - 1] = ROBOT;
                    grid[col][row - 2] = BOX;
                    robotLoc[myRobot][1] = row - 1;
                    boxLoc[myRobot][1] = row - 2;
                    //lock the file to write to and open it
                    FILE *output = fopen(outputFile,"a");
                    pthread_mutex_lock(&fileLock);
                    fprintf(output,"robot %d push S\n",myRobot);
                    fclose(output);
                    pthread_mutex_unlock(&fileLock);
                    break;
                case EAST:
                    //and the box is on our east side
                    rotateData->direction = NORTH;
                    move(rotateData);
                    rotateData->direction = EAST;
                    move(rotateData);
                    data->boxSide = SOUTH;
                    push(data);
                    break;
                case WEST:
                    //and the box is on our west side
                    rotateData->direction = NORTH;
                    move(rotateData);
                    rotateData->direction = WEST;
                    move(rotateData);
                    data->boxSide = SOUTH;
                    push(data);
                    break;
            }
            break;
        case EAST:
            //if we need to move east, we rotate appropriately for the case we have and then push
            switch (myBoxSide) {
                case NORTH:
                    //and the box is on our north side
                    rotateData->direction = WEST;
                    move(rotateData);
                    rotateData->direction = NORTH;
                    move(rotateData);
                    data->boxSide = EAST;
                    push(data);
                    break;
                case SOUTH:
                    //and the box is on our south side
                    rotateData->direction = WEST;
                    move(rotateData);
                    rotateData->direction = SOUTH;
                    move(rotateData);
                    data->boxSide = EAST;
                    push(data);
                    break;
                case EAST:
                    //and the box is on our east side
                    grid[col][row] = EMPTY;
                    grid[col + 2][row] = BOX;
                    grid[col + 1][row] = ROBOT;
                    robotLoc[myRobot][0] = col + 1;
                    boxLoc[myRobot][0] = col + 2;
                    //lock the file to write to and open it
                    FILE *output = fopen(outputFile,"a");
                    pthread_mutex_lock(&fileLock);
                    fprintf(output,"robot %d push E\n",myRobot);
                    fclose(output);
                    pthread_mutex_unlock(&fileLock);
                    break;
                case WEST:
                    //and the box is on our west side
                    rotateData->direction = NORTH;
                    move(rotateData);
                    rotateData->direction = WEST;
                    move(rotateData);
                    data->boxSide = SOUTH;
                    push(data);
                    break;

            }
            break;
        case WEST:
            //if we need to move west, we rotate appropriately for the case we have and then push
            switch (myBoxSide) {
                case NORTH:
                    //and the box is on our north side
                    rotateData->direction = EAST;
                    move(rotateData);
                    rotateData->direction = NORTH;
                    move(rotateData);
                    data->boxSide = WEST;
                    push(data);
                    break;
                case SOUTH:
                    //and the box is on our south side
                    rotateData->direction = EAST;
                    move(rotateData);
                    rotateData->direction = SOUTH;
                    move(rotateData);
                    data->boxSide = WEST;
                    push(data);
                    break;
                case EAST:
                    //and the box is on our east side
                    rotateData->direction = NORTH;
                    move(rotateData);
                    rotateData->direction = EAST;
                    move(rotateData);
                    data->boxSide = SOUTH;
                    push(data);
                    break;
                case WEST:
                    //and the box is on our west side
                    grid[col][row] = EMPTY;
                    grid[col - 2][row] = BOX;
                    grid[col - 1][row] = ROBOT;
                    robotLoc[myRobot][0] = col - 1;
                    boxLoc[myRobot][0] = col - 2;
                    //lock the file to write to and open it
                    FILE *output = fopen(outputFile,"a");
                    pthread_mutex_lock(&fileLock);
                    fprintf(output,"robot %d push W\n",myRobot);
                    fclose(output);
                    pthread_mutex_unlock(&fileLock);
                    break;
            }
            break;
        case IN_PLACE:
            //if it is already in place, we do nothing
            break;
        default:
            //if we don't recognize the movement command
            printf("Error in movement Integer.");
            exit(2);
            break;
    }

    free(rotateData);
    //if the debug_grid switch is set to 1, this will print the state grid
    printGrid();
    return;
}

/**
 * Reads in from the input file the number of rows and columns,
 * the number of boxes(and therefore robots) and the number of doors.
 * It then randomly places those onto the state grid and into the
 * appropriate arrays, which assigns robots to boxes and boxes to doors.
 * The function also writes all the initial states into a file output.txt
 */
void initializeApplication(void) {
    //allocate the memory for the array showing if the bots are done
    isDone = (int *) calloc(numBoxes, sizeof(int));

    //initialize the file lock
    pthread_mutex_init(&fileLock,NULL);

    //Set up the file we output to
    FILE *output = fopen(outputFile, "w");

    //check for an error
    if (output == NULL) {
        printf("Problem writing to the file.\n");
        exit(4);
    }

    //output parameters to the file
    fprintf(output, "%d x %d Grid (Col x Row) with %d Boxes and %d Doors",
            numCols, numRows, numBoxes, numDoors);
    fprintf(output, "\n");

    //	Allocate the grid
    grid = (int **) calloc((size_t) numCols, sizeof(int *));
    for (int i = 0; i < numCols; i++)
        grid[i] = (int *) calloc((size_t) numRows, sizeof(int));
    message = (char **) malloc(MAX_NUM_MESSAGES * sizeof(char *));
    for (int k = 0; k < MAX_NUM_MESSAGES; k++)
        message[k] = (char *) malloc((MAX_LENGTH_MESSAGE + 1) * sizeof(char));

    //	seed the pseudo-random generator
    srand((unsigned int) time(NULL));

    //allocate memory for our global arrays
    robotLoc = (int **) calloc((size_t) numBoxes, sizeof(int *));
    boxLoc = (int **) calloc((size_t) numBoxes, sizeof(int *));
    doorAssign = (int *) calloc((size_t) numBoxes, sizeof(int));
    doorLoc = (int **) calloc((size_t) numDoors, sizeof(int *));

    //allocate memory for the box and robot location lists
    for (int i = 0; i < numBoxes; i++) {
        robotLoc[i] = (int *) calloc(2, sizeof(int));
        boxLoc[i] = (int *) calloc(2, sizeof(int));
    }
    //allocate memory for the door location list
    for (int i = 0; i < numDoors; i++) {
        doorLoc[i] = (int *) calloc(2, sizeof(int));
    }

    //temporary variables to hold our random numbers we generate
    int randRow;
    int randCol;
    int randDoor;

    //we use this array to make sure that we don't have duplicate locations
    int **check = (int **) calloc((size_t) numDoors + (numBoxes * 2), sizeof(int *));
    for (int i = 0; i < numDoors + (numBoxes * 2); i++) {
        check[i] = (int *) calloc(2, sizeof(int));
    }

    //we use this index as a limit for our check array so we are only checking
    //legitimate coordinates
    int checkIndex = 0;
    fprintf(output, "\n");
    //assign each box to a spot in the grid
    for (int i = 0; i < numBoxes; i++) {
        //generate new random numbers
        randRow = rand() % numRows;
        randCol = rand() % numCols;

        //make sure the box isn't on the edge of the grid
        if (randRow == numRows - 1) {
            randRow--;
        } else if (randRow == 0) {
            randRow++;
        }
        if (randCol == numCols - 1) {
            randCol--;
        } else if (randCol == 0) {
            randCol++;
        }

        //box location
        boxLoc[i][1] = randRow;
        boxLoc[i][0] = randCol;

        //check for duplicate entries
        while (checkForDupe(boxLoc[i][0], boxLoc[i][1], check, checkIndex) == 1) {
            //if we need to reroll, we reseed the generator
            srand((unsigned int) time(NULL));
            randRow = rand() % numRows;
            randCol = rand() % numCols;
            //make sure the box isn't on the edge of the grid
            if (randRow == numRows - 1) {
                randRow--;
            } else if (randRow == 0) {
                randRow++;
            }
            if (randCol == numCols - 1) {
                randCol--;
            } else if (randCol == 0) {
                randCol++;
            }
            boxLoc[i][1] = randRow;
            boxLoc[i][0] = randCol;
        }
        //assign to check list and increment counter
        check[checkIndex][1] = boxLoc[i][1];
        check[checkIndex][0] = boxLoc[i][0];
        checkIndex++;

    }
    //assign each robot an initial location
    for (int i = 0; i < numBoxes; i++) {
        robotLoc[i][1] = rand() % numRows;
        robotLoc[i][0] = rand() % numCols;

        //check for duplicate entries
        while (checkForDupe(robotLoc[i][0], robotLoc[i][1], check, checkIndex) == 1) {
            //reseed random for rerolls
            srand((unsigned int) time(NULL));
            robotLoc[i][1] = rand() % numRows;
            robotLoc[i][0] = rand() % numCols;
        }
        //assign the entry to the check list
        check[checkIndex][1] = robotLoc[i][1];
        check[checkIndex][0] = robotLoc[i][0];

        //increment our length counter
        checkIndex++;
    }


    //assign each door a location and a box to be associated
    //with it
    for (int i = 0; i < numDoors; i++) {
        //door location
        doorLoc[i][1] = rand() % numRows;
        doorLoc[i][0] = rand() % numCols;
        //check for duplicate entries
        while (checkForDupe(doorLoc[i][0], doorLoc[i][1], check, checkIndex) == 1) {
            //reseed random for rerolls
            srand((unsigned int) time(NULL));
            doorLoc[i][1] = rand() % numRows;
            doorLoc[i][0] = rand() % numCols;
        }
        //print to the file the location of the door
        fprintf(output, "Door %d Location: (%d, %d)\n", i, doorLoc[i][0], doorLoc[i][1]);
        //add location to check list
        check[checkIndex][1] = doorLoc[i][0];
        check[checkIndex][0] = doorLoc[i][1];
        //increment the counter
        checkIndex++;
        //pick a random robot/box pair for the door
        randDoor = rand() % numDoors;
        //assign the box associated with this door
        doorAssign[i] = randDoor;
    }
    fprintf(output, "\n");
    //for each box, print its location in the output file
    for (int i = 0; i < numBoxes; i++) {
        fprintf(output, "Box %d Initial Location: (%d, %d)\n", i, boxLoc[i][0], boxLoc[i][1]);
    }
    fprintf(output, "\n");
    //cycle through robot array to output the initial locations in the right order
    for (int i = 0; i < numBoxes; i++) {
        fprintf(output, "Robot %d Initial Location: (%d, %d) with a goal of Door %d\n",
                i, robotLoc[i][0], robotLoc[i][1], doorAssign[i]);
    }
    fprintf(output,"\n");
    fclose(output);
    //once the coordinates are initialized, we can free our check array
    for (int i = 0; i < numDoors + (numBoxes * 2); i++) {
        free(check[i]);
    }
    //Populate the grid with our values
    for (int i = 0; i < numBoxes; i++) {
        int r, c;
        r = robotLoc[i][1];
        c = robotLoc[i][0];
        grid[c][r] = ROBOT;
        r = boxLoc[i][1];
        c = boxLoc[i][0];
        grid[c][r] = BOX;
    }

    printGrid();
    //run the robots
    //initialize pthread id's and attr
    pthread_t botThread[numBoxes];
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    for(int i = 0;i < numBoxes; i++){
        //allocate space for the data we pass(robot number)
        int* data = calloc(1,sizeof(int*));
        *data = i;
        //create the thread and free the allocated data inside the thread when it finishes
        pthread_create(&botThread[i],&attr,startRobot,data);
        //detach the thread
        pthread_detach(botThread[i]);
    }
    return;
}
/** Compare the location of the box and it's respective robot's location
 * Calculate the step the robot needs to take to move toward the box
 * @param botNum   Bot number respective to the box
 * @ret the move data in a struct
 */
struct pushData *compBotnBox(int botNum) {
    //Allocating memory for the data struct we are passing
    struct pushData *pushInfo = (struct pushData *) malloc(sizeof(struct pushData));
    pushInfo->boxId = botNum;
    if (boxLoc[botNum][0] == robotLoc[botNum][0]) { // If Xs are the same
        if (boxLoc[botNum][1] == robotLoc[botNum][1]) { // if the Ys are the same
            pushInfo->direction = IN_PLACE; // Robot in positon
            pushInfo->numSpaces = 0;
        } else if (robotLoc[botNum][1] < boxLoc[botNum][1]) { // robot is below the box
            pushInfo->direction = NORTH; // Move north
            pushInfo->numSpaces = boxLoc[botNum][1] - robotLoc[botNum][1]; // num spaces need to move
        } else {
            pushInfo->direction = SOUTH; // Move south
            pushInfo->numSpaces = robotLoc[botNum][1] - boxLoc[botNum][1]; // num spaces need to move
        }
    } else if (boxLoc[botNum][1] == robotLoc[botNum][1]) { // If the Ys are the same
        if (boxLoc[botNum][0] == robotLoc[botNum][0]) { // If Xs are the same
            pushInfo->direction = IN_PLACE; // Robot in positon
            pushInfo->numSpaces = 0;
        } else if (boxLoc[botNum][0] < robotLoc[botNum][0]) { // The box is to the left of the bot
            pushInfo->direction = WEST; // Move west
            pushInfo->numSpaces = robotLoc[botNum][0] - boxLoc[botNum][0]; // num spaces need to move
        } else { // The box is to the right of the bot
            pushInfo->direction = EAST; // move east
            pushInfo->numSpaces = boxLoc[botNum][0] - robotLoc[botNum][0]; // num spaces need to move
        }
    } else if (boxLoc[botNum][0] < robotLoc[botNum][0]) { // The box is to the left of the bot
        pushInfo->direction = WEST; // Move west
        pushInfo->numSpaces = robotLoc[botNum][0] - boxLoc[botNum][0]; // num spaces need to move
    } else if (boxLoc[botNum][0] > robotLoc[botNum][0]) { // The box is to the right of the bot
        pushInfo->direction = EAST; // move east
        pushInfo->numSpaces = boxLoc[botNum][0] - robotLoc[botNum][0]; // num spaces need to move
    }
    if ((robotLoc[botNum][0] == boxLoc[botNum][0] && abs(robotLoc[botNum][1] - boxLoc[botNum][1]) == 1) ||
        (robotLoc[botNum][1] == boxLoc[botNum][1] && abs(robotLoc[botNum][0] - boxLoc[botNum][0]) == 1)) {
        pushInfo->direction = IN_PLACE; // Box is in place
    }
    return pushInfo;
}

/** Compare the box location and it's door location
 * calculate the number of steps and the direction the box needs to move
 * @param  boxNUm   Box identifier
 * @ret the data in the struct
 */
struct pushData *compBoxnDoor(int boxNum) {
    //Allocating memory for the data struct we are passing
    struct pushData *pushInfo = (struct pushData *) calloc(1, sizeof(struct pushData));
    pushInfo->boxId = boxNum;
    int doorId = doorAssign[boxNum];

    if (boxLoc[boxNum][0] == doorLoc[doorId][0]) // if x is the same
    {
        if (boxLoc[boxNum][1] == doorLoc[doorId][1]) // if y is the same
        {
            pushInfo->direction = IN_PLACE; // A == 4
            pushInfo->numSpaces = 0;
        } else if (boxLoc[boxNum][1] < doorLoc[doorId][1]) // if box is directly below the door
        {
            pushInfo->direction = NORTH; // N == 0
            pushInfo->numSpaces = doorLoc[doorId][1] - boxLoc[boxNum][1];

        } else {
            pushInfo->direction = SOUTH; // S == 1
            pushInfo->numSpaces = boxLoc[boxNum][1] - doorLoc[doorId][1];
        }
    } else if (boxLoc[boxNum][1] == doorLoc[doorId][1]) // if y is the same
    {
        if (boxLoc[boxNum][0] == doorLoc[doorId][0]) // not sure if redundant
        {
            pushInfo->direction = IN_PLACE;
            pushInfo->numSpaces = 0;
        } else if (boxLoc[boxNum][0] < doorLoc[doorId][0]) {
            pushInfo->direction = EAST; // E == 2
            pushInfo->numSpaces = doorLoc[doorId][0] - boxLoc[boxNum][0];
        } else {
            pushInfo->direction = WEST; // W == 3
            pushInfo->numSpaces = boxLoc[boxNum][0] - doorLoc[doorId][0];
        }
    } else if (boxLoc[boxNum][0] < doorLoc[doorId][0]) {
        pushInfo->direction = EAST;
        pushInfo->numSpaces = doorLoc[doorId][0] - boxLoc[boxNum][0];
    } else if (boxLoc[boxNum][0] > doorLoc[doorId][0]) {
        pushInfo->direction = WEST;
        pushInfo->numSpaces = boxLoc[boxNum][0] - doorLoc[doorId][0];
    }

    if (boxLoc[boxNum][0] == robotLoc[boxNum][0]) {
        if (boxLoc[boxNum][1] > robotLoc[boxNum][1]) {
            pushInfo->boxSide = NORTH; // underneath the box
        } else if (boxLoc[boxNum][1] < robotLoc[boxNum][1]) {
            pushInfo->boxSide = SOUTH; //  above the box
        }
    } else if (boxLoc[boxNum][1] == robotLoc[boxNum][1]) {
        if (boxLoc[boxNum][0] < robotLoc[boxNum][0]) {
            pushInfo->boxSide = WEST; //to the right of the box
        } else if (boxLoc[boxNum][0] > robotLoc[boxNum][0]) {
            pushInfo->boxSide = EAST; // to the left of the box
        }
    }

    return pushInfo;
}
