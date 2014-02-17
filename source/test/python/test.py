import testCppParser
import testOovBuilder
import testSupport

# This runs the functions starting with "test" in any imported module above.
def test():
    testSupport.runModules(globals())
