/*
 * DuplicatesView.h
 *
 *  Created on: Jun 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef DUPLICATESVIEW_H_
#define DUPLICATESVIEW_H_

#include "Duplicates.h"

enum eDupReturn { DR_Success, DR_NoDupFilesFound, DR_UnableToCreateDirectory };
eDupReturn createDuplicatesFile(/*OovStringRef const projDir,*/ std::string &outFn);

#endif /* DUPLICATESVIEW_H_ */
