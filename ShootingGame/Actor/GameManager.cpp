#include "pch.h"
#include "GameManager.h"

using namespace Craft;

void GameManager::SetPlayerDead(const Craft::Vector2& position)
{
    if (_isPlayerDead)
    {
        return;
    }

    _isPlayerDead = true;
    _playerDeadPosition = position;

    if (_onPlayerDead)
    {
        _onPlayerDead();
    }
}
