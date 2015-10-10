/*
 * CppParser.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
*/

#include "CppInstr.h"
#include "Project.h"
#include "Debug.h"
#include "File.h"
#include "CoverageHeaderReader.h"
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>             // for unlink
#include <limits.h>
#include <algorithm>
#include "clang-c/Index.h"


#define DEBUG_PARSE 0
#if(DEBUG_PARSE)
#define DEBUG_DUMP_CURSORS 0
static DebugFile sLog("DebugCppInstr.txt");
#endif

std::string getFirstNonCommentToken(CXCursor cursor)
    {
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    clang_tokenize(tu, range, &tokens, &nTokens);
    std::string str;
    for (size_t i = 0; i < nTokens; i++)
        {
        CXTokenKind kind = clang_getTokenKind(tokens[i]);
        if(kind != CXToken_Comment)
            {
            CXStringDisposer spelling = clang_getTokenSpelling(tu, tokens[i]);
            str += spelling;
            break;
            }
        }
    clang_disposeTokens(tu, tokens, nTokens);
    return str;
    }

/// @todo - this is a duplicate of a function in ParseBase.cpp
void appendCursorTokenString(CXCursor cursor, std::string &str)
    {
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    clang_tokenize(tu, range, &tokens, &nTokens);
    for (size_t i = 0; i < nTokens; i++)
        {
        CXTokenKind kind = clang_getTokenKind(tokens[i]);
        if(kind != CXToken_Comment)
            {
            if(str.length() != 0)
                str += " ";
            CXStringDisposer spelling = clang_getTokenSpelling(tu, tokens[i]);
            str += spelling;
            }
        }
    clang_disposeTokens(tu, tokens, nTokens);
    }

static void dumpCursor(FILE *fp, char const * const str, CXCursor cursor)
    {
    if(fp)
        {
        CXStringDisposer name(clang_getCursorSpelling(cursor));
        CXStringDisposer spstr = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
        std::string tokenStr;
        appendCursorTokenString(cursor, tokenStr);
        if(tokenStr.length() > 100)
            {
            tokenStr.resize(100);
            tokenStr += "...";
            }

        fprintf(fp, "%s: %s %s   %s\n", str, spstr.c_str(), name.c_str(), tokenStr.c_str());
        fflush(fp);
        }
    }

#if(DEBUG_PARSE)
void debugInstr(CXCursor cursor, char const * const str, int instrCount)
    {
    if(instrCount == 9)
        {
        printf("Found: %d\n", instrCount);
        fprintf(sLog.mFp, "Found: %d\n", instrCount);
        }
    fprintf(sLog.mFp, "INSTR: ");
    dumpCursor(sLog.mFp, str, cursor);
    }
#endif

static void appendLineEnding(std::string &str)
    {
#ifdef __linux__
    str += '\n';
#else
    str += "\r\n";
#endif
    }


bool CppFileContents::read(char const *fn)
    {
    SimpleFile file;
    eOpenStatus openStat = file.open(fn, M_ReadWriteExclusive, OE_Binary);
    OovStatus status(openStat == OS_Opened, SC_File);
    if(status.ok())
        {
        int size = file.getSize();
        mFileContents.resize(size);
        int actual = 0;
        status = file.read(mFileContents.data(), size, actual);
        }
    if(!status.ok())
        {
        OovString str = "Unable to read %s ";
        str += fn;
        str += "\n";
        status.report(ET_Error, str.getStr());
        }
    return status.ok();
    }

bool CppFileContents::write(OovStringRef const fn)
    {
    SimpleFile file;
    eOpenStatus openStat = file.open(fn, M_WriteExclusiveTrunc, OE_Binary);
    OovStatus status(openStat == OS_Opened, SC_File);
    if(status.ok())
        {
        OovString includeCov = "#include \"OovCoverage.h\"";
        appendLineEnding(includeCov);
        updateMemory();
        status = file.write(includeCov.c_str(), includeCov.length());
        }
    if(status.ok())
        {
        status = file.write(mFileContents.data(), mFileContents.size());
        }
    if(!status.ok())
        {
        OovString str = "Unable to write %s ";
        str += fn;
        str += "\n";
        status.report(ET_Error, str.getStr());
        }
    return status.ok();
    }

