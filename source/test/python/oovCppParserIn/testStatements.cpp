
class testStatementsClass
	{
	public:
		void testStatementsFunc();
		void calledFunc(int x)
			{}
	};

void testStatementsClass::testStatementsFunc()
	{
	int data[] = { 1, 2 };
	int data2[] = { 1, 2 };
	int val = 0;
	int x=0;
	for(; x<sizeof(data); x++)
		{
		if(x & val)
			{
			// This doesn't end up in the output because
			// there are no interesting relations or calls.
			for(auto &i : data)
				{
				val += x * i;
				}
			for(auto &i2 : data2)
				{
				calledFunc(i2);
				}
			}
		val++;
		}
	if(x & val)
		x++;
	}
