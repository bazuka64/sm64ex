#include "pch.h"

extern "C" {
#include <types.h>
#include <object_fields.h>
#include <game/camera.h>
#include <pc/gfx/gfx_pc.h>
#include <model_ids.h>
#include <behavior_data.h>
#include <game/object_helpers.h>
#include <game/print.h>
#include <engine/behavior_script.h>
#include <game/interaction.h>
}

#include "mmd.h"

std::vector<struct MMDModel *> mmd_models;
struct VMDAnimation *idle;
struct VMDAnimation *oiroke;
GLuint program;
std::vector<Object *> mmd_objects;

extern struct Object *gCurrentObject;
#define o gCurrentObject

static struct ObjectHitbox sMMDHitbox = {
    /* interactType:      */ INTERACT_MMD,
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 0,
    /* health:            */ 0,
    /* numLootCoins:      */ 0,
    /* radius:            */ 72,
    /* height:            */ 500,
    /* hurtboxRadius:     */ 0,
    /* hurtboxHeight:     */ 0,
};

extern "C" void bhv_mmd_init() {
    mmd_objects.push_back(o);
    o->oAnimations = (Animation **) idle;
    obj_set_hitbox(o, &sMMDHitbox);
}

extern "C" void bhv_mmd_update() {
    if (o->oTimer == 10 * 30) {
        o->oTimer = 0;
        if (o->oAnimations == (Animation **) idle) {
            o->oAnimations = (Animation **) oiroke;
        } else {
            o->oAnimations = (Animation **) idle;
        }
    }
    o->oInteractStatus = 0;
}

void coin_spawner_helper(const std::vector<glm::vec3> &start_to_end) {
    if (start_to_end.size() < 2)
        return;

    for (size_t i = 0; i < start_to_end.size() - 1; ++i) {
        const glm::vec3 &start = start_to_end[i];
        const glm::vec3 &end = start_to_end[i + 1];
        float max_len = glm::distance(start, end);
        glm::vec3 direction = glm::normalize(end - start);
        float dist = 0;

        while (dist < max_len) {
            struct Object *coin = spawn_object(o, MODEL_YELLOW_COIN, bhvYellowCoin);
            coin->oPosX = start.x + direction.x * dist;
            coin->oPosY = start.y + direction.y * dist;
            coin->oPosZ = start.z + direction.z * dist;
            dist += 200;
        }
    }
}

// x:0 y:1000 z:-2000
// mario_start: -1328, 260, 4664
extern "C" void bhv_coin_spawner_init(void) {

    {
        std::vector<glm::vec3> start_to_end;
        start_to_end.push_back(glm::vec3(-1000, 400, 4000));
        start_to_end.push_back(glm::vec3(0, 1000, -2000 + 200 * 20));
        start_to_end.push_back(glm::vec3(0, 1000, -2000));
        coin_spawner_helper(start_to_end);
    }
    {
        std::vector<glm::vec3> start_to_end;
        start_to_end.push_back(glm::vec3(-2000, 400, 4000));
        start_to_end.push_back(glm::vec3(-2000 - 2000, 400 + 2000, 4000 - 2000));
        start_to_end.push_back(glm::vec3(-2000 - 2000, 400 + 2000, 4000 - 3000));
        start_to_end.push_back(glm::vec3(-1600, 4200, -4200));
        coin_spawner_helper(start_to_end);
    }
}

extern "C" void bhv_breakable_box_spawner_init() {
    for (int i = 0; i < 10; i++) {
        struct Object *box = spawn_object(o, MODEL_BREAKABLE_BOX, bhvBreakableBox);
        box->oPosX = -1328 - 500;
        box->oPosY = 260 + i * 200;
        box->oPosZ = 4664 + 500;
    }
    
    {
        std::vector<glm::vec3> start_to_end;
        start_to_end.push_back(glm::vec3(-1328 - 500, 260 + 1 * 200, 4664 + 200));
        start_to_end.push_back(glm::vec3(-1328 - 500, 260 + 11 * 200, 4664 + 200));
        start_to_end.push_back(glm::vec3(-1328 - 500, 260 + 11 * 200, 4664 + 500));
        coin_spawner_helper(start_to_end);
    }
}

void spawn_goomba() {
    struct Object *enemy = spawn_object(o, MODEL_GOOMBA, bhvGoomba);
    // ポジションはランダム x:-8000~8000 y:2000 z:-8000~8000
    enemy->oPosX = (random_float() - 0.5) * 16000;
    enemy->oPosY = 2000;
    enemy->oPosZ = (random_float() - 0.5) * 16000;
    enemy->parentObj = enemy;
}

extern "C" void bhv_enemy_spawner_init() {
    for (int i = 0; i < 200; i++) {
        spawn_goomba();
    }
}

extern s16 gPrevFrameObjectCount;
extern "C" void bhv_enemy_spawner_update() {
    // 1秒に1回クリボーをスポーン
    if (o->oTimer % 30 == 0) {
        spawn_goomba();
    }
    print_text_fmt_int(0, 20, "%d", gPrevFrameObjectCount);
}

extern "C" void bhv_deformable_box_init(){
    
}

extern "C" void bhv_deformable_box_update(){
    
}

extern "C" void hook_from_clear_level() {
    mmd_objects.clear();
}

std::string res_path = "../../src/hack/res/";

extern "C" void hook_from_main_func() {
    mmd_models.push_back(new MMDModel(res_path + "meirin/meirin.pmx"));
    mmd_models.push_back(new MMDModel(res_path + "mima/mima.pmx"));
    mmd_models.push_back(new MMDModel(res_path + "kasen/kasen.pmx"));
    mmd_models.push_back(new MMDModel(res_path + "kazami/kazami.pmx"));
    idle = new VMDAnimation(res_path + "1.ぼんやり待ち_(490f_移動なし).vmd");
    oiroke = new VMDAnimation(res_path + "モデル眺めるようポーズ集/脇見せ.vmd");
    program = LoadShader();
}

extern "C" void hook_from_gfx_run() {

    glm::vec3 camera_pos(gLakituState.pos[0], gLakituState.pos[1], gLakituState.pos[2]);
    glm::vec3 camera_focus(gLakituState.focus[0], gLakituState.focus[1], gLakituState.focus[2]);
    extern struct CameraFOVStatus sFOVState;
    glm::mat4 view = glm::lookAt(camera_pos, camera_focus, glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(sFOVState.fov), gfx_current_dimensions.aspect_ratio, 100.f, 20000.f);

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, (float *) &view);
    glUniformMatrix4fv(glGetUniformLocation(program, "proj"), 1, GL_FALSE, (float *) &proj);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    for (Object *obj : mmd_objects) {
        float mmd_scale = 25;
        if (obj->oBehParams2ndByte >= 2)
            mmd_scale *= 0.95;
        glm::mat4 world = glm::translate(glm::mat4(1), glm::vec3(obj->oPosX, obj->oPosY, obj->oPosZ));
        world = glm::scale(world, glm::vec3(mmd_scale));
        glUniformMatrix4fv(glGetUniformLocation(program, "world"), 1, GL_FALSE, (float *) &world);
        mmd_models[obj->oBehParams2ndByte]->Draw((VMDAnimation *) obj->oAnimations);
    }

    glDisable(GL_DEPTH_TEST);
}