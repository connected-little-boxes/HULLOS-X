#ifndef SCRIPT_INCLUDED

#define SCRIPT_INCLUDED

#define ERROR_OK 0

#define COMMAND_ANGRY 0
#define COMMAND_HAPPY 1
#define COMMAND_MOVE 2
#define COMMAND_TURN 3
#define COMMAND_ARC 4
#define COMMAND_DELAY 5
#define COMMAND_COLOUR 6
#define COMMAND_COLOR 7
#define COMMAND_PIXEL 8
#define COMMAND_SET 9
#define COMMAND_IF 10
#define COMMAND_DO 11
#define COMMAND_WHILE 12
#define COMMAND_INTIME 13
#define COMMAND_ENDIF 14
#define COMMAND_FOREVER 15
#define COMMAND_ENDWHILE 16
#define COMMAND_SOUND 17
#define COMMAND_UNTIL 18
#define COMMAND_CLEAR 19
#define COMMAND_RUN 20
#define COMMAND_BACKGROUND 21
#define COMMAND_ELSE 22
#define COMMAND_RED 23
#define COMMAND_GREEN 24
#define COMMAND_BLUE 25
#define COMMAND_YELLOW 26
#define COMMAND_MAGENTA 27
#define COMMAND_CYAN 28
#define COMMAND_WHITE 29
#define COMMAND_BLACK 30
#define COMMAND_WAIT 31
#define COMMAND_STOP 32
#define COMMAND_BEGIN 33
#define COMMAND_END 34
#define COMMAND_PRINT 35
#define COMMAND_PRINTLN 36
#define COMMAND_BREAK 37
#define COMMAND_DURATION 38
#define COMMAND_CONTINUE 39
#define COMMAND_ANGLE 40
#define COMMAND_DANCE 41
#define COMMON_VARIABLE 42

#define SCRIPT_INPUT_BUFFER_LENGTH 80
#define COMMAND_NAME_TERMINATOR '#'
#define COMMAND_ALIAS_SEPARATOR ','
#define STATEMENT_TERMINATOR 0x0D

#define ERROR_MISSING_TIME_VALUE_IN_INTIME 1
#define ERROR_INVALID_INTIME 2
#define ERROR_MISSING_MOVE_DISTANCE 3
#define ERROR_NOT_IMPLEMENTED 4
#define ERROR_MISSING_ANGLE_IN_TURN 5
#define ERROR_INVAILD_ANGLE_SEPARATOR_IN_ARC 6
#define VARIABLE_USED_BEFORE_IT_WAS_CREATED 7
#define ERROR_INVALID_DIGIT_IN_NUMBER 8
#define ERROR_INVALID_HARDWARE_READ_DEVICE 9
#define ERROR_INVALID_VARIABLE_NAME_IN_SET 10
#define ERROR_TOO_MANY_VARIABLES 11
#define ERROR_NO_EQUALS_IN_SET 12
#define ERROR_CHARACTERS_ON_THE_END_OF_THE_LINE_AFTER_WAIT 13
#define ERROR_INVALID_OPERATOR_IN_EXPRESSION 14
#define ERROR_MISSING_TIME_IN_DELAY 15
#define ERROR_MISSING_RED_VALUE_IN_COLOUR 16
#define ERROR_MISSING_GREEN_VALUE_IN_COLOUR 17
#define ERROR_MISSING_BLUE_VALUE_IN_COLOUR 18
#define ERROR_MISSING_OPERATOR_IN_COMPARE 19
#define ERROR_ENDIF_BEFORE_IF 20
#define ERROR_INVALID_CONSTRUCTION_MATCHING_ENDIF 21
#define FOREVER_BEFORE_DO 22
#define ERROR_INVALID_CONSTRUCTION_MATCHING_FOREVER 23
#define UNTIL_BEFORE_DO 24
#define ERROR_INVALID_CONSTRUCTION_MATCHING_UNTIL 25
#define ERROR_INVALID_COMMAND 26
#define ERROR_MISSING_OPERATOR_IN_WHILE 27
#define ERROR_INVALID_CONSTRUCTION_MATCHING_ENDWHILE 28
#define ENDWHILE_BEFORE_WHILE 29
#define ERROR_SCRIPT_INPUT_BUFFER_OVERFLOW 30
#define ERROR_INTIME_EXPECTED_BUT_NOT_SUPPLIED 31
#define ERROR_ELSE_WITHOUT_IF 32
#define ERROR_INDENT_OUTWARDS_WITHOUT_ENCLOSING_COMMAND 33
#define ERROR_INDENT_INWARDS_WITHOUT_A_VALID_ENCLOSING_STATEMENT 34
#define ERROR_INDENT_OUTWARDS_HAS_INVALID_OPERATION_ON_STACK 35
#define ERROR_INDENT_OUTWARDS_DOES_NOT_MATCH_ENCLOSING_STATEMENT_INDENT 36
#define ERROR_MISSING_CLOSE_QUOTE_ON_PRINT 37
#define ERROR_MISSING_PITCH_VALUE_IN_SOUND 38
#define ERROR_MISSING_DURATION_VALUE_IN_SOUND 39
#define ERROR_NOT_A_WAIT_AT_THE_END_OF_SOUND 40
#define ERROR_BEGIN_WHEN_COMPILING_PROGRAM 41
#define ERROR_END_WHEN_NOT_COMPILING_PROGRAM 42
#define ERROR_STOP_WHEN_COMPILING_PROGRAM 43
#define ERROR_RUN_WHEN_COMPILING_PROGRAM 44
#define ERROR_CLEAR_WHEN_COMPILING_PROGRAM 45
#define ERROR_FOREVER_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 46
#define ERROR_WHILE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 47
#define ERROR_IF_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 48
#define ERROR_ELSE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 49
#define ERROR_ELSE_MUST_BE_PART_OF_AN_INDENTED_BLOCK 50
#define ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_BREAK 51
#define ERROR_BREAK_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 52
#define ERROR_INVALID_COMMAND_AFTER_DURATION_SHOULD_BE_WAIT 53
#define ERROR_SECOND_COMMAND_IN_SOUND_IS_NOT_DURATION 54
#define ERROR_CONTINUE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM 55
#define ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_CONTINUE 56
#define ERROR_NO_RADIUS_IN_ARC 57
#define ERROR_NO_ANGLE_IN_ARC 58
#define ERROR_NO_COMMAND_START_CHAR 59
#define ERROR_INVALID_VARIABLE_NAME_IN_SET_COMMON_VARIABLE 60
#define ERROR_NO_ASSIGNMENT_NAME_IN_SET 61
#define ERROR_TOKEN_TOO_LARGE 62
#define ERROR_EMPTY_TOKEN 63
#define ERROR_MISSING_VARIABLE_IN_SIMPLE_ASSIGNMENT 64


