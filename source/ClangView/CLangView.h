/*
 * CLangView.h
 * Created on: August 19, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "clang-c/Index.h"
#include "OovString.h"
#include "Gui.h"
#include <vector>

#define VIEWER_VERSION "1.0"

enum FuncTypes {
    FT_ChildLabel,

// Cross ref
    FT_getCursorSpelling,
    FT_getCursorDisplayName,
    FT_getCursorDefinition,
    FT_getCursorUSR,

// Type info
    FT_getCursorKindSpelling,
    FT_getTypeSpelling,
//    FT_getTypeKindSpelling,
    FT_getCanonicalType,
    FT_getTypedefDeclUnderlyingType,
    FT_getPointeeType,
//  clang_getCursorType(cursor);        // Same as type spelling
//  clang_getTypeDeclaration

// Cursor manip
    FT_getCursorReferenced,
    FT_getCursorSemanticParent,
    FT_getCursorLexicalParent,
//    FT_isDeclaration, clang_isExpression, ...

// AST introspec
// clang_getSpecializedCursorTemplate
    };

class FunctionRequests:public std::vector<FuncTypes>
    {
    };

class AstFuncResult
    {
    public:
        void getFuncResult(CXCursor cursor, FuncTypes ft);
        OovString const &getStr() const
            { return mResultString; }
        /// Use clang_Cursor_isNull to check if there is a child cursor.
        CXCursor getChildCursor() const
            { return mChildCursor; }

    private:
        OovString mResultString;
        CXCursor mChildCursor;
        /// Returns a string and possibly a new child cursor.
        /// @param childCursor Possibly new child, otherwise same as parent.
        OovString getFuncValues(CXCursor cursor, FuncTypes funcType,
            CXCursor &childCursor);
        static char const *getFuncName(FuncTypes funcType);
    };

/// Each item in the tree view is directly correlated to one
/// of these ListItems.
class ListItem
    {
    public:
        ListItem()
            {}
        void setCursor(CXCursor cursor)
            { mCursor = cursor; }
        CXCursor getCursor() const
            { return mCursor; }

        void setGuiItem(GuiTreeItem item)
            { mGuiTreeItem = item; }
        GuiTreeItem getGuiItem() const
            { return mGuiTreeItem; }

        /// Cursor must be set first.
        void getFuncResult(FuncTypes ft)
            { mFuncResult.getFuncResult(mCursor, ft); }

        /// Must call getFuncResult first.
        OovString const &getResultStr() const
            { return mFuncResult.getStr(); }
        CXCursor getResultChildCursor() const
            { return mFuncResult.getChildCursor(); }

        void addChildItem(ListItem const &item)
            { mChildren.push_back(item); }
        std::vector<ListItem> &getChildren()
            { return mChildren; }
        std::vector<ListItem> const &getChildren() const
            { return mChildren; }
        /// @param path Same as GtkTreePath. Colon separated list of numbers.
        static ListItem *findChildItem(ListItem *rootItem, OovString path);

//        OovString getDebugStr() const;

    private:
        GuiTreeItem mGuiTreeItem;
        CXCursor mCursor;
        AstFuncResult mFuncResult;
        std::vector<ListItem> mChildren;
    };

class VisitorCommand
    {
    public:
        FunctionRequests mFuncRequests;
        ListItem *mParentListItem;

        void set(ListItem *parentItem, FunctionRequests const &fr)
            {
            mParentListItem = parentItem;
            mFuncRequests = fr;
            }
    };

class CLangViewer
    {
    public:
        CLangViewer();
        void parse(OovStringRef fileName, size_t num_clang_args,
            char const * const clang_args[]);
        void addInitialRootItems();
        /// Some items have a single first child that is a label
        /// Others have the remaining children.
        bool okToAddChildren(ListItem const &item, int funcCount) const
            {
            return(item.getChildren().size() == 0 ||
                item.getChildren().size() == funcCount-1);
            }
        /// Only adds children if ok to add.
        void addChildren(ListItem *item);
        void addChildrenOfPath(OovStringRef path);
        void addChildrenOfSelectedItem();
        void showWindow();
        void find();
        static CXChildVisitResult visitCursor(CXCursor cursor,
            CXCursor parent, CXClientData client_data);

    private:
        CXTranslationUnit mTransUnit;
        ListItem mListTree;
        GuiTree mTreeView;
        VisitorCommand mVisitorCommand;

        ListItem *addChild(ListItem &parent, CXCursor cursor, FuncTypes ft);
        CXChildVisitResult visitCursor(CXCursor cursor, CXCursor /*parent*/);
    };
