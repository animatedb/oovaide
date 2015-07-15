/*
 * ClassDiagramView.cpp
 *
 *  Created on: Jun 17, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ClassDiagramView.h"
#include "Options.h"
#include "OptionsDialog.h"
#include "DiagramDrawer.h"
#include "Svg.h"

static ClassDiagramView *gClassDiagramView;
static GraphPoint gStartPosInfo;

static const ClassDrawOptions &getDrawOptions(GuiOptions const &guiOptions)
    {
    static ClassDrawOptions dopts;
    dopts.drawAttributes = guiOptions.getValueBool(OptGuiShowAttributes);
    dopts.drawOperations = guiOptions.getValueBool(OptGuiShowOperations);
    dopts.drawOperParams = guiOptions.getValueBool(OptGuiShowOperParams);
    dopts.drawOperReturn = guiOptions.getValueBool(OptGuiShowOperReturn);
    dopts.drawAttrTypes = guiOptions.getValueBool(OptGuiShowAttrTypes);
    dopts.drawOperTypes = guiOptions.getValueBool(OptGuiShowOperTypes);
    dopts.drawPackageName = guiOptions.getValueBool(OptGuiShowPackageName);
    dopts.drawOovSymbols = guiOptions.getValueBool(OptGuiShowOovSymbols);
    dopts.drawOperParamRelations = guiOptions.getValueBool(OptGuiShowOperParamRelations);
    dopts.drawOperBodyVarRelations = guiOptions.getValueBool(OptGuiShowOperBodyVarRelations);
    dopts.drawRelationKey = guiOptions.getValueBool(OptGuiShowRelationKey);
    return dopts;
    }

ClassDiagramListener::~ClassDiagramListener()
    {}

AddClassDialog *sAddClassDialog;
AddClassDialog::AddClassDialog():
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("AddClassDialog"))),
    mSelectedType(nullptr), mSelectedAddType(ClassGraph::AN_AllStandard)
    {
    mAddClassTree.init(*Builder::getBuilder(), "AddClassesTreeview",
            "Add", GuiTree::CT_BoolString, "Class");
    sAddClassDialog = this;
    }

void AddClassDialog::selectNodes(ClassDiagram const &classDiagram, ModelType const *type,
        ClassGraph::eAddNodeTypes addType, std::vector<ClassNode> &nodes)
    {
    mSelectedType = type;
    mSelectedAddType = addType;
    mAddClassTree.clear();
    GuiTreeItem root;
    nodes.erase(nodes.begin());     // First node is always self.
    for(auto const &node : nodes)
        {
        GuiTreeItem item = mAddClassTree.appendRow(root);
        mAddClassTree.setText(item, node.getType()->getName());
        bool present = classDiagram.isNodeTypePresent(node);
        mAddClassTree.setCheckbox(item, !present);
        }
    }


void ClassDiagramView::initialize(const ModelData &modelData,
        ClassDiagramListener &listener,
        OovTaskStatusListener &taskStatusListener)
    {
    mListener = &listener;
    mClassDiagram.initialize(modelData, mNullDrawer, getDrawOptions(mGuiOptions),
            *this, mForegroundBusyDialog, taskStatusListener);
    }

void ClassDiagramView::gotoClass(OovStringRef const className)
    {
    if(mListener)
        {
        mListener->gotoClass(className);
        }
    }

void ClassDiagramView::updateGraph(bool reposition)
    {
    mClassDiagram.updateGraph(reposition);
    updateDrawingAreaSize();
    }

void ClassDiagramView::updateDrawingAreaSize()
    {
    GraphSize size = mClassDiagram.getGraphSize().getZoomed(
            mClassDiagram.getDesiredZoom(), mClassDiagram.getDesiredZoom());
    GtkWidget *widget = getDiagramWidget();
    gtk_widget_set_size_request(widget, size.x, size.y);
    }

void ClassDiagramView::drawToDrawingArea()
    {
    GtkCairoContext cairo(getDiagramWidget());
    CairoDrawer cairoDrawer(cairo.getCairo());
    cairoDrawer.clearAndSetDefaults();

    mClassDiagram.drawDiagram(cairoDrawer);
    updateDrawingAreaSize();
    }

void ClassDiagramView::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(getDiagramWidget());
    SvgDrawer svgDrawer(fp, cairo.getCairo());

    mClassDiagram.drawDiagram(svgDrawer);
    }

void ClassDiagramView::buttonPressEvent(const GdkEventButton *event)
    {
    gClassDiagramView = this;
    gStartPosInfo.set(static_cast<int>(event->x), static_cast<int>(event->y));
    }

extern "C" G_MODULE_EXPORT void on_AddClassStandardRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/);

void ClassDiagramView::displayAddClassDialog()
    {
    AddClassDialog dialog;

    ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
    if(node && node->getType())
        {
        OovString title = "Add Class ";
        title += node->getType()->getName();
        dialog.setTitle(title);
        }

    // Fill the dialog with the default radio button and selected class info.
    GtkWidget *radio = Builder::getBuilder()->getWidget("AddClassStandardRadiobutton");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), true);
    GtkWidget *relayout = Builder::getBuilder()->getWidget("AddClassRelayoutCheckbutton");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(relayout), false);
    // Even though the toggle was set above, it doesn't trigger unless it has changed.
    on_AddClassStandardRadiobutton_toggled(radio, nullptr);
    if(dialog.run(true))
        {
        getDiagram().addRelatedNodesRecurse(getDiagram().getModelData(),
            dialog.getSelectedType(), dialog.getSelectedAddType());
        updateGraph(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(relayout)));
        }
    }

void ClassDiagramView::selectAddClass(ModelType const *type,
        ClassGraph::eAddNodeTypes addType)
    {
    std::vector<ClassNode> nodes;
    getDiagram().getRelatedNodesRecurse(getDiagram().getModelData(),
            type, nodes, addType);
    sAddClassDialog->selectNodes(getDiagram(), type, addType, nodes);
    }

void ClassDiagramView::displayListContextMenu(guint button, guint32 acttime, gpointer data)
    {
    gClassDiagramView = this;
    GtkMenu *menu = Builder::getBuilder()->getMenu("ClassListPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

void ClassDiagramView::viewSource(OovStringRef const module, unsigned int lineNum)
    {
    ::viewSource(mGuiOptions, module, lineNum);
    }

void ClassDiagramView::displayDrawContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GdkEventButton *event = static_cast<GdkEventButton*>(data);
    ClassNode *node = getNode(static_cast<int>(event->x), static_cast<int>(event->y));
    OovStringRef const nodeMenus[] =
        {
        "GotoClassMenuitem",
        "AddStandardMenuitem", "AddAllMenuitem",
        "AddSuperclassesMenuitem", "AddSubclassesMenuitem",
        "AddMembersUsingMenuitem", "AddMemberUsersMenuitem",
        "AddFuncParamsUsingMenuitem", "AddFuncParamUsersMenuitem",
        "AddFuncBodyVarUsingMenuitem", "AddFuncBodyVarUsersMenuitem",
        "RemoveClassMenuitem", "ViewSourceMenuitem"
        };
    Builder *builder = Builder::getBuilder();
    for(size_t i=0; i<sizeof(nodeMenus)/sizeof(nodeMenus[i]); i++)
        {
        gtk_widget_set_sensitive(builder->getWidget(
                nodeMenus[i]), node != nullptr);
        }

    GtkMenu *menu = builder->getMenu("DrawClassPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    gStartPosInfo.set(static_cast<int>(event->x), static_cast<int>(event->y));
    }


// This is set in glade.
//    gtk_widget_add_events(mDrawingArea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
//          GDK_FOCUS_CHANGE_MASK);
void ClassDiagramView::buttonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
        {
//      if(abs(event->x - gPressInfo.startPos.x) > 5 ||
//              abs(event->y - gPressInfo.startPos.y) > 5)
            {
            ClassNode *node = getNode(
                    gStartPosInfo.x, gStartPosInfo.y);
            if(node)
                {
                GraphPoint clickOffset(static_cast<int>(event->x),
                    static_cast<int>(event->y));
                clickOffset.sub(gStartPosInfo);
                GraphPoint newPos(node->getPosition());
                newPos.add(clickOffset.getZoomed(1/mClassDiagram.getDesiredZoom(),
                        1/mClassDiagram.getDesiredZoom()));

                node->setPosition(newPos);
                mClassDiagram.setModified();
                drawToDrawingArea();
                }
            }
//      else
            {
/*
            DiagramNode *node = getNode(event->x, event->y);
            if(node)
                {
//              gDiagramGraph.clearGraph();
//              gLastSelectedClassName = node->getClassifier()->getName();
//              gDiagramGraph.addNodes(gModelData, node->getClassifier());
//              gDiagramGraph.setGraph(gModelData, getDrawOptions());
//              gDiagramGraph.drawDiagram(getDrawOptions());
                }
*/
            }
        }
    else
        {
        displayDrawContextMenu(event->button, event->time,
            reinterpret_cast<gpointer>(const_cast<GdkEventButton*>(event)));
        }
    }


