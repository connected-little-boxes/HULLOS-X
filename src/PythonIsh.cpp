#include <Arduino.h>
#include "HullOSVariables.h"
#include "Utils.h"
#include "HullOSScript.h"
#include "PythonIsh.h"

#include <Arduino.h>

const char pythonishcommandNames[] =
	"angry,cross,mad#"	 // COMMAND_ANGRY       0
	"happy,pleased,glad#" // COMMAND_HAPPY       1
	"move#"			 // COMMAND_MOVE        2
	"turn#"			 // COMMAND_TURN        3
	"arc#"			 // COMMAND_ARC         4
	"delay#"		 // COMMAND_DELAY       5
	"colour#"		 // COMMAND_COLOUR      6
	"color#"		 // COMMAND_COLOR       7
	"pixel#"		 // COMMAND_PIXEL       8
	"set#"			 // COMMAND_SET         9
	"if#"			 // COMMAND_IF         10
	"do#"			 // COMMAND_DO         11
	"while#"		 // COMMAND_WHILE      12
	"intime#"		 // COMMAND_INTIME     13
	"endif#"		 // COMMAND_ENDIF      14
	"forever#"		 // COMMAND_FOREVER    15
	"endwhile#"		 // COMMAND_ENDWHILE   16
	"sound#"		 // COMMAND_SOUND      17
	"until#"		 // COMMAND_UNTIL      18
	"clear#"		 // COMMAND_CLEAR      19
	"run#"			 // COMMAND_RUN        20
	"background#"	 // COMMAND_BACKGROUND 21
	"else#"			 // COMMAND_ELSE       22
	"red#"			 // COMMAND_RED        23
	"green#"		 // COMMAND_GREEN      24
	"blue#"			 // COMMAND_BLUE       25
	"yellow#"		 // COMMAND_YELLOW     26
	"magenta#"		 // COMMAND_MAGENTA    27
	"cyan#"			 // COMMAND_CYAN       28
	"white#"		 // COMMAND_WHITE      29
	"black#"		 // COMMAND_BLACK      30
	"wait#"			 // COMMAND_WAIT       31
	"stop#"			 // COMMAND_STOP       32
	"begin#"		 // COMMAND_BEGIN      33
	"end#"			 // COMMAND_END        34
	"write#"		 // COMMAND_PRINT      35
	"print,scream,shout,whisper,say#"		 // COMMAND_PRINTLN    36
	"break#"		 // COMMAND_BREAK      37
	"duration#"		 // COMMAND_DURATION   38
	"continue#"		 // COMMAND_CONTINUE   39
	"angle#"		 // COMMAND_ANGLE      40
	"dance#"		 // COMMAND_DANCE      41
	"an,a,the,my,your,our#"	// COMMON_VARIABLE 42
	;



const char angryCommand[] = "PF20";

int compileAngry()
{
#ifdef SCRIPT_DEBUG
	Serial.println(F("Compiling angry: "));
#endif // SCRIPT_DEBUG

	sendCommand(angryCommand);
	previousStatementStartedBlock = false;
	return ERROR_OK;
}

const char happyCommand[] = "PF1";

int compileHappy()
{
#ifdef SCRIPT_DEBUG
	Serial.println(F("Compiling happy: "));
#endif // SCRIPT_DEBUG

	sendCommand(happyCommand);
	previousStatementStartedBlock = false;
	return ERROR_OK;
}

const char completeAwaitCommand[] = "CA";

