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

OovErrorListener::~OovErrorListener()
    {}

void OovError::setListener(OovErrorListener *listener)
    {
    sListener = listener;
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

