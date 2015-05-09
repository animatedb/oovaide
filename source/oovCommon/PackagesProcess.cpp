/*
 * PackagesProcess.cpp
 *
 *  Created on: Jan 29, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Packages.h"
#ifdef __linux__
#include "OovProcess.h"
#endif
#include <algorithm>


#ifdef __linux__

class TextProcessor:public OovProcessListener
    {
    public:
	virtual ~TextProcessor()
	    {}
	virtual void onStdOut(OovStringRef const out, size_t len) override;
	virtual void onStdErr(OovStringRef const out, size_t len) override;
	virtual void processComplete() override
	    {}
	bool spawn(char const * const procPath, char const * const *argv);

    public:
	std::string mText;
    };

bool TextProcessor::spawn(char const * const procPath, char const * const *argv)
    {
    mText.clear();
    OovPipeProcess proc;
    int exitCode;
    return(proc.spawn(procPath, argv, *this, exitCode));
    }

void TextProcessor::onStdErr(OovStringRef const out, size_t len)
    {
    mText += std::string(out, len);
    }

void TextProcessor::onStdOut(OovStringRef const out, size_t len)
    {
    mText += std::string(out, len);
    }

OovStringVec AvailablePackages::getAvailablePackages()
    {
    if(mPackageNames.size() == 0)
	{
	TextProcessor proc;
	OovProcessChildArgs args;
	args.addArg("pkg-config");
	args.addArg("--list-all");
	if(proc.spawn("pkg-config", args.getArgv()))
	    {
	    mPackageNames.parseString(proc.mText, '\n');
	    for(auto &pkgName : mPackageNames)
		{
		size_t pos = pkgName.find_first_of(" \t");
		if(pos != std::string::npos)
		    pkgName.resize(pos);
		}
	    std::sort(mPackageNames.begin(), mPackageNames.end());
	    }
	}
    return mPackageNames;
    }

Package AvailablePackages::getPackage(OovStringRef const pkgName) const
    {
    /// @todo - should check if all paths are ok with this.
    Package pkg(pkgName, "/usr");

    TextProcessor proc;
    OovProcessChildArgs args;
    args.addArg("pkg-config");
    args.addArg(pkgName);
    args.addArg("--cflags");
    if(proc.spawn("pkg-config", args.getArgv()))
	{
	CompoundValue pkgflags;
	CompoundValue incFlags;
	CompoundValue cppFlags;
	pkgflags.parseString(proc.mText, ' ');
	for(auto &flag : pkgflags)
	    {
	    if(flag[0] == '-' && flag[1] == 'I')
		{
		flag.erase(0, 2);
		if(flag.compare(0, pkg.getRootDir().length(), pkg.getRootDir()) == 0)
		    flag.erase(0, pkg.getRootDir().length());
		incFlags.push_back(flag);
		}
	    else if(flag[0] == '-')
		cppFlags.push_back(flag);
	    }
	pkg.setCompileInfo(incFlags.getAsString(), cppFlags.getAsString());
	}
    args.clearArgs();
    args.addArg("pkg-config");
    args.addArg(pkgName);
    args.addArg("--libs");
    if(proc.spawn("pkg-config", args.getArgv()))
	{
	CompoundValue pkgFlags;
	CompoundValue libFlags;
	CompoundValue linkFlags;
	pkgFlags.parseString(proc.mText, ' ');
	for(auto &flag : pkgFlags)
	    {
	    if(flag[0] == '-' && flag[1] == 'l')
		{
		flag.erase(0, 2);
		libFlags.push_back(flag);
		}
	    else if(flag[0] == '-')
		{
		linkFlags.push_back(flag);
		}
	    }
	pkg.setLinkInfo("lib", libFlags.getAsString(), linkFlags.getAsString());
	}
    return pkg;
    }

#endif
