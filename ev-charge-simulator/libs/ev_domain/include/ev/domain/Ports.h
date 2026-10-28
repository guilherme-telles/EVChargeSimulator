#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "ev/domain/Model.h"
#include "ev/domain/ValidationError.h"

namespace ev::domain {

struct ScenarioLoadResult {
    std::optional<ChargeRequest> scenario;
    std::vector<ValidationError> errors;
    std::vector<Warning> warnings;
    [[nodiscard]] bool ok() const { return scenario.has_value(); }
};

/// Port for scenario persistence (implemented in ev_persistence).
class IScenarioRepository {
public:
    virtual ~IScenarioRepository() = default;
    /// Saves atomically. Returns an empty vector on success.
    [[nodiscard]] virtual std::vector<ValidationError> save(const ChargeRequest& scenario,
                                                            const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual ScenarioLoadResult load(const std::filesystem::path& path) const = 0;
};

/// Port for the vehicle catalog (implemented in ev_catalog).
class IVehicleCatalog {
public:
    virtual ~IVehicleCatalog() = default;
    [[nodiscard]] virtual std::vector<Vehicle> list() const = 0;
    [[nodiscard]] virtual std::optional<Vehicle> find(const std::string& id) const = 0;
};

}  // namespace ev::domain
