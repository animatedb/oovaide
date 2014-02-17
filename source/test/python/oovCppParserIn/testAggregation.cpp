
#include "testAggregation.h"

// root -> leaf2a -> int
//                -> leaf3a      -> int
//                               -> multiLeaf
//                -> multiLeaf

class classLeaf2a
	{
		int classLeaf2a_intMember;
	public:
		classLeaf3a classLeaf2a_leaf3aMember;
	private:
		const classMultiLeaf *classLeaf2a_multiLeafMember;
	};

class classRootAggr:public classBase
	{
	classLeaf2a classRoot_leaf2aMember;
	};

void classBase::func(int &a)
	{
	if(a==0)
		{
		int x=1;
		func(x);
		}
	}
