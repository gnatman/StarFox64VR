# Star Fox 64 VR - Motion Simulator & Telemetry Guide

Star Fox 64 VR features real-time UDP telemetry output and a built-in **Motion Cueing Engine** specifically designed for 6DOF motion platforms (such as the **DOF Reality 6DOF P6**, as well as 2DOF/3DOF seat-movers and G-seats).

---

## 1. Quick Start with SimHub

1. **Install SimHub**: Download and install [SimHub](https://www.simhubdash.com/) (version 9.0+ recommended).
2. **Deploy Definition**:
   * Copy [`docs/simhub/StarFox64VR.simdef`](file:///c:/Users/gnatm/Code/StarFox64VR/docs/simhub/StarFox64VR.simdef) to your SimHub game definition directory (typically `%userprofile%\Documents\SimHub\GameDefinitions` or SimHub installation directory).
   * Ensure [`PluginsData/Common/ResolveCache.json`](file:///c:/Users/gnatm/Code/StarFox64VR/PluginsData/Common/ResolveCache.json) is accessible by SimHub.
3. **Configure Telemetry**:
   * Launch `Starship.exe`.
   * Press **F1** to open the menubar and go to **Settings -> Telemetry**.
   * Check **Enable UDP Telemetry** (`gTelemetry.Enabled`).
   * Set **Port** to match SimHub (default: `20777`).

---

## 2. Telemetry Packet Specification

Telemetry is transmitted over UDP via [`TelemetryManager.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/src/port/TelemetryManager.cpp) at game tick rate (~60Hz / real-time delta).

* **Protocol Signature**: `0x8F140EDE` (SimHub compatible binary packet format).
* **Payload Size**: 219 bytes packed binary struct.
* **Fields Broadcast**:
  * `YawDegrees`, `PitchDegrees`, `RollDegrees` (Euler attitude angles).
  * `YawRateDegreesPerSecond`, `PitchRateDegreesPerSecond`, `RollRateDegreesPerSecond` (Angular rates).
  * `LocalSurgeMs2`, `LocalSwayMs2`, `LocalHeaveMs2` (Linear accelerations in Arwing body frame).
  * `LocalVelocityForwardMps`, `LocalVelocityLateralMps`, `LocalVelocityUpMps`.
  * `Throttle` (0.0 to 1.0, with detent for Boost).
  * `Brake` (0.0 to 1.0 for Air Brakes).
  * `EngineRpm` & `EngineMaxRpm`.
  * Event Flags: Boosting, Braking, Barrel Roll, Somersault / Loop, Damage Hit, Shield loss.

---

## 3. Motion Cueing Engine for 6DOF Platforms

Commercial flight and racing simulators cannot reproduce full 360° rolls or continuous circular loops without tangling cables or hitting physical actuator limits. The DOF Reality 6DOF P6, for example, has an actuator deflection limit of $\pm 17^\circ$.

The custom **Motion Cueing Engine** ([`src/port/MotionCueing.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/src/port/MotionCueing.cpp)) translates extreme game maneuvers into safe, immersive physical cues:

### 16.0° Hard Rig Ceiling
* All output angles are protected by a hyperbolic tangent soft-knee curve (`SoftLimit`) and hard-clamped to **16.0°** (`kMotionRigAbsoluteLimitDeg`), leaving a safety margin before actuator stops.

### Realistic Bank Washout
* **Initial Bank**: When initiating a turn, the rig tilts into the roll (default max bank: 12.0°).
* **Sustained Turn Washout**: Holding a bank gradually eases the rig back to **50%** of the angle (`SustainedFraction = 0.50`) over a configurable washout window (`BankWashoutSec = 3.0s`). This simulates human vestibular adaptation to coordinated turns while preserving actuator headroom for quick reversals.

### Barrel Roll Handling
* The raw 360° spin is **never** sent to the physical rig.
* Instead, triggering a barrel roll produces:
  1. A sharp **roll kick** (default: 6.0°) in the roll direction.
  2. An accompanying **lateral sway jolt** (default: 6.0 m/s²).
  3. A rapid decay back to normal flight baseline.

### Aerobatic Loops & Somersaults
* Continuous vertical loops are translated into:
  1. A steady **nose-up pitch tilt** (default: 8.0°).
  2. Positive **vertical heave / G-load acceleration** (default: 6.0 m/s²).
  3. Seamless return to level flight upon loop exit.

### Yaw Drift Prevention
* Pure heading yaw is unwrapped, and only high-passed **yaw rate** is fed to the yaw axis with a leaky decay (`YawWashoutSec = 1.5s`). The rig never winds up or rotates off-center during 360° dogfights.

### Slew Rate Limiting & Cutscene Fading
* Output angular velocities are rate-limited (`SlewDegPerSec = 60.0°/s`) to prevent sudden mechanical shocks during level teleports or collisions.
* When entering menus, pause screens, or non-interactive cutscenes, the rig automatically and smoothly centers itself.

---

## 4. In-Game Telemetry Configuration (ImGui)

Press **F1** and navigate to **Settings -> Telemetry** to adjust cues in real time:

| Setting | Default | Description |
| :--- | :--- | :--- |
| **Enable UDP Telemetry** | On | Toggles UDP broadcast |
| **Port** | `20777` | Target UDP port |
| **Overall Intensity** | `1.00x` | Master scaling multiplier for all cues |
| **Rig Safety Limit** | `16.0°` | Hard mechanical ceiling (max 16.0° for P6) |
| **Max Bank Angle** | `12.0°` | Maximum roll tilt for steady banks |
| **Max Pitch Angle** | `10.0°` | Maximum pitch up/down tilt |
| **Max Yaw Angle** | `8.0°` | Maximum momentary yaw angle |
| **Sustained Bank Fraction** | `0.50` | Fraction of bank retained during held turns |
| **Bank Washout Time** | `3.0 s` | Duration to ease from peak to sustained bank |
| **Yaw Washout Time** | `1.5 s` | Leaky decay time for yaw turns |
| **Barrel Roll Kick** | `6.0°` | Roll impulse angle on barrel roll trigger |
| **Barrel Roll Sway** | `6.0 m/s²` | Lateral sway G-cue on barrel roll |
| **Loop Pitch Tilt** | `8.0°` | Pitch tilt during somersault loops |
| **Loop Heave (G-Load)** | `6.0 m/s²` | Vertical heave G-cue during loops |
| **Damage Hit Jolt** | `6.0 m/s²` | Impulse jolt when Arwing takes damage |
| **Max Slew Rate** | `60.0°/s` | Maximum angular speed to prevent jerks |
| **Invert Roll / Pitch / Yaw** | Off | Axis inversion toggles for inverted rigs |
| **Reset Motion Defaults** | - | Resets all cueing parameters to factory defaults |

A **Live Rig Cue Output Readout** at the bottom of the menu displays real-time telemetry values (`Roll`, `Pitch`, `Yaw`, `Surge`, `Sway`, `Heave`) to easily monitor platform commands while tuning.
