// ModelTypeRelations.java
// Created on: Oct 19, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.HashSet;
import java.util.Iterator;

// Lists all relations between types/classes including
// inheritance(implements or extends), and aggregation.
public class ModelTypeRelations implements Iterable<ModelTypeRelation>
    {
    public ModelTypeRelations()
        {
        super();
        mTypeRelations = new HashSet<ModelTypeRelation>();
        }

    public void addRelation(ModelTypeRelation rel)
        { mTypeRelations.add(rel); }

    public Iterator<ModelTypeRelation> getTypes()
        { return mTypeRelations.iterator(); }

    public Iterator<ModelTypeRelation> iterator()
        { return getTypes(); }

    HashSet<ModelTypeRelation> mTypeRelations;
    }
