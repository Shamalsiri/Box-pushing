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
//
#include "gl_frontEnd.h"

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);

void displayStatePane(void);

void initializeApplication(void);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern const int GRID_PANE, STATE_PANE;
extern int gMainWindow, gSubwindow[2];

struct pushData
{
  char direction;
  int numSpaces;
};

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
int **grid;
int numRows = -1;    //	height of the grid
int numCols = -1;    //	width
int numBoxes = -1;    //	also the number of robots
int numDoors = -1;    //	The number of doors.

//arrays for holding the robot/door data
int **robotLoc;
int **boxLoc;
int *doorAssign;
int **doorLoc;

int numLiveThreads = 0;        //	the number of live robot threads

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

    for (int i = 0; i < numBoxes; i++) {
        //	here I would test if the robot thread is still live
        drawRobotAndBox(i, robotLoc[i][1], robotLoc[i][0], boxLoc[i][1], boxLoc[i][0], doorAssign[i]);
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
        numRows = atoi(argv[1]);
        numCols = atoi(argv[2]);
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
//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void) {
    outputFile = fopen("output.txt", "w");
    if (outputFile == NULL) {
        printf("Problem writing to the file.\n");
        exit(4);
    }
    fprintf(outputFile, "%d x %d Grid (Row x Col) with %d Boxes and %d Doors\n",
            numRows, numCols, numBoxes, numDoors);
    fprintf(outputFile, "\n");

    //	Allocate the grid
    grid = (int **) calloc((size_t) numRows, sizeof(int *));
    for (int i = 0; i < numRows; i++)
        grid[i] = (int *) calloc((size_t) numCols, sizeof(int));

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
    //assign each door a location and a box to be associated
    //with it
    for (int i = 0; i < numDoors; i++) {
        //door location
        doorLoc[i][0] = rand() % numRows;
        doorLoc[i][1] = rand() % numCols;
        //check for duplicate entries
        while (checkForDupe(doorLoc[i][0], doorLoc[i][1], check, checkIndex) == 1) {
            srand((unsigned int) time(NULL));
            doorLoc[i][0] = rand() % numRows;
            doorLoc[i][1] = rand() % numCols;
        }
        fprintf(outputFile, "Door %d Location: (%d, %d)\n", i, doorLoc[i][0], doorLoc[i][1]);
        check[checkIndex][0] = doorLoc[i][0];
        check[checkIndex][1] = doorLoc[i][1];
        randDoor = rand() % numDoors;
        //assign the box associated with this door
        doorAssign[i] = randDoor;
        checkIndex++;
    }
    fprintf(outputFile, "\n");
    //assign each box and robot an initial location
    for (int i = 0; i < numBoxes; i++) {
        robotLoc[i][0] = rand() % numRows;
        robotLoc[i][1] = rand() % numCols;
        //check for duplicate entries
        while (checkForDupe(robotLoc[i][0], robotLoc[i][1], check, checkIndex) == 1) {
            srand((unsigned int) time(NULL));
            robotLoc[i][0] = rand() % numRows;
            robotLoc[i][1] = rand() % numCols;
        }
        check[checkIndex][0] = robotLoc[i][0];
        check[checkIndex][1] = robotLoc[i][1];
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
        boxLoc[i][0] = randRow;
        boxLoc[i][1] = randCol;
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
            boxLoc[i][0] = randRow;
            boxLoc[i][1] = randCol;
        }
        fprintf(outputFile, "Box %d Initial Location: (%d, %d)\n", i, boxLoc[i][0], boxLoc[i][1]);
        check[checkIndex][0] = boxLoc[i][0];
        check[checkIndex][1] = boxLoc[i][1];
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
    //	normally, here I would initialize the location of my doors, boxes,
    //	and robots, and create threads (not necessarily in that order).
    //	For the handout I have nothing to do.
}
void initializeBot(int botNum)
{
  //we neee to Check if grid boxX, boxY-1 is available
  // if boxX and boxY -1 is available
  robotLoc[botNum][0] = boxLoc[botNum][0];
  robotLoc[botNum][1] = boxLoc[botNum][1] - 1;
}

struct pushData compBoxnDoor(int boxNum)
{
  struct pushData pushInfo;

  if(boxLoc[boxNum][0] == doorLoc[boxNum][0]) // if x is the same
  {
    if(boxLoc[boxNum][1] == doorLoc[boxNum][1]) // if y is the same
    {
      pushInfo.direction = 'A';
      pushInfo.numSpaces = 0;
    }
    else if (boxLoc[boxNum][1] < doorLoc[boxNum][1]) // if box is directly below the door
    {
      pushInfo.direction = 'N';
      pushInfo.numSpaces = doorLoc[boxNum][1] - boxLoc[boxNum][1];

    }
    else
    {
      pushInfo.direction = 'S';
      pushInfo.numSpaces = boxLoc[boxNum][1] - doorLoc[boxNum][1];
    }
  }
  else if (boxLoc[boxNum][1] == doorLoc[boxNum][1]) // if y is the same
  {
    if (boxLoc[boxNum][0] == doorLoc[boxNum][0]) // not sure if redundant
    {
      pushInfo.direction = 'A';
      pushInfo.numSpaces = 0;
    }
    else if (boxLoc[boxNum][0] < doorLoc[boxNum][0])
    {
      pushInfo.direction = 'E';
      pushInfo.numSpaces = doorLoc[boxNum][0] - boxLoc[boxNum][0];
    }
    else
    {
      pushInfo.direction = 'W';
      pushInfo.numSpaces = boxLoc[boxNum][0] - doorLoc[boxNum][0];
    }
  }
  else if (boxLoc[boxNum][0] < doorLoc[boxNum][0])
  {
    pushInfo.direction = 'E';
    pushInfo.numSpaces = doorLoc[boxNum][0] - boxLoc[boxNum][0];
  }
  else if (boxLoc[boxNum][0] > doorLoc[boxNum][0])
  {
    pushInfo.direction = 'W';
    pushInfo.numSpaces = boxLoc[boxNum][0] - doorLoc[boxNum];
  }

  return pushInfo;
}
