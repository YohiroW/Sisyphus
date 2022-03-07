#pragma once

#include "Base/Misc.h"
#include "TestCaseInterface.h"

BEGIN_NAMESPACE_GEAR

#if 1
#define RUN_TESTCASE_SIMPLE( TestCaseClass, TestCaseName ) \
    TestCaseClass* _test_case_##TestCaseName = new TestCaseClass(#TestCaseName); \
    _test_case_##TestCaseName->RunTest(); \
	delete _test_case_##TestCaseName; \
    _test_case_##TestCaseName = nullptr;

#define RUN_TESTCASE_CONDITIONAL( TestCaseClass, TestCaseName ) \
    TestCaseClass* _test_case_##TestCaseName = new TestCaseClass(#TestCaseName); \
    if(!_test_case_##TestCaseName->RunTest()) \
    {  }\
    delete _test_case_##TestCaseName; \
    _test_case_##TestCaseName = nullptr;
#endif

class TestCase: public ITestCaseInterface
{
    typedef TestCase Super;

public:
    TestCase(const StdString& InTestCaseName) :
        Name(InTestCaseName)
    {
    }

    ~TestCase() {};

    virtual bool RunTest() override
    {
        return true;
    }

private:
    StdString Name;
};

class TestCaseDebug : public TestCase
{
public:
    TestCaseDebug(const StdString& InTestCaseName) :
        TestCase(InTestCaseName)
    {
    }

    ~TestCaseDebug() {};

    virtual bool RunTest() override
	{
        ApplicationMisc::PrintMessage("TEST");

        return true;
    }
};

END_NAMESPACE

