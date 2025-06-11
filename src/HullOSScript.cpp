#include <Arduino.h>
#include "HullOSScript.h"
#include "HullOSVariables.h"

void (*outputFunction)(unsigned char);


const char* getErrorMessage(int code) {
    switch (code) {
        case ERROR_OK: return "No error.";
        case ERROR_MISSING_TIME_VALUE_IN_INTIME: return "Missing time value in 'intime'.";
        case ERROR_INVALID_INTIME: return "Invalid 'intime' value.";
        case ERROR_MISSING_MOVE_DISTANCE: return "Missing move distance.";
        case ERROR_NOT_IMPLEMENTED: return "Feature not implemented.";
        case ERROR_MISSING_ANGLE_IN_TURN: return "Missing angle in 'turn'.";
        case ERROR_INVAILD_ANGLE_SEPARATOR_IN_ARC: return "Invalid angle separator in 'arc'.";
        case VARIABLE_USED_BEFORE_IT_WAS_CREATED: return "Variable used before it was created.";
        case ERROR_INVALID_DIGIT_IN_NUMBER: return "Invalid digit in number.";
        case ERROR_INVALID_HARDWARE_READ_DEVICE: return "Invalid hardware read device.";
        case ERROR_INVALID_VARIABLE_NAME_IN_SET: return "Invalid variable name in 'set'.";
        case ERROR_TOO_MANY_VARIABLES: return "Too many variables declared.";
        case ERROR_NO_EQUALS_IN_SET: return "No equals sign in 'set' statement.";
        case ERROR_CHARACTERS_ON_THE_END_OF_THE_LINE_AFTER_WAIT: return "Unexpected characters after 'wait'.";
        case ERROR_INVALID_OPERATOR_IN_EXPRESSION: return "Invalid operator in expression.";
        case ERROR_MISSING_TIME_IN_DELAY: return "Missing time value in 'delay'.";
        case ERROR_MISSING_RED_VALUE_IN_COLOUR: return "Missing red value in 'colour'.";
        case ERROR_MISSING_GREEN_VALUE_IN_COLOUR: return "Missing green value in 'colour'.";
        case ERROR_MISSING_BLUE_VALUE_IN_COLOUR: return "Missing blue value in 'colour'.";
        case ERROR_MISSING_OPERATOR_IN_COMPARE: return "Missing operator in comparison.";
        case ERROR_ENDIF_BEFORE_IF: return "'endif' before 'if'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_ENDIF: return "No matching 'if' for 'endif'.";
        case FOREVER_BEFORE_DO: return "'forever' before 'do'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_FOREVER: return "Invalid block for 'forever'.";
        case UNTIL_BEFORE_DO: return "'until' before 'do'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_UNTIL: return "Invalid block for 'until'.";
        case ERROR_INVALID_COMMAND: return "Invalid command.";
        case ERROR_MISSING_OPERATOR_IN_WHILE: return "Missing operator in 'while'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_ENDWHILE: return "No matching 'while' for 'endwhile'.";
        case ENDWHILE_BEFORE_WHILE: return "'endwhile' before 'while'.";
        case ERROR_SCRIPT_INPUT_BUFFER_OVERFLOW: return "Script input buffer overflow.";
        case ERROR_INTIME_EXPECTED_BUT_NOT_SUPPLIED: return "'intime' expected but not supplied.";
        case ERROR_ELSE_WITHOUT_IF: return "'else' without matching 'if'.";
        case ERROR_INDENT_OUTWARDS_WITHOUT_ENCLOSING_COMMAND: return "Indent outwards without enclosing command.";
        case ERROR_INDENT_INWARDS_WITHOUT_A_VALID_ENCLOSING_STATEMENT: return "Indent inwards without valid enclosing statement.";
        case ERROR_INDENT_OUTWARDS_HAS_INVALID_OPERATION_ON_STACK: return "Indent outwards with invalid operation on stack.";
        case ERROR_INDENT_OUTWARDS_DOES_NOT_MATCH_ENCLOSING_STATEMENT_INDENT: return "Indent outwards mismatch.";
        case ERROR_MISSING_CLOSE_QUOTE_ON_PRINT: return "Missing close quote on 'print'.";
        case ERROR_MISSING_PITCH_VALUE_IN_SOUND: return "Missing pitch value in 'sound'.";
        case ERROR_MISSING_DURATION_VALUE_IN_SOUND: return "Missing duration in 'sound'.";
        case ERROR_NOT_A_WAIT_AT_THE_END_OF_SOUND: return "Missing 'wait' at end of 'sound'.";
        case ERROR_BEGIN_WHEN_COMPILING_PROGRAM: return "'begin' not allowed inside another program.";
        case ERROR_END_WHEN_NOT_COMPILING_PROGRAM: return "'end' outside of program.";
        case ERROR_STOP_WHEN_COMPILING_PROGRAM: return "'stop' not allowed inside program definition.";
        case ERROR_RUN_WHEN_COMPILING_PROGRAM: return "'run' not allowed inside program definition.";
        case ERROR_CLEAR_WHEN_COMPILING_PROGRAM: return "'clear' not allowed inside program definition.";
        case ERROR_FOREVER_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'forever' used outside a program.";
        case ERROR_WHILE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'while' used outside a program.";
        case ERROR_IF_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'if' used outside a program.";
        case ERROR_ELSE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'else' used outside a program.";
        case ERROR_ELSE_MUST_BE_PART_OF_AN_INDENTED_BLOCK: return "'else' must be indented.";
        case ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_BREAK: return "'break' without enclosing loop.";
        case ERROR_BREAK_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'break' outside a program.";
        case ERROR_INVALID_COMMAND_AFTER_DURATION_SHOULD_BE_WAIT: return "Expected 'wait' after 'duration'.";
        case ERROR_SECOND_COMMAND_IN_SOUND_IS_NOT_DURATION: return "Second command in 'sound' is not 'duration'.";
        case ERROR_CONTINUE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'continue' outside a program.";
        case ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_CONTINUE: return "'continue' without enclosing loop.";
        case ERROR_NO_RADIUS_IN_ARC: return "Missing radius in 'arc'.";
        case ERROR_NO_ANGLE_IN_ARC: return "Missing angle in 'arc'.";
        case ERROR_NO_COMMAND_START_CHAR: return "Missing command start character.";
        case ERROR_INVALID_VARIABLE_NAME_IN_SET_COMMON_VARIABLE: return "Invalid variable name in shared 'set'.";
		case ERROR_NO_ASSIGNMENT_NAME_IN_SET: return "No valid assignment name in 'set'.";
		case ERROR_TOKEN_TOO_LARGE: return "Token too large.";
		case ERROR_EMPTY_TOKEN: return "Missing token.";
		case ERROR_MISSING_VARIABLE_IN_SIMPLE_ASSIGNMENT: return "Missing variable in simple assignment.";
        default: return "Unknown error.";
    }
}

