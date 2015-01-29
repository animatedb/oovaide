/*
 * CoverageHeaderReader.h
 *
 *  Created on: Sep 17, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef COVERAGEHEADERREADER_H_
#define COVERAGEHEADERREADER_H_

#include "File.h"
#include "FilePath.h"
#include <map>

class CoverageHeaderReader
    {
    public:
	CoverageHeaderReader():
	    mNumInstrumentedLines(0)
	    {}
	int getNumInstrumentedLines() const
	    { return mNumInstrumentedLines; }
	static FilePath getFn(OovStringRef const outDir);
	// Reads the instr def file into the map
	void read(SharedFile &outDefFile);
	std::map<OovString, int> const &getMap() const
	    { return mInstrDefineMap; }

    protected:
	std::map<OovString, int> mInstrDefineMap;	// file define, count
	int mNumInstrumentedLines;

	void insertBufToMap(OovString const &buf);
    };

#endif /* COVERAGEHEADERREADER_H_ */
