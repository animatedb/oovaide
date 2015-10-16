// fastgene.h
// \copyright 1992 DCBlaha.  Distributed under the GPL.

/**
* This code does not use a source and destination gene pool but instead modifies
* the genes in only one population. This means that the memory requirements
* are lower and also there is no need for useless copying between the gene
* pools. Mathematically this should be equivalent to other genetic algorithms.
*/

#ifndef FASTGENE_H
#define FASTGENE_H

#include <vector>
#include <stdint.h>
#include <memory.h>

typedef uint16_t QualityType;
// The randomizer, crossover and mutation are working on GeneValue sized parts of each gene.
typedef uint16_t GeneValue;

typedef uint8_t GeneByteValue;
typedef GeneByteValue *GenePtr;         // A pointer to gene strings
typedef const GeneByteValue *ConstGenePtr;      // A pointer to gene strings
typedef std::vector<QualityType> QualityHistogram;


/// This class holds the data for a genetic algorithm.
// Each gene's layout is:
//      uint8_t GeneBytes[genesize]
//      QualityType Quality
class GenePool
    {
    protected:
        size_t numgenes;                        /// Number of genes in pool
        size_t genesize;                        /// Length of each gene in bytes, not including quality
        double muterate;                /// Mutation rate
        std::vector<GeneByteValue> genes;       /// Memory for all genes
        std::vector<GenePtr> bestgenes;         /// List of good genes to cross over
        std::vector<GenePtr> worstgenes;        /// List of bad genes to overwrite
        GeneValue min;                  /// Minimum gene value
        GeneValue max;

        /// This fills the quality value in all of the genes.
        void computeQuality();
        /// Crossover a pair of good genes and put the results into some bad genes.
        /// This does a GeneValue sized crossover. (Not bitwise)
        void crossover();
        /// Mutate some bits in the genes.
        void mutate();
        /// Build a list of best and worst genes so that crossover can be performed.
        void buildBestWorstList();

        GenePool():
            numgenes(0), genesize(0), muterate(.1), min(0), max(0)
            {}
        virtual ~GenePool()
            {}
        /// Get a gene pool. This allocates memory for the gene pool and initializes the
        /// genes with random data.
        /// @param genebytes Length of each gene in bytes, not including quality
        /// @param numgenes Number of genes to have in pool
        void initialize(size_t genebytes, size_t numgenes, double crossoverrate=0.35,
            double mutaterate=0.005, GeneValue minrand=0, GeneValue maxrand=255);

    protected:
        /// Called before calculating single genes.
        virtual void setupQualityEachGeneration()
            {}
        /// This function is called for every gene. It is passed the gene to
        /// test and returns the quality of the gene.
        virtual QualityType calculateSingleGeneQuality(size_t geneIndex) const = 0;
        virtual void randomizeGene(size_t geneIndex);
        /// Offset is byte based.
        virtual void randomizeGeneValue(size_t geneIndex, size_t offset);
        /// Generate a random number between and including the min and max
        static GeneValue randRange(size_t min, size_t max);
        // Convert the input offset so that it fits on a gene value boundary.
        size_t geneValueBoundary(size_t offset)
            { return(offset / sizeof(GeneValue) * sizeof(GeneValue)); }

    public:
        /// Do one generation of evolution for the geen pool.
        void singleGeneration();
        size_t getNumGenes() const
            { return numgenes; }
        GenePtr getGene(size_t index)
            { return(&genes[(genesize + sizeof(QualityType)) * index]); }
        ConstGenePtr getGene(size_t index) const
            { return(&genes[(genesize + sizeof(QualityType)) * index]); }
        void setValue(size_t geneIndex, size_t offset, GeneValue val)
            { memcpy(&getGene(geneIndex)[offset], &val, sizeof(val)); }
        GeneValue getValue(size_t geneIndex, size_t offset) const
            {
            GeneValue val;
            memcpy(&val, &getGene(geneIndex)[offset], sizeof(val));
            return val;
            }
        QualityType getGeneQuality(size_t geneIndex) const
            {
            QualityType val;
            memcpy(&val, &getGene(geneIndex)[genesize], sizeof(val));
            return val;
            }
        void setGeneQuality(size_t geneIndex, QualityType quality)
            { memcpy(&getGene(geneIndex)[genesize], &quality, sizeof(quality)); }
        /// Build a histogram of the quality values of the genes.
        /// This may resize the histogram.
        void getQualityHistogram(QualityHistogram &qualities) const;
        size_t getBestGeneIndex();
    };

#endif
