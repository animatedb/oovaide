

class baseClass
{
  public:
	baseClass():
		mBaseProtMem(0)
		{}
	int mBasePubMem;
	void baseFunc();
	void baseInlineFunc(int iparam1)
		{
		int y;
		}
	bool constBaseInlineFunc(int iparam1) const
		{
		int y;
		return true;
		}
  protected:
	int *mBaseProtMem;
  private:
	const int *mBasePrivMem;
	class NestedClass
		{
		int mNestedMem;
		};
};

class protDerive1Class:protected baseClass
{
   public:
	const baseClass mDerive1PubMem;
	void deriveFunc1(baseClass *pparam1, int *p2);
	void deriveFunc2(const baseClass &rparam1, int i2);
	void deriveFunc3(const baseClass *&prparam1);
	void deriveFunc4(baseClass param1);
};

class protDerive2Class:private baseClass
{
private:
	void derive2Func1(const protDerive1Class &pparam1)
		{
		protDerive1Class d1;
		d1.deriveFunc2(*this, 0);
		baseInlineFunc(0);
		}
};

class pubDerive3Class:public baseClass
{
};

void baseClass::baseFunc()
{
    mBasePubMem = 4 + 5;
}

void protDerive1Class::deriveFunc1(baseClass *pparam1, int *p2)
{
    int g = pparam1->mBasePubMem + *p2;
}

void protDerive1Class::deriveFunc2(const baseClass &rparam1, int i2)
{
	if(rparam1.constBaseInlineFunc(1))
		{
		rparam1.constBaseInlineFunc(2);
		}
	else
		{
		rparam1.constBaseInlineFunc(3);
		}
	if(rparam1.constBaseInlineFunc(4))
		int x=0;
	if(i2 > 0)
		{
	    int g = rparam1.mBasePubMem + i2;
		pubDerive3Class pd3c;
		rparam1.constBaseInlineFunc(i2);
		baseInlineFunc(i2);
		for(int i=(i2==3); i<i2; i++)
			{
			baseInlineFunc(i);
			}
		}
	int x=0;
	for(; x<i2; x++)
		{}
	if(i2 & 0)
		i2++;
}

void protDerive1Class::deriveFunc3(const baseClass *&prparam1)
{
	int data[] = { 1, 2 };
	for(auto &i : data)
		{
		baseInlineFunc(i);
		}
    prparam1 = &mDerive1PubMem;
}

void protDerive1Class::deriveFunc4(baseClass param1)
{
}
