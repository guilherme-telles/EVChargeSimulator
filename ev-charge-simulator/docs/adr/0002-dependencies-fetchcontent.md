# ADR-0002: Dependencies via CMake FetchContent

- Status: Accepted
- Date: 2026-10-26
- Related: §5.4

## Context

The spec suggests vcpkg or Conan as alternatives. The project needs only TinyXML-2 and GoogleTest at build time.

## Decision

Use `FetchContent` in [cmake/Dependencies.cmake](../../cmake/Dependencies.cmake) with pinned tags:
TinyXML-2 `10.0.0`, GoogleTest `v1.15.2`. Qt 6 is located with `find_package` (installed separately).

## Consequences

- No extra tooling; first configure needs network access.
- Upgrades are a one-line tag change, reviewed via PR.
- If more dependencies are added (spdlog, CLI11, fmt), revisit vcpkg manifest mode.
