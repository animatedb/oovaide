/*
 * Duplicates.h
 * Created on: Feb 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "OovString.h"

enum eDupReturn { DR_Success, DR_NoDupFilesFound, DR_UnableToCreateDirectory };
eDupReturn createDuplicatesFile(/*OovStringRef const projDir,*/ std::string &outFn);