void printError(int code) {
    Serial.print("Error ");
    Serial.print(code);
    Serial.print(": ");
    Serial.println(getErrorMessage(code));
}

char scriptInputBuffer[SCRIPT_INPUT_BUFFER_LENGTH];

int scriptInputBufferPos;

// The line number in the script
// Used when reporting errors

int scriptLineNumber;

// Flag to indicate an error has been detected
// Used for error reporting

bool programError;

// Flag to indicate that a program is being compiled - i.e. a begin keyword has been detected

bool compilingProgram = false;

// The start position of the command in the input buffer
// Set by decodeCommand
// Shared with all the functions below
char *commandStartPos;

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

// The position in the input buffer
// Set to the start of the command buffer by decodeScriptLine
// Shared with all the functions below and updated by them

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

char *bufferPos;

// The position in the command names
// Set to the start of the command names by decodeScriptLine
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

int scriptCommandPos;

// The indent level of the current statement
// Starts at 0 and increases with each block construction
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

int currentIndentLevel;

// True if the previous statement started a block
// This statement is allowed to set a new indent level
//

bool displayErrors = true;

bool previousStatementStartedBlock;

void resetScriptLine()
{
	scriptInputBufferPos = 0;
}

int skipInputSpaces()
{
	int result = 0;

	while (*bufferPos == ' ')
	{
		result++;
		bufferPos++;
	}
	return result;
}

