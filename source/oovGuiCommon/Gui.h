/*
 * Gui.h
 *
 *  Created on: Jul 10, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef GUI_H_
#define GUI_H_

#include "Builder.h"
#include "OovString.h"
#include <vector>


#define GUI_OK "_OK"
#define GUI_CANCEL "_Cancel"

/// Displays a dialog that allows choosing a file path.
class PathChooser
    {
    public:
        void setDefaultPath(OovStringRef const fn)
            { mDefaultPath = fn; }
        bool ChoosePath(GtkWindow *parent, OovStringRef const dlgName,
                GtkFileChooserAction action, OovString &path);
    private:
        OovString mDefaultPath;
    };

/// This displays a dialog and handles running it.
class Dialog
    {
    public:
        Dialog(GtkDialog *dlg=nullptr, GtkWindow *parent=nullptr):
            mDialog(dlg)
            {
            setDialog(dlg, parent);
            }
        virtual ~Dialog();
        void setDialog(GtkDialog *dlg, GtkWindow *parent=nullptr);
        void setTitle(OovStringRef title)
            { gtk_window_set_title(GTK_WINDOW(mDialog), title); }
        GtkWidget *addButton(const gchar *text, gint response_id)
            { return gtk_dialog_add_button(mDialog, text, response_id); }
        int runHideCancel();
        // The response from this only works if the OK button Response ID in
        // Glade is set to -5 (GTK_RESPONSE_OK), and cancel is set to
        // -6 (GTK_RESPONSE_CANCEL).
        //
        // In glade, set the callback for the dialog's GtkWidget "delete-event" to
        // gtk_widget_hide_on_delete for the title bar close button to work.
        // This prevents the close button from destroying the dialog. In glade,
        // this is set on the GtkDialog top level window under the GtkWidget signals.
        bool run(bool hideDialogAfterButtonPress = false);
        virtual void beforeRun()
            {}
        // Return true to allow closing dialog, return false to keep dialog open.
        virtual bool afterRun(bool /*ok*/)
            { return true; }
        GtkWidget *getContentArea()
            { return gtk_dialog_get_content_area(mDialog); }
        void destroy()
            { gtk_widget_destroy(GTK_WIDGET(mDialog)); }
        GtkDialog *getDialog()
            { return mDialog; }

    private:
        GtkWindow *mParentWindow;
        GtkDialog *mDialog;
        void preRun();
    };

/// Many GTK functions return a string that must be freed.  This class
/// provides a constructor that moves the string into an std::string, then
/// frees the GTK string.
class GuiText:public std::string
    {
    public:
        GuiText(void *text):
            std::string(reinterpret_cast<char*>(text))
            { g_free(text); }
    };

