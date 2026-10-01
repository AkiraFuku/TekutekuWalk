#include "GameObject.h"
#include "Collider.h"

GameObject::~GameObject() = default;

void GameObject::SetCollider(std::unique_ptr<Collider> collider)
{
    collider_ = std::move(collider);
}
