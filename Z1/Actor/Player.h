#pragma once
#include <Actor/Pawn.h>

namespace Craft
{
    class BoxComponent;
    class SpriteRendererComponent;
    class Sprite;
}

class SwordAttack;

class Player : public Pawn
{
    TYPE_DECLARATIONS(Player, Pawn)

public:
    Player(Craft::Vector2 position, int maxHp);

    int ConsumeMoveSteps(float deltaTime);
    void ClearMoveRemainder();
    void MoveBy(const Craft::Vector2& delta);

    inline bool HasSword() const { return _hasSword; }
    void EquipSword() { _hasSword = true; } // TODO: 확장

    bool IsAttacking() const;
    void SetActiveAttack(const std::shared_ptr<SwordAttack>& attack) { _activeAttack = attack; }
    void CancelAttack();

protected:
    void OnDeath(const std::shared_ptr<Craft::Actor>& damageInstigator) override;

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::BoxComponent> _box;
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;
    std::weak_ptr<SwordAttack> _activeAttack;

    float _moveSpeed = 20.f;    // 초당 셀 20칸
    float _moveRemainder = 0.f;

    bool _hasSword = true;  // TEMP
};
