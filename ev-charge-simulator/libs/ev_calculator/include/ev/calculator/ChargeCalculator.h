#pragma once

#include <optional>
#include <vector>

#include "ev/domain/Model.h"
#include "ev/domain/ValidationError.h"

namespace ev::calculator {

/// Either a valid result or a list of typed errors, never both.
struct CalculationOutcome {
    std::optional<domain::ChargeResult> result;
    std::vector<domain::ValidationError> errors;
    [[nodiscard]] bool ok() const { return result.has_value(); }
};

class IChargeCalculator {
public:
    virtual ~IChargeCalculator() = default;
    [[nodiscard]] virtual CalculationOutcome calculate(const domain::ChargeRequest& request) const = 0;
};

/// Base engine (spec 2.1). No dependency on UI or filesystem.
class ChargeCalculator final : public IChargeCalculator {
public:
    [[nodiscard]] CalculationOutcome calculate(const domain::ChargeRequest& request) const override;
};

/// P_efetiva limit imposed by the supply side: min(P_carregador, P_limite_instalação).
[[nodiscard]] domain::Kilowatts supplyLimit(const domain::ChargerSpec& charger);

}  // namespace ev::calculator
