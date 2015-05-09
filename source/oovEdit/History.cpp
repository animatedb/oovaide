/*
 * History.cpp
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "History.h"


void HistoryItem::setText(bool set, GtkTextBuffer *buffer)
    {
    GtkTextIter start;
    if(set)
	{
	HistoryItem::getIter(buffer, mOffset, &start);
	gtk_text_buffer_insert(buffer, &start, mText.c_str(),
            static_cast<gint>(mText.length()));
	}
    else
	{
	GtkTextIter end;
	HistoryItem::getIter(buffer, mOffset, &start);
	HistoryItem::getIter(buffer, mOffset+mText.length(), &end);
	gtk_text_buffer_delete(buffer, &start, &end);
	}
    }


