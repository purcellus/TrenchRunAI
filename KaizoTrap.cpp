//Made by Austin Purcell
// -S for the assembly code
//Comments about the trench game:
/*  Holy moley this game is ingeniously made.
    You can't stall the game by going back and forth (Can only move backwards to capture an opponent, have to make a move).
    You can't immediately attack on the first turn (X Wings block TIE fighters, walls block X Wings, X Wings can't take death star in first turn,
        has to be behind).
*/

/*Heuristic Ideas:
    Longest delay:  
    Have tie fighter move sideways, then x wing one up diagonally.  repeat.
    
    Best Space:  Don't care about enemy pieces, only moves to attack.
    higher the number, higher the priority to get a piece on that spot
    x wing takes priority, harder to diagonal.  Both can use #s, though.
    even:  x wing
    odd:  Tie fighter
    
    Note that the map is switched.
        7 7 8 9 8 7 7  (HUMAN)  //note that the 7's to the diagonal of the wall are terrible for x wings.
        - 6 ~ * ~ 6 -
        6 - - 8 - - 6
        6 - - - - - 6
        7 - - 8 - - 7
        - - + @ + - -
        7 - 7 8 7 - 7  (COMPUTER)
    
    
    Block Moves Idea:
        Turn 1:
        Get center left or center right x wing to right under their Death star.  This will make it
        harder for the opponent to move.  Nothing can immediately attack it, as well.
        Turn 2:
        Assuming the enemy can't lose any pieces, put center left or center right tie fighter to
        right under our death star.  This forces the enemy to lose a piece, as nothing can attack
        the tie fighter (blocked by walls, and an enemy tie fighter would have to approach, leaving
        itself open).
        Turn 3:
        

    Alternating colors:
        If both x wings were captured and they were on a "black" space, then black spaces are safer,
        since only TIE fighters can attack.

*/


/*Optimization Ideas:
    Think on the opponent's time:  Threads
    
    Zobrist Hashing
    Iterative deepening:  Deepen if more time available and quit early, etc.

    Heuristic Deepening:  Go through more layers if there's something important, i.e. piece captured.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <cmath>
               
#define yoffoldy YWIDTH*movestack[movestackoff + 1]               
#define yoffnewy YWIDTH*movestack[movestackoff + 3]
#define humantieoffset counter + 1
#define computerxwingoffset counter + 2              
#define computertieoffset counter + 3               
#define movestackoff curdepth*6               
               
using namespace std;//to use cout without doing std::

//the 'move character' to show the possible moves on board.
const char MOVECHAR = 'o';//a character to indicate that at least one piece can move there.  MOstly for debugging purposes.
const char EMPTYCHAR = '-';//a character to indicate that this piece is blank.

const int MAXDEPTH = 8;//the maximum depth of the minimax algorithm by default.  Can change under certain circumstances..

//heuristic values
const int BELOWWORST = -256;//no heuristic value will go beyond these values.
const int ABOVEBEST = 256;

const int XWIDTH = 7;//Just in case you try to make it modular.
const int YWIDTH = 7;
const int NUMOFSQUARES = 49;               
char boardarray[YWIDTH*XWIDTH];//The board is global.  Or, interstellar, hehe.

//y x values, respectively.
const int LISTSIZE = 420;//list size per depth, ex.  100 moves * 4 = 400 elements per depth for moves.
/*x wings can go up to 6 moves going forwards, or two moves going backwards
//8
//tie fighters can go up to 6 moves going forwards, 6 moves going sideways, or one move going backwards.
//13
//each legal move requires 5 parts to it.
//oldx, oldy, newx, newy, piecenum.
//so, 8*4 + 13*4 = 32 + 52.
//= 84 moves
//84 * 5 = 420.*/

int listoflegalmoves[LISTSIZE*MAXDEPTH];//There can be a maximum of 84 legal moves per turn, and 5 characters per move (old and new location).
/*hence, 84 * 5 = 420.
//Keep in mind this is an upper bound:  There can certainly be less moves.
//And keep in mind this is an overestimate.
//4 ties = 4 * 12 = 48
//4 x wings = 4 * 12 = 48
//oldx, oldy, newx, newy, piecenum*/

int movenum[MAXDEPTH];//the displacer for listoflegalmoves.
//The amount of list of moves is equal to the MAXDEPTH, since the same layer moves will just be removed.
char userinput[4];//The user's way of inputting the four below variables.
// int PIECEGONE = 15;//indicates that a piece is gone, by moving it out of bounds

const int NUMOFPIECES = 4;//number of pieces, since same # of x wings and tie fighters, just use this number
//Use these arrays to quickly find movable pieces, instead of iteratively searching the array for them.


int piecepositions[NUMOFPIECES*8];//list of all piece positions.
/*first four are human x wing
//next four are human tie fighters
//four comp x wing
//four tie fighters.
	//first part is y axis
	//second part is x axis.*/

int capturedpieces[NUMOFPIECES*4];//the list of captured pieces
/*0 to 3 = human x wing
//next four are human tie
//next four are comp x wing
//four tie fighters*/
int captureindicator;//shows the order of pieces captured.  Put this in the captured pieces array,
//to see which piece was captured first.
int movestack[6*MAXDEPTH];//list of moves that are currently made.
/*1:  piecetomovex:  old location
//2:  piecetomovey
//3:  piecenewx	:  new location
//4:  piecenewy
//5:  piecetomove, old location character.
//6:  boardarray[yoffnewy + movestack[movestackoff+3]], new location character
//TODO just use the move stack, pass in only depth when making and unmaking moves.*/

int listofhorizontaltiemoves[24*MAXDEPTH];//if the piece moved horizontally a turn previous.
//24 because 4 tie fighters can make up to 6 horizontal moves each.
//movenum is stored here.
int horizontalmovenum[MAXDEPTH];//the displacer for listofhorizontaltiemoves
//so uhhh...a horizontal move num at 0 apparently is equal to horizontal human.  NOT GOOD.
int horizontalhuman;//indicates if a horizontal move was made in the previous turn.
int horizontalcomputer;
int humanmovenum;//the move the human makes out of main.


void setup();
void printBoard();

int legalXWing(int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, char piecetomove, int curdepth, int piecenum);//see if the move is a valid x wing move
int legalTieFighter(int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, char piecetomove, int curdepth, int piecenum);//see if the move is a valid tie fighter move
int validateInput( int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, int curdepth, int piecenum);//see if the move in general is valid
int checkGameOver();
int checkNoMoves(int whichplayer, int curdepth);

void findHumanMoves(int curdepth);//find a list of valid human moves
void findComputerMoves(int curdepth);//find a list of valid computer moves
int checkListOfMoves();//check the human move with the list of available human moves.
int checkListOfHorizontalMoves(int movenumber, int curdepth);//check to see if the move made was horizontal tie.
void showListOfMoves(int curdepth);
void showAllMoves();//show all moves stored after a turn.
void showNewMovesOnBoard(int newx, int newy);//debugging
void cleanBoard();
//void showPieces();//show all pieces
//void showListStack(int curdepth);//show the list stack.

int evaluate(int curdepth);//evaluate the heuristic value.
int maxMove(int curdepth, int beta);
int minMove(int curdepth, int alpha);
int makeAMove();

int getHumanMove();

void movePiece(int curdepth, int piecenum);
void resetPiecePosition( int whichplayer, int curdepth, int piecenum);
int checkPieceRemoved(int curdepth, int piecenum);

void doubleCaptureIndicators();//double the capture indicators so that when a move is undone, it doesn't undo a piece being captured that it shouldn't.


//simple evaluate:  just return 0.  See ply effectiveness.
//Fastest, and should be, due to high pruning.
/*int evaluate(int curdepth)
{
    return 0;
}*/

//simple evaluate:  just return a random value.  See randomization of moves.
//slowest, due to pruning ineffectiveness.
/*int evaluate(int curdepth)
{
    return rand()%1024 - 512;
}*/


//evaluate only based on pieces captured over the course of the match.
//so far, the most effective.
int evaluate(int curdepth)
{
    int pieceadvantage = 0;//the piece advantage.
	//cout << moveadvantage << "\n";//debug
	for (int piececounter = 0; piececounter < NUMOFPIECES*4; piececounter++)
	{//look at the pieces captured.
		if (capturedpieces[piececounter] != 0 && piececounter < 8)
		{//if a human piece was captured
			pieceadvantage++;//AI is happy:  increase the value
		}
		else if (capturedpieces[piececounter] != 0 && piececounter >= 8)
		{//if a computer piece was captured
			pieceadvantage-=2;
		}
	}
    return pieceadvantage;
}

//evaluate only based on the pieces captured in this minimax iteration.  previous iterations don't matter.
//Based on empirical trials, not as effective as evaluating all pieces.
/*int evaluate(int curdepth)
{
    int pieceadvantage = 0;//the piece advantage.
	//cout << moveadvantage << "\n";//debug
	for (int piececounter = 0; piececounter < NUMOFPIECES*4; piececounter++)
	{//look at the pieces captured.
		if (capturedpieces[piececounter] != 0 && capturedpieces[piececounter] < 16 && piececounter < 8 )
		{//if a human piece was captured
			pieceadvantage++;//AI is happy:  increase the value
		}
		else if (capturedpieces[piececounter] != 0 && capturedpieces[piececounter] < 16 && piececounter >= 8)
		{//if a computer piece was captured
			pieceadvantage--;
		}
	}
    return pieceadvantage;
}*/


//evaluate based only on available moves over the course of the algorithm.
/*int evaluate(int curdepth)
{//evaluate the heuristic value, how good the move is.
    //first, look at each movenumber:  0, 2, etc. are computers, and 1, 3, etc. are humans.
    //Compare the two.
	int moveadvantage = 0;
	for (int counter = 0; counter < curdepth; counter++)
	{//look at each depth's list of moves number
		if (counter % 2 == 0)
		{//if this is the ai's moves, add them
			moveadvantage += movenum[counter];
		}
		else
		{//if this is the human's moves, subtract them
			moveadvantage -= movenum[counter];
		}
	}
	//moveadvantage = moveadvantage*1;
	

    
    
	return moveadvantage;
}*/

//evaluate based only on available moves at the end of the algorithm (the leaf and off by one).
/*int evaluate(int curdepth)
{//evaluate the heuristic value, how good the move is.
    //first, look at each movenumber:  0, 2, etc. are computers, and 1, 3, etc. are humans.
    //Compare the two.
	int moveadvantage = 0;
	for (int counter = curdepth - 2; counter < curdepth; counter++)
	{//look at each depth's list of moves number
		if (counter % 2 == 0)
		{//if this is the ai's moves, add them
			moveadvantage += movenum[counter];
		}
		else
		{//if this is the human's moves, subtract them
			moveadvantage -= movenum[counter];
		}
	}
	//moveadvantage = moveadvantage*1;
	

    
    
	return moveadvantage;
}*/


