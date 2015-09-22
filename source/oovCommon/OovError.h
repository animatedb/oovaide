/*
 * OovError.h
 *
 *  Created on: Sept. 18, 2015
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OovString.h"

enum OovErrorTypes { ET_Error, ET_Info, ET_Diagnostic };

class OovErrorListener
    {
    public:
        virtual void errorListener(OovStringRef str, OovErrorTypes et) = 0;
        virtual ~OovErrorListener();
    };

class OovError
    {
    public:
        /// If a listener is not set, this defaults to using stdout and stderr.
        static void setListener(OovErrorListener *listener);
        static void report(OovErrorTypes et, OovStringRef str);
    };
