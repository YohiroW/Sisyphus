#include "Base/Log.h"

BEGIN_NAMESPACE_GEAR

// instances in global scope
Logger* GLogger;

Logger::~Logger()
{
	//
}

Logger& Logger::Get()
{
	Logger* singleton = GLogger;

	if(singleton)
	{
		return *singleton;
	}
	else
	{
		LOG(Debug, "Logger instance no exist, see if initialization of Gear instance failed.");
		return *GLogger;
	}
}

void Logger::Log(LogType level, LogCategory category, StdString text)
{
#ifdef ENABLE_LOGGING
	LogInternal(level, category, text);
#endif
}

void Logger::LogInternal(LogType level, LogCategory category, StdString text)
{ 
	// TODO: need to introduce time stamp


	// TODO: string formatted need to implement
	assertf((uint8)level < (uint8)LogType::LT_LogTypeNum, "[Error] Invalid log level!");
	assertf((uint8)category < (uint8)LogCategory::LC_LogCategoryNum, "[Error] Invalid log category!");

	StdString logType = LogTypeName[(uint8)level];
	StdString logCategory = LogCategoryName[(uint8)category];
	StdString msg = logType + logCategory + text;

#ifdef LOG_TO_FILE
	// TODO: to see if file system need to implement
	
#endif // LOG_TO_FILE

	ApplicationMisc::OutputString(msg);
}

END_NAMESPACE