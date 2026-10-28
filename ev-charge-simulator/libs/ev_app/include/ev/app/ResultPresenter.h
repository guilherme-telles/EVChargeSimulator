#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "ev/domain/Model.h"

namespace ev::app {

inline constexpr const char* kEstimateDisclaimer =
    "Resultado estimado: pode variar com temperatura, envelhecimento da bateria, compartilhamento de "
    "potência, curva de carga do veículo e perdas. Não é medição certificada nem base fiscal.";

/// Display-ready strings (pt-BR) for one result. The UI only binds these; it never formats or computes.
struct ResultView {
    std::string duration;
    std::string durationDecimal;
    std::string totalCost;
    std::string addedRange;
    std::string currentRange;
    std::string finalRange;
    std::string batteryEnergy;
    std::string gridEnergy;
    std::string effectivePower;
    std::string efficiency;
    std::string curveModel;
    std::string energyCost;
    std::string timeCost;
    std::string fixedCost;
    std::vector<std::string> assumptions;
    std::vector<std::string> warnings;
};

[[nodiscard]] ResultView presentResult(const domain::ChargeRequest& request, const domain::ChargeResult& result);

/// Structured plain-text summary for "copy summary" (RF-010 MVP).
[[nodiscard]] std::string buildSummary(const domain::ChargeRequest& request, const domain::ChargeResult& result);

struct ComparisonEntry {
    std::string label;
    domain::ChargeRequest request;
    domain::ChargeResult result;
};

/// Indexes of the cheapest and fastest scenarios (RF-008). Empty when fewer than two entries.
struct ComparisonHighlights {
    std::optional<std::size_t> lowestCost;
    std::optional<std::size_t> fastest;
};

[[nodiscard]] ComparisonHighlights highlight(const std::vector<ComparisonEntry>& entries);

}  // namespace ev::app