//based on moves and pieces captured during the duration:  See the average mobility and piece capture advantage.
/*int evaluate(int curdepth)
{//evaluate the heuristic value, how good the move is.
    //first, look at each movenumber:  0, 2, etc. are computers, and 1, 3, etc. are humans.
    //Compare the two.
	int moveadvantage = 0;
	for (int counter = 0; counter < curdepth; counter++)
	{//look at each depth's list of moves number
		if (counter % 2 == 0)
		{//if this is the ai's moves, add them
			moveadvantage += movenum[counter];
		}
		else
		{//if this is the human's moves, subtract them
			moveadvantage -= movenum[counter]*2;
		}
	}
	//moveadvantage = moveadvantage*1;
	
	int pieceadvantage = 0;//the piece advantage.
	//cout << moveadvantage << "\n";//debug
	for (int piececounter = 0; piececounter < NUMOFPIECES*4; piececounter++)
	{//look at the pieces captured.
		if (capturedpieces[piececounter] != 0 && piececounter < 8)
		{//if a human piece was captured
			pieceadvantage++;//AI is happy:  increase the value
		}
		else if (capturedpieces[piececounter] != 0 && piececounter >= 8)
		{//if a computer piece was captured
			pieceadvantage -= 2;
		}
	}
	pieceadvantage = pieceadvantage * 5;//add ratio, to make it affect the thingy.
	//int thevalue = rand() % 1000 ;
	int thevalue = moveadvantage + pieceadvantage;
    
    
    
    
	return thevalue;
}*/


//based on moves and pieces captured at the end of the algorithm.  
/*int evaluate(int curdepth)
{//evaluate the heuristic value, how good the move is.
    //first, look at each movenumber:  0, 2, etc. are computers, and 1, 3, etc. are humans.
    //Compare the two.
	int moveadvantage = 0;
	for (int counter = curdepth - 2; counter < curdepth; counter++)
	{//look at each depth's list of moves number
		if (counter % 2 == 0)
		{//if this is the ai's moves, add them
			moveadvantage += movenum[counter];
		}
		else
		{//if this is the human's moves, subtract them
			moveadvantage -= movenum[counter];
		}
	}
	//moveadvantage = moveadvantage*1;
	
	int pieceadvantage = 0;//the piece advantage.
	//cout << moveadvantage << "\n";//debug
	for (int piececounter = 0; piececounter < NUMOFPIECES*4; piececounter++)
	{//look at the pieces captured.
		if (capturedpieces[piececounter] != 0 && capturedpieces[piececounter] < 16 &&  piececounter < 8)
		{//if a human piece was captured
			pieceadvantage++;//AI is happy:  increase the value
		}
		else if (capturedpieces[piececounter] != 0 && capturedpieces[piececounter] < 16 &&  piececounter >= 8)
		{//if a computer piece was captured
			pieceadvantage--;
		}
	}
	pieceadvantage = pieceadvantage * 5;//add ratio, to make it affect the thingy.
	//int thevalue = rand() % 1000 ;
	int thevalue = moveadvantage + pieceadvantage;
    
    
    
    
	return thevalue;
}*/




int main()
{//Start here
    setup();//initialize the board
    printBoard();//show the board state
    int humanmessedup = 1;//indicates if the human messed up, and the human's turn.
    captureindicator = 1;//initially, no pieces are captured.
	humanmovenum = 0;//to see if human tried to make a horizontal tie move.
    //start at 1, we put into the list:  the indicator decrements if a piece is unremoved
	horizontalhuman = 0;
	horizontalcomputer = 0;
	
    cout << "Would you like to go first?  Enter -1 If you do, and 0 if you don't:  ";
    scanf("%d", &humanmessedup);//might as well use the humanmessedup value.
	//movenum[curdepth] = 1;//to prevent immediate game over if computer starts.
	int start = humanmessedup;
    for(;;)
    {//Start the game, and keep playing
		horizontalhuman = horizontalhuman - 1;//pretend to decrement.
		cout << "horizontalhuman is currently " << horizontalhuman << "\n";
		horizontalmovenum[0] = 0;//reset counter.  
        while (humanmessedup == -1)
        {//while the person keeps messing up
		    //printBoard();//print the new board state, or remind person of current board state.
            
            humanmessedup = getHumanMove();//get the human's move.
            fflush(stdout);//to flush any outputs.
        }
        //Here, I assume the user inputted a valid play, so I now update the board state
        
        
        if (start == 0)
		{
			start = 1;
		}
		else
		{
			movePiece( 0, humanmessedup);//This is a true move.  The stack should have the move to make
			//the humanmessedup variable also acts as a piecenum variable, passed from checklistofmoves.
			//cout << "Movestack at main is " << char(movestack[4]) << "\n";//debug
			//cout << "Humanmovenum at main is " << humanmovenum << "\n";
			if (char(movestack[4]) == 't' && checkListOfHorizontalMoves(humanmovenum , 0) == 1 && horizontalhuman != 1)
			{//if this was a horizontal move, pretend it was one by setting the horizontal value.
				//TODO FIX THIS SO IT DOESN'T DO THIS AFTER MAKING THE FIRST VERTICAL MOVE, FOR SOME REASON.
				//No seriously, fix this.
				cout << "Human made a horizontal move.  Set value to 2.  humanmovenum = " << humanmovenum << "\n";
				horizontalhuman = 2;
			}
		}	
		
        doubleCaptureIndicators();
		//cleanBoard();//clean board before showing updated board state
	    printBoard();//show updated state

        if (checkGameOver() == 1)
		{
            cout << " Game Over:  You win\n";
			exit(0);
		}
        //AI's turn:  Determine best play
		//cout << "Horizontal human ! is now " << horizontalhuman << "\n";//debug
        makeAMove();
		//cout << "Horizontal human ! is now " << horizontalhuman << "\n";//debug

        doubleCaptureIndicators();
        //update board state
        //display board and announce move
		//cout << "Horizontal human is now " << horizontalhuman << "\n";

		//cleanBoard();//clean board after, to show the computer's possible moves.		
        printBoard();
        //check if game over.
		if (checkGameOver() == 1)
		{
            cout << "Game Over:  I Win\n";
			exit(0);
		}
        humanmessedup = -1;//reset, so human can make another input.
    }


}


void setup()
{//Set up the initial board state:
    /*
    
     7 - T T - T T -  (COMPUTER)
     6 - - ~ * ~ - -
     5 X X - - - X X
     4 - - - - - - -
     3 x x - - - x x
     2 - - + @ + - -
     1 - t t - t t -  (HUMAN)
       A B C D E F G
    */

    //Hypothetically, since this is an array of strings, I could initialize it as such.
    //More hypothetically, I can change the positions array to hold one element per object,
	//since the board is now one dimensional.
	

	piecepositions[0] = 4;//list of human x wings, start with y pos then x pos
	piecepositions[1] = 0;
	piecepositions[2] = 4;
	piecepositions[3] = 1;
	piecepositions[4] = 4;
	piecepositions[5] = 5;
	piecepositions[6] = 4;
	piecepositions[7] = 6;
	
    piecepositions[8] = 6;//list of human tie fighters
	piecepositions[9] = 1;
	piecepositions[10] = 6;
	piecepositions[11] = 2;
	piecepositions[12] = 6;
	piecepositions[13] = 4;
	piecepositions[14] = 6;
	piecepositions[15] = 5;
	
	
	
	piecepositions[16] = 2;//list of computer x wings
	piecepositions[17] = 0;
	piecepositions[18] = 2;
	piecepositions[19] = 1;
	piecepositions[20] = 2;
	piecepositions[21] = 5;
	piecepositions[22] = 2;
	piecepositions[23] = 6;
		
	piecepositions[24] = 0;//list of computer tie fighters
	piecepositions[25] = 1;
	piecepositions[26] = 0;
	piecepositions[27] = 2;
	piecepositions[28] = 0;
	piecepositions[29] = 4;
	piecepositions[30] = 0;
	piecepositions[31] = 5;
	
    boardarray[0] = EMPTYCHAR;//blank space
    boardarray[1] = 'T';//Computer Tie Fighter.
    boardarray[2] = 'T';//Computer Tie Fighter.
    boardarray[3] = EMPTYCHAR;
    boardarray[4] = 'T';//Computer Tie Fighter.
    boardarray[5] = 'T';//Computer Tie Fighter.
    boardarray[6] = EMPTYCHAR;
        		
    boardarray[7] = EMPTYCHAR;
    boardarray[8] = EMPTYCHAR;
    boardarray[9] = '~';//Computer Wall
    boardarray[10] = '*';//Computer Death Star
    boardarray[11] = '~';//Computer Wall
    boardarray[12] = EMPTYCHAR;
    boardarray[13] = EMPTYCHAR;
                    
    boardarray[14] = 'X';//Computer X wing
    boardarray[15] = 'X';//Computer X wing
    boardarray[16] = EMPTYCHAR;
    boardarray[17] = EMPTYCHAR;
    boardarray[18] = EMPTYCHAR;
    boardarray[19] = 'X';//Computer X wing
    boardarray[20] = 'X';//Computer X wing
    
    boardarray[21] = EMPTYCHAR;
    boardarray[22] = EMPTYCHAR;
    boardarray[23] = EMPTYCHAR;
    boardarray[24] = EMPTYCHAR;
    boardarray[25] = EMPTYCHAR;
    boardarray[26] = EMPTYCHAR;
    boardarray[27] = EMPTYCHAR;
    
    boardarray[28] = 'x';//Human X wing
    boardarray[29] = 'x';//Human X wing
    boardarray[30] = EMPTYCHAR;
    boardarray[31] = EMPTYCHAR;
    boardarray[32] = EMPTYCHAR;
    boardarray[33] = 'x';//Human X wing
    boardarray[34] = 'x';//Human X wing
        
    boardarray[35] = EMPTYCHAR;
    boardarray[36] = EMPTYCHAR;
    boardarray[37] = '+';//Human Wall
    boardarray[38] = '@';//Human Death Star
    boardarray[39] = '+';//Human Wall
    boardarray[40] = EMPTYCHAR;
    boardarray[41] = EMPTYCHAR;
    
    boardarray[42] = EMPTYCHAR;//blank space
    boardarray[43] = 't';//Human Tie Fighter.
    boardarray[44] = 't';//Human Tie Fighter.
    boardarray[45] = EMPTYCHAR;
    boardarray[46] = 't';//Human Tie Fighter.
    boardarray[47] = 't';//Human Tie Fighter.
    boardarray[48] = EMPTYCHAR;
    
    
    for (int counter = 0; counter < NUMOFPIECES*4; counter++)
    {
        capturedpieces[counter] = 0;
    }
    
    
    
}

