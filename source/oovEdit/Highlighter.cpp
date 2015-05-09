/*
 * Highlighter.cpp
 *
 *  Created on: Sep 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Highlighter.h"
#include "Debug.h"
#include <chrono>


#if(SHARED_QUEUE)
HighlighterSharedQueue Highlighter::sSharedQueue;
#endif


class CXStringDisposer:public std::string
    {
    public:
	CXStringDisposer(const CXString &xstr):
	    std::string(clang_getCString(xstr))
	    {
	    clang_disposeString(xstr);
	    }
    };

#define DEBUG_PARSE 0
#define DEBUG_HIGHLIGHT 0

#if(DEBUG_PARSE)
static DebugFile sLog("DebugEditParse.txt", false);
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
static void dumpCursor(char const * const str, CXCursor cursor)
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

    fprintf(sLog.mFp, "%s: %s %s   %s\n", str, spstr.c_str(), name.c_str(), tokenStr.c_str());
    fflush(sLog.mFp);
    }
static void dumpInt(char const * const str, int val)
    {
    fprintf(sLog.mFp, "%s: %d %x\n", str, val, val);
    fflush(sLog.mFp);
    }
#define DUMP_PARSE(str, cursor) dumpCursor(str, cursor);
#define DUMP_PARSE_INT(str, val) dumpInt(str, val);
#else
#define DUMP_PARSE(str, cursor)
#define DUMP_PARSE_INT(str, val)
#endif

#if(DEBUG_HIGHLIGHT)
static DebugFile sLog("DebugEditHighlight.txt", false);
static void dump(char const * const str, int val)
    {
    fprintf(sLog.mFp, "%s %d\n", str, val);
    fflush(sLog.mFp);
    }
#define DUMP_THREAD(str) dump(str, 0);
#define DUMP_THREAD_INT(str, val) dump(str, val);
#else
#define DUMP_THREAD(str)
#endif


void TokenRange::tokenize(CXTranslationUnit transUnit/*, CXFile srcFile ,
	int startLine, int endLine*/)
    {
    CXCursor cursor = clang_getTranslationUnitCursor(transUnit);
    CXSourceRange range = clang_getCursorExtent(cursor);

    CXToken *tokens = 0;
    unsigned int numTokens = 0;
    clang_tokenize(transUnit, range, &tokens, &numTokens);
    resize(numTokens);
    for (size_t i = 0; i < numTokens-1; i++)
	{
	at(i).mTokenKind = clang_getTokenKind(tokens[i]);
	CXSourceRange tokRange = clang_getTokenExtent(transUnit, tokens[i]);
	clang_getExpansionLocation(clang_getRangeStart(tokRange), NULL, NULL,
		NULL, &at(i).mStartOffset);
	clang_getExpansionLocation(clang_getRangeEnd(tokRange), NULL, NULL,
		NULL, &at(i).mEndOffset);
	}
    clang_disposeTokens(transUnit, tokens, numTokens);
    }

Tokenizer::~Tokenizer()
    {
    // There is a bug somewhere that the wrong tokenizer is deleted.
    // Probably related to EditFiles::removeNotebookPage(GtkWidget *pageWidget)
    if(mTransUnit)
	clang_disposeTranslationUnit(mTransUnit);
    }

