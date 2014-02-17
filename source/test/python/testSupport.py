
import traceback

_testSuitePassedCount = 0
_testCountForAllSuites = 0
_errorCount = 0

def dumpError(str):
    global _errorCount
    print (str)
    _errorCount += 1
    traceback.print_stack()


def _allTestsInit():
    global _testCountForAllSuites
    _testCountForAllSuites = 0

def _allTestsComplete():
    global _testCountForAllSuites
    global _testSuitePassedCount
    print("total tests: " + str(_testCountForAllSuites))
    if _testSuitePassedCount == _testCountForAllSuites:
        print("All Passed")
    else:
        print("FAILED TESTS: " + str(_testCountForAllSuites-_testSuitePassedCount))


def _logTestStart(testName):
    global _testCountForAllSuites
    print(" - " + testName)
    _testCountForAllSuites += 1

def _logTestComplete(testName):
    global _testSuitePassedCount
    global _errorCount
    if _errorCount == 0:
        _testSuitePassedCount += 1
        stat = "Passed"
    else:
        stat = "Failed"
        _errorCount = 0
    print("---" + testName + " " + stat)

def runModules(topGlobals):
    _allTestsInit()
    for moduleName, module in topGlobals.items():
        funcNameList = []
        if moduleName.startswith("test") and moduleName != "test" and moduleName != "testSupport":
            print("*** " + moduleName + " ***")
            for funcName in dir(module):
                if funcName.startswith("test") and funcName != "testSupport":
                    function = module.__dict__.get(funcName)
                    _logTestStart(funcName)
                    function()
                    _logTestComplete(funcName)
#                   funcNameList.append((inspect.findsource(function)[1], funcName))
#        if len(funcNameList) > 0:
#            function = module.__dict__.get(sorted(funcNameList)[0][1])
    _allTestsComplete()
