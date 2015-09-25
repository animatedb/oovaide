// Call expressions


// Use:
//	clang -c -x c++ -Xclang -ast-dump -fsyntax-only test.h
namespace TestCallExpressions
{

class parent
    {
    public:
        parent()
            {}
        ~parent()
            {}
        parent(int x)
            { parentFunc(x); }
        int parentFunc(int x)
            { return x; }
    };

void globalFunc1()
    {
    }

void globalFunc2(parent c)
    {
	parent d = c;
	globalFunc1();
    }

static parent gparent;

class otherClass
    {
    public:
        otherClass(int x):
            mInt(x)
            {
            }
    private:
        int mInt;
    };

// AST is different than children from the CIndex interface.
class child:public parent
    {
    public:						// AccesssSpecDecl
        child():
            parent(3)	// non this member ref constructor
            {}

        void childFuncNonThisMemberRef()
            {
            // Call to parent class.
            // First child is:			CXCursor_MemberRefExpr, "parentFunc".
            // First grandchild is:		CXCursor_FirstInvalid
            parentFunc(-1);		//     CXXMemberCallExpr
				    //		 MemberExpr
				    //		   ImplicitCastExpr
				    //		     CXXThisExpr
				    //     Params start here...
            }

        void childFuncGlobal(parent *p)		// CXXMethodDecl
            {					//   CompoundStmt
            // Call to function.
            // First child is:			CXCursor_FirstExpr, "parentFunc".
            // First grandchild is:		CXCursor_DeclRefExpr, "parentFunc".
            globalFunc1();
            }

		void childFuncGlobalImplicit(parent *p)	// CXXMethodDecl
			{
            // Call to implicit conversion
            // First child is:			CXCursor_IntegerLiteral
            // First grandchild is:		CXCursor_FirstInvalid
            // Then should be same as function call.
            globalFunc2(0);		// Implicit conversion
			}

        void childFuncMemberRefs(parent *p)	// CXXMethodDecl
            {
            // Call to parameter method.
            // Has MemberExpr/ImplicitCastExpr/CXXThisExpr
            // KEY IS CXXMemberCallExpr has child "ImplicitCastExpr/CXXThisExpr"

            // First child is:			CXCursor_MemberRefExpr, "parentFunc".
            // First grandchild is:		CXCursor_DeclRefExpr, "p".
            p->parentFunc(1);	// Has MemberExpr/ImplicitCastExpr/DeclRefExpr

            // Call to member method.
            // Note that the mAttr is part of the CXXMemberCallExpr
            mAttr.parentFunc(2);	// Has MemberExpr/MemberExpr/CXXThisExpr

            // Call to global method.
            gparent.parentFunc(3);

            child::parentFunc(4);

            otherClass other(5);
            }

        void childFuncNonMembers()
            {
            parent parentDef;
            parentDef.parentFunc(parentDef.parentFunc(6));
            }

        void childFuncChild()
            {
            childFuncChild();
            }

    public:
        parent mAttr;
    };

class grandChild:public child
    {
    public:
        void grandChildFunc()
            {
            childFuncNonThisMemberRef();
            mAttr.parentFunc(0);
            getChild().mAttr.parentFunc(1);
            }

        void grandChildFuncNested()
            {
            mChild.mAttr.parentFunc(1);
            }

        child &getChild()
            { return mChild; }

    private:
        child mChild;
    };


};

/*
TranslationUnitDecl 0xc29df0 <<invalid sloc>> <invalid sloc>
|-TypedefDecl 0xc2a0e0 <<invalid sloc>> <invalid sloc> implicit __builtin_va_list 'char *'
|-CXXRecordDecl 0xc2a110 <test.h:5:1, line:10:2> line:5:7 referenced class parent definition
| 
|-CXXRecordDecl 0xc2a1e0 <col:1, col:7> col:7 implicit referenced class parent
| 
|-AccessSpecDecl 0xc2a240 <line:7:2, col:8> col:2 public
| `-CXXMethodDecl 0xc2a290 <line:8:3, line:9:5> line:8:8 used parentF 'void (void) __attribute__((thiscall))'
|   `-CompoundStmt 0xc2a318 <line:9:4, col:5>
`-CXXRecordDecl 0xc2a330 <line:12:1, line:27:2> line:12:7 class child definition
  |-public 'class parent'
  |-CXXRecordDecl 0xc2a450 <col:1, col:7> col:7 implicit class child
  |-AccessSpecDecl 0xc2a4b0 <line:14:2, col:8> col:2 public
  |-CXXMethodDecl 0xc2a550 <line:15:3, line:24:4> line:15:8 call 'void (class parent *) __attribute__((thiscall))'
  | |-ParmVarDecl 0xc2a4e0 <col:13, col:21> col:21 used p 'class parent *'
  | `-CompoundStmt 0xc2a770 <line:16:4, line:24:4>
  |   |-CXXMemberCallExpr 0xc2a668 <line:17:4, col:12> 'void'
  |   | `-MemberExpr 0xc2a648 <col:4> '<bound member function type>' ->parentF 0xc2a290
  |   |   `-ImplicitCastExpr 0xc2a688 <col:4> 'class parent *' <UncheckedDerivedToBase (parent)>
  |   |     `-CXXThisExpr 0xc2a638 <col:4> 'class child *' this
  |   |-CXXMemberCallExpr 0xc2a6e0 <line:21:4, col:15> 'void'
  |   | `-MemberExpr 0xc2a6bc <col:4, col:7> '<bound member function type>' ->parentF 0xc2a290
  |   |   `-ImplicitCastExpr 0xc2a6b0 <col:4> 'class parent *' <LValueToRValue>
  |   |     `-DeclRefExpr 0xc2a698 <col:4> 'class parent *' lvalue ParmVar 0xc2a4e0 'p' 'class parent *'
  |   `-CXXMemberCallExpr 0xc2a750 <line:23:4, col:18> 'void'
  |     `-MemberExpr 0xc2a730 <col:4, col:10> '<bound member function type>' .parentF 0xc2a290
  |       `-MemberExpr 0xc2a710 <col:4> 'class parent' lvalue ->mAttr 0xc2a5f0
  |         `-CXXThisExpr 0xc2a700 <col:4> 'class child *' this
  |-AccessSpecDecl 0xc2a5d0 <line:25:2, col:9> col:2 private
  `-FieldDecl 0xc2a5f0 <line:26:3, col:10> col:10 referenced mAttr 'class parent'
*/
