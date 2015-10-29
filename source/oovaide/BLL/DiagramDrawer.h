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


#define MIN_FONT_SIZE 5
#define MAX_FONT_SIZE 30

/// Each diagram can have its own font / font size.  When a diagram is saved,
/// the font is saved with the diagram.
/// If the global font is set, it applies to future diagrams, except if a
/// diagram is loaded from a file, the font from the file is kept.
class Diagram
    {
    public:
        Diagram():
            mDiagramBaseFontSize(getGlobalFontSize())
            {}
        void setDiagramBaseFontSize(double size)
            { mDiagramBaseFontSize = size; }
        /// This should only be set when the user sets the size. Not
        /// when a drawing is loaded.
        void setDiagramBaseAndGlobalFontSize(double size)
            {
            mDiagramBaseFontSize = size;
            setGlobalFontSize(size);
            }
        double getDiagramBaseFontSize() const
            { return mDiagramBaseFontSize; }
        static void setGlobalFontSize(double size);
        static double getGlobalFontSize();

    private:
        // This is the base font size for the diagram.
        double mDiagramBaseFontSize;
    };


/// Provides an abstract interface for drawing. This interface can be used for
/// drawing to screen, files (svg) or to a null device.
class DiagramDrawer
    {
    public:
        DiagramDrawer(Diagram &diagram):
            mDiagram(diagram),
            mCurrentDrawingFontSize(diagram.getDiagramBaseFontSize())
            {}
        virtual ~DiagramDrawer();
        /// These must be called before any other drawing function.
        virtual void setDiagramSize(GraphSize size) = 0;
        virtual void setCurrentDrawingFontSize(double size)
            { mCurrentDrawingFontSize = size; }

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
        double getCurrentDrawingFontSize() const
            { return mCurrentDrawingFontSize; }

    private:
        Diagram &mDiagram;
        // This can be affected by zoom, or by relative font sizes from the base font size.
        // This is not the base diagram font size. See the Diagram class.
        double mCurrentDrawingFontSize;
    };

class DiagramDependencyDrawer
    {
    public:
        virtual size_t getNumNodes() const = 0;
        virtual void setNodePosition(size_t nodeIndex, GraphPoint pos) = 0;
        virtual GraphPoint getNodePosition(size_t nodeIndex) const = 0;
        virtual OovString const getNodeName(size_t nodeIndex) const = 0;
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
        virtual void setupQualityEachGeneration() override;
        virtual QualityType calculateSingleGeneQuality(size_t geneIndex) const override;
        GeneValue getYPosition(size_t geneIndex, size_t nodeIndex) const;
        void setYPosition(size_t geneIndex, size_t nodeIndex, GeneValue val);
        size_t getDrawingHeight(size_t geneIndex) const;
        size_t getNodeOverlapCount(size_t geneIndex) const;
        size_t getNodeYDistances(size_t geneIndex) const;
        size_t getMaxNodesInColumn() const;
        bool nodesOverlap(size_t geneIndex, size_t node1, size_t node2) const;
    };

#endif /* DIAGRAMDRAWER_H_ */
