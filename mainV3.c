//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2018-12-05
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zconf.h>
//
#include "gl_frontEnd.h"
/**
 * Debugging print statement switches
 */
#define DEBUG_GRID 1
#define DEBUG_MOVE 1
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

struct pushData *initializeBot(int);

void push(struct pushData *);

struct pushData *compBoxnDoor(int);

void updateRobots();

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
FILE *outputFile;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

/**
 * Method that updates the location of the robots individually, one iteration
 */
void updateRobots(){
    /**
     * Movement of the robots, one at a time
     */
    usleep(robotSleepTime);
    //for every robot
    for (int i = 0; i < numBoxes; i++) {
        //initialize a data structure we can read the results of and assign
        //it as the return of our first initialize call
        if(isDone[i] == FALSE) {
            struct pushData *tempData = initializeBot(i);
            //if the robot is not in place
            if (tempData->direction != IN_PLACE) {
                //we set the array to not done
                isDone[i] = FALSE;
                //move based on our function call
                move(tempData);
                //free the function call data
                free(tempData);
            } else {
                //if the robot is in place but we aren't done yet
                if (isDone[i] == FALSE) {
                    //call the method that gets our push information
                    tempData = compBoxnDoor(i);
                    //push that information
                    push(tempData);
                    //if we are in place again
                    if (tempData->direction == IN_PLACE) {
                        //we set our done state to true
                        isDone[i] = TRUE;
                        //update the grid locations to have the locations removed
                        grid[robotLoc[i][0]][robotLoc[i][1]] = EMPTY;
                        grid[boxLoc[i][0]][boxLoc[i][1]] = EMPTY;
                    }
                }
                //free function call data
                free(tempData);
            }
        }
    }
    return;
}

