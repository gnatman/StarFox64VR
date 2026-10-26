#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef __cplusplus
#include <string>
#include <chrono>
#include <winsock2.h>
#include "MotionCueing.h"

class TelemetryManager {
public:
    static TelemetryManager* GetInstance();
    TelemetryManager();
    ~TelemetryManager();
    void Init();
    void Update();
    void LoadSettings();
private:
    static TelemetryManager* mInstance;
    bool mEnabled = false;
    std::string mIpAddress;
    int mPort;
    SOCKET mSocket;
    struct sockaddr_in mDestAddr;

    uint64_t mInstanceId = 0;
    uint64_t mSessionId = 0;
    uint64_t mPacketCounter = 0;
    double mSessionTimeSeconds = 0.0;
    uint32_t mDiscontinuityCounter = 0;

    MotionCueing mCueing;
    std::chrono::steady_clock::time_point mLastUpdate;
    bool mHasLastUpdate = false;
    float mSmoothedDt = 1.0f / 30.0f;

    // Raw (unwrapped) orientation, used only by the debug "raw orientation" mode.
    float mRawYawUnwrapped = 0.0f;
    float mLastRawYaw = 0.0f;
};
#endif
#ifdef __cplusplus
extern "C" {
#endif
void TelemetryManager_Init();
void TelemetryManager_Update();
#ifdef __cplusplus
}
#endif
