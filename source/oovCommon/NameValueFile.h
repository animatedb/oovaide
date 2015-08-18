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
//      Colon separates name from value
//      {} identifies the begin and end of a record.
//      end of line is end of a name value pair
//
// Differences from json are:
//      name values are not quoted
//      no array support

#include <map>
#include <string>
#include <vector>
#include <set>
#include "File.h"


/// This is just a collection of a few static functions for conversion of
/// collections to and from strings.
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
        /// Add a string argument to the collection.
        void addArg(OovStringRef const arg)
            { push_back(arg); }
        /// Convert the collection to a string with delimiters between
        /// the items.
        /// The delimiter is \n for editing in an editor, and a semicolon
        /// for passing on the command line or saved in the file.
        OovString getAsString(char delimiter=';') const
            { return CompoundValueRef::getAsString(*this, delimiter); }
        /// Parse a string with delimiters into the collection.
        void parseString(OovStringRef const str, char delimiter=';')
            { CompoundValueRef::parseStringRef(str, *this, delimiter); }
        /// Returns index to found string, else npos.
        size_t find(OovStringRef const str);
        /// WARNING - This should not be saved to the options file.
        /// This is for passing command line arguments in Windows/DOS.
        /// This can only be used if the switch (ex: -I) is already stored in the argument.
        void quoteAllArgs();
        static void quoteCommandLineArg(std::string &str);
    };


/// This holds a collection of named value items, where each value is a string.
/// Each string value can be a compound value.
class NameValueRecord
    {
    public:
        NameValueRecord():
            mSaveNullValues(false)
            {}

        /// This is the delimiter between the name and value string.
        /// The delimiter for each name, value item is a '\n'.
        static char const mapDelimiter = '|';

        /// Clear all values.
        void clear()
            { mNameValues.clear(); }

        /// Append one name, value item.
        /// @param optionName The name of the item.
        /// @param value The value of the item.
        void setNameValue(OovStringRef const optionName, OovStringRef const value);

        /// Get a value associated with the name
        OovString getValue(OovStringRef const optionName) const;

        /// Set a name value item, where the value is a bool.
        /// @param optionName The name of the item.
        /// @param val The boolean value.
        void setNameValueBool(OovStringRef const optionName, bool val);

        /// Get the value of an item that is a boolean.
        /// @param optionName The name of the item to look up.
        bool getValueBool(OovStringRef const optionName) const;

        /// Get all of the items in the collection.
        const std::map<OovString, OovString> &getNameValues() const
            { return mNameValues; }

        /// Write all items to the file pointer.
        /// @param fp The file pointer to write to.
        void write(FILE *fp);

        /// Read all items from the file pointer.
        /// @param fp The file pointer to read from.
        void read(FILE *fp);

        /// Take a string with '\n' and add the items to the collection.
        /// @param buf The buffer to read and add to the collection.
        void insertBufToMap(OovString const buf);

        /// Read the items and for each item, insert a '\n' as the delimiter.
        /// @param buf The buffer to write from the collection.
        void readMapToBuf(OovString &buf);

        /// Set a flag to indicate whether to save empty items.
        /// @param save Set true to save empty items.
        void saveNullValues(bool save)
            { mSaveNullValues = save; }

    private:
        bool mSaveNullValues;
        std::map<OovString, OovString> mNameValues;
        bool getLine(FILE *fp, OovString &str);
        void insertLine(OovString line);
    };

/// This can store a bunch of items in a file. Each item is on a separate line
/// and has a name and value separated by a separator.  Anything after the first
/// separator is a value, which means the value can contain characters that
/// are the same as the separator.
class NameValueFile:public NameValueRecord
    {
    public:
        NameValueFile()
            {}
        NameValueFile(OovStringRef const fn)
            {
            setFilename(fn);
            }
        /// Get the file name of the file.
        const std::string &getFilename() const
            { return mFilename; }
        /// Set the file name of the file.
        void setFilename(OovStringRef const fn)
            { mFilename = fn; }

        /// Read the file into the map of items.
        bool readFile();
        /// Write the file from the map of items.
        bool writeFile();

        /// Read the file using the file poniter. This does not use the filename.
        void readFile(FILE *fp);
        /// Write the file using the file poniter. This does not use the filename.
        void writeFile(FILE *fp);
        /// Seek to the beginning of the file.
        void seekStart(FILE *fp)
            { fseek(fp, 0, SEEK_SET); }

        /// Read the file using shared file access.  This may take some time
        /// if the file is being written.
        bool readFileShared();

        /// Clear and update map by reading current file, and lock/open file.
        /// This function is meant to be used with the writeFileExclusive
        /// funtion.  This function is used first to read the existing file
        /// into the map. Then the map can be modified, and finally the file
        /// is written with writeFileExclusive.
        /// @todo - This does not need to reread files if they haven't changed
        /// since the first read.
        bool writeFileExclusiveReadUpdate(SharedFile &file);

        /// Write exclusive means that a normal file write is performed with
        /// no sharing, and it will fail if the file is open somewhere else.
        /// THIS WILL NOT SHRINK FILES!
        bool writeFileExclusive(SharedFile &file);

    private:
        OovString mFilename;
        bool readOpenedFile(SharedFile &file);
    };


#endif /* NAMEVALUEFILE_H_ */
