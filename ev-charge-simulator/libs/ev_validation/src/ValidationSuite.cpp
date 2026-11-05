#include "ev/validation/ValidationSuite.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ev/calculator/ChargeCalculator.h"
#include "ev/domain/ValidationError.h"

namespace ev::validation {
namespace {

using namespace ev::domain;
using ev::calculator::CalculationOutcome;
using ev::calculator::ChargeCalculator;

ChargeRequest referenceRequest() {
    ChargeRequest request;
    request.battery = {KilowattHours{60.0}, Percent{20.0}, Percent{80.0}, 0.90};
    request.vehicleMaxPower = Kilowatts{7.4};
    request.efficiency.consumptionKwhPer100Km = 16.0;
    request.charger.type = ChargerType::AC;
    request.charger.ratedPower = Kilowatts{11.0};
    request.tariff.energyRatePerKwh = 0.95;
    return request;
}

bool closeTo(double actual, double expected, double tolerance) {
    return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}

std::string number(double value, int decimals = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(decimals) << value;
    std::string formatted = out.str();
    std::replace(formatted.begin(), formatted.end(), '.', ',');
    return formatted;
}

bool hasCode(const CalculationOutcome& outcome, const char* code) {
    return !outcome.ok() &&
           std::any_of(outcome.errors.begin(), outcome.errors.end(),
                       [code](const ValidationError& error) { return error.code == code; });
}

std::string describeErrors(const CalculationOutcome& outcome) {
    if (outcome.ok()) return "Cálculo aceito (nenhum erro)";
    std::string output;
    for (const auto& error : outcome.errors) {
        if (!output.empty()) output += ", ";
        output += error.code;
    }
    return output.empty() ? "Cálculo rejeitado sem código" : output;
}

CheckResult result(const char* id, const char* requirement, const char* category,
                   const char* description, std::string expected, std::string observed, bool passed) {
    return {id, requirement, category, description, expected, std::move(observed), passed};
}

std::string csvCell(const std::string& input) {
    std::string escaped(1, '"');
    for (char c : input) {
        if (c == '"') escaped += "\"\"";
        else if (c == '\r' || c == '\n') escaped += ' ';
        else escaped += c;
    }
    return escaped + '"';
}

} // namespace

std::vector<CheckResult> runQuickChecks() {
    const ChargeCalculator engine;
    std::vector<CheckResult> checks;
    checks.reserve(14);
    auto r = referenceRequest();
    auto out = engine.calculate(r);
    bool ok = out.ok();
    bool pass = ok && closeTo(out.result->batteryEnergy.value(), 36.0, 0.01) &&
                closeTo(out.result->gridEnergy.value(), 40.0, 0.01) &&
                closeTo(out.result->duration.value() * 60.0, 40.0 / 7.4 * 60.0, 1.0) &&
                closeTo(out.result->cost.total.value(), 38.0, 0.01) &&
                closeTo(out.result->addedRange.value(), 225.0, 1.0);
    checks.push_back(result("UT-001", "RF-003/004/005/006", "Referência", "Cenário oficial 20% → 80%",
        "36 kWh bateria | 40 kWh rede | 324,32 min | R$ 38,00 | 225 km",
        ok ? number(out.result->batteryEnergy.value()) + " kWh bateria | " +
             number(out.result->gridEnergy.value()) + " kWh rede | " +
             number(out.result->duration.value() * 60.0) + " min | R$ " +
             number(out.result->cost.total.value()) + " | " +
             number(out.result->addedRange.value(), 0) + " km" : describeErrors(out), pass));

    checks.push_back(result("UT-002", "RN-03", "Limites", "Veículo limita o carregador",
        "Potência efetiva = 7,40 kW",
        ok ? "Potência efetiva = " + number(out.result->effectivePower.value()) + " kW" : describeErrors(out),
        ok && closeTo(out.result->effectivePower.value(), 7.4, 1.0e-9)));

    r = referenceRequest();
    r.charger.installationLimit = Kilowatts{3.7};
    out = engine.calculate(r);
    checks.push_back(result("UT-003", "RN-03", "Limites", "Instalação limita a potência",
        "Potência efetiva = 3,70 kW",
        out.ok() ? "Potência efetiva = " + number(out.result->effectivePower.value()) + " kW" : describeErrors(out),
        out.ok() && closeTo(out.result->effectivePower.value(), 3.7, 1.0e-9)));

    r = referenceRequest();
    r.battery.targetSoc = r.battery.currentSoc;
    out = engine.calculate(r);
    checks.push_back(result("UT-004", "RN-01", "Entrada inválida", "SoC alvo igual ao atual",
        "Cálculo rejeitado: VAL-002", describeErrors(out), hasCode(out, codes::kTargetNotAboveCurrent)));

    r = referenceRequest();
    r.battery.targetSoc = Percent{10.0};
    out = engine.calculate(r);
    checks.push_back(result("UT-005", "RN-01", "Entrada inválida", "SoC alvo menor que o atual",
        "Cálculo rejeitado: VAL-002", describeErrors(out), hasCode(out, codes::kTargetNotAboveCurrent)));

    r = referenceRequest();
    r.battery.currentSoc = Percent{-5.0};
    out = engine.calculate(r);
    checks.push_back(result("UT-006", "RF-002", "Entrada inválida", "SoC fora de 0% a 100%",
        "Cálculo rejeitado: VAL-001", describeErrors(out), hasCode(out, codes::kSocOutOfRange)));

    r = referenceRequest();
    r.charger.ratedPower = Kilowatts{0.0};
    out = engine.calculate(r);
    checks.push_back(result("UT-007", "RN-03", "Entrada inválida", "Potência zero",
        "Cálculo rejeitado: VAL-003", describeErrors(out), hasCode(out, codes::kPowerNotPositive)));

    r = referenceRequest();
    r.battery.efficiency = 1.0;
    out = engine.calculate(r);
    checks.push_back(result("UT-008", "RN-04", "Eficiência", "Eficiência de 100%",
        "Energia da rede = energia bateria = 36,00 kWh",
        out.ok() ? "Rede = " + number(out.result->gridEnergy.value()) + " kWh | bateria = " +
                   number(out.result->batteryEnergy.value()) + " kWh" : describeErrors(out),
        out.ok() && closeTo(out.result->gridEnergy.value(), 36.0, 0.01) &&
                    closeTo(out.result->gridEnergy.value(), out.result->batteryEnergy.value(), 1.0e-9)));

    r = referenceRequest();
    r.battery.efficiency = 0.5;
    out = engine.calculate(r);
    checks.push_back(result("UT-009", "RN-04", "Eficiência", "Eficiência de 50%",
        "Energia da rede = 72,00 kWh",
        out.ok() ? "Energia da rede = " + number(out.result->gridEnergy.value()) + " kWh" : describeErrors(out),
        out.ok() && closeTo(out.result->gridEnergy.value(), 72.0, 0.01)));

    r = referenceRequest();
    r.tariff.energyRatePerKwh = 0.0;
    out = engine.calculate(r);
    checks.push_back(result("UT-010", "RF-005", "Tarifa", "Tarifa zerada",
        "Custo = R$ 0,00 (demais saídas válidas)",
        out.ok() ? "Custo = R$ " + number(out.result->cost.total.value()) +
                   " | rede = " + number(out.result->gridEnergy.value()) + " kWh" : describeErrors(out),
        out.ok() && closeTo(out.result->cost.total.value(), 0.0, 1.0e-9) &&
                    closeTo(out.result->gridEnergy.value(), 40.0, 0.01)));

    r = referenceRequest();
    r.tariff.fixedFee = Brl{5.0};
    r.tariff.ratePerMinute = 0.10;
    out = engine.calculate(r);
    const double composite = 38.0 + 5.0 + (40.0 / 7.4 * 60.0) * 0.1;
    checks.push_back(result("UT-011", "RF-005", "Tarifa", "Tarifa composta: energia + tempo + fixa",
        "Custo = R$ 75,43 (valor não arredondado internamente)",
        out.ok() ? "Custo = R$ " + number(out.result->cost.total.value()) : describeErrors(out),
        out.ok() && closeTo(out.result->cost.total.value(), composite, 0.01)));

    r = referenceRequest();
    r.efficiency.consumptionKwhPer100Km = 0.0;
    out = engine.calculate(r);
    checks.push_back(result("UT-012", "RF-002", "Entrada inválida", "Consumo de 0 kWh/100 km",
        "Cálculo rejeitado: VAL-005", describeErrors(out), hasCode(out, codes::kConsumptionNotPositive)));

    r = referenceRequest();
    r.battery.targetSoc = Percent{100.0};
    r.curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{3.0}}};
    out = engine.calculate(r);
    const double segmentedHours = 60.0 * 0.60 / 0.90 / 7.4 + 60.0 * 0.20 / 0.90 / 3.0;
    checks.push_back(result("UT-013", "RF-012", "Curva de carga", "Potência reduzida acima de 80%",
        "Tempo segmentado = " + number(segmentedHours, 3) + " h",
        out.ok() ? "Tempo segmentado = " + number(out.result->duration.value(), 3) + " h" : describeErrors(out),
        out.ok() && out.result->usesChargeCurve && closeTo(out.result->duration.value(), segmentedHours, 0.001)));

    r = referenceRequest();
    r.battery.usableCapacity = KilowattHours{1.0e6};
    r.vehicleMaxPower = Kilowatts{1.0e-3};
    r.battery.currentSoc = Percent{0.0};
    r.battery.targetSoc = Percent{100.0};
    r.battery.efficiency = 0.5;
    out = engine.calculate(r);
    const bool finite = out.ok() && std::isfinite(out.result->duration.value()) &&
                        std::isfinite(out.result->gridEnergy.value()) &&
                        std::isfinite(out.result->cost.total.value()) &&
                        std::isfinite(out.result->addedRange.value());
    checks.push_back(result("UT-014", "RNF-003", "Robustez", "Valores extremos válidos",
        "Resultados finitos (sem NaN ou infinito)",
        out.ok() ? (finite ? "Resultados finitos" : "Resultado contém NaN ou infinito") : describeErrors(out), finite));
    return checks;
}

std::string reportAsCsv(const std::vector<CheckResult>& results) {
    std::ostringstream csv;
    csv << "ID;Requisito;Categoria;Cenário;Esperado;Obtido;Status\r\n";
    for (const auto& check : results) {
        csv << csvCell(check.id) << ';' << csvCell(check.requirement) << ';'
            << csvCell(check.category) << ';' << csvCell(check.description) << ';'
            << csvCell(check.expected) << ';' << csvCell(check.observed) << ';'
            << (check.passed ? "APROVADO" : "REPROVADO") << "\r\n";
    }
    return csv.str();
}

} // namespace ev::validation
