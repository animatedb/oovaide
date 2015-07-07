/*
 * ClassDiagram.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSDIAGRAM_H_
#define CLASSDIAGRAM_H_

#include "ClassGraph.h"


/// This defines functions used to interact with a class diagram. The
/// ClassDiagram uses the ClassDrawer to draw the ClassGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ClassDiagram
    {
    public:
        ClassDiagram():
            mModelData(nullptr), mDesiredZoom(1.0)
                {}
        /// The taskStatusListener's are used in the add class functions, and in
        /// the genetic repositioning code.
        /// The background is on the background thread, and cannot directly
        /// call GUI functions.  The foreground has to call the GUI, since
        /// the function doesn't complete to let the GUI run.
        void initialize(const ModelData &modelData,
            DiagramDrawer &nullDrawer, ClassDrawOptions const &options,
            ClassGraphListener &graphListener,
            OovTaskStatusListener &foregroundTaskStatusListener,
            OovTaskStatusListener &backgroundTaskStatusListener);

        ///*** These functions change the number of nodes.
        /// This clears the graph and starts from the starting class/node.
        void restart();
        void clearGraph()
            { mClassGraph.clearGraph(); }
        // Create a new graph and add a class node.
        void clearGraphAndAddClass(OovStringRef const className,
            ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All,
            int depth=DEPTH_IMMEDIATE_RELATIONS, bool reposition=true);
        // Add a class node and related nodes to an existing graph.
        void addClass(OovStringRef const className,
            ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All,
            int depth=DEPTH_IMMEDIATE_RELATIONS, bool reposition=true);
        void addRelatedNodesRecurse(const ModelData &model, const ModelType *type,
            ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All,
            int maxDepth=DEPTH_IMMEDIATE_RELATIONS)
            { mClassGraph.addRelatedNodesRecurse(model, type, addType, maxDepth); }
        void getRelatedNodesRecurse(const ModelData &model, const ModelType *type,
                std::vector<ClassNode> &nodes,
                ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All,
                int maxDepth=DEPTH_IMMEDIATE_RELATIONS)
            { mClassGraph.getRelatedNodesRecurse(model, type, addType, maxDepth, nodes); }
        void removeNode(ClassNode const &node, const ModelData &modelData)
            { mClassGraph.removeNode(node, modelData); }
        bool isNodeTypePresent(ClassNode const &node) const
            { return mClassGraph.isNodeTypePresent(node); }

        // This sets the drawer for computing sizes.
        void setNullDrawer(DiagramDrawer &drawer)
            { mClassGraph.setDrawer(drawer); }

        ///*** These functions can change size or positions of nodes
        void changeOptions(ClassDrawOptions const &options)
            { mClassGraph.changeOptions(options); }
        /// This repositions nodes, and can be slow.
        void updateGraph(bool resposition);

        /// Redraw the graph. Does not change the graph or change or access the model.
        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);
        void saveDiagram(FILE *fp);
        void loadDiagram(FILE *fp);

        ClassNode *getNode(int x, int y);
        const std::vector<ClassNode> &getNodes() const
            { return mClassGraph.getNodes(); }
        std::vector<ClassNode> &getNodes()
            { return mClassGraph.getNodes(); }
        void setZoom(double zoom)
            { mDesiredZoom = zoom; }
        double getDesiredZoom() const
            { return mDesiredZoom; }

        GraphSize getGraphSize() const
            { return mClassGraph.getGraphSize(); }
        void setModified()
            { mClassGraph.setModified(); }
        bool isModified() const
            { return mClassGraph.isModified(); }

        const ModelData &getModelData() const
            { return *mModelData; }
        std::string const &getLastSelectedClassName() const
            { return mLastSelectedClassName; }
        static const int DEPTH_IMMEDIATE_RELATIONS = 2;
        static const int DEPTH_SINGLE_CLASS = 1;

    private:
        const ModelData *mModelData;
        ClassGraph mClassGraph;
        OovString mLastSelectedClassName;
        double mDesiredZoom;
        void setLastSelectedClassName(OovStringRef const name)
            { mLastSelectedClassName = name; }
    };

#endif