namespace Gui
    {
        inline GtkWindow *getWindow(GtkWidget *widget)
            { return GTK_WINDOW(widget); }
        GtkWindow *getMainWindow();
        void clear(GtkTextView *textview);
        inline void clear(GtkEntry *textEntry)
            { gtk_entry_set_text(textEntry, ""); }
        inline void clear(GtkComboBoxText *box)
            { gtk_combo_box_text_remove_all(box); }
        void appendText(GtkTextView *textview, OovStringRef const text);
        inline void appendText(GtkComboBoxText *box, OovStringRef const text)
            { gtk_combo_box_text_append_text(box, text); }
        void scrollToCursor(GtkTextView *textview);
        inline void setText(GtkEntry *textentry, OovStringRef const text)
            { gtk_entry_set_text(textentry, text); }
        inline void setText(GtkLabel *label, OovStringRef const text)
            { gtk_label_set_text(label, text); }
        inline void setText(GtkTextView *view, OovStringRef const text)
            {
            clear(view);
            appendText(view, text);
            }
        inline void setValue(GtkSpinButton *spin, double val)
            { gtk_spin_button_set_value(spin, val); }
        inline double getValue(GtkSpinButton *spin)
            { return gtk_spin_button_get_value(spin); }
        inline void setRange(GtkSpinButton *spin, double min, double max)
            { gtk_spin_button_set_range(spin, min, max); }

        GuiText getText(GtkTextView *textview);
        inline OovStringRef const getText(GtkEntry *entry)
            { return gtk_entry_get_text(entry); }
        inline OovStringRef const getText(GtkLabel *label)
            { return gtk_label_get_text(label); }
        inline GuiText getText(GtkComboBoxText *cb)
            { return GuiText(gtk_combo_box_text_get_active_text(cb)); }
        OovStringRef const getSelectedText(GtkTextView *textview);
        int getCurrentLineNumber(GtkTextView *textView);
        GuiText getCurrentLineText(GtkTextView *textView);
        inline void setSelected(GtkComboBox *cb, int index)
            { gtk_combo_box_set_active(cb, index); }
        inline void setSelected(GtkComboBoxText *cb, int index)
            { gtk_combo_box_set_active(GTK_COMBO_BOX(cb), index); }

        inline void setEnabled(GtkButton *w, bool enabled)
            { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }
        inline void setEnabled(GtkEntry *w, bool enabled)
            { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }
        inline void setEnabled(GtkLabel *w, bool enabled)
            { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }
        inline void setEnabled(GtkMenuItem *w, bool enabled)
            { gtk_widget_set_sensitive(GTK_WIDGET(w), enabled); }

        inline void setCheckbox(GtkCheckButton *w, bool set)
            { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), set); }
        inline bool getCheckbox(GtkCheckButton *w)
            { return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)); }

        inline bool hasFocus(GtkWidget *w)
            { return(gtk_widget_is_focus(w)); }
        inline bool isVisible(GtkWidget *w)
            { return(gtk_widget_get_visible(w)); }
        inline void setVisible(GtkWidget *w, bool show)
            {
            if(show)
                gtk_widget_show_all(GTK_WIDGET(w));
            else
                gtk_widget_hide(GTK_WIDGET(w));
            }
        inline void setVisible(GtkWindow *w, bool show)
            { setVisible(GTK_WIDGET(w), show); }
        inline void setVisible(GtkButton *w, bool show)
            { setVisible(GTK_WIDGET(w), show); }
        inline void setVisible(GtkLabel *w, bool show)
            { setVisible(GTK_WIDGET(w), show); }

        bool messageBox(OovStringRef const msg,
                GtkMessageType msgType=GTK_MESSAGE_ERROR,
                GtkButtonsType buttons=GTK_BUTTONS_OK);
        inline int appendPage(GtkNotebook *notebook, GtkWidget *child,
            GtkWidget *tabLabel)
            { return gtk_notebook_append_page(notebook, child, tabLabel); }
        inline int getCurrentPage(GtkNotebook *notebook)
            { return gtk_notebook_get_current_page(notebook); }
        inline void setCurrentPage(GtkNotebook *notebook, int page)
            { gtk_notebook_set_current_page(notebook, page); }
        int findTab(GtkNotebook *notebook, OovStringRef const tabName);
        inline int getNumPages(GtkNotebook *notebook)
            { return gtk_notebook_get_n_pages(notebook); }
        inline GtkWidget *getNthPage(GtkNotebook *notebook, int pageNum)
            { return gtk_notebook_get_nth_page(notebook, pageNum); }
        void reparentWidget(GtkWidget *windowToMove, GtkContainer *newParent);
        inline void redraw(GtkWidget *widget)
            { gtk_widget_queue_draw(widget); }
    };

class GuiTextIter
    {
    public:
        static int getIterOffset(GtkTextIter iter)
            { return gtk_text_iter_get_offset(&iter); }
        static bool decIter(GtkTextIter *iter)
            { return gtk_text_iter_backward_char(iter); }
        static bool incLineIter(GtkTextIter *iter)
            { return gtk_text_iter_forward_line(iter);  }
// DEAD CODE
//        static bool incIter(GtkTextIter *iter)
//            { return gtk_text_iter_forward_char(iter); }
    };

