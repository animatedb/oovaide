// ModelTypeRefs.java
// Created on: Oct 19, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.ArrayList;
import java.util.Iterator;

// Lists all relations between types/classes including
// inheritance(implements or extends), and aggregation.
public class ModelTypeRefs implements Iterable<ModelTypeRef>
    {
    public ModelTypeRefs()
        {
        mTypeRefs = new ArrayList<ModelTypeRef>();
        }

    public void addTypeRef(ModelTypeRef ref)
        { mTypeRefs.add(ref); }

    public ArrayList<ModelTypeRef> getTypeRefs()
        { return mTypeRefs; }

    public Iterator<ModelTypeRef> getTypeRefsIter()
        { return mTypeRefs.iterator(); }

    public Iterator<ModelTypeRef> iterator()
        { return getTypeRefsIter(); }

    ArrayList<ModelTypeRef> mTypeRefs;
    }
