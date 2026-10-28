#include "ev/app/SimulationService.h"

#include <utility>

namespace ev::app {

using namespace ev::domain;

ChargeRequest referenceScenario() {
    ChargeRequest r;
    r.vehicle = {"manual", "Exemplo de referência (ERS 2.3)"};
    r.battery = {KilowattHours{60.0}, Percent{20.0}, Percent{80.0}, 0.90};
    r.vehicleMaxPower = Kilowatts{7.4};
    r.efficiency.consumptionKwhPer100Km = 16.0;
    r.charger.type = ChargerType::AC;
    r.charger.ratedPower = Kilowatts{11.0};
    r.tariff.energyRatePerKwh = 0.95;
    return r;
}

SimulationService::SimulationService(const calculator::IChargeCalculator& calculator,
                                     const IScenarioRepository& repository)
    : calculator_(calculator), repository_(repository), active_(referenceScenario()) {}

calculator::CalculationOutcome SimulationService::simulate(const ChargeRequest& request) const {
    return calculator_.calculate(request);
}

ScenarioLoadResult SimulationService::openScenario(const std::filesystem::path& path) {
    ScenarioLoadResult loaded = repository_.load(path);
    if (loaded.ok()) {
        active_ = *loaded.scenario;
    }
    return loaded;
}

std::vector<ValidationError> SimulationService::saveActiveScenario(const std::filesystem::path& path) const {
    return repository_.save(active_, path);
}

calculator::CalculationOutcome SimulationService::addToComparison(std::string label, const ChargeRequest& request) {
    auto outcome = calculator_.calculate(request);
    if (outcome.ok()) {
        comparison_.push_back({std::move(label), request, *outcome.result});
    }
    return outcome;
}

}  // namespace ev::app
