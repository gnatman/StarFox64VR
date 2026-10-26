#pragma once
// Motion cueing for physically limited motion rigs (tuned for a DOF Reality P6, 17 deg/axis).
// Converts the Arwing's real attitude into bounded "cue" angles and accelerations:
//  - steady banks/pitch are soft-limited and partially washed out over time
//  - barrel rolls become a short roll kick + sideways jolt (never a 360 roll)
//  - somersaults / U-turns become a nose-up tilt + heave (never a 360 pitch)
//  - yaw only responds to yaw *rate* and washes back to center
// Output sign conventions follow SimHub: roll + = right, pitch + = nose down,
// yaw + = clockwise, surge + = accelerating, sway + = to the right, heave + = up.

#include "TelemetryRaw.h"

// Absolute ceiling on any cue angle, regardless of settings (rig max is 17 deg).
constexpr float kMotionRigAbsoluteLimitDeg = 16.0f;

struct MotionCueSettings {
    float intensity = 1.0f;              // global multiplier on everything
    float rigLimit = 16.0f;              // deg, final soft-knee ceiling (clamped to kMotionRigAbsoluteLimitDeg)
    float maxRoll = 12.0f;               // deg, soft limit for steady bank
    float maxPitch = 10.0f;              // deg, soft limit for steady pitch
    float maxYaw = 8.0f;                 // deg, soft limit for yaw cue
    float sustainedFraction = 0.5f;      // share of a held bank/pitch kept after washout
    float bankWashoutSec = 3.0f;         // time constant of bank/pitch washout
    float yawWashoutSec = 1.5f;          // time constant of yaw washout
    float barrelRollKick = 6.0f;         // deg, roll kick during barrel roll
    float barrelRollSway = 6.0f;         // m/s^2, sideways jolt during barrel roll
    float loopPitch = 8.0f;              // deg, nose-up tilt during somersault/U-turn
    float loopHeave = 6.0f;              // m/s^2, G-load heave during somersault/U-turn
    float hitJolt = 6.0f;                // m/s^2, jolt when taking damage
    float accelScale = 0.25f;            // game units/tick/s -> m/s^2
    float maxAccel = 15.0f;              // m/s^2 clamp for surge/sway/heave
    float slewDegPerSec = 60.0f;         // max change rate of cue angles
    bool invertRoll = false;
    bool invertPitch = false;
    bool invertYaw = false;

    static MotionCueSettings LoadFromCVars();
};

struct MotionCueOutput {
    float roll = 0, pitch = 0, yaw = 0;                // deg, bounded
    float rollRate = 0, pitchRate = 0, yawRate = 0;    // deg/s of the cue
    float surge = 0, sway = 0, heave = 0;              // m/s^2, bounded
    bool discontinuity = false;                        // filters were reset this tick
    bool active = false;
};

class MotionCueing {
public:
    MotionCueOutput Update(const TelemetryRaw& raw, float dt, const MotionCueSettings& s);
    void Reset();

private:
    void ResetHistory(const TelemetryRaw& raw);

    bool mHasHistory = false;
    bool mWasActive = false;
    int mLastPlayerState = -1;
    int mLastDamage = 0;
    float mLastPos[3] = {};

    // unwrapped angle trackers
    float mLastYawRaw = 0, mYawUnwrapped = 0;
    float mLastBarrelRaw = 0;
    float mLastLoopRaw = 0;

    float mLastSpeed = 0;
    float mLastVelY = 0;

    // filter states
    float mBankLP = 0, mPitchLP = 0;   // low-pass of soft-limited bank/pitch (for washout)
    float mYawHP = 0;                  // leaky integrator of yaw rate
    float mYawRateSm = 0;
    float mBarrelEnv = 0;              // signed, -1..1
    float mLoopEnv = 0;                // 0..1
    float mHitEnv = 0;                 // 0..1
    float mHitSign = 1;
    float mSurgeSm = 0, mSwaySm = 0, mHeaveSm = 0;

    // final outputs (slew-limited)
    float mOutRoll = 0, mOutPitch = 0, mOutYaw = 0;
};

// Last output, for the in-game live readout.
const MotionCueOutput& MotionCueing_GetLastOutput();
void MotionCueing_SetLastOutput(const MotionCueOutput& out);
