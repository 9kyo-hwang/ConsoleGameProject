#include <pch.h>
#include "GameLevel.h"
#include <fstream>

#include <Math/Vector2.h>

#include <Actor/Box.h>
#include <Actor/Ground.h>
#include <Actor/Player.h>
#include <Actor/Target.h>
#include <Actor/Wall.h>

void GameLevel::OnInitialized()
{
    Level::OnInitialized();

    // 레베 로드 완료 시 Map 로드
    LoadMap("Map.txt");
}

void GameLevel::Draw()
{
    Level::Draw();  // Level에서 액터 Draw 호출
}

void GameLevel::LoadMap(const std::string& filename)
{
    std::string path = "../Assets/Stages/" + filename;

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
