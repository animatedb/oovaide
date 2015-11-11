/*
 * Contexts.cpp
 * Created on: May 14, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "Contexts.h"
#include "Gui.h"
#include "BuildConfigReader.h"
#include "IncludeMap.h"

static bool sTreeViewRightClick = false;
static GdkEventButton sTreeViewRightClickButton;
static Contexts *sContexts;


Contexts::Contexts(OovProject &proj):
    mProject(proj), mJournal(proj.getGuiOptions()), mCurrentContext(C_BinaryComponent),
    mEditorContainer(mProject.getModelData())
    {
    sContexts = this;
    mEditorContainer.setListener(this);
    }

void Contexts::init(OovTaskStatusListener &taskStatusListener)
    {
    Builder *builder = Builder::getBuilder();
    mComponentList.init(*builder, "ModuleTreeview", "Module List");
    mIncludeList.init(*builder);
    mClassList.init(*builder);
    mOperationList.init(*builder);
    mZoneList.init();
    mJournalList.init(*builder);
    mJournal.init(*builder, mProject.getModelData(), mProject.getIncMap(),
            *this, taskStatusListener);
    }

void Contexts::clear()
    {
    mProject.clearAnalysis();
    mJournal.clear();
    mComponentList.clear();
    mIncludeList.clear();
    mClassList.clear();
    clearSelectedComponent();
    mJournalList.clear();
    mOperationList.clear();
    mZoneList.clear();
    }

void Contexts::displayPortion(OovStringRef className)
    {
    if(std::string(className).length() > 0)
        {
        updateOperationList(mProject.getModelData(), className);
        mJournal.displayPortion(className);
        updateJournalList();
        }
    }

void Contexts::displaySelectedClassDiagram()
    {
    if(mProject.isAnalysisReady())
        {
        std::string className = getSelectedClass();
        if(mCurrentContext == C_Class)
            {
            if(sTreeViewRightClick)
                {
                ClassDiagramView *classDiagram = mJournal.getCurrentClassDiagram();
                classDiagram->displayListContextMenu(sTreeViewRightClickButton.button,
                        sTreeViewRightClickButton.time, nullptr);
                sTreeViewRightClick = false;
                }
            else
                {
                displayClass(className);
                }
            }
        else if(mCurrentContext == C_Portion)
            {
            displayPortion(className);
            }
        }
    }

void Contexts::displayOperationsDiagram()
    {

    std::string className = getSelectedClass();;
    std::string operName = mOperationList.getSelected();
    size_t spacePos = operName.find(' ');
    bool isConst = false;
    if(spacePos != std::string::npos)
        {
        operName.resize(spacePos);
        isConst = true;
        }
    displayOperation(className, operName, isConst);
    }

void Contexts::displayJournal()
    {
    if(mCurrentContext == C_Journal)
        {
        mJournal.setCurrentRecord(mJournalList.getSelectedIndex());
        gtk_widget_queue_draw(Builder::getBuilder()->getWidget("DiagramDrawingarea"));
        }
    }

void Contexts::displayComponentDiagram()
    {
    // The component list should be available all the time as long as a
    // component can be selected.
//    if(mProject.isAnalysisReady())
        {
        std::string fn = mComponentList.getSelectedFileName();
        if(fn.length() > 0)
            {
            viewSource(mProject.getGuiOptions(), fn, 1);
            }
        }
    }

void Contexts::displayIncludeDiagram()
    {
    if(mProject.isAnalysisReady())
        {
        // While the graph is initialized or destructed, there is no name.
        OovString incName = mIncludeList.getSelected();
        if(std::string(incName).length() > 0)
            mJournal.displayInclude(incName);
        }
    }

void Contexts::displayZoneDiagram()
    {
    if(mProject.isAnalysisReady())
        {
        ZoneDiagramView *zoneDiagramView = mJournal.getCurrentZoneDiagram();
        if(zoneDiagramView)
            {
            std::string comp = mZoneList.getComponentTree().getSelected('/');
            if(comp.length() > 0)
                {
                bool show = mZoneList.getComponentTree().toggleSelectedCheckbox();
                zoneDiagramView->setFilter(comp, !show);
                zoneDiagramView->updateGraphAndRequestRedraw();
                }
            }
        }
    }

void Contexts::zoneTreeviewButtonRelease(const GdkEventButton *event)
    {
    if(mProject.isAnalysisReady())
        {
        ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
        if(zoneDiagramView)
            {
            zoneDiagramView->listDisplayContextMenu(event);
            }
        }
    }

void Contexts::setContext(eContexts context)
    {
    enum ePageIndices { PI_Module, PI_Zone, PI_Class, PI_Include, PI_Seq, PI_Journal };
    ePageIndices page = PI_Module;

    mCurrentContext = context;
    switch(context)
        {
        case C_BinaryComponent:
            clearSelectedComponent();;
            mJournal.displayComponents();
            updateJournalList();
            page = PI_Module;
            break;

        case C_Include:
            if(mProject.isAnalysisReady())
                {
                page = PI_Include;
                displayIncludeDiagram();
                updateJournalList();
                }
            break;

        case C_Zone:
            if(mProject.isAnalysisReady())
                {
                mJournal.displayWorldZone();
                updateJournalList();
                page = PI_Zone;
                }
            break;

        case C_Class:
            displaySelectedClassDiagram();
            page = PI_Class;
            break;

        case C_Portion:
            displaySelectedClassDiagram();
            page = PI_Class;
            break;

        // Operation is always related to class, so it must be initialized
        // to first operation of class.
        case C_Operation:
            displayOperationsDiagram();
            page = PI_Seq;
            break;

        case C_Journal:
            displayJournal();
            page = PI_Journal;
            break;
        }
    GtkNotebook *notebook = GTK_NOTEBOOK(Builder::getBuilder()->getWidget("ListNotebook"));
    gtk_notebook_set_current_page(notebook, page);
    }

bool Contexts::handleEditorMessages(EditorContainerCommands &command)
    {
    bool handleMessage = false;
    if(!mEditorMessages.empty())
        {
        OovIpcMsg msg = mEditorMessages.top();
        mEditorMessages.pop();
        command = static_cast<EditorContainerCommands>(msg.getCommand());
        switch(command)
            {
            case ECC_ViewClassDiagram:
                displayClass(msg.getArg(1));
                break;

            case ECC_ViewPortionDiagram:
                displayPortion(msg.getArg(1));
                break;

            case ECC_RunAnalysis:
            case ECC_Build:
            case ECC_StopAnalysis:
                handleMessage = true;
                break;

            default:
                break;
            }
        }
    return handleMessage;
    }

void Contexts::updateContextAfterAnalysisCompletes()
    {
    mComponentList.updateComponentList();
    mProject.loadAnalysisFiles();
    }

void Contexts::updateContextAfterProjectLoaded()
    {
    mComponentList.updateComponentList();
    updateIncludeList();
    updateClassList();
    mZoneList.update();
    Gui::setCurrentPage(GTK_NOTEBOOK(Builder::getBuilder()->getWidget("ListNotebook")), 0);
    }

void Contexts::updateJournalList()
    {
    mJournalList.clear();
    for(const auto &rec : mJournal.getRecords())
        {
        mJournalList.appendText(rec->getFullName(true));
        }
    }

void Contexts::updateClassList()
    {
    mClassList.clear();
    ModelData &modelData = mProject.getModelData();
    for(size_t i=0; i<modelData.mTypes.size(); i++)
        {
        if(modelData.mTypes[i]->getDataType() == DT_Class)
            mClassList.appendText(modelData.mTypes[i]->getName());
        }
    mClassList.sort();
    }

void Contexts::updateIncludeList()
    {
    std::set<OovString> files = mProject.getIncMap().getAllFiles();
    for(auto const &file : files)
        {
        mIncludeList.appendText(file);
        }
    }

void Contexts::updateOperationList(const ModelData &modelData,
        OovStringRef const className)
    {
    mOperationList.clear();
    const ModelClassifier *cls = modelData.getTypeRef(className)->getClass();
    if(cls)
        {
        for(size_t i=0; i<cls->getOperations().size(); i++)
            {
            const ModelOperation *oper = cls->getOperations()[i].get();
            std::string opStr = oper->getName();
            if(oper->isConst())
                {
                opStr += ' ';
                opStr += "const";
                }
            mOperationList.appendText(opStr);
            }
        mOperationList.sort();
        }
    }

void Contexts::displayClass(OovStringRef const className)
    {
    // While the graph is initialized or destructed, there is no name.
    if(std::string(className).length() > 0)
        {
        mClassList.setSelected(className);
        updateOperationList(mProject.getModelData(), className);
        mJournal.displayClass(className);
        updateJournalList();
        }
    }

void Contexts::displayOperation(OovStringRef const className,
        OovStringRef const operName, bool isConst)
    {
    // While the graph is initialized or destructed, there is no name.
    if(std::string(className).length() > 0)
        {
        if(std::string(operName).length() > 0)
            {
            mJournal.displayOperation(className, operName, isConst);
            updateJournalList();
            }
        }
    }





extern "C" G_MODULE_EXPORT void on_ClassTreeview_cursor_changed(
        GtkWidget *button, gpointer data)
    {
    sContexts->displaySelectedClassDiagram();
    }

extern "C" G_MODULE_EXPORT bool on_ClassTreeview_button_press_event(
        GtkWidget *button, GdkEvent *event, gpointer data)
    {
    GdkEventButton *eventBut = reinterpret_cast<GdkEventButton*>(event);
    if(eventBut->button == 3)   // Right button
        {
        sTreeViewRightClickButton = *eventBut;
        sTreeViewRightClick = true;
        }
    return false;       // Not handled - continue processing
    }

extern "C" G_MODULE_EXPORT void on_OperationsTreeview_cursor_changed(
        GtkWidget *button, gpointer data)
    {
    sContexts->displayOperationsDiagram();
    }


extern "C" G_MODULE_EXPORT void on_JournalTreeview_cursor_changed(
        GtkWidget *button, gpointer data)
    {
    sContexts->displayJournal();
    }

extern "C" G_MODULE_EXPORT void on_ModuleTreeview_cursor_changed(
        GtkWidget *button, gpointer data)
    {
    sContexts->displayComponentDiagram();
    }

extern "C" G_MODULE_EXPORT void on_ZoneTreeview_cursor_changed(
        GtkWidget *widget, gpointer data)
    {
    sContexts->displayZoneDiagram();
    }

extern "C" G_MODULE_EXPORT void on_IncludeTreeview_cursor_changed(
        GtkWidget *widget, gpointer data)
    {
    sContexts->displayIncludeDiagram();
    }

extern "C" G_MODULE_EXPORT gboolean on_ZoneTreeview_button_press_event(
        GtkWidget *widget, const GdkEventButton *event)
    {
    // This prevents the right click display of popup from changing a checkbox.
    return(event->button != 1);
    }

extern "C" G_MODULE_EXPORT gboolean on_ZoneTreeview_button_release_event(
        GtkWidget *widget, const GdkEventButton *event)
    {
    bool handled = false;
    if(event->button != 1)
        {
        sContexts->zoneTreeviewButtonRelease(event);
        handled = true;
        }
    return handled;
    }


extern "C" G_MODULE_EXPORT void on_BinaryComponentToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_BinaryComponent);
    }

extern "C" G_MODULE_EXPORT void on_IncludeDiagramToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_Include);
    }

extern "C" G_MODULE_EXPORT void on_ZoneDiagramToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_Zone);
    }

extern "C" G_MODULE_EXPORT void on_ClassDiagramToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_Class);
    }

extern "C" G_MODULE_EXPORT void on_PortionDiagramToolbutton_clicked(GtkWidget *widget,
        gpointer user_data)
    {
    sContexts->setContext(Contexts::C_Portion);
    }

extern "C" G_MODULE_EXPORT void on_OperationDiagramToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_Operation);
    }

extern "C" G_MODULE_EXPORT void on_JournalToolbutton_clicked(
        GtkWidget * /*widget*/, gpointer /*user_data*/)
    {
    sContexts->setContext(Contexts::C_Journal);
    }