bool spinToCommandEnd(const char * commandNames)
{
	while (true)
	{
		char ch = *(commandNames + scriptCommandPos);

		if (ch == 0)
			// end of the string in memory
			return false;

		if (ch == COMMAND_NAME_TERMINATOR)
		{
			// move past the terminator
			scriptCommandPos++;
			return true;
		}

		// move to the next character
		scriptCommandPos++;
	}
}

bool spinToNextCommandAlias(const char * commandNames)
{
	while (true)
	{
		char ch = commandNames[scriptCommandPos];

		if (ch == 0)
			// end of the string in memory
			return false;

		if (ch == COMMAND_ALIAS_SEPARATOR)
		{
			// move past the separator
			scriptCommandPos++;
			return true;
		}

		if (ch == COMMAND_NAME_TERMINATOR)
		{
			// move past the terminator
			return false;
		}

		// move to the next character
		scriptCommandPos++;
	}
}


// #define COMPARE_COMMAND_DEBUG

ScriptCompareCommandResult compareCommand(const char * commandNames)
{
	// Start at the buffer position

	char *comparePos = bufferPos;
	char *startPos = bufferPos;

	while (true)
	{
		char ch = commandNames[scriptCommandPos];

#ifdef COMPARE_COMMAND_DEBUG
		Serial.print(ch);
#endif

		if (ch == 0)
			return END_OF_COMMANDS;

		char inputCh = toLowerCase(*comparePos);

		// If we have reached the end of the command and the end of the input at the same time
		// we have a match. End of the input is a space or the end of the line

		if (ch == COMMAND_ALIAS_SEPARATOR)
		{
			// we've found an alias for the command
			// this will only happen the middle of a statement so we don't need to worry about the end of
			// the input
			bufferPos = comparePos;
#ifdef COMPARE_COMMAND_DEBUG
			Serial.println("..match");
#endif
			return COMMAND_MATCHED;
		}

		if (ch == COMMAND_NAME_TERMINATOR && (inputCh == ' ' | inputCh == 0))
		{
			// Set the buffer position to the end of the command
			bufferPos = comparePos;

#ifdef COMPARE_COMMAND_DEBUG
			Serial.println("..match");
#endif
			return COMMAND_MATCHED;
		}

		if (ch != inputCh)
		{
			// this doesn't match the command - but it might match an alias
			// see if we can find one
			// move past the alias terminator

			if (spinToNextCommandAlias(commandNames))
			{
				// reset the input position to the start
				comparePos = bufferPos;
#ifdef COMPARE_COMMAND_DEBUG
				Serial.println("..separator");
#endif
			}
			else
			{
#ifdef COMPARE_COMMAND_DEBUG
				Serial.println("..fail");
#endif
				return COMMAND_NOT_MATCHED;
			}
		}
		scriptCommandPos++;
		comparePos++;
	}
}


// Decodes the command held in the area of memory referred to by bufferPos
int decodeCommandName(const char * commandNames)
{
	// Set the position in the command list to the start of the list
	scriptCommandPos = 0;

	// Set the command counter to 0
	int commandNumber = 0;

	skipInputSpaces();

	// ignore empty lines
	if (*bufferPos == 0)
		return COMMAND_EMPTY_LINE;

	// Set commandStartPos to point to the start of the statement being decoded
	// Used when decoding colour names

	commandStartPos = bufferPos;

	// it is a system command - just return this immediately

	if (*bufferPos == '*')
	{
		// skip past the *
		bufferPos++;
		// return the command type
		return COMMAND_SYSTEM_COMMAND;
	}

	while (true)
	{
		char *nameStart = bufferPos;

		ScriptCompareCommandResult result = compareCommand(commandNames);

		switch (result)
		{
		case COMMAND_MATCHED:
			return commandNumber;

		case COMMAND_NOT_MATCHED:
			if (!spinToCommandEnd(commandNames))
				return -1;
			commandNumber++;
			break;

		case END_OF_COMMANDS:
			return -1;
			break;
		}
	}
}

