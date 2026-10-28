# ADR-0007: Error code extensions

- Status: Accepted
- Date: 2026-10-26
- Related: §4.2, RN-02, UT-012

## Context

The spec defines VAL-001..004, IO-001 and CALC-001 only. Some mandatory rules had no code.

## Decision

| Code | Condition | Message (pt-BR) |
|------|-----------|-----------------|
| VAL-005 | Consumption ≤ 0 (UT-012, blocks the whole calculation) | Informe um consumo médio maior que zero (kWh/100 km). |
| VAL-006 | Usable capacity ≤ 0 | Informe uma capacidade útil de bateria maior que zero. |
| VAL-007 | Negative tariff value (RN-02) | Valores de tarifa não podem ser negativos. |
| VAL-008 | Rated range ≤ 0 when informed | Informe uma autonomia nominal maior que zero ou deixe em branco. |
| VAL-009 | Invalid curve bands | Faixas da curva de carga inválidas: ... |
| IO-002 | Save failure | Não foi possível salvar o cenário. O arquivo anterior foi preservado. |
| WARN-001 | No curve: constant power model (RN-07) | (non-blocking) |
| WARN-002 | Final range capped (RN-08) | (non-blocking) |
| WARN-003 | Unknown XML element ignored | (non-blocking) |
| WARN-004 | Decimal comma normalized in XML | (non-blocking) |

VAL-005..008 were approved by the requester. **VAL-009, IO-002 and WARN-001..004 were added during
implementation and still need Product approval.**

Non-finite inputs (NaN/∞) fail the same rule as the field (e.g. NaN SoC → VAL-001).
