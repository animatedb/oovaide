/*
 * Gui.cpp
 *
 *  Created on: Jul 10, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Gui.h"
#include <string.h>
#include <algorithm>


bool PathChooser::ChoosePath(GtkWindow *parent, OovStringRef const dlgName,
        GtkFileChooserAction action, OovString &path)
    {
    char const * openButtonFlag =
            (action == GTK_FILE_CHOOSER_ACTION_OPEN ||
                    action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) ?
                            "_Open": "_Save";
    GtkWidget *dialog = gtk_file_chooser_dialog_new(dlgName, parent,
        action, GUI_CANCEL, GTK_RESPONSE_CANCEL,
        openButtonFlag, GTK_RESPONSE_ACCEPT, nullptr);
    bool success = false;
    if(dialog)
        {
        if(mDefaultPath.length())
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), mDefaultPath.c_str());
        success = (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT);
        if (success)
            {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            path = filename;
            g_free(filename);
            }
        gtk_widget_destroy(dialog);
        }
    return success;
    }

Dialog::~Dialog()
    {}

void Dialog::setDialog(GtkDialog *dlg, GtkWindow *parent)
    {
    mDialog = dlg;
    if(!parent)
        {
        parent = Gui::getMainWindow();
        }
    if(parent)
        {
        gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
        }
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


GtkWindow *Gui::getMainWindow()
    {
    return GTK_WINDOW(Builder::getBuilder()->getWidget("MainWindow"));
    }


void Gui::clear(GtkTextView *textview)
    {
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    gtk_text_buffer_set_text(textbuf, "", 0);
    }

void Gui::appendText(GtkTextView *textview, OovStringRef const text)
    {
    GtkTextBuffer *textbuf = gtk_text_view_get_buffer(textview);
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(textbuf, &iter);
    gtk_text_buffer_insert(textbuf, &iter, text, text.numBytes());
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

OovStringRef const Gui::getSelectedText(GtkTextView *textview)
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

bool Gui::messageBox(OovStringRef const msg, GtkMessageType msgType,
        GtkButtonsType buttons)
    {
    GtkWindow *main = getMainWindow();
    GtkWidget *widget = gtk_message_dialog_new(main, GTK_DIALOG_MODAL,
        msgType, buttons, "%s", msg.getStr());
    int resp = gtk_dialog_run(GTK_DIALOG(widget));
    gtk_widget_destroy(widget);
    return(resp == GTK_RESPONSE_OK || resp == GTK_RESPONSE_YES);
    }


/////////////


GtkTextIter GuiTextBuffer::getCursorIter(GtkTextBuffer *buf)
    {
    GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
    GtkTextIter curLoc;
    gtk_text_buffer_get_iter_at_mark(buf, &curLoc, mark);
    return curLoc;
    }

OovString GuiTextBuffer::getText(GtkTextBuffer *buf, int startOffset, int endOffset)
    {
    GtkTextIter startIter;
    GtkTextIter endIter;
    gtk_text_buffer_get_iter_at_offset(buf, &startIter, startOffset);
    gtk_text_buffer_get_iter_at_offset(buf, &endIter, endOffset);
    GuiText str(gtk_text_buffer_get_text(buf, &startIter, &endIter, false));
    return str;
    }

char GuiTextBuffer::getChar(GtkTextBuffer *buf, int offset)
    {
    OovString str = getText(buf, offset, offset+1);
    char c = '\0';
    if(str.length())
        c = str[0];
    return c;
    }

void GuiTextBuffer::erase(GtkTextBuffer *buf, int startOffset, int endOffset)
    {
    GtkTextIter startIter;
    GtkTextIter endIter;
    gtk_text_buffer_get_iter_at_offset(buf, &startIter, startOffset);
    gtk_text_buffer_get_iter_at_offset(buf, &endIter, endOffset);
    gtk_text_buffer_delete(buf, &startIter, &endIter);
    }

bool GuiTextBuffer::isCursorAtEnd(GtkTextBuffer *buf)
    {
    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(buf, &endIter);
    return(getCursorOffset(buf) == getIterOffset(endIter));
    }

void GuiTextBuffer::moveCursorToEnd(GtkTextBuffer *buf)
    {
    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(buf, &endIter);
    gtk_text_buffer_place_cursor(buf, &endIter);
    }


/////////////

void GuiTreeView::clear()
    {
    removeSelected();
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    if(GTK_IS_LIST_STORE(model))
        {
        GtkListStore *store = GTK_LIST_STORE(model);
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(
                gtk_tree_view_get_model(mTreeView)), &iter))
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
        GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
        if(model)
            {
            if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(
                    model), &iter))
                {
                while(gtk_tree_store_remove(store, &iter))
                    {
                    }
                }
            }
        }
    }

void GuiTreeView::removeSelected()
    {
    GtkTreeIter iter;
    GtkTreeModel *model;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
    if(sel)
        {
        if(gtk_tree_selection_get_selected(sel, &model, &iter))
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
    }

void GuiTreeView::scrollToPath(GuiTreePath &path)
    {
    gtk_tree_view_scroll_to_cell(mTreeView, path.getPath(), nullptr, false, 0, 0);
    }

int GuiTreeView::getNumChildren(GuiTreeItem const &item) const
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    return gtk_tree_model_iter_n_children(model, nullptr);
    }



void GuiList::init(Builder &builder, OovStringRef const widgetName,
        OovStringRef const title)
    {
    mTreeView = GTK_TREE_VIEW(builder.getWidget(widgetName));
    if(!gtk_tree_view_get_column(mTreeView, 0))
        {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
            title, renderer, "text", LIST_ITEM, nullptr);
        gtk_tree_view_append_column(GTK_TREE_VIEW(mTreeView), column);

        GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
        gtk_tree_view_set_model(mTreeView, GTK_TREE_MODEL(store));
        g_object_unref(store);
        }
    }

void GuiList::appendText(OovStringRef const str)
    {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(mTreeView));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, LIST_ITEM, str.getStr(), -1);
    }

void GuiList::sort()
    {
    GtkTreeSortable *sort = GTK_TREE_SORTABLE(gtk_tree_view_get_model(mTreeView));
    gtk_tree_sortable_set_sort_column_id(sort, 0, GTK_SORT_ASCENDING);
    }

OovStringVec GuiList::getText() const
    {
    OovStringVec text;
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(mTreeView));
    if(gtk_tree_model_get_iter_first(model, &iter))
        {
        do
            {
            GuiListStringValue val;
            char const *v = val.getStringFromModel(model, &iter);
            if(v)
                text.push_back(v);
            } while(gtk_tree_model_iter_next(model, &iter));
        }
    return text;
    }

OovString GuiList::getSelected() const
    {
    GtkTreeIter iter;
    GtkTreeModel *model;
    std::string selectedStr;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
    if(sel)
        {
        if (gtk_tree_selection_get_selected(sel, &model, &iter))
            {
            char *value;
            gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
            selectedStr = value;
            g_free(value);
            }
        }
    return selectedStr;
    }

int GuiList::getSelectedIndex() const
    {
    GtkTreeModel *model;
    int index = -1;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
    if(sel)
        {
        GList *listOfPaths = gtk_tree_selection_get_selected_rows(sel, &model);
        if (listOfPaths)
            {
            gint *p = gtk_tree_path_get_indices((GtkTreePath*)g_list_nth_data(listOfPaths, 0));
            if(p)
                index = *p;
            g_list_free(listOfPaths);
            }
        }
    return index;
    }

void GuiList::setSelected(OovStringRef const str)
    {
    if(getSelected().compare(str) != 0)
        {
        GtkTreeIter iter;
        GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
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
                        gtk_tree_view_scroll_to_cell(mTreeView, path, NULL, false, 0, 0);
                        gtk_tree_view_set_cursor(mTreeView, path, NULL, false);
                        gtk_tree_path_free(path);
                        }
                    break;
                    }
                g_free(value);
                } while(gtk_tree_model_iter_next(model, &iter));
            }
        }
    }

OovString GuiList::findLongestMatch(OovStringRef const str)
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    GtkTreeIter iter;
    OovString bestStr;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
        {
        int bestMatchCount = 0;
        do
            {
            GuiListStringValue val;
            char const *v = val.getStringFromModel(model, &iter);
            if(v)
                {
                int count = StringCompareNoCaseNumCharsMatch(str, v);
                if(count > bestMatchCount)
                    {
                    bestStr = v;
                    bestMatchCount = count;
                    }
                }
            } while(gtk_tree_model_iter_next(model, &iter));
        }
    return bestStr;
    }


void GuiTree::init(Builder &builder, OovStringRef const widgetName,
        OovStringRef const titleCol1, ColumnTypes columnType,
        OovStringRef const titleCol2)
    {
    mTreeView = GTK_TREE_VIEW(builder.getWidget(widgetName));
    if(!gtk_tree_view_get_column(mTreeView, 0))
        {
        GtkTreeStore *store = nullptr;
        switch(columnType)
            {
            case CT_String:
                addStringColumn(DCI_StringColumnIndex, titleCol1);
                store = gtk_tree_store_new(1, G_TYPE_STRING);
                break;

            case CT_StringBool:
                addStringColumn(DCI_StringColumnIndex, titleCol1);
                addBoolColumn(DCI_BoolColumnIndex, titleCol2);
                store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
                break;

            case CT_BoolString:
                addBoolColumn(DCI_BoolColumnIndex, titleCol1);
                addStringColumn(DCI_StringColumnIndex, titleCol2);
				store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
                break;
            }
        if(store)
            {
            gtk_tree_view_set_model(mTreeView, GTK_TREE_MODEL(store));
            }
        g_object_unref(store);
        }
    }

void GuiTree::addStringColumn(int dataColumnIndex, OovStringRef title)
    {
    // Add a string column as the first column
    GtkCellRenderer *textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *textColumn = gtk_tree_view_column_new_with_attributes(
        title, textRenderer, "text", dataColumnIndex, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(mTreeView), textColumn);
    }

void GuiTree::addBoolColumn(int dataColumnIndex, OovStringRef title)
    {
    // Add a boolean checkbox column
    GtkCellRenderer *toggleRenderer = gtk_cell_renderer_toggle_new();
    GtkTreeViewColumn *toggleColumn = gtk_tree_view_column_new_with_attributes
            (title, toggleRenderer, "active", dataColumnIndex, NULL);
    gtk_tree_view_append_column(mTreeView, toggleColumn);
    }


GuiTreeItem GuiTree::appendRow(GuiTreeItem parentItem)
    {
    GuiTreeItem item(false);
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    if(GTK_IS_LIST_STORE(model))
        {
        GtkListStore *store = GTK_LIST_STORE(model);
        gtk_list_store_append(store, item.getPtr());
        }
    else
        {
        GtkTreeStore *store = GTK_TREE_STORE(model);
        gtk_tree_store_append(store, item.getPtr(), parentItem.getPtr());
        }
    return item;
    }

GuiTreeItem GuiTree::appendText(GuiTreeItem parentItem, OovStringRef const str)
    {
    GuiTreeItem item = appendRow(parentItem);
    setText(item, str);
    return item;
    }

void GuiTree::setText(GuiTreeItem item, OovStringRef const str)
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    if(GTK_IS_LIST_STORE(model))
        {
        GtkListStore *store = GTK_LIST_STORE(model);
        gtk_list_store_set(store, item.getPtr(), DCI_StringColumnIndex, str.getStr(), -1);
        }
    else
        {
        GtkTreeStore *store = GTK_TREE_STORE(model);
        gtk_tree_store_set(store, item.getPtr(), DCI_StringColumnIndex, str.getStr(), -1);
        }
    }

bool GuiTree::getSelectedItem(GuiTreeItem &childItem) const
    {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
    bool haveSel = false;
    if(sel)
        {
        GtkTreeModel *model;
        childItem.setRoot(false);
        haveSel = gtk_tree_selection_get_selected(sel, &model, childItem.getPtr());
        }
    return haveSel;
    }

void GuiTree::expandRow(GuiTreePath &path)
    {
    gtk_tree_view_expand_row(mTreeView, path.getPath(), false);
    }

#if(1)
OovString const GuiTree::getSelected(char delimiter) const
    {
    OovStringVec names = getSelected();
    OovString name;
    name.join(names, delimiter);
    return name;
    }

#else
OovString const GuiTree::getSelected(char delimiter) const
    {
    GtkTreeIter iter;
    OovString name;
    if(getSelectedIter(&iter))
        {
        name = getNodeName(iter, delimiter);
        }
    return name;
    }
#endif

OovStringVec const GuiTree::getSelected() const
    {
    GuiTreeItem item;
    OovStringVec names;
    if(getSelectedItem(item))
        {
        names = getNodeVec(item);
        }
    return names;
    }

OovString GuiTree::getText(GuiTreeItem item) const
    {
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    char *nodeValue;
    gtk_tree_model_get(model, item.getPtr(), DCI_StringColumnIndex, &nodeValue, -1);
    OovString str = nodeValue;
    g_free(nodeValue);
    return str;
    }

OovStringVec const GuiTree::getNodeVec(GuiTreeItem item) const
    {
    GuiTreeItem parent(false);
    OovStringVec names;
    names.push_back(getText(item));
    // Walk up the tree to the root.
    GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
    while(gtk_tree_model_iter_parent(model, parent.getPtr(), item.getPtr()))
        {
        names.push_back(getText(parent));
        item = parent;
        }
    std::reverse(names.begin(), names.end());
    return names;
    }

OovString const GuiTree::getNodeName(GuiTreeItem item, char delimiter) const
    {
    OovStringVec names = getNodeVec(item);
    OovString name;
    name.join(names, delimiter);
    return name;
    }

void GuiTree::setCheckbox(GuiTreeItem item, bool set)
    {
    gtk_tree_store_set(getTreeStore(), item.getPtr(), DCI_BoolColumnIndex, set, -1);
    }

bool GuiTree::getCheckbox(GuiTreeItem item)
    {
    GValue value = { 0, 0 };
    gtk_tree_model_get_value(getModel(), item.getPtr(), DCI_BoolColumnIndex, &value);
    bool val = (g_value_get_boolean(&value) == TRUE);
    g_value_unset(&value);
    return val;
    }

void GuiTree::setChildCheckboxes(GuiTreeItem item, bool set)
    {
    GuiTreeItem child(false);
    int numChilds = gtk_tree_model_iter_n_children(getModel(), item.getPtr());
    for(int i=0; i<numChilds; i++)
        {
        if(gtk_tree_model_iter_nth_child(getModel(), child.getPtr(), item.getPtr(), i))
            {
            setCheckbox(child, set);
            setChildCheckboxes(child, set);
            }
        }
    }

OovStringVec GuiTree::getSelectedChildNodeNames(char delimiter)
    {
    GuiTreeItem item;
    OovStringVec names;
    if(getSelectedItem(item))
        {
        names = getChildNodeNames(item, delimiter);
        }
    return names;
    }

OovStringVec const GuiTree::getChildNodeNames(GuiTreeItem item, char delimiter)
    {
    GuiTreeItem child(false);
    OovStringVec names;

    int numChilds = gtk_tree_model_iter_n_children(getModel(), item.getPtr());
    for(int i=0; i<numChilds; i++)
        {
        if(gtk_tree_model_iter_nth_child(getModel(), child.getPtr(), item.getPtr(), i))
            {
            names.push_back(getNodeName(child, delimiter));
            OovStringVec newNames = getChildNodeNames(child, delimiter);
            for(auto const &newName : newNames)
                {
                names.push_back(newName);
                }
            }
        }
    return names;
    }

void GuiTree::setAllCheckboxes(bool set)
    {
    setChildCheckboxes(GuiTreeItem(), set);
    }

void GuiTree::setSelectedChildCheckboxes(bool set)
    {
    GuiTreeItem item;
    if(getSelectedItem(item))
        {
        setChildCheckboxes(item, set);
        }
    }

bool GuiTree::toggleSelectedCheckbox()
    {
    GuiTreeItem item;
    bool val = false;
    if(getSelectedItem(item))
        {
        val = getCheckbox(item);
        setCheckbox(item, !val);
        }
    return !val;
    }

bool GuiTree::getSelectedCheckbox(bool &checked)
    {
    GuiTreeItem item;
    checked = false;
    bool selected = getSelectedItem(item);
    if(selected)
        {
        selected = getCheckbox(item);
        }
    return selected;
    }

void GuiTree::setSelected(OovStringVec const &names)
    {
    OovString name;
    name.join(names, ':');
    GuiTreeItem item = getItem(name, '/');
    GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
    gtk_tree_selection_select_iter(sel, item.getPtr());
    }

void GuiTree::setSelected(OovString const &name, char delimiter)
    {
    OovStringVec tokens = name.split(delimiter);
    setSelected(tokens);
    }

bool GuiTree::findNodeItem(GuiTreeItem parent,
        OovString const &name, char delimiter, GuiTreeItem &item)
    {
    OovStringVec tokens = name.split(delimiter);
    bool found = false;
    if(tokens.size() > 0)
        {
        GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
        item.setRoot(false);
        bool success = gtk_tree_model_iter_children(model, item.getPtr(),
                parent.getPtr());
        while(success && !found)
            {
            if(tokens[0].compare(getText(item)) == 0)
                {
                if(tokens.size() > 1)
                    {
                    OovString childName;
                    tokens.erase(tokens.begin());
                    childName.join(tokens, delimiter);
                    GuiTreeItem parent = item;
                    item.setRoot(false);
                    found = findNodeItem(parent, childName, delimiter, item);
                    }
                else
                    {
                    found = true;
                    }
                }
            if(!found)
                {
                success = gtk_tree_model_iter_next(model, item.getPtr());
                }
            }
        }
    return found;
    }

GuiTreeItem GuiTree::getItem(OovString const &name, char delimiter)
    {
    GuiTreeItem item;
    if(name.length() > 0)
        {
        findNodeItem(GuiTreeItem(), name, delimiter, item);
        }
    return item;
    }


TaskBusyDialog::TaskBusyDialog():
        mBuilder(nullptr), mParent(nullptr),
        mKeepGoing(true), mTotalIters(0), mStartTime(0)
    {
    }

TaskBusyDialog::~TaskBusyDialog()
    {
    showDialog(false);
    }

void TaskBusyDialog::startTask(char const *str, size_t totalIters)
    {
    mDialogText = str;
    mTotalIters = totalIters;
    time(&mStartTime);
    mKeepGoing = true;
    }

static void BackgroundResponse(GtkDialog * /*dialog*/, gint response_id,
        gpointer bkgDlg)
    {
    if(response_id == GTK_RESPONSE_CANCEL)
        {
        static_cast<TaskBusyDialog*>(bkgDlg)->cancelButtonPressed();
        }
    }

