#pragma once
#include <Actor/Pawn.h>

namespace Craft
{
    class BoxComponent;
    class SpriteRendererComponent;
    class Sprite;
}

class SwordAttack;
class Projectile;

class Player : public Pawn
{
    TYPE_DECLARATIONS(Player, Pawn)

public:
    Player(Craft::Vector2 position, int maxHp);

    void Tick(float deltaTime) override;

    inline bool HasSword() const { return _hasSword; }
    void EquipSword() { _hasSword = true; } // TODO: 확장

    inline Craft::Vector2 GetMovementInputDirection() const { return _moveInput; }

    bool Shieldable(const Projectile& projectile) const;
    bool IsAttacking() const;
    void SetActiveAttack(const std::shared_ptr<SwordAttack>& attack) { _activeAttack = attack; }
    void CancelAttack();

protected:
    void OnDeath(const std::shared_ptr<Pawn>& damageInstigator) override;

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::BoxComponent> _box;
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;
    std::weak_ptr<SwordAttack> _activeAttack;

    bool _hasSword = false;  // TEMP
    Craft::Vector2 _moveInput = Craft::Vector2::Zero;
};
