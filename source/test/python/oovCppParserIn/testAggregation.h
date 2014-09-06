
class classBase
    {
    public:
        void func(int &a);
    };

class classMultiLeaf
    {
    classMultiLeaf():
        classMultiLeaf_intMember(0)
        {}
    int classMultiLeaf_intMember;
    };

class classLeaf3a
    {
    classLeaf3a():
        classLeaf3a_leaf3aMember(0)
        {}
    int classLeaf3a_leaf3aMember;
    classMultiLeaf classLeaf3a_multiLeafMember;
    };

