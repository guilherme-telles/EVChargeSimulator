#pragma once

#include <vector>

#include "ev/domain/Model.h"
#include "ev/domain/ValidationError.h"

namespace ev::domain {

/// Centralized input validation (RF-002, RN-01..RN-04). Empty result means the request is valid.
[[nodiscard]] std::vector<ValidationError> validate(const ChargeRequest& request);

}  // namespace ev::domain