void Tokenizer::parse(OovStringRef fileName, OovStringRef buffer, size_t bufLen,
	char const * const clang_args[], size_t num_clang_args)
    {
    try {
//    unsigned options = clang_defaultEditingTranslationUnitOptions();
    // This is needed to support include directives.
//    options |= CXTranslationUnit_DetailedPreprocessingRecord;
// These options are not for the parse function, they are for the code complete at func.
//    unsigned options = clang_defaultCodeCompleteOptions();
    unsigned options = clang_defaultEditingTranslationUnitOptions();
    if(!mSourceFile)
	{
	mSourceFilename = fileName;
	CXIndex index = clang_createIndex(1, 1);
	mTransUnitMutex.lock();
	mTransUnit = clang_parseTranslationUnit(index, fileName,
	    clang_args, static_cast<int>(num_clang_args), 0, 0, options);
	mTransUnitMutex.unlock();

#if(0)
	printf("%s\n", fileName);
	int numDiags = clang_getNumDiagnostics(mTransUnit);
	for (int i = 0; i<numDiags; i++)
	    {
	    CXDiagnostic diag = clang_getDiagnostic(mTransUnit, i);
//	    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
//	    if(sev >= CXDiagnostic_Error)
	    CXStringDisposer diagStr = clang_formatDiagnostic(diag,
		clang_defaultDiagnosticDisplayOptions());
		printf("%s\n", diagStr.c_str());
	    }
	fflush(stdout);
#endif
	}
    else
	{
	CXUnsavedFile file;
	file.Filename = mSourceFilename.c_str();
	file.Contents = buffer;
	file.Length = bufLen;
	mTransUnitMutex.lock();
	int stat = clang_reparseTranslationUnit(mTransUnit, 1, &file, options);
	if(stat != 0)
	    {
	    clang_disposeTranslationUnit(mTransUnit);
	    mTransUnit = nullptr;
	    }
	mTransUnitMutex.unlock();
	}
    mSourceFile = clang_getFile(mTransUnit, fileName);
	}
    catch(...)
	{
	DUMP_PARSE_INT("Tokenizer::parse - CRASHED", 0);
	}
    }

void Tokenizer::tokenize(/*int startLine, int endLine, */TokenRange &tokens)
    {
    mTransUnitMutex.lock();
    if(mTransUnit)
	tokens.tokenize(mTransUnit/*, mSourceFile, startLine, endLine*/);
    mTransUnitMutex.unlock();
    }


#if(0)
static CXCursor getCursorUsingTokens(CXTranslationUnit tu, CXCursor cursor,
	unsigned int desiredOffset)
    {
    // The following does not return a more definitive cursor.
    // For example, if a compound statement is returned, this does not find
    // any variables in the statement.
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens;
    unsigned numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);
    unsigned int closestOffset = 0;
    for(unsigned int i=0; i<numTokens; i++)
	{
	CXSourceLocation loc = clang_getTokenLocation(tu, tokens[i]);
	unsigned int offset;
	CXFile file;
	clang_getSpellingLocation(loc, &file, nullptr, nullptr, &offset);
	if(offset < desiredOffset && offset > closestOffset)
	    {
	    closestOffset = offset;
	    cursor = clang_getCursor(tu, loc);
	    }
	}
    clang_disposeTokens(tu, tokens, numTokens);
    return cursor;
    }
#endif

#if(0)
struct visitTranslationUnitData
    {
    visitTranslationUnitData(CXFile file, unsigned offset):
	mFile(file), mDesiredOffset(offset), mSize(INT_MAX)
	{}
    CXFile mFile;
    unsigned mDesiredOffset;
    unsigned mSize;
    CXCursor mCursor;
    };

#include <memory.h>
static bool clangFilesEqual(CXFile f1, CXFile f2)
    {
// Not declared?
//    return clang_File_isEqual(f1, f2);
    CXFileUniqueID f1id;
    CXFileUniqueID f2id;
    if(f1)
	clang_getFileUniqueID(f1, &f1id);
    if(f2)
	clang_getFileUniqueID(f2, &f2id);
    return(memcmp(&f1id.data, &f2id.data, sizeof(f1id.data)) == 0);
    }

static CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent,
	CXClientData client_data)
    {
    visitTranslationUnitData *data = static_cast<visitTranslationUnitData*>(client_data);
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXSourceLocation startLoc = clang_getRangeStart(range);
    CXSourceLocation endLoc = clang_getRangeEnd(range);
    CXFile file;
    unsigned startOffset;
    unsigned endOffset;
    clang_getSpellingLocation(startLoc, &file, nullptr, nullptr, &startOffset);
    clang_getSpellingLocation(endLoc, &file, nullptr, nullptr, &endOffset);
    // Use the smallest cursor that surrounds the desired location.
    unsigned size = endOffset - startOffset;
    if(clangFilesEqual(file, data->mFile) &&
	    startOffset < data->mDesiredOffset && endOffset > data->mDesiredOffset)
	{
	if(size < data->mSize)
	    {
	    data->mCursor = cursor;
	    data->mSize = size;

	    fprintf(sLog.mFp, "GOOD:\n   ");
	    }
	}
    CXStringDisposer sp = clang_getCursorSpelling(cursor);
    CXStringDisposer kind = clang_getCursorKindSpelling(cursor.kind);
    std::string fn;
    if(file)
	{
	CXStringDisposer s = clang_getFileName(file);
	fn = s;
	}
    fprintf(sLog.mFp, "%s %s off %d size %d des offset %d file %s\n",
	    kind.c_str(), sp.c_str(), startOffset,
	    size, data->mDesiredOffset, fn.c_str());
    DUMP_PARSE("visitTU", cursor);
//    clang_visitChildren(cursor, ::visitTranslationUnit, client_data);
//    return CXChildVisit_Continue;
    return CXChildVisit_Recurse;
    }

static CXCursor myGetCursorAtOffset(CXTranslationUnit tu, CXFile file, unsigned offset)
    {
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    visitTranslationUnitData data(file, offset);
    clang_visitChildren(rootCursor, ::visitTranslationUnit, &data);
    fprintf(sLog.mFp, "--\n");
    fflush(sLog.mFp);
    CXCursor cursor = data.mCursor;
    return cursor;
    }
#endif

// If this doesn't descend to a detailed cursor, then most likely there is a compile
// error.
// If it returns CXCursor_NoDeclFound on an include directive, then the options for
// clang_parseTranslationUnit do not support it.
static CXCursor getCursorAtOffset(CXTranslationUnit tu, CXFile file,
	unsigned desiredOffset)
    {
    CXSourceLocation loc = clang_getLocationForOffset(tu, file, desiredOffset);
    CXCursor cursor = clang_getCursor(tu, loc);
//    CXSourceRange range = clang_getTokenExtent(tu, CXToken)
//    cursor = getCursorUsingTokens(tu, cursor, desiredOffset);
    return cursor;
    }

void Tokenizer::getLineColumn(size_t charOffset, unsigned int &line, unsigned int &column)
    {
    CXSourceLocation loc = clang_getLocationForOffset(mTransUnit, mSourceFile, charOffset);
    clang_getSpellingLocation(loc, nullptr, &line, &column, nullptr);
    }


