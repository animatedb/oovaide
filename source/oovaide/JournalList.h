/*
 * JournalList.h
 *
 *  Created on: Aug 22, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef JOURNALLIST_H_
#define JOURNALLIST_H_

#include "Gui.h"

class JournalList:public GuiList
    {
    public:
        void init(Builder &builder)
            {
            GuiList::init(builder, "JournalTreeview", "Journal");
            }
    };


#endif /* JOURNALLIST_H_ */
