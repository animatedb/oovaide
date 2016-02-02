/*
 * CLangView.cpp
 * Created on: August 19, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "CLangView.h"
#include "Builder.h"    // Small wrapper around GtkBuilder
#include "OovProcessArgs.h"
#include "FilePath.h"
#include "Project.h"
#include "BuildConfigReader.h"
#include "IncludeMap.h"
#include "Gui.h"
#ifndef __linux__
#include <windows.h>        // For SW_SHOWNORMAL
#endif
#include <string.h>


CLangViewer *gClangViewer;

class CXStringDisposer:public std::string
    {
    public:
        CXStringDisposer(const CXString &xstr):
            std::string(clang_getCString(xstr))
            {
            clang_disposeString(xstr);
            }
    };


void AstFuncResult::getFuncResult(CXCursor cursor, FuncTypes funcType)
    {
    OovString str;
    str += getFuncName(funcType);
    if(str.length() > 0)
        {
        str += ": ";
        }
    str += getFuncValues(cursor, funcType, mChildCursor);
    mResultString = str;
    }

OovString AstFuncResult::getFuncValues(CXCursor cursor, FuncTypes funcType,
    CXCursor &childCursor)
    {
    OovString str;
    childCursor = clang_getNullCursor();
    switch(funcType)
        {
        case FT_ChildLabel:
            str = CXStringDisposer(clang_getCursorKindSpelling(cursor.kind));
            str += ": ";
            str += CXStringDisposer(clang_getCursorSpelling(cursor));
            // Treat this special since we use it as the root of child elements.
            childCursor = cursor;
            break;

        case FT_getCursorSpelling:
            str = CXStringDisposer(clang_getCursorSpelling(cursor));
            break;

        case FT_getCursorDisplayName:
            str = CXStringDisposer(clang_getCursorDisplayName(cursor));
            break;

        case FT_getCursorDefinition:
            childCursor = clang_getCursorDefinition(cursor);
            str = CXStringDisposer(clang_getCursorSpelling(childCursor));
            break;

        case FT_getCursorUSR:
            str = CXStringDisposer(clang_getCursorUSR(cursor));
            break;

        case FT_getCursorKindSpelling:
            str = CXStringDisposer(clang_getCursorKindSpelling(cursor.kind));
            break;

        case FT_getTypeSpelling:
            {
            CXType type = clang_getCursorType(cursor);
            str = CXStringDisposer(clang_getTypeSpelling(type));
            childCursor = clang_getTypeDeclaration(type);
            }
            break;
/*
        case FT_getTypeKindSpelling:
            {
//            CXType type = clang_getCursorType(cursor);
            str = CXStringDisposer(clang_getTypeKindSpelling(typeKind));
            }
            break;
*/
        case FT_getCanonicalType:
            {
            CXType type = clang_getCursorType(cursor);
            type = clang_getCanonicalType(type);
            str = CXStringDisposer(clang_getTypeSpelling(type));
            childCursor = clang_getTypeDeclaration(type);
            }
            break;

        case FT_getTypedefDeclUnderlyingType:
            {
            CXType type = clang_getTypedefDeclUnderlyingType(cursor);
            str = CXStringDisposer(clang_getTypeSpelling(type));
            childCursor = clang_getTypeDeclaration(type);
            }
            break;

        case FT_getPointeeType:
            {
            CXType type = clang_getCursorType(cursor);
            type = clang_getPointeeType(type);
            str = CXStringDisposer(clang_getTypeSpelling(type));
            childCursor = clang_getTypeDeclaration(type);
            }
            break;

        case FT_getCursorReferenced:
            childCursor = clang_getCursorReferenced(cursor);
            str = CXStringDisposer(clang_getCursorSpelling(childCursor));
            break;

        case FT_getCursorSemanticParent:
            childCursor = clang_getCursorSemanticParent(cursor);
            str = CXStringDisposer(clang_getCursorSpelling(childCursor));
            break;

        case FT_getCursorLexicalParent:
            childCursor = clang_getCursorLexicalParent(cursor);
            str = CXStringDisposer(clang_getCursorSpelling(childCursor));
            break;
        }
    return str;
    }

