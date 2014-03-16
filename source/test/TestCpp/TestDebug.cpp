// TestDebug.cpp

#include "TestCpp.h"
#include "../../oovEdit/DebugResult.h"

class cDebugUnitTest:public cTestCppModule
    {};

static cDebugUnitTest gDebugUnitTest;



TEST_F(gDebugUnitTest, DebugResultVarStringTest)
    {
    char const * const var = "{curly}";
    char const * const dbgVar = "value=\"0x40d1af <_ZStL19piecewise_construct+159>"
        " \\\"{curly}\\\"\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(var) != std::string::npos, true);
    }

TEST_F(gDebugUnitTest, DebugResultVarArrayTest)
    {
    char const * const var[] = { " { } [ ] , \" \\ = ", "test2" };
    // Debugger string:     \" { } [ ] , \\\" \\\\ = \"
    char const * const dbgVar = "value=\"{0x40d21a <_ZStL19piecewise_construct+266>"
        " \\\"{}[]\\\\\\\"=\\\", 0x40d220 <_ZStL19piecewise_construct+272> \\\"test2\\\"}\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(var[1]) != std::string::npos, true);
    }

TEST_F(gDebugUnitTest, DebugResultStdStringTest)
    {
    std::string val = "test";
    char const * const dbgVar = "";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(val) != std::string::npos, true);
    }


int main()
    {
    gDebugUnitTest.runAllTests();
    }
