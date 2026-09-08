#include "pch.h"
#include "Actor.h"

Actor::Actor(Z1::Protocol::ActorKind kind)
{
    info.kind = kind;
    info.id = sIdGenerator++;
}