void TaskBusyDialog::showDialog(bool show)
    {
    if(!mBuilder)
        {
        mBuilder = Builder::getBuilder();
        }
    if(mBuilder)
        {
        GtkWidget *dlg = mBuilder->getWidget("BackgroundProgressDialog");
        if(show != gtk_widget_get_visible(dlg))
            {
            if(show)
                {
                setDialog(GTK_DIALOG(dlg), mParent);
                g_signal_connect(G_OBJECT(dlg), "response",
                      G_CALLBACK(BackgroundResponse), this);
                gtk_widget_show_all(dlg);
                }
            else
                {
                gtk_widget_hide(dlg);
                }
            }
        }
    }

bool TaskBusyDialog::updateProgressIteration(size_t currentIter,
        OovStringRef text, bool allowRecurse)
    {
    if(!mBuilder)
        {
        mBuilder = Builder::getBuilder();
        }
    if(mDialogText.length())
        {
        std::string totalStr = mDialogText;
        totalStr += "\nPress cancel to abort this operation.";
        Gui::setText(GTK_LABEL(mBuilder->getWidget("ProgressDialogCancelLabel")), totalStr.c_str());
        mDialogText.clear();
        }
    if(mKeepGoing)      // It appears that after a cancel, the widgets are not valid.
        {
        time_t curTime;
        time(&curTime);
        GtkWidget *dlg = mBuilder->getWidget("BackgroundProgressDialog");
        bool visible = gtk_widget_get_visible(dlg);
        if(!visible && mTotalIters > 0 && curTime > mStartTime+1)
            showDialog(true);
        if(visible)
            {
            if(text && curTime != mStartTime)
                {
                GtkLabel *textLabel = GTK_LABEL(mBuilder->getWidget("ProgressTextLabel"));
                Gui::setText(textLabel, text);
                mStartTime = curTime;
                }
            if(mBuilder)
                {
                GtkProgressBar *bar = GTK_PROGRESS_BAR(mBuilder->getWidget("BackgroundProgressbar"));
                gtk_progress_bar_set_fraction(bar, (double)currentIter/mTotalIters);
                }
            if(allowRecurse)
                {
                // Allow progress update to display.
                // For some reason, gtk_events_pending never quits.
                for(size_t i=0; i<50 && g_main_context_pending(NULL); i++)
                    {
                    gtk_main_iteration();
                    }
                }
            }
        }
    return mKeepGoing;
    }

int Gui::findTab(GtkNotebook *book, OovStringRef const tabName)
    {
    int tabIndex = -1;
    for(int i=0; i<gtk_notebook_get_n_pages(book); i++)
        {
        GtkWidget *child = gtk_notebook_get_nth_page(book, i);
        GtkLabel *label = GTK_LABEL(gtk_notebook_get_tab_label(book, child));
        if(strcmp(getText(label), tabName) == 0)
            {
            tabIndex = i;
            break;
            }
        }
    return tabIndex;
    }

GuiHighlightTag::GuiHighlightTag():
        mTag(nullptr)
    {
    }

void GuiHighlightTag::setForegroundColor(GtkTextBuffer *textBuffer,
        char const * const name, char const * const color)
    {
    if(!mTag)
        mTag = gtk_text_buffer_create_tag(textBuffer, name, NULL);
    g_object_set(G_OBJECT(mTag), "foreground", color, "foreground-set", TRUE, NULL);
    }
