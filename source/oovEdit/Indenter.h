// Name        : Indenter.h
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#include "Gui.h"

struct StartLineInfo
    {
    StartLineInfo(GtkTextIter iter)
        {
        cursPosFromStartLine = gtk_text_iter_get_line_offset(&iter);
        cursPosFromStartBuf = gtk_text_iter_get_offset(&iter);
        }
    int cursPosFromStartLine;
    int cursPosFromStartBuf;
    int getStartLinePosFromStartBuf() const
        { return (cursPosFromStartBuf-cursPosFromStartLine); }
    };


// Tab at beginning of line goes to previous line indent. If prev line is 0, then tab.
class Indenter
    {
    public:
        Indenter():
            mTextBuffer(nullptr)
            {}
        void init(GtkTextBuffer *textBuffer)
            { mTextBuffer = textBuffer; }
        /// Run key press functions from GtkWidget key-press-event
        /// Returns true for key handled.
        bool tabPressed();
        /// Returns true for key handled.
        bool shiftTabPressed();
        /// Returns true for key handled.
        bool backspacePressed();
        /// Alternates between beginning of line and first non-space character in line.
        /// Returns true for key handled.
        bool homePressed();

    private:
        GtkTextBuffer *mTextBuffer;
        static int getIndentSize()
            { return 4; }
        void dentHighlightedRegion(bool in, GtkTextIter startSelIter, GtkTextIter endSelIter);
        void getFirstNonSpaceOnLineIter(GtkTextIter startLineIter, GtkTextIter *iter);
        int countSpaces(char const * const text, bool expandTabs);
        void getCursorIter(GtkTextIter *iter);
        int getLineNumber(GtkTextIter iter);
        void addSpaces(int count);
    };

