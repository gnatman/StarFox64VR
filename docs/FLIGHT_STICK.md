# Star Fox 64 VR - HOTAS & Flight Stick Guide

Star Fox 64 VR includes native support for modern **HOTAS** (Hands On Throttle-And-Stick) systems, dual sticks, and independent USB flight hardware.

---

## 1. Supported Flight Hardware

Any USB joystick, throttle, or rudder pedals recognized by Windows and SDL2 will work:
* **Thrustmaster**: TCA Sidestick Airbus Edition, T.16000M FCS, Warthog, TWCS Throttle, TFRP Rudder Pedals.
* **Logitech / Saitek**: X52 / X52 Pro, X56 Rhino, Extreme 3D Pro.
* **VKB**: Gladiator NXT EVO, Gunfighter.
* **VIRPIL**: MongoosT-50, Constellation Alpha.
* **Turtle Beach / Honeycomb / CH Products**.

---

## 2. Zero-Point Neutral Calibration Fix

Certain commercial flight sticks (notably the Thrustmaster TCA Airbus and dual-engine throttles) initialize axes with non-zero neutral biases or report raw analog endpoints that cause the ship to drift downwards when the stick is resting in the center position.

Star Fox 64 VR features a built-in neutral calibration and deadzone system in `libultraship`:
1. **Physical Center Detection**: In [`libultraship/src/controller/physicaldevice/ConnectedPhysicalDeviceManager.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/controller/physicaldevice/ConnectedPhysicalDeviceManager.cpp), raw axis readings are normalized against detected resting state.
2. **Neutral Offset Calibration**: In [`libultraship/src/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.cpp), deadzones are applied symmetrically around the calibrated true neutral, eliminating unprompted pitch-down or yaw drift.

---

## 3. Recommended Controller Mapping

Open the controller configuration window via **F1 -> Controller -> Controller Settings**:

| N64 Function | Flight Hardware Input | Rationale |
| :--- | :--- | :--- |
| **Analog Pitch (Up/Down)** | **Stick Y-Axis** | Forward = Pitch Down, Pull Back = Pitch Up |
| **Analog Yaw (Left/Right)** | **Stick X-Axis** | Left = Turn Left, Right = Turn Right |
| **Z Trigger (Bank Left)** | **Stick Z-Twist Left** or **Left Rudder** | Double-tap for left barrel roll |
| **R Trigger (Bank Right)** | **Stick Z-Twist Right** or **Right Rudder** | Double-tap for right barrel roll |
| **A Button (Laser Fire)** | **Index Trigger** | Rapid combat firing |
| **B Button (Plasma Bomb)** | **Thumb Weapon Button / Pickle** | Easy access under thumb |
| **C-Up (Boost)** | **Throttle Forward Detent** | Push forward to boost |
| **C-Down (Air Brake)** | **Throttle Aft Detent** | Pull back to brake |
| **C-Left / C-Right** | **Hat Switch Left / Right** | Camera pan / Wingman commands |
| **Start Button** | **Base Button / Menu Button** | Pause menu |

---

## 4. Force Feedback & Haptics

Star Fox 64 VR integrates with `SDL_Haptic` and DirectInput rumble:
* **Laser Recoil**: Crisp momentary haptic pulses upon firing.
* **Damage & Explosions**: Directional force feedback impulses when struck by enemy fire or hitting terrain.
* **Boost & Brake Resistance**: Sustained high-frequency rumble while engaging afterburners or air brakes.
* **Barrel Roll Aerodynamic Kick**: Gyroscopic resistance during snap rolls.
