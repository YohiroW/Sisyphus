#pragma once

#include "Gear.h"

BEGIN_NAMESPACE_GEAR

// Interfaces to handle something like debug report, path, system info and so on..
class ApplicationMisc
{
public:
	static void OutputString(const Char* str);
	//static void OutputString(const WChar* str);
 //   static void OutputString(const StdString& str);

    static void PrintMessage(const Char* msg);
	//static void PrintMessage(const WChar* msg);
	//static void PrintMessage(const StdString& msg);

	static void PrintErrorMessage(const Char* err);



};

END_NAMESPACE