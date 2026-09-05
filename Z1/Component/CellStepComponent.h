#pragma once

#include <Component/ActorComponent.h>

class CellStepComponent : public Craft::ActorComponent
{
    TYPE_DECLARATIONS(CellStepComponent, Craft::ActorComponent)

public:
    explicit CellStepComponent(float cellsPerSecond);
    ~CellStepComponent() override = default;

    int ConsumeCellSteps(float deltaTime);
    void ResetRemainder();

    void SetCellsPerSecond(float cellsPerSecond);

private:
    float _cellsPerSecond = 0.f;
    float _remainder = 0.f;
};
