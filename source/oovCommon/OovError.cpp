/*
 * OovError.cpp
 *
 *  Created on: Sept. 18, 2015
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OovError.h"

class StdErrorListener:public OovErrorListener
    {
    public:
        void errorListener(OovStringRef str, OovErrorTypes et)
            {
            if(et == ET_Error)
                {
                fprintf(stderr, "%s\n", str.getStr());
                }
            else
                {
                printf("%s\n", str.getStr());
                }
            }
    };

StdErrorListener sStdListener;

OovErrorListener *sListener = &sStdListener;
OovErrorComponent sErrorComponent;

OovErrorListener::~OovErrorListener()
    {}

void OovError::setListener(OovErrorListener *listener)
    {
    sListener = listener;
    }

void OovError::setComponent(OovErrorComponent component)
    {
    sErrorComponent = component;
    }

char const *OovError::getComponentString()
    {
    char const *str = "";
    switch(sErrorComponent)
        {
        case EC_Oovaide:         str = "Oovaide";         break;
        case EC_OovBuilder:     str = "OovBuilder";     break;
        case EC_OovCMaker:      str = "OovCMaker";      break;
        case EC_OovCovInstr:    str = "OovCovInstr";    break;
        case EC_OovCppParser:   str = "OovCppParser";   break;
        case EC_OovDbWriter:    str = "OovDbWriter";    break;
        case EC_OovEdit:        str = "OovEdit";        break;
        }
    return str;
    }

void OovError::report(OovErrorTypes et, OovStringRef errStr)
    {
    if(sListener)
        {
        OovString err;
        err = errStr;
        err += '\n';
        sListener->errorListener(err, et);
        }
    }

static int sReportNeededErrorCount;
int OovStatus::mScopeCounter;

void OovStatus::set(bool success, OovStatusClass sc)
    {
    if(!success)
        {
        if(mStatus & RS_ReportNeeded)
            {
            report(ET_Error, "Error overwritten");
            }
        mStatus = sc | RS_ReportNeeded | sErrorComponent;
        addError(mStatus);
        }
    }

void OovStatus::reportStr(OovErrorTypes et, OovStringRef str)
    {
    OovString err = OovError::getComponentString();
    err += ":";
    err += str;
    OovError::report(et, err);
    }

void OovStatus::report(OovErrorTypes et, OovStringRef str)
    {
    reportStr(et, str);
    // report could be called even if no error was registered.
    if(mStatus & RS_ReportNeeded)
        {
        removeError(mStatus);
        }
    mStatus &= ~RS_ReportNeeded;
    }

void OovStatus::clearError()
    {
    // this could be called even if no error was registered.
    if(!ok())
        {
        removeError(mStatus);
        }
    mStatus = 0;
    }

void OovStatus::checkErrors()
    {
    if(sReportNeededErrorCount != 0)
        {
        reportStr(ET_Error, "Missing error reporting");
        sReportNeededErrorCount = 0;
        }
    }

void OovStatus::addError(int err)
    {
    sReportNeededErrorCount++;
    }

void OovStatus::removeError(int err)
    {
    sReportNeededErrorCount--;
    }
