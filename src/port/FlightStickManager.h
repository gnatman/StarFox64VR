#pragma once
#ifdef __cplusplus
class FlightStickManager {
public:
    static FlightStickManager* GetInstance();
    FlightStickManager();
    ~FlightStickManager();
    void Init();
    void Update();
    void UpdateFFB();
    void ProcessInput(struct OSContPad* pad);
    bool IsEnabled();
private:
    static FlightStickManager* mInstance;
};
#endif
