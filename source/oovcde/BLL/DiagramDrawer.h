/*
 * DiagramDrawer.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef DIAGRAMDRAWER_H_
#define DIAGRAMDRAWER_H_

#include <vector>
#include "OovString.h"
#include "Graph.h"
#include "FastGene.h"
#include "Options.h"
#include "EditorContainer.h"    // Make viewSource available.


typedef std::vector<GraphPoint> OovPolygon;
// Starting from the desired length, find the first position with a
// break character. Returned pos is always between min and max.
size_t findSplitPos(std::string const &str, size_t minLength,
        size_t maxLength);
inline const char *getSplitStringPrefix()
    { return "  "; }

/// @param originalIndices Use the new string index to get the original index.
void splitStrings(OovStringVec &strs, size_t minLength,
    size_t maxLength=std::string::npos,
	std::vector<size_t> *originalIndices=nullptr);

struct DrawString
    {
    DrawString()
        {}
    DrawString(GraphPoint p, OovStringRef const s):
        pos(p), str(s)
        {}
    GraphPoint pos;
    OovString str;
    };

class DiagramArrow
    {
    public:
        DiagramArrow(GraphPoint producer, GraphPoint consumer, int arrowSize);
        GraphPoint getLeftArrowPosition()
            { return mLeftPos; }
        GraphPoint getRightArrowPosition()
            { return mRightPos; }
    private:
        GraphPoint mLeftPos;
        GraphPoint mRightPos;
    };

/// Provides an abstract interface for drawing. This interface can be used for
/// drawing to screen, files (svg) or to a null device.
class DiagramDrawer
    {
    public:
        DiagramDrawer():
            mFontSize(10.0)
            {}
        virtual ~DiagramDrawer();
        /// This must be called before any other function.
        virtual void setDiagramSize(GraphSize size) = 0;
        virtual void setFontSize(double size)
            { mFontSize = size; }
        double getFontSize() const
            { return mFontSize; }
        virtual void drawRect(const GraphRect &rect)=0;
        virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
                bool dashed=false)=0;
        virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor) = 0;
        virtual void drawEllipse(const GraphRect &rect) = 0;
        virtual void drawPoly(const OovPolygon &poly, Color fillColor)=0;
        // The point specifies the bottom left corner of the text.
        virtual void drawText(const GraphPoint &p, OovStringRef const text)=0;
        virtual float getTextExtentWidth(OovStringRef const name) const =0;
        virtual float getTextExtentHeight(OovStringRef const name) const =0;
        /// These functions indicate when the start/end of some grouping
        /// of shapes is performed so similar attributes can be used.
        virtual void groupShapes(bool /*start*/, Color /*lineColor*/,
            Color /*fillColor*/)
            {}
        virtual void groupText(bool /*start*/, bool /*italics*/)
            {}
        /// Gets the width of a wide character, and divides by the divisor.
        /// Returns a minimum of one.
        int getPad(int div=10) const;
        static void getConnectionPoints(GraphRect const &consumerRect,
                GraphRect const &supplierRect,
                GraphPoint &consumerPoint, GraphPoint &supplierPoint);

    private:
        double mFontSize;
    };

class DiagramDependencyDrawer
    {
    public:
        virtual size_t getNumNodes() const = 0;
        virtual void setNodePosition(size_t nodeIndex, GraphPoint pos) = 0;
        virtual GraphPoint getNodePosition(size_t nodeIndex) const = 0;
        virtual OovString const &getNodeName(size_t nodeIndex) const = 0;
        virtual size_t getNumConnections() const = 0;
        virtual void getConnection(size_t ci, size_t &consumerIndex,
                size_t &supplierIndex) const = 0;
        virtual ~DiagramDependencyDrawer()
            {}

        GraphRect getNodeRect(DiagramDrawer &drawer, size_t nodeIndex) const;
        static const size_t NO_INDEX = static_cast<size_t>(-1);

    protected:
        size_t getNodeIndex(DiagramDrawer &drawer, GraphPoint p,
                size_t numNodes) const;
        /// This returns a column position for each depth.
        /// This returns positions without the margin.
        std::vector<int> getColumnPositions(DiagramDrawer &drawer,
                std::vector<size_t> const &depths) const;
        void drawArrowDependency(DiagramDrawer &drawer,
                GraphPoint &consumerPoint, GraphPoint &supplierPoint);
    };

class DistinctColors
    {
    public:
        static size_t getNumColors();
        static Color getColor(size_t index);
    };

/// This defines functionality to use a genetic algorithm used to layout the
/// node positions for the include diagram. The genetic algorithm will place
/// the objects with minimally overlapping lines.
/// @todo - and nodes that have more relations are closer to each other?
class DiagramDependencyGenes:public GenePool
    {
    public:
        void initialize(DiagramDependencyDrawer &drawer, size_t nodeHeight);
        // Do some iterations, and move the positions from the gene pool into
        // the drawer.
        void updatePositionsInDrawer();

    private:
        DiagramDependencyDrawer *mDrawer;
        size_t mNodeHeight;
        size_t mMaxDrawingHeight;
        size_t mMaxDistanceQ;
        size_t mMaxOverlapQ;
        size_t mMaxHeightQ;
        void setupQualityEachGeneration() override;
        QualityType calculateSingleGeneQuality(size_t geneIndex) const override;
        GeneValue getYPosition(size_t geneIndex, size_t nodeIndex) const;
        void setYPosition(size_t geneIndex, size_t nodeIndex, GeneValue val);
        size_t getDrawingHeight(size_t geneIndex) const;
        size_t getNodeOverlapCount(size_t geneIndex) const;
        size_t getNodeYDistances(size_t geneIndex) const;
        size_t getMaxNodesInColumn() const;
        bool nodesOverlap(size_t geneIndex, size_t node1, size_t node2) const;
    };

#endif /* DIAGRAMDRAWER_H_ */
