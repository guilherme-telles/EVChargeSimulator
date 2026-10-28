#pragma once

#include <utility>

#include "ev/domain/Model.h"

namespace ev::calculator {

/// Cost strategy (spec 5.3): allows simple or composite tariffs.
class ITariffStrategy {
public:
    virtual ~ITariffStrategy() = default;
    [[nodiscard]] virtual domain::CostBreakdown calculate(domain::KilowattHours gridEnergy,
                                                          domain::Hours duration) const = 0;
};

/// Cost = E_rede × tarifa_kWh + taxa_fixa + tempo_min × tarifa_minuto (spec 2.1).
class CompositeTariff final : public ITariffStrategy {
public:
    explicit CompositeTariff(domain::Tariff tariff) : tariff_(std::move(tariff)) {}
    [[nodiscard]] domain::CostBreakdown calculate(domain::KilowattHours gridEnergy,
                                                  domain::Hours duration) const override;

private:
    domain::Tariff tariff_;
};

}  // namespace ev::calculator
