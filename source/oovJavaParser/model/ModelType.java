// ModelType.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.HashSet;
import java.util.Iterator;


public class ModelType implements Iterable<ModelMethod>
    {
        public ModelType()
            {
            super();
            mMethods = new HashSet<ModelMethod>();
            mRelations = new ModelTypeRelations();
            mMemberVars = new ModelTypeRefs();
            mModule = false;
            }

        public void setTypeName(String typeName)
            { mTypeName = typeName; }

        public String getTypeName()
            { return mTypeName; }

        public void addMethod(ModelMethod method)
            { mMethods.add(method); }

        public void addRelation(ModelTypeRelation rel)
            { mRelations.addRelation(rel); }

        public void setModule()
            { mModule = true; }

        public boolean isDefined()
            {
            return mModule;
//            return ((mMemberVars.size() > 0) || (mRelations.size() > 0) ||
//                (mMethods.size() > 0));
            }


        public ModelTypeRelations getRelations()
            { return mRelations; }

        // Use this to add member variables.
        public ModelTypeRefs getMemberVars()
            { return mMemberVars; }

        public Iterator<ModelMethod> getMethods()
            { return mMethods.iterator(); }

        public boolean getModule()
            { return mModule; }

        public Iterator<ModelMethod> iterator()
            { return getMethods(); }

        String mTypeName;
	HashSet<ModelMethod> mMethods;
        ModelTypeRelations mRelations;
        ModelTypeRefs mMemberVars;
        boolean mModule;
    };
