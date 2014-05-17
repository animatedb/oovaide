/*
 * Gui.cpp
 *
 *  Created on: Jul 10, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Gui.h"


PathChooser::~PathChooser()
    {
    if(mDialog)
	{
	gtk_widget_destroy(mDialog);
	}
    }

bool PathChooser::ChoosePath(GtkWindow *parent, char const * const dlgName,
	GtkFileChooserAction action, std::string &path)
    {
    if(mDialog)
	{
	gtk_widget_destroy(mDialog);
//	delete mDialog;
	}
    mDialog = gtk_file_chooser_dialog_new(dlgName, parent,
	action, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    if(mDefaultPath.length())
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(mDialog), mDefaultPath.c_str());
    bool success = (gtk_dialog_run(GTK_DIALOG (mDialog)) == GTK_RESPONSE_ACCEPT);
    if (success)
	{
	char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(mDialog));
	path = filename;
	g_free(filename);
	}
    return success;
    }

int Dialog::runHideCancel()
    {
    beforeRun();
    int ret = gtk_dialog_run(mDialog);
    afterRun(ret == GTK_RESPONSE_OK);
    if(ret == GTK_RESPONSE_CANCEL)
	gtk_widget_hide(GTK_WIDGET(getDialog()));
    return ret;
    }

bool Dialog::run(bool hideDialogAfterButtonPress)
    {
    beforeRun();
    bool ok = (gtk_dialog_run(mDialog) == GTK_RESPONSE_OK);
    afterRun(ok);
    if(hideDialogAfterButtonPress)
	gtk_widget_hide(GTK_WIDGET(getDialog()));
    return ok;
    }


void Gui::clear(GtkTextView *textview)
    {
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    gtk_text_buffer_set_text(textbuf, "", 0);
    }

void Gui::appendText(GtkTextView *textview, const std::string &text)
    {
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    gtk_text_buffer_insert_at_cursor(textbuf, text.c_str(), text.length());
    }

void Gui::scrollToCursor(GtkTextView *textview)
    {
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    GtkTextMark *mark = gtk_text_buffer_get_insert(textbuf);
/*
    if(mark)
	{
	// Move to beginning of line.
	GtkTextIter startFind;
	gtk_text_buffer_get_iter_at_mark(textbuf, &startFind, mark);
	gtk_text_iter_backward_line(&startFind);
	gtk_text_buffer_move_mark(textbuf, mark, &startFind);
	}
*/
    if(mark)
	{
	gtk_text_view_scroll_mark_onscreen(textview, mark);
	}
    }

GuiText Gui::getText(GtkTextView *textview)
    {
    GtkTextIter start;
    GtkTextIter end;
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    gtk_text_buffer_get_start_iter(textbuf, &start);
    gtk_text_buffer_get_end_iter(textbuf, &end);
    return GuiText(gtk_text_buffer_get_text(textbuf, &start, &end, FALSE));
    }

char const * const Gui::getSelectedText(GtkTextView *textview)
    {
    GtkTextIter start;
    GtkTextIter end;
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    gtk_text_buffer_get_selection_bounds(textbuf, &start, &end);
    return gtk_text_buffer_get_text(textbuf, &start, &end, FALSE);
    }

int Gui::getCurrentLineNumber(GtkTextView *textView)
    {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(textView);
    GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buf, &iter, mark);
    return(gtk_text_iter_get_line(&iter) + 1);
    }

GuiText Gui::getCurrentLineText(GtkTextView *textView)
    {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(textView);
    GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buf, &iter, mark);

    GtkTextIter startIter = iter;
    GtkTextIter endIter = iter;
    gtk_text_iter_set_line_offset(&startIter, 0);
    gtk_text_iter_forward_to_line_end(&endIter);
    return gtk_text_buffer_get_text(buf, &startIter, &endIter, false);

    // Alternate method
    /*
    GtkTextBuffer *textBuf = gtk_text_view_get_buffer(textView);

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_selection_bounds(textBuf, &start, &end);
    int lineNum = gtk_text_iter_get_line(&start);
    gtk_text_buffer_get_iter_at_line(textBuf, &start, lineNum);
    gtk_text_buffer_get_iter_at_line(textBuf, &end, lineNum+1);
    return gtk_text_buffer_get_text(textBuf, &start, &end, false);
*/
    }

void Gui::reparentWidget(GtkWidget *windowToMove, GtkContainer *newParent)
    {
    g_object_ref(GTK_WIDGET(windowToMove));
    GtkWidget *old_parent = gtk_widget_get_parent(GTK_WIDGET(windowToMove));
    if(old_parent)
    	gtk_container_remove(GTK_CONTAINER(old_parent), GTK_WIDGET(windowToMove));
    gtk_container_add(GTK_CONTAINER(newParent), GTK_WIDGET(windowToMove));
    g_object_unref(GTK_WIDGET(windowToMove));
    }

bool Gui::messageBox(char const * const msg, GtkMessageType msgType,
	GtkButtonsType buttons)
    {
    GtkWidget *widget = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL,
	msgType, buttons, "%s", msg);
    int resp = gtk_dialog_run(GTK_DIALOG(widget));
    gtk_widget_destroy(widget);
    return(resp == GTK_RESPONSE_OK || resp == GTK_RESPONSE_YES);
    }


/////////////


void GuiList::init(Builder &builder, char const * const widgetName,
	char const * const title)
    {
    mListWidget = GTK_TREE_VIEW(builder.getWidget(widgetName));
    if(!gtk_tree_view_get_column(mListWidget, 0))
	{
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
	    title, renderer, "text", LIST_ITEM, nullptr);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mListWidget), column);

	GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model(mListWidget, GTK_TREE_MODEL(store));
	g_object_unref(store);
	}
    }

