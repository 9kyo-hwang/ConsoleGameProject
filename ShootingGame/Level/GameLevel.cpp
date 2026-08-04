#include "pch.h"
#include "GameLevel.h"
#include <Actor/Player.h>

void GameLevel::OnInitialized()
{
    Level::OnInitialized();

    SpawnActor<Player>();
}