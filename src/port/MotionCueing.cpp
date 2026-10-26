#include "MotionCueing.h"
#include "public/bridge/consolevariablebridge.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kDegToRad = kPi / 180.0f;

float WrapDeg(float a) {
    a = std::fmod(a + 180.0f, 360.0f);
    if (a < 0) a += 360.0f;
    return a - 180.0f;
}

// Shortest signed angular difference (handles 359 -> 1 as +2).
float DeltaDeg(float now, float prev) {
    return WrapDeg(now - prev);
}

// Smooth limiter: ~linear for small x, approaches +/-limit asymptotically.
float SoftLimit(float x, float limit) {
    if (limit <= 0.0f) return 0.0f;
    return limit * std::tanh(x / limit);
}

// Linear up to `knee`, then compresses so the result never exceeds `limit`.
float SoftKneeClamp(float x, float limit) {
    const float knee = limit * 0.75f;
    const float ax = std::fabs(x);
    if (ax <= knee) return x;
    const float room = limit - knee;
    const float y = knee + room * std::tanh((ax - knee) / room);
    return std::copysign(y, x);
}

// First-order low-pass step toward target with time constant tau.
float LowPass(float state, float target, float tau, float dt) {
    if (tau <= 0.0f) return target;
    const float a = 1.0f - std::exp(-dt / tau);
    return state + (target - state) * a;
}

// Asymmetric envelope: fast attack, slower release.
float Envelope(float state, float target, float attackTau, float releaseTau, float dt) {
    const bool rising = std::fabs(target) > std::fabs(state);
    return LowPass(state, target, rising ? attackTau : releaseTau, dt);
}

float Slew(float current, float target, float maxRate, float dt) {
    const float maxStep = maxRate * dt;
    return current + std::clamp(target - current, -maxStep, maxStep);
}

MotionCueOutput gLastOutput;

} // namespace

const MotionCueOutput& MotionCueing_GetLastOutput() { return gLastOutput; }
void MotionCueing_SetLastOutput(const MotionCueOutput& out) { gLastOutput = out; }

MotionCueSettings MotionCueSettings::LoadFromCVars() {
    MotionCueSettings d; // defaults
    MotionCueSettings s;
    s.intensity = CVarGetFloat("gTelemetry.Cue.Intensity", d.intensity);
    s.rigLimit = std::clamp(CVarGetFloat("gTelemetry.Cue.RigLimit", d.rigLimit), 0.0f, kMotionRigAbsoluteLimitDeg);
    s.maxRoll = CVarGetFloat("gTelemetry.Cue.MaxRoll", d.maxRoll);
    s.maxPitch = CVarGetFloat("gTelemetry.Cue.MaxPitch", d.maxPitch);
    s.maxYaw = CVarGetFloat("gTelemetry.Cue.MaxYaw", d.maxYaw);
    s.sustainedFraction = std::clamp(CVarGetFloat("gTelemetry.Cue.SustainedFraction", d.sustainedFraction), 0.0f, 1.0f);
    s.bankWashoutSec = CVarGetFloat("gTelemetry.Cue.BankWashoutSec", d.bankWashoutSec);
    s.yawWashoutSec = CVarGetFloat("gTelemetry.Cue.YawWashoutSec", d.yawWashoutSec);
    s.barrelRollKick = CVarGetFloat("gTelemetry.Cue.BarrelRollKick", d.barrelRollKick);
    s.barrelRollSway = CVarGetFloat("gTelemetry.Cue.BarrelRollSway", d.barrelRollSway);
    s.loopPitch = CVarGetFloat("gTelemetry.Cue.LoopPitch", d.loopPitch);
    s.loopHeave = CVarGetFloat("gTelemetry.Cue.LoopHeave", d.loopHeave);
    s.hitJolt = CVarGetFloat("gTelemetry.Cue.HitJolt", d.hitJolt);
    s.accelScale = CVarGetFloat("gTelemetry.Cue.AccelScale", d.accelScale);
    s.maxAccel = CVarGetFloat("gTelemetry.Cue.MaxAccel", d.maxAccel);
    s.slewDegPerSec = CVarGetFloat("gTelemetry.Cue.SlewDegPerSec", d.slewDegPerSec);
    s.invertRoll = CVarGetInteger("gTelemetry.Cue.InvertRoll", 0) != 0;
    s.invertPitch = CVarGetInteger("gTelemetry.Cue.InvertPitch", 0) != 0;
    s.invertYaw = CVarGetInteger("gTelemetry.Cue.InvertYaw", 0) != 0;
    return s;
}

void MotionCueing::Reset() {
    *this = MotionCueing();
}

