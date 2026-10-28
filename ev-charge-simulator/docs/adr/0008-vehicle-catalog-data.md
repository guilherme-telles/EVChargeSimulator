# ADR-0008: Vehicle catalog data source

- Status: Accepted (provisional)
- Date: 2026-10-26
- Related: D-03, RF-013, risk "Dados incorretos de veículos" (§11.2)

## Decision

- The catalog is a local XML file, [libs/ev_catalog/data/vehicles.xml](../../libs/ev_catalog/data/vehicles.xml),
  copied next to the desktop executable as `data/vehicles.xml`.
- It ships 4 real models with **approximate public figures that have NOT been verified**. Range standards differ
  (PBEV/INMETRO, WLTP, EPA); consumption is derived as `capacity / range × 100`.
- The UI shows a warning that catalog data is approximate; manual entry is always available.

## Consequences

**Before any release, Product must replace or verify every entry against official manufacturer documentation**
and record the source per vehicle (`source` attribute).
