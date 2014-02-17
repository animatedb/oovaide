/*
 * Gui.h
 *
 *  Created on: Jul 10, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef GUI_H_
#define GUI_H_

#include "Builder.h"
#include <string>
#include <vector>


class PathChooser
    {
    public:
	PathChooser():
	    mDialog(nullptr)
	    {}
	~PathChooser();
	void setDefaultPath(char const * const fn)
	    { mDefaultPath = fn; }
	bool ChoosePath(GtkWindow *parent, char const * const dlgName,
		GtkFileChooserAction action, std::string &path);
    private:
	GtkWidget *mDialog;
	std::string mDefaultPath;
    };

class Dialog
    {
    public:
	Dialog(GtkDialog *dlg):
	    mDialog(dlg)
	    {}
	virtual ~Dialog()
	    {}
	GtkWidget *addButton(const gchar *text, gint response_id)
	    { return gtk_dialog_add_button(mDialog, text, response_id); }
	// The response from this only works if the OK button in Glade is
	// set to -5, and cancel is set to -6.
	bool run(bool hide = false);
	virtual void beforeRun()
	    {}
	virtual void afterRun(bool ok)
	    {}
	GtkWidget *getActionArea()
	    { return gtk_dialog_get_content_area(mDialog); }
	void destroy()
	    { gtk_widget_destroy(GTK_WIDGET(mDialog)); }
	GtkDialog *getDialog()
	    { return mDialog; }
    private:
	GtkDialog *mDialog;
    };

class GuiText:public std::string
    {
    public:
	GuiText(void *text):
	    std::string(reinterpret_cast<char*>(text))
	    { g_free(text); }
    };

class Gui
    {
    public:
	static void clear(GtkTextView *textview);
	static void clear(GtkComboBoxText *box)
	    { gtk_combo_box_text_remove_all(box); }
	static void appendText(GtkTextView *textview, const std::string &text);
	static void appendText(GtkComboBoxText *box, const std::string &text)
	    { gtk_combo_box_text_append_text(box, text.c_str()); }
	static void scrollToCursor(GtkTextView *textview);
	static void setText(GtkEntry *textentry, char const * const text)
	    { gtk_entry_set_text(textentry, text); }
	static void setText(GtkTextView *view, char const * const text)
	    {
	    clear(view);
	    appendText(view, text);
	    }
	static GuiText getText(GtkTextView *textview);
	static char const * const getText(GtkEntry *entry)
	    { return gtk_entry_get_text(entry); }
	static char const * const getText(GtkComboBoxText *cb)
	    { return gtk_combo_box_text_get_active_text(cb); }
	static char const * const getCurrentLineText(GtkTextView *textView);
	static void setSelected(GtkComboBox *cb, int index)
	    { gtk_combo_box_set_active(cb, index); }

	static void setEnabled(GtkButton *w, bool enabled)
	    { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }
	static void setEnabled(GtkLabel *w, bool enabled)
	    { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }

	static void setVisible(GtkButton *w, bool show)
	    { setVisible(GTK_WIDGET(w), show); }
	static void setVisible(GtkLabel *w, bool show)
	    { setVisible(GTK_WIDGET(w), show); }
	static void setVisible(GtkWidget *w, bool show)
	    {
	    if(show)
		gtk_widget_show_all(GTK_WIDGET(w));
	    else
		gtk_widget_hide(GTK_WIDGET(w));
	    }

	static bool messageBox(char const * const msg,
		GtkMessageType msgType=GTK_MESSAGE_ERROR,
		GtkButtonsType buttons=GTK_BUTTONS_OK);
	static int appendPage(GtkNotebook *notebook, GtkWidget *child, GtkWidget *tabLabel)
	    { return gtk_notebook_append_page(notebook, child, tabLabel); }
	static int getCurrentPage(GtkNotebook *notebook)
	    { return gtk_notebook_get_current_page(notebook); }
	static void setCurrentPage(GtkNotebook *notebook, int page)
	    { gtk_notebook_set_current_page(notebook, page); }
	static int getNumPages(GtkNotebook *notebook)
	    { return gtk_notebook_get_n_pages(notebook); }
	static void reparentWidget(GtkWidget *windowToMove, GtkContainer *newParent);
    };

class GuiList
    {
    public:
	enum TreeViewItems
	 {
	   LIST_ITEM = 0,
	   N_COLUMNS
	 };
	GuiList():
	    mListWidget(nullptr)
	    {}
	void init(Builder &builder, char const * const widgetName,
		char const * const title);
	void appendText(char const * const str);
	void clear();
	void sort();
	std::vector<std::string> getText() const;
	std::string getSelected() const;
	int getSelectedIndex() const;
	void setSelected(char const * const str);
	void removeSelected();

    private:
	GtkTreeView *mListWidget;
    };

class GuiTreeItem
    {
    friend class GuiTree;
    public:
	GuiTreeItem(bool root = true):
	    mRoot(root)
	    {}
	bool isRoot() const
	    { return mRoot; }
    private:
	GtkTreeIter mIter;
	bool mRoot;
    };

/// This only supports a 2 level tree at this time.
class GuiTree
    {
    public:
	enum TreeViewItems
	 {
	   LIST_ITEM = 0,
	   N_COLUMNS
	 };

	void init(Builder &builder, char const * const widgetName,
		char const * const title);
	/// Returns the newly created child item.
	GuiTreeItem appendText(GuiTreeItem parentItem, char const * const str);
	void clear();
	/// first element is parent, second is child
	void getSelected(std::vector<std::string> &names) const;
	void clearSelection()
	    { gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(mTreeWidget)); }
//	int getSelectedIndex() const;
//	void setSelected(char const * const str);

    private:
	GtkTreeView *mTreeWidget;
    };

#endif /* GUI_H_ */