class GuiTextBuffer:public GuiTextIter
    {
    public:
        static GtkTextBuffer *getBuffer(GtkTextView *view)
            { return gtk_text_view_get_buffer(view); }
        /// Iterators are invalid after many types of buffer modifications.
        static GtkTextIter getCursorIter(GtkTextBuffer *buf);
        static GtkTextIter getStartIter(GtkTextBuffer *buf)
            {
            GtkTextIter iter;
            gtk_text_buffer_get_start_iter(buf, &iter);
            return iter;
            }
        static GtkTextIter getEndIter(GtkTextBuffer *buf)
            {
            GtkTextIter iter;
            gtk_text_buffer_get_end_iter(buf, &iter);
            return iter;
            }
        static GtkTextIter getIterAtOffset(GtkTextBuffer *buf, int offset)
            {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_offset(buf, &iter, offset);
            return iter;
            }
        static GtkTextIter getLineIter(GtkTextBuffer *buf, int lineNum)
            {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_line(buf, &iter, lineNum);
            return iter;
            }
        static int getCursorOffset(GtkTextBuffer *buf)
            { return getIterOffset(getCursorIter(buf)); }
        static OovString getText(GtkTextBuffer *buf, int startOffset, int endOffset);
        static char getChar(GtkTextBuffer *buf, int offset);
        static void erase(GtkTextBuffer *buf, int startOffset, int endOffset);
        static bool isCursorAtEnd(GtkTextBuffer *buf);
        static void moveCursorToEnd(GtkTextBuffer *buf);
        static void moveCursorToIter(GtkTextBuffer *buf, GtkTextIter iter);
    };

class GuiTreePath
    {
    public:
        GuiTreePath(GtkTreeModel *model, GtkTreeIter *iter)
            {
            mOwnPath = true;
            mTreePath = gtk_tree_model_get_path(model, iter);
            }
        GuiTreePath(GtkTreePath *path)
            {
            mOwnPath = false;
            mTreePath = path;
            }
        GuiTreePath(OovStringRef path)
            {
            mOwnPath = true;
            mTreePath = gtk_tree_path_new_from_string(path);
            }
        ~GuiTreePath()
            {
            if(mOwnPath)
                {
                gtk_tree_path_free(mTreePath);
                }
            }
        GtkTreePath *getPath()
            { return mTreePath; }
        /// Returns a colon separated list of numbers.
        /// Use OovString::split(':') to get a vector.
        OovString getStr() const
            {
            char *str = gtk_tree_path_to_string(mTreePath);
            OovString oovstr = str;
            g_free(str);
            return oovstr;
            }

    private:
        bool mOwnPath;
        GtkTreePath *mTreePath;
    };

class GuiTreeItem
    {
    public:
        GuiTreeItem(bool root=true)
            { setRoot(root); }
        bool isRoot() const
            { return(mIter.stamp == 0); }
        void setRoot(bool root=true)
            {
            // If not root, set to non-zero so getPtr returns the actual pointer.
            if(root)
                mIter.stamp = 0;
            else
                mIter.stamp = 1;
            }
        // The root is the top level, but is invisible and has no text value.
        GtkTreeIter *getPtr()
            { return(isRoot() ? nullptr : &mIter); }

    private:
        GtkTreeIter mIter;
    };

/// This wraps a GtkTreeView.  A tree view is used for both trees and lists.
class GuiTreeView
    {
    public:
        GuiTreeView():
            mTreeView(nullptr)
            {}
        void access(GtkTreeView *treeView)
            { mTreeView = treeView; }

        bool getSelected(GtkTreeModel *&model, GtkTreeIter &iter);
        bool findImmediateChildPartialMatch(OovStringRef const str, GuiTreeItem parentItem,
            GuiTreeItem &item);
        OovString findNoCaseLongestMatch(OovStringRef const str);
        bool getNthChild(GuiTreeItem parentItem, int childIndex, GuiTreeItem &item);
        /// A default item will return the number of children at the root.
        int getNumChildren(GuiTreeItem &item) const;

        void scrollToPath(GuiTreePath &path);

        void removeSelected();
        void removeItem(GuiTreeItem item);
        void removeNthChild(GuiTreeItem parentItem, int childIndex);
        void clear();

        GtkTreeView *getTreeView()
            { return mTreeView; }

    protected:
        GtkTreeView *mTreeView;
    };

