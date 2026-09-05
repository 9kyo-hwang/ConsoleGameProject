#pragma once

struct Timer
{
    Timer(float time) : _target(time), _elapsed(0.f) {}

    inline void Tick(float deltaTime) { _elapsed += deltaTime; }
    
    inline float ElapsedTime() const { return _elapsed; }
    inline bool TimeOver() const { return _elapsed >= _target; }

    inline void Set(float time) { _target = time; Reset(); } // 목표 시간 설정 + Reset도 같이.
    inline void Reset() { _elapsed = 0.f; }
    inline void Complete() { _elapsed = _target; }

private:
    float _target;
    float _elapsed;
};