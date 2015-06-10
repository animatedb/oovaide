// This file is for testing the c++ parser for different code flow constructs.
// This can be used for complexity calculations or for sequence diagrams.

class Funcs
    {
    public:
    void funcDo() {}
    void funcWhile() {}
    void funcIf() {}
    void funcElse() {}
    void funcElseIf() {}
    void funcFor() {}
    void funcForRange() {}
    void funcCase() {}
    void funcDefault() {}
    void funcNested() {}
    };

class FlowTest
    {
    void flowIfEmpty(int a, int b);
    void flowIfExprEmpty(int a, int b);
    void flowIfEmptyFunc(int a, int b, Funcs &funcs);
    void flowIfCompound(int a, int b, Funcs &funcs);
    
    void flowLoopsEmpty(int a, int b);
    void flowLoopsCompound(int a, int b, Funcs &funcs);

    void flowSwitch(int a, int b);
    void flowSwitchFunc(int a, int b, Funcs &funcs);
    void statementNested();
    int intStatement(int x)
        { return x; }
    };

void FlowTest::flowIfEmpty(int a, int b)
    {
    if(a)
        a--;
    if(a)
        a++;
    else if(a>1)
        a++;
    else if(a>2)
        a++;
    else if(a>3)
        a++;

    if(b)
        b++;
    else if(b>1)
        a++;
    else if(b>2)
        a++;
    else
        a++;
}

void FlowTest::flowIfExprEmpty(int a, int b)
    {
	int vec[10];
    if(a)
        vec[0] = a;
    if(vec[0])
        a++;
    else if(b>1)
        flowIfEmpty(a, b);
    else if(b>2)
        a++;
    }

void FlowTest::flowIfEmptyFunc(int a, int b, Funcs &funcs)
    {
	int vec[10];
    if(a)
        funcs.funcIf();
    if(vec[0])
        funcs.funcIf();
    else if(b>1)
        funcs.funcElseIf();
    else if(b>2)
        funcs.funcElseIf();
    else if(b>3)
        funcs.funcElseIf();
    }

void FlowTest::flowIfCompound(int a, int b, Funcs &funcs)
    {
    if(a > 1)
        {
        a--;
        funcs.funcIf();
        }
    else
        {
        funcs.funcElse();
        }
    if(a > 2)
        {
        a++;
        funcs.funcIf();
        if(b > 1)
            {
            funcs.funcNested();
            }
        else
            {
            funcs.funcNested();
            }
        }
    else if(a > 3)
        {
        funcs.funcElseIf();
        a++;
        if(b > 2)
            {
            funcs.funcNested();
            }
        else
            {
            funcs.funcNested();
            }
        }
    else if(a > 4)
        {
        funcs.funcElseIf();
        a++;
        }
    }

void FlowTest::flowLoopsEmpty(int a, int b)
    {
    char vec[5];
    for(int x=0; x<10; x++)
        a--;
    for(auto &item : vec)
        item++;
    while(a)
        a--;
    }

void FlowTest::flowLoopsCompound(int a, int b, Funcs &funcs)
    {
    char vec[5];
    while(a)
        {
        funcs.funcWhile();
        a--;
        }
    for(int x=0; x<10; x++)
        {
        funcs.funcFor();
        a--;
        }
    for(auto &item : vec)
        {
        funcs.funcForRange();
        item++;
        }
    do
        {
        funcs.funcDo();
        a++;
        } while(a>5);
    }

/*
void FlowTest::flowSwitch(int a, int b)
    {
    switch(a)
        {
        case 4:
            break;      // CXCursor_BreakStmt child of case 4

        case 5:
            b++;        // CXCursor_UnaryOperator
        case 6:
        case 7:         // CXCursor_CaseStmt child of 6?
            b++;
        break;

        case 8:
            if(a>5)
                b=3;
            else
                b=4;
            break;

        case 9:
        default:
            a++;
            break;
        }
    }
*/

void FlowTest::flowSwitchFunc(int a, int b, Funcs &funcs)
    {
    switch(a)
        {
        case 5:
            b++;
            funcs.funcCase();
        case 6:
        case 7:
            b++;
            funcs.funcCase();
        break;

        case 8:
            if(a>5)
                {
                b=4;
            	funcs.funcCase();
                }
            else
                {
                b=3;
            	funcs.funcCase();
                }
            break;

        case 9:
            if(a>7)
		{
                b = 4;
            	funcs.funcIf();
		}
            else
		{
                b = 6;
            	funcs.funcElse();
		}
            break;

	case 10:
            switch(b)
                {
                case '4':
                    a = 5;
            	    funcs.funcCase();
                    break;
                }
            break;

        case 11:
        default:
            a++;
            funcs.funcDefault();
            break;

        case 12:
        case 13:
            if(a>9)
                {
                b = 7;
        	    funcs.funcCase();
                }
            else
                {
                b = 8;
        	    funcs.funcCase();
                }
            break;

		case 14:
    	    funcs.funcCase();
			return;

		case 15:
			if(b > 5)
				{
	    	    funcs.funcCase();
				return;
				}

		case 16:	// Note that a==15 falls through to here in some cases.
    	    funcs.funcCase();
			return;
		}
    }

// Test statements in the expressions of loops.
void FlowTest::statementNested()
    {
    do
        {
        while(intStatement(1) >= 1)
            {
            char vec[5];
            for(int i=0; i<intStatement(2); i++)
                {
                vec[i] = 0;
                }
            }
        } while(intStatement(3) >= 3);
    }