#define COMMAND_SYSTEM_COMMAND 100
#define COMMAND_EMPTY_LINE 101

void printError(int code);

extern char scriptInputBuffer[SCRIPT_INPUT_BUFFER_LENGTH];

extern int scriptInputBufferPos;

extern char scriptInputBuffer[SCRIPT_INPUT_BUFFER_LENGTH];

extern int scriptInputBufferPos;

// The line number in the script
// Used when reporting errors

extern int scriptLineNumber;

// Flag to indicate an error has been detected
// Used for error reporting

extern bool programError;

// Flag to indicate that a program is being compiled - i.e. a begin keyword has been detected

extern bool compilingProgram;

// The start position of the command in the input buffer
// Set by decodeCommand
// Shared with all the functions below
extern char *commandStartPos;

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

// The position in the input buffer
// Set to the start of the command buffer by decodeScriptLine
// Shared with all the functions below and updated by them

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

extern char *bufferPos;

// The position in the command names
// Set to the start of the command names by decodeScriptLine
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

extern int scriptCommandPos;

// The indent level of the current statement
// Starts at 0 and increases with each block construction
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

extern int currentIndentLevel;

// True if the previous statement started a block
// This statement is allowed to set a new indent level
//

extern bool previousStatementStartedBlock;

extern bool displayErrors;

// The function to be used to send out compiled bytes.
// Set at the start of the line by decodeScriptLine
// Shared with all the functions below

// YET ANOTHER REASON THAT THIS COMPILER IS NOT REENTRANT

extern void (*outputFunction)(unsigned char);

void resetScriptLine();

enum ScriptCompareCommandResult
{
	END_OF_COMMANDS,
	COMMAND_MATCHED,
	COMMAND_NOT_MATCHED
};

int skipInputSpaces();
bool spinToNextCommandAlias(const char * commandNames);
bool spinToCommandEnd(const char * commandNames);
int decodeCommandName(const char * commandNames);
ScriptCompareCommandResult compareCommand(const char * commandNames);
void writeBytesFromBuffer(int length);
void writeMatchingStringFromBuffer(char *string);
int processSingleValue();
int processValue();
void sendCommand(const char *command);
void endCommand();
void abandonCompilation();

#ifdef SCRIPT_DEBUG

#define DUMP_BUFFER_SIZE 20
#define DUMP_BUFFER_LIMIT DUMP_BUFFER_SIZE - 1

extern char dumpBuffer[DUMP_BUFFER_SIZE];

extern int dumpBufferPos = 0;

boolean addDumpByte(byte b);

void displayDump();

void dumpByte(byte b);

#endif

#endif