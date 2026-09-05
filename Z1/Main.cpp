#include "pch.h"
#include <Game/Game.h>
#include <Windows.h>

int main()
{
    SetConsoleTitleA("The Legend of Zelda");

    Game game;
    game.Run();

    return 0;
}