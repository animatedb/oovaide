/*
 * Builder.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Builder.h"
#include "FilePath.h"
#include "Project.h"

static Builder *sBuilder;

void Builder::init()
    {
    if(!sBuilder)
	mGtkBuilder = gtk_builder_new();
    sBuilder = this;
    }

Builder *Builder::getBuilder()
    {
    return sBuilder;
    }

bool Builder::addFromFile(OovStringRef const fn)
    {
    GError *err = nullptr;
    init();
    OovString path = Project::getBinDirectory();
    path += fn;
    gtk_builder_add_from_file(mGtkBuilder, path.getStr(), &err);
    return(!err);
    }
