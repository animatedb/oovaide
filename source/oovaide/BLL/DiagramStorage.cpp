/*
 * DiagramStorage.cpp
 *
 *  Created on: Jun 24, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramStorage.h"


char const *DiagramStorage::getDrawingTypeName(eDiagramStorageTypes fileType)
    {
    static char const *typeStrs[] = { "Component", "Include", "Zone", "Class",
            "Portion", "Sequence"
        };
    return typeStrs[fileType];
    }

void DiagramStorage::setDrawingHeader(NameValueFile &file,
        eDiagramStorageTypes drawingType, OovStringRef drawingName)
    {
    file.setNameValue("FileType", getDrawingTypeName(drawingType));
    file.setNameValue("DrawingName", drawingName);
    }

void DiagramStorage::getDrawingHeader(NameValueFile &file,
        eDiagramStorageTypes &drawingType, OovString &drawingName)
    {
    OovString fileTypeStr = file.getValue("FileType");
    for(int i=DST_FIRST_TYPE; i<DST_NUM_TYPES; i++)
        {
        if(fileTypeStr == getDrawingTypeName(static_cast<eDiagramStorageTypes>(i)))
            {
            drawingType = static_cast<eDiagramStorageTypes>(i);
            break;
            }
        }
    drawingName = file.getValue("DrawingName");
    }