void printBoard()
{//print out the current board state
    cout << endl;
    cout << "7 " << boardarray[0] << " " <<  boardarray[1] << " " << boardarray[2] << " " << boardarray[3] << " " << boardarray[4] << " " << boardarray[5] << " " << boardarray[6] << "   COMPUTER";
    printf("\n");//new lines, to make things look nice.
	cout << "6 " << boardarray[7] << " " <<  boardarray[8] << " " << boardarray[9] << " " << boardarray[10] << " " << boardarray[11] << " " << boardarray[12] << " " << boardarray[13];
    printf("\n");
	cout << "5 " << boardarray[14] << " " <<  boardarray[15] << " " << boardarray[16] << " " << boardarray[17] << " " << boardarray[18] << " " << boardarray[19] << " " << boardarray[20];
    printf("\n");    
	cout << "4 " << boardarray[21] << " " <<  boardarray[22] << " " << boardarray[23] << " " << boardarray[24] << " " << boardarray[25] << " " << boardarray[26] << " " << boardarray[27];
    printf("\n");
	cout << "3 " << boardarray[28] << " " <<  boardarray[29] << " " << boardarray[30] << " " << boardarray[31] << " " << boardarray[32] << " " << boardarray[33] << " " << boardarray[34];
    printf("\n");
	cout << "2 " << boardarray[35] << " " <<  boardarray[36] << " " << boardarray[37] << " " << boardarray[38] << " " << boardarray[39] << " " << boardarray[40] << " " << boardarray[41];
    printf("\n");    
	cout << "1 " << boardarray[42] << " " <<  boardarray[43] << " " << boardarray[44] << " " << boardarray[45] << " " << boardarray[46] << " " << boardarray[47] << " " << boardarray[48] << "   HUMAN";
    printf("\n");
    cout << "  A B C D E F G";
    printf("\n\n");

}


int legalXWing(int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, char piecetomove, int curdepth, int piecenum)
{//the valid rules for moving an X Wing
    //1.  Moves diagonally.
    //2.  Can only move backwards if capturing an enemy piece.
    //3.  Can't jump above an occupied space.
    

    if (abs(piecetomovex - piecenewx) != abs(piecetomovey - piecenewy))
    {//first, ensure that the piece moved diagonally (x and y delta are the same)
        /*cout << "Invalid X Wing Move:  Not Diagonal.  Please enter valid move";
        printf("\n");*/
        return 1;
    } 

    //see which way is backwards for the piece.
    int backstep = 0;//the step that is considered backwards, relative to the board's y axis
    if (piecetomove == 'x')
    {//if this is a human x wing
        backstep = 1;
    }
    else if (piecetomove == 'X')
    {//if this is a computer x wing
        backstep = -1;
    }
    
    
    //Make sure the x wing doesn't jump over a piece
	int xstep = 0;//step directions
    int ystep = 0;
    
	if ( piecetomovex > piecenewx)
    {
        xstep = -1;
    }
    else
    {
        xstep = 1;
    }
	
	if (piecetomovey > piecenewy)
    {
        ystep = -1;
    }
	else
    {
        ystep = 1;
    }
    
	//So, cool thing is, The number of steps up or down is the same number of steps left or right.  So, I only have to check one condition.  Neat!
	int curx = piecetomovex + xstep;//Might as well add one now, don't check the piece's original position.
    int cury = piecetomovey + ystep;
    for(;curx != piecenewx ; curx = curx + xstep)
    {//Realization:  I don't care which one is checked, since both deltas are the same, ex. move up one left one, or up three right three.
		if (boardarray[cury*YWIDTH + curx] == EMPTYCHAR || boardarray[cury*YWIDTH + curx] == MOVECHAR)
        {//if the tie fighter didn't jump over or capture a piece
            if (ystep != backstep)
            {//if this piece isn't moving backwards, we can add this to a move.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
                /*cout << "Adding legal x wing move " << listoflegalmoves[movenum[curdepth]] << " " << listoflegalmoves[movenum[curdepth]+1] 
                << " " << listoflegalmoves[movenum[curdepth]+2] << " " << listoflegalmoves[movenum[curdepth]+3] << " "<<"\n";
                cout << "Adding legal x wing move (ASCII Mode)" << (char)(listoflegalmoves[movenum[curdepth]]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+1])+'0') 
                << " " << (char)(listoflegalmoves[movenum[curdepth]+2]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+3])+'0') << " "<<"\n";
                *///Debug        
                movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row. 
            }                
        } 
        else if ((piecetomove == 'x' && (boardarray[YWIDTH*cury + curx] == 'X' || boardarray[YWIDTH*cury + curx] == 'T'))
                || (piecetomove == 'X' && (boardarray[YWIDTH*cury + curx] == 'x' || boardarray[YWIDTH*cury + curx] == 't')))
        {//if this piece is going to capture, add the move, but return after, since going after would jump over a piece.
            //doesn't matter where from, if we got here, then it's fine to move forwards or backwards.
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
            /*cout << "Adding legal x wing move " << listoflegalmoves[movenum[curdepth]] << " " << listoflegalmoves[movenum[curdepth]+1] 
			<< " " << listoflegalmoves[movenum[curdepth]+2] << " " << listoflegalmoves[movenum[curdepth]+3] << " "<<"\n";
            cout << "Adding legal x wing move (ASCII Mode)" << (char)(listoflegalmoves[movenum[curdepth]]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+1])+'0') 
		    << " " << (char)(listoflegalmoves[movenum[curdepth]+2]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+3])+'0') << " "<<"\n";
            *///Debug        
            movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.           
            return 0;
        }
        else if (piecetomove == 'x' && boardarray[YWIDTH*cury + curx] == '*' && ((piecetomovex == 2 || piecetomovex == 4) && piecetomovey == 0))
        {//if this would capture a death star, add it.
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
            /*cout << "Adding legal x wing move " << listoflegalmoves[movenum[curdepth]] << " " << listoflegalmoves[movenum[curdepth]+1] 
			<< " " << listoflegalmoves[movenum[curdepth]+2] << " " << listoflegalmoves[movenum[curdepth]+3] << " "<<"\n";
            cout << "Adding legal x wing move (ASCII Mode)" << (char)(listoflegalmoves[movenum[curdepth]]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+1])+'0') 
		    << " " << (char)(listoflegalmoves[movenum[curdepth]+2]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+3])+'0') << " "<<"\n";
            *///Debug        
            movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.           
            return 0;
        }
        else if (piecetomove == 'X'  && boardarray[YWIDTH*cury + curx] == '@' && ((piecetomovex == 2 || piecetomovex == 4) && piecetomovey == 6))
        {
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
            listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
            /*cout << "Adding legal x wing move " << listoflegalmoves[movenum[curdepth]] << " " << listoflegalmoves[movenum[curdepth]+1] 
			<< " " << listoflegalmoves[movenum[curdepth]+2] << " " << listoflegalmoves[movenum[curdepth]+3] << " "<<"\n";
            cout << "Adding legal x wing move (ASCII Mode)" << (char)(listoflegalmoves[movenum[curdepth]]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+1])+'0') 
		    << " " << (char)(listoflegalmoves[movenum[curdepth]+2]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+3])+'0') << " "<<"\n";
            *///Debug        
            movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.           
            return 0;
        }
        else
        {//return 1 since they went to try to jump over a piece..
            return 1;
        }
		cury = cury + ystep;//also include the increment of the ystep.
    }
	
    
    //last part, have to double check something, make sure it doesn't capture it's own piece.
    if (piecetomove == 'x' && (boardarray[YWIDTH*piecenewy + piecenewx] == 'x' || boardarray[YWIDTH*piecenewy + piecenewx] == 't' ||
		boardarray[YWIDTH*piecenewy + piecenewx] == '@'))
    {//if the human's new location would capture it's own piece.
        /*cout << "Invalid X Wing Move:  Can't capture one's piece.  Please enter a valid move.";
		printf("\n");*/
		return 1;
    }
     else if (piecetomove == 'X' && (boardarray[YWIDTH*piecenewy + piecenewx] == 'X' || boardarray[YWIDTH*piecenewy + piecenewx] == 'T' ||
		boardarray[YWIDTH*piecenewy +piecenewx] == '*'))
	{//if the computer's new location would capture it's own piece.
		/*cout << "I (the computer) made an Invalid X Wing Move:  Can't capture one's piece.  Please enter a valid move.";
		printf("\n");*/
		return 1;
	}
    else if (ystep == backstep && ((piecetomove == 'x' && (boardarray[YWIDTH*piecenewy + piecenewx] != 'X' && boardarray[YWIDTH*piecenewy + piecenewx] != 'T'))
            || (piecetomove == 'X' && (boardarray[YWIDTH*piecenewy + piecenewx] != 'x' && boardarray[YWIDTH*piecenewy + piecenewx] != 't'))))
    {//if this went backwards without capturing at the end, exit without adding the move.
        return 1;
    }
    
	//here, assume we've made a correct move.
    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecenewx;//make sure we use the current step to add
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = piecenewy;
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
    /*cout << "Adding legal x wing move " << listoflegalmoves[movenum[curdepth]] << " " << listoflegalmoves[movenum[curdepth]+1] 
			<< " " << listoflegalmoves[movenum[curdepth]+2] << " " << listoflegalmoves[movenum[curdepth]+3] << " "<<"\n";
	cout << "Adding legal x wing move (ASCII Mode)" << (char)(listoflegalmoves[movenum[curdepth]]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+1])+'0') 
		    << " " << (char)(listoflegalmoves[movenum[curdepth]+2]+'A') << " " << (char)((YWIDTH - listoflegalmoves[movenum[curdepth]+3])+'0') << " "<<"\n";
    *///Debug        
    movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
    //Increment after showing the move, so we don't increment and see a bunch of '0's
    return 0;//assume they made a right move.
    
}

