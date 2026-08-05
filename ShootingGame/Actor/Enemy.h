#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class Enemy : public Craft::Actor
{
    TYPE_DECLARATIONS(Enemy, Craft::Actor)

    enum class MoveDirection
    {
        Left = -1,
        Right = 1
    };
public:
    // x좌표는 random하게
    Enemy(const std::string& image = "(o0o)", int posY = 5);
    ~Enemy() override = default;

private:
    void Tick(float deltaTime) override;
    void OnCollision(const std::shared_ptr<Actor>& other) override;

private:
    MoveDirection _dir = MoveDirection::Left;
    float _posX = 0.f;
    float _moveSpeed = 5.f;
    Timer _timer;   // 적이 탄환을 발사하는 간격
};

