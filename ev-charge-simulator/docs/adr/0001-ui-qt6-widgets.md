# ADR-0001: Desktop UI with Qt 6 Widgets

- Status: Accepted
- Date: 2026-10-26
- Related: D-01, §7.3, RNF-005, RNF-006

## Context

The spec recommends Qt 6 with MVVM for a sustainable desktop product, keeping the engine in pure C++.

## Decision

- `apps/desktop` uses **Qt 6 Widgets**.
- The "ViewModel" role lives in `libs/ev_app` (`SimulationService`, `ResultPresenter`), which is plain C++ and
  tested with GoogleTest. `MainWindow` is a thin View: it reads widgets, calls the service and binds
  pre-formatted strings. It contains no formulas.
- The desktop target is built only when `find_package(Qt6 COMPONENTS Widgets)` succeeds, so the core and tests
  build on machines without Qt (`core` preset).

## Consequences

- Qt licensing (LGPLv3 vs commercial) must be reviewed before distribution.
- UI tests (Qt Test, spec §8.1 "UI") are not yet implemented; behaviour is covered at the `ev_app` level.