class GuiListStringValue
    {
    public:
        GuiListStringValue():
            mValue(G_VALUE_INIT)
            {
            }
        ~GuiListStringValue()
            {
            clear();
            }
        char const *getStringFromModel(GtkTreeModel *model, GtkTreeIter *iter)
            {
            clear();
            gtk_tree_model_get_value(model, iter, 0, &mValue);
            return g_value_get_string(&mValue);
            }

    private:
        GValue mValue;
        void clear()
            {
            if(G_VALUE_TYPE(&mValue) == G_TYPE_STRING)
                {
                g_value_unset(&mValue);
                }
            }
    };

/// This is a single column list.
class GuiList:public GuiTreeView
    {
    public:
        enum TreeViewItems
         {
           LIST_ITEM = 0,
           N_COLUMNS
         };
        void init(Builder &builder, OovStringRef const widgetName,
                OovStringRef const title);
        void appendText(OovStringRef const str);
        void sort();
        OovStringVec getText() const;
        OovString getSelected() const;
        int getSelectedIndex() const;
        void setSelected(OovStringRef const str);
    };

/// This only supports a few types of trees.  A tree where each item is a
/// string, or a tree where each item is a string and a boolean checkbox.
class GuiTree:public GuiTreeView
    {
    public:
        enum ColumnTypes { CT_String, CT_StringBool, CT_BoolString };
        // This is the order of the data store, not the visible columns.
        enum DataColumnIndices { DCI_StringColumnIndex, DCI_BoolColumnIndex };

        void init(Builder &builder, OovStringRef const widgetName,
            OovStringRef const titleCol1, ColumnTypes ct=CT_String,
            OovStringRef const totleCol2="");

        /// Returns the newly created child item.
        GuiTreeItem appendRow(GuiTreeItem parentItem);
        /// Appends a row and sets a string in the row
        GuiTreeItem appendText(GuiTreeItem parentItem, OovStringRef const str);
        /// First element is parent, second is child
        OovStringVec const getSelected() const;
        /// First element is parent, second is child,...
        /// The returned string is the elements joined together using the
        /// delimiter passed in.
        OovString const getSelected(char delimiter) const;
        void clearSelection()
            {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(mTreeView);
            if(sel)
                {
                gtk_tree_selection_unselect_all(sel);
                }
            }
        void setSelected(OovStringVec const &names);
        void setSelected(OovString const &name, char delimeter);
        void setText(GuiTreeItem item, OovStringRef const str);
        OovString getText(GuiTreeItem item) const;

        // This is for a CT_StringBool list.
        void setAllCheckboxes(bool set);
        void setSelectedChildCheckboxes(bool set);
        // This is for a CT_StringBool list.
        // The value returned is true if the checkbox is set after the toggle.
        bool toggleSelectedCheckbox();
        bool getSelectedCheckbox(bool &checked);
        void setCheckbox(GuiTreeItem item, bool set);
        bool getCheckbox(GuiTreeItem item);

        OovStringVec getSelectedChildNodeNames(char delimiter);
        OovStringVec getAllChildNodeNames(char delimiter)
            { return getChildNodeNames(GuiTreeItem(), delimiter); }

        GuiTreeItem getItem(OovString const &name, char delimiter);
        GtkTreeModel *getModel()
            { return gtk_tree_view_get_model(mTreeView); }
        GtkTreeModel const *getModel() const
            { return gtk_tree_view_get_model(mTreeView); }
        GtkTreeStore *getTreeStore()
            {
            GtkTreeModel *model = gtk_tree_view_get_model(mTreeView);
            return GTK_IS_TREE_STORE(model) ? GTK_TREE_STORE(model) : nullptr;
            }
        bool getSelectedItem(GuiTreeItem &item) const;
        void expandRow(GuiTreePath &path);

    private:
        bool findNodeItem(GuiTreeItem parent, OovString const &name,
                char delimiter, GuiTreeItem &item);
        void setChildCheckboxes(GuiTreeItem item, bool set);
        /// First element is parent, second is child,...
        /// The returned string is the elements joined together using the
        /// delimiter passed in.
        OovStringVec const getChildNodeNames(GuiTreeItem item, char delimeter);
        OovStringVec const getNodeVec(GuiTreeItem item) const;
        OovString const getNodeName(GuiTreeItem item, char delimiter) const;

        void addStringColumn(int column, OovStringRef title);
        void addBoolColumn(int column, OovStringRef title);
    };

