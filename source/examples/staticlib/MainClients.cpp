// MainClients.cpp
#include "MainClients.h"
#include <stdio.h>

void Client::takeBag(BlackSheep &sheep)
    {
    WoolBag bag = sheep.getBag();
    printf("%s takes bag %s\n", getName(), bag.quantity());
    }

WoolWorld::WoolWorld()
    {
    master.takeBag(sheep);
    dame.takeBag(sheep);
    boy.takeBag(sheep);
    if(sheep.haveWool())
        printf("Still have wool\n");
    else
        printf("No wool left\n");
    }

int main(int argc, const char* argv[])
    {
    WoolWorld world;
    }
