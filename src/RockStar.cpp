#include <Arduino.h>
#include "HullOSScript.h"
#include "RockStar.h"
#include "HullOSVariables.h"
#include "Utils.h"

#define MAX_TOKENS 10
#define MAX_TOKEN_LENGTH 20
#define MAX_DELIMITERS 5
#define MAX_DELIM_LENGTH 4

const char rockstarCommandNames[] =
    "angry,cross,mad#"                // COMMAND_ANGRY       0
    "happy,pleased,glad#"             // COMMAND_HAPPY       1
    "move#"                           // COMMAND_MOVE        2
    "turn#"                           // COMMAND_TURN        3
    "arc#"                            // COMMAND_ARC         4
    "delay#"                          // COMMAND_DELAY       5
    "colour#"                         // COMMAND_COLOUR      6
    "color#"                          // COMMAND_COLOR       7
    "pixel#"                          // COMMAND_PIXEL       8
    "set#"                            // COMMAND_SET         9
    "if#"                             // COMMAND_IF         10
    "do#"                             // COMMAND_DO         11
    "while#"                          // COMMAND_WHILE      12
    "intime#"                         // COMMAND_INTIME     13
    "endif#"                          // COMMAND_ENDIF      14
    "forever#"                        // COMMAND_FOREVER    15
    "endwhile#"                       // COMMAND_ENDWHILE   16
    "sound#"                          // COMMAND_SOUND      17
    "until#"                          // COMMAND_UNTIL      18
    "clear#"                          // COMMAND_CLEAR      19
    "run#"                            // COMMAND_RUN        20
    "background#"                     // COMMAND_BACKGROUND 21
    "else#"                           // COMMAND_ELSE       22
    "red#"                            // COMMAND_RED        23
    "green#"                          // COMMAND_GREEN      24
    "blue#"                           // COMMAND_BLUE       25
    "yellow#"                         // COMMAND_YELLOW     26
    "magenta#"                        // COMMAND_MAGENTA    27
    "cyan#"                           // COMMAND_CYAN       28
    "white#"                          // COMMAND_WHITE      29
    "black#"                          // COMMAND_BLACK      30
    "wait#"                           // COMMAND_WAIT       31
    "stop#"                           // COMMAND_STOP       32
    "begin#"                          // COMMAND_BEGIN      33
    "end#"                            // COMMAND_END        34
    "write#"                          // COMMAND_PRINT      35
    "print,scream,shout,whisper,say#" // COMMAND_PRINTLN    36
    "break#"                          // COMMAND_BREAK      37
    "duration#"                       // COMMAND_DURATION   38
    "continue#"                       // COMMAND_CONTINUE   39
    "angle#"                          // COMMAND_ANGLE      40
    "dance#"                          // COMMAND_DANCE      41
    "an,a,the,my,your,our#"           // COMMON_VARIABLE 42
    ;

// Helper to check if input at position p matches any delimiter
static int isDelimiter(const char *p, const char delimiters[MAX_DELIMITERS][MAX_DELIM_LENGTH])
{
    for (int d = 0; d < MAX_DELIMITERS; d++)
    {
        const char *delim = delimiters[d];
        if (delim[0] == '\0')
            continue;

        int i = 0;
        while (delim[i] && p[i] && delim[i] == p[i])
            i++;
        if (delim[i] == '\0')
            return i; // matched, return length
    }
    return 0; // not a delimiter
}

bool findCommandMatch(const char *input, char *matchedBuf, int *outCommand) {
    const char *p = (const char *)rockstarCommandNames;
    int commandIndex = 0;

    while (*p) {
        const char *groupStart = p;

        while (*p && *p != '#') {
            const char *q = p;
            const char *w = input;

            // Compare current synonym with input
            while (*q && *w && tolower(*q) == tolower(*w) && *q != ',' && *q != '#') {
                q++;
                w++;
            }

            // Matched if input ended and next table char is separator
            if (*w == '\0' && (*q == ',' || *q == '#')) {
                // Copy matched word
                char *out = matchedBuf;
                while (*p && *p != ',' && *p != '#') {
                    *out++ = *p++;
                }
                *out = '\0';
                *outCommand = commandIndex;
                return true;
            }

            // Skip to start of next synonym
            while (*p && *p != ',' && *p != '#') p++;
            if (*p == ',') p++;  // move past comma
        }

        // Move past '#' to start next group
        if (*p == '#') {
            p++;
            commandIndex++;
        }
    }

    return false;
}

int splitInput(const char *input, char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH])
{
    int tokenIndex = 0;
    int charIndex = 0;

    // Clear output tokens
    for (int i = 0; i < MAX_TOKENS; i++)
    {
        tokens[i][0] = '\0';
    }

    while (*input)
    {
        // Skip leading spaces
        while (*input == ' ')
            input++;

        // End of input
        if (*input == '\0')
            break;

        // Start filling token
        charIndex = 0;

        while (*input && *input != ' ')
        {
            if (charIndex >= MAX_TOKEN_LENGTH - 1)
            {
                Serial.print("Error: Token too long at token ");
                Serial.println(tokenIndex);
                return tokenIndex;
            }
            tokens[tokenIndex][charIndex++] = *input++;
        }

        tokens[tokenIndex][charIndex] = '\0';
        tokenIndex++;

        if (tokenIndex >= MAX_TOKENS)
        {
            Serial.println("Error: Too many tokens.");
            return tokenIndex;
        }
    }

    return tokenIndex;
}

char nextToken[MAX_TOKEN_LENGTH];

int getNextToken(){

    skipInputSpaces();
    
    int tokenOffset=0;

    while(true){

        char ch = *bufferPos;

        if(ch==0 || ch==' ') {
            nextToken[tokenOffset]=0;
            return ERROR_OK;
        }

        nextToken[tokenOffset]=ch;
        bufferPos++;
        tokenOffset++;
        if(tokenOffset+1>=MAX_TOKEN_LENGTH)
        {
            return ERROR_TOKEN_TOO_LARGE;
        }
    }
}

char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];

int dropSimpleAssignment(char * variableName){


    Serial.printf("Dropping simple assignment to %s\n", nextToken);

    if(nextToken[0]==0){
        return ERROR_MISSING_VARIABLE_IN_SIMPLE_ASSIGNMENT;
    }

    // Simple assignment is followed directly by the value

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

	sendCommand("VS");

	writeBytesFromBuffer(getVariableNameLength(position));

	skipInputSpaces();

	outputFunction('='); // write the equals

	skipInputSpaces();

	return processValue();

}

int rockstarProcessScriptLine(char *input, void (*output)(byte))
{
    Serial.printf("Got a line to decode %s\n", input);

    bufferPos = input;

    int result = getNextToken();

    if(result != ERROR_OK){
        return result;
    }

    if(nextToken[0]==0){
        return ERROR_EMPTY_TOKEN;
    }

    if(endsWith(nextToken,"'s")){
        strip_end(nextToken,2);
        resetScriptLine();
        return dropSimpleAssignment(nextToken);
    }
    
    if(endsWith(nextToken,"'re")){
        strip_end(nextToken,3);
        resetScriptLine();
        return dropSimpleAssignment(nextToken);
    }

    resetScriptLine();
    return ERROR_OK;


    int count = splitInput(input, tokens);

    Serial.print("Split into ");
    Serial.print(count);
    Serial.println(" tokens:");

    if (count == 0)
    {
        resetScriptLine();
        return ERROR_OK;
    }

    for (int i = 0; i < count; i++)
    {
        Serial.print("Token ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(tokens[i]);
    }

    char matched[20];
    int command;

    if (findCommandMatch(tokens[0], matched, &command))
    {
        Serial.print("Matched command ");
        Serial.print(command);
        Serial.print(" as '");
        Serial.print(matched);
        Serial.println("'");

        switch (command)
        {
        case COMMAND_PRINT:

            /* code */
            break;
        
        default:
            break;
        }
    }
    else
    {
        Serial.println("No match found.");
    }

    resetScriptLine();

    return ERROR_OK;
}

int rockstarProcessScriptChar(char b, void (*output)(byte))
{
    Serial.printf("Rockstar got a %c %d\n", b, b);

    if (b == '\n')
        b = STATEMENT_TERMINATOR;

    if (scriptInputBufferPos == SCRIPT_INPUT_BUFFER_LENGTH)
        return ERROR_SCRIPT_INPUT_BUFFER_OVERFLOW;

    if (b != STATEMENT_TERMINATOR)
    {
        scriptInputBuffer[scriptInputBufferPos++] = b;
        return ERROR_OK;
    }

    scriptInputBuffer[scriptInputBufferPos] = 0;

    return rockstarProcessScriptLine(scriptInputBuffer, output);
}