int legalTieFighter(int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, char piecetomove, int curdepth, int piecenum)
{//the valid rules for moving a TIE fighter
    //1.  Moves horizontally or vertically onto an empty space.
    //2.  Can only move sideways once every other turn.  Cannot move sideways twice in one turn.
    //3.  Can only move backwards if capturing an enemy piece.
    //4.  Can't jump above an occupied space.
    int horizontalmove = 0;//indicates if the move was horizontal.  If so, add it to list of horizontal moves via movenum.
	
    
    //see which way is backwards for the piece.
    int backstep = 0;//the step that is considered backwards, relative to the board's y axis
    if (piecetomove == 't')
    {//if this is a human tie
        backstep = 1;
    }
    else if (piecetomove == 'T')
    {//if this is a computer tie
        backstep = -1;
    }
       
    
    //deprecated:  findHumanmoves and findComputer moves makes it so it can only go vertical or horizontally.
    /*if ((piecetomovex - piecenewx) != 0 && (piecetomovey - piecenewy) != 0)
    {//first, ensure that the piece is moved horizontally or vertically (one of the deltas must be 0)
        //cout << "Invalid Tie Fighter Move:  Not Horizontal or Vertical move.  Please enter valid move";
        //printf("\n");
        return 1;
    }*/
    

	
	
   
    int ystep = 0;//incrementer, declared here so the last step can utilize this.
    
    if (piecetomovey - piecenewy == 0)
    {//horizontal move(No y delta)
	
		if (piecetomove == 't')
		{//if this is the human's TIE fighter.
			if (horizontalhuman >= 1)
			{//if the tie fighter already moved horizontally last turn.
				//cout << "A Tie fighter cannot move horizontally twice in a row.  Please enter a valid input.";
				//printf("\n");
				return 1;
			} 
			else
			{//increase value to 2:  Always decremented each turn, including this one
				//TODO figure out how to solve this
				horizontalmove = 1;
			}
		} 
		else if (piecetomove == 'T')
		{//assuming it's the computer's TIE fighter.
			if (horizontalcomputer >= 1)
			{//if the tie fighter moved sideways last turn
				//cout << "I (The computer) made an error:  A Tie fighter cannot move horizontally twice in a row.  Please enter a valid input.";
				//printf("\n");
				return 1;
			}
			else
			{
				horizontalmove = 1;
			}          
		} 
			
        int xstep = 0;//incrementer
        if (piecetomovex > piecenewx)
        {//setup counter to go from old point to new point, making incrementer negative if necessary
			//cout << "piecetomovex > piecenewx.  piecetomovex = " << piecetomovex << " piecenewx = " << piecenewx << "\n";
            xstep = -1;//trying to go to new piece, ex.  oldx = 6, newx = 4, go by -1
        }
        else
        {
			//cout << "piecetomovex < piecenewx.  piecetomovex = " << piecetomovex << " piecenewx = " << piecenewx << "\n";		
            xstep = 1;
        }
        
        int curx = piecetomovex + xstep;//initialize variable to be one off it's starting point, so it doesn't check itself.
        /*we add by one so it doesn't check itself.  Also because if it moves by one step, 
        //no need to check if it jumps, since it isn't jumping anything if it is just moving adjacently.*/
        for ( ; curx != piecenewx; curx = curx + xstep)
        {//increment the steps to the new position, only have to worry about the x axis
			//I'm making it != so that it can work both for increment and decrement.
            
            /*cout << "Checking current x: " << curx;//DEBUG see xstep
            printf("\n");*///debug
            if (boardarray[YWIDTH*piecetomovey + curx] == EMPTYCHAR || boardarray[YWIDTH*piecetomovey + curx] == MOVECHAR )
            {//if the tie fighter isn't jumping over a piece, or is capturing an enemy piece.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = piecenewy;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
                //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
				//	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";

				listofhorizontaltiemoves[24*curdepth + horizontalmovenum[curdepth]] = movenum[curdepth];//put the movenumber that was horizontal, we can check it later.
				//cout << "		listofhorizontaltiemoves[24*curdepth + blah] = " << movenum[curdepth] << " horizontalmovenum[curdepth] =  " << horizontalmovenum[curdepth] << " at curdepth " << curdepth <<"\n";//debug 
				horizontalmovenum[curdepth] = horizontalmovenum[curdepth] + 1;//increment displacer for list of horizontal moves.
				movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.             
            }
            else if ((piecetomove == 't' && (boardarray[YWIDTH*piecetomovey + curx] == 'X' || boardarray[YWIDTH*piecetomovey + curx] == 'T'))
                || (piecetomove == 'T' && (boardarray[YWIDTH*piecetomovey + curx] == 'x' || boardarray[YWIDTH*piecetomovey + curx] == 't')))
            {//if it is capturing a piece, do the same, but return, since checking after would be considered jumping
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = curx;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = piecenewy;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.       
                //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
				//	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";

				listofhorizontaltiemoves[24*curdepth + horizontalmovenum[curdepth]] = movenum[curdepth];//put the movenumber that was horizontal, we can check it later.
				//cout << "		listofhorizontaltiemoves[24*curdepth + blah] = " << movenum[curdepth] << " horizontalmovenum[curdepth] =  " << horizontalmovenum[curdepth] << " at curdepth " << curdepth <<"\n";//debug 
				horizontalmovenum[curdepth] = horizontalmovenum[curdepth] + 1;//increment displacer for list of horizontal moves.
				movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
                return 0;
            }
			else
            {//might as well return, since it is trying to jump over a piece.

                  /*TODO cout << "Can't jump over a piece (x axis).  please enter a valid input.";
                printf("\n");*/
                return 1;
            }
        }
        
        //if we get here, we can assume that it reached to the destination:  Add this play as well.
        
    } 
	else
    {//vertical move(no x delta)
        if (piecetomovey > piecenewy)
        {//setup counter to go from old point to new point, making incrementer negative if necessary
			//cout << "piecetomovey > piecenewy.  piecetomovey = " << piecetomovey << " piecenewy = " << piecenewy << "\n";				
            ystep = -1;//trying to go to new piece, ex.  oldx = 6, newx = 4, go by -1
        }
        else
        {
			//cout << "piecetomovey < piecenewy.  piecetomovey = " << piecetomovey << " piecenewy = " << piecenewy << "\n";						
            ystep = 1;
        }
        
        int cury = piecetomovey + ystep;//initialize variable to be one off it's starting point, so it doesn't check itself.
        /*we add by one so it doesn't check itself.  Also because if it moves by one step, 
        //no need to check if it jumps, since it isn't jumping anything if it is just moving adjacently.*/
        for ( ; cury != piecenewy; cury = cury + ystep)
        {//increment the steps to the new position, only have to worry about the x axis
			//I'm making it != so that it can work both for increment and decrement.
            
            /*cout << "Checking current y: " << cury;//DEBUG see xstep
            printf("\n");*///debug
                      if (boardarray[YWIDTH*cury + piecetomovex] == EMPTYCHAR || boardarray[YWIDTH*cury + piecetomovex] == MOVECHAR )
            {//if the tie fighter isn't jumping over a piece, or is capturing an enemy piece.
                if (ystep != backstep)
                {//if this tie fighter isn't moving backwards, you can add it.
                    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecetomovex;//make sure we use the current step to add
                    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
                    listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.                
                    //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
                    //	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";
                    movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
                }
            }
            else if ((piecetomove == 't' && (boardarray[YWIDTH*cury + piecetomovex] == 'X' || boardarray[YWIDTH*cury + piecetomovex] == 'T'))
                || (piecetomove == 'T' && (boardarray[YWIDTH*cury + piecetomovex] == 'x' || boardarray[YWIDTH*cury + piecetomovex] == 't')))
            {//if it is capturing a piece, do the same, but return, since checking after would be considered jumping
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecetomovex;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.              
                //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
				//	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";
 				movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
               
                return 0;
            }
			else if (piecetomove == 't' && boardarray[YWIDTH*cury + piecenewx] == '*' && (piecetomovex == 3 && piecetomovey == 0))
            {//if the tie's trying to capture the death star not from behind (human)
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecetomovex;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.              
                //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
				//	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";
 				movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
                return 0;
            }
            else if (piecetomove == 'T'  && boardarray[YWIDTH*cury + piecenewx] == '@' && (piecetomovex == 3 && piecetomovey == 6))
            {//same as before, but computer.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecetomovex;//make sure we use the current step to add
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = cury;
                listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.              
                //cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
				//	<< " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << "\n";
 				movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
                return 1;
            }
            else
            {//might as well return, since it is trying to jump over a piece.

                  /*TODO cout << "Can't jump over a piece (x axis).  please enter a valid input.";
                printf("\n");*/
                return 1;
            }
        }
		
		
		
		
    }
    
    
    if (piecetomove == 't' && (boardarray[YWIDTH*piecenewy + piecenewx] == 'x' || boardarray[YWIDTH*piecenewy + piecenewx] == 't' ||
	boardarray[YWIDTH*piecenewy + piecenewx] == '@'))
	{//if the human's new location would capture it's own piece.
		/*cout << "Invalid Tie Fighter Move:  Can't capture one's piece.  Please enter a valid move.";
		printf("\n");*/
		return 1;
	}
    else if (piecetomove == 'T' && (boardarray[YWIDTH*piecenewy + piecenewx] == 'X' || boardarray[YWIDTH*piecenewy + piecenewx] == 'T' ||
		boardarray[YWIDTH*piecenewy + piecenewx] == '*'))
	{//if the computer's new location would capture it's own piece.
		/*cout << "I (the computer) made an Invalid Tie Fighter Move:  Can't capture one's piece.  Please enter a valid move.";
		printf("\n");*/
		return 1;
	}
 
    //Make sure the piece doesn't go backwards, unless it can capture
    if (ystep == backstep && piecetomove == 't' && piecenewy > piecetomovey && 
		boardarray[YWIDTH*piecenewy + piecenewx] != 'X' && boardarray[YWIDTH*piecenewy + piecenewx] != 'T' && 
		boardarray[YWIDTH*piecenewy + piecenewx] != '*')
    {//if this is a player's TIE fighter, remember that board is flipped for y axis.

        /*cout << "Invalid Tie Fighter Move:  Can't go backwards without capturing enemy piece.  Please enter a valid move.";
		printf("\n");*/          
        return 1;


    } 
    else if (ystep == backstep && piecetomove == 'T' && piecenewy < piecetomovey && 
			boardarray[YWIDTH*piecenewy + piecenewx] != 'x' && boardarray[YWIDTH*piecenewy + piecenewx] != 't' && 
			boardarray[YWIDTH*piecenewy + piecenewx] != '@')
    {//if this is a computer's TIE fighter trying to go backwards to a place without an enemy piece.

        /*cout << "I (The computer) made an Invalid Tie Fighter Move:  Can't go backwards without capturing enemy piece.";
		printf("\n");*/
        return 1;
    }
    
    
    
	//if it gets here, we can assume to add this move to the list, since it passed the other preconditions.

	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]] = piecetomovex;//add old and new locations to list of legal moves to play.
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+1] = piecetomovey;//offset by movenum[curdepth].
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+2] = piecenewx;//make sure we use the current step to add
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+3] = piecenewy;
	listoflegalmoves[LISTSIZE*curdepth + movenum[curdepth]+4] = piecenum;//the piece being moved.
	if (horizontalmove == 1)
	{//if this was a horizontal move, add it to list of horizontal tie moves.
		listofhorizontaltiemoves[24*curdepth + horizontalmovenum[curdepth]] = movenum[curdepth];//put the movenumber that was horizontal, we can check it later.
		//cout << "Adding horizontal tie move " << char(listoflegalmoves[movenum[curdepth]+0] + 'A')<< " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+1] + '0')
        //    << " " << char(listoflegalmoves[movenum[curdepth]+2] + 'A') << " " << char(YWIDTH - listoflegalmoves[movenum[curdepth]+3] + '0') << " "
        //    << "for piecenum = " << piecenum << "\n";
		horizontalmovenum[curdepth] = horizontalmovenum[curdepth] + 1;//increment displacer for list of horizontal moves.
	}

    movenum[curdepth] = movenum[curdepth] + 5;//offset based on number of elements per pseudo row.
    //cout << "movenum after legal at depth " << curdepth << " is now " << movenum[curdepth] << "\n";  
    //Increment after showing the move, so we don't increment and see a bunch of '0's
    return 0;//assume they made a right move.
    
}

int validateInput( int piecetomovex, int piecetomovey, int piecenewx, int piecenewy, char piecetomove, int curdepth, int piecenum)
{//general method to validate input.
    //1 = person messed up.
    

	//debug
	//cout << "Piece isn't toppling itself " << char(piecetomovex + 'A') << " " << char(piecenewx + 'A') << " " << char( YWIDTH - piecetomovey + '0') << " " << char(YWIDTH - piecenewy + '0') << "\n";

    if (((piecetomovex - piecenewx) == 0 && (piecetomovey - piecenewy) == 0) 
        || (boardarray[YWIDTH*piecenewy + piecenewx] == '+' || boardarray[YWIDTH*piecenewy + piecenewx] == '~'))
	{//if the piece was trying to capture a wall
		/*cout << "You cannot capture a wall piece.  Please re enter input.";
		printf("\n");*/
		return 1;
	}
	else if (piecetomove == 'x' || piecetomove == 'X')
    {//x wing, 
        return legalXWing(piecetomovex, piecetomovey, piecenewx, piecenewy, piecetomove, curdepth, piecenum);
    } 
	else if (piecetomove == 't' || piecetomove == 'T')
    {//tie fighter,
        return legalTieFighter(piecetomovex, piecetomovey, piecenewx, piecenewy, piecetomove, curdepth, piecenum);
    } 
	else
    {//if it isn't the player's stuff, it's something else.  And they can't mess with it
        /*cout << piecetomove << " is not your piece to move.  Please re enter input" << piecetomovex << " " << piecetomovey;
        printf("\n");*/
        return 1;
    }
    
}

int checkGameOver()
{//See if the game is over:  IF the death star is taken or no more legal moves
    if (boardarray[10] != '*')
    {//if either of the player's death stars are no longer death stars (can do this since the previous method validates that something
        //can be on this space
        //cout << "Game Over:  My Death Star is destroyed.  You win.";
        //printf("\n");
		return 1;
    } else if (boardarray[38] != '@')
	{
		//cout << "Game Over:  Your Death Star is destroyed.  I win.";
		//printf("\n");
		return 1;
	}
	return 0;

}

int checkNoMoves(int whichplayer, int curdepth)
{//see if there are no valid moves the player can make:  if so, end game
	if (movenum[curdepth] == 0)
    {//If the player cannot currently move any pieces.  Check list of legal moves, if there's nothing there, then game over, pal.
        cout << "Game Over:  No more pieces to move.";
        if (whichplayer == 0)
        {//if the human has no more moves
            cout << "I win";
            printf("\n"); 
			
        }
		else
        {//if this was the computer that had no more moves.
            cout << "You win";
            printf("\n");
        }
        return 1;
    }
	return 0;
}

void findHumanMoves(int curdepth)
{//finds the list of moves a human can make
	int piecenum = 0;//the piece that is being moved.
    for (;piecenum < NUMOFPIECES*2;piecenum++)
    {//get location:  increment by 2, since we're treating this similar to a 2d array, first part y pos, second part x pos
       
        
        //here, we only need to check four cases:  The absolute limits of the x wing's position to move.
        //We let the validation add the list of moves.
		
		
        
        
        //cout << "Current piecenum and capturedpieces " << piecenum << " " << capturedpieces[piecenum] <<  "\n";
		
        if (capturedpieces[piecenum] == 0 )
		{//if the piece still exists, I can tell since I force pieces out of the board.
            int piecetomovey = piecepositions[piecenum*2];
            int piecetomovex = piecepositions[piecenum*2+1];//offset, for x position.
            char piecetomove = boardarray[YWIDTH*piecetomovey + piecetomovex];
			//cout << "current stuff in findHumanMoves:  " << char(piecetomovex + 'A') << char(YWIDTH - piecetomovey + '0') << " " << piecetomove << "\n";
			
            
            
			if (piecenum < 4  && piecetomove == 'x')
            {//if this is a human x wing
                //cout << "x wing piecenum = " << piecenum << "\n";

                //top left and bottom right corner cases
                if (piecetomovex <= piecetomovey)
                {//if x will hit 0 before y in the top left corner.
                    //top left, x is always 0
                    validateInput(piecetomovex, piecetomovey, 0, piecetomovey - piecetomovex, piecetomove, curdepth, piecenum);
                    //bottom right, y is always YWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, piecetomovex + ((YWIDTH - 1) - piecetomovey), YWIDTH - 1, piecetomove, curdepth, piecenum);
                    
                }
                else
                {//if y would hit 0 before x in the top left corner.
                    //top left, y is always 0
                    validateInput(piecetomovex, piecetomovey, piecetomovex - piecetomovey, 0 , piecetomove, curdepth, piecenum);
                    //bottom right, x is always XWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, XWIDTH - 1, piecetomovey + ((XWIDTH - 1) - piecetomovex) , piecetomove, curdepth, piecenum);
                }
                //top right and bottom left corner cases
                if (piecetomovex <= ((YWIDTH - 1) - piecetomovey))
                {//if y would hit the 0 before x reaches XWIDTH.
                    //top right, y is always 0
                    validateInput(piecetomovex, piecetomovey, piecetomovex + piecetomovey, 0, piecetomove, curdepth, piecenum); 
                    //bottom left, x is always 0
                    validateInput(piecetomovex, piecetomovey, 0, piecetomovey + piecetomovex, piecetomove, curdepth, piecenum);                    
                }
                else
                {//if x will hit the XWIDTH before y hits 0.
                    //top right, x is always XWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, XWIDTH - 1, piecetomovey - ((XWIDTH - 1) - piecetomovex), piecetomove, curdepth, piecenum);    
                    //bottom left, y is always YWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, piecetomovex - ((YWIDTH - 1) - piecetomovey), YWIDTH - 1, piecetomove, curdepth, piecenum);    
                    
                }
                
                
                //deprecated: Not going to brute force search for possible moves.
                /*for (int xstep = 0; xstep < XWIDTH; xstep++)
                {//look at all x points
                    for (int ystep = 0; ystep < YWIDTH; ystep++)
                    {//look at all y points.
                        //cout << "Checking newx " << xstep << " newy " << ystep << "\n";//debug
                        validateInput(piecetomovex, piecetomovey, xstep, ystep, piecetomove, curdepth, piecenum);
                    }
                }*/
            }			
			
			if (piecenum>= 4 && piecetomove == 't')
			{//if this is a tie fighter, do the optimized validate input based on the fact that it can move vertical or horizontal only, so go to the board limits and find out the values.
				//basically, check the edges of the map that the rook can move to (max and min horizontal no y change, and vice versa.)
				validateInput(piecetomovex, piecetomovey, 0, piecetomovey, piecetomove, curdepth, piecenum );
				validateInput(piecetomovex, piecetomovey, XWIDTH-1, piecetomovey, piecetomove, curdepth, piecenum  );
				validateInput(piecetomovex, piecetomovey, piecetomovex, 0, piecetomove, curdepth, piecenum );
				validateInput(piecetomovex, piecetomovey, piecetomovex, YWIDTH-1, piecetomove, curdepth, piecenum );				
			}
			

        }
		
    }
	
    

 
}

void findComputerMoves(int curdepth)
{//finds the list of moves a computer can make
	int piecenum = 8;//here, we start after the halfway point, which happens to be NUMOFPIECES*8/4
	
	
	
	
    for (;piecenum < NUMOFPIECES*4;piecenum++)
    {//get location:  increment by 2, since we're treating this similar to a 2d array, first part y pos, second part x pos
		//cout << "Current piecenum and capturedpieces " << piecenum << " " << capturedpieces[piecenum] << " and " << capturedpieces[piecenum] << "\n";


		if (capturedpieces[piecenum] == 0 )
		{//if the piece still exists, I can tell since I force pieces out of the board.
			//make sure to check that the piece isn't captured and the piece is proper.
			//Here, we do the sloppy way, check every element.

            int piecetomovey = piecepositions[piecenum*2];
            int piecetomovex = piecepositions[piecenum*2+1];//offset, for x position.
            char piecetomove = boardarray[YWIDTH*piecetomovey + piecetomovex];
            
			if (piecenum < 12 && piecetomove == 'X')
            {
              //top left and bottom right corner cases
                if (piecetomovex <= piecetomovey)
                {//if x will hit 0 before y in the top left corner.
                    //top left, x is always 0
                    validateInput(piecetomovex, piecetomovey, 0, piecetomovey - piecetomovex, piecetomove, curdepth, piecenum);
                    //bottom right, y is always YWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, piecetomovex + ((YWIDTH - 1) - piecetomovey), YWIDTH - 1, piecetomove, curdepth, piecenum);
                    
                }
                else
                {//if y would hit 0 before x in the top left corner.
                    //top left, y is always 0
                    validateInput(piecetomovex, piecetomovey, piecetomovex - piecetomovey, 0 , piecetomove, curdepth, piecenum);
                    //bottom right, x is always XWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, XWIDTH - 1, piecetomovey + ((XWIDTH - 1) - piecetomovex) , piecetomove, curdepth, piecenum);
                }
                //top right and bottom left corner cases
                if (piecetomovex <= ((YWIDTH - 1) - piecetomovey))
                {//if y would hit the 0 before x reaches XWIDTH.
                    //top right, y is always 0
                    validateInput(piecetomovex, piecetomovey, piecetomovex + piecetomovey, 0, piecetomove, curdepth, piecenum); 
                    //bottom left, x is always 0
                    validateInput(piecetomovex, piecetomovey, 0, piecetomovey + piecetomovex, piecetomove, curdepth, piecenum);                    
                }
                else
                {//if x will hit the XWIDTH before y hits 0.
                    //top right, x is always XWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, XWIDTH - 1, piecetomovey - ((XWIDTH - 1) - piecetomovex), piecetomove, curdepth, piecenum);    
                    //bottom left, y is always YWIDTH - 1
                    validateInput(piecetomovex, piecetomovey, piecetomovex - ((YWIDTH - 1) - piecetomovey), YWIDTH - 1, piecetomove, curdepth, piecenum);    
                    
                }
                   
                
            }
			
			
			if (piecenum >= 12 && piecetomove == 'T')
			{//if this is a tie fighter, do the optimized validate input based on the fact that it can move vertical or horizontal only, so go to the board limits and find out the values.
				validateInput(piecetomovex, piecetomovey, 0, piecetomovey, piecetomove, curdepth, piecenum );
				validateInput(piecetomovex, piecetomovey, XWIDTH-1, piecetomovey, piecetomove, curdepth, piecenum  );
				validateInput(piecetomovex, piecetomovey, piecetomovex, 0, piecetomove, curdepth, piecenum );
				validateInput(piecetomovex, piecetomovey, piecetomovex, YWIDTH-1, piecetomove, curdepth, piecenum );				
			}
						
			//cout << "current stuff in findComputerMoves:  " << char(piecetomovex + 'A') << char(YWIDTH - piecetomovey + '0') << " " << piecetomove << "\n";          
        }
    }
	

}


