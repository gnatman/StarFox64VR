# Star Fox 64 VR (Starship VR)

A modern PC port of **Star Fox 64** (based on HarbourMasters *Starship* and *libultraship*) transformed into a full **Virtual Reality motion simulator experience** with OpenXR stereo 6DOF rendering, first-person cockpit flight, HOTAS flight stick controls, and UDP motion simulator telemetry.

---

## Key Features

* **OpenXR Virtual Reality**:
  * Native 6DOF head tracking and stereo rendering for Meta Quest, Valve Index, Bigscreen Beyond, HTC Vive, and Windows Mixed Reality headsets.
  * Direct3D 11 rendering pipeline with high refresh rate support (72Hz, 80Hz, 90Hz, 120Hz).
  * Desktop mirror window presentation for streaming or local display.
* **First-Person Cockpit View**:
  * Fly directly from inside the Arwing's cockpit with interactive physical head tracking across the canopy.
  * Custom interior cockpit tub floor and rear bulkheads ([`Display_CockpitFloorVR`](file:///c:/Users/gnatm/Code/StarFox64VR/src/engine/fox_display.c)) to prevent see-through terrain below the craft.
  * **All-Range Cockpit Mode**: Option to force first-person cockpit camera during 360° dogfights (Katina, Bolse, Sector Z, and boss battles).
  * Adjustable cockpit dashboard distance, seat height, scale, canopy positioning, and glass glare/transparency slider (`gCockpitOpacity`).
* **Virtual Cinema Screen & 2D HUD**:
  * Title screen, main menu, opening story crawl, map / level select, and pause screens are projected onto a floating virtual cinema screen layer in 3D space, preventing motion sickness and eye fatigue.
  * In-game HUD (shields, lasers, bombs, and radio dialog portraits) floats comfortably in the pilot's view with proper alpha accumulation.
* **3D Celestial Starfield**:
  * Replaces original 2D screen-space orthographic stars with a 3D celestial sphere (1,000 stars distributed via Fibonacci spiral at $R = 8,500$ units) in space levels (Meteo, Sector X/Y, Area 6).
  * Stars render at true optical infinity with per-eye perspective and 6DOF head tracking, eliminating stereo divergence and double vision.
* **Motion Simulator Telemetry & Motion Cueing Engine**:
  * Broadcasts real-time UDP telemetry (port `20777`) compatible with **SimHub**, FlyPT, and motion platforms.
  * Built-in **Motion Cueing Algorithm** tuned for 6DOF motion rigs (such as the **DOF Reality 6DOF P6**):
    * **16.0° Rig Safety Limit**: Soft-knee hyperbolic curve and hard clamp protecting actuator travel limits.
    * **Realistic Bank Washout**: Sustained banking washes out back to 50% of peak angle to prevent actuator saturation.
    * **Barrel Roll Handling**: 360° spins are converted into an initial roll kick (6.0°) and lateral sway jolt (6.0 m/s²) instead of rolling the rig over.
    * **Loop / Somersault Handling**: Continuous pitch loops translate into a steady pitch-up tilt (8.0°) and positive heave G-load (6.0 m/s²).
    * **Yaw Leaky Washout & Slew Rate Limiting**: Prevents heading wind-up and limits angular velocities to avoid harsh jolts.
* **HOTAS & Flight Stick Support**:
  * Raw SDL2 joystick and throttle quadrant support (Thrustmaster TCA Airbus / T.16000M, Logitech X52 / X56, VKB, etc.).
  * Automatic **zero-point neutral calibration fix**, eliminating unprompted pitch-down or yaw drift on commercial joysticks with biased center axes.
  * Force feedback and haptic rumble for laser fire, bomb drops, damage impacts, and boost/brake resistance.

---

## Documentation Guides

* [**Virtual Reality Guide**](docs/VR.md): Detailed OpenXR setup, cockpit camera controls, canopy transparency, and 3D starfield architecture.
* [**Motion Simulator & Telemetry Guide**](docs/MOTION_SIMULATOR.md): SimHub integration, UDP packet format, and Motion Cueing configuration for 6DOF rigs.
* [**HOTAS & Flight Stick Guide**](docs/FLIGHT_STICK.md): Joystick mapping recommendations, neutral axis calibration, and force feedback setup.
* [**Building Instructions**](docs/BUILDING.md): Compiling with `-DENABLE_VR=ON`, asset extraction, and packaging.

---

## Quick Start

Star Fox 64 VR does not include copyrighted assets. You are required to provide a supported copy of the game.

### 1. Verify your ROM dump
Supported ROMs are US 1.0 and US 1.1 Rev A versions. Verify using SHA-1:
* **US 1.0**: `D8B1088520F7C5F81433292A9258C1184AFA1457`
* **US 1.1**: `09F0D105F476B00EFA5303A3EBC42E60A7753B7A`

*Note: Your ROM must be in `.z64` format.*

### 2. Launching in VR
1. Start your VR headset and connect via Meta Quest Link / AirLink / Virtual Desktop or SteamVR.
2. Launch [`Starship.exe`](file:///c:/Users/gnatm/Code/StarFox64VR/Starship.exe).
3. The game will automatically detect OpenXR and render directly into your headset.

---

## Configuration & In-Game Menu

Press **F1** at any time to toggle the ImGui menu bar:

* **Settings -> VR Virtual Screen & HUD**: Adjust virtual 2D screen distance and size.
* **Settings -> VR Cockpit View**: Adjust cockpit forward/back, height, scale, canopy glass opacity, and toggle All-Range Cockpit mode.
* **Settings -> Telemetry**: Configure UDP port, rig safety limits (up to 16.0°), bank washout, barrel roll kick, loop heave, and monitor real-time motion cues.
* **Controller -> Controller Settings**: Map HOTAS axes, buttons, deadzones, and calibrate stick neutral positions.

### Default Shortcuts
| Key | Action |
| :--- | :--- |
| **F1** | Toggle ImGui menu bar |
| **F4** | Reset Game |
| **F11** | Fullscreen Toggle |
| **Tab** | Toggle Alternate Assets |

---

## Special Thanks & Credits

* **HarbourMasters Team**: SonicDcer, Lywx, and the HM64 team for creating Starship and Libultraship.
* **ZeldaRET & Decompals**: For the fundamental Star Fox 64 decompilation work.
* **DarioSamo**: For graphics rendering expertise and renderer guidance.
* **Thar0 & TheBoy181**: For widescreen enhancements and original starfield improvements.