void MotionCueing::ResetHistory(const TelemetryRaw& raw) {
    mLastYawRaw = raw.yaw;
    mYawUnwrapped = raw.yaw;
    mLastBarrelRaw = raw.rollBarrel;
    mLastLoopRaw = raw.pitchAerobatic;
    mLastSpeed = std::sqrt(raw.vel[0] * raw.vel[0] + raw.vel[1] * raw.vel[1] + raw.vel[2] * raw.vel[2]);
    mLastVelY = raw.vel[1];
    mLastDamage = raw.damage;
    for (int i = 0; i < 3; i++) mLastPos[i] = raw.pos[i];

    // Seed washout filters with the current attitude so there is no jump.
    // (Outputs themselves are NOT reset; the slew limiter eases the rig over.)
    mYawHP = 0;
    mYawRateSm = 0;
    mSurgeSm = mSwaySm = mHeaveSm = 0;
    mHasHistory = true;
}

MotionCueOutput MotionCueing::Update(const TelemetryRaw& raw, float dt, const MotionCueSettings& s) {
    MotionCueOutput out;
    dt = std::clamp(dt, 1.0f / 240.0f, 1.0f / 10.0f);

    // ---- Discontinuity detection ----
    bool disc = false;
    if (!mHasHistory) disc = true;
    if (raw.active && !mWasActive) disc = true;
    if (raw.playerState != mLastPlayerState) {
        const bool flyingStates = (raw.playerState == 3 || raw.playerState == 5) &&
                                  (mLastPlayerState == 3 || mLastPlayerState == 5); // ACTIVE <-> U_TURN
        if (!flyingStates) disc = true;
    }
    if (mHasHistory) {
        const float dx = raw.pos[0] - mLastPos[0];
        const float dy = raw.pos[1] - mLastPos[1];
        const float dz = raw.pos[2] - mLastPos[2];
        if (dx * dx + dy * dy + dz * dz > 3000.0f * 3000.0f) disc = true; // teleport / warp
    }
    if (disc) ResetHistory(raw);
    mLastPlayerState = raw.playerState;
    mWasActive = raw.active;
    out.discontinuity = disc;
    out.active = raw.active;

    // ---- Derived rates (all from unwrapped deltas) ----
    const float dYaw = DeltaDeg(raw.yaw, mLastYawRaw);
    mLastYawRaw = raw.yaw;
    mYawUnwrapped += dYaw;
    const float yawRate = dYaw / dt; // deg/s, game sign

    const float barrelRate = (raw.rollBarrel - mLastBarrelRaw) / dt; // accumulates, no wrap needed
    mLastBarrelRaw = raw.rollBarrel;

    const float loopRate = DeltaDeg(raw.pitchAerobatic, mLastLoopRaw) / dt;
    mLastLoopRaw = raw.pitchAerobatic;

    const float speed = std::sqrt(raw.vel[0] * raw.vel[0] + raw.vel[1] * raw.vel[1] + raw.vel[2] * raw.vel[2]);
    const float dSpeed = (speed - mLastSpeed) / dt;
    mLastSpeed = speed;
    const float dVelY = (raw.vel[1] - mLastVelY) / dt;
    mLastVelY = raw.vel[1];
    for (int i = 0; i < 3; i++) mLastPos[i] = raw.pos[i];

    // Targets in GAME sign (roll + = left, pitch + = nose up, yaw + = game rot.y)
    float rollTarget = 0, pitchTarget = 0, yawTarget = 0;
    float surgeTarget = 0, swayTurn = 0, swayBarrel = 0, heaveTarget = 0;

    if (raw.active) {
        // ---- Roll: steady bank, soft-limited, partially washed out ----
        const float bank = WrapDeg(raw.rollStick + raw.rollBank);
        const float softBank = SoftLimit(bank, s.maxRoll);
        mBankLP = LowPass(mBankLP, softBank, s.bankWashoutSec, dt);
        rollTarget = softBank - (1.0f - s.sustainedFraction) * mBankLP;

        // ---- Barrel roll: kick + sideways jolt (never the 360 spin) ----
        const bool barrelActive = (raw.rollState != 0) && (std::fabs(barrelRate) > 90.0f);
        const float barrelDir = (raw.rollRate != 0) ? (raw.rollRate > 0 ? 1.0f : -1.0f)
                                                    : (barrelRate >= 0 ? 1.0f : -1.0f);
        mBarrelEnv = Envelope(mBarrelEnv, barrelActive ? barrelDir : 0.0f, 0.06f, 0.25f, dt);
        rollTarget += mBarrelEnv * s.barrelRollKick;

        // ---- Pitch: stick pitch soft-limited + partial washout ----
        const float pitch = WrapDeg(raw.pitchStick);
        const float softPitch = SoftLimit(pitch, s.maxPitch);
        mPitchLP = LowPass(mPitchLP, softPitch, s.bankWashoutSec, dt);
        pitchTarget = softPitch - (1.0f - s.sustainedFraction) * mPitchLP;

        // ---- Loops / U-turns: nose-up + heave (never the 360 pitch) ----
        const bool loopActive = raw.somersault || (raw.playerState == 5 /* PLAYERSTATE_U_TURN */) ||
                                (std::fabs(WrapDeg(raw.pitchAerobatic)) > 5.0f) || (std::fabs(loopRate) > 30.0f);
        mLoopEnv = Envelope(mLoopEnv, loopActive ? 1.0f : 0.0f, 0.15f, 0.4f, dt);
        pitchTarget += mLoopEnv * s.loopPitch;

        // ---- Yaw: washed-out yaw rate (never absolute heading) ----
        mYawRateSm = LowPass(mYawRateSm, yawRate, 0.05f, dt);
        // leaky integrator == high-pass of heading
        mYawHP += mYawRateSm * dt - mYawHP * (dt / std::max(s.yawWashoutSec, 0.05f));
        yawTarget = SoftLimit(mYawHP, s.maxYaw);

        // ---- Accelerations ----
        surgeTarget = dSpeed * s.accelScale;
        // centripetal accel from turning: v * omega (rad/s), game yaw sign
        swayTurn = speed * (mYawRateSm * kDegToRad) * s.accelScale;
        heaveTarget = dVelY * s.accelScale + mLoopEnv * s.loopHeave;

        // barrel roll sideways jolt, game roll sign (+ = left)
        swayBarrel = mBarrelEnv * s.barrelRollSway;

        // hit jolt
        if (raw.damage > 0 && mLastDamage <= 0) {
            mHitEnv = 1.0f;
            mHitSign = (raw.knockback[0] >= 0.0f) ? 1.0f : -1.0f;
        }
    } else {
        // Out of gameplay: let everything ease back to neutral.
        mBankLP = LowPass(mBankLP, 0.0f, 0.5f, dt);
        mPitchLP = LowPass(mPitchLP, 0.0f, 0.5f, dt);
        mYawHP = LowPass(mYawHP, 0.0f, 0.5f, dt);
        mBarrelEnv = LowPass(mBarrelEnv, 0.0f, 0.2f, dt);
        mLoopEnv = LowPass(mLoopEnv, 0.0f, 0.2f, dt);
    }
    mLastDamage = raw.damage;

    mHitEnv = LowPass(mHitEnv, 0.0f, 0.15f, dt);
    heaveTarget += -mHitEnv * s.hitJolt;

    // ---- Convert to SimHub sign conventions ----
    // game roll + = left  -> SimHub roll + = right (sway + = right)
    // game pitch + = up   -> SimHub pitch + = nose down
    const float rollSign = s.invertRoll ? 1.0f : -1.0f;
    const float pitchSign = s.invertPitch ? 1.0f : -1.0f;
    const float yawSign = s.invertYaw ? -1.0f : 1.0f;
    float rollSH = rollSign * rollTarget;
    float pitchSH = pitchSign * pitchTarget;
    float yawSH = yawSign * yawTarget;
    const float swaySH = yawSign * swayTurn + rollSign * swayBarrel + mHitSign * mHitEnv * s.hitJolt * 0.5f;

    // ---- Intensity, rig ceiling, slew limiting ----
    const float limit = std::min(s.rigLimit, kMotionRigAbsoluteLimitDeg);
    rollSH = SoftKneeClamp(rollSH * s.intensity, limit);
    pitchSH = SoftKneeClamp(pitchSH * s.intensity, limit);
    yawSH = SoftKneeClamp(yawSH * s.intensity, limit);

    const float prevRoll = mOutRoll, prevPitch = mOutPitch, prevYaw = mOutYaw;
    mOutRoll = Slew(mOutRoll, rollSH, s.slewDegPerSec, dt);
    mOutPitch = Slew(mOutPitch, pitchSH, s.slewDegPerSec, dt);
    mOutYaw = Slew(mOutYaw, yawSH, s.slewDegPerSec, dt);

    // Absolute safety net.
    mOutRoll = std::clamp(mOutRoll, -kMotionRigAbsoluteLimitDeg, kMotionRigAbsoluteLimitDeg);
    mOutPitch = std::clamp(mOutPitch, -kMotionRigAbsoluteLimitDeg, kMotionRigAbsoluteLimitDeg);
    mOutYaw = std::clamp(mOutYaw, -kMotionRigAbsoluteLimitDeg, kMotionRigAbsoluteLimitDeg);

    out.roll = mOutRoll;
    out.pitch = mOutPitch;
    out.yaw = mOutYaw;
    out.rollRate = (mOutRoll - prevRoll) / dt;
    out.pitchRate = (mOutPitch - prevPitch) / dt;
    out.yawRate = (mOutYaw - prevYaw) / dt;

    mSurgeSm = LowPass(mSurgeSm, surgeTarget, 0.08f, dt);
    mSwaySm = LowPass(mSwaySm, swaySH, 0.08f, dt);
    mHeaveSm = LowPass(mHeaveSm, heaveTarget, 0.08f, dt);
    out.surge = std::clamp(mSurgeSm * s.intensity, -s.maxAccel, s.maxAccel);
    out.sway = std::clamp(mSwaySm * s.intensity, -s.maxAccel, s.maxAccel);
    out.heave = std::clamp(mHeaveSm * s.intensity, -s.maxAccel, s.maxAccel);

    return out;
}
