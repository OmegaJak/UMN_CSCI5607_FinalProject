#include <SDL.h>
#include <cmath>
#include "constants.h"
#include "controller.h"
#include "key.h"
#include "map.h"

Key::Key(Model* model, Map* map, char id, glm::vec2 pos) : GameObject(model, map) {
    id_ = id;

    bounding_box_vertices_ = bounding_box_->GetBoxVertices();  // This has to happen before we translate the key
    transform->Translate(glm::vec3(pos.x, pos.y, 0));
    InitTransform();
}

void Key::Update() {
    Door* door = map_->IntersectsDoorWithId(this, id_);
    if (holder_ != nullptr && door != nullptr) {
        door->GoAway();
        GoAway();
        holder_->UseKey();
        holder_ = nullptr;
    }

    if (holder_ != nullptr) {
        InitBoundingBox(bounding_box_vertices_);
        bounding_box_->transform->ClearParent();
    }

    GameObject::Update();
}

void Key::GoAway() {
    transform->ClearParent();
    transform->ResetAndSetTranslation(glm::vec3(0, 0, -3));
}

void Key::SetHolder(Controller* player) {
    holder_ = player;
}

void Key::Drop() {
    holder_ = nullptr;
    InitTransform();
    drop_time_ = SDL_GetTicks();
}

bool Key::CanBePickedUp() {
    return SDL_GetTicks() - drop_time_ > KEY_DROP_PICKUP_COOLDOWN_MS;
}

void Key::InitTransform() {
    glm::vec2 previous_pos = glm::vec2(transform->X(), transform->Y());
    transform->ClearParent();
    transform->ResetAndSetTranslation(glm::vec3(previous_pos, KEY_HEIGHT));
    transform->Rotate(M_PI / 2, glm::vec3(1, 0, 0));

    InitBoundingBox(bounding_box_vertices_);
    bounding_box_->transform->ClearParent();
}