char const *AstFuncResult::getFuncName(FuncTypes funcType)
    {
    char const *funcStr = "";
    switch(funcType)
        {
        case FT_ChildLabel:
            funcStr = "";
            break;

        case FT_getCursorSpelling:
            funcStr = "getCursorSpelling";
            break;

        case FT_getCursorDisplayName:
            funcStr = "getCursorDisplayName";
            break;

        case FT_getCursorDefinition:
            funcStr = "getCursorDefinition/getCursorSpelling";
            break;

        case FT_getCursorUSR:
            funcStr = "getCursorUSR";
            break;

        case FT_getCursorKindSpelling:
            funcStr = "getCursorKindSpelling";
            break;

        case FT_getTypeSpelling:
            funcStr = "getCursorType/getTypeSpelling";
            break;

        case FT_getCanonicalType:
            funcStr = "getCursorType/getCanonicalType/getCursorSpelling";
            break;

        case FT_getTypedefDeclUnderlyingType:
            funcStr = "getTypedefDeclUnderlyingType/getTypeSpelling";
            break;

        case FT_getPointeeType:
            funcStr = "getCursorType/getPointeeType/getTypeSpelling";
            break;

        case FT_getCursorReferenced:
            funcStr = "getCursorReferenced/getCursorSpelling";
            break;

        case FT_getCursorSemanticParent:
            funcStr = "getCursorSemanticParent/getCursorSpelling";
            break;

        case FT_getCursorLexicalParent:
            funcStr = "getCursorLexicalParent/getCursorSpelling";
            break;
        }
    return funcStr;
    }

/// Recursive
ListItem *ListItem::findChildItem(ListItem *rootItem, OovString path)
    {
    ListItem *listItem = nullptr;
    size_t colonPos = path.find(':');
    if(colonPos != std::string::npos)
        {
        OovString numStr(path, 0, colonPos);
        int index;
        if(numStr.getInt(0, rootItem->mChildren.size(), index))
            {
            ListItem *child = &rootItem->mChildren[index];
            listItem = findChildItem(child, path.substr(colonPos+1));
            }
        }
    else
        {
        int index;
        if(path.getInt(0, rootItem->mChildren.size(), index))
            {
            listItem = &rootItem->mChildren[index];
            }
        }
    return listItem;
    }

/*
OovString ListItem::getDebugStr() const
    {
    OovString str;
    str.appendInt((int)mCursor.data[0]);  // This is unique
    return str;
    }
*/


/////////////

CLangViewer::CLangViewer():
    mTransUnit(nullptr)
    {
    gClangViewer = this;
    mTreeView.init(*Builder::getBuilder(), "MainTreeview", "");
    }

CXChildVisitResult CLangViewer::visitCursor(
    CXCursor cursor, CXCursor parent, CXClientData client_data)
    {
    CLangViewer *viewer = static_cast<CLangViewer*>(client_data);
    return viewer->visitCursor(cursor, parent);
    }

void CLangViewer::showWindow()
    {
    GtkWidget *widget = Builder::getBuilder()->getWidget("MainWindow");
    gtk_widget_show(widget);
    gtk_main();
    }

void CLangViewer::parse(OovStringRef fileName, size_t num_clang_args,
    char const * const clang_args[])
    {
    CXIndex index = clang_createIndex(1, 1);
    if(index)
        {
        unsigned options = CXTranslationUnit_DetailedPreprocessingRecord;
        mTransUnit = clang_parseTranslationUnit(index, fileName,
            clang_args, static_cast<int>(num_clang_args), 0, 0, options);
        clang_disposeIndex(index);
        }
    }

void CLangViewer::addInitialRootItems()
    {
    CXCursor rootCursor = clang_getTranslationUnitCursor(mTransUnit);
    mListTree.setCursor(rootCursor);
    addChildren(&mListTree);
    }

void CLangViewer::addChildrenOfSelectedItem()
    {
    GuiTreeItem guiItem;
    mTreeView.getSelectedItem(guiItem);
    GuiTreePath guiPath(mTreeView.getModel(), guiItem.getPtr());
    addChildrenOfPath(guiPath.getStr());
    }

void CLangViewer::addChildrenOfPath(OovStringRef path)
    {
    ListItem *listItem = ListItem::findChildItem(&mListTree, path);
    addChildren(listItem);
    }

void CLangViewer::addChildren(ListItem *listItem)
    {
    FunctionRequests funcRequests;
    funcRequests.push_back(FT_ChildLabel);
    funcRequests.push_back(FT_getCursorSpelling);
    funcRequests.push_back(FT_getCursorDisplayName);
    funcRequests.push_back(FT_getCursorDefinition);
    funcRequests.push_back(FT_getCursorUSR);

    funcRequests.push_back(FT_getTypeSpelling);
    funcRequests.push_back(FT_getCursorKindSpelling);
    funcRequests.push_back(FT_getCanonicalType);
    funcRequests.push_back(FT_getTypedefDeclUnderlyingType);
    funcRequests.push_back(FT_getPointeeType);

    funcRequests.push_back(FT_getCursorReferenced);
    funcRequests.push_back(FT_getCursorSemanticParent);
    funcRequests.push_back(FT_getCursorLexicalParent);
    if(okToAddChildren(*listItem, funcRequests.size()))
        {
        mVisitorCommand.set(listItem, funcRequests);
        clang_visitChildren(listItem->getCursor(), CLangViewer::visitCursor, this);
        }
    }

ListItem *CLangViewer::addChild(ListItem &parent, CXCursor cursor, FuncTypes ft)
    {
    ListItem listItem;
    listItem.setCursor(cursor);     // Set the base cursor
    listItem.getFuncResult(ft);
    GuiTreeItem guiItem = mTreeView.appendText(parent.getGuiItem(),
        listItem.getResultStr());
    listItem.setCursor(listItem.getResultChildCursor());
    listItem.setGuiItem(guiItem);
    parent.addChildItem(listItem);
    return(&parent.getChildren()[parent.getChildren().size()-1]);
    }

CXChildVisitResult CLangViewer::visitCursor(CXCursor cursor,
    CXCursor /*parent*/)
    {
    // Add first item, then add other items to first item.
    ListItem *parentListItem = mVisitorCommand.mParentListItem;
    parentListItem = addChild(*parentListItem, cursor,
        mVisitorCommand.mFuncRequests[0]);
    for(size_t i=1; i<mVisitorCommand.mFuncRequests.size(); i++)
        {
        addChild(*parentListItem, cursor, mVisitorCommand.mFuncRequests[i]);
        }
    return CXChildVisit_Continue;
    }

struct SearchData
    {
    OovString srchStr;
    GuiTree *tree;
    };

static gboolean ViewerGtkTreeModelForeachFunc(GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer data)
    {
    bool stopSearching = false;
    SearchData *srchData = static_cast<SearchData*>(data);
    GValue value = { 0, 0 };
    gtk_tree_model_get_value(model, iter, 0, &value);
    if(G_VALUE_TYPE(&value) == G_TYPE_STRING)
        {
        OovString itemStr = g_value_get_string(&value);
        if(itemStr.find(srchData->srchStr) != std::string::npos)
            {
            GtkTreeSelection *treesel = gtk_tree_view_get_selection(
                srchData->tree->getTreeView());
            gtk_tree_selection_select_iter(treesel, iter);

            GuiTreePath path(model, iter);
            srchData->tree->expandRow(path);
            srchData->tree->scrollToPath(path);
            stopSearching = true;
            }
        }
    return stopSearching;
    }

void CLangViewer::find()
    {
    GtkDialog *dlg = GTK_DIALOG(Builder::getBuilder()->getWidget("FindDialog"));;
    Dialog findDlg(dlg);
    if(findDlg.run(true))
        {
        GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget("FindEntry"));

        GtkTreeModel *model = mTreeView.getModel();
        SearchData srchData;
        srchData.srchStr = Gui::getText(entry);
        srchData.tree = &mTreeView;
        gtk_tree_model_foreach(model, ViewerGtkTreeModelForeachFunc,
            reinterpret_cast<gpointer>(&srchData));
        }
    }

static void displayBrowserFile(OovStringRef const fileName)
    {
#ifdef __linux__
    pid_t pid=fork();
    if(!pid)
        {
        char const *prog = "/usr/bin/xdg-open";
        char const *args[3];
        args[0] = prog;
        args[1] = fileName.getStr();
        args[2] = nullptr;
//printf("FF %s\n", fp.c_str());
        execvp(prog, const_cast<char**>(args));
        }
#else
    FilePath fpTest(fileName, FP_File);
    ShellExecute(NULL, "open", fpTest.getAsWindowsPath().getStr(),
        NULL, NULL, SW_SHOWNORMAL);
#endif
    }

///
static void displayHelpFile(OovStringRef const fileName)
    {
    FilePath fullFn;
    static char const *dirs[] = { "help", "..\\..\\web\\userguide" };
    OovStatus status(true, SC_File);
    for(auto const dir : dirs)
        {
        fullFn.setPath(dir, FP_Dir);
        fullFn.appendFile(fileName);
        if(FileIsFileOnDisk(fullFn, status))
            {
            break;
            }
        }
    if(!FileIsFileOnDisk(fullFn, status))
        {
        fullFn.setPath("http://oovaide.sourceforge.net/userguide", FP_Dir);
        fullFn.appendFile(fileName);
        }
    status.reported();
    displayBrowserFile(fullFn);
    }

