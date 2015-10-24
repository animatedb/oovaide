// ModelData.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.ArrayList;
import java.util.Iterator;


public class ModelData implements Iterable<ModelType>
    {
        public ModelData()
            {
            mTypes = new ArrayList<ModelType>();
            }

        public void setModuleName(String moduleName)
            { mModuleName = moduleName; }

        public void addType(ModelType type)
            { mTypes.add(type); }

        public void setPackage(String packageName)
            { mPackageName = packageName; }

        public String getModuleName()
            { return mModuleName; }

        public ModelType findType(String name)
            {
            for(int i=0; i<mTypes.size(); i++)
                {
                ModelType type = mTypes.get(i);
                if(type.getTypeName().compareTo(name) == 0)
                    {
                    return type;
                    }
                }
            return null;
            }

        public String getPackage()
            { return mPackageName; }

        public Iterator<ModelType> getTypes()
            { return mTypes.iterator(); }

        public Iterator<ModelType> iterator()
            { return getTypes(); }

        String mModuleName;
	ArrayList<ModelType> mTypes;
        String mPackageName;
    };
