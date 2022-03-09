#pragma once

#include "Gear.h"

BEGIN_NAMESPACE_GEAR

#define LOG_DETAIL( _level, _category, _text ) \
	Logger::Get().Log(LogType::LT_##_level, LogCategory::LC_##_category, _text)

#define LOG( _category, _text ) LOG_DETAIL(Log, _category, _text)

#define LOG_ERR( _category, _text ) LOG_DETAIL(Error, _category, _text)


#define LOG_TYPE_NUM 4
enum class LogType: uint8
{
    LT_Log = 0,
    LT_Warning,
    LT_Error,
    LT_Fatal,
    LT_LogTypeNum
};

#define LOG_CATEGORY_NUM 6
enum class LogCategory
{
    LC_Asset,
    LC_Animation,
    LC_Debug,    
    LC_Game,
    LC_Render,
    LC_Log,
    //....
    LC_LogCategoryNum,
    LC_Mask = 0xf
};

static_assert(LogCategory::LC_LogCategoryNum< LogCategory::LC_Mask, "[error] Category num exceeded..");

// Global logger, should keep only one instance in memory
class Logger
{
public:
    Logger() = default;
    ~Logger();

    static Logger& Get();

    // interfaces
    void Log(LogType level, LogCategory category, StdString text);

protected:
    void LogInternal(LogType level, LogCategory category, StdString text);

private:
    // TODO: string formatted need to implement
    StdString LogCategoryName[LOG_CATEGORY_NUM] {
        "Asset:", 
        "Animation:", 
        "Debug:", 
        "Game:",
        "Render:",
        "Log"
    };

    // TODO: string formatted need to implement
	StdString LogTypeName[LOG_TYPE_NUM] {
	    "[Log]",
	    "[Warning]",
	    "[Error]",
	    "[Fatal]"
	};
};

END_NAMESPACE