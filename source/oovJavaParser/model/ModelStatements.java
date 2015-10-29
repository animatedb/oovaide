// ModelStatements.java
// Created on: Oct 21, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.ArrayList;
import java.util.Iterator;

public class ModelStatements implements Iterable<ModelStatement>
    {
    public ModelStatements()
        {
        super();
        mStatements = new ArrayList<ModelStatement>();
        }

    public void addStatement(ModelStatement statement)
        { mStatements.add(statement); }

    public ArrayList<ModelStatement> getStatements()
        { return mStatements; }

    public Iterator<ModelStatement> getStatementIter()
        { return mStatements.iterator(); }

    public Iterator<ModelStatement> iterator()
        { return getStatementIter(); }

    ArrayList<ModelStatement> mStatements;
    }
