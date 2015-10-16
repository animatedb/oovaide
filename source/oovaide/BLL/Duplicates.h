/*
 * Duplicates.h
 * Created on: Feb 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "OovString.h"

#ifndef DUPLICATES_H

class DuplicateOptions
    {
    public:
        DuplicateOptions():
            mFindDupsInLines(false), mNumTokenMatches(4)
            {}
        // The default for dups in lines is false since it is fairly rare, but when it
        // does happen, can create huge dup files.
        // See LLVM_DEFINE_OVERLOAD in VariadicFunction.h in the LLVM project for an example.
        bool mFindDupsInLines;
        size_t mNumTokenMatches;
    };

class DuplicateLineInfo
    {
    public:
        DuplicateLineInfo():
            mTotalDupLines(0), mFile1StartLine(0), mFile2StartLine(0)
            {}

    public:
        int mTotalDupLines;
        int mFile1StartLine;
        int mFile2StartLine;
        OovString mFile1;
        OovString mFile2;
    };


bool getDuplicateLineInfo(DuplicateOptions const &options,
        std::vector<DuplicateLineInfo> &dupLineInfo);

#endif
