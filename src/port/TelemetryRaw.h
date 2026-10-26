#pragma once
// Raw game-state snapshot exported from the C game code to the C++ telemetry layer.
// Roll and pitch are kept as their separate game components so the motion cueing
// stage can treat banks, barrel rolls and loops differently.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TelemetryRaw {
    int active;       // 1 = in gameplay and player is flying/driving
    int paused;       // 1 = game paused
    int playerState;  // PlayerState enum
    int form;         // PlayerForm enum
    int levelMode;    // LevelMode enum

    float pos[3];     // world position (game units)
    float vel[3];     // world velocity (game units per tick)
    float baseSpeed;
    float boostSpeed;
    float knockback[3];

    float pitchStick;     // rot.x + xRot_120 (degrees, + = nose up)
    float pitchAerobatic; // aerobaticPitch: somersault / U-turn loop (0..360)
    float yaw;            // rot.y (degrees, unbounded / wrapping)
    float rollStick;      // rot.z (degrees, + = bank left)
    float rollBank;       // zRotBank: Z/R hold bank (degrees, + = bank left)
    float rollBarrel;     // zRotBarrelRoll: barrel roll spin (accumulating degrees)

    int rollState;    // != 0 while a barrel roll is in progress
    int rollRate;     // signed barrel roll rate (+ = rolling left)
    int boostActive;
    int somersault;
    int damage;       // pending damage (> 0 when just hit)
    int hitTimer;
} TelemetryRaw;

void GetTelemetryRaw(TelemetryRaw* out);

#ifdef __cplusplus
}
#endif