void CppFileContents::updateMemory()
    {
    int numInsertedBytes = 0;
    for(auto &mapIter : mInsertMap)
        {
        std::vector<char> strVec;
        int strLen = mapIter.second.length();
        for(int i=0; i<strLen; i++)
            {
            strVec.push_back(mapIter.second[i]);
            }
        size_t offset = numInsertedBytes + mapIter.first;
        if(offset < mFileContents.size())
            {
            mFileContents.insert(mFileContents.begin() + offset,
                strVec.begin(), strVec.end());
            }
        numInsertedBytes += strLen;
        }
    }

void CppFileContents::insert(OovStringRef const str, int origFileOffset)
    {
    mInsertMap.insert(std::pair<int, OovString>(origFileOffset, str));
    }



class CrashDiagnostics
    {
    public:
        CrashDiagnostics():
            mCrashed(false)
            {}
        void saveMostRecentParseLocation(char const * const diagStr, CXCursor cursor)
            {
            mDiagStr = diagStr;
            mMostRecentCursor = cursor;
#if(DEBUG_DUMP_CURSORS)
            dumpCursor(sLog.mFp, diagStr, cursor);
#endif
            }
        void setCrashed()
            { mCrashed = true; }
        bool hasCrashed() const
            { return mCrashed; }
        void dumpCrashed(FILE *fp)
            {
            if(mCrashed)
                {
                fprintf(fp, "CRASHED: %s\n", mDiagStr.c_str());
                try
                    {
                    dumpCursor(fp, "", mMostRecentCursor);
                    }
                catch(...)
                    {
                    }
                }
            }
    private:
        bool mCrashed;
        std::string mDiagStr;
        CXCursor mMostRecentCursor;
    };
static CrashDiagnostics sCrashDiagnostics;


static CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppInstr *parser = static_cast<CppInstr*>(client_data);
    return parser->visitTranslationUnit(cursor, parent);
    }

static CXChildVisitResult visitFunctionAddInstr(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppInstr *parser = static_cast<CppInstr*>(client_data);
    return parser->visitFunctionAddInstr(cursor, parent);
    }


/////////////////////////

OovString sFileDefine;

static void setFileDefine(OovStringRef const fn, OovStringRef const srcRootDir)
    {
    OovString relFn = Project::getSrcRootDirRelativeSrcFileName(fn, srcRootDir);
    OovString fileDef = "COV_";
    fileDef += relFn;
    fileDef.replaceStrs("//", "_");
    fileDef.replaceStrs("/", "_");
    fileDef.replaceStrs(".", "_");
    fileDef.replaceStrs(":", "");
    sFileDefine = fileDef;
    }

static std::string getFileDefine()
    {
    return sFileDefine;
    }

void CppInstr::makeCovInstr(OovString &covStr)
    {
    appendLineEnding(covStr);
    covStr += "COV_IN(";
    covStr += getFileDefine();
    covStr += ", ";
    covStr.appendInt(mInstrCount++);
    covStr += ");";
    appendLineEnding(covStr);
    }

void CppInstr::insertCovInstr(int offset)
    {
    OovString covStr;
    makeCovInstr(covStr);
    insertOutputText(covStr, offset);
    }

bool CppInstr::isParseFile(SourceLocation const &loc) const
    {
    // clang_File_isEqual
    FilePath fn(loc.getFn(), FP_File);
    return(fn == mTopParseFn);
    }

bool CppInstr::isParseFile(CXFile const &file) const
    {
    CXStringDisposer fnStr(clang_getFileName(file));
    FilePath fn(fnStr, FP_File);
    return(fn == mTopParseFn);
    }


