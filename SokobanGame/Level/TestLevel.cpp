#include <pch.h>
#include "TestLevel.h"
#include "../Actor/TestActor.h"

void TestLevel::OnInitialized()
{
    Level::OnInitialized();

    auto actor = SpawnActor<TestActor>();
}