extern "C" G_MODULE_EXPORT void on_GotoClassMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
    if(node)
        {
        gClassDiagramView->gotoClass(node->getType()->getName());
        }
    }

extern "C" G_MODULE_EXPORT void on_RestartMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->getDiagram().restart();
    }

extern "C" G_MODULE_EXPORT void on_RelayoutMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->updateGraph(true);
    }

void handlePopup(ClassGraph::eAddNodeTypes addType)
    {
    static int depth = 0;
    depth++;
    if(depth == 1)
        {
        ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
        if(node)
            {
            gClassDiagramView->getDiagram().addRelatedNodesRecurse(
                    gClassDiagramView->getDiagram().getModelData(),
                    node->getType(), addType);
            gClassDiagramView->updateGraph(true);
            }
        }
    depth--;
    }

extern "C" G_MODULE_EXPORT void on_AddAllMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_All);
    }

extern "C" G_MODULE_EXPORT void on_AddStandardMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_AllStandard);
    }

extern "C" G_MODULE_EXPORT void on_AddSuperclassesMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_Superclass);
    }

extern "C" G_MODULE_EXPORT void on_AddSubclassesMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_Subclass);
    }

extern "C" G_MODULE_EXPORT void on_AddMembersUsingMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_MemberChildren);
    }

extern "C" G_MODULE_EXPORT void on_AddMemberUsersMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_MemberUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncParamsUsingMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_FuncParamsUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncParamUsersMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_FuncParamsUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncBodyVarUsingMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_FuncBodyUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncBodyVarUsersMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    handlePopup(ClassGraph::AN_FuncBodyUsers);
    }

extern "C" G_MODULE_EXPORT void on_RemoveClassMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
    if(node)
        {
        gClassDiagramView->getDiagram().removeNode(*node,
                gClassDiagramView->getDiagram().getModelData());
        gClassDiagramView->drawToDrawingArea();
        }
    }

extern "C" G_MODULE_EXPORT void on_RemoveAllMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->getDiagram().clearGraph();
    gClassDiagramView->updateGraph(true);
    }

extern "C" G_MODULE_EXPORT void on_ViewSourceMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
    if(node)
        {
        const ModelClassifier *classifier = node->getType()->getClass();
        if(classifier && classifier->getModule())
            {
            gClassDiagramView->viewSource(classifier->getModule()->getModulePath(),
                classifier->getLineNum());
            }
        }
    }

extern "C" G_MODULE_EXPORT void on_ClassPreferencesMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
    if(node)
        {
        ClassPreferencesDialog dlg;
        if(dlg.run(*Builder::getBuilder(), node->getNodeOptions()))
            {
            gClassDiagramView->getDiagram().changeOptions(getDrawOptions(
                    gClassDiagramView->getGuiOptions()));
            gClassDiagramView->drawToDrawingArea();
            }
        }
    }

extern "C" G_MODULE_EXPORT void on_Zoom1Menuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->getDiagram().setZoom(1);
    gClassDiagramView->drawToDrawingArea();
    }

extern "C" G_MODULE_EXPORT void on_ZoomHalfMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->getDiagram().setZoom(0.5);
    gClassDiagramView->drawToDrawingArea();
    }

extern "C" G_MODULE_EXPORT void on_ZoomQuarterMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->getDiagram().setZoom(0.25);
    gClassDiagramView->drawToDrawingArea();
    }

extern "C" G_MODULE_EXPORT void on_ClassListAddSingleClassMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    GuiList list;
    list.access(GTK_TREE_VIEW(Builder::getBuilder()->getWidget("ClassTreeview")));
    gClassDiagramView->addClass(list.getSelected(), ClassGraph::AN_All,
            ClassDiagram::DEPTH_SINGLE_CLASS, false);
    }

extern "C" G_MODULE_EXPORT void on_ClassListAddWithStandardRelationsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    GuiList list;
    list.access(GTK_TREE_VIEW(Builder::getBuilder()->getWidget("ClassTreeview")));
    gClassDiagramView->addClass(list.getSelected(), ClassGraph::AN_AllStandard);
    }

extern "C" G_MODULE_EXPORT void on_ClassListAddAllRelationsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    GuiList list;
    list.access(GTK_TREE_VIEW(Builder::getBuilder()->getWidget("ClassTreeview")));
    gClassDiagramView->addClass(list.getSelected(), ClassGraph::AN_All);
    }

extern "C" G_MODULE_EXPORT void on_AddSelectedMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gClassDiagramView->displayAddClassDialog();
    }


void handleToggle(GtkWidget *widget,  ClassGraph::eAddNodeTypes addType)
    {
    static int depth = 0;
    depth++;
    if(depth == 1)
        {
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
            {
            ClassNode *node = gClassDiagramView->getNode(gStartPosInfo.x, gStartPosInfo.y);
            if(node)
                {
                gClassDiagramView->selectAddClass(node->getType(), addType);
                }
            }
        }
    depth--;
    }

extern "C" G_MODULE_EXPORT void on_AddClassStandardRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_AllStandard);
    }

extern "C" G_MODULE_EXPORT void on_AddClassAllRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_All);
    }

extern "C" G_MODULE_EXPORT void on_AddClassSuperclassRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_Superclass);
    }

extern "C" G_MODULE_EXPORT void on_AddClassSubclassRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_Subclass);
    }

extern "C" G_MODULE_EXPORT void on_AddClassMemberRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_MemberChildren);
    }

extern "C" G_MODULE_EXPORT void on_AddClassMemberUserRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_MemberUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddClassFuncParamRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_FuncParamsUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddClassFuncParamUserRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_FuncParamsUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddClassFuncBodyRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_FuncBodyUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddClassFuncBodyUserRadiobutton_toggled(
    GtkWidget *widget, gpointer /*data*/)
    {
    handleToggle(widget, ClassGraph::AN_FuncBodyUsers);
    }