struct ChildCountVisitor
{
    ChildCountVisitor(int count):
        mCount(count)
        {
        mFoundCursor = clang_getNullCursor();
        }
    int mCount;
    CXCursor mFoundCursor;
};
static CXChildVisitResult ChildNthVisitor(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    ChildCountVisitor *context = static_cast<ChildCountVisitor*>(client_data);
    CXChildVisitResult res = CXChildVisit_Continue;
    if(context->mCount == 0)
        {
        context->mFoundCursor = cursor;
        res = CXChildVisit_Break;
        }
    else
        context->mCount--;
    return res;
    }

static CXCursor getNthChildCursor(CXCursor cursor, int nth)
    {
    ChildCountVisitor visitorData(nth);
    clang_visitChildren(cursor, ChildNthVisitor, &visitorData);
    CXCursor childCursor = clang_getNullCursor();
    if(visitorData.mCount == 0)
        childCursor = visitorData.mFoundCursor;
    return childCursor;
    }
/*
static CXChildVisitResult ChildInstrNonCompoundVisitor(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppInstr *context = static_cast<CppInstr*>(client_data);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    if(clang_isStatement(cursKind))
        {
        context->insertNonCompoundInstr(cursor);
        }
    return CXChildVisit_Continue;
    }

void CppInstr::instrChildNonCompoundStatements(CXCursor cursor)
    {
    clang_visitChildren(cursor, ChildInstrNonCompoundVisitor, this);
    }
*/

void CppInstr::insertNonCompoundInstr(CXCursor cursor)
    {
    if(!clang_Cursor_isNull(cursor))
        {
        CXCursorKind cursKind = clang_getCursorKind(cursor);
// return statements that are children of if statements must be instrumented
// if not in a compound statement.
//      if(cursKind != CXCursor_CompoundStmt /*&& clang_isStatement(cursKind)*/)

        // If this is a compound statement, then it will be instrumented anyway.
        if(cursKind != CXCursor_CompoundStmt)
            {
            bool doInstr = true;
            // There is sometimes a parsing error where a null statement is returned.
            // This occurs when all headers are not included or defines are not proper?
            if(cursKind == CXCursor_NullStmt)
                {
                CXStringDisposer name(clang_getCursorSpelling(cursor));
                if(name[0] != ';')
                    {
                    SourceLocation loc(cursor);
                    fprintf(stderr, "Unable to instrument line %d\n", loc.getLine());
                    doInstr = false;
                    }
                }
            if(doInstr)
                {
#if(DEBUG_PARSE)
                debugInstr(cursor, "insertNCI", mInstrCount);
#endif
                SourceRange range(cursor);
                OovString covStr;
                makeCovInstr(covStr);
                covStr.insert(0, "{");
                insertOutputText(covStr, range.getStartLocation().getOffset());
                insertOutputText("\n}\n", range.getEndLocation().getOffset()+1);
                }
            }
        }
    }


