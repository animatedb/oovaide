/*
 * ZoneDiagramView.h
 *
 *  Created on: Jun 19, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef ZONEDIAGRAMVIEW_H_
#define ZONEDIAGRAMVIEW_H_

#include "ZoneDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include "Components.h"
#include "Gui.h"


class ZoneDiagramListener
    {
    public:
        virtual ~ZoneDiagramListener();
        virtual void gotoClass(OovStringRef const className) = 0;
    };

class ZoneClassInfoToolTipWindow
    {
    public:
        ZoneClassInfoToolTipWindow():
            mTopWindow(nullptr), mLabel(nullptr)
            {}
        ~ZoneClassInfoToolTipWindow();
        void hide();
        void lostPointer();
        void handleCursorMovement(int x, int y, OovStringRef str);

    private:
        GtkWidget *mTopWindow;
        GtkLabel *mLabel;
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
    };

// This is a tree of components (directories), no modules (filenames)
class ZoneDiagramList
    {
    public:
        void init();
        void update();
        GuiTreeItem getParent(std::string const &compName)
            {
            std::string parent = ComponentTypesFile::getComponentParentName(compName);
            return mComponentTree.getItem(parent, '/');
            }
        void clear()
            {
            mComponentTree.clear();
            }
        GuiTree &getComponentTree()
            { return mComponentTree; }

    private:
        GuiTree mComponentTree;
        ComponentTypesFile mComponentFile;
    };


class ZoneDiagramView
    {
    public:
        ZoneDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions), mListener(nullptr),
            mNullDrawer(mZoneDiagram)
            {}
        void initialize(const ModelData &modelData,
                ZoneDiagramListener &listener)
            {
            mListener = &listener;
            mZoneDiagram.initialize(modelData);
            setCairoContext();
            }

        void clearGraphAndAddWorldZone()
            {
            setCairoContext();
            mZoneDiagram.clearGraphAndAddWorldZone();
            updateGraphAndRequestRedraw();
            }
        // This does not update the drawing
        void setFilter(std::string moduleName, bool set)
            { mZoneDiagram.setFilter(moduleName, set); }

        // For use by extern functions.
        void graphButtonPressEvent(const GdkEventButton *event);
        void graphButtonReleaseEvent(const GdkEventButton *event);
        void listDisplayContextMenu(const GdkEventButton *event);
        ZoneDiagram &getDiagram()
            { return mZoneDiagram; }
        ZoneDrawOptions const &getDrawOptions() const
            { return mZoneDiagram.getDrawOptions(); }

        void drawToDrawingArea();
        OovStatusReturn drawSvgDiagram(File &file);

        void gotoClass(OovStringRef const className);
        void showChildComponents(bool show);
        void showAllComponents(bool show);

        bool isModified() const
            { return mZoneDiagram.isModified(); }
        void viewClassSource();
        void updateGraphAndRequestRedraw()
            {
            mZoneDiagram.updateGraph(mNullDrawer);
            gtk_widget_queue_draw(getDiagramWidget());
            }
        void handleDrawingAreaLostPointer()
            { mToolTipWindow.lostPointer(); }
        void handleDrawingAreaMotion(int x, int y);
        void viewSource(OovStringRef const module, unsigned int lineNum);
        void setFontSize(int size)
            {
            mZoneDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            }
        int getFontSize()
            { return mZoneDiagram.getDiagramBaseFontSize(); }

    private:
        GuiOptions const &mGuiOptions;
        ZoneDiagram mZoneDiagram;
        ZoneDiagramListener *mListener;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        ZoneClassInfoToolTipWindow mToolTipWindow;
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            mNullDrawer.setCurrentDrawingFontSize(mZoneDiagram.getDiagramBaseFontSize());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