int handleInTime()
{
	skipInputSpaces(); // find the next character

	if (*bufferPos != 0)
	{
		// Spin further down the commands looking for an intime command
		int command = decodeCommandName(pythonishcommandNames);

		if (command == COMMAND_INTIME) // 13 is the offset in the command names of the intime word
		{
			outputFunction(',');

			skipInputSpaces();

			if (*bufferPos == 0)
				return ERROR_MISSING_TIME_VALUE_IN_INTIME; // missing time number

			int result = processValue();

			if (result != ERROR_OK)
				return result;
		}
		else
		{
			return ERROR_INTIME_EXPECTED_BUT_NOT_SUPPLIED;
		}
	}

	// always send a wait command

	endCommand(); // end the movement command
	sendCommand(completeAwaitCommand);

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

// The value to be supplied gives a limit of some kind, either distance or angle
// If the value is missed off the program provides a large default value and the
// the program does not pause during the move. If the distance value is given this
// will cause the program to pause until the distance has been travelled

const char largeLimitValue[] = "20000";

int handleValueIntimeAndBackground()
{
	if (*bufferPos == 0)
	{
		// No value, that's fine - just out the large limit
		sendCommand(largeLimitValue);
		return ERROR_OK;
	}
	else
	{
		// have a value - process it
		skipInputSpaces();

		int result = processValue();

		if (result != ERROR_OK)
			return result;
	}

	return handleInTime();
}

const char moveCommand[] = "MF";

int compileMove()
{
#ifdef SCRIPT_DEBUG
	Serial.println(F("Compiling move: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a move
	previousStatementStartedBlock = false;

	sendCommand(moveCommand);

	skipInputSpaces();

	return handleValueIntimeAndBackground();
}

const char danceCommand[] = "MF100\rCA\rMF-100\rCA";

int compileDance()
{
	// Not allowed to indent after a move
	previousStatementStartedBlock = false;

	// send dance moves
	sendCommand(danceCommand);

	return ERROR_OK;
}

const char turnCommand[] = "MR";

int compileTurn()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling turn: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a turn
	previousStatementStartedBlock = false;

	skipInputSpaces();

	sendCommand(turnCommand);

	return handleValueIntimeAndBackground();
}

const char arcCommand[] = "MA";

int compileArc()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling arc: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after an arc
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_NO_RADIUS_IN_ARC;
	}

	sendCommand(arcCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	// Spin further down the commands looking for an intime command
	int command = decodeCommandName(pythonishcommandNames);

	if (command != COMMAND_ANGLE)
	{
		return ERROR_NO_ANGLE_IN_ARC;
	}

	outputFunction(',');

	skipInputSpaces();

	return handleValueIntimeAndBackground();
}

const char delayCommand[] = "CD";

int compileDelay()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling delay: "));
#endif // SCRIPT_DEBUG

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_TIME_IN_DELAY;
	}

	sendCommand(delayCommand);

	previousStatementStartedBlock = false;

	return processValue();
}

const char colourCommand[] = "PC";

int compileColour()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling colour: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_RED_VALUE_IN_COLOUR;
	}

	sendCommand(colourCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	if (*bufferPos != ',')
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	outputFunction(',');

	bufferPos++;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	if (*bufferPos != ',')
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	outputFunction(',');

	bufferPos++;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	return processValue();
}

const char namedColourCommand[] = "PN";

int compileSimpleColor()
{
	sendCommand(namedColourCommand);

	// Send the first character of the colour name
	outputFunction(*commandStartPos);

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

int compileBlack()
{
	sendCommand(namedColourCommand);

	// Send the black colour name
	outputFunction('k');

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

int compilePixel()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling pixel: "));
#endif // SCRIPT_DEBUG

	return ERROR_NOT_IMPLEMENTED;
}

const char soundCommand[] = "ST";
const char defaultSoundDuration[] = "500";

int compileSound()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling sound: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_PITCH_VALUE_IN_SOUND;
	}

	sendCommand(soundCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	outputFunction(',');

	bool gotWait = false;

	if (*bufferPos == 0)
	{
		// no duration or wait - use default duration
		sendCommand(defaultSoundDuration);
	}
	else
	{
		// Spin further down the commands looking for duration or wait

		int command = decodeCommandName(pythonishcommandNames);

		if (command == COMMAND_WAIT)
		{
			// send the default duration
			sendCommand(defaultSoundDuration);
			// need to wait for the command to finish
			gotWait = true;
		}
		else
		{
			if (command == COMMAND_DURATION)
			{
				skipInputSpaces();
				result = processValue();
				if (result != ERROR_OK)
					return result;
			}
			else
			{
				return ERROR_SECOND_COMMAND_IN_SOUND_IS_NOT_DURATION;
			}

			skipInputSpaces();

			if (*bufferPos != 0)
			{
				// Now look for a wait
				command = decodeCommandName(pythonishcommandNames);

				if (command == COMMAND_WAIT)
				{
					gotWait = true;
				}
				else
				{
					return ERROR_INVALID_COMMAND_AFTER_DURATION_SHOULD_BE_WAIT;
				}
			}
		}
	}

	outputFunction(',');

	if (gotWait)
		outputFunction('W');
	else
		outputFunction('N');

	return ERROR_OK;
}

const char setCommand[] = "VS";

int compileAssignment()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling set: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a set
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (checkIdentifier(bufferPos) != VARIABLE_NAME_OK)
		return ERROR_INVALID_VARIABLE_NAME_IN_SET;

	int position;

	if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
	{
		if (createVariable(bufferPos, &position) == NO_ROOM_FOR_VARIABLE)
		{
			return ERROR_TOO_MANY_VARIABLES;
		}
	}

	sendCommand(setCommand);

	writeBytesFromBuffer(getVariableNameLength(position));

	skipInputSpaces();

	if (*bufferPos != '=')
	{
		return ERROR_NO_EQUALS_IN_SET;
	}

	bufferPos++;		 // skip past the equals
	outputFunction('='); // write the equals

	skipInputSpaces();

	return processValue();
}

