#include "pch.h"
#include "Timer.h"

Timer::Timer(float targetTime)
    : _targetTime(targetTime), _elapsedTime(0.f)
{
}

void Timer::Tick(float deltaTime)
{
    _elapsedTime = std::clamp<float>(_elapsedTime + deltaTime, 0.f, _targetTime);
}
