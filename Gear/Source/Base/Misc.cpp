#include "Base/Misc.h"

BEGIN_NAMESPACE_GEAR

void ApplicationMisc::OutputString(const Char* str)
{
	PrintMessage(str);
}

void ApplicationMisc::OutputString(const WChar* str)
{
	PrintMessage(str);
}

void ApplicationMisc::OutputString(const StdString& str)
{
	PrintMessage(str);
}

void ApplicationMisc::PrintMessage(const Char* msg)
{
    printf("%s", msg);
}

void ApplicationMisc::PrintMessage(const WChar* msg)
{
	printf("%ls", msg);
}

void ApplicationMisc::PrintMessage(const StdString& msg)
{
	PrintMessage(msg.c_str());
}

void ApplicationMisc::PrintErrorMessage(const Char* err)
{
    printf("Error occurs in method %s:: %s(%d)", __FUNCTION__, __FILE__, __LINE__);
}

END_NAMESPACE