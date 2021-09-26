#pragma once

class ITestCaseInterface
{
public:
    virtual ~ITestCaseInterface()
    {
    }

    virtual bool RunTest() = 0;
}