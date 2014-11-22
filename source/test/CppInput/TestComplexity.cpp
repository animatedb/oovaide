
class ComplexityControlTest
    {
    public:
        void testTwoIf(bool c1, bool c2);
        void testIfElse(bool c1);
        void testIfElseIf(bool c1, bool c2);
        void testNestedIf(bool c1, bool c2);
        void testThreeIf(bool c1, bool c2, bool c3);
        void testThreeIfSame(int v1);
        void testCase(int v1);
        void testCaseAndDefault(int v1);
        void testIfOr();
        void testIfAnd();

    private:
        void a(){}
        void b(){}
        void c(){}
        void d(){}
        int cond1()
            { return 0; }
        int cond2()
            { return 0; }
    };


class ComplexityDataChild
    {
    public:
        void funcVal(int x){}
        void funcRef(int &x){}
        int funcRet(){ return 1; }
    };

class ComplexityDataTest
    {
    public:
        int testInputBoolParam(bool v1);
        int testInputUnsignedParam(unsigned int v1);
        int testInputSignedParam(int v1);
        int testInputPointerParam(int *p1);
        int testReadMemberRef();
        void testWriteMemberRef();
        int testReadPointerMemberRef();
        void testWritePointerMemberRef();
        void testMemberFuncVal();
        int testMemberFuncRef();
        int testMemberFuncRet();

    private:
        int mMember1;
        ComplexityDataChild mChild;
    };


void ComplexityControlTest::testTwoIf(bool c1, bool c2)
    {
    if(c1)
        {
        a();
        }
    if(c2)
        {
        b();
        }
    }

void ComplexityControlTest::testIfElse(bool c1)
    {
    if(c1)
        {
        a();
        }
    else
        {
        b();
        }
    }

void ComplexityControlTest::testIfElseIf(bool c1, bool c2)
    {
    if(c1)
        {
        a();
        }
    else if(c2)
        {
        b();
        }
    }

void ComplexityControlTest::testNestedIf(bool c1, bool c2)
    {
    if(c1)
        {
        a();
        if(c2)
            {
            b();
            }
        }
    }

void ComplexityControlTest::testThreeIf(bool c1, bool c2, bool c3)
    {
    if(c1)
        {
        a();
        }
    if(c2)
        {
        b();
        }
    if(c3)
        {
        c();
        }
    }

#define DEF_ONE 1

void ComplexityControlTest::testThreeIfSame(int v1)
    {
    enum { ENUM_ONE=1 };
    const int CONST_ONE=1;
    if(v1 == DEF_ONE)
        {
        a();
        }
    if(v1 == ENUM_ONE)
        {
        b();
        }
    if(v1 == CONST_ONE)
        {
        c();
        }
    }

void ComplexityControlTest::testCase(int v1)
    {
    switch(v1)
        {
        case 1:
            a();
            break;

        case 2:
        case 3:
            b();
            break;
        }
    }

void ComplexityControlTest::testCaseAndDefault(int v1)
    {
    switch(v1)
        {
        case 1:
            a();
            break;

        case 2:
            b();
        case 3:
            c();
            break;

		default:
            d();
            break;
        }
    }

void ComplexityControlTest::testIfOr()
    {
    if(cond1() || cond2())
        a();
    }

void ComplexityControlTest::testIfAnd()
    {
    if(cond1() && cond2())
        a();
    }

int ComplexityDataTest::testInputBoolParam(bool v1)
    {
    return !v1;
    }

int ComplexityDataTest::testInputUnsignedParam(unsigned int v1)
    {
    return v1 / 2;
    }

int ComplexityDataTest::testInputSignedParam(int v1)
    {
    return v1 / 2;
    }

int ComplexityDataTest::testInputPointerParam(int *p1)
    {
    return *p1 / 2;
    }

int ComplexityDataTest::testReadMemberRef()
    {
    return mMember1 / 2;
    }

void ComplexityDataTest::testWriteMemberRef()
    {
    mMember1 = 8;
    }

int ComplexityDataTest::testReadPointerMemberRef()
    {
    const int *p = &mMember1;
    return *p;
    }

void ComplexityDataTest::testWritePointerMemberRef()
    {
    int *p = &mMember1;
    *p = 8;
    }

void ComplexityDataTest::testMemberFuncVal()
    {
    mChild.funcVal(1);
    }

int ComplexityDataTest::testMemberFuncRef()
    {
    int val;
    mChild.funcRef(val);
    return val;
    }

int ComplexityDataTest::testMemberFuncRet()
    {
    return mChild.funcRet();
    }
