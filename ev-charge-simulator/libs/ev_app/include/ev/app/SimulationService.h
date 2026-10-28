#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "ev/app/ResultPresenter.h"
#include "ev/calculator/ChargeCalculator.h"
#include "ev/domain/Ports.h"

namespace ev::app {

/// Reference example from spec 2.3 (60 kWh, 20% -> 80%, 11 kW charger, 7.4 kW vehicle, 90%, R$ 0,95/kWh).
[[nodiscard]] domain::ChargeRequest referenceScenario();

/// Application service: UI -> SimulationService -> calculator / repository (spec 5.1).
class SimulationService {
public:
    SimulationService(const calculator::IChargeCalculator& calculator, const domain::IScenarioRepository& repository);

    [[nodiscard]] calculator::CalculationOutcome simulate(const domain::ChargeRequest& request) const;

    [[nodiscard]] const domain::ChargeRequest& activeScenario() const { return active_; }
    void setActiveScenario(domain::ChargeRequest scenario) { active_ = std::move(scenario); }

    /// Replaces the active scenario only when the file is fully valid (RNF-002, IT-002).
    domain::ScenarioLoadResult openScenario(const std::filesystem::path& path);
    [[nodiscard]] std::vector<domain::ValidationError> saveActiveScenario(const std::filesystem::path& path) const;

    /// Calculates and appends to the comparison only when the request is valid.
    calculator::CalculationOutcome addToComparison(std::string label, const domain::ChargeRequest& request);
    void clearComparison() { comparison_.clear(); }
    [[nodiscard]] const std::vector<ComparisonEntry>& comparison() const { return comparison_; }

private:
    const calculator::IChargeCalculator& calculator_;
    const domain::IScenarioRepository& repository_;
    domain::ChargeRequest active_;
    std::vector<ComparisonEntry> comparison_;
};

}  // namespace ev::app
