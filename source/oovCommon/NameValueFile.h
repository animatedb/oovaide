/*
 * NameValueFile.h
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef NAMEVALUEFILE_H_
#define NAMEVALUEFILE_H_

#include "OovString.h"

// This file format borrows slightly from json.
//
// The reserved characters are :{} and end of line
// 	Colon separates name from value
//	{} identifies the begin and end of a record.
//	end of line is end of a name value pair
//
// Differences from json are:
//	name values are not quoted
//	no array support

#include <map>
#include <string>
#include <vector>
#include <set>
#include "File.h"

class CompoundValueRef
    {
    public:
	static void addArg(OovStringRef const arg, OovStringVec &vec)
	    { vec.push_back(arg); }
	/// The delimiter is '\n' for editing in an editor, and a semicolon
	/// for passing on the command line or saved in the file.
	static OovString getAsString(const OovStringVec &vec,
		char delimiter=';');
	static OovString getAsString(const OovStringSet &stdset,
		char delimiter=';');
	static OovStringVec parseString(OovStringRef const str,
		char delimiter=';');
	static void parseStringRef(OovStringRef const str, OovStringVec &vec,
		char delimiter=';');
    };

/// This builds compound string arguments delimited with a semicolon.
/// This can help build options such as:
/// CppArgs|-I/mingw/include;-IC:/Program Files/GTK+-Bundle-3.6.1/include
class CompoundValue:public OovStringVec
    {
    public:
	CompoundValue()
	    {}
	CompoundValue(OovStringRef const str, char delimiter=';')
	    { parseString(str, delimiter); }
	CompoundValue(OovStringVec const &strs)
	    {
	    for(auto const &str : strs)
		{
		addArg(str);
		}
	    }
	static const size_t npos = static_cast<size_t>(-1);
	void addArg(OovStringRef const arg)
	    { push_back(arg); }
	/// The delimiter is \n for editing in an editor, and a semicolon
	/// for passing on the command line or saved in the file.
	OovString getAsString(char delimiter=';') const
	    { return CompoundValueRef::getAsString(*this, delimiter); }
	void parseString(OovStringRef const str, char delimiter=';')
	    { CompoundValueRef::parseStringRef(str, *this, delimiter); }
	// Returns index to found string, else npos.
	size_t find(OovStringRef const str);
	/// WARNING - This should not be saved to the options file.
	/// This is for passing command line arguments in Windows/DOS.
	/// This can only be used if the switch (ex: -I) is already stored in the argument.
	void quoteAllArgs();
	static void quoteCommandLineArg(std::string &str);
    };

class NameValueRecord
    {
    public:
	NameValueRecord():
	    mSaveNullValues(false)
	    {}
	static char const mapDelimiter = '|';
	void clear()
	    { mNameValues.clear(); }
	void setNameValue(OovStringRef const optionName, OovStringRef const value);
	OovString getValue(OovStringRef const optionName) const;
	void setNameValueBool(OovStringRef const optionName, bool val);
	bool getValueBool(OovStringRef const optionName) const;
	const std::map<OovString, OovString> &getNameValues() const
	    { return mNameValues; }
	void write(FILE *fp);
	void read(FILE *fp);
	void insertBufToMap(OovString const buf);
	void readMapToBuf(OovString &buf);
	void saveNullValues(bool save)
	    { mSaveNullValues = save; }

    private:
	bool mSaveNullValues;
	std::map<OovString, OovString> mNameValues;
	bool getLine(FILE *fp, OovString &str);
	void insertLine(OovString line);
    };

/// This can store a bunch of options in a file. Each option is on a separate line
/// and has a name and value separated by a colon.  Anything after the first
/// colon is a value, which means the value can contain colons.
class NameValueFile:public NameValueRecord
    {
    public:
	NameValueFile()
	    {}
	NameValueFile(OovStringRef const fn)
	    {
	    setFilename(fn);
	    }
	const std::string &getFilename() const
	    { return mFilename; }
	void setFilename(OovStringRef const fn)
	    { mFilename = fn; }
	bool readFile();
	bool writeFile();

	bool readFileShared();
	// These functions must be used together and assume that the current map
	// can be discarded and reread.
	// Clear and update map by reading current file, and lock/open file.
	// THIS WILL NOT SHRINK FILES!
	/// @todo - This does not need to reread files if they haven't changed
	/// since the first read.
	bool writeFileExclusiveReadUpdate(SharedFile &file);
	bool writeFileExclusive(SharedFile &file);

    private:
	OovString mFilename;
	bool readOpenedFile(SharedFile &file);
    };


#endif /* NAMEVALUEFILE_H_ */
