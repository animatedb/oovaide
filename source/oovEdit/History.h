/*
 * History.h
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include "Gui.h"

/// This is for undo and redo.
class HistoryItem
    {
    public:
	HistoryItem(bool insert, int offset, char const * const text, int len):
	    mInsert(insert), mOffset(offset), mText(text, len)
	    {}
	void undo(GtkTextBuffer *buffer)
	    {
	    setText(!mInsert, buffer);
	    }
	void redo(GtkTextBuffer *buffer)
	    {
	    setText(mInsert, buffer);
	    }
	void setText(bool set, GtkTextBuffer *buffer);
	static gint getOffset(const GtkTextIter *iter)
	    { return gtk_text_iter_get_offset(iter); }
	static void getIter(GtkTextBuffer *buffer, int offset, GtkTextIter *iter)
	    { gtk_text_buffer_get_iter_at_offset(buffer, iter, offset); }

    protected:
	bool mInsert;
	gint mOffset;
	std::string mText;
    };



#endif /* HISTORY_H_ */
