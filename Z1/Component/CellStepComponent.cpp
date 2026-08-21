#include "pch.h"
#include "CellStepComponent.h"

CellStepComponent::CellStepComponent(float cellsPerSecond)
    : _cellsPerSecond(cellsPerSecond)
{
}

int CellStepComponent::ConsumeCellSteps(float deltaTime)
{
    _remainder += _cellsPerSecond * deltaTime;

    const int steps = static_cast<int>(_remainder);
    _remainder -= steps;

    return steps;
}

void CellStepComponent::ResetRemainder()
{
    _remainder = 0.f;
}

void CellStepComponent::SetCellsPerSecond(float cellsPerSecond)
{
    _cellsPerSecond = cellsPerSecond;
}
