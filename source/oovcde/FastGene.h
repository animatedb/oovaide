// fastgene.h
// \copyright 1992 DCBlaha.  Distributed under the GPL.

/**
* This code does not use a source and destination gene pool but instead modifies
* the genes in only one population. This means that the memory requirements
* are lower and also there is no need for useless copying between the gene
* pools. Mathematically this should be equivalent to other genetic algorithms.
*/

#include <vector>
#include <stdint.h>
#include <memory.h>

typedef uint16_t QualityType;
// The randomizer, crossover and mutation are working on GeneValue sized parts of each gene.
typedef uint16_t GeneValue;

typedef uint8_t GeneByteValue;
typedef GeneByteValue *GenePtr;		// A pointer to gene strings
typedef const GeneByteValue *ConstGenePtr;	// A pointer to gene strings
typedef std::vector<QualityType> QualityHistogram;


/// This class holds the data for a genetic algorithm.
// Each gene's layout is:
//	uint8_t GeneBytes[genesize]
//	QualityType Quality
class GenePool
    {
    protected:
	int numgenes;			/// Number of genes in pool
	int genesize;			/// Length of each gene in bytes, not including quality
	double muterate;		/// Mutation rate
	std::vector<GeneByteValue> genes;	/// Memory for all genes
	std::vector<GenePtr> bestgenes;		/// List of good genes to cross over
	std::vector<GenePtr> worstgenes;	/// List of bad genes to overwrite
	int min;			/// Minimum gene byte value
	int max;

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
	void initialize(int genebytes, int numgenes, double crossoverrate=0.35,
	    double mutaterate=0.005, int minrand=0, int maxrand=255);

    protected:
	/// Called before calculating single genes.
	virtual void setupQualityEachGeneration()
	    {}
	/// This function is called for every gene. It is passed the gene to
	/// test and returns the quality of the gene.
	virtual QualityType calculateSingleGeneQuality(int geneIndex) const = 0;
	virtual void randomizeGene(int geneIndex);
	/// Offset is byte based.
	virtual void randomizeGeneValue(int geneIndex, int offset);
	/// Generate a random number between and including the min and max
	static GeneValue randRange(int min, int max);
	// Convert the input offset so that it fits on a gene value boundary.
	int geneValueBoundary(int offset)
	    { return(offset / sizeof(GeneValue) * sizeof(GeneValue)); }

    public:
	/// Do one generation of evolution for the geen pool.
	void singleGeneration();
	GenePtr getGene(int index)
	    { return(&genes[(genesize + sizeof(QualityType)) * index]); }
	ConstGenePtr getGene(int index) const
	    { return(&genes[(genesize + sizeof(QualityType)) * index]); }
	void setValue(int geneIndex, int offset, GeneValue val)
	    { memcpy(&getGene(geneIndex)[offset], &val, sizeof(val)); }
	GeneValue getValue(int geneIndex, int offset) const
	    {
	    GeneValue val;
	    memcpy(&val, &getGene(geneIndex)[offset], sizeof(val));
	    return val;
	    }
	QualityType getGeneQuality(int geneIndex) const
	    {
	    QualityType val;
	    memcpy(&val, &getGene(geneIndex)[genesize], sizeof(val));
	    return val;
	    }
	void setGeneQuality(int geneIndex, QualityType quality)
	    { memcpy(&getGene(geneIndex)[genesize], &quality, sizeof(quality)); }
	/// Build a histogram of the quality values of the genes.
	/// This may resize the histogram.
	void getQualityHistogram(QualityHistogram &qualities) const;
	int getBestGeneIndex();
    };
