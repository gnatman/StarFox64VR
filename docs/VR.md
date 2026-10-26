# Star Fox 64 VR - Virtual Reality Guide

This guide details the OpenXR Virtual Reality implementation in **Star Fox 64 VR (Starship VR)**, including head tracking, stereo rendering, cockpit enhancements, virtual screen projection, and the 3D celestial starfield.

---

## 1. Supported Headsets & Runtimes

Star Fox 64 VR utilizes standard **OpenXR 1.0** with Direct3D 11 swapchains. It is compatible with all modern OpenXR-compliant PC VR runtimes:

* **Meta Quest (Quest 2, 3, Pro)**: Meta Quest Link (Cable), AirLink, or Virtual Desktop (using VDXR or SteamVR OpenXR runtime).
* **Valve Index**: SteamVR OpenXR runtime.
* **Bigscreen Beyond**: SteamVR OpenXR runtime.
* **HTC Vive / Vive Pro / Focus 3**: SteamVR or HTC Vive OpenXR runtime.
* **Windows Mixed Reality**: Windows Mixed Reality OpenXR runtime or SteamVR.

Ensure your active OpenXR runtime is set in your VR software (e.g., in the Meta Quest desktop app under *Settings -> General -> OpenXR Runtime*, or in SteamVR under *Settings -> OpenXR*).

---

## 2. Core Architecture

### Stereo Rendering & Matrix Injection
In non-VR Starship, the Fast3D interpreter translates N64 display lists into Direct3D 11 draws using a single projection matrix and modelview stack.

Under VR (`ENABLE_VR`):
1. **Per-Eye Traversal**: For every frame, the display list runs twice (once for the Left Eye, once for the Right Eye) into OpenXR swapchain textures.
2. **Head Pose & Stereo Offset**: Fast3D queries [`VRRuntime::GetViewMatrix()`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/vr/VRRuntime.cpp#L462) and [`VRRuntime::GetProjectionMatrix()`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/vr/VRRuntime.cpp#L442).
3. **Matrix Replacement**: In [`libultraship/src/graphic/Fast3D/interpreter.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/graphic/Fast3D/interpreter.cpp#L1180-L1204), when `applyVR` is active:
   $$\mathbf{v}_{\text{eye}} = \mathbf{V}_{\text{vr}} \times (\mathbf{M}_{\text{game}} \times \mathbf{v}_{\text{model}})$$
   $$\mathbf{v}_{\text{clip}} = \mathbf{P}_{\text{vr}} \times \mathbf{v}_{\text{eye}}$$
   This decouples physical head rotation and inter-pupillary distance (IPD) from the Arwing's flight model while preserving the game's original camera orientation.

---

## 3. Cockpit View Mode

### 6DOF First-Person Cockpit
During active gameplay, the camera is positioned inside the Arwing's cockpit, providing a true 6DOF flight experience:
* Looking around in physical space lets you track enemies, asteroids, and wingmen through the cockpit canopy.
* The Arwing instrument panel and controls render directly in front of the pilot.

### All-Range Mode Cockpit
Original Star Fox 64 restricted cockpit view to on-rails segments. Star Fox 64 VR includes full **All-Range Cockpit Mode**:
* Check **Settings -> VR Cockpit View -> Force Cockpit in All-Range Mode** (`gVRForceCockpitAllRange`).
* Works in Katina, Bolse, Sector Z, and boss battles (Star Wolf, Andross).
* The custom camera routine [`Camera_UpdateCockpit360`](file:///c:/Users/gnatm/Code/StarFox64VR/src/engine/fox_play.c) matches 360° flight orientation without camera clipping.

### Cockpit Tub Floor & Interior Geometry
The original N64 cockpit model was open at the bottom because the terrain below was occluded on CRT displays. In 6DOF VR, looking down previously revealed see-through terrain.
* Star Fox 64 VR renders a custom interior tub floor and rear bulkhead ([`Display_CockpitFloorVR()`](file:///c:/Users/gnatm/Code/StarFox64VR/src/engine/fox_display.c#L718)), enclosing the cabin with dark, matte fighter-jet bulkheads.

### Canopy Glass & Cockpit Positioning
Tune the cockpit geometry to your personal physical height and preferred seating position in **Settings -> VR Cockpit View**:
* **Cockpit Forward/Back (`gVRCockpitZ`)**: Adjusts dashboard distance (default: `-8.0`).
* **Cockpit Height (`gVRCockpitY`)**: Adjusts vertical seat height (default: `-4.0`).
* **Cockpit Scale (`gVRCockpitScale`)**: Scales instrument panel size (default: `1.25x`).
* **Canopy Glass Position & Scale (`gVRCanopyZ`, `gVRCanopyY`, `gVRCanopyScale`)**: Offsets the canopy frame and glass relative to the pilot.
* **Canopy Glass Opacity (`gCockpitOpacity`)**: Controls the reflectivity/glare of the canopy glass:
  * `0`: Completely transparent (maximum clarity).
  * `120`: Default realistic tinted glass with subtle sheen.
  * `255`: High-opacity glare.

---

## 4. Virtual Cinema Screen & HUD

To prevent motion sickness and maintain crisp text legibility, 2D interfaces are not plastered flat across your eyes. Instead, they are rendered into an OpenXR **Composition Quad Layer**:

1. **Projected 2D Menus**:
   * The Title Screen, Main Menu, Opening Sequence & Story Crawl, Map / Level Select, and Pause Screens are projected onto a high-resolution virtual screen floating in 3D space in front of you.
2. **Gameplay HUD Overlay**:
   * Shields, lasers, bomb counters, score, and incoming radio transmissions with animated character portraits float comfortably within the pilot's forward field of view without depth collision against 3D cockpit glass.
3. **Alpha Accumulation & Blend Fix**:
   * In [`libultraship/src/graphic/Fast3D/backends/gfx_direct3d11.cpp`](file:///c:/Users/gnatm/Code/StarFox64VR/libultraship/src/graphic/Fast3D/backends/gfx_direct3d11.cpp), Direct3D 11 blend state uses `SrcBlendAlpha = ONE; DestBlendAlpha = INV_SRC_ALPHA;`, preventing black-box clipping or see-through holes in front of 3D objects.
4. **Configuration**:
   * Under **Settings -> VR Virtual Screen & HUD**, adjust **Screen Distance** (`gVRHUDDistance`, default `1.50 m`) and **Screen Width** (`gVRHUDWidth`, default `1.00 m`).

---

## 5. 3D Celestial Starfield

### The Problem in Original Code
In space levels (Meteo, Sector X, Sector Y, Area 6), original Star Fox 64 rendered stars using 2D screen-space orthographic projection (`Lib_InitOrtho`). In VR, this caused:
1. Identical 2D pixel coordinates drawn to both eyes (zero stereo disparity across divergent VR lenses), causing severe wall-eyed divergence and double vision.
2. Head rotation did not move the stars, locking them to the player's retinas while 3D asteroids moved independently.

### The 3D Solution
Implemented in [`src/engine/fox_bg.c:Background_DrawStarfieldVR()`](file:///c:/Users/gnatm/Code/StarFox64VR/src/engine/fox_bg.c#L330):
* Generates a 3D celestial sphere of 1,000 stars at $R = 8,500$ units using a Fibonacci spiral distribution with natural color temperatures (brilliant white, class O/B blue-white, solar yellow, and warm red).
* Every star quad is tangentially aligned to face the origin $(0, 0, 0)$ across the full $360^\circ$ sphere.
* Renders in 3D perspective space with the camera's rotational look-at matrix. Fast3D applies per-eye OpenXR projection and 6DOF head tracking.
* Depth writing and depth testing are disabled (`G_ZBUFFER` cleared, `G_CC_SHADE` vertex colors), placing the stars at true optical infinity behind all asteroids, lasers, and ships with zero eye strain.

---

## 6. VR Performance & Display Refresh Rates

Star Fox 64 VR supports native high refresh rates (72Hz, 80Hz, 90Hz, 120Hz) via the `XR_FB_display_refresh_rate` OpenXR extension.
* Press **F1** to open the ImGui menubar.
* Enable **VR Performance Overlay** to monitor per-stage frame budgets (HMD sync/idle, stereo draw time, desktop mirror presentation, and N64 game logic).
