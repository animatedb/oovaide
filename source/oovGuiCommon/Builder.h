/*
* Builder.h
*
*  Created on: Jun 19, 2013
*  \copyright 2013 DCBlaha.  Distributed under the GPL.
*/

#ifndef BUILDER_H_
#define BUILDER_H_

#include <gtk/gtk.h>

class Builder
    {
    public:
	~Builder()
	    {
	    g_object_unref(mGtkBuilder);
	    }
	void init();
	bool addFromFile(char const * const fn);
	void connectSignals()
	    {
	    gtk_builder_connect_signals(mGtkBuilder, nullptr);
	    }
	GtkWidget *getWidget(char const * const widgetName)
	    {
	    return GTK_WIDGET(gtk_builder_get_object(mGtkBuilder, widgetName));
	    }
	GtkMenu *getMenu(char const * const menuName)
	    {
	    return GTK_MENU(gtk_builder_get_object(mGtkBuilder, menuName));
	    }
	static Builder *getBuilder();
    private:
	GtkBuilder *mGtkBuilder;
    };



#endif /* BUILDER_H_ */
