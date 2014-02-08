/*
 * ClassList.h
 *
 *  Created on: Jun 19, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSLIST_H_
#define CLASSLIST_H_

#include "Gui.h"


class ClassList:public GuiList
    {
    public:
	void init(Builder &builder)
	    {
	    GuiList::init(builder, "ClassTreeview", "Class List");
	    }
    };



#endif /* CLASSLIST_H_ */
