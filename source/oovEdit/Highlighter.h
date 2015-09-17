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

#if(CINDEX_VERSION_MAJOR >= 6)
#define CODE_COMPLETE 0
#else
#define CODE_COMPLETE 1
#endif

struct Token
    {
    CXTokenKind mTokenKind;
    unsigned int mStartOffset;
    unsigned int mEndOffset;
    };

class TokenRange:public std::vector<Token>
    {
    public:
        void tokenize(CXTranslationUnit transUnit);
    };

enum eFindTokenTypes { FT_FindDecl, FT_FindDef };

class CLangLock
    {
    friend class CLangAutoLock;
    public:
        void lock(int line, class Tokenizer *tok);
        void unlock();
    };

// Similar to std::lock_guard except there is a line number for debugging.
class CLangAutoLock
    {
    public:
        CLangAutoLock(CLangLock &lock, int line, class Tokenizer *tok):
            mLock(lock)
            { mLock.lock(line, tok); }
        ~CLangAutoLock()
            { mLock.unlock(); }

    private:
        CLangLock &mLock;
    };

/// The CLang translation unit must be protected from multithreading.
/// This class protects it using a mutex. All access to functions starting
/// with "clang_" must be protected using the lock.  Remember that there is
/// a separate tokenizer for each source file, so there are multiple locks.
///
/// All public functions are protected with a lock.
class Tokenizer
    {
    public:
        Tokenizer():
            mTransUnit(0), mSourceFile(nullptr)
            {}
        ~Tokenizer();
        void parse(OovStringRef fileName, OovStringRef buffer, size_t bufLen,
            char const * const clang_args[], size_t num_clang_args);
        // line numbers are 1 based.
        void tokenize(/*int startLine, int endLine,*/ TokenRange &highlight);
        bool findToken(eFindTokenTypes ft, size_t origOffset, std::string &fn,
            size_t &offset);
        OovString getClassNameAtLocation(size_t origOffset);
        void getMethodNameAtLocation(size_t origOffset, OovString &className,
            OovString &methodName);
#if(CODE_COMPLETE)
        OovStringVec codeComplete(size_t offset);
#else
        OovStringVec getMembers(size_t offset);
#endif

    private:
        CXTranslationUnit mTransUnit;
        CLangLock mCLangLock;
        CXFile mSourceFile;
        OovString mSourceFilename;
        CXCursor getCursorAtOffset(CXTranslationUnit tu, CXFile file,
            unsigned desiredOffset);
        void getLineColumn(size_t charOffset, unsigned int &line, unsigned int &column);
    };

class HighlightTags
    {
    public:
        void initTags(GtkTextBuffer *textBuffer);
        GtkTextTag *getTag(int tagIndex)
            { return mTags[tagIndex].getTextTag(); }

    private:
        GuiHighlightTag mTags[5];
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
        void setParseTask(OovStringRef const buffer, size_t bufLen)
            {
            mTask = HT_Parse;
            mParseSourceBuffer.assign(buffer, bufLen);
            }
        void setShowMembersTask(size_t offset)
            {
            mTask = HT_ShowMembers;
            mOffset = offset;
            }
        void setFindTokenTask(eFindTokenTypes ft, size_t origOffset)
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

        size_t mOffset;

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


// This contains all data that is used by the background thread.
// This means it also contains all data shared between the foreground and
// background thread.
class HighlighterBackgroundThreadData:public ThreadedWorkBackgroundQueue<
    class HighlighterBackgroundThreadData, HighlightTaskItem>
    {
    public:
        HighlighterBackgroundThreadData():
            mParseRequestCounter(0), mParseFinishedCounter(0),
            mTaskResults(HT_None), mFindTokenResultOffset(0)
            {}
        virtual ~HighlighterBackgroundThreadData();
        void initArgs(OovStringRef const filename,
            char const * const clang_args[], int num_clang_args);
        void makeParseRequest()
            { mParseRequestCounter++; }
        bool isParseNeeded() const
            { return(mParseRequestCounter != mParseFinishedCounter); }
        TokenRange getParseResults();
        OovStringVec getShowMembersResults();
        void getFindTokenResults(std::string &fn, size_t &offset);
        eHighlightTask getTaskResults() const
            { return mTaskResults; }
        // Called by HighlighterSharedQueue on background thread.
        void processItem(HighlightTaskItem const &item);
        Tokenizer &getTokenizer()
            { return mTokenizer; }

    private:
        OovString mFilename;
        OovProcessChildArgs mClang_args;
        Tokenizer mTokenizer;
        int mParseRequestCounter;
        int mParseFinishedCounter;
        std::mutex mResultsLock;

        // All results must be protected with mResultsLock.
        eHighlightTask mTaskResults;
        TokenRange mTokenResults;
        OovStringVec mShowMemberResults;
        OovString mFindTokenResultFilename;
        size_t mFindTokenResultOffset;
    };


// The GTK code takes more CPU processing time when more tags are applied.
// For this reason, the tags are only applied to the visible viewing
// area.  This makes a huge difference in how much processing time the
// editor takes.  When about 10 files of size of 10K were viewed,  a
// substantial amount of CPU power was taken (perhaps something like 50%)
class Highlighter
    {
    public:
        /// This interface requires that no parameters change during
        /// the lifetime of this class.
        Highlighter():
            mTokenState(TS_AppliedTokens)
            {}
        /// This can be called whenever the buffer for the file has changed.
        /// It will reparse the buffer.  It will initiate a parse of the
        /// file on the background thread.
        void highlightRequest(OovStringRef const filename,
            char const * const clang_args[], int num_clang_args);

        /// This should be periodically called from something like an idle function.
        /// Return is OR'ed values that show available task results.  This checks
        /// whether the background thread is complete and saves the tokens so
        /// they can be applied on tags whenever a draw is required.
        ///      HT_Parse is handled internally.
        ///      HT_ShowMembers, call getShowMembers().
        eHighlightTask highlightUpdate(GtkTextView *textView, OovStringRef const buffer,
            size_t bufLen);
        void showMembers(size_t offset);
        OovStringVec getShowMembers()
            { return mBackgroundThreadData.getShowMembersResults(); }
        void findToken(eFindTokenTypes ft, size_t origOffset);
        void getFindTokenResults(std::string &fn, size_t &offset)
            { return mBackgroundThreadData.getFindTokenResults(fn, offset); }
        OovString getClassNameAtLocation(size_t offset)
            {
            return mBackgroundThreadData.getTokenizer().getClassNameAtLocation(
                   offset);
            }
        void getMethodNameAtLocation(size_t offset, OovString &className,
                OovString &methodName)
            {
            mBackgroundThreadData.getTokenizer().getMethodNameAtLocation(
                    offset, className, methodName);
            }

        /// This should be called from the draw function.  It applies
        /// the tokens to the tags in the viewable area of the buffer.
        /// The viewable area is defined by topOffset and botOffset.
        bool applyTags(GtkTextBuffer *textBuffer, int topOffset, int botOffset);

    private:
        HighlighterBackgroundThreadData mBackgroundThreadData;
        HighlightTags mHighlightTags;
        TokenRange mHighlightTokens;
        enum TokenStates { TS_HighlightRequest, TS_GotTokens,
            TS_AppliedTokens };
        TokenStates mTokenState;

#if(SHARED_QUEUE)
        static HighlighterSharedQueue sSharedQueue;
#endif
    };

#endif /* HIGHLIGHTER_H_ */