// Finds variable declarations inside function bodies.
CXChildVisitResult CppInstr::visitFunctionAddInstr(CXCursor cursor, CXCursor parent)
    {
#if(DEBUG_DUMP_CURSORS)
    static int level = 0;
    level++;
    for(int i=0; i<level; i++)
        fprintf(sLog.mFp, "  ");
#endif

    sCrashDiagnostics.saveMostRecentParseLocation("FV", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
        {
        case CXCursor_DoStmt:
            if(isParseFile(cursor))
                {
                insertNonCompoundInstr(getNthChildCursor(cursor, 0));
                }
            break;

        case CXCursor_WhileStmt:
            if(isParseFile(cursor))
                {
                insertNonCompoundInstr(getNthChildCursor(cursor, 1));
                }
            break;

        case CXCursor_CXXForRangeStmt:
            if(isParseFile(cursor))
                {
                insertNonCompoundInstr(getNthChildCursor(cursor, 5));
                }
            break;

        case CXCursor_ForStmt:
            if(isParseFile(cursor))
                {
                insertNonCompoundInstr(getNthChildCursor(cursor, 3));
                }
            break;

        case CXCursor_CaseStmt:
            if(isParseFile(cursor))
                {
                CXCursor childCursor = getNthChildCursor(cursor, 1);
                CXCursorKind childCursKind = clang_getCursorKind(childCursor);
                if(childCursKind != CXCursor_CaseStmt && childCursKind != CXCursor_DefaultStmt)
                    {
                    insertNonCompoundInstr(childCursor);
                    }
                }
            break;

        case CXCursor_DefaultStmt:
            if(isParseFile(cursor))
                {
                CXCursor childCursor = getNthChildCursor(cursor, 0);
                CXCursorKind childCursKind = clang_getCursorKind(childCursor);
                if(childCursKind != CXCursor_CaseStmt && childCursKind != CXCursor_DefaultStmt)
                    {
                    insertNonCompoundInstr(childCursor);
                    }
                }
            break;

        case CXCursor_IfStmt:
            // An if statement has up to 3 children, test expr, if body, else body
            // The else body can be an if statement.
            if(isParseFile(cursor))
                {
                insertNonCompoundInstr(getNthChildCursor(cursor, 1));
                CXCursor childCursor = getNthChildCursor(cursor, 2);
                CXCursorKind childCursKind = clang_getCursorKind(childCursor);
                if(childCursKind != CXCursor_IfStmt)
                    insertNonCompoundInstr(childCursor);
                }
            break;

        case CXCursor_CompoundStmt:
            {
            CXCursorKind parentKind = clang_getCursorKind(parent);
            // Don't instrument braces after a switch.
            if(parentKind != CXCursor_SwitchStmt)
                {
                SourceLocation loc(cursor);
                if(isParseFile(cursor))
                    {
                    // Avoid macros such as the following:
                    // #define DECLARE_FOOID(libid) static void InitLibId() { int x = libid; }
                    // None of the get...Location functions return the defined macro location
                    // This doesn't work either:    if(clang_Location_isFromMainFile(loc.getLoc()))
//                  if(isParseFile(file))
                    std::string str = getFirstNonCommentToken(cursor);
                    if(str.length() > 0 && str[0] == '{')
                        {
#if(DEBUG_PARSE)
                        debugInstr(cursor, "insertCS", mInstrCount);
#endif
                        insertCovInstr(loc.getOffset()+1);
                        }
                    }
                }
            }
            break;

        // case CXCursor_SwitchStmt
        // case CXCursor_StmtExpr
        // case CXCursor_FirstStmt
        // case CXCursor_LabelStmt
        // case CXCursor_UnexposedStmt
        // case CXCursor_GotoStmt
        // case CXCursor_IndirectStmt
        // case CXCursor_ContinueStmt
        // case CXCursor_BreakStmt
        // case CXCursor_AsmStmt
        // case CXCursor_TryStmt, finally, etc.
        default:
            break;
        }
    // The StmtExpr is a strange GNU extension that is present in things
    // like "if(GTK_IS_LIST_STORE(model))". If this is instrumented, the coverage macro
    // is inserted within the if expression part of the statement.
    if(cursKind != CXCursor_StmtExpr)
        {
        clang_visitChildren(cursor, ::visitFunctionAddInstr, this);
        }
//    return CXChildVisit_Recurse;
#if(DEBUG_DUMP_CURSORS)
    level--;
#endif
    return CXChildVisit_Continue;
    }

CXChildVisitResult CppInstr::visitTranslationUnit(CXCursor cursor, CXCursor parent)
    {
    CXChildVisitResult result = CXChildVisit_Recurse;
    sCrashDiagnostics.saveMostRecentParseLocation("TU", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
        {
        case CXCursor_CXXMethod:
        case CXCursor_FunctionDecl:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case  CXCursor_ConversionFunction:
            visitFunctionAddInstr(cursor, parent);
            break;

        case CXCursor_InclusionDirective:
            result = CXChildVisit_Continue;     // Skip included files
            break;

        default:
            break;
        }
    return result;
    }

class CoverageHeader:public CoverageHeaderReader
    {
    public:
        /// Updates the coverage instrumentation data. This includes how many
        /// instrumentation coverage lines there are for each file, and makes
        /// a base define for each file that is used to index into the
        /// coverage array that is used for the hit counts for each instrumented
        /// line.
        void update(OovStringRef const outMapFn, OovStringRef const srcFn,
                int numInstrLines);

    private:
        /// Writes the instr def map to the file
        /// @todo - srcFn isn't used.
        void write(SharedFile &outDefFile, OovStringRef const srcFn,
                int numInstrLines);
    };

void CoverageHeader::update(OovStringRef const outDefFn, OovStringRef const srcFn,
        int numInstrLines)
    {
    SharedFile outDefFile;
    eOpenStatus stat = outDefFile.open(outDefFn, M_ReadWriteExclusive, OE_Binary);
    if(stat == OS_Opened)
        {
        OovStatus status = read(outDefFile);
        if(status.ok())
            {
            write(outDefFile, srcFn, numInstrLines);
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to update coverage file");
            }
        }
    }

void CoverageHeader::write(SharedFile &outDefFile, OovStringRef const /*srcFn*/,
        int numInstrLines)
    {
    std::string fnDef = getFileDefine();
    mInstrDefineMap[fnDef] = numInstrLines;

    int totalCount = 0;
    for(auto const &defItem : mInstrDefineMap)
        {
        totalCount += defItem.second;
        }

    if(outDefFile.isOpen())
        {
        OovString buf;
        static char const *lines[] =
            {
            "// Automatically generated file by OovCovInstr\n",
            "// This file should not normally be edited manually.\n",
            "#define COV_IN(fileIndex, instrIndex) gCoverage[fileIndex+instrIndex]++;\n",
            };
        for(size_t i=0; i<sizeof(lines)/sizeof(lines[0]); i++)
            {
            buf += lines[i];
            }
        buf += "#define COV_TOTAL_INSTRS ";
        buf.appendInt(totalCount);
        buf += "\n";
        buf += "extern unsigned short gCoverage[COV_TOTAL_INSTRS];\n";

        int coverageCount = 0;
        for(auto const &defItem : mInstrDefineMap)
            {
            OovString def = "#define ";
            def += defItem.first;
            def += " ";
            def.appendInt(coverageCount);
            coverageCount += defItem.second;
            def += " // ";
            def.appendInt(defItem.second);
            def += "\n";
            buf += def;
            }
        outDefFile.truncate();
        OovStatus status = outDefFile.seekBegin();
        if(status.ok())
            {
            status = outDefFile.write(&buf[0], static_cast<int>(buf.size()));
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to write coverage source");
            }
        }
    }

void CppInstr::updateCoverageHeader(OovStringRef const fn, OovStringRef const covDir,
        int numInstrLines)
    {
    CoverageHeader header;
    header.update(header.getFn(covDir), fn, numInstrLines);
    }

// This is for updating coverage information.  An alternative is to create a
// log that updates run-time info. The problem with this is that it can
// generate huge logs.  The user can comment out instrumentation lines, but
// it might be better if the user could select parts of source code to instrument.
// That code would look something like the following.
/*
class OovMonitorWriter
        {
        public:
                OovMonitorWriter():
                        mFp(0)
                        {}
                ~OovMonitorWriter()
                        {
                        fclose(mFp);
                        }
                void append(int fileIndex, int instrIndex)
                        {
                        if(!mFp)
                                {
                                mFp = fopen("OovMonitor.txt", "a");
                                }
                                {
//                              std::lock_guard<std::mutex> writeMutexLock(mWriteMutex);
                                std::stringstream id;
                                id << std::this_thread::get_id();
                                fprintf(mFp, "%s %d %d\n", id.str().c_str(), fileIndex, instrIndex);
                                fflush(mFp);
                                }
                        }

        private:
                FILE *mFp;
//              std::mutex mWriteMutex;
        };

OovMonitorWriter sOovMonitor;

void OovMonitor(int fileIndex, int instrIndex)
        {
        sOovMonitor.append(fileIndex, instrIndex);
        }
*/
void CppInstr::updateCoverageSource(OovStringRef const /*fn*/, OovStringRef const covDir)
    {
    FilePath outFn(covDir, FP_Dir);
    outFn.appendDir("covLib");
    OovStatus status = FileEnsurePathExists(outFn);
    if(status.ok())
        {
        outFn.appendFile("OovCoverage.cpp");

            if(!FileIsFileOnDisk(outFn, status))
            {
            File file;
            status = file.open(outFn, "w");

                static char const *lines[] = {
                "// Automatically generated file by OovCovInstr\n",
                "// This appends coverage data to either a new or existing file,\n"
                "// although the number of instrumented lines in the project must match.\n"
                "// This file must be compiled and linked into the project.\n",
                "#include <stdio.h>\n",
                "#include \"OovCoverage.h\"\n",
                "\n",
                "unsigned short gCoverage[COV_TOTAL_INSTRS];\n",
                "\n",
                "class cCoverageOutput\n",
                "  {\n",
                "  public:\n",
                "  cCoverageOutput()\n",
                "    {\n",
                "    // Initialize because some compilers may not initialize statics (TI)\n",
                "    for(int i=0; i<COV_TOTAL_INSTRS; i++)\n",
                "      gCoverage[i] = 0;\n",
                "    }\n",
                "  ~cCoverageOutput()\n",
                "    {\n",
                "      update();\n",
                "    }\n",
                "  void update()\n",
                "    {\n",
                "    read();\n",
                "    write();\n",
                "    }\n",
                "\n",
                "  private:\n",
                "  int getFirstIntFromLine(FILE *fp)\n",
                "    {\n",
                "   char buf[80];\n",
                "   fgets(buf, sizeof(buf), fp);\n",
                "   unsigned int tempInt = 0;\n",
                "           sscanf(buf, \"%u\", &tempInt);\n",
                "   return tempInt;\n",
                "    }\n",
                "  void read()\n",
                "    {\n",
                "    FILE *fp = fopen(\"OovCoverageCounts.txt\", \"r\");\n",
                "    if(fp)\n",
                "      {\n",
                "      int numInstrs = getFirstIntFromLine(fp);\n",
                "      if(numInstrs == COV_TOTAL_INSTRS)\n",
                "        {\n",
                "        for(int i=0; i<COV_TOTAL_INSTRS; i++)\n",
                "          {\n",
                "          gCoverage[i] += getFirstIntFromLine(fp);\n",
                "          }\n",
                "        }\n",
                "      fclose(fp);\n",
                "      }\n",
                "    }\n",
                "  void write()\n",
                "    {\n",
                "    FILE *fp = fopen(\"OovCoverageCounts.txt\", \"w\");\n",
                "    if(fp)\n",
                "      {\n",
                "      fprintf(fp, \"%d   # Number of instrumented lines\\n\", COV_TOTAL_INSTRS);\n",
                "      for(int i=0; i<COV_TOTAL_INSTRS; i++)\n",
                "        {\n",
                "        fprintf(fp, \"%u\", gCoverage[i]);\n",
                "        gCoverage[i] = 0;\n",
                "        fprintf(fp, \"\\n\");\n",
                "        }\n",
                "      fclose(fp);\n",
                "      }\n",
                "    }\n",
                "  };\n",
                "\n",
                "cCoverageOutput coverageOutput;\n"
                "\n",
                "void updateCoverage()\n",
                "  { coverageOutput.update(); }\n"

                };
            for(size_t i=0; i<sizeof(lines)/sizeof(lines[0]) && status.ok(); i++)
                {
                status = file.putString(lines[i]);
                if(!status.ok())
                    {
                    break;
                    }
                }
            }
        }
    if(!status.ok())
        {
        OovString err = "Unable to update coverage source ";
        err += outFn;
        status.report(ET_Error, err);
        }
    }

CppInstr::eErrorTypes CppInstr::parse(OovStringRef const srcFn, OovStringRef const srcRootDir,
        OovStringRef const outDir,
        char const * const clang_args[], int num_clang_args)
    {
    eErrorTypes errType = ET_None;

    mOutputFileContents.read(srcFn);
    mTopParseFn.setPath(srcFn, FP_File);
    FilePath rootDir(srcRootDir, FP_Dir);
    setFileDefine(mTopParseFn, rootDir);

    CXIndex index = clang_createIndex(1, 1);

// This doesn't appear to change anything.
//    clang_toggleCrashRecovery(true);
    // Get inclusion directives to be in AST.
    unsigned options = 0;
    CXTranslationUnit tu;
    CXErrorCode errCode = clang_parseTranslationUnit2(index, srcFn,
        clang_args, num_clang_args, 0, 0, options, &tu);
    if(errCode == CXError_Success)
        {
        CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
        try
            {
            clang_visitChildren(rootCursor, ::visitTranslationUnit, this);
            }
        catch(...)
            {
            errType = ET_ParseError;
            sCrashDiagnostics.setCrashed();
            }

        std::string outFileName;
        for(int i=0; i<num_clang_args; i++)
            {
            std::string testArg = clang_args[i];
            if(testArg.compare("-o") == 0)
                {
                if(i+1 < num_clang_args)
                    outFileName = clang_args[i+1];
                }
            }
        try
            {
            mOutputFileContents.write(outFileName);
            }
        catch(...)
            {
            errType = ET_ParseError;
            }
        std::string outErrFileName = outFileName;
        outErrFileName += "-err.txt";
        size_t numDiags = clang_getNumDiagnostics(tu);
        if(numDiags > 0 || sCrashDiagnostics.hasCrashed())
            {
            FILE *fp = fopen(outErrFileName.c_str(), "w");
            if(fp)
                {
                sCrashDiagnostics.dumpCrashed(fp);
                for (size_t i = 0; i<numDiags; i++)
                    {
                    CXDiagnostic diag = clang_getDiagnostic(tu, i);
                    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
                    if(errType == ET_None || errType == ET_CompileWarnings)
                        {
                        if(sev >= CXDiagnostic_Error)
                            errType = ET_CompileErrors;
                        else
                            errType = ET_CompileWarnings;
                        }
                    CXStringDisposer diagStr = clang_formatDiagnostic(diag,
                        clang_defaultDiagnosticDisplayOptions());
                        fprintf(fp, "%s\n", diagStr.c_str());
                    }

                fprintf(fp, "Arguments: %s %s %s ", static_cast<char const *>(srcFn),
                        static_cast<char const *>(srcRootDir),
                        static_cast<char const *>(outDir));
                for(int i=0 ; i<num_clang_args; i++)
                    {
                    fprintf(fp, "%s ", clang_args[i]);
                    }
                fprintf(fp, "\n");

                fclose(fp);
                }
            }
        else
            {
            unlink(outErrFileName.c_str());
            }
        FilePath covDir(outDir, FP_Dir);
        updateCoverageHeader(mTopParseFn, covDir, mInstrCount);
        updateCoverageSource(mTopParseFn, covDir);
        }
    else
        {
        errType = ET_CLangError;
        }
    return errType;
    }
