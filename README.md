# F4-RainSplashes (RainSplashesF4SE)

F4SE plugin for **Fallout 4** that spawns **rain splash** effects around the player during rainy weather, with per-intensity tiers, optional debug markers, weather overrides (JSON), and an in-game **Menu Framework 3** settings UI.

Conceptually similar to [Splashes of Storms](https://github.com/powerof3/SplashesOfStorms) (Skyrim); this is a **Fallout 4** implementation using the game’s **`BSTempEffectDebris`** pipeline and **`ProcessLists`** (not raw `NiNode` attachment).

**Repository:** [https://github.com/DCCStudios/F4-RainSplashes](https://github.com/DCCStudios/F4-RainSplashes)

## Requirements (players)

- **Fallout 4** with **F4SE** installed.
- **[Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/43375)** — `version-*.bin` in `Data\F4SE\Plugins\` must match your **`Fallout4.exe`** (Steam vs MS Store vs GOG, and patch level).
- **[F4SE Menu Framework 3](https://www.nexusmods.com/fallout4/mods/61765)** (ImGui in-game menu). The plugin registers a settings page with the framework.

This source tree’s **Address Library IDs** are aligned with **pre–Next-Gen** `Fallout4.exe` (e.g. **1.10.163**). If you run **Next-Gen** (1.10.980+), you need a build whose `REL::ID` values and `RelSanity.cpp` checks match your Address Library database.

## Install (from a release or your own build)

Typical layout:

```
Data\F4SE\Plugins\RainSplashesF4SE.dll
Data\F4SE\Plugins\RainSplashesF4SE.ini
Data\F4SE\Plugins\RainSplashesF4SE\RainSplashesF4SE_WeatherOverrides.json   (optional)
```

Logs: `Documents\My Games\Fallout4\F4SE\RainSplashesF4SE.log`

Open the **Menu Framework** overlay in-game and find the **Rain Splashes** (or similarly named) page to change options live. Use **Save settings to INI** to persist.

## Features (short)

- Rain detection using weather data and optional **JSON overrides** for per-weather behavior.
- **Light / Medium / Heavy** tiers: ray density, radius, scales, lifetimes, mesh paths.
- **Debug markers** (vanilla `MarkerX.nif`) to visualize hit points.
- **Cover threshold** (vertical distance from player) to reduce splashes under overhangs.
- **Loose-file check** for mesh paths in the UI and before spawning, to avoid crashes on bad paths (archive-only NIFs may still show a false warning; see in-game text).

## Building from source

**Prerequisites**

- **Visual Studio 2022** (C++ desktop workload), **CMake** 3.21+.
- **[vcpkg](https://github.com/microsoft/vcpkg)** with `VCPKG_ROOT` set (CMake toolchain is picked up from the environment).
- **CommonLibF4** vendored next to this repo so this path exists (see `CMakeLists.txt`):

  `../PluginTemplate/CommonLibF4/CommonLibF4`

  Adjust `add_subdirectory` in `CMakeLists.txt` if your layout differs.

**Configure & build**

```powershell
cd F4-RainSplashes
cmake --preset release
cmake --build build/release --config Release
```

Output DLL (and copied INI) are written under:

`Compile\F4SE\Plugins\`

Optional: set **`Fallout4Path`** in the environment and enable **`COPY_BUILD`** in CMake to copy the DLL into your game after build.

## Configuration

- **`RainSplashesF4SE.ini`** — lives next to the DLL; master toggle, thresholds, tier mesh paths, scales, etc.
- **`Data\F4SE\Plugins\RainSplashesF4SE\RainSplashesF4SE_WeatherOverrides.json`** — optional weather categorization / overrides (see plugin code and any shipped example).

Mesh paths in INI are stored with a **`Meshes\`** prefix where applicable. The runtime debris spawn uses paths **relative to `Meshes\`** internally; the plugin normalizes and validates under your game’s **`Data`** folder when it can resolve it.

Third-party code (F4SE Menu Framework, CommonLibF4, F4SE headers) remains under its original licenses.

## Credits

- Inspired by **Splashes of Storms** (Skyrim) by **powerof_three**.
- **F4SE**, **CommonLibF4**, **Address Library**, **Menu Framework 3** authors and maintainers.
