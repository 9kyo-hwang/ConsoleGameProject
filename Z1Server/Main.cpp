#include "pch.h"
#include <Sockets/Runtime.h>
#include <Server.h>
#include <iostream>

int main()
{
    Net::Runtime runtime;
    if (!runtime.IsValid())
    {
        return 1;
    }

    Server server;
    if (!server.Start(7777))
    {
        return 1;
    }

    server.WaitForShutdown();

    return 0;
}