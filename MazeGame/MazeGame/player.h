#pragma once
#include <vector>
#include "game_object.h"
#include "key.h"
#include "vr_camera.h"

class Player : public GameObject {
   public:
    Player(VRCamera* camera, Map* map);

    void Update() override;

    void UseKey();
    void DropKey();

   private:
    void RegenerateBoundingBox();
    void InitializeKeyLocation(Key* key);

    VRCamera* camera_;
    std::vector<glm::vec4> box_;
    Key* held_key_;

    float vertical_velocity = 0.0f, forward_velocity = 0.0f, right_velocity = 0.0f;
    bool on_ground = true;
    bool stuck_in_object = false;
};
