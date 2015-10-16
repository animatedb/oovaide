/*
 * PortionDiagram.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTION_DIAGRAM_H
#define PORTION_DIAGRAM_H

#include "PortionDrawer.h"
#include "Gui.h"

class PortionDiagram
    {
    public:
        PortionDiagram():
            mModelData(nullptr)
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
                classifier = type->getClass();
                }
            return classifier;
            }

        size_t getNodeIndex(DiagramDrawer &drawer, GraphPoint p) const
            { return mPortionDrawer.getNodeIndex(drawer, p); }
        void setPosition(size_t nodeIndex, GraphPoint newPoint)
            { mPortionDrawer.setPosition(nodeIndex, newPoint); }
        void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
            { mPortionDrawer.setPosition(nodeIndex, startPoint, newPoint); }

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
