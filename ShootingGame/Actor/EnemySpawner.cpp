#include "pch.h"
#include "EnemySpawner.h"
#include <Actor/Enemy.h>
#include <Math/MathUtility.h>   // 랜덤
#include <Level/Level.h>    // 액터 생성 요청을 위함
#include <vector>

using namespace Craft;

// 해당 파일 내에서만 존재하는, 적 생성 시 사용할 글자값
static std::vector<std::string> enemyTypes
{
    ";:^:;", "zZwZz", "oO@Oo", "<-=->", ")qOp(", "(oOo)"
};

EnemySpawner::EnemySpawner()
    : Super()
{
    // 적 생성 간격
    _timer.SetTargetTime(FMath::RandRange(1.5f, 3.f));
}

void EnemySpawner::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _timer.Tick(deltaTime);
    if (!_timer.IsTimeout())
    {
        return;
    }

    _timer.Reset();
    Spawn();
}

void EnemySpawner::Spawn()
{
    const uint64 count = enemyTypes.size();
    const int32 index = FMath::RandRange(0, count - 1);
    const int32 posY = FMath::RandRange(1, 10);

    GetOwner()->SpawnActor<Enemy>(enemyTypes[index], posY);
}
