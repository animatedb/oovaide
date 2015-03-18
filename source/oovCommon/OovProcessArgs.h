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

	// WARNING: The arg[0] must be added.
	void addArg(OovStringRef const argStr);
	void clearArgs()
	    { mArgStrings.clear(); }
	char const * const *getArgv() const;
	const int getArgc() const
	    { return mArgStrings.size(); }
	OovString getArgsAsStr() const;
	void printArgs(FILE *fh) const;

    private:
	OovStringVec mArgStrings;
	mutable std::vector<char const*> mArgv;	// These are created temporarily during getArgv
    };


#endif /* OOVPROCESSARGS_H_ */
