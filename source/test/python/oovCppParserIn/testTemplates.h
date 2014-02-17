
#include <vector>

class templItem
	{
	public:
		int iVal;
	};

class templClassInherited:public std::vector<templItem>
	{
	};

class templClassHasMembers
	{
	std::vector<templItem> itemVector;
	std::vector<templItem*> itemPtrVector;

	void func2(int x) const
		{}
	void func()
		{
		std::vector<templItem> data;
		for(auto x : data)
			{
			func2(x.iVal);
			}
		}
	};
