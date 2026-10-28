# Changelog

All notable changes to this project are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions follow
[Semantic Versioning](https://semver.org/) with tags `vMAJOR.MINOR.PATCH`.

## [Unreleased]

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
