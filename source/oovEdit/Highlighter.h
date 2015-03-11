/*
 * Highlighter.h
 *
 *  Created on: Sep 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include "clang-c/Index.h"
#include "Gui.h"
#include "OovString.h"
#include "OovThreadedBackgroundQueue.h"
#include "OovProcess.h"

struct Token
    {
    CXTokenKind mTokenKind;
    unsigned int mStartOffset;
    unsigned int mEndOffset;
    };

class TokenRange:public std::vector<Token>
    {
    public:
	void tokenize(CXTranslationUnit transUnit/* , CXFile srcFile,
		int startLine, int endLine*/);
    };

enum eFindTokenTypes { FT_FindDecl, FT_FindDef };

class Tokenizer
    {
    public:
	Tokenizer():
	    mTransUnit(0), mSourceFile(nullptr)
	    {}
	~Tokenizer();
	void parse(OovStringRef fileName, OovStringRef buffer, int bufLen,
		char const * const clang_args[], int num_clang_args);
	// line numbers are 1 based.
	void tokenize(/*int startLine, int endLine,*/ TokenRange &highlight);
	bool findToken(eFindTokenTypes ft, int origOffset, std::string &fn, int &offset);
	OovStringVec getMembers(int offset);

    private:
	CXTranslationUnit mTransUnit;
	std::mutex mTransUnitMutex;
	CXFile mSourceFile;
	std::string mSourceFilename;
    };

class HighlightTag
    {
    public:
	HighlightTag();
	~HighlightTag();
	void setForegroundColor(GtkTextBuffer *textBuffer, char const * const name,
		char const * const color);
	bool isInitialized() const
	    { return mTag != nullptr; }
	GtkTextTag *getTextTag() const
	    { return mTag; }
    private:
	GtkTextTag *mTag;
    };

class HighlightTags
    {
    public:
	void initTags(GtkTextBuffer *textBuffer);
	GtkTextTag *getTag(int tagIndex)
	    { return mTags[tagIndex].getTextTag(); }

    private:
	HighlightTag mTags[5];
    };


enum eHighlightTask
    {
    HT_None,
    // These can be ored together when they are used as result flags.
    HT_Parse=0x01, HT_FindToken=0x02, HT_ShowMembers=0x04
    };


// A shared queue was used between all highlighters, but didn't solve the
// problem in Windows, that the CPU usage goes up as each file view is added in
// the editor. It must be the parser in the mTokenizer.
// This flag doesn't totally work yet. Probably the waitForCompletion.
#define SHARED_QUEUE 0

class HighlightTaskItem
    {
    public:
#if(SHARED_QUEUE)
        HighlightTaskItem(class Highlighter *highlighter=nullptr):
            mHighlighter(highlighter),
#else
	HighlightTaskItem():
#endif
            mTask(HT_None), mOffset(0), mFindTokenFt(FT_FindDecl)
            {}
        void setParseTask(OovStringRef const buffer, int bufLen)
            {
            mTask = HT_Parse;
	    mParseSourceBuffer.assign(buffer, bufLen);
            }
        void setShowMembersTask(int offset)
            {
            mTask = HT_ShowMembers;
            mOffset = offset;
            }
        void setFindTokenTask(eFindTokenTypes ft, int origOffset)
            {
            mTask = HT_FindToken;
            mFindTokenFt = ft;
            mOffset = origOffset;
            }
        eHighlightTask getTask() const
            { return mTask; }

    public:
#if(SHARED_QUEUE)
        class Highlighter *mHighlighter;
#endif
        eHighlightTask mTask;

	// Parameters needed for background thread parsing
	OovString mParseSourceBuffer;

        int mOffset;

        eFindTokenTypes mFindTokenFt;
    };


#if(SHARED_QUEUE)
// This queue is shared by all highlighters. There is a highlighter for each
// view.
class HighlighterSharedQueue:public ThreadedWorkBackgroundQueue<
    class HighlighterSharedQueue, HighlightTaskItem>
    {
    public:
        void processItem(HighlightTaskItem const &item);
    };
#endif

class Highlighter:public ThreadedWorkBackgroundQueue<
    class Highlighter, HighlightTaskItem>
    {
    public:
	Highlighter():
	    mTaskResults(HT_None), mFindTokenResultOffset(0),
		mParseRequestCounter(0), mParseFinishedCounter(0)
	    {
	    }
	~Highlighter()
	    {
#if(SHARED_QUEUE)
            sSharedQueue.waitForCompletion();
#else
            stopAndWaitForCompletion();
#endif
	    }
	/// @todo - this interface requires that no parameters change during
	/// the lifetime of this class.
	void highlightRequest(OovStringRef const filename,
		char const * const clang_args[], int num_clang_args);
        // Return is OR'ed values that show available task results.
        //      HT_Parse is handled internally.
        //      HT_ShowMembers, call getShowMembers().
	eHighlightTask highlightUpdate(GtkTextView *textView, OovStringRef const buffer,
            int bufLen);
        void showMembers(int offset);
        OovStringVec getShowMembers();
        void findToken(eFindTokenTypes ft, int origOffset);
        void getFindTokenResults(std::string &fn, int &offset);
        // Called by HighlighterSharedQueue on background thread.
        void processItem(HighlightTaskItem const &item);

    private:
	Tokenizer mTokenizer;
	HighlightTags mHighlightTags;
	OovString mFilename;
	OovProcessChildArgs mClang_args;

        std::mutex mResultsLock;
        eHighlightTask mTaskResults;
#if(SHARED_QUEUE)
        static HighlighterSharedQueue sSharedQueue;
#endif
	TokenRange mTokenResults;
        OovStringVec mShowMemberResults;
        OovString mFindTokenResultFilename;
        int mFindTokenResultOffset;

	int mParseRequestCounter;
	int mParseFinishedCounter;
	void applyTags(GtkTextBuffer *textBuffer, const TokenRange &tokens);
    };

#endif /* HIGHLIGHTER_H_ */
