/*
 * OperationList.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONLIST_H_
#define OPERATIONLIST_H_

#include "Gui.h"

class OperationList:public GuiList
    {
    public:
	void init(Builder &builder)
	    {
	    GuiList::init(builder, "OperationsTreeview", "Operation List");
	    }
    };

#endif /* OPERATIONLIST_H_ */
