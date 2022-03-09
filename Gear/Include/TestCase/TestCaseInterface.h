#pragma once

BEGIN_NAMESPACE_GEAR

class ITestCaseInterface
{
public:
    virtual ~ITestCaseInterface() {}

    virtual bool RunTest() = 0;
};

END_NAMESPACE