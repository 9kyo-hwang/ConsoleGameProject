#include "pch.h"
#include "GameLevel.h"
#include <Actor/Player.h>
#include <Actor/EnemySpawner.h>

void GameLevel::OnInitialized()
{
    Level::OnInitialized();

    SpawnActor<Player>();
    SpawnActor<EnemySpawner>();
}