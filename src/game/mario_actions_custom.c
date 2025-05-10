#include <PR/ultratypes.h>
#include "types.h"
#include "sm64.h"
#include "engine/math_util.h"
#include "engine/behavior_script.h"
#include "mario.h"
#include "mario_step.h"
#include "print.h"
#include "object_helpers.h"
#include "engine/surface_collision.h"
#include "game/camera.h"
#include "game_init.h"
#include "behavior_data.h"
#include "game/object_list_processor.h"

s32 perform_ground_step_16(struct MarioState *m);

// 5秒で終了
s32 act_shakusi_0(struct MarioState *m) {

    const int SHAKUSI_VEL = 100 * 5;

    switch (m->actionState) {
        case 0: { // 初期化
            // cam_select_alt_mode(CAM_SELECTION_FIXED);

            m->actionState = 1;
        } break;
        case 1: { // アングルぎめ
            m->flags |= MARIO_METAL_CAP;
            m->actionState = 2;

            // ledge
            // struct WallCollisionData wallCols;
            // struct Surface *wall;
            // s16 wallAngle;
            // s16 wallDYaw;

            // wallCols.x = m->pos[0];
            // wallCols.y = m->pos[1];
            // wallCols.z = m->pos[2];
            // wallCols.radius = 10.0f;
            // wallCols.offsetY = -10.0f;

            // if (find_wall_collisions(&wallCols) != 0) {
            //     wall = wallCols.walls[wallCols.numWalls - 1];
            //     wallAngle = atan2s(wall->normal.z, wall->normal.x);
            //     wallAngle += 0x8000;
            //     wallDYaw = wallAngle - m->faceAngle[1];

            //     if (wallDYaw > -0x4000 && wallDYaw < 0x4000) {
            //         m->pos[0] = wallCols.x - 20.0f * wall->normal.x;
            //         m->pos[2] = wallCols.z - 20.0f * wall->normal.z;

            //         m->faceAngle[0] = 0;
            //         m->faceAngle[1] = wallAngle + 0x8000;

            //         set_mario_action(m, ACT_LEDGE_CLIMB_DOWN, 0);
            //         set_mario_animation(m, MARIO_ANIM_CLIMB_DOWN_LEDGE);
            //     }
            // }

            // m->faceAngle をランダムに決める
            int count = 0;
            while (count++ < 5) {
                m->faceAngle[1] = random_u16();
                // intended_pos
                Vec3f intendedPos;
                intendedPos[0] = m->pos[0] + sins(m->faceAngle[1]) * SHAKUSI_VEL;
                intendedPos[2] = m->pos[2] + coss(m->faceAngle[1]) * SHAKUSI_VEL;
                // struct Surface *pfloor;
                // int height = find_floor(intendedPos[0], m->pos[1], intendedPos[2], &pfloor);

                struct Surface *temp_floor = m->floor;
                int height = find_floor(intendedPos[0], m->pos[1], intendedPos[2], &m->floor);
                if (!m->floor) {
                    m->floor = temp_floor;
                    continue;
                }
                u32 is_slip = mario_floor_is_slippery(m);
                m->floor = temp_floor;

                if (height > m->pos[1] - 100 && height < m->pos[1] + 100 && !is_slip) {
                    break;
                }
            }
            mario_set_forward_vel(m, SHAKUSI_VEL);
            break;
        }
        case 2: { // 移動中
            // forwardvelを適用
            Vec3f prev_pos;
            vec3f_copy(prev_pos, m->pos);
            s32 perform_ground_step_16(struct MarioState * m);
            s32 result = perform_ground_step_16(m);
            // print_text_fmt_int(0,10,"%d",result);
            switch (result) {
                case GROUND_STEP_LEFT_GROUND:
                case GROUND_STEP_HIT_WALL:
                    m->actionState = 1;
                    break;
            }
            u32 is_slip = mario_floor_is_slippery(m);
            if (is_slip) {
                m->actionState = 1;
            }

            // 城のドアの角でスタックするのを防ぐ
            if (m->pos[0] == prev_pos[0] && m->pos[2] == prev_pos[2]) {
                m->actionState = 1;
            }
            break;
        }
    }

    // 毎フレームrボタン押す
    // gPlayer1Controller->buttonDown |= R_TRIG;

    m->actionTimer++;
    if (m->actionTimer > 5 * 30) {
        m->flags &= ~MARIO_METAL_CAP;
        // cam_select_alt_mode(CAM_SELECTION_MARIO);
        m->forwardVel = 0;
        set_mario_action(m, ACT_IDLE, 0);
    }

    return FALSE;
}