struct stackItem
{
	byte constructionType;
	int count;
	byte indentLevel;
};

#define STACK_SIZE 10

struct stackItem operation[STACK_SIZE];

int operationStackPointer;

#define EMPTY_STACK -1
#define IF_CONSTRUCTION_STACK_ITEM 1
#define WHILE_CONSTRUCTION_STACK_ITEM 3
#define FOREVER_CONSTRUCTION_STACK_ITEM 4

int labelCounter;

void dropValue(int value)
{
	while (true)
	{
		char ch = '0' + (value % 10);
		outputFunction(ch);
		value = value / 10;
		if (value == 0)
			break;
	}
	return;
}

// Push an operation onto the operation stack.
// This manages the if, do and while constructions
//
void push_operation(byte type, byte count)
{
	operation[operationStackPointer].constructionType = type;
	operation[operationStackPointer].count = count;
	operation[operationStackPointer].indentLevel = currentIndentLevel;
	operationStackPointer++;
}

// Get the type of the top operation without removing anything from the stack
// We need to use this to check to make sure that the end element of a construction
// matches the start element.

bool inline operation_stack_empty()
{
	return operationStackPointer == 0;
}

byte top_operation_type()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].constructionType;
}

int top_operation_label()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].count;
}

byte top_operation_indent_level()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].indentLevel;
}

// Get the top value on the operation stack
int pop_operation_count()
{
	operationStackPointer--;
	return operation[operationStackPointer].count;
}

const char labelCommand[] = "CLl";

void dropLabel(int labelCounter)
{
	// first character of the label
	sendCommand(labelCommand);
	dropValue(labelCounter);
}

void dropLabelStatement(int labelCounter)
{
	dropLabel(labelCounter);
	endCommand();
}

void pushLabel(byte labelType)
{
	labelCounter++; // move on to the next construction
	push_operation(labelType, labelCounter);
	dropLabel(labelCounter);
}

const char jumpCommand[] = "CJl";

void dropJump(int labelCounter)
{
	sendCommand(jumpCommand);
	dropValue(labelCounter);
}

void dropJumpCommand(int labelCounter)
{
	dropJump(labelCounter);
	endCommand();
}

void beginCompilingStatements()
{
	currentIndentLevel = 0;
	previousStatementStartedBlock = false;
	operationStackPointer = 0;
	labelCounter = 0;
	resetScriptLine();
	scriptLineNumber = 1;	 // start at the first line
	programError = false;	 // indicate that no errors were detected
	compilingProgram = true; // indicate that we are compiling a program
}

const char endCommandText[] = "RX";

const char failedCommandText[] = "RA";

void endCompilingStatements()
{
	if (programError)
	{
		sendCommand(failedCommandText);
		Serial.println("Errors");
	}
	else
	{
		sendCommand(endCommandText);
		Serial.println("OK");
	}

	compilingProgram = false;
}

// Drops a comparison statement
int dropComparisonStatement(int labelNo, bool trueTest)
{
	outputFunction('C');

	if (trueTest)
		outputFunction('T');
	else
		outputFunction('F');

	skipInputSpaces();

	// Get the first value in the logical expression
	int result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	// Skip to the logical operator
	skipInputSpaces();

	// Get the logical operator
	struct logicalOp *ifOp = findLogicalOp(bufferPos);

	// Abandon if there is no matching logical operator
	if (ifOp == NULL)
	{
		// no logical operator
		return ERROR_MISSING_OPERATOR_IN_COMPARE;
	}

	// Write out the logical operator
	writeMatchingStringFromBuffer(ifOp->operatorCh);

	// Skip to the second operand
	skipInputSpaces();

	// process the second operand
	result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	// if we get here the condition is valid and we need to drop out the destination label
	// for the branch past the

	outputFunction(','); // write the comma

	// Drop out the first character of the label (which is l)
	outputFunction('l');
	// drop the label counter value
	dropValue(labelNo);

	return ERROR_OK;
}