/// This dialog is meant for tasks that execute in the background while the
/// user can do other things.  This dialog presents progress information and
/// allows the user to cancel background processing.
class TaskBusyDialog:public Dialog
    {
    public:
        TaskBusyDialog();
        ~TaskBusyDialog();
        void setParentWindow(GtkWindow *parent)
            { mParent = parent; }

        /// This does not call GUI functions, can be on any thread
        /// This resets the mKeepGoing flag and start time
        /// @param str          Some text to display about the task that is running.
        /// @param totalIters   The total number of iterations that will be run.
        void startTask(char const *str, size_t totalIters);

        // Return is false when a cancel is performed.
        // This is a GUI function, must be on the GUI thread
        /// @param text         Some text that will be updated and displayed to
        ///                     the user periodically.
        /// @param allowRecurse	Generally only set this true if a modal dialog
        ///                     is displayed, and this is called from the GUI thread.
        bool updateProgressIteration(size_t currentIter, OovStringRef text,
                bool allowRecurse = true);

        /// This is only needed if the destructor is not used to close the dialog.
        /// This is a GUI function, must be on the GUI thread
        void endTask()
            { showDialog(false); }

        /// Public since this is called from signal
        void cancelButtonPressed()
            { mKeepGoing = false; }

    private:
        std::string mDialogText;
        Builder *mBuilder;
        GtkWindow *mParent;
        bool mKeepGoing;
        size_t mTotalIters;
        time_t mStartTime;
        void showDialog(bool show);
    };

/// This updates once a second.
class TaskTimedBusyDialog
    {
    public:
        TaskTimedBusyDialog(GtkWindow *parentWindow, size_t count,
            char const *startText):
            mIndex(0), mTotalCount(count), mUpdateTime(0)
            {
            mProgressDlg.setParentWindow(parentWindow);
            mProgressDlg.startTask(startText, mTotalCount);
            }
        ~TaskTimedBusyDialog()
            {
            mProgressDlg.endTask();
            }
        bool keepGoing();
        size_t getIndex() const
            { return(mIndex-1); }

    private:
        TaskBusyDialog mProgressDlg;
        size_t mIndex;
        size_t mTotalCount;
        time_t mUpdateTime;
    };

class GuiHighlightTag
    {
    public:
        GuiHighlightTag();
        // Looks like the tag doesn't need to be deleted since it is owned
        // by the buffer. A destructor isn't needed.

        // A color can be "red", "black", etc.
        void setForegroundColor(GtkTextBuffer *textBuffer, char const * const name,
                char const * const color);
        bool isInitialized() const
            { return mTag != nullptr; }
        void applyTag(GtkTextBuffer *textBuffer, GtkTextIter *start,
                GtkTextIter *end)
            {
            gtk_text_buffer_apply_tag(textBuffer, mTag, start, end);
            }
        GtkTextTag *getTextTag() const
            { return mTag; }
    private:
        GtkTextTag *mTag;
    };

#endif /* GUI_H_ */
