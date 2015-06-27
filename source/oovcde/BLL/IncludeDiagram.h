/*
 * IncludeDiagram.h
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDE_DIAGRAM_H
#define INCLUDE_DIAGRAM_H

#include "IncludeDrawer.h"
#include "DiagramDrawer.h"
#include "Gui.h"

class IncludeDiagram
    {
    public:
        IncludeDiagram():
            mIncludeMap(nullptr), mModified(false)
            {}
        void initialize(const IncDirDependencyMapReader &includeMap);
        void clearGraphAndAddInclude(DiagramDrawer &drawer, OovStringRef incName);
        void restart(DiagramDrawer &drawer)
            {
            clearGraphAndAddInclude(drawer, mLastIncName);
            }
        void relayout(DiagramDrawer &drawer)
            {
            mIncludeDrawer.updateGraph(drawer, mIncludeGraph);
            }

        size_t getNodeIndex(DiagramDrawer &drawer, GraphPoint p) const
            { return mIncludeDrawer.getNodeIndex(drawer, p); }
        void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
            {
            mModified = true;
            mIncludeDrawer.setPosition(nodeIndex, startPoint, newPoint);
            }
        std::vector<IncludeNode> const &getNodes() const
            { return mIncludeGraph.getNodes(); }
        void addSuppliers(OovStringRef incName)
            { mIncludeGraph.addSuppliers(incName); }

        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);
        GraphSize getDrawingSize(DiagramDrawer &drawer) const
            { return mIncludeDrawer.getDrawingSize(drawer); }
        bool isModified() const
            { return mModified; }

    private:
        const IncDirDependencyMapReader *mIncludeMap;
        bool mModified;
        OovString mLastIncName;
        IncludeGraph mIncludeGraph;
        IncludeDrawer mIncludeDrawer;

        void updateDrawingAreaSize();
    };

#endif
