#include <stdio.h>
#include <stdarg.h>
#include "utils.h"
#include "scheduler.h"

char *DashColorTable[]=
{
    "\033[22;30m",
    "\033[22;31m",
    "\033[22;32m",
    "\033[22;33m",
    "\033[22;34m",
    "\033[22;35m",
    "\033[22;36m",
    "\033[22;37m",
    "\033[01;30m",
    "\033[01;31m",
    "\033[01;32m",
    "\033[01;33m",
    "\033[01;34m",
    "\033[01;35m",
    "\033[01;36m",
    "\033[01;37m",
    "\033[0m"
    
};


void DashLog(FILE *stream,int log_level,int default_log_level,enum DashPrintColor color, const char *agent, const char *fmt,va_list arg_list)
{
    if(log_level<default_log_level)
        return;
    
    char *s_level;
    switch(log_level)
    {
        case DASH_LOG_NOTE:
            s_level="note";
            break;
        case DASH_LOG_DEBUG:
            s_level="debug";
            break;
        case DASH_LOG_WARNING:
            s_level="warning";
            break;
        case DASH_LOG_ERROR:
            s_level="error";
            break;
        case DASH_LOG_FATAL:
            s_level="fatal";
            break;
        default:
            s_level="unknown";
                    
                    
    }
    fprintf(stream,"\n%s%-14s ",DashColorTable[color],agent);
    fprintf(stream,"%s [%6f]",DashColorTable[DASH_COLOR_BLUE],Scheduler::instance().clock());
    fprintf(stream,"%s[%-7s]",DashColorTable[DASH_COLOR_RED],s_level);
    fprintf(stream,":%s ",DashColorTable[DASH_RESTORE_COLOR]);
    
   
    vfprintf(stream, fmt,arg_list);
    
    //fprintf(stream,"\n");
}