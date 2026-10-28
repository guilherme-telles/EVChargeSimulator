#include "ev/app/ResultPresenter.h"

#include <sstream>

#include "ev/app/Formatting.h"
#include "ev/calculator/ChargeCalculator.h"

namespace ev::app {

using namespace ev::domain;

ResultView presentResult(const ChargeRequest& request, const ChargeResult& result) {
    ResultView view;
    view.duration = formatHoursMinutes(result.duration.value());
    view.durationDecimal = formatHoursDecimal(result.duration.value());
    view.totalCost = formatBrl(result.cost.total.value());
    view.addedRange = formatKm(result.addedRange.value());
    view.currentRange = formatKm(result.currentRange.value());
    view.finalRange = formatKm(result.finalRange.value());
    view.batteryEnergy = formatKwh(result.batteryEnergy.value());
    view.gridEnergy = formatKwh(result.gridEnergy.value());
    view.effectivePower = formatKw(result.effectivePower.value());
    view.efficiency = formatPercent(result.efficiency);
    view.curveModel = result.usesChargeCurve
                          ? "Curva segmentada (" + std::to_string(request.curve.segments.size()) + " faixas)"
                          : "Potência constante";
    view.energyCost = formatBrl(result.cost.energy.value());
    view.timeCost = formatBrl(result.cost.time.value());
    view.fixedCost = formatBrl(result.cost.fixed.value());

    const double supply = calculator::supplyLimit(request.charger).value();
    view.assumptions = {
        "Energia na bateria = capacidade útil × (SoC desejado − SoC atual) / 100",
        "Energia da rede = energia na bateria / eficiência (" + view.efficiency + ")",
        "Potência efetiva = mín(carregador, veículo, instalação) = mín(" + formatKw(supply) + " de oferta; " +
            formatKw(request.vehicleMaxPower.value()) + " do veículo)" +
            (result.usesChargeCurve ? ", ajustada por faixa de SoC" : ""),
        "Tempo = energia da rede / potência efetiva" +
            std::string(result.usesChargeCurve ? ", somado por faixa de SoC" : ""),
        "Custo = energia × tarifa + taxa fixa + minutos × tarifa por minuto",
        "Autonomia adicionada = energia na bateria / consumo (" +
            formatDecimalTrimmed(request.efficiency.consumptionKwhPer100Km, 2) + " kWh/100 km)",
        "Valores calculados em precisão total; arredondamento apenas na exibição (meio para longe do zero)",
    };
    for (const auto& w : result.warnings) {
        view.warnings.push_back(w.code + ": " + w.message);
    }
    return view;
}

std::string buildSummary(const ChargeRequest& request, const ChargeResult& result) {
    const ResultView v = presentResult(request, result);
    std::ostringstream out;
    out << "Simulação de recarga" << (request.vehicle.name.empty() ? "" : " - " + request.vehicle.name) << "\n"
        << "SoC: " << formatDecimalTrimmed(request.battery.currentSoc.value(), 1) << "% -> "
        << formatDecimalTrimmed(request.battery.targetSoc.value(), 1) << "%\n"
        << "Tempo estimado: " << v.duration << " (" << v.durationDecimal << ")\n"
        << "Custo total: " << v.totalCost << " (energia " << v.energyCost << "; tempo " << v.timeCost
        << "; taxa fixa " << v.fixedCost << ")\n"
        << "Autonomia adicionada: " << v.addedRange << " (final estimada " << v.finalRange << ")\n"
        << "Energia da rede: " << v.gridEnergy << "; energia na bateria: " << v.batteryEnergy << "\n"
        << "Potência efetiva: " << v.effectivePower << "; eficiência: " << v.efficiency << "; modelo: "
        << v.curveModel << "\n";
    for (const auto& w : v.warnings) {
        out << "Aviso: " << w << "\n";
    }
    out << kEstimateDisclaimer << "\n";
    return out.str();
}

ComparisonHighlights highlight(const std::vector<ComparisonEntry>& entries) {
    ComparisonHighlights h;
    if (entries.size() < 2) {
        return h;
    }
    std::size_t cheapest = 0;
    std::size_t fastest = 0;
    for (std::size_t i = 1; i < entries.size(); ++i) {
        if (entries[i].result.cost.total.value() < entries[cheapest].result.cost.total.value()) {
            cheapest = i;
        }
        if (entries[i].result.duration.value() < entries[fastest].result.duration.value()) {
            fastest = i;
        }
    }
    h.lowestCost = cheapest;
    h.fastest = fastest;
    return h;
}

}  // namespace ev::app