void displayGridPane(void) {
    //	This is OpenGL/glut magic.  Don't touch
    glutSetWindow(gSubwindow[GRID_PANE]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //-----------------------------
    //	CHANGE THIS
    //-----------------------------
    //	Here I hard-code myself some data for robots and doors.  Obviously this code
    //	this code must go away.  I just want to show you how to display the information
    //	about a robot-box pair or a door.

    //update our robot states
    updateRobots();

    for (int i = 0; i < numBoxes; i++) {
        //	here I would test if the robot thread is still live
        if (isDone[i] == 0) {
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
    int numMessages = numBoxes;
    for(int i = 0; i < numBoxes;i++){

    }

    sprintf(message[0], "We have %d doors", numDoors);
    sprintf(message[1], "I like cheese");
    sprintf(message[2], "System time is %ld", time(NULL));

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


//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	the initialization of numRows, numCos, numDoors, numBoxes.
//------------------------------------------------------------------------
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
            //Critical area
            //Check if the area north is free, if so we move, if not we do nothing
            if (row < numRows - 1 && grid[col][(row + 1)] == EMPTY) {
                if (DEBUG_MOVE)
                    printf("North is free for robot %d. Moving North to (%d,%d).\n", data->boxId, col, row + 1);
                grid[col][(row + 1)] = ROBOT;
                robotLoc[myRobot][0] = col;
                robotLoc[myRobot][1] = row + 1;
                grid[col][row] = EMPTY;
            } else {
                if (DEBUG_MOVE)
                    printf("North is blocked for robot %d trying coordinates (%d,%d).\n", data->boxId,
                           col,
                           (row + 1));
            }
            break;
            //Moving South
        case SOUTH:
            //Check if the area south is free, if so we move, if not we do nothing
            //Critical Area
            if (row > 0 && grid[col][row - 1] == EMPTY) {
                if (DEBUG_MOVE)
                    printf("South is free for robot %d. Moving South to (%d,%d).\n", data->boxId, col, row - 1);
                grid[col][(row - 1)] = ROBOT;
                robotLoc[myRobot][0] = col;
                robotLoc[myRobot][1] = row - 1;
                grid[col][row] = EMPTY;
            } else {
                if (DEBUG_MOVE)
                    printf("South is blocked for robot %d trying coordinates (%d,%d).\n", data->boxId, col, (row - 1));
            }
            break;
            //Moving East
        case EAST:
            //Check if the area east is free, if so we move, if not we do nothing
            //Critical Area
            if (col < numCols - 1 && grid[col + 1][row] == EMPTY) {
                if (DEBUG_MOVE)
                    printf("East is free for robot %d. Moving East to (%d,%d).\n", data->boxId, col + 1, row);
                grid[col + 1][row] = ROBOT;
                robotLoc[myRobot][0] = col + 1;
                robotLoc[myRobot][1] = row;
                grid[col][row] = EMPTY;
            } else {
                if (DEBUG_MOVE)
                    printf("East is blocked for robot %d trying coordinates (%d,%d).\n", data->boxId, col + 1, row);
            }
            break;
            //Moving West
        case WEST:
            //Check if the area west is free, if so we move, if not we do nothing
            //Critical Area
            if (col > 0 && grid[col - 1][row] == EMPTY) {
                if (DEBUG_MOVE)
                    printf("West is free for robot %d. Moving West to (%d,%d).\n", data->boxId, col - 1, row);
                grid[col - 1][row] = ROBOT;
                robotLoc[myRobot][0] = col - 1;
                robotLoc[myRobot][1] = row;
                grid[col][row] = EMPTY;
            } else {
                if (DEBUG_MOVE)
                    printf("West is blocked for robot %d trying coordinates (%d,%d).\n", data->boxId,
                           col - 1, row);
            }
            break;
        default:
            //If we have something other than our expected values we print an error and exit out
            printf("Error in movement Integer.");
            exit(2);
            break;
    }
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
                case NORTH:
                    if (grid[col][row + 2] == EMPTY) {
                        grid[col][row] = EMPTY;
                        grid[col][row + 2] = BOX;
                        grid[col][row + 1] = ROBOT;
                        robotLoc[myRobot][1] = row + 1;
                        boxLoc[myRobot][1] = row + 2;
                    }
                    break;
                case SOUTH:
                    if (grid[col + 1][row] == EMPTY && grid[col + 1][row - 1] == EMPTY) {
                        rotateData->direction = WEST;
                        move(rotateData);
                        rotateData->direction = SOUTH;
                        move(rotateData);
                        data->boxSide = WEST;
                        push(data);
                    }
                    break;
                case EAST:
                    if (grid[col][row - 1] == EMPTY && grid[col + 1][row - 1] == EMPTY) {
                        rotateData->direction = SOUTH;
                        move(rotateData);
                        rotateData->direction = EAST;
                        move(rotateData);
                        data->boxSide = NORTH;
                        push(data);
                    }
                    break;
                case WEST:
                    if (grid[col][row - 1] == EMPTY && grid[col - 1][row - 1] == EMPTY) {
                        rotateData->direction = SOUTH;
                        move(rotateData);
                        rotateData->direction = WEST;
                        move(rotateData);
                        data->boxSide = NORTH;
                        push(data);
                    }
                    break;
            }
            break;
        case SOUTH:
            switch (myBoxSide) {
                case NORTH:
                    if (grid[col - 1][row] == EMPTY && grid[col - 1][row + 1] == EMPTY) {
                        rotateData->direction = WEST;
                        move(rotateData);
                        rotateData->direction = NORTH;
                        move(rotateData);
                        data->boxSide = EAST;
                        push(data);
                    }
                    break;
                case SOUTH:
                    if (grid[col][row - 2] == EMPTY) {
                        grid[col][row] = EMPTY;
                        grid[col][row - 1] = ROBOT;
                        grid[col][row - 2] = BOX;
                        robotLoc[myRobot][1] = row - 1;
                        boxLoc[myRobot][1] = row - 2;
                    }
                    break;
                case EAST:
                    if (grid[col][row + 1] == EMPTY && grid[col + 1][row + 1] == EMPTY) {
                        rotateData->direction = NORTH;
                        move(rotateData);
                        rotateData->direction = EAST;
                        move(rotateData);
                        data->boxSide = SOUTH;
                        push(data);
                    }
                    break;
                case WEST:
                    if (grid[col][row + 1] == EMPTY && grid[col - 1][row + 1] == EMPTY) {
                        rotateData->direction = NORTH;
                        move(rotateData);
                        rotateData->direction = WEST;
                        move(rotateData);
                        data->boxSide = SOUTH;
                        push(data);
                    }
                    break;
            }
            break;
        case EAST:
            switch (myBoxSide) {
                case NORTH:
                    if (grid[col - 1][row] == EMPTY && grid[col - 1][row + 1] == EMPTY) {
                        rotateData->direction = WEST;
                        move(rotateData);
                        rotateData->direction = NORTH;
                        move(rotateData);
                        data->boxSide = EAST;
                        push(data);
                    }
                    break;
                case SOUTH:
                    if (grid[col - 1][row] == EMPTY && grid[col - 1][row - 1] == EMPTY) {
                        rotateData->direction = WEST;
                        move(rotateData);
                        rotateData->direction = SOUTH;
                        move(rotateData);
                        data->boxSide = EAST;
                        push(data);
                    }
                    break;
                case EAST:
                    if (grid[col + 2][row] == EMPTY) {
                        grid[col][row] = EMPTY;
                        grid[col + 2][row] = BOX;
                        grid[col + 1][row] = ROBOT;
                        robotLoc[myRobot][0] = col + 1;
                        boxLoc[myRobot][0] = col + 2;
                    }
                    break;
                case WEST:
                    if (grid[col][row + 1] == EMPTY && grid[col - 1][row + 1] == EMPTY) {
                        rotateData->direction = NORTH;
                        move(rotateData);
                        rotateData->direction = WEST;
                        move(rotateData);
                        data->boxSide = SOUTH;
                        push(data);
                    }
                    break;

            }
            break;
        case WEST:
            switch (myBoxSide) {
                case NORTH:
                    if (grid[col + 1][row] == EMPTY && grid[col + 1][row + 1] == EMPTY) {
                        rotateData->direction = EAST;
                        move(rotateData);
                        rotateData->direction = NORTH;
                        move(rotateData);
                        data->boxSide = WEST;
                        push(data);
                    }
                    break;
                case SOUTH:
                    if (grid[col + 1][row] == EMPTY && grid[col + 1][row - 1] == EMPTY) {
                        rotateData->direction = EAST;
                        move(rotateData);
                        rotateData->direction = SOUTH;
                        move(rotateData);
                        data->boxSide = WEST;
                        push(data);
                    }
                    break;
                case EAST:
                    if (grid[col][row + 1] == EMPTY && grid[col + 1][row + 1] == EMPTY) {
                        rotateData->direction = NORTH;
                        move(rotateData);
                        rotateData->direction = EAST;
                        move(rotateData);
                        data->boxSide = SOUTH;
                        push(data);
                    }
                    break;
                case WEST:
                    if (grid[col - 2][row] == EMPTY) {
                        grid[col][row] = EMPTY;
                        grid[col - 2][row] = BOX;
                        grid[col - 1][row] = ROBOT;
                        robotLoc[myRobot][0] = col - 1;
                        boxLoc[myRobot][0] = col - 2;
                    }
                    break;
            }
            break;
        default:
            printf("Error in movement Integer.");
            break;
    }
    free(rotateData);
    //if the debug_grid switch is set to 1, this will print the state grid
    printGrid();
    return;
}
//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================

