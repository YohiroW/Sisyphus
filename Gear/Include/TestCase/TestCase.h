#pragma once

#include "Base/Log.h"
#include "Base/Misc.h"
#include "TestCase/TestCaseInterface.h"

BEGIN_NAMESPACE_GEAR

#define RUN_TESTCASE_SIMPLE(_testCaseClass, _testCaseName)      \
    _testCaseClass _test_case_##testCaseName(#_testCaseName);   \
    _test_case_##_testCaseName.RunTest();

#define RUN_TESTCASE_CONDITIONAL(_testCaseClass, _testCaseName) \
    _testCaseClass _test_case_##testCaseName(#_testCaseName);   \
    if(!_test_case_##_testCaseName.RunTest()) {                 \
        LOG_ERR(Debug,"Test case FAILED to Run!");              \
    }

#define DECLARE_AND_IMPLEMENT_TESTCASE(_name)     \
    class _name : public TestCase                 \
    {                                             \
    public:                                       \
	    _name(const StdString& InTestCaseName) :  \
		    TestCase(InTestCaseName){}            \
        ~_name() {};                              \
        virtual bool RunTest() override;          \
    };                                            \
    bool _name::RunTest()

class TestCase: public ITestCaseInterface
{
    typedef TestCase Super;

public:
    TestCase(const StdString& InTestCaseName) :
        Name(InTestCaseName)
    {
    }

    ~TestCase() override {};

    virtual bool RunTest() override
    {
        return true;
    }

private:
    StdString Name;
};

DECLARE_AND_IMPLEMENT_TESTCASE(TestCaseDebug)
{
	ApplicationMisc::PrintMessage("TEST");

	return true;
}

DECLARE_AND_IMPLEMENT_TESTCASE(TestCaseLogOutput)
{
    LOG(Debug, "Test log feature.");
	
    return true;
}

END_NAMESPACE