int checkListOfMoves()
{//check the list of moves with this, see if any of them are equal to the user's input.  Only to be used with human input.
	//cout << "Checking list of legal moves " << (movenum[curdepth]/4) << "\n";//divide by offset, oldx oldy newx newy(4)
    //cout << "List of Legal Moves:  ";
    for (int counter = 0; counter < movenum[0]; counter = counter + 5)
    {//increment through each move, showing them.  Each move is four elements, so increment by that many elements.
        //char xold = listoflegalmoves[counter] + 'A';//just like with int to char, need to displace by ASCII text
        //char yold = (YWIDTH - listoflegalmoves[counter+1]) + '0';// to get inverse, a = width - b
        //char xnew = listoflegalmoves[counter+2] + 'A';
        //char ynew = (YWIDTH - listoflegalmoves[counter+3]) + '0';
    
        //cout << " " << xold << "" << yold << "" << xnew << "" << ynew;
		
		if (movestack[0] == listoflegalmoves[counter] && movestack[1] == listoflegalmoves[counter+1] 
			&& movestack[2] == listoflegalmoves[counter+2] && movestack[3] == listoflegalmoves[counter+3])
		{//if the move the person entered matches one of the valid moves.
			humanmovenum = counter;//to see if a horizontal move was made in main method.
			//cout << "Humanmovenum at checklistofmoves = " << humanmovenum << "\n";			
			return listoflegalmoves[counter+4];//success, return the piecenum to be used in movepiece at the main method. 
		}
		
    }
    //printf("\n");
	return -1;
}

int checkListOfHorizontalMoves(int movenumber, int curdepth)
{//check to see if the move was horizontal, to be used in minimax algorithm.
	//cout << "              horizontalmovenum[curdepth] = " << horizontalmovenum[curdepth] << " and curdepth = " << curdepth << "\n";
	for (int counter = curdepth*24; counter < horizontalmovenum[curdepth]; counter++)
	{//iterate through list of horizontal moves 
		//cout << "Movenumber is " << movenumber << " and listofhorizontalmoves at counter is " 
		//	<< listofhorizontaltiemoves[counter] << " with counter as " << counter << " \n";//debug
		if (movenumber == listofhorizontaltiemoves[counter])
		{//if the move was indeed a horizontal one
			//cout << "This move is horizontal at curdepth " << curdepth << "\n";
			return 1;
		}
	}
	return 0;
}


void showListOfMoves(int curdepth)
{//show the list of legal moves to immediately make;
    cout << "Number of legal moves at listindex "<< curdepth*LISTSIZE << " is " << (movenum[curdepth]/5) << "\n";//divide by offset, oldx oldy newx newy(4)
    cout << "List of Legal Moves:  ";
	int counter = 0;
    for (; counter < movenum[curdepth]; counter = counter + 5)
    {//increment through each move, showing them.  Each move is four elements, so increment by that many elements.
        char xold = listoflegalmoves[LISTSIZE*curdepth + counter] + 'A';//just like with int to char, need to displace by ASCII text
        char yold = (YWIDTH - listoflegalmoves[LISTSIZE*curdepth + counter+1]) + '0';// to get inverse, a = width - b
        char xnew = listoflegalmoves[LISTSIZE*curdepth + counter+2] + 'A';
        char ynew = (YWIDTH - listoflegalmoves[LISTSIZE*curdepth + counter+3]) + '0';
		//int piecemoved = listoflegalmoves[LISTSIZE*curdepth + counter + 4];//debug for which piece moved.
        cout << " " << xold  << yold  << xnew  << ynew ;//<< " piecenum: " << piecemoved;//debug after comment , before << that's before piecenum
		
		showNewMovesOnBoard(listoflegalmoves[curdepth + counter+2], listoflegalmoves[curdepth + counter+3]);
    }
    printf("\n");
}

void showAllMoves()
{//Debug, to show all moves on the array, for each depth.
    for (int counter = 0; counter < MAXDEPTH; counter++)
    {//look at each movenum, and see list of moves per thingy.
        if (movenum[counter] == 0)
        {//if that depth had no moves, end.  No need to show.
            break;
        }
        showListOfMoves(counter);
        //cleanBoard();
    }
}


void showNewMovesOnBoard(int newx, int newy)
{//show graphically the possible moves
	if (boardarray[newy*YWIDTH + newx] == EMPTYCHAR)
	{//if this is a blank spot, then we can replace:  Don't replace any pieces.
		boardarray[newy*YWIDTH + newx] = MOVECHAR;//show on board the movement possibilities
	}
}

void cleanBoard()
{//clean the board of possible moves
	for (int xloc = 0; xloc < XWIDTH; xloc++)
	{//Do the lazy look at the whole board thing.
		for (int yloc = 0; yloc < YWIDTH; yloc++)
		{
			if (boardarray[yloc*YWIDTH + xloc] == MOVECHAR)
			{//if this was considered a possible move, reset it.  Might not be possible next turn.
				boardarray[yloc*YWIDTH + xloc] = EMPTYCHAR;
			}
		}
	}
}


/*void showPieces()
{//debug function.  Show piece's status, current location and if it is captured.
	int piecenum = 0;//piecenum
    for (int counter = 0; counter < NUMOFPIECES*2;  counter += 2)
    {
        cout << "Human X Wing Position:  " << char(piecepositions[piecenum*2+1]  + 'A') << char(YWIDTH - piecepositions[piecenum*2] + '0');
        cout << " Capture Status:  " << capturedpieces[piecenum] << "\n";
        cout << "Human Tie Position:  " << char(piecepositions[piecenum*2+9]  + 'A') << char(YWIDTH - piecepositions[piecenum*2+8] + '0');
        cout << " Capture Status:  " << capturedpieces[piecenum+4] << "\n";
        cout << "Computer x wing Position:  " << char(piecepositions[piecenum*2+17]  + 'A') << char(YWIDTH - piecepositions[piecenum*2+16] + '0');
        cout << " Capture Status:  " << capturedpieces[piecenum+8] << "\n";
        cout << "Computer tie Position:  " << char(piecepositions[piecenum*2+25]  + 'A') << char(YWIDTH - piecepositions[piecenum*2+24] + '0');
        cout << " Capture Status:  " << capturedpieces[piecenum+12] << "\n";
		
		piecenum++;//increment piece number
    }
    cout << "CaptureIndicator " << captureindicator << "\n";


	
}*/

/*void showListStack(int curdepth)
{//debug function.  show the stack of moves that caused the error.
    cout << "Stack:  \n";
    for (int counter = 0; counter < curdepth+1; counter++)
    {   
        cout << char(movestack[counter*6] + 'A') << char(YWIDTH - movestack[counter*6 + 1] + '0') << char(movestack[counter*6 + 2] + 'A') 
			<< char(YWIDTH - movestack[counter*6 + 3] + '0') << " " << char(movestack[counter*6 + 4]) << " " << char(movestack[counter*6 + 5]) << "\n";
        showListOfMoves(counter);
    }
}*/

int maxMove(int curdepth, int beta)
{//computer (best) move
    //cout << "MaxalgoDepth " << curdepth << "\n";
	int temphorizontal = horizontalcomputer;//placeholder, to make sure it doesn't mess up too much.	
	int best = BELOWWORST;
    movenum[curdepth] = 0;//haven't found a list of moves yet.   
	horizontalmovenum[curdepth] = 0;//haven't had a list of horizontal tie moves yet, either
	horizontalhuman = temphorizontal;//replace the piece, since the above messes with it.	
	horizontalcomputer = horizontalcomputer - 1;
	if (curdepth == MAXDEPTH)
	{//if we reached the end of the depth we can search, evaluate this move's heuristic value
        //cout << "evaluate\n";
		horizontalcomputer = temphorizontal;
		return evaluate( curdepth );
	}
    if (checkGameOver() == 1)
    {//if it was game over here, then the human won.
		horizontalcomputer = temphorizontal;	
        return BELOWWORST + 1 + curdepth;//use curdepth to indicate how much more winning it is:  earlier win(lower curdepth) = better
    }
	findComputerMoves(curdepth);
	//showListOfMoves(curdepth);//debug
	
    if (movenum[curdepth] == 0)
    {
		horizontalcomputer = temphorizontal;	
        return BELOWWORST + 1 + curdepth;
    }
    
    
    

	for (int movecounter = LISTSIZE*curdepth; movecounter < LISTSIZE*curdepth + movenum[curdepth]; movecounter = movecounter + 5)
	{//go through each move, and pretend to move the piece.
		//char xold = listoflegalmoves[movecounter] + 'A';//just like with int to char, need to displace by ASCII text
        //char yold = (YWIDTH - listoflegalmoves[movecounter+1]) + '0';// to get inverse, a = width - b
        //char xnew = listoflegalmoves[movecounter+2] + 'A';
        //char ynew = (YWIDTH - listoflegalmoves[movecounter+3]) + '0';
        //cout << " " << xold << "" << yold << "" << xnew << "" << ynew;//Debug, to see moves made
		
		//put the move on the stack
		movestack[movestackoff] = listoflegalmoves[movecounter];
		movestack[movestackoff + 1] = listoflegalmoves[movecounter+1];
		movestack[movestackoff+2] = listoflegalmoves[movecounter+2];
		movestack[movestackoff+3] = listoflegalmoves[movecounter+3];
		movestack[movestackoff+4] = boardarray[yoffoldy + movestack[movestackoff]];
        movestack[movestackoff + 5] = boardarray[yoffnewy + movestack[movestackoff+2]];        
		//char charx =  movestack[movestackoff] + 'A';//debug
		//char chary =  YWIDTH - movestack[movestackoff + 1] + '0';
		//cout << "piecetomove = " << piecetomove << " at " << charx << chary << " in depth " << curdepth << "\n";//debug
		//cout << "Movenumber to go into method is " << movecounter % (LISTSIZE*curdepth) << "\n";//debug
		if (movestack[movestackoff + 4] == 'T' && horizontalcomputer != 1 && checkListOfHorizontalMoves(movecounter % (LISTSIZE*curdepth),curdepth) == 1)
		{//if this was a horizontal move, pretend it was one by setting the horizontal value.
			horizontalcomputer = 2;
		}
		
		
		movePiece(curdepth, listoflegalmoves[movecounter+4]);//pretend to move the piece
		int score = minMove(curdepth + 1, best);//go to max move, and increment depth by one.
		
		if (score > best)
		{//if the score is better than the best move
            //cout << "Found a better move. at " << curdepth << " with value of " << score << " \n";
			best = score;//change best to current score.
            
		}
        //printBoard();//debug
		resetPiecePosition(1, curdepth, listoflegalmoves[movecounter+4]);
		//reset the horizontal value.
		if (movestack[movestackoff + 4] == 'T' && checkListOfHorizontalMoves(movecounter,curdepth) == 1)
		{//if this was a horizontal move, stop pretending it was one by resetting the horizontal value.
			cout << "resetting horizontal tie computer\n";
			horizontalcomputer = temphorizontal;
		}
		
        if (best >= beta)
        {//if the best score will already not matter, just return best:  This value will never change beta, since beta is looking for values smaller
			horizontalcomputer = temphorizontal;
            return best;
        }
        //printBoard();//debug
		//cout << "\n";//debug
	}
    
    //cout << "\n";//debug
	horizontalcomputer = temphorizontal;	
	return best;
}

