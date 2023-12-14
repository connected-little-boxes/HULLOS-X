#pragma once

//#define DEBUG debug

//#define MONITOR_MEMORY

#ifdef MONITOR_MEMORY

#define START_MEMORY_MONITOR() start_memory_monitor()
#define DISPLAY_MEMORY_MONITOR(s) display_memory_monitor(s)

#else

#define START_MEMORY_MONITOR() 
#define DISPLAY_MEMORY_MONITOR(s)

#endif

#ifdef DEBUG

#define TRACELOG(s) Serial.print(s)
#define TRACE_HEX(s) Serial.print(s, HEX)
#define TRACELOGLN(s) messageLogf(s)
#define TRACE_HEXLN(s) messageLogf(s, HEX)

#else

#define TRACELOG(s) 
#define TRACE_HEX(s) 
#define TRACELOGLN(s) 
#define TRACE_HEXLN(s) 

#endif
