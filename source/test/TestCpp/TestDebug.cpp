// TestDebug.cpp

#include "TestCpp.h"
#include "../../oovEdit/DebugResult.h"

class cDebugUnitTest:public cTestCppModule
    {};

static cDebugUnitTest gDebugUnitTest;



// String returned from debugger:
// value="0x40d1af <_ZStL19piecewise_construct+159> \\\"{curly}\\\""
// value="0x40d14a <_ZStL19piecewise_construct+58> \"\\\"0x40d1af
//     <_ZStL19piecewise_construct+159> \\\\\\\"{curly}\\\\\\\"\\\"\""
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

// String returned from debugger:
// value="{static npos = <optimized out>, _M_dataplus = {<std::allocator<char>> =
// {<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>},
// _M_p = 0x9629f4 \"C:\\\\test\"}}"
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
//	}
TEST_F(gDebugUnitTest, DebugResultStdStringTest)
    {
    std::string val = "C:\\test";
    char const * const dbgVar = "value=\"{static npos = <optimized out>, "
	" _M_dataplus = {<std::allocator<char>> ="
	"{<__gnu_cxx::new_allocator<char>> = {<No data fields>}, <No data fields>},"
	"_M_p = 0x9629f4 \\\"C:\\\\\\\\test\\\"}}\"";

    cDebugResult debRes;
    debRes.parseResult(dbgVar);
    std::string str = debRes.getAsString();
    EXPECT_EQ(str.find(val) != std::string::npos, true);
    }


int main()
    {
    gDebugUnitTest.runAllTests();
    }