int minMove(int curdepth, int alpha)
{//human (worst) move   
    //cout << "MinalgoDepth " << curdepth << "\n";	
	int temphorizontal = horizontalhuman;//placeholder, to make sure it doesn't mess up too much.		
	int worst = ABOVEBEST;
    movenum[curdepth] = 0;//haven't found a list of moves yet.    
	//cout << "  MINMOVES horizontalhuman = " << horizontalhuman << " curdepth "  << curdepth << " \n";	
	horizontalmovenum[curdepth] = 0;//haven't had a list of horizontal tie moves yet, either	
	//TODO for some reason, this line above is changing the value of horziontalhumnan.  I don't know why.
	horizontalhuman = temphorizontal;//replace the piece, since the above messes with it.
	//cout << "   MINMOVES horizontalhuman = " << horizontalhuman << "\n";
	horizontalhuman = horizontalhuman - 1;//pretend to decrement.
	
	if (curdepth >= MAXDEPTH)
	{//if we reached the end of the depth we can search, evaluate this move's heuristic value.
        //cout << "Evaluate\n";
		horizontalhuman = temphorizontal;		
		return evaluate( curdepth );
	}
    else if (checkGameOver() == 1)
    {//if a game over occurred, then the computer won.
		horizontalhuman = temphorizontal;			
        return ABOVEBEST - (1+curdepth);
    }
	findHumanMoves(curdepth);
	//showListOfMoves(curdepth);//debug
	
    if (movenum[curdepth] == 0)
    {
		horizontalhuman = temphorizontal;			
        return ABOVEBEST - (1+curdepth);
    }
    
    
	for (int movecounter = LISTSIZE*curdepth; movecounter < LISTSIZE*curdepth + movenum[curdepth]; movecounter = movecounter + 5)
	{//go through each move, and pretend to move the piece.

		//put the move on the stack
		movestack[movestackoff] = listoflegalmoves[movecounter];
		movestack[movestackoff + 1] = listoflegalmoves[movecounter+1];
		movestack[movestackoff+2] = listoflegalmoves[movecounter+2];
		movestack[movestackoff+3] = listoflegalmoves[movecounter+3];
		movestack[movestackoff+4] = boardarray[yoffoldy + movestack[movestackoff]];
        movestack[movestackoff + 5] = boardarray[yoffnewy + movestack[movestackoff+2]];  
        //char charx =  movestack[movestackoff] + 'A';//debug
		//char chary =  YWIDTH - movestack[movestackoff + 1] + '0';
		//cout << "piecetomove = " << piecetomove << " at " << charx << chary << "in depth " << curdepth << " \n";//debug
		//cout << "Movenumber to go into method at minmove is " << movecounter % (LISTSIZE*curdepth) << "\n";//debug

		if (movestack[movestackoff + 4] == 't' && horizontalhuman != 1 &&checkListOfHorizontalMoves(movecounter % (LISTSIZE*curdepth),curdepth) == 1)
		{//if this was a horizontal move, pretend it was one by setting the horizontal value.
			horizontalhuman = 2;
		}
		
		
		movePiece(curdepth, listoflegalmoves[movecounter+4]);//pretend to move the piece
		int score = maxMove(curdepth + 1, worst);//go to max move, and increment depth by one.
		
		if (score < worst)
		{//if the score is better than the best move
            //cout << "Found a worse move at " << curdepth << ".  With value of "<< score << " \n";
			worst = score;//change best to current score.
            
		}
        //printBoard();//debug        
		resetPiecePosition(0,  curdepth, listoflegalmoves[movecounter+4]);
		if (movestack[movestackoff + 4] == 't' && checkListOfHorizontalMoves(movecounter % (LISTSIZE*curdepth),curdepth) == 1)
		{//if this was a horizontal move, pretend it was one by setting the horizontal value.
			horizontalhuman = temphorizontal;
		}
        //printBoard();//debug
		//cout << "\n";//debug
        if (worst <= alpha)
        {//if this value will no longer affect the outcome, since this value will always be less than the alpha, and alpha doesn't change unless the value is greater.
			//cout << "pruning at minmove...\n";
			horizontalhuman = temphorizontal;		
            return worst;
        }
	}
	
	
    //cout << "\n";//debug
	horizontalhuman = temphorizontal;	
	return worst;
}

int makeAMove()
{//The computer make the move
    int best = BELOWWORST;
    //Now, make the algorithm
    int curdepth = 0;//since we start the algorithm here, we didn't go deeper.  But, as we search through the plies, this number will increase.
    movenum[curdepth] = 0;//haven't found a list of moves yet.
	horizontalmovenum[curdepth] = 0;//haven't had a list of horizontal tie moves yet, either	
	horizontalcomputer--;//decrement horizontal computer, since it technically has been past a turn.
	//this can be done before, since this is the real move.
	int temphorizontal = horizontalcomputer;//place holder, since recursion will alter horizontalcomputer, may not need.
	
	//Take a look at each of the computer's moves, based on their pieces.
	findComputerMoves(curdepth);//find list of computer's moves.
	showListOfMoves(curdepth);//debug, show list of computer's moves.
	if (checkNoMoves(1, 0) == 1)
	{//makes sure there is a list of moves.  If not, end the game.
		exit(0);//Opponent Won.
	}
	
	//temp piece:  Current best move.
	int bestpiecetomovex = listoflegalmoves[0];//go to first move, don't care otherwise.
	int bestpiecetomovey = listoflegalmoves[1];
	int bestpiecenewx = listoflegalmoves[2];
	int bestpiecenewy = listoflegalmoves[3];
	int bestpiecenum = listoflegalmoves[4];//the piecenum
	int piecenum = 0;
	//cout << "FIRST Current piece to move " << piecetomove << "\n";//debug
    int bestmovenum = 0;//for horizontal checking
	
	for (int movecounter = 0; movecounter < movenum[curdepth]; movecounter = movecounter + 5)
	{//go through each move, and pretend to move the piece.
		//char xold = listoflegalmoves[movecounter] + 'A';//just like with int to char, need to displace by ASCII text
        //char yold = (YWIDTH - listoflegalmoves[movecounter+1]) + '0';// to get inverse, a = width - b
        //char xnew = listoflegalmoves[movecounter+2] + 'A';
        //char ynew = (YWIDTH - listoflegalmoves[movecounter+3]) + '0';
        //cout << " " << xold << "" << yold << "" << xnew << "" << ynew;//Debug, to see moves made
		//cout << "HORIZONTAL HUMAN IN MAKEAMOVE LOOP CURRENTLY " << horizontalhuman << "\n";//debug
				
        movestack[0] = listoflegalmoves[movecounter];
        movestack[1] = listoflegalmoves[movecounter+1];
        movestack[2] = listoflegalmoves[movecounter+2];
        movestack[3] = listoflegalmoves[movecounter+3];
        movestack[4] = boardarray[yoffoldy + movestack[movestackoff]];
        movestack[5] = boardarray[yoffnewy + movestack[movestackoff+2]];
		piecenum = listoflegalmoves[movecounter+4];
		/*char charx =  movestack[movestackoff] + 'A';
		char chary =  YWIDTH - movestack[movestackoff + 1] + '0';
		cout << "piecetomove = " << piecetomove << " at " << charx << chary << "\n";*///debug
		if (movestack[movestackoff + 4] == 'T' && checkListOfHorizontalMoves(movecounter ,curdepth) == 1)
		{//if this was a horizontal move, pretend it was one by setting the horizontal value.
			horizontalcomputer = 2;
		}
		
		movePiece( curdepth, listoflegalmoves[movecounter+4]);//pretend to move the piece
		int score = minMove(curdepth + 1, best);//go to min move, and increment depth by one.
		
		if (score > best)
		{//if the score is better than the best move
            //cout << "Found a better move. \n";
			best = score;//change best to current score.
			bestpiecetomovex = listoflegalmoves[movecounter];
			bestpiecetomovey = listoflegalmoves[movecounter+1];
			bestpiecenewx = listoflegalmoves[movecounter+2];
			bestpiecenewy = listoflegalmoves[movecounter+3];
			bestpiecenum = listoflegalmoves[movecounter+4];
			bestmovenum = movecounter;
		}
		resetPiecePosition(1, curdepth, listoflegalmoves[movecounter+4]);
		if (movestack[movestackoff + 4] == 'T' && horizontalcomputer != 1 && checkListOfHorizontalMoves(movecounter ,curdepth) == 1)
		{//if this was a horizontal move, pretend it was one by setting the horizontal value.
			horizontalcomputer = temphorizontal;
		}
		//cout << "\n";//debug
	}
	
	
	char xold = bestpiecetomovex + 'A';//just like with int to char, need to displace by ASCII text
	char xoldinv = 'G' - bestpiecetomovex;
    char yold = (YWIDTH - bestpiecetomovey) + '0';// to get inverse, a = width - b
	char yoldinv = '8' - (YWIDTH - bestpiecetomovey);
    char xnew = bestpiecenewx + 'A';
	char xnewinv = 'G' - bestpiecenewx;
    char ynew = (YWIDTH - bestpiecenewy) + '0';
	char ynewinv = '8' - (YWIDTH - bestpiecenewy);
    //cout << " " << xold << "" << yold << "" << xnew << "" << ynew;//Debug, to see moves made
	
	//Now make the real move.  Put the best move on the stack, to use it
	movestack[0] = bestpiecetomovex;
	movestack[1] = bestpiecetomovey;
	movestack[2] = bestpiecenewx;
	movestack[3] = bestpiecenewy;
	movestack[4] = boardarray[yoffoldy + movestack[movestackoff]];
    movestack[5] = boardarray[yoffnewy + movestack[movestackoff+2]];
	movePiece(curdepth, bestpiecenum);//now really move the piece.
	if (movestack[movestackoff + 4] == 'T' && checkListOfHorizontalMoves(bestmovenum,curdepth) == 1)
	{//if this was a horizontal move, now ACTUALLY set the value.  No need to mod by LISTSIZE*curdepth, since the start is 0
		horizontalcomputer = 2;
	}
	
    cout << "I made my move " << xold << yold << xnew << ynew << " ( " << xoldinv << yoldinv << xnewinv << ynewinv << " )\n";
	cout << "evaluation result is " << best << "\n";
    //showPieces();//debug
	//showListStack(MAXDEPTH);//debug
    //showAllMoves();//debug
	return best;
}


