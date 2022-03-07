#pragma once

#include "Gear.h"

BEGIN_NAMESPACE_GEAR

enum LogType: uint8
{
    LT_Log,
    LT_Warning,
    LT_Error,
    LT_Fatal
};

enum LogCategory: uint16
{
    LC_Asset,
    LC_Animation,
    LC_Debug,    
    LC_Game,
    LC_Render,
    //....
    LC_Mask = 0xf,
    LC_LogCategoryNum
};

static_assert(LogCategory::LC_LogCategoryNum> LogCategory::LC_Mask, "[error] Category num exceeded..");

// Global logger, should keep only one instance in memory
class Logger
{
public:
    Logger() = default;
    ~Logger();

    // interfaces
    void Log(LogType level, LogCategory category, ...);

protected:
    void LogInternal(LogType level, LogCategory category, ...);
};

END_NAMESPACE