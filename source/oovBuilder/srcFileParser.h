//
// C++ Interface: srcFileParser
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
//
#include "DirList.h"
#include "ComponentFinder.h"
#include <map>
#include <vector>
#include "Debug.h"
#include "OovThreadedWaitQueue.h"


class VerboseDumper
    {
    public:
        void open(OovStringRef const outPath);
        void logOutputOld(OovStringRef const fn);
        void logProcess(OovStringRef const srcFile,  char const * const *argv, int argc);
        void logProgress(OovStringRef const progress);
    private:
        FILE *mFp;
    };
extern VerboseDumper sVerboseDump;


/// Recursively finds source files, and parses the source file
/// for static information, and saves into analysis files.
class srcFileParser:public dirRecurser, public ThreadedWorkWaitQueue<CppChildArgs, srcFileParser>
{
public:
    srcFileParser(const ComponentFinder &compFinder):
        mSrcRootDir(nullptr), mAnalysisDir(nullptr), mComponentFinder(compFinder)
        {}
    virtual ~srcFileParser()
        {}
    bool analyzeSrcFiles(OovStringRef const srcRootDir, OovStringRef const analysisDir);

    // Called by ThreadedWorkQueue
    bool processItem(CppChildArgs const &item);

private:
    InProcMutex mListenerStdMutex;
    char const * mSrcRootDir;
    char const * mAnalysisDir;
    OovStringVec mExcludeDirs;
    OovStringVec mIncDirArgs;
    const ComponentFinder &mComponentFinder;

    virtual bool processFile(OovStringRef const filePath) override;
};

