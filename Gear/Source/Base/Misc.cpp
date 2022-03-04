#include "Base/Misc.h"

BEGIN_NAMESPACE_GEAR

void ApplicationMisc::OutputString(const StdString& str)
{
    PrintMessage(str);
}

void ApplicationMisc::OutputString(const Char* str)
{
}

void ApplicationMisc::OutputString(const WChar* str)
{
    PrintMessage(str);
}

void ApplicationMisc::PrintMessage(const Char* msg)
{
    printf("%s", msg);
}

void ApplicationMisc::PrintMessage(const WChar* msg)
{
    printf("%s",msg);
}

void ApplicationMisc::PrintMessage(const StdString& msg)
{
    printf("%s", msg);
}

void ApplicationMisc::PrintErrorMessage(const Char* err)
{
    printf("Error occurs in method %s:: %s\(%d\)", __FUNCTION__, __LINE__, __FILE__);
}

END_NAMESPACE