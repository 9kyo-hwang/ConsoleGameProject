#pragma once

// 설정된 초 카운팅
class Timer
{
public:
    Timer(float targetTime = 1.f);

    void Tick(float deltaTime);
    inline void Reset() { _elapsedTime = 0.f; }
    inline void SetTargetTime(float time) { _targetTime = time; }
    inline bool IsTimeout() const { return _elapsedTime >= _targetTime; }

private:
    float _elapsedTime = 0.f;
    float _targetTime = 0.f;
};

