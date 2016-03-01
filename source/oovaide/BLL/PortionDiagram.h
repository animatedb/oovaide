/*
 * PortionDiagram.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTION_DIAGRAM_H
#define PORTION_DIAGRAM_H

#include "PortionDrawer.h"
#include "Gui.h"

class PortionDiagram:public Diagram
    {
    public:
        PortionDiagram():
            mModelData(nullptr), mPortionDrawer(*this)
            {}
        void initialize(const ModelData &modelData);
        void clearGraphAndAddClass(DiagramDrawer &drawer, OovStringRef className);
        void relayout(DiagramDrawer &drawer)
            {
            mPortionDrawer.updateNodePositions(drawer);
            }
        GraphSize getDrawingSize(DiagramDrawer &drawer)
            { return mPortionDrawer.getDrawingSize(drawer); }
        OovStringRef getCurrentClassName() const
            { return mCurrentClassName; }
        ModelClassifier const *getCurrentClass() const
            {
            const ModelClassifier *classifier = nullptr;
            ModelType const *type = mModelData->getTypeRef(mCurrentClassName);
            if(type)
                {
                classifier = ModelClassifier::getClass(type);
                }
            return classifier;
            }

        size_t getNodeIndex(DiagramDrawer &drawer, GraphPoint p) const
            { return mPortionDrawer.getNodeIndex(drawer, p); }
        void setPosition(size_t nodeIndex, GraphPoint newPoint)
            { mPortionDrawer.setPosition(nodeIndex, newPoint); }
        void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
            { mPortionDrawer.setPosition(nodeIndex, startPoint, newPoint); }

        bool isSelected(size_t nodeIndex) const
            { return mPortionDrawer.isSelected(nodeIndex); }
        void setSingleSelection(size_t nodeIndex)
            { mPortionDrawer.setSingleSelection(nodeIndex); }
        // If no nodes are contained in the rectangle, this clears the selection.
        void setRectSelection(GraphPoint p1, GraphPoint p2)
            { mPortionDrawer.setRectSelection(p1, p2); }

        void moveSelection(GraphPoint p)
            { mPortionDrawer.moveSelection(p); }
        void alignTopSelection()
            { mPortionDrawer.alignTopSelection(); }
        void alignLeftSelection()
            { mPortionDrawer.alignLeftSelection(); }
        void spaceEvenlyDownSelection()
            { mPortionDrawer.spaceEvenlyDownSelection(); }

        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);
        bool isModified() const
            { return mPortionGraph.isModified(); }

        /// For storage.
        std::vector<PortionNode> const &getNodes() const
            { return mPortionGraph.getNodes(); }
        GraphPoint getNodePosition(size_t nodeIndex) const
            { return mPortionDrawer.getNodePosition(nodeIndex); }
        OovStatusReturn saveDiagram(File &file);
        OovStatusReturn loadDiagram(File &file, DiagramDrawer &diagDrawer);

    private:
        const ModelData *mModelData;
        PortionGraph mPortionGraph;
        PortionDrawer mPortionDrawer;
        OovString mCurrentClassName;
    };

#endif
