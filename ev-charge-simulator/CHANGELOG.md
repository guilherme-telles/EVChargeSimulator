# Changelog

All notable changes to this project are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions follow
[Semantic Versioning](https://semver.org/) with tags `vMAJOR.MINOR.PATCH`.

## [Unreleased]

### Added

- Windows post-build step that runs `windeployqt` on `ev_desktop`, copying the Qt DLLs, plugins and the
  Qt WebEngine runtime (`QtWebEngineProcess`, resources, locales) next to the executable so it runs from the build tree.
- Windows executable icon: `apps/desktop/resources/app.rc` and `app.ico` (generated from `icons/Icon.png`).
- `.gitignore` rules for the Windows portable package output (`EVChargeSimulator_*_win64/` and `.zip`).

### Changed

- The CMake warning for the desktop app now lists the missing Qt 6 components (e.g. `WebEngineWidgets`,
  `WebChannel`) when Qt is found but incomplete, instead of reporting "Qt 6 not found".
- Portable package instructions: `windeployqt` now excludes the unused `qtposition_nmea` plugin (it requires
  Qt Serial Port), and the documented package size is updated to about 220 MB (about 100 MB zipped) because of Qt WebEngine.
- README prerequisites now list the required Qt add-on modules (WebEngine, WebChannel, Positioning and
  `qtdeclarative`) and include a Windows `CMAKE_PREFIX_PATH` example.

### Fixed

- `ev_desktop.exe` failed to start on Windows with "Qt6WebChanneld.dll not found" because the Qt runtime was
  not deployed next to the executable.
- Configuring with the desktop app enabled on Windows failed because `apps/desktop/resources/app.rc` was missing.

## [0.2.0] - 2026-11-02

### Added

- Initial CAD vehicle viewer component using Qt Widgets and OpenCASCADE.
- STEP file import workflow using `StepImporter` and `AIS_Shape`.
- Initial support for 3D model interaction, including rotation, pan, zoom and view reset.
- Interactive map module using `QWebEngineView`, Qt WebChannel and Leaflet.
- Map functionality for origin and destination selection, route visualization and charging station discovery.
- Vehicle range indicators on the map, based on battery state of charge and rated vehicle range.
- Development container environment based on Ubuntu 24.04 for consistent C++/Qt development.
- Cross-platform CMake configuration with dedicated presets for Linux and Windows.
- Windows build support using Visual Studio 2022 (MSVC x64) and Qt 6.
- VS Code-based development workflow for both Linux Dev Container and native Windows environments.
- Windows deployment instructions using `windeployqt` for packaging Qt runtime dependencies.
- `.gitignore` rules for build artifacts, IDE files and generated files.

### Changed

- Desktop application source organization into dedicated modules:
  `Layout`, `Input`, `Validation`, `Results`, `Comparison` and `Style`.
- UI theme organization with centralized color definitions in `EVTheme.h`.
- Extraction of application styling into a dedicated `MainWindowStyle.cpp` module.
- Refined UI components and comparison table layout.
- Improved desktop interface organization and component separation.
- CMake desktop target organization to accommodate the modular source structure.
- CMake configuration to support Qt 6 components required by the desktop application, including Widgets, Network, WebEngineWidgets and WebChannel.
- CMake preset organization to preserve existing Linux development, core, release and coverage workflows while adding the Windows MSVC build configuration.
- README documentation updated with Linux and Windows development setup, build instructions, testing and deployment procedures.

### Fixed

- Improved comparison table header and column sizing behavior.
- Adjusted UI layout and spacing issues identified during the desktop interface refactoring.
- Improved map layout integration within the desktop application's stacked-page navigation.
- Corrected vehicle range calculations displayed on the map to account for the current battery state of charge.
- Improved map and vehicle indicator update flow when selecting a vehicle.
- Improved build configuration consistency between Linux and Windows environments.


## [0.1.0] - 2026-10-26

### Added
- `ev_domain`: strong unit types, model (`BatterySpec`, `ChargerSpec`, `Tariff`, `VehicleEfficiency`,
  `ChargeCurve`, `ChargeRequest`, `ChargeResult`), centralized validation and error codes VAL-001..009.
- `ev_calculator`: `IChargeCalculator`, constant and segmented charge curve models (`IChargeCurveModel`),
  composite tariff (`ITariffStrategy`: energy + fixed fee + per-minute).
- `ev_persistence`: versioned XML scenarios (schema 1.0) with TinyXML-2, 1 MB / depth limits,
  atomic save, decimal-comma normalization and unknown-element warnings.
- `ev_catalog`: local XML vehicle catalog with approximate, **unverified** data for 4 models.
- `ev_app`: `SimulationService`, pt-BR formatting, result presenter, scenario comparison, copyable summary.
- `apps/desktop`: Qt 6 Widgets UI with inline validation, result cards, details, warnings, comparison
  table, open/save XML and copy summary.
- GoogleTest suites: unit (UT-001..015), component (service, presenter, p95 performance) and integration
  (IT-001..009, catalog).
- GitHub Actions CI (Linux GCC + Qt, coverage gate, Windows MSVC), issue/PR templates, ADRs.