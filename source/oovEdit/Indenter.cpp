// Name        : Indenter.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#include "Indenter.h"
#include <ctype.h>	// For isspace
#include <memory.h>	// For memset


bool Indenter::tabPressed()
    {
    GtkTextIter startSelIter;
    GtkTextIter endSelIter;
    if(gtk_text_buffer_get_selection_bounds(mTextBuffer, &startSelIter, &endSelIter))
	{
	if(getLineNumber(startSelIter) == getLineNumber(endSelIter))
	    {
	    GtkTextIter cursPos;
	    getCursorIter(&cursPos);
	    int cursPosFromStartLine = gtk_text_iter_get_line_offset(&cursPos);
	    gtk_text_buffer_delete(mTextBuffer, &startSelIter, &endSelIter);
	    addSpaces(getIndentSize() - (cursPosFromStartLine % getIndentSize()));
	    }
	else
	    {
	    dentHighlightedRegion(true, startSelIter, endSelIter);
	    }
	}
    else
	{
	GtkTextIter cursPos;
	getCursorIter(&cursPos);
	int cursPosFromStartLine = gtk_text_iter_get_line_offset(&cursPos);
	if(cursPosFromStartLine == 0)
	    {
	    // Indent to previous line indent
	    GtkTextIter prevLine = cursPos;
	    gtk_text_iter_backward_line(&prevLine);
	    GuiText text(gtk_text_buffer_get_text(mTextBuffer, &prevLine,
		    &cursPos, true));
	    int spaces = countSpaces(text.c_str(), true);
	    if(spaces < getIndentSize())
		spaces = getIndentSize();
	    addSpaces(spaces);
	    }
	else
	    {
	    addSpaces(getIndentSize() - (cursPosFromStartLine % getIndentSize()));
	    }
	}
    return true;
    }

bool Indenter::shiftTabPressed()
    {
    GtkTextIter startSelIter;
    GtkTextIter endSelIter;
    bool handled = false;
    if(gtk_text_buffer_get_selection_bounds(mTextBuffer, &startSelIter, &endSelIter))
	{
	dentHighlightedRegion(false, startSelIter, endSelIter);
	handled = true;
	}
    return handled;
    }

bool Indenter::backspacePressed()
    {
    GtkTextIter startSelIter;
    GtkTextIter endSelIter;
    bool handled = false;
    if(!gtk_text_buffer_get_selection_bounds(mTextBuffer, &startSelIter, &endSelIter))
	{
	handled = true;
	GtkTextIter cursPos;
	getCursorIter(&cursPos);
	StartLineInfo startLine(cursPos);

	if(startLine.cursPosFromStartLine != 0)
	    {
	    int backTabSize = (startLine.cursPosFromStartLine % getIndentSize());
	    if(backTabSize == 0 && startLine.cursPosFromStartLine >= getIndentSize())
		backTabSize = getIndentSize();

	    GtkTextIter backPosIter;
	    gtk_text_buffer_get_iter_at_offset(mTextBuffer, &backPosIter,
		    startLine.cursPosFromStartBuf-backTabSize);
	    GuiText text(gtk_text_buffer_get_text(mTextBuffer, &backPosIter,
		    &cursPos, true));
	    if(countSpaces(text.c_str(), true) < backTabSize)
		{
		gtk_text_buffer_get_iter_at_offset(mTextBuffer, &backPosIter,
			startLine.cursPosFromStartBuf-1);
		}
	    gtk_text_buffer_delete(mTextBuffer, &backPosIter, &cursPos);
	    }
	else
	    handled = false;
	}
    return handled;
    }

bool Indenter::homePressed()
    {
    GtkTextIter iter;
    GtkTextIter cursPos;
    getCursorIter(&cursPos);

    GtkTextIter startLineIter;
    StartLineInfo startLine(cursPos);
    gtk_text_buffer_get_iter_at_offset(mTextBuffer, &startLineIter,
	    startLine.getStartLinePosFromStartBuf());
    if(startLine.cursPosFromStartLine != 0)
	{
	iter = startLineIter;
	}
    else
	{
	getFirstNonSpaceOnLineIter(startLineIter, &iter);
	}
    gtk_text_buffer_select_range(mTextBuffer, &iter, &iter);
    return true;
    }

void Indenter::getFirstNonSpaceOnLineIter(GtkTextIter startLineIter, GtkTextIter *iter)
    {
    GtkTextIter endLineIter = startLineIter;
    gtk_text_iter_forward_line(&endLineIter);
    GuiText text(gtk_text_buffer_get_text(mTextBuffer, &startLineIter,
	    &endLineIter, true));
    StartLineInfo startLine(startLineIter);
    gtk_text_buffer_get_iter_at_offset(mTextBuffer, iter,
	    countSpaces(text.c_str(), false) + startLine.getStartLinePosFromStartBuf());
    }

void Indenter::dentHighlightedRegion(bool in, GtkTextIter startSelIter, GtkTextIter endSelIter)
    {
    int startLine = getLineNumber(startSelIter);
    int endLine = getLineNumber(endSelIter);
    for(int line=startLine; line<endLine; line++)
	{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(mTextBuffer, &iter, line);
	getFirstNonSpaceOnLineIter(iter, &iter);
	gtk_text_buffer_select_range(mTextBuffer, &iter, &iter);
	if(in)
	    tabPressed();
	else
	    backspacePressed();
	}
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &startSelIter, startLine);
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &endSelIter, endLine);
    gtk_text_buffer_select_range(mTextBuffer, &startSelIter, &endSelIter);
    }

int Indenter::countSpaces(char const * const text, bool expandTabs)
    {
    char const * p = text;
    int count = 0;
    while(isspace(*p))
	{
	if(*p == '\t' && expandTabs)
	    count += 8-(count % 8);
	else
	    count++;
	p++;
	}
    return count;
    }

void Indenter::getCursorIter(GtkTextIter *iter)
    {
    GtkTextMark *cursor = gtk_text_buffer_get_mark(mTextBuffer, "insert");
    gtk_text_buffer_get_iter_at_mark(mTextBuffer, iter, cursor);
    }

int Indenter::getLineNumber(GtkTextIter iter)
    {
    return gtk_text_iter_get_line(&iter);
    }

void Indenter::addSpaces(int count)
    {
    char text[count];
    memset(text, ' ', count);
    gtk_text_buffer_insert_at_cursor(mTextBuffer, text, count);
    }
