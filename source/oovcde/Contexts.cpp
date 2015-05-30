/*
 * Contexts.cpp
 * Created on: May 14, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "Contexts.h"
#include "Gui.h"

static bool sDisplayClassViewRightClick = false;
static Contexts *sContexts;


Contexts::Contexts(OovProject &proj):
    mProject(proj), mCurrentPage(PI_Component)
    {
    sContexts = this;
    }

void Contexts::init(OovTaskStatusListener &taskStatusListener)
    {
    Builder *builder = Builder::getBuilder();
    mClassList.init(*builder);
    mComponentList.init(*builder, "ModuleTreeview", "Module List");
    mOperationList.init(*builder);
    mZoneList.init();
    mJournalList.init(*builder);
    mJournal.init(*builder, mProject.getModelData(), *this, taskStatusListener);
    }

void Contexts::clear()
    {
    mJournal.clear();
    mClassList.clear();
    mComponentList.clear();
    clearSelectedComponent();
    mJournalList.clear();
    mOperationList.clear();
    mZoneList.clear();
    }


void Contexts::classTreeViewCursorChanged()
    {
    if(mProject.isAnalysisReady())
	{
	std::string className = getSelectedClass();
	if(mCurrentPage == Contexts::PI_Class)
	    {
	    if(sDisplayClassViewRightClick)
		{
		mJournal.addClass(className);
		sDisplayClassViewRightClick = false;
		}
	    else
		{
		displayClass(className);
		}
	    }
	else if(mCurrentPage == Contexts::PI_Portion)
	    {
	    if(std::string(className).length() > 0)
		{
		mJournal.displayPortion(className);
		updateOperationList(mProject.getModelData(), className);
		updateJournalList();
		}
	    }
	}
    }

void Contexts::operationsTreeviewCursorChanged()
    {

    std::string className = getSelectedClass();;
    std::string operName = getSelectedOperation();
    size_t spacePos = operName.find(' ');
    bool isConst = false;
    if(spacePos != std::string::npos)
	{
	operName.resize(spacePos);
	isConst = true;
	}
    displayOperation(className, operName, isConst);
    }

void Contexts::journalTreeviewCursorChanged()
    {
    if(mCurrentPage == Contexts::PI_Journal)
	{
	mJournal.setCurrentRecord(getSelectedJournalIndex());
	gtk_widget_queue_draw(Builder::getBuilder()->getWidget("DiagramDrawingarea"));
	}
    }

void Contexts::moduleTreeviewCursorChanged()
    {
    // The component list should be available all the time as long as a
    // component can be selected.
//    if(mProject.isAnalysisReady())
	{
	std::string fn = getSelectedComponent();
	if(fn.length() > 0)
	    {
	    viewSource(fn, 1);
	    }
	}
    }

void Contexts::zoneTreeviewCursorChanged()
    {
    if(mProject.isAnalysisReady())
	{
	ZoneDiagram *zoneDiagram = mJournal.getCurrentZoneDiagram();
	if(zoneDiagram)
	    {
	    bool show = false;
	    std::string comp = getZoneList().getComponentTree().getSelected('/');
	    if(comp.length() > 0)
		{
		show = getZoneList().getComponentTree().toggleSelectedCheckbox();
		zoneDiagram->setFilter(comp, !show);
		zoneDiagram->updateDiagram();
		}
	    }
	}
    }

void Contexts::zoneTreeviewButtonRelease(const GdkEventButton *event)
    {
    if(mProject.isAnalysisReady())
	{
	ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
	if(zoneDiagram)
	    {
	    zoneDiagram->listDisplayContextMenu(event);
	    }
	}
    }

void Contexts::listNotebookSwitchPage(int pageNum)
    {
    mCurrentPage = static_cast<ePageIndices>(pageNum);
    Builder *builder = Builder::getBuilder();
    switch(pageNum)
	{
	case PI_Component:
	    clearSelectedComponent();;
	    mJournal.displayComponents();
	    updateJournalList();
	    break;

	case PI_Zone:
	    if(mProject.isAnalysisReady())
		{
		mJournal.displayWorldZone();
		updateJournalList();
		}
	    break;

	case PI_Class:
	    {
	    classTreeViewCursorChanged();

	    GtkWidget *childWidget = builder->getWidget("ClassTreeview");
	    GtkWidget *parentWidget = builder->getWidget("ClassScrolledwindow");
	    Gui::reparentWidget(childWidget, GTK_CONTAINER(parentWidget));
	    }
	    break;

	case PI_Portion:
	    {
	    classTreeViewCursorChanged();

	    GtkWidget *childWidget = builder->getWidget("ClassTreeview");
	    GtkWidget *parentWidget = builder->getWidget("PortionScrolledwindow");
	    Gui::reparentWidget(childWidget, GTK_CONTAINER(parentWidget));
	    }
	    break;

	// Operation is always related to class, so it must be initialized
	// to first operation of class.
	case PI_Seq:
	    operationsTreeviewCursorChanged();
	    break;

	case PI_Journal:
	    journalTreeviewCursorChanged();
	    break;
	}
    }

void Contexts::updateJournalList()
    {
    mJournalList.clear();
    for(const auto &rec : mJournal.getRecords())
	{
	mJournalList.appendText(rec->getFullName(true));
	}
    }

void Contexts::updateClassList(OovStringRef const className)
    {
    mClassList.setSelected(className);
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
    // While the graph is initialized, there is no name.
    if(std::string(className).length() > 0)
	{
	updateClassList(className);
	updateOperationList(mProject.getModelData(), className);
	mJournal.displayClass(className);
	updateJournalList();
	}
    }

void Contexts::displayOperation(OovStringRef const className,
	OovStringRef const operName, bool isConst)
    {
    // While the graph is initialized, there is no name.
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
    sContexts->classTreeViewCursorChanged();
    }

extern "C" G_MODULE_EXPORT bool on_ClassTreeview_button_press_event(
	GtkWidget *button, GdkEvent *event, gpointer data)
    {
    GdkEventButton *eventBut = reinterpret_cast<GdkEventButton*>(event);
    if(eventBut->button == 3)	// Right button
    	{
    	sDisplayClassViewRightClick = true;
    	}
    return false;	// Not handled - continue processing
    }

extern "C" G_MODULE_EXPORT void on_OperationsTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    sContexts->operationsTreeviewCursorChanged();
    }


extern "C" G_MODULE_EXPORT void on_JournalTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    sContexts->journalTreeviewCursorChanged();
    }

extern "C" G_MODULE_EXPORT void on_ModuleTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    sContexts->moduleTreeviewCursorChanged();
    }

extern "C" G_MODULE_EXPORT void on_ZoneTreeview_cursor_changed(
	GtkWidget *widget, gpointer data)
    {
    sContexts->zoneTreeviewCursorChanged();
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


extern "C" G_MODULE_EXPORT void on_ListNotebook_switch_page(GtkNotebook *notebook,
	GtkWidget *page, guint page_num, gpointer user_data)
    {
    sContexts->listNotebookSwitchPage(page_num);
    }

