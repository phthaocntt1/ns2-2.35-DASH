#ifndef ns_dashutils_h
#define ns_dashutils_h


enum DashLogLevel{
    DASH_LOG_NOTE=0,
    DASH_LOG_DEBUG,
    DASH_LOG_WARNING,
    DASH_LOG_ERROR,
    DASH_LOG_FATAL
            
};

enum DashPrintColor{
    DASH_COLOR_BLACK,
    DASH_COLOR_RED,
    DASH_COLOR_GREEN,
    DASH_COLOR_BROWN,
    DASH_COLOR_BLUE,
    DASH_COLOR_MAGENTA,
    DASH_COLOR_CYAN,
    DASH_COLOR_GREY,
    DASH_COLOR_DARK_GREY,
    DASH_COLOR_LIGHT_RED,
    DASH_COLOR_LIGHT_GREEN,
    DASH_COLOR_YELLOW,
    DASH_COLOR_LIGHT_BLUE,
    DASH_COLOR_LIGHT_MEGENTA,
    DASH_COLOR_LIGHT_CYAN,
    DASH_COLOR_WHITE,
    DASH_RESTORE_COLOR
    
};

void DashLog(FILE *stream,int log_level,int default_log_level,enum DashPrintColor color, const char *agent, const char *fmt,va_list arg_list);

#endif