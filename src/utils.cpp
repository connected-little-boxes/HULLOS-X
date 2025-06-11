#include <Arduino.h>
#include <limits.h>
#include "utils.h"
#include "messages.h"

int rand_seed=1234;
int rand_mult=8121;
int rand_add=28411;
int rand_modulus=134456;

int localRand()
{
  rand_seed = (rand_mult * rand_seed + rand_add) % rand_modulus;
  return rand_seed;
}

int localRand(int limit)
{
    return localRand() % limit;
}

void localSrand(int seed)
{    
    rand_seed = seed;
}

int localRand(int low, int high)
{
    int diff = high - low;
    return localRand(diff) + low;
}

unsigned long ulongDiff(unsigned long end, unsigned long start)
{
    if (end >= start)
    {
        return end - start;
    }
    else
    {
        return ULONG_MAX - start + end + 1;
    }
}

bool strContains(char *searchMe, char *findMe)
{
    while (*searchMe != 0)
    {
        if (*searchMe == *findMe)
        {
            // found the start of the find string
            // try to match the rest of the find string from here
            char *s1 = searchMe;
            char *f1 = findMe;
            while (*s1 == *f1)
            {
                f1++;
                s1++;
                if (*f1 == 0)
                {
                    // hit the end of the find string - return with a win
                    return true;
                }
                if (*s1 == 0)
                {
                    // hit the end of the search string before we
                    // completed the find - return with a fail
                    return false;
                }
            }
        }
        searchMe++;
    }
    return false;
}

int getUnalignedInt(unsigned char *source)
{
    int result;
    memcpy((unsigned char *)&result, source, sizeof(int));
    return result;
}

float getUnalignedFloat(unsigned char *source)
{
    float result;
    memcpy((unsigned char *)&result, source, sizeof(float));
    return result;
}

void putUnalignedFloat(float fval, unsigned char *dest)
{
    unsigned char *source = (unsigned char *)&fval;
    memcpy(dest, source, sizeof(float));
}

float getUnalignedDouble(unsigned char *source)
{
    double result;
    memcpy((unsigned char *)&result, source, sizeof(double));
    return result;
}

void putUnalignedDouble(double dval, unsigned char *dest)
{
    unsigned char *source = (unsigned char *)&dval;
    memcpy(dest, source, sizeof(double));
}

int currentHeap;

void start_memory_monitor()
{
#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
    currentHeap = ESP.getFreeHeap();
#else
    currentHeap = 0;
#endif
}

void display_memory_monitor( char * item)
{
#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
    int newHeap = (int)ESP.getFreeHeap();
    int heapChange = currentHeap - newHeap;
    if(heapChange != 0) {
        displayMessage("   %s has grabbed %d of memory %d left\n", item, heapChange, newHeap);
        currentHeap = newHeap;
    }
#else
    displayMessage("   Memory monitoring not enabled for this platform\n", item);
#endif
}

void appendFormattedString(char * dest, int limit, const char *format, ...)
{
    // Use va_list to handle variable number of arguments
    va_list args;
    va_start(args, format);
    
    int availableSpace = limit - strlen(dest) - 1;

    if(availableSpace <= 0){
        return;
    }

    // Use vsnprintf to format the string
    char buffer[availableSpace];
    
    vsnprintf(buffer, availableSpace, format, args);

    strcat(dest,buffer);

    va_end(args);
}


void getProcID (char * dest, int length)
{
#if defined(ESP32DOIT)
    snprintf(dest, length, "%06lx", (unsigned long)ESP.getEfuseMac());
#endif

#if defined(WEMOSD1MINI)
    snprintf(dest, length, "%06lx", (unsigned long)ESP.getChipId());
#endif

#if defined(PICO)
	char id_buffer [(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1];

	pico_get_unique_board_id_string(id_buffer,(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1);

	snprintf(dest, length, "%s", id_buffer);
#endif
}

unsigned long getProcIDSalt()
{

#if defined(ESP32DOIT)
    return (unsigned long)ESP.getEfuseMac();
#endif

#if defined(WEMOSD1MINI)
   return (unsigned long)ESP.getChipId();
#endif

#if defined(PICO)
    int bufferLength = (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1;
	char id_buffer [(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1];
    unsigned long result = 0;

    for(int i=0;i<(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES);i++)
    {
        result = result + id_buffer[i];
    }

    return result;
#endif

}

bool endsWith(const char *str, const char *suffix) {
    int lenStr = 0, lenSuffix = 0;

    // Manually calculate lengths (no strlen)
    while (str[lenStr]) lenStr++;
    while (suffix[lenSuffix]) lenSuffix++;

    // Suffix longer than string? Can't match
    if (lenSuffix > lenStr) return false;

    // Compare characters from the end
    for (int i = 0; i < lenSuffix; i++) {
        if (str[lenStr - lenSuffix + i] != suffix[i]) {
            return false;
        }
    }

    return true;
}

void strip_end(char *str, int n) {
    if (n < 0) return;  // Ignore negative n

    char *p = str;
    while (*p) p++;     // Move to the end of the string

    while (n-- > 0 && p > str) {
        --p;
        *p = '\0';
    }
}