/**
 * Reads in from the input file the number of rows and columns,
 * the number of boxes(and therefore robots) and the number of doors.
 * It then randomly places those onto the state grid and into the
 * appropriate arrays, which assigns robots to boxes and boxes to doors.
 * The function also writes all the initial states into a file output.txt
 */
void initializeApplication(void) {
    //allocate the memory for the array showing if the bots are done
    isDone = (int*)calloc(numBoxes,sizeof(int));

    //Set up the file we output to
    outputFile = fopen("output.txt", "w");

    //check for an error
    if (outputFile == NULL) {
        printf("Problem writing to the file.\n");
        exit(4);
    }

    //output parameters to the file
    fprintf(outputFile, "%d x %d Grid (Col x Row) with %d Boxes and %d Doors",
            numCols, numRows, numBoxes, numDoors);
    fprintf(outputFile, "\n");

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
    for (int i = 0; i < numBoxes; i++) {
        robotLoc[i] = (int *) calloc(2, sizeof(int));
        boxLoc[i] = (int *) calloc(2, sizeof(int));
    }
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
    fprintf(outputFile, "\n");
    //assign each box and robot an initial location
    for (int i = 0; i < numBoxes; i++) {
        robotLoc[i][1] = rand() % numRows;
        robotLoc[i][0] = rand() % numCols;
        //check for duplicate entries
        while (checkForDupe(robotLoc[i][0], robotLoc[i][1], check, checkIndex) == 1) {
            srand((unsigned int) time(NULL));
            robotLoc[i][1] = rand() % numRows;
            robotLoc[i][0] = rand() % numCols;
        }
        check[checkIndex][1] = robotLoc[i][1];
        check[checkIndex][0] = robotLoc[i][0];
        checkIndex++;
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
        fprintf(outputFile, "Box %d Initial Location: (%d, %d)\n", i, boxLoc[i][0], boxLoc[i][1]);
        check[checkIndex][1] = boxLoc[i][1];
        check[checkIndex][0] = boxLoc[i][0];
        checkIndex++;

    }

    fprintf(outputFile, "\n");
    //assign each door a location and a box to be associated
    //with it
    for (int i = 0; i < numDoors; i++) {
        //door location
        doorLoc[i][1] = rand() % numRows;
        doorLoc[i][0] = rand() % numCols;
        //check for duplicate entries
        while (checkForDupe(doorLoc[i][0], doorLoc[i][1], check, checkIndex) == 1) {
            srand((unsigned int) time(NULL));
            doorLoc[i][1] = rand() % numRows;
            doorLoc[i][0] = rand() % numCols;
        }
        fprintf(outputFile, "Door %d Location: (%d, %d)\n", i, doorLoc[i][0], doorLoc[i][1]);
        check[checkIndex][1] = doorLoc[i][0];
        check[checkIndex][0] = doorLoc[i][1];
        randDoor = rand() % numDoors;
        //assign the box associated with this door
        doorAssign[i] = randDoor;
        checkIndex++;
    }

    fprintf(outputFile, "\n");
    //cycle through robot array to output the initial locations in the right order
    for (int i = 0; i < numBoxes; i++) {
        fprintf(outputFile, "Robot %d Initial Location: (%d, %d) with a goal of Door %d\n",
                i, robotLoc[i][0], robotLoc[i][1], doorAssign[i]);
    }
    fclose(outputFile);
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

    //	normally, here I would initialize the location of my doors, boxes,
    //	and robots, and create threads (not necessarily in that order).
    //	For the handout I have nothing to do.
}

struct pushData *initializeBot(int botNum) {

    struct pushData *pushInfo = (struct pushData *) malloc(sizeof(struct pushData));
    pushInfo->boxId = botNum;
    if (boxLoc[botNum][0] == robotLoc[botNum][0]) {
        if (boxLoc[botNum][1] == robotLoc[botNum][1]) {
            pushInfo->direction = 4;
            pushInfo->numSpaces = 0;
        } else if (robotLoc[botNum][1] < boxLoc[botNum][1]) {
            pushInfo->direction = NORTH;
            pushInfo->numSpaces = boxLoc[botNum][1] - robotLoc[botNum][1];
        } else {
            pushInfo->direction = 1;
            pushInfo->numSpaces = robotLoc[botNum][1] - boxLoc[botNum][1];
        }
    } else if (boxLoc[botNum][1] == robotLoc[botNum][1]) {
        if (boxLoc[botNum][0] == robotLoc[botNum][0]) {
            pushInfo->direction = 4;
            pushInfo->numSpaces = 0;
        } else if (boxLoc[botNum][0] < robotLoc[botNum][0]) {
            pushInfo->direction = 3;
            pushInfo->numSpaces = robotLoc[botNum][0] - boxLoc[botNum][0];
        } else {
            pushInfo->direction = 2;
            pushInfo->numSpaces = boxLoc[botNum][0] - robotLoc[botNum][0];
        }
    } else if (boxLoc[botNum][0] < robotLoc[botNum][0]) {
        pushInfo->direction = 3;
        pushInfo->numSpaces = robotLoc[botNum][0] - boxLoc[botNum][0];
    } else if (boxLoc[botNum][0] > robotLoc[botNum][0]) {
        pushInfo->direction = 2;
        pushInfo->numSpaces = robotLoc[botNum][0] - boxLoc[botNum][0];
    }
    if ((robotLoc[botNum][0] == boxLoc[botNum][0] && abs(robotLoc[botNum][1] - boxLoc[botNum][1]) == 1) ||
        (robotLoc[botNum][1] == boxLoc[botNum][1] && abs(robotLoc[botNum][0] - boxLoc[botNum][0]) == 1)) {
        pushInfo->direction = IN_PLACE;
    }
    return pushInfo;
}

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
