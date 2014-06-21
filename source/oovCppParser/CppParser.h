/*
 * CppParser.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CPPPARSER_H_
#define CPPPARSER_H_

#include "ModelObjects.h"
#include "ParseBase.h"
#include <map>
#include <set>
#include "NameValueFile.h"
#include "FilePath.h"

/// This works to build include paths, and to build include file dependencies
/// This makes a file that keeps a map of paths, and for each path:
///	the last time the path dependencies changed,
///	the last time the paths were checked,
///	the included filepath (such as "gtk/gtk.h"), and the directory to get to that file.
/// This file does not store paths that the compiler searches by default.
class IncDirDependencyMap:public NameValueFile
    {
    public:
	void read(char const * const outDir, char const * const incPath);
	void write();
	void insert(const std::string &includerPath, const FilePath &includedPath);

    private:
	/// This map keeps all included paths for every includer that was parsed.
	/// This map is <includerPath, includedPath's>
	std::map<std::string, std::set<std::string>> mParsedIncludeDependencies;
    };

/// This parses a C++ source file, then saves important data into a file.
class CppParser
    {
    public:
	CppParser():
	    mClassifier(nullptr), mOperation(nullptr),
	    mCondStatements(nullptr)
	    {}
	enum eErrorTypes { ET_None, ET_CompileWarnings, ET_CompileErrors,
	    ET_NoSourceFile, ET_ParseError };
        /// Parses a C++ source file.
	eErrorTypes parse(char const * const srcFn, char const * const srcRootDir,
		char const * const outDir,
		char const * const clang_args[], int num_clang_args);
	/// These are public for access by global functions callbacks.
	CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitTranslationUnitForIncludes(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitRecord(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddArgs(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddVars(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddStatements(CXCursor cursor, CXCursor parent);

    private:
        /// This contains all parsed information.
	ModelData mModelData;
	ModelClassifier *mClassifier;      /// Current class being parsed.
	ModelOperation *mOperation;        /// Current operation being parsed.
	ModelCondStatements *mCondStatements;  /// Current conditional statement
	FilePath mTopParseFn;   /// The top level file that is being parsed.
	Visibility mClassMemberAccess;
	IncDirDependencyMap mIncDirDeps;
	ModelType *createOrGetBaseTypeRef(CXCursor cursor, RefType &rt);
	ModelClassifier *createOrGetClassRef(char const * const name);
	ModelType *createOrGetDataTypeRef(CXCursor cursor);
	void addOperationParts(CXCursor cursor, bool addParams);
	void addRecord(CXCursor cursor, Visibility vis);
    };


#endif /* CPPPARSER_H_ */