int getHumanMove()
{//get the human move:
    /*Do this by first getting the piece based on the position (x and y values)
        Then, move that piece.  The validationchecking makes sure that the move works.
    */
	movenum[0] = 0;//start with no legal moves
    findHumanMoves(0);//find the list of moves a human can make.
    //cout << "finished finding human moves\n";//debug
	   
    showListOfMoves(0);//show the list of moves.
	if (checkNoMoves(0, 0) == 1)
	{//makes sure there is a list of moves.  If not, end the game.
		exit(0);//AI won.
	}
	printBoard();//Show the board state, after the moves have been shown.
    //ask user for x and y coordinate of piece.
    cout << "Enter the play to be made (Current xy, then New xy):  ";
    scanf("%s", userinput);
    
    movestack[0] = userinput[0] - 'A';//Since this is ASCII, i'll just minus by 'A'(65), to get the ascii to 0, then add 1.  The difference
    //is the actual spot, ex. 'C' - 'A' = 2, all nice and dandy :) .
    movestack[1] = YWIDTH - (userinput[1] - '0');//Since the input is one off our array, just minus by one, the displacement.
    //However, since this is in ASCII, we minus by ASCII 1.
    //We also have to inverse the values:  1 = 6, 2 = 5, 7 = 0 etc.
    //(a + a') mod b = 0
    //b - a = a'
    movestack[2] = userinput[2] - 'A';//Same as movestack[0];
    movestack[3] = YWIDTH - (userinput[3] - '0');//Same as movestack[0 + 1];

    movestack[4] = boardarray[YWIDTH*movestack[1] + movestack[0]];//get the piece

    int imessup = 0;//indicates the person messed up
	imessup = checkListOfMoves();//check the move based on the list of legal moves
    //I could just put this here, to waste time, instead of check based on list of moves.  Hehehe...
    return imessup;
    
}


void movePiece(int curdepth, int piecenum)
{//move the piece to the new location, and have the old location replaced by a blank space
	//if trumove is 1, then we also look for the piece that moved:  Otherwise, ignore it
	//shouldn't have to do above, so commented out.
	
	//update the piece position on list of piece positions.
	//do piecenum*2 because if you don't you'll interfere with the next piece's location.  
	// ex.  piecenum = 0  1    2  3 
	//				  [x1 y1] [x2 y2]
	piecepositions[piecenum*2] = movestack[movestackoff + 3];//new y location.
	piecepositions[piecenum*2 + 1] = movestack[movestackoff + 2];//new x location.
	//cout << "piecenum move " << piecenum << " moved to " << char(piecepositions[piecenum*2 + 1] + 'A') << char(YWIDTH - piecepositions[piecenum*2] + '0') << "\n";
	
	
	checkPieceRemoved(curdepth, piecenum);
	
    //do this before actually swapping, or error will occur (checkpiece will check this piece moving).
    
    boardarray[yoffnewy + movestack[movestackoff+2]] = movestack[movestackoff + 4];//replace the new spot with the piece
    boardarray[yoffoldy + movestack[movestackoff]] = EMPTYCHAR;//clear the old place with a blank spot.
    //don't replace it with a potential movestack[movestackoff + 5];  That's the piece being captured.  don't flip em.
}

//if whichplayer = 0, it is human.  Computer otherwise.
void resetPiecePosition( int whichplayer, int curdepth, int piecenum)
{//undo the piece move, rather than a whole board move
    //cout << "resetting piece position\n";//debug
	
	//update the piece position on list of piece positions:  Go backwards
	piecepositions[piecenum*2] = movestack[movestackoff + 1];//old y location
	piecepositions[piecenum*2+1] = movestack[movestackoff];
	//cout << "piecenum undone move " << piecenum << " moved to " << char(piecepositions[piecenum*2 + 1] + 'A') << char(YWIDTH - piecepositions[piecenum*2] + '0') << "\n";
	
	char piecetolife = EMPTYCHAR;//the piece that will replace the undone location (newx and newy)
		
	if (movestack[movestackoff + 5] < 16)
	{
		capturedpieces[movestack[movestackoff + 5]] = 0;//reset piece captured: It is no longer captured.
		if (movestack[movestackoff + 5] < 4)
		{//if this is a human x wing
			//cout << "Uncapturing human x wing\n";
			piecetolife = 'x';
		}
		else if (movestack[movestackoff + 5] < 8)
		{
			//cout << "Uncapturing human tie\n";
			piecetolife = 't';
		}
		else if (movestack[movestackoff + 5] < 12)
		{
			//cout << "Uncapturing comp x wing\n";		
			piecetolife = 'X';
		}
		else
		{
			//cout << "Uncapturing comp tie fighter\n";		
			piecetolife = 'T';
		}
	}


	
	//cout << "printing 'o' at " << char(movestack[movestackoff+2]+ 'A') << char(YWIDTH - movestack[movestackoff+3] + '0') << "\n";//debugt
	boardarray[yoffnewy + movestack[movestackoff+2]] = piecetolife;//replace the new spot with the old spot, which was the 'captured' piece.
	
	
	//cout << "Current piece to reset is " << piecetomove << "\n";          //movestack[5*curdepth + 4]
    boardarray[yoffoldy + movestack[movestackoff]] = movestack[movestackoff + 4];//replace the old spot with the piece originally there..

	boardarray[10] = '*';//just reset these, just because I know that these will always be reverted.
	boardarray[38] = '@';
}

int checkPieceRemoved(int curdepth, int piecenum)
{//checks to see if a piece was removed, or rather, if the new location interfered with what the old location was.
	//cout << "CHecking piece captured...\n";//debug
    for (int piecetocheck = 0; piecetocheck < NUMOFPIECES; piecetocheck ++)
	{//check each piece to see if their old move matches with the move actually made.  Also, make sure the piece isn't captured, it's redundant
		//also...make sure the piece doesn't capture itself (piecetocheck != piecenum)
		if (capturedpieces[piecetocheck] == 0 && piecetocheck != piecenum 
            && movestack[movestackoff+3] == piecepositions[piecetocheck*2] && movestack[movestackoff + 2] == piecepositions[piecetocheck*2+1] 
			)
		{//if the human x wing piece was taken (new position is equal to where they are now).

			capturedpieces[piecetocheck] = captureindicator;
            captureindicator++;
            movestack[movestackoff + 5] = piecetocheck;//put the piecenum that was captured, so we can easily undo the capture if it occurred.
            
            //cout << "captureing: captureindicator is now " << captureindicator << " by " << piecenum << "\n";          
			//cout << "Human x wing " << piecetocheck << " captured at " << char(movestack[movestackoff+2] + 'A') << char(YWIDTH - movestack[movestackoff+3] + '0') << " from " 
			//	<< char(movestack[movestackoff] + 'A') << char(YWIDTH - movestack[movestackoff+1] + '0') << "\n";//debug
			return 0;
		}
		else if (capturedpieces[piecetocheck+4] == 0 && piecetocheck + 4 != piecenum 
            && movestack[movestackoff+3] == piecepositions[piecetocheck*2+8] && movestack[movestackoff + 2] == piecepositions[piecetocheck*2+9] 
			)
		{//human TIE fighter

			capturedpieces[piecetocheck + 4] = captureindicator;
            captureindicator++;
            movestack[movestackoff + 5] = piecetocheck+4;

            //cout << "captureing:  captureindicator is now " << captureindicator << " by " << piecenum << "\n";      
            //cout << "Human Tie fighter " << counter << " captured at " << char(movestack[movestackoff+2] + 'A') << char(YWIDTH - movestack[movestackoff+3] + '0') <<"\n";//debug
			return 0;
		}
		else if (capturedpieces[piecetocheck+8] == 0 && piecetocheck + 8 != piecenum 
            && movestack[movestackoff+3] == piecepositions[piecetocheck*2+16] && movestack[movestackoff + 2] == piecepositions[piecetocheck*2+17]
			)
		{//computer x wing

			capturedpieces[piecetocheck + 8] = captureindicator;
            captureindicator++;
            movestack[movestackoff + 5] = piecetocheck + 8;
            
            //cout << "captureing:   captureindicator is now " << captureindicator << " by " << char(piecetomove) << "\n";                    
			//cout << "Computer x wing " << counter << " captured at " << char(movestack[movestackoff+2] + 'A') << char(YWIDTH - movestack[movestackoff+3] + '0') <<"\n";//debug
			return 0;
		}
		else if (capturedpieces[piecetocheck + 12] == 0 && piecetocheck + 12 != piecenum 
            && movestack[movestackoff+3] == piecepositions[piecetocheck*2+24] && movestack[movestackoff + 2] == piecepositions[piecetocheck*2+25]
			)
		{//computer TIE fighter

			capturedpieces[piecetocheck + 12] = captureindicator;
            captureindicator++;
            movestack[movestackoff + 5] = piecetocheck + 12;
            
            //cout << "captureing:    captureindicator is now " << captureindicator << " by " << char(piecetomove) << "\n";             
			//cout << "Computer Tie fighter " << counter << " captured at " << char(movestack[movestackoff+2] + 'A') << char(YWIDTH - movestack[movestackoff+3] + '0') <<"\n";//debug
			return 0;
		}
	}
	movestack[movestackoff + 5] = 32;//dummy value, to revert to a blank space.
	return 1;//here, nothing was captured
}

void doubleCaptureIndicators()
{//double the capture values, so they do not get uncaptured.
    for (int counter = 0; counter < NUMOFPIECES*4; counter++)
    {
        if (capturedpieces[counter] < NUMOFPIECES*4)
        {
            capturedpieces[counter] = capturedpieces[counter] * NUMOFPIECES*4;//basically, go to a number such that this can never be unremoved
        }
    }

}