void writeBytesFromBuffer(int length)
{
	for (int i = 0; i < length; i++)
	{
		outputFunction(*bufferPos);
		bufferPos++;
	}
}

void writeMatchingStringFromBuffer(char *string)
{
	while (*string)
	{
		outputFunction(*bufferPos);
		bufferPos++;
		string++;
	}
}

#ifdef SCRIPT_DEBUG

#define DUMP_BUFFER_SIZE 20
#define DUMP_BUFFER_LIMIT DUMP_BUFFER_SIZE - 1

char dumpBuffer[DUMP_BUFFER_SIZE];

int dumpBufferPos = 0;

boolean addDumpByte(byte b)
{
	if (dumpBufferPos < DUMP_BUFFER_LIMIT)
	{
		dumpBuffer[dumpBufferPos] = b;
		dumpBufferPos++;
		return true;
	}
	return false;
}

void displayDump()
{
	dumpBuffer[dumpBufferPos] = 0;
	Serial.println(dumpBuffer);
	dumpBufferPos = 0;
}

void dumpByte(byte b)
{
	storeProgramByte(b);
	if (b == STATEMENT_TERMINATOR)
	{
		displayDump();
	}
	else
	{
		addDumpByte((char)b);
	}
}

#endif

int processSingleValue()
{
	skipInputSpaces();

	if (isVariableNameStart(bufferPos))
	{
		// its a variable
		int position;

		if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
		{
			return VARIABLE_USED_BEFORE_IT_WAS_CREATED;
		}

		// copy the variable into the instruction

		int variableLength = getVariableNameLength(position);

		for (int i = 0; i < variableLength; i++)
		{
			outputFunction(*bufferPos);
			bufferPos++;
		}
		return ERROR_OK;
	}

	if (isdigit(*bufferPos) | (*bufferPos == '+') | (*bufferPos == '-'))
	{
		bool firstch = true;

		while (true)
		{
			char ch = *bufferPos;

			if (ch < '0' | ch > '9')
			{
				if (firstch)
				{
					if (ch != '+' && ch != '-')
					{
						return ERROR_INVALID_DIGIT_IN_NUMBER;
					}
				}
				else
				{
					return ERROR_OK;
				}
			}
			outputFunction(ch);
			firstch = false;
			bufferPos++;
		}
	}

	if (*bufferPos == READING_START_CHAR)
	{
		// Move past the start character

		bufferPos++;

		struct reading *reader = getReading(bufferPos);

		if (reader == NULL)
		{
			return ERROR_INVALID_HARDWARE_READ_DEVICE;
		}

		// Drop out the char to start the hardware name

		outputFunction(READING_START_CHAR);

		// copy the variable into the instruction

		int readerLength = strlen(reader->name);

		for (int i = 0; i < readerLength; i++)
		{
			outputFunction(*bufferPos);
			bufferPos++;
		}
		return ERROR_OK;
	}
	return ERROR_NO_COMMAND_START_CHAR;
}

int processValue()
{
	int result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
		// Just a single value - no expression
		return ERROR_OK;

	if (validOperator(*bufferPos))
	{
		// write out the operator
		outputFunction(*bufferPos);

		// move past the operator
		bufferPos++;

		skipInputSpaces();

		// process the second value
		return processSingleValue();
	}

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

void sendCommand(const char *command)
{
	int pos = 0;

	while (true)
	{
		byte b = command[pos];

		if (b == 0)
			break;

		outputFunction(b);
		pos++;
	}
}

void endCommand()
{
	outputFunction(STATEMENT_TERMINATOR);
}

void abandonCompilation()
{
	programError = true;
}