f32 dist;

s32 act_shakusi(struct MarioState *m) {

    //const int SHAKUSI_VEL = 100 * 5;
    const int SHAKUSI_VEL = 200 ;

    switch (m->actionState) {
        case 0: { // 初期化
            m->flags |= MARIO_METAL_CAP;
            m->actionState = 1;
        } break;
        case 1: { // アングルぎめ

            struct Object *goomba = cur_obj_find_nearest_object_with_behavior(bhvGoomba, &dist);
            if (goomba) {
                if(!(goomba->oAction < 100)){//クリボーが死ぬ間際なら
                    gCurrentObject = goomba;
                    goomba = cur_obj_find_nearest_object_with_behavior(bhvGoomba, &dist);
                }
                
                // angleをクリボーに向かせる
                m->faceAngle[1] = goomba->oAngleToMario + 0x8000;
            }
             
            // m->faceAngle をランダムに決める
            int count = 0;
            while (count++ < 5) {
                // intended_pos
                Vec3f intendedPos;
                intendedPos[0] = m->pos[0] + sins(m->faceAngle[1]) * SHAKUSI_VEL;
                intendedPos[2] = m->pos[2] + coss(m->faceAngle[1]) * SHAKUSI_VEL;
                
                struct Surface *temp_floor = m->floor;
                int height = find_floor(intendedPos[0], m->pos[1], intendedPos[2], &m->floor);
                if (!m->floor) {
                    m->floor = temp_floor;
                    continue;
                }
                u32 is_slip = mario_floor_is_slippery(m);
                m->floor = temp_floor;
                
                if (height > m->pos[1] - 100 && height < m->pos[1] + 100 && !is_slip) {
                    break;
                }
                
                m->faceAngle[1] = random_u16();
            }
            

            mario_set_forward_vel(m, SHAKUSI_VEL);
            m->actionState = 2;

            break;
        }
        case 2: { // 移動中
            Vec3f prev_pos;
            vec3f_copy(prev_pos, m->pos);

            s32 result = perform_ground_step_16(m);
            switch (result) {
                case GROUND_STEP_LEFT_GROUND:
                case GROUND_STEP_HIT_WALL:
                    m->actionState = 1;
                    break;
            }

            u32 is_slip = mario_floor_is_slippery(m);
            if (is_slip) {
                m->actionState = 1;
            }

            // 城のドアの角でスタックするのを防ぐ
            if (m->pos[0] == prev_pos[0] && m->pos[2] == prev_pos[2]) {
                m->actionState = 1;
            }

            // クリボーに当たったら終了
            f32 dx = m->pos[0] - prev_pos[0];
            f32 dz = m->pos[2] - prev_pos[2];
            f32 delta = sqrtf(dx * dx + dz * dz);
            dist -= delta;
            if(dist < 0) {
                m->actionState = 1;
            }

            break;
        }
    }
    print_text_fmt_int(0, 10, "%d", dist);
    m->actionTimer++;
    if (m->actionTimer > 5 * 30) {
        m->flags &= ~MARIO_METAL_CAP;
        m->forwardVel = 0;
        set_mario_action(m, ACT_IDLE, 0);
    }

    return FALSE;
}

s32 mario_execute_custom_action(struct MarioState *m) {
    s32 cancel;

    /* clang-format off */
    switch (m->action) {
        case ACT_SHAKUSI:           cancel = act_shakusi(m);           break;
    }
    /* clang-format on */

    return cancel;
}