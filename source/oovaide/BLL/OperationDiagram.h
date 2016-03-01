/*
 * OperationDiagram.h
 *
 *  Created on: Jul 29, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONDIAGRAM_H_
#define OPERATIONDIAGRAM_H_

#include "OperationDrawer.h"
#include "Builder.h"


struct OperationDiagramParams
    {
    OperationDiagramParams():
        mIsConst(false)
        {}
    std::string mClassName;
    std::string mOpName;
    bool mIsConst;
    };

/// This defines functions used to interact with an operation diagram. The
/// OperationDiagram uses the OperationDrawer to draw the OperationGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class OperationDiagram:public Diagram
    {
    public:
        OperationDiagram():
            mModelData(nullptr), mOperationDrawer(*this)
            {}
        void initialize(const ModelData &modelData)
            { mModelData = &modelData; }
        void clearGraph()
            { mOpGraph.clearGraph(); }
        void clearGraphAndAddOperation(OovStringRef const className,
                OovStringRef const opName, bool isConst);
        void restart();
        const OperationNode *getNode(int x, int y) const
            { return mOpGraph.getNode(x, y); }
        const OperationClass *getClass(int x, int y) const
            { return mOpGraph.getClass(x, y); }
        const OperationCall *getOperation(int x, int y) const
            { return mOpGraph.getOperation(x, y); }
        void removeNode(const OperationNode *node)
            { mOpGraph.removeNode(node); }
        void addOperDefinition(const OperationCall &call)
            { mOpGraph.addOperDefinition(call); }
        void removeOperDefinition(const OperationCall &opcall)
            { mOpGraph.removeOperDefinition(opcall); }
        void addOperCallers(const OperationCall &call)
            { mOpGraph.addOperCallers(*mModelData, call); }
        void addVariableReferencesFromGraphNodes(OperationNode const *node)
            { mOpGraph.addVariableReferencesFromGraphNodes(node); }
        OovStringRef getNodeName(const OperationCall &opCall) const
            { return mOpGraph.getNodeName(opCall); }

        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);

        void updateDiagram();
        // Not const because this changes graph positions.
        GraphSize getDrawingSize(DiagramDrawer &drawer)
            { return mOperationDrawer.getDrawingSize(drawer, mOpGraph, mOptions); }
        bool isModified() const
            { return mOpGraph.isModified(); }

    private:
        const ModelData *mModelData;
        OperationDrawOptions mOptions;
        OperationGraph mOpGraph;
        OperationDrawer mOperationDrawer;
        OperationDiagramParams mLastOperDiagramParams;
    };


#endif /* OPERATIONDIAGRAM_H_ */
