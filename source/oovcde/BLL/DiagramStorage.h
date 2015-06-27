/*
 * DiagramStorage.h
 *
 *  Created on: Jun 24, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef DIAGRAMSTORAGE_H_
#define DIAGRAMSTORAGE_H_

#include "NameValueFile.h"

enum eDiagramStorageTypes
    {
    // WARNING: To rearrange these, see DiagramStorage::getDrawingTypeName
    DST_FIRST_TYPE=0, DST_Component=DST_FIRST_TYPE, DST_Include, DST_Zone, DST_Class, DST_Portion,
    DST_Sequence, DST_NUM_TYPES
    };

class DiagramStorage
    {
    public:
        static void getDrawingHeader(NameValueFile &file,
                eDiagramStorageTypes &fileType, OovString &drawingName);
        static void setDrawingHeader(NameValueFile &file,
                eDiagramStorageTypes fileType, OovStringRef drawingName);

    private:
        static char const *getDrawingTypeName(eDiagramStorageTypes fileType);
    };


#endif /* DIAGRAMSTORAGE_H_ */