void GuiList::appendText(char const * const str)
    {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(mListWidget));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
    }

void GuiList::clear()
    {
    removeSelected();
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(mListWidget));
    GtkTreeIter iter;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gtk_tree_view_get_model(mListWidget)), &iter))
	{
	while(gtk_list_store_remove(store, &iter))
	    {
	    }
	}
    }

void GuiList::sort()
    {
    GtkTreeSortable *sort = GTK_TREE_SORTABLE(gtk_tree_view_get_model(mListWidget));
    gtk_tree_sortable_set_sort_column_id(sort, 0, GTK_SORT_ASCENDING);
    }

std::vector<std::string> GuiList::getText() const
    {
    std::vector<std::string> text;
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(mListWidget));
    if(gtk_tree_model_get_iter_first(model, &iter))
	{
	do
	    {
	    GValue val = { 0 };
	    gtk_tree_model_get_value(model, &iter, 0, &val);
	    char const *v = g_value_get_string(&val);
	    if(v)
		text.push_back(v);
	    g_value_unset(&val);
	    } while(gtk_tree_model_iter_next(model, &iter));
	}
    return text;
    }

std::string GuiList::getSelected() const
    {
    GtkTreeIter iter;
    GtkTreeModel *model;
    std::string selectedStr;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mListWidget);
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
        {
        char *value;
        gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
        selectedStr = value;
        g_free(value);
        }
    return selectedStr;
    }

void GuiList::removeSelected()
    {
    GtkTreeIter iter;
    GtkTreeModel *model;
    std::string selectedStr;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mListWidget);
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
        {
	if(GTK_IS_LIST_STORE(model))
	    {
	    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	    }
	else
	    {
	    gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	    }
        }
    }

int GuiList::getSelectedIndex() const
    {
    GtkTreeModel *model;
    int index = -1;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mListWidget);
    GList *listOfPaths = gtk_tree_selection_get_selected_rows(sel, &model);
    if (listOfPaths)
        {
	gint *p = gtk_tree_path_get_indices((GtkTreePath*)g_list_nth_data(listOfPaths, 0));
	if(p)
	    index = *p;
        g_list_free(listOfPaths);
        }
    return index;
    }

void GuiList::setSelected(char const * const str)
    {
    if(getSelected().compare(str) != 0)
	{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(mListWidget);
	if(gtk_tree_model_get_iter_first(model, &iter))
	    {
	    do
		{
		gchar *value;
		gtk_tree_model_get(model, &iter, LIST_ITEM, &value, -1);
		if(std::string(str).compare(value) == 0)
		    {
		    GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		    if(path)
			{
			gtk_tree_view_set_cursor(mListWidget, path, NULL, false);
			gtk_tree_view_scroll_to_cell(mListWidget, path, NULL, false, 0, 0);
			gtk_tree_path_free(path);
			}
		    break;
		    }
		g_free(value);
		} while(gtk_tree_model_iter_next(model, &iter));
	    }
	}
    }

void GuiTree::init(Builder &builder, char const * const widgetName,
	char const * const title)
    {
    mTreeWidget = GTK_TREE_VIEW(builder.getWidget(widgetName));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", LIST_ITEM, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(mTreeWidget), column);

    GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
    gtk_tree_view_set_model(mTreeWidget, GTK_TREE_MODEL(store));
    g_object_unref(store);
    }

GuiTreeItem GuiTree::appendText(GuiTreeItem parentItem, char const * const str)
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeWidget);
    GuiTreeItem item(false);
    if(GTK_IS_LIST_STORE(model))
	{
	GtkListStore *store = GTK_LIST_STORE(model);
	gtk_list_store_append(store, &item.mIter);
	gtk_list_store_set(store, &item.mIter, LIST_ITEM, str, -1);
	}
    else
	{
	GtkTreeStore *store = GTK_TREE_STORE(model);
	if(parentItem.isRoot())
	    gtk_tree_store_append(store, &item.mIter, NULL);
	else
	    gtk_tree_store_append(store, &item.mIter, &parentItem.mIter);

	gtk_tree_store_set(store, &item.mIter, LIST_ITEM, str, -1);
	}
    return item;
    }

void GuiTree::clear()
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeWidget);
    if(GTK_IS_LIST_STORE(model))
	{
	GtkListStore *store = GTK_LIST_STORE(model);
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(
		gtk_tree_view_get_model(mTreeWidget)), &iter))
	    {
	    while(gtk_list_store_remove(store, &iter))
		{
		}
	    }
	}
    else
	{
	GtkTreeStore *store = GTK_TREE_STORE(model);
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(
		gtk_tree_view_get_model(mTreeWidget)), &iter))
	    {
	    while(gtk_tree_store_remove(store, &iter))
		{
		}
	    }
	}
    }

void GuiTree::getSelected(std::vector<std::string> &names) const
    {
    GtkTreeIter parent;
    GtkTreeIter child;
    GtkTreeModel *model;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeWidget);
    if (gtk_tree_selection_get_selected(sel, &model, &child))
        {
        if(gtk_tree_model_iter_parent(model, &parent, &child))
            {
    	    char *value;
	    gtk_tree_model_get(model, &parent, LIST_ITEM, &value,  -1);
	    names.push_back(value);
	    g_free(value);

	    gtk_tree_model_get(model, &child, LIST_ITEM, &value,  -1);
	    names.push_back(value);
	    g_free(value);
            }
        }
    }
