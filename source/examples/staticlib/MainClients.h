// MainClients.h
#include "BlackSheep.h"

class Client
    {
    public:
        virtual char const * const getName() = 0;
        void takeBag(BlackSheep &sheep);
    };

class Master:public Client
    {
    public:
        virtual char const * const getName()
            { return "Master"; }
    };

class Dame:public Client
    {
    public:
        virtual char const * const getName()
            { return "Dame"; }
    };

class LittleBoy:public Client
    {
    public:
        virtual char const * const getName()
            { return "LittleBoy"; }
    };

class WoolWorld
    {
    public:
        WoolWorld();

    private:
    	BlackSheep sheep;
    	Master master;
    	Dame dame;
    	LittleBoy boy;
    };
