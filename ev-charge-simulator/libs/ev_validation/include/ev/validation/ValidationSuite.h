#pragma once

#include <string>
#include <vector>

namespace ev::validation {

/// Uma verificação rápida do motor, vinculada à ERS-VE-001, seção 8.2.
/// A aprovação refere-se APENAS aos cenários executados, não à cobertura de código.
struct CheckResult {
    std::string id;
    std::string requirement;
    std::string category;
    std::string description;
    std::string expected;
    std::string observed;
    bool passed = false;
};

/// Executa casos determinísticos contra o ChargeCalculator real, sem alterar o cenário do usuário.
[[nodiscard]] std::vector<CheckResult> runQuickChecks();

/// CSV UTF-8 com separador ';' para abertura no Excel (pt-BR).
[[nodiscard]] std::string reportAsCsv(const std::vector<CheckResult>& results);

} // namespace ev::validation
