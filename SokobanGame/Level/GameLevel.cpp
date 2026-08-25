#include <pch.h>
#include "GameLevel.h"
#include <fstream>
#include <cassert>

#include <Math/Vector2.h>

#include <Actor/Box.h>
#include <Actor/Ground.h>
#include <Actor/Player.h>
#include <Actor/Target.h>
#include <Actor/Wall.h>

#include <Render/Renderer.h>

using namespace Craft;

void GameLevel::OnInitialized()
{
    Level::OnInitialized();

    // 레베 로드 완료 시 Map 로드
    LoadMap("Stage1.txt");
}

void GameLevel::Draw()
{
    Level::Draw();  // Level에서 액터 Draw 호출

    if (_isGameCleared)
    {
        Renderer::Get().Submit(Sprite::Create("Game Clear!"), Vector2(30, 0));
    }
}

bool GameLevel::CanMoveTo(const Vector2& from, const Vector2& to)
{
    if (_isGameCleared)
    {
        return false;
    }

    std::vector<std::shared_ptr<Actor>> boxes;
    for (const auto& actor : actors)
    {
        if (actor->IsA<Box>())
        {
            boxes.emplace_back(actor);
        }
    }

    std::shared_ptr<Actor> boxActor = nullptr;
    for (const auto& box : boxes)
    {
        if (box->GetPosition() == to)
        {
            boxActor = box;
            break;
        }
    }

    // 1. 이동하려는 위치에 박스가 있다면?
    if (boxActor)
    {
        // 박스 다음 위치 구하기
        // - 플레이어 방향 정보 필요: 다음 위치 - 현재 위치
        Vector2 direction = to - from;
        Vector2 posToBox = boxActor->GetPosition() + direction;

        /*
        * 원래 덧셈은 위치 + 위치 / 벡터 + 벡터만 가능
        * 하지만 게임 상 좌표 계산을 위해 위치(좌표) + 벡터(크기 + 방향)을 허용함
        * 이를 동차좌표계라 부름 | 어파인 변환
        * (x, y, w): w가 0이면 벡터, 1이면 위치 / (x, y, z, w)
        * 박스 위치의 경우 
        - to(x2, y2, 1) - from(x1, y1, 1) = direction(x2 - x1, y2 - y1, 0): 벡터
        - actor(x3, y3, 1) + direction(x2 - x1, y2 - y1, 0) = posToBox(x3 + x2 - x1, y3 + y2 - 1, 1): 위치
        - 그래서 벡터 + 벡터, 벡터 + 위치는 되지만 위치 + 위치는 w = 2가 되어 UB
        */


        // 다음 위치가 박스라면 이동 불가
        for (const auto& otherBox : boxes)
        {
            if (otherBox == boxActor)
            {
                continue;
            }

            if (otherBox->GetPosition() == posToBox)
            {
                return false;
            }
        }

        // 다음 위치가 벽이면 이동 불가, 나머지는 가능
        for (const auto& actor : actors)
        {
            if (actor->GetPosition() != posToBox)
            {
                continue;
            }

            if (actor->IsA<Wall>())
            {
                return false;
            }

            if (actor->IsA<Ground>() || actor->IsA<Target>())
            {
                boxActor->SetPosition(posToBox);

                _isGameCleared = IsGameCleared();

                return true;
            }
        }
    }

    // 2. 플레이어만 이동시키면 됨
    for (const auto& actor : actors)
    {
        if (actor->GetPosition() != to)
        {
            continue;
        }

        if (actor->IsA<Wall>())
        {
            return false;
        }

        // 땅이거나 타겟인 케이스
        return true;
    }

    return false;
}

void GameLevel::LoadMap(const std::string& filename)
{
    std::string path = "../Content/Stages/" + filename;

    // 파일에 아무런 작업을 하지 않고 binary로 오픈
    std::ifstream file(path, std::ios_base::binary);
    assert(file.is_open());

    // 파일 크기 확인: 0 위치에서 [end]만큼 offset
    file.seekg(0, std::ios_base::end);
    const std::streampos filesize = file.tellg();

    // 다시 커서 처음으로 옮기기: 0 위치에서 [begin] offset
    file.seekg(0, std::ios_base::beg);

    // 파일 내용을 담을 버퍼 추가
    std::string buffer{};
    buffer.resize((size_t)filesize);
    
    // 맵 데이터 특성 상 문자 하나하나가 의미가 있어
    // 통째로 복사해오고 확인하는 구조로 최적화를 꾀함
    file.read(buffer.data(), filesize);

    int32 index = 0;
    Craft::Vector2 position{};

    while (true)
    {
        if (index >= filesize)
        {
            break;
        }

        char ch = buffer[index++];
        if (ch == '\r') // Windows 개행 CLRF: \r\n -> \r 건너뛰기
        {
            continue;
        }

        if (ch == '\n')
        {
            ++position.y;
            position.x = 0;
            continue;
        }

        switch (ch)
        {
        case '#':
        {
            // 벽
            SpawnActor<Wall>(position);
            break;
        }
        case '.':
        {
            // 빈 공간
            SpawnActor<Ground>(position);
            break;
        }
        case 'p':
        {
            // 플레이어: 박스와 함께 이동 가능한 물체라 이동한 자리엔 Ground가 남음
            SpawnActor<Ground>(position);
            SpawnActor<Player>(position);
            break;
        }
        case 'b':
        {
            // 박스
            SpawnActor<Ground>(position);
            SpawnActor<Box>(position);
            break;
        }
        case 't':
        {
            // 목표지점
            SpawnActor<Target>(position);
            ++_targetScore;
            break;
        }
        default: break;
        }

        ++position.x;
    }

    file.close();
}

bool GameLevel::IsGameCleared()
{
    int32 currentScore = 0;

    // 박스 위치 == 타겟 위치 개수
    std::vector<std::shared_ptr<Actor>> boxes;
    std::vector<std::shared_ptr<Actor>> targets;

    for (const auto& actor : actors)
    {
        if (actor->IsA<Box>())
        {
            boxes.emplace_back(actor);
        }
        else if (actor->IsA<Target>())
        {
            targets.emplace_back(actor);
        }
    }

    for (const auto& box : boxes)
    {
        for (const auto& target : targets)
        {
            if (box->GetPosition() == target->GetPosition())
            {
                ++currentScore;
            }
        }
    }

    return currentScore == _targetScore;
}
