#pragma once
#include "../util/hkinc.h"

namespace Aimbot {
    inline float fov = 15.0f;
    inline float smoothing = 6.0f;
    inline bool enabled = true;

    struct CTraceFilter {
        std::uintptr_t m_skip_entity;
    };

    struct CGameTrace {
        std::uintptr_t m_pEntity;
        float fraction;
        // Add more fields if needed
    };


    static Vector3 SmoothAngle(const Vector3& from, const Vector3& to, float factor) {
        Vector3 delta = to - from;
        delta.Normalize();
        return from + delta / factor;
    }

    static Vector3 GetBonePosition(uintptr_t entity, int boneIndex) {
        using namespace cs2_dumper::schemas::client_dll;
        if (!entity) return {};

        const uintptr_t gameSceneNode = mem.Read<uintptr_t>(entity + C_BaseEntity::m_pGameSceneNode);
        if (!gameSceneNode) return {};

        const uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + CSkeletonInstance::m_modelState + 0x80);
        if (!boneArray) return {};

        return mem.Read<Vector3>(boneArray + boneIndex * 0x20);
    }

    static Vector3 GetLocalEyePosition(uintptr_t localPlayer) {
        return GetBonePosition(localPlayer, 6);  // Head bone
    }

    static Vector3 GetViewAngles() {
        return mem.Read<Vector3>(mem.client + cs2_dumper::offsets::client_dll::dwViewAngles);
    }

    static void WriteViewAngles(const Vector3& angles) {
        Vector3 normalized = angles;
        normalized.Normalize();
        mem.Write(mem.client + cs2_dumper::offsets::client_dll::dwViewAngles, normalized);
    }

    static Vector3 CalcAngle(const Vector3& src, const Vector3& dst) {
        Vector3 delta = dst - src;
        float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
        Vector3 angles;
        angles.x = -atan2f(delta.z, hyp) * (180.0f / pi_f);
        angles.y = atan2f(delta.y, delta.x) * (180.0f / pi_f);
        angles.z = 0;
        angles.Normalize();
        return angles;
    }

    static float GetFov(const Vector3& viewAngle, const Vector3& targetAngle) {
        Vector3 delta = viewAngle - targetAngle;
        delta.Normalize();
        return sqrtf(delta.x * delta.x + delta.y * delta.y);
    }

    static bool IsVisible(uintptr_t entity, int localPlayerIndex) {
        uint64_t spottedByMask = mem.Read<uint64_t>(entity + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState + cs2_dumper::schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask);
        return (spottedByMask & (1ULL << localPlayerIndex)) != 0;
    }

    static void RunOnce() {
        if (!enabled) return;

        uintptr_t entityList = mem.Read<uintptr_t>(mem.client + cs2_dumper::offsets::client_dll::dwEntityList);
        uintptr_t localPlayer = mem.Read<uintptr_t>(mem.client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
        if (!entityList || !localPlayer) return;

        int localTeam = mem.Read<int>(localPlayer + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        Vector3 localEye = GetLocalEyePosition(localPlayer);
        Vector3 viewAngles = GetViewAngles();

        float bestFov = fov;
        Vector3 bestAngle;

        for (int i = 0; i < 64; ++i) {
            uintptr_t listEntry = mem.Read<uintptr_t>(entityList + ((8 * (i & 0x7FFF)) >> 9) + 16);
            if (!listEntry) continue;

            uintptr_t controller = mem.Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
            if (!controller) continue;

            uint32_t pawnHandle = mem.Read<uint32_t>(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
            if (!pawnHandle) continue;

            uintptr_t pawnEntry = mem.Read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 16);
            if (!pawnEntry) continue;

            uintptr_t entity = mem.Read<uintptr_t>(pawnEntry + 120 * (pawnHandle & 0x1FF));
            if (!entity || entity == localPlayer) continue;

            int health = mem.Read<int>(entity + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
            int team = mem.Read<int>(entity + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
            if (health <= 0 || team == localTeam) continue;

            // Use the fixed visibility check with localPlayer
            int localIndex = mem.Read<int>(mem.client + cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_iIDEntIndex);
            if (!IsVisible(entity, localIndex)) continue;

            Vector3 head = GetBonePosition(entity, 6);
            if (head.IsZero()) continue;

            Vector3 aimAngle = CalcAngle(localEye, head);
            float curFov = GetFov(viewAngles, aimAngle);

            if (curFov < bestFov) {
                bestFov = curFov;
                bestAngle = aimAngle;
            }
        }

        if (bestFov < fov)
            WriteViewAngles(SmoothAngle(viewAngles, bestAngle, smoothing));
    }
}