// Desired functions:
//	Go to definition of variable/function
//	Go to declaration of function/class
bool Tokenizer::findToken(eFindTokenTypes ft, size_t origOffset, std::string &fn,
	size_t &line)
    {
    mTransUnitMutex.lock();
    CXCursor startCursor = getCursorAtOffset(mTransUnit, mSourceFile, origOffset);
    DUMP_PARSE("find:start cursor", startCursor);
    // Instantiating type - <class> <type> - CXCursor_TypeRef
    // Method declaration - <class> { <method>(); }; CXCursor_NoDeclFound
    // Method definition - <class>::<method>(){} - CXCursor_CXXMethod
    // Class/method usage - <class>.<method>()
    // Instance usage - method(){ int v = typename[4]; } - CXCursor_DeclStmt
    //		clang_getCursorSemanticParent returns method
    //		clang_getCursorDefinition returns invalid
    if(startCursor.kind == CXCursor_InclusionDirective)
	{
	CXFile file = clang_getIncludedFile(startCursor);
	if(file)
	    {
	    CXStringDisposer cfn = clang_getFileName(file);
	    fn = cfn;
	    line = 1;
	    }
	else
	    {
	    /// @todo - need to get the full path.
//	    CXStringDisposer cfn = clang_getCursorSpelling(cursor);
//	    fn = cfn;
	    line = 1;
	    }
	}
    else
	{
	CXCursor cursor = startCursor;
	switch(ft)
	    {
	    case FT_FindDecl:
		cursor = clang_getCursorReferenced(startCursor);
		DUMP_PARSE("find:decl", cursor);
		break;

	    case FT_FindDef:
		// If instantiating a type (CXCursor_TypeRef), this goes to the type declaration.
		cursor = clang_getCursorDefinition(startCursor);
		// Method call can return invalid file when the definition is not in this
		// translation unit.  This really needs to load the file with the definition.
		if(clang_getCursorKind(cursor) == CXCursor_InvalidFile)
		    {
		    DUMP_PARSE("find:def-invalid", cursor);
		    cursor = clang_getCursorReferenced(startCursor);
		    }
		DUMP_PARSE("find:def", cursor);
    //	    cursor = clang_getCursor(mTransUnit, clang_getCursorLocation(cursor));
    //	    cursor = clang_getCursorDefinition(cursor);
		break;
    //    	cursor = clang_getCursorReferenced(cursor);
    //	cursor = clang_getCanonicalCursor(cursor);
    //	cursor = clang_getCursorSemanticParent(cursor);
    //	cursor = clang_getCursorLexicalParent(cursor);
	    }
	if(!clang_Cursor_isNull(cursor))
	    {
	    CXSourceLocation loc = clang_getCursorLocation(cursor);

	    CXFile file;
	    unsigned int uline;
	    clang_getFileLocation(loc, &file, &uline, nullptr, nullptr);
	    if(file)
		{
		line = uline;
		CXStringDisposer cfn = clang_getFileName(file);
		fn = cfn;
		}
	    }
	}
    mTransUnitMutex.unlock();
    return(fn.size() > 0);
    }


#if(!CODE_COMPLETE)

struct visitClassData
    {
    visitClassData(OovStringVec &members):
	mMembers(members)
	{}
    OovStringVec &mMembers;
    };

static CXChildVisitResult visitClass(CXCursor cursor, CXCursor /*parent*/,
	CXClientData client_data)
    {
    visitClassData *data = static_cast<visitClassData*>(client_data);
    DUMP_PARSE("visitClass", cursor);
    switch(cursor.kind)
	{
	case CXCursor_ClassDecl:
	    clang_visitChildren(cursor, ::visitClass, &data);
	    break;

/* Prevent infinite recursion
	case CXCursor_CXXBaseSpecifier:
	    {
	    CXType classCursorType = clang_getCursorType(cursor);
	    CXCursor classCursor = clang_getTypeDeclaration(classCursorType);
	    clang_visitChildren(classCursor, ::visitClass, &data);
	    }
	    break;
*/

	case CXCursor_CXXMethod:
	case CXCursor_FieldDecl:
	    {
	    CXStringDisposer name(clang_getCursorSpelling(cursor));
	    data->mMembers.push_back(name);
	    }
	    break;

	default:
	    break;
	}
    return CXChildVisit_Continue;
    // return CXChildVisit_Recurse;
    }
#endif

#if(CODE_COMPLETE)
OovStringVec Tokenizer::codeComplete(size_t offset)
    {
    OovStringVec strs;
    unsigned options = 0;
// This gets more than we want.
//    unsigned options = clang_defaultCodeCompleteOptions();
    unsigned int line;
    unsigned int column;
    getLineColumn(offset, line, column);
    CXCodeCompleteResults *results = clang_codeCompleteAt(mTransUnit,
	    mSourceFilename.getStr(), line, column,
	    nullptr, 0, options);
    if(results)
	{
	clang_sortCodeCompletionResults(&results->Results[0], results->NumResults);
	for(size_t ri=0; ri<results->NumResults /*&& ri < 50*/; ri++)
	    {
	    OovString str;
	    CXCompletionString compStr = results->Results[ri].CompletionString;
	    size_t numChunks = clang_getNumCompletionChunks(compStr);
	    for(size_t ci=0; ci<numChunks && ci < 30; ci++)
		{
		CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(compStr, ci);
		// We will discard return values from functions, so the first
		// chunk returned will be the identifier or function name.  Function
		// arguments will be returned after a space, so they can be
		// discarded easily.
		if(chunkKind == CXCompletionChunk_TypedText || str.length())
		    {
		    CXStringDisposer chunkStr = clang_getCompletionChunkText(compStr, ci);
		    if(str.length() != 0)
			str += ' ';
		    str += chunkStr;
		    }
		}
	    strs.push_back(str);
	    }
	clang_disposeCodeCompleteResults(results);
	}
    return strs;
    }

#else

OovStringVec Tokenizer::getMembers(int offset)
    {
    OovStringVec members;
    visitClassData data(members);
    // The start cursor is probably a MemberRefExpr.
    mTransUnitMutex.lock();
    CXCursor memberRefCursor = getCursorAtOffset(mTransUnit, mSourceFile, offset);
    DUMP_PARSE_INT("getMembers", offset);
    DUMP_PARSE("getMembers:memberref", memberRefCursor);
    if(memberRefCursor.kind == CXCursor_MemberRefExpr ||
	memberRefCursor.kind == CXCursor_DeclRefExpr)
	{
	CXType classCursorType = clang_getCursorType(memberRefCursor);
	CXCursor classCursor = clang_getTypeDeclaration(classCursorType);
	DUMP_PARSE("getMembers:classref", classCursor);
	if(classCursor.kind == CXCursor_ClassDecl || classCursor.kind == CXCursor_StructDecl ||
		classCursor.kind == CXCursor_UnionDecl)
	    {
	    // CXCursor classCursor = clang_getCursorReferenced(startCursor);
	    // classCursor = clang_getCursorDefinition(classCursor);
	    clang_visitChildren(classCursor, ::visitClass, &data);
	    }
	}
    else
	{
//	myGetCursorAtOffset(mTransUnit, mSourceFile, offset);
	}
    mTransUnitMutex.unlock();
    return members;
    }
#endif


HighlightTag::HighlightTag():
	mTag(nullptr)
    {
    }

void HighlightTag::setForegroundColor(GtkTextBuffer *textBuffer,
	char const * const name, char const * const color)
    {
    if(!mTag)
	mTag = gtk_text_buffer_create_tag(textBuffer, name, NULL);
    g_object_set(G_OBJECT(mTag), "foreground", color, "foreground-set", TRUE, NULL);
    }

HighlightTag::~HighlightTag()
    {
    /// @todo - delete tag?
    }

void HighlightTags::initTags(GtkTextBuffer *textBuffer)
    {
    if(!mTags[CXToken_Punctuation].isInitialized())
	{
	mTags[CXToken_Punctuation].setForegroundColor(textBuffer, "punc", "black");
	mTags[CXToken_Keyword].setForegroundColor(textBuffer, "key", "blue");
	mTags[CXToken_Identifier].setForegroundColor(textBuffer, "iden", "black");
	mTags[CXToken_Literal].setForegroundColor(textBuffer, "lit", "red");
	mTags[CXToken_Comment].setForegroundColor(textBuffer, "comm", "dark green");
	}
    }


//////////////

HighlighterBackgroundThreadData::~HighlighterBackgroundThreadData()
    {
#if(SHARED_QUEUE)
    sSharedQueue.waitForCompletion();
#else
    stopAndWaitForCompletion();
#endif
    }

void HighlighterBackgroundThreadData::initArgs(OovStringRef const filename,
	char const * const clang_args[], int num_clang_args)
    {
    if(mClang_args.getArgc() == 0)
	{
	mFilename = filename;
	mClang_args.clearArgs();
	for(int i=0; i<num_clang_args; i++)
	    {
	    mClang_args.addArg(clang_args[i]);
	    }
	}
    }

OovStringVec HighlighterBackgroundThreadData::getShowMembersResults()
    {
    OovStringVec members = mShowMemberResults;
    std::lock_guard<std::mutex> lock(mResultsLock);
    mTaskResults = static_cast<eHighlightTask>(mTaskResults & ~HT_ShowMembers);
    return members;
    }

TokenRange HighlighterBackgroundThreadData::getParseResults()
    {
    TokenRange retTokens;
    std::lock_guard<std::mutex> lock(mResultsLock);
    if(mTaskResults & HT_Parse)
	{
	retTokens = std::move(mTokenResults);
	mTaskResults = static_cast<eHighlightTask>(mTaskResults & ~HT_Parse);
	}
    return retTokens;
    }

void HighlighterBackgroundThreadData::getFindTokenResults(std::string &fn,
    size_t &offset)
    {
    std::lock_guard<std::mutex> lock(mResultsLock);
    fn = mFindTokenResultFilename;
    offset = mFindTokenResultOffset;
    mTaskResults = static_cast<eHighlightTask>(mTaskResults & ~HT_FindToken);
    }

void HighlighterBackgroundThreadData::processItem(HighlightTaskItem const &item)
    {
    DUMP_THREAD("processItem");
    switch(item.getTask())
        {
        case HT_Parse:
            {
            DUMP_THREAD("processItem-Parse");
            int counter = mParseRequestCounter;
            mTokenizer.parse(mFilename, item.mParseSourceBuffer,
                item.mParseSourceBuffer.length(),
                mClang_args.getArgv(), mClang_args.getArgc());
            std::lock_guard<std::mutex> lock(mResultsLock);
            mParseFinishedCounter = counter;
            mTokenizer.tokenize(mTokenResults);
            mTaskResults = static_cast<eHighlightTask>(mTaskResults | HT_Parse);
            DUMP_THREAD("processItem-Parse end");
            }
            break;

        case HT_FindToken:
            {
            OovString fn;
            size_t offset;
            DUMP_THREAD("processItem-FindToken");
            if(mTokenizer.findToken(item.mFindTokenFt, item.mOffset,
                fn, offset))
                {
		std::lock_guard<std::mutex> lock(mResultsLock);
                mFindTokenResultFilename = fn;
                mFindTokenResultOffset = offset;
                mTaskResults = static_cast<eHighlightTask>(mTaskResults | HT_FindToken);
                }
            DUMP_THREAD("processItem-FindToken end");
            }
            break;

        case HT_ShowMembers:
            {
            DUMP_THREAD("processItem-ShowMembers");
#if(CODE_COMPLETE)
            OovStringVec vec = mTokenizer.codeComplete(item.mOffset);
#else
            OovStringVec vec = mTokenizer.getMembers(item.mOffset);
#endif
            std::lock_guard<std::mutex> lock(mResultsLock);
            mShowMemberResults = vec;
            mTaskResults = static_cast<eHighlightTask>(mTaskResults | HT_ShowMembers);
            DUMP_THREAD("processItem-ShowMembers end");
            }
            break;

        default:
            break;
        }
    }



//////////////

void Highlighter::highlightRequest(
	OovStringRef const filename,
	char const * const clang_args[], int num_clang_args)
    {
    DUMP_THREAD("highlightRequest");
/*
    GdkRectangle rect;
    gtk_text_view_get_visible_rect(textView, &rect);
    int startBufY;
    int endBufY;
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_WIDGET,
	    0, rect.y, NULL, &startBufY);
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_WIDGET,
	    0, rect.y+rect.height, NULL, &endBufY);

    // Get iterators
    GtkTextIter startIter;
    GtkTextIter endIter;
    gtk_text_view_get_line_at_y(textView, &startIter, startBufY, NULL);
    gtk_text_view_get_line_at_y(textView, &endIter, endBufY, NULL);
*/
//    int startLineY = gtk_text_iter_get_line(&startIter);
//    int endLineY = gtk_text_iter_get_line(&endIter);
//    if(startLineY < 1)
//	startLineY = 1;

    mBackgroundThreadData.initArgs(filename, clang_args, num_clang_args);
    mBackgroundThreadData.makeParseRequest();
    DUMP_THREAD("highlightRequest-end");
    }

eHighlightTask Highlighter::highlightUpdate(GtkTextView *textView,
	OovStringRef const buffer, size_t bufLen)
    {
    DUMP_THREAD("highlightUpdate");
    if(mBackgroundThreadData.isParseNeeded())
	{
#if(SHARED_QUEUE)
	if(!sSharedQueue.isQueueBusy())
#else
	if(!mBackgroundThreadData.isQueueBusy())
#endif
	    {
	    DUMP_THREAD("highlightUpdate - set parse task");
#if(SHARED_QUEUE)
	    HighlightTaskItem task(this);
#else
	    HighlightTaskItem task;
#endif
	    task.setParseTask(buffer, bufLen);
#if(SHARED_QUEUE)
	    sSharedQueue.addTask(task);
#else
	    mBackgroundThreadData.addTask(task);
#endif
	    }
	}
    if(!mBackgroundThreadData.isParseNeeded() &&
	    mBackgroundThreadData.getTaskResults() & HT_Parse)
	{
	applyTags(gtk_text_view_get_buffer(textView), mBackgroundThreadData.getParseResults());
	}
    DUMP_THREAD("highlightUpdate-end");
    return mBackgroundThreadData.getTaskResults();
    }


void Highlighter::showMembers(size_t offset)
    {
    DUMP_THREAD("showMembers");
#if(SHARED_QUEUE)
    HighlightTaskItem task(this);
#else
    HighlightTaskItem task;
#endif
    task.setShowMembersTask(offset);
#if(SHARED_QUEUE)
    sSharedQueue.addTask(task);
#else
    mBackgroundThreadData.addTask(task);
#endif
    }

void Highlighter::findToken(eFindTokenTypes ft, size_t origOffset)
    {
    DUMP_THREAD("findToken");
#if(SHARED_QUEUE)
    HighlightTaskItem task(this);
#else
    HighlightTaskItem task;
#endif
    task.setFindTokenTask(ft, origOffset);
#if(SHARED_QUEUE)
    sSharedQueue.addTask(task);
#else
    mBackgroundThreadData.addTask(task);
#endif
    }

// On Windows, when tags are applied, 38% of CPU time is used for
// seven oovBuilder source modules open.  This is true even though the
// applyTags function is not normally running while idle. When only half
// the tags for each file is applied, then it drops to 25% usage.
void Highlighter::applyTags(GtkTextBuffer *textBuffer, const TokenRange &tokens)
    {
    DUMP_THREAD("applyTags");
    mHighlightTags.initTags(textBuffer);

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(textBuffer, &start, &end);
    gtk_text_buffer_remove_all_tags(textBuffer, &start, &end);
    for(size_t i=0; i<tokens.size(); i++)
	{
	gtk_text_buffer_get_iter_at_offset(textBuffer, &start, 
            static_cast<gint>(tokens[i].mStartOffset));
	gtk_text_buffer_get_iter_at_offset(textBuffer, &end,
            static_cast<gint>(tokens[i].mEndOffset));
	gtk_text_buffer_apply_tag(textBuffer,
		mHighlightTags.getTag(tokens[i].mTokenKind), &start, &end);
	}
    DUMP_THREAD("applyTags-end");
    }

#if(SHARED_QUEUE)
void HighlighterSharedQueue::processItem(HighlightTaskItem const &item)
    {
    item.mHighlighter->processItem(item);
    }
#endif
