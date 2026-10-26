#include "TelemetryManager.h"
#include "TelemetryPacket.h"
#include "TelemetryRaw.h"
#include "public/bridge/consolevariablebridge.h"

#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <spdlog/spdlog.h>
#include <math.h>

#include <random>
#include <chrono>
#include <algorithm>

TelemetryManager* TelemetryManager::mInstance = nullptr;
TelemetryManager::TelemetryManager() { Init(); }
TelemetryManager::~TelemetryManager() {}

TelemetryManager* TelemetryManager::GetInstance() {
    if (!mInstance) mInstance = new TelemetryManager();
    return mInstance;
}

void TelemetryManager::LoadSettings() {}

void TelemetryManager::Init() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int port = CVarGetInteger("gTelemetry.Port", 20777);
    const char* ip = CVarGetString("gTelemetry.IP", "127.0.0.1");

    mDestAddr.sin_family = AF_INET;
    mDestAddr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &mDestAddr.sin_addr);

    std::random_device rd;
    std::mt19937_64 gen(rd());
    mInstanceId = gen();
    mSessionId = gen();
    mPacketCounter = 0;
}

void TelemetryManager::Update() {
    if (!CVarGetInteger("gTelemetry.Enabled", 1)) return;

    // ---- Measure the real time between game ticks ----
    const auto now = std::chrono::steady_clock::now();
    float dt = 1.0f / 30.0f;
    if (mHasLastUpdate) {
        dt = std::chrono::duration<float>(now - mLastUpdate).count();
    }
    mLastUpdate = now;
    mHasLastUpdate = true;
    dt = std::clamp(dt, 1.0f / 240.0f, 1.0f / 10.0f);
    mSmoothedDt = mSmoothedDt + (dt - mSmoothedDt) * 0.2f; // smooth out frame jitter

    TelemetryRaw raw;
    GetTelemetryRaw(&raw);

    const MotionCueSettings settings = MotionCueSettings::LoadFromCVars();
    const MotionCueOutput cue = mCueing.Update(raw, mSmoothedDt, settings);
    MotionCueing_SetLastOutput(cue);
    if (cue.discontinuity) {
        mDiscontinuityCounter++;
    }
    if (!raw.paused) {
        mSessionTimeSeconds += mSmoothedDt;
    }

    TelemetryPacket pkt = {};

    // SimHub Required Protocol Header (55 bytes)
    pkt.GameSignature = TELEMETRY_GAME_SIGNATURE;
    pkt.TelemetrySignature = TELEMETRY_PROTOCOL_SIGNATURE;
    pkt.LayoutMajorVersion = 1;
    pkt.LayoutMinorVersion = 0;
    pkt.EmitterInstanceId = mInstanceId;
    pkt.PacketId = 0;
    pkt.PacketsCounter = ++mPacketCounter;
    pkt.IsSessionRunning = 1;
    pkt.IsSessionPaused = raw.paused ? 1 : 0;
    pkt.SessionId = mSessionId;
    pkt.IsReplay = 0;
    pkt.IsUserInControl = raw.active ? 1 : 0;
    pkt.IsAIInControl = 0;
    pkt.IsSpectator = 0;
    pkt.SessionTimeSeconds = mSessionTimeSeconds;
    pkt.PhysicsDiscontinuityCounter = mDiscontinuityCounter;

    // ---- Orientation & rates: motion cue (bounded for the rig) ----
    if (CVarGetInteger("gTelemetry.RawOrientation", 0)) {
        // Debug only: true attitude (NOT rig safe). Yaw unwrapped, roll excludes barrel spin.
        float dYaw = fmodf(raw.yaw - mLastRawYaw + 540.0f, 360.0f) - 180.0f;
        mLastRawYaw = raw.yaw;
        mRawYawUnwrapped += dYaw;
        pkt.YawDegrees = mRawYawUnwrapped;
        pkt.PitchDegrees = -(raw.pitchStick + raw.pitchAerobatic);
        pkt.RollDegrees = -(raw.rollStick + raw.rollBank + raw.rollBarrel);
    } else {
        pkt.YawDegrees = cue.yaw;
        pkt.PitchDegrees = cue.pitch;
        pkt.RollDegrees = cue.roll;
    }
    pkt.YawRateDegreesPerSecond = cue.yawRate;
    pkt.PitchRateDegreesPerSecond = cue.pitchRate;
    pkt.RollRateDegreesPerSecond = cue.rollRate;

    // ---- Accelerations: motion cue ----
    pkt.LocalSurgeMs2 = cue.surge;
    pkt.LocalSwayMs2 = cue.sway;
    pkt.LocalHeaveMs2 = cue.heave;

    // ---- Velocity / position (informational, for dashboards) ----
    const float fwdSpeed = raw.baseSpeed + raw.boostSpeed;
    pkt.LocalVelocityLateralMps = raw.vel[0];
    pkt.LocalVelocityUpMps = raw.vel[1];
    pkt.LocalVelocityForwardMps = fwdSpeed;
    pkt.VehiclePositionEast = raw.pos[0];
    pkt.VehiclePositionUp = raw.pos[1];
    pkt.VehiclePositionNorth = raw.pos[2];

    pkt.FlightIsOnGround = (raw.form == 1 /* FORM_LANDMASTER */) ? 1 : 0;
    const float speed = sqrtf(raw.vel[0] * raw.vel[0] + raw.vel[1] * raw.vel[1] + raw.vel[2] * raw.vel[2]);
    pkt.GroundSpeedKmh = (std::max)(speed, fwdSpeed) * 3.6f;

    // ---- Engine & controls ----
    pkt.Throttle = raw.boostActive ? 1.0f : 0.5f;
    pkt.Brake = 0.0f;
    strncpy_s(pkt.Gear, "1", sizeof(pkt.Gear));
    pkt.EngineMaxRpm = 10000.0f;
    pkt.EngineRpm = 3000.0f + std::clamp(fwdSpeed / 60.0f, 0.0f, 1.0f) * 6000.0f;

    // ---- Contact surface ----
    const uint16_t surface = pkt.FlightIsOnGround ? SIMHUB_SURFACE_PRIMARY : SIMHUB_SURFACE_NONE;
    pkt.TyreContactSurfaceFrontLeft = surface;
    pkt.TyreContactSurfaceFrontRight = surface;
    pkt.TyreContactSurfaceRearLeft = surface;
    pkt.TyreContactSurfaceRearRight = surface;

    // ---- Custom fields ----
    pkt.IsRaceActive = raw.active ? 1 : 0;
    int32_t flags = 0;
    if (raw.boostActive) flags |= TELEMETRY_EVENT_BOOSTING;
    if (raw.damage > 0) flags |= TELEMETRY_EVENT_HIT;
    if (raw.rollState != 0) flags |= TELEMETRY_EVENT_SPINOUT;
    pkt.EventFlags = flags;

    sendto(mSocket, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&mDestAddr, sizeof(mDestAddr));
}

extern "C" {
    void TelemetryManager_Init() { TelemetryManager::GetInstance()->Init(); }
    void TelemetryManager_Update() { TelemetryManager::GetInstance()->Update(); }
}
