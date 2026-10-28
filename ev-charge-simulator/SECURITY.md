# Security Policy / Política de Segurança

## Supported versions / Versões suportadas

| Version | Supported |
|---------|-----------|
| 0.1.x   | ✅        |

## Reporting a vulnerability / Como reportar

**Do not open a public issue for security problems.** Use GitHub's
[private vulnerability reporting](https://docs.github.com/code-security/security-advisories/guidance-on-reporting-and-writing-information-about-vulnerabilities/privately-reporting-a-security-vulnerability)
on this repository, or contact the maintainers privately.

**Não abra issue pública para problemas de segurança.** Use o reporte privado de vulnerabilidades
do GitHub neste repositório ou contate os mantenedores em particular.

Please include / Inclua:
- affected version or commit / versão ou commit afetado;
- a minimal scenario XML or steps to reproduce / XML mínimo ou passos para reproduzir;
- impact assessment / avaliação de impacto.

## Security controls in scope / Controles no escopo (ERS-VE-001 §6.3, RNF-008)

- XML files larger than 1 MB are rejected before parsing.
- Element depth is limited (`kMaxElementDepth = 8`); catalog limited to 1000 vehicles.
- No external entities, DTDs or arbitrary paths are resolved (TinyXML-2 does not process DTDs).
- Files are parsed into a temporary object and fully validated before replacing the active scenario.
- Saves use a temporary file + rename so a failed write does not corrupt the previous file.
- No remote data collection in the MVP (RNF-009).

## Known gaps / Lacunas conhecidas

- Fuzzing of the XML parser (IT-010, libFuzzer) is not yet implemented.
- CodeQL, Dependabot and sanitizer jobs from spec §10.3 are not yet configured in CI.
