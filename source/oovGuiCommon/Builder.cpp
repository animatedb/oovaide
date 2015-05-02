/*
 * Builder.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Builder.h"
#include "FilePath.h"

static Builder *gBuilder;

void Builder::init()
    {
    if(!gBuilder)
	mGtkBuilder = gtk_builder_new();
    gBuilder = this;
    }

Builder *Builder::getBuilder()
    {
    return gBuilder;
    }

bool Builder::addFromFile(OovStringRef const fn)
    {
    GError *err = nullptr;
    init();
    if(gtk_builder_add_from_file(mGtkBuilder, fn, &err) == 0)
	{
	FilePath layoutPath("/usr/local/bin", FP_Dir);
	layoutPath.appendFile(fn);
	err = nullptr;
	gtk_builder_add_from_file(mGtkBuilder, layoutPath.getStr(), &err);
	}
    return(!err);
    }
