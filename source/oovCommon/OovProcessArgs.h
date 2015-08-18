/*
 * OovProcessArgs.h
 *
 *  Created on: Feb 21, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVPROCESSARGS_H_
#define OOVPROCESSARGS_H_

#include "OovString.h"

/// Builds arguments for a child process.
class OovProcessChildArgs
    {
    public:
        OovProcessChildArgs()
            { clearArgs(); }

        /// Add command line arguments for the child process.
        /// WARNING: The arg[0] must be added.
        void addArg(OovStringRef const argStr);
        /// Erase all arguments.
        void clearArgs()
            { mArgStrings.clear(); }
        /// Get all arguments. The last array item is a null.
        char const * const *getArgv() const;
        /// Get the number of child arguments.
        size_t getArgc() const
            { return mArgStrings.size(); }
        /// Get the arguments as a single string.
        OovString getArgsAsStr() const;
        ///  Print all arguments to a file.
        void printArgs(FILE *fh) const;

    private:
        OovStringVec mArgStrings;
        mutable std::vector<char const*> mArgv; // These are created temporarily during getArgv
    };


#endif /* OOVPROCESSARGS_H_ */
