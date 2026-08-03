#include <Engine/Engine.h>
#include <Level/GameLevel.h>

using namespace Craft;

class A
{
public:
    void Test() {}
};

int main()
{
    Engine engine;
    engine.AddNewLevel<GameLevel>();
    engine.Run();

    //A* a = nullptr;
//a->Test();

// 위 호출은 C언어 단에서 Test(this) 형태
// this == nullptr인 상황이고, this를 접근하는 케이스가 없으니 실행도 됨
// 하지만 this에 접근하는 순간 터짐

// 함수 호출 / 호출 종료 시 스택 메모리의 변화(호출 규약, stdcall vs cdecl)

    return 0;
}