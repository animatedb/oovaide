/*
 * ControlWindow.h
 *
 *  Created on: Dec 18, 2015
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CONTROLWINDOW_H_
#define CONTROLWINDOW_H_

#include "Builder.h"
#include "Gui.h"

class ControlWindow
    {
    public:
        enum ControlTabs { CT_Find, CT_Control, CT_Data, CT_Stack };
        static GtkWidget *getTabView(ControlTabs ct)
            {
            char const *str;
            switch(ct)
                {
                case CT_Find:       str = "FindTextview";     break;
                case CT_Control:    str = "ControlTextview";  break;
                case CT_Data:       str = "DataTreeview";     break;
                case CT_Stack:      str = "StackTextview";    break;
                }
            return(Builder::getBuilder()->getWidget(str));
            }
        static void showNotebookTab(ControlTabs ct)
            {
            char const *str;
            switch(ct)
                {
                case CT_Find:      str = "Find";      break;
                case CT_Control:   str = "Control";   break;
                case CT_Data:      str = "Data";      break;
                case CT_Stack:     str = "Stack";     break;
                }
            GtkNotebook *book = GTK_NOTEBOOK(Builder::getBuilder()->getWidget("InteractNotebook"));
            Gui::setCurrentPage(book, Gui::findTab(book, str));
            }
    };

#endif /* CONTROLWINDOW_H_ */
