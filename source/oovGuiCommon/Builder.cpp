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

Builder::Builder():
    mGtkBuilder(nullptr)
    {
    sBuilder = this;
    }

Builder *Builder::getBuilder()
    {
    if(!sBuilder->mGtkBuilder)
        sBuilder->mGtkBuilder = gtk_builder_new();
    return sBuilder;
    }

bool Builder::addFromFile(OovStringRef const fn)
    {
    GError *err = nullptr;
    OovString path = Project::getBinDirectory();
    path += fn;
    getBuilder();
    gtk_builder_add_from_file(mGtkBuilder, path.getStr(), &err);
    return(!err);
    }
