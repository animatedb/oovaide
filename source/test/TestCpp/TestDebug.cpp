// TestDebug.cpp

#include "TestCpp.h"
#include "../../oovEdit/DebugResult.h"

class DebugUnitTest:public TestCppModule
    {
    public:
        DebugUnitTest():
            TestCppModule("Debug")
            {}
    };

static DebugUnitTest gDebugUnitTest;

static std::string quoteStr(char const * const str)
    {
    std::string requiredStr = "\"";
    requiredStr += str;
    requiredStr += "\"";
    return requiredStr;
    }

// Test a simple integer variable.
TEST_F(gDebugUnitTest, DebugResultIntTest)
    {
    // Starting variable
//    int var = 1;
    // Variable returned in debugger
        //  value="1"
    // Variable encoded in c++
    char const * const dbgVar = "value=\"1\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.compare("value = 1") == 0, true);
    }

// Test a simple variable containing curly braces.
TEST_F(gDebugUnitTest, DebugResultVarStringTest)
    {
    // Starting variable
    char const * const var = "{curly}";
    // Variable returned in debugger
        //  value = 0x40d139 <_ZStL19piecewise_construct+41> \"{curly}\""
    // Variable encoded in c++
    char const * const dbgVar = "value="
        " 0x40d139 <_ZStL19piecewise_construct+41> \\\"{curly}\\\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(quoteStr(var)) != std::string::npos, true);
    }

// Test an array of two strings.
TEST_F(gDebugUnitTest, DebugResultVarArrayTest)
    {
    char const * const var[] = { " { } [ ] , \" \\ = ", "test2" };
        //  value="{
        //      0x414303 <_ZStL19piecewise_construct+491>
        //                          \" { } [ ] , \\\" \\\\ = \",
        //      0x41421f <_ZStL19piecewise_construct+263> \"test2\"
        //      }"
    char const * const dbgVar = "value=\"{"
        "0x40d21a <_ZStL19piecewise_construct+266>"
        " \\\" { } [ ] , \\\\\\\" \\\\\\\\ = \\\", "
        "0x40d220 <_ZStL19piecewise_construct+272> \\\"test2\\\""
        "}\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(quoteStr(var[0])) != std::string::npos, true);
    EXPECT_EQ(str.find(quoteStr(var[1])) != std::string::npos, true);
    }

// String returned from debugger:
//
//value= "{
//	static npos = <optimized out>,
//	_M_dataplus =
//		{
//		<std::allocator<char>> =
//		 	{
//			<__gnu_cxx::new_allocator<char>> =
//				{
//				<No data fields>
//				},
//			<No data fields>
//			},
//			 _M_p = 0x9629f4 \"C:\\\\test\"
//		}
//	}"
/*
TEST_F(gDebugUnitTest, DebugResultStdStringTest)
    {
    std::string val = "C:\\test";
    // value=\\\"{static npos = <optimized out>,  _M_dataplus = {"
    // "<std::allocator<char>> ={"
    // "<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>},"
    // "_M_p = "
    // "0x9629f4 \\\\\\\"C:\\\\\\\\\\\\\\\\test\\\\\\\"}"
    // "}\\\"\""
    char const * const dbgVar = "value=\"{static npos = <optimized out>, "
	" _M_dataplus = {<std::allocator<char>> ="
	"{<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>},"
	"_M_p = 0x9629f4 \\\"C:\\\\\\\\test\\\"}}\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(val) != std::string::npos, true);
    }
*/
