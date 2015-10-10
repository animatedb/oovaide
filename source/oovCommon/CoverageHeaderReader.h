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

/// This is for code coverage header source file that is included and added
/// to the project being analyzed.  The coverage header file contains:
///     - An array of coverage counts for the whole project.  The array contains
///       one entry for every instrumented line in the project.
///     - One #define macro for each instrumented source or header file, that
///       defines the starting index for the file.  Each macro looks something
///       like:  "#define COV_file 42 // 20", where the 42 is the starting
///       index into the array, and the 20 is the number of instrumented
///       lines in that file.
class CoverageHeaderReader
    {
    public:
        CoverageHeaderReader():
            mNumInstrumentedLines(0)
            {}
        /// Returns the total number of instrumented lines in the project.
        int getNumInstrumentedLines() const
            { return mNumInstrumentedLines; }
        /// Get the name of the coverage header file.
        /// @param outDir The directory for the coverage files.  This will return
        ///     outDir + "/covLib/OovCoverage.h"
        static FilePath getFn(OovStringRef const outDir);
        /// Reads the coverage header file into memory
        /// @param outDefFile This must contain the full path to the coverage
        ///     header file.
        OovStatusReturn read(SharedFile &outDefFile);
        /// Returns the map where each item is a #define macro name for each
        /// instrumented file and a count of the number of instrumented lines
        /// for the instrumented file.
        std::map<OovString, int> const &getMap() const
            { return mInstrDefineMap; }

    protected:
        std::map<OovString, int> mInstrDefineMap;       // file define, count
        int mNumInstrumentedLines;      // The total number of instrumented lines.

        void insertBufToMap(OovString const &buf);
    };

#endif /* COVERAGEHEADERREADER_H_ */