/// @todo - duplicate code from oovEdit/FileEditView.cpp
// This currently does not get the package command line arguments,
// but usually these won't be needed for compilation.
static void getCppArgs(OovStringRef const srcName, OovProcessChildArgs &args)
    {
    ProjectReader proj;
    ProjectBuildArgs buildArgs(proj);
    OovStatus status = proj.readProject(Project::getProjectDirectory());
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read project to get CPP args");
        }
    buildArgs.setConfig(OptFilterValueBuildModeAnalyze, BuildConfigAnalysis);
    OovStringVec cppArgs = buildArgs.getCompileArgs();
    for(auto const &arg : cppArgs)
        {
        args.addArg(arg);
        }

    BuildConfigReader cfg;
    std::string incDepsPath = cfg.getIncDepsFilePath();
    IncDirDependencyMapReader incDirMap;
    status = incDirMap.read(incDepsPath);
    if(status.ok())
        {
        OovStringVec incDirs = incDirMap.getNestedIncludeDirsUsedBySourceFile(srcName);
        for(auto const &dir : incDirs)
            {
            std::string arg = "-I";
            arg += dir;
            args.addArg(arg);
            }
        }
    else
        {
        status.reported();
        }
    }


int main(int argc, char **argv)
    {
    gtk_init(&argc, &argv);
    Builder builder;
    if(Builder::getBuilder()->addFromFile("ClangViewer.glade"))
        {
        Builder::getBuilder()->connectSignals();
        CLangViewer viewer;

        OovProcessChildArgs args;
        OovString filename;
        for(int i=1; i<argc; i++)
            {
            // Don't know how to find the file name. A period could exist in
            // a directory or filename. This requires that the filename is
            // not a switch '-', and contains a period for .h, .cpp, etc.
            // Some args don't have a '-' such as "-x c++".
            if(argv[i][0] != '-' && strchr(argv[i], '.') != nullptr)
                {
                filename = argv[i];
                }
            else if(argv[i][0] == '-' && argv[i][1] == 'p')
                {
                OovString projName = FilePathFixFilePath(&argv[i][2]);
                Project::setProjectDirectory(projName);
                }
            else
                {
                // The passed in arguments must already be quoted correctly.
                // For example, if a path is specified as "-I\te st\", in a
                // batch file, the correct quoting is "-I\te st\\".
                args.addArg(argv[i]);
                }
            }
        if(filename.length() > 0)
            {
            getCppArgs(filename, args);
            viewer.parse(filename, args.getArgc(), args.getArgv());
            viewer.addInitialRootItems();
            }
        viewer.showWindow();
        }
    else
        {
        Gui::messageBox("The file ClangViewer.glade must be in the executable directory.");
        }
    }


extern "C" G_MODULE_EXPORT void on_FileOpenImagemenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    }

extern "C" G_MODULE_EXPORT void on_FileQuitImagemenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
#if(GTK_MINOR_VERSION >= 10)
    gtk_window_close(Gui::getMainWindow());
#endif
    }

extern "C" G_MODULE_EXPORT void on_AddMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClangViewer->addChildrenOfSelectedItem();
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutImagemenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    char const * const comments = "This is an AST viewer based on CLang";
    GtkWidget *widget = Builder::getBuilder()->getWidget("MainWindow");
    gtk_show_about_dialog(GTK_WINDOW(widget), "program-name", "CLangView",
        "version", "Version " VIEWER_VERSION, "comments", comments, nullptr);
    }

extern "C" G_MODULE_EXPORT gboolean on_MainTreeview_row_expanded(
    GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path,
    gpointer user_data)
    {
    gClangViewer->addChildrenOfPath(GuiTreePath(path).getStr());
    return true;
    }

extern "C" G_MODULE_EXPORT void on_HelpContentsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    displayHelpFile("CLangViewHelp.html");
    }

extern "C" G_MODULE_EXPORT void on_FindMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClangViewer->find();
    }

extern "C" G_MODULE_EXPORT void on_FindEntry_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    GtkWidget *dlg = Builder::getBuilder()->getWidget("FindDialog");
    g_signal_emit_by_name(dlg, "response", GTK_RESPONSE_OK);
    }