int compileIf()
{

	if (!compilingProgram)
	{
		return ERROR_IF_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling if: "));
#endif // SCRIPT_DEBUG

	labelCounter++; // move on to the next label

	// Add the start of the if to the operation stack

	push_operation(IF_CONSTRUCTION_STACK_ITEM, labelCounter);

	int result = dropComparisonStatement(labelCounter, false);

	labelCounter++; // reserve a label for use by else - if any

	previousStatementStartedBlock = true;

	return result;
}

int compileElse()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling else: "));
#endif // SCRIPT_DEBUG
	if (!compilingProgram)
	{
		return ERROR_ELSE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	return ERROR_OK;
}

int compileWhile()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling while: "));
#endif // SCRIPT_DEBUG

	if (!compilingProgram)
	{
		return ERROR_WHILE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	// First drop out a label so that
	// we can branch back to the top

	pushLabel(WHILE_CONSTRUCTION_STACK_ITEM);

	// Going to follow this command with another
	endCommand();

	labelCounter++; // move on to the next label

	// Now insert the branch past the loop

	previousStatementStartedBlock = true;

	return dropComparisonStatement(labelCounter, false);
}

int compileForever()
{

#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling forever: "));
#endif // SCRIPT_DEBUG

	if (!compilingProgram)
	{
		return ERROR_FOREVER_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	// First drop out a label so that
	// we can branch back to the top

	pushLabel(FOREVER_CONSTRUCTION_STACK_ITEM);

	labelCounter++; // move on to the next label

	// Now insert the branch past the loop

	previousStatementStartedBlock = true;

	return ERROR_OK;
}

// finds the most recent loop construction on the
// operation stack. Returns -1 if no suitable construction found
//

#define NO_LABEL_FOR_LOOP_ON_STACK -1

int findTopLoopConstructionLabel()
{
	// Start the search at the top of the stack
	// Remember that
	int searchStackPointer = operationStackPointer;

	// If the operation stack pointer is zero there is nothing
	// on the stack
	while (searchStackPointer > 0)
	{
		searchStackPointer--; // climb down the stack
							  // pointer aways points to next free location
		byte constructionType = operation[searchStackPointer].constructionType;

		if ((constructionType == WHILE_CONSTRUCTION_STACK_ITEM) || (constructionType == FOREVER_CONSTRUCTION_STACK_ITEM))
		{
			// found a loop construction
			// return the label from that loop
			return operation[searchStackPointer].count;
		}
	}

	// If we get here there is no loop construction available - which is an error

	return NO_LABEL_FOR_LOOP_ON_STACK;
}

int compileBreak()
{

	// Not allowed to indent after a break
	previousStatementStartedBlock = false;

#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling break: "));
#endif // SCRIPT_DEBUG

	if (!compilingProgram)
	{
		return ERROR_BREAK_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	int operation_label = findTopLoopConstructionLabel();

	if (operation_label == NO_LABEL_FOR_LOOP_ON_STACK)
		return ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_BREAK;

	// first label value is the jump for the loop repeat
	// next label value is the label after the end of the loop

	dropJump(operation_label + 1);
	return ERROR_OK;
}

int compileContinue()
{

#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling break: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a continue
	previousStatementStartedBlock = false;

	if (!compilingProgram)
	{
		return ERROR_CONTINUE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	int operation_label = findTopLoopConstructionLabel();

	if (operation_label == NO_LABEL_FOR_LOOP_ON_STACK)
		return ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_CONTINUE;

	// first label value is the jump for the loop repeat

	dropJump(operation_label);

	return ERROR_OK;
}

/// Program control commands - not part of the script
//

const char clearVariablesCommand[] = "VC";

int clearProgram()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Performing clear program: "));
#endif // SCRIPT_DEBUG

	if (compilingProgram)
	{
		return ERROR_CLEAR_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(clearVariablesCommand);

	return ERROR_OK;
}

const char runCommand[] = "RS";

int runProgram()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Performing run program: "));
#endif // SCRIPT_DEBUG

	if (compilingProgram)
	{
		return ERROR_RUN_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(runCommand);

	return ERROR_OK;
}

const char waitCommand[] = "CA";

int compileWait()
{
	// Not allowed to indent after a wait
	previousStatementStartedBlock = false;

	sendCommand(waitCommand);

	return ERROR_OK;
}

const char stopCommand[] = "RH";

int compileStop()
{

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	if (compilingProgram)
	{
		return ERROR_STOP_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(stopCommand);
	return ERROR_OK;
}

const char clearCommand[] = "RC";
const char beginCommand[] = "RM";

int compileBegin()
{
	// Not allowed to indent after a begin
	previousStatementStartedBlock = false;

	if (compilingProgram)
	{
		return ERROR_BEGIN_WHEN_COMPILING_PROGRAM;
	}

	beginCompilingStatements();
	sendCommand(clearCommand);
	endCommand();
	sendCommand(beginCommand);
	return ERROR_OK;
}

int compileEnd()
{
	// Not allowed to indent after a end
	previousStatementStartedBlock = false;

	if (!compilingProgram)
	{
		return ERROR_END_WHEN_NOT_COMPILING_PROGRAM;
	}

	endCompilingStatements();

	return ERROR_OK;
}

// compile a print statement
// The command is followed by an expression or a string of text enclosed in " characters
//
int compilePrint()
{
	// Not allowed to indent after a print
	previousStatementStartedBlock = false;

	// first character of the write command
	outputFunction('W');

	skipInputSpaces();

	if (*bufferPos == '"')
	{
		// start of a message - just drop out the string of text
		outputFunction('T');

		bufferPos++; // skip the starting double quote
		while (*bufferPos != 0 && *bufferPos != '"')
		{
			outputFunction(*bufferPos);
			bufferPos++;
		}
		if (*bufferPos == 0)
		{
			return ERROR_MISSING_CLOSE_QUOTE_ON_PRINT;
		}
		else
		{
			return ERROR_OK;
		}
	}
	else
	{
		// start of a value - just drop out the expression
		outputFunction('V');
		// dropping a value - just process it
		return processValue();
	}
}

const char newlineCommand[] = "WL";

int compilePrintln()
{
	// Not allowed to indent after a println
	previousStatementStartedBlock = false;

	compilePrint();

	// Going to follow this command with another
	endCommand();

	sendCommand(newlineCommand);
	return ERROR_OK;
}

// Decodes a zero terminated script line into a sequence of byte commands
// The byte commnds are sent to the output function specified
// The script line is not buffered, and must not change while this function is running


bool checkForAssignmentName(){
	const char * names = " is, are, are, am, was, were,'s,'re";

	char * namePos = (char *) names;

	// Loop round each of the items in the names list
	while(true){

		// set the source to the input buffer
		char * sourcePos = bufferPos;

		// get the first character out of the name we are looking for
		char firstNameCh = *namePos;

		if(firstNameCh == ' '){

			// if the search name starts with a space we skip leading spaces in the source
			// this lets us handle 's and 're correctly 

			while(true){

				char ch = *sourcePos;

				if(ch == 0){
					// reached the end during search for a space
					return false;
				}

				if(ch!=' '){
					// move past the space at the start of the name
					namePos++;
					break;
				}

				// move down the source
				sourcePos++;
			}
		}

		// when we get here we have the source and the name lined up

		// now we can start looking for a match to the current name

		while(true)
		{
			if(*namePos==*sourcePos)
			{
				// got a character match 
				
				// move on to the next characters in the source and name test	

				namePos++;
				sourcePos++;

				// has the name ended?

				if(*namePos==',' || *namePos==0)
				{
					// got a complete  match in the names string
					// skip the source pos to the character beyond the name
					bufferPos = sourcePos;
					return true;
				}
			}
			else
			{
				// mismatched character in the name - spin to the next name - if any - in the test names

				while(true)
				{

					// get the next character
					char ch = *namePos;

					// move beyond it
					namePos++;

					if(ch == 0)
					{
						// reached the end of the names
						return false;
					}

					if(ch==',')
					{
						// got another name following - move on

						if(*namePos == 0){
							// handle a trailing comma with nothing after it
							return false;
						}

						break;
					}

				}
			}
		}
	}
}


int compileDirectCommand()
{
	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	while (*bufferPos)
	{
		outputFunction(*bufferPos);
		bufferPos++;
	}
	return ERROR_OK;
}

#define SCRIPT_DEBUG

int compileCommonVariable()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling common variable: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a set
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (checkIdentifier(bufferPos) != VARIABLE_NAME_OK)
		return ERROR_INVALID_VARIABLE_NAME_IN_SET_COMMON_VARIABLE;

	int position;

	if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
	{
		if (createVariable(bufferPos, &position) == NO_ROOM_FOR_VARIABLE)
		{
			return ERROR_TOO_MANY_VARIABLES;
		}
	}

	sendCommand(setCommand);

	writeBytesFromBuffer(getVariableNameLength(position));

	skipInputSpaces();

	if (checkForAssignmentName())
	{
		return ERROR_NO_ASSIGNMENT_NAME_IN_SET;
	}

	outputFunction('='); // write the equals

	skipInputSpaces();

	return processValue();
}


int processCommand(byte commandNo)
{
	switch (commandNo)
	{
	case COMMAND_ANGRY: // angry
		return compileAngry();

	case COMMAND_HAPPY: // happy
		return compileHappy();

	case COMMAND_MOVE: // move
		return compileMove();

	case COMMAND_TURN: // turn
		return compileTurn();

	case COMMAND_ARC: // arc
		return compileArc();

	case COMMAND_DELAY: // delay
		return compileDelay();

	case COMMAND_COLOUR: // colour
		return compileColour();

	case COMMAND_COLOR: // color
		return compileColour();

	case COMMAND_PIXEL: // pixel
		return compilePixel();

	case COMMAND_IF: // if
		return compileIf();

	case COMMAND_WHILE: // while
		return compileWhile();

	case COMMAND_CLEAR: // clear
		return clearProgram();

	case COMMAND_RUN: // run
		return runProgram();

	case COMMAND_ELSE: // else
		return compileElse();

	case COMMAND_FOREVER: // forever
		return compileForever();

	case COMMAND_SET:
		return compileAssignment();

	case COMMAND_RED:
	case COMMAND_BLUE:
	case COMMAND_GREEN:
	case COMMAND_MAGENTA:
	case COMMAND_CYAN:
	case COMMAND_YELLOW:
	case COMMAND_WHITE:
		return compileSimpleColor();

	case COMMAND_BLACK:
		return compileBlack();

	case COMMAND_WAIT:
		return compileWait();

	case COMMAND_STOP:
		return compileStop();

	case COMMAND_BEGIN:
		return compileBegin();

	case COMMAND_END:
		return compileEnd();

	case COMMAND_PRINT:
		return compilePrint();

	case COMMAND_PRINTLN:
		return compilePrintln();

	case COMMAND_SYSTEM_COMMAND:
		return compileDirectCommand();

	case COMMAND_SOUND:
		return compileSound();

	case COMMAND_BREAK:
		return compileBreak();

	case COMMAND_CONTINUE:
		return compileContinue();

	case COMMON_VARIABLE:
		return compileCommonVariable();

	default:
		return compileAssignment();
	}

	return ERROR_INVALID_COMMAND;
}

// Called when the indent level of a new line is less than that of the
// previous line. This means that we are closing off one or more indent
// levels. Also called at the end of compilation to close off any loops
// or conditions. Supplied with the new level of indent and the
// number of the command on the statement being indented

// #define SCRIPT_DEBUG_INDENT_OUT

int indentOutToNewIndentLevel(byte indent, int commandNo)
{
	int result;
	int labelNo;

#ifdef SCRIPT_DEBUG_INDENT_OUT
	Serial.println("Indent out to new indent level");
	Serial.print("Indent: ");
	Serial.print(indent);
	Serial.print(" Command: ");
	Serial.print(commandNo);
	Serial.print(" Current Indent Level: ");
	Serial.println(currentIndentLevel);
#endif

	while (indent < currentIndentLevel)
	{
#ifdef SCRIPT_DEBUG_INDENT_OUT
		Serial.println("Looping");
#endif
		if (operation_stack_empty())
		{
#ifdef SCRIPT_DEBUG_INDENT_OUT
			Serial.println("Operation stack empty");
#endif
			// This should not happen as it is not possible to
			// indent code without creating an enclosing command
			return ERROR_INDENT_OUTWARDS_WITHOUT_ENCLOSING_COMMAND;
		}

		// pull back the indent level to the previous one
		currentIndentLevel = top_operation_indent_level();

		// if this indent level is not the same as the indent
		// level of the item on the top of the stack we just close
		// this element off

#ifdef SCRIPT_DEBUG_INDENT_OUT
		Serial.print("New Current Indent Level: ");
		Serial.println(currentIndentLevel);
#endif
		// Generate the code to match the end of the
		// enclosing statement

		switch (top_operation_type())
		{
		case IF_CONSTRUCTION_STACK_ITEM:
#ifdef SCRIPT_DEBUG_INDENT_OUT
			Serial.print("Handling an if..");
#endif
			// might have an else clause for an if construction

			// need to spin up through the operation stack looking for
			// the if condition with the same indent as this else, as that is the
			// one that matches. Any other items that we find (including do) will
			// need to be closed off at this point

			if (currentIndentLevel == indent &&
				commandNo == COMMAND_ELSE)
			{
#ifdef SCRIPT_DEBUG_INDENT_OUT
				Serial.print("...with an else");
#endif

				// get the destination label for the end of the code
				// skipped past if the if condition is not obeyed
				// This code will be obeyed as the else clause

				// get the label number for the label reached if we jump
				// past the code controlled by the if

				labelNo = pop_operation_count();

				// drop a jump to the next label number
				// this number was reserved when the if was created
				// this is the position which will mark the end of the
				// code performed by the else - when we see the endif

				dropJumpCommand(labelNo + 1);

				// Now drop a label to serve as the destination of the
				// jump past the if clause code. This is the code obeyed
				// if else is the case.

				dropLabel(labelNo); // drop the label that is jumped

				// Now need to push a label number for the endif to use
				// to create the destination label for the jump past the
				// else code

				push_operation(IF_CONSTRUCTION_STACK_ITEM, labelNo + 1);

				// Allow statements after this one to indent
				previousStatementStartedBlock = true;
			}
			else
			{
#ifdef SCRIPT_DEBUG_INDENT_OUT
				Serial.print("...on its own");
#endif
				dropLabelStatement(pop_operation_count());
			}
			break;

		case WHILE_CONSTRUCTION_STACK_ITEM:

			labelNo = pop_operation_count();

			dropJumpCommand(labelNo);

			dropLabelStatement(labelNo + 1);
			break;

		case FOREVER_CONSTRUCTION_STACK_ITEM:

			labelNo = pop_operation_count();

			dropJumpCommand(labelNo);

			dropLabelStatement(labelNo + 1);
			break;

		default:
			result = ERROR_INDENT_OUTWARDS_HAS_INVALID_OPERATION_ON_STACK;
			break;
		}
	}
	// When we get here the indent of this statement should match the
	// the indent level pushed onto the operation stack when we started
	// this block
	if (indent != currentIndentLevel)
	{
		result = ERROR_INDENT_OUTWARDS_DOES_NOT_MATCH_ENCLOSING_STATEMENT_INDENT;
	}
	else
	{
		result = ERROR_OK;
	}

	return result;
}

int decodeScriptLine(char *input, void (*output)(unsigned char))
{

	// Set the shared buffer pointer to point to the statement being decoded
	bufferPos = input;

	// Set the output function to point to the statement being output
	outputFunction = output;

	int result;

	byte indent = skipInputSpaces();

	// Lines that start with a # are comments
	if (*bufferPos == '#')
	{
		return ERROR_OK;
	}

	int commandNo = decodeCommandName(pythonishcommandNames);

	if (commandNo == COMMAND_EMPTY_LINE)
	{
		// Ignore empty lines
		return ERROR_OK;
	}

#ifdef SCRIPT_DEBUG

	Serial.print(previousStatementStartedBlock);
	Serial.print(" Current indent: ");
	Serial.print(currentIndentLevel);
	Serial.print("Indent: ");
	Serial.println(indent);

	Serial.print("Compiling: ");
	Serial.println(input);

#endif // SCRIPT_DEBUG

	// Find the position of the first item
	// sort out any outward indents

	if (compilingProgram)
	{
		if (indent < currentIndentLevel)
		{
			// new statement is being outdented
			result = indentOutToNewIndentLevel(indent, commandNo);
			if (result == ERROR_OK)
			{
				result = processCommand(commandNo);
			}
		}
		else
		{
			if (indent > currentIndentLevel)
			{
				// Indenting the text
				// Only valid if we were pre-ceded by a
				// statement that can cause an indent
				if (previousStatementStartedBlock)
				{
					// It's OK to increase the indent if you're starting a new block
					// Set the new indent level for this block
					currentIndentLevel = indent;
					// Now process the command
					result = processCommand(commandNo);
				}
				else
				{
					// Inconsistent indents are bad
					result = ERROR_INDENT_INWARDS_WITHOUT_A_VALID_ENCLOSING_STATEMENT;
				}
			}
			else
			{
				// At the same level - just process the command
				result = processCommand(commandNo);
			}
		}
	}
	else
	{
		// Immediate mode
		result = processCommand(commandNo);
	}

	if (result != ERROR_OK)
	{
		abandonCompilation();

		if (compilingProgram)
		{
			Serial.print("Line:  ");
			Serial.print(scriptLineNumber);
			Serial.print(" ");
		}

		Serial.print("Error: ");
		Serial.print(result);
		Serial.print(" ");
		Serial.println(input);
		printError(result);
	}

	endCommand();

	return result;
}

int pythonIshdecodeScriptChar(char b, void (*output)(unsigned char))
{
	// convert linefeeds into carriage return

	if (b == '\n')
		b = STATEMENT_TERMINATOR;

	if (scriptInputBufferPos == SCRIPT_INPUT_BUFFER_LENGTH)
		return ERROR_SCRIPT_INPUT_BUFFER_OVERFLOW;

	if (b == STATEMENT_TERMINATOR)
	{
		scriptInputBuffer[scriptInputBufferPos] = 0;
		int result = decodeScriptLine(scriptInputBuffer, output);
		scriptLineNumber++; // move on to the next line
		resetScriptLine();
		return result;
	}

	scriptInputBuffer[scriptInputBufferPos++] = b;
	return ERROR_OK;
}

void testScript()
{
	beginCompilingStatements();
	clearVariables();

#ifdef SCRIPT_DEBUG

	Serial.print("Script test");

#endif // SCRIPT_DEBUG

	//  Serial.println(decodeCommandName("forward"));
	//  Serial.println(decodeCommandName("back"));
	//  Serial.println(decodeCommandName("wallaby"));

	//  decodeScriptLine("angry", dumpByte);
	//  decodeScriptLine("happy", dumpByte);

#ifdef SCRIPT_MOVE_TEST

	decodeScriptLine("move 50", dumpByte);
	decodeScriptLine("move 50 intime 10", dumpByte);
	decodeScriptLine("move", dumpByte);
	decodeScriptLine("move ", dumpByte);
	decodeScriptLine("move zz", dumpByte);
	decodeScriptLine("move 50zz", dumpByte);
	decodeScriptLine("move 50 intime", dumpByte);
	decodeScriptLine("move 50 intime 10", dumpByte);

#endif

	// #define SCRIPT_TURN_TEST

#ifdef SCRIPT_TURN_TEST

	decodeScriptLine("turn 50", dumpByte);
	decodeScriptLine("turn 50 intime 10", dumpByte);
	decodeScriptLine("turn", dumpByte);
	decodeScriptLine("turn ", dumpByte);
	decodeScriptLine("turn zz", dumpByte);
	decodeScriptLine("turn 50zz", dumpByte);
	decodeScriptLine("turn 50 intime", dumpByte);
	decodeScriptLine("turn 50 intime 10", dumpByte);

#endif

	// #define SCRIPT_ARC_TEST

#ifdef SCRIPT_ARC_TEST
	decodeScriptLine("arc 90, 180", dumpByte);
	decodeScriptLine("arc 90, 180 intime 100", dumpByte);
	decodeScriptLine("arc 90 , 80", dumpByte);
	decodeScriptLine("arc 90 ,80", dumpByte);
	decodeScriptLine("arc", dumpByte);
	decodeScriptLine("arc ", dumpByte);
	decodeScriptLine("arc zz", dumpByte);
	decodeScriptLine("arc 90", dumpByte);
	decodeScriptLine("arc 90,", dumpByte);
	decodeScriptLine("arc 90,zz", dumpByte);
	decodeScriptLine("arc 90+ 80", dumpByte);
#endif

	// #define SET_TEST
#ifdef SET_TEST
	decodeScriptLine("move x", dumpByte);
	decodeScriptLine("set x=99", dumpByte);
	decodeScriptLine("move x", dumpByte);
	decodeScriptLine("set x=x+1", dumpByte);
	decodeScriptLine("move x+10", dumpByte);

#endif

	// #define DELAY_TEST

#ifdef DELAY_TEST

	decodeScriptLine("delay 100", dumpByte);
	decodeScriptLine("delay", dumpByte);
	decodeScriptLine("delay ", dumpByte);
	decodeScriptLine("delay zz", dumpByte);
#endif

	// #define COLOUR_TEST

#ifdef COLOUR_TEST
	decodeScriptLine("colour 255,128,0", dumpByte);
	decodeScriptLine("colour 255,128,", dumpByte);
	decodeScriptLine("colour 255,128", dumpByte);
	decodeScriptLine("colour 255,", dumpByte);
	decodeScriptLine("colour 255", dumpByte);
	decodeScriptLine("colour ", dumpByte);
	decodeScriptLine("colour", dumpByte);

	decodeScriptLine("color 255,128,0", dumpByte);
	decodeScriptLine("color 255,128,", dumpByte);
	decodeScriptLine("color 255,128", dumpByte);
	decodeScriptLine("color 255,", dumpByte);
	decodeScriptLine("color 255", dumpByte);
	decodeScriptLine("color ", dumpByte);
	decodeScriptLine("color", dumpByte);

#endif

	// #define IF_TEST

#ifdef IF_TEST
	decodeScriptLine("if 1 > 20", dumpByte);
	decodeScriptLine("colour 255,128,0", dumpByte);
	decodeScriptLine("endif", dumpByte);
	decodeScriptLine("if 1 >= 20", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("endif", dumpByte);

#endif

	// #define IF_ELSE_TEST

#ifdef IF_ELSE_TEST

	decodeScriptLine("do", dumpByte);
	decodeScriptLine("if %dist > 20", dumpByte);
	decodeScriptLine("    colour 255,0,0", dumpByte);
	decodeScriptLine("else", dumpByte);
	decodeScriptLine("    colour 0,255,0", dumpByte);
	decodeScriptLine("endif", dumpByte);
	decodeScriptLine("forever", dumpByte);

#endif

	// #define DO_TEST

#ifdef DO_TEST
	decodeScriptLine("set count = 0", dumpByte);
	decodeScriptLine("do", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("colour 0,0,0", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("set count = count + 1", dumpByte);
	decodeScriptLine("until count > 10", dumpByte);
#endif

	// #define WHILE_TEST

#ifdef WHILE_TEST
	decodeScriptLine("set count = 0", dumpByte);
	decodeScriptLine("while count < 10", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("colour 0,0,0", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("set count = count + 1", dumpByte);
	decodeScriptLine("endwhile", dumpByte);
#endif

	//  decodeScriptLine("turn 90", dumpByte);
	//  decodeScriptLine("arc 90, 180", dumpByte);
	//  decodeScriptLine("delay 10", dumpByte);
	//  decodeScriptLine("colour 255,0,255", dumpByte);
	//  decodeScriptLine("color 255,0,255", dumpByte);
	//  decodeScriptLine("pixel 255,255,255,0", dumpByte);
}
