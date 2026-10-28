#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ev/domain/Ports.h"

namespace ev::catalog {

inline constexpr std::size_t kMaxCatalogVehicles = 1000;

/// Local XML vehicle catalog (RF-013). All entries are validated on load.
class XmlVehicleCatalog final : public domain::IVehicleCatalog {
public:
    explicit XmlVehicleCatalog(std::vector<domain::Vehicle> vehicles) : vehicles_(std::move(vehicles)) {}

    [[nodiscard]] std::vector<domain::Vehicle> list() const override { return vehicles_; }
    [[nodiscard]] std::optional<domain::Vehicle> find(const std::string& id) const override;

private:
    std::vector<domain::Vehicle> vehicles_;
};

struct CatalogLoadResult {
    std::optional<XmlVehicleCatalog> catalog;
    std::vector<domain::ValidationError> errors;
    [[nodiscard]] bool ok() const { return catalog.has_value(); }
};

[[nodiscard]] CatalogLoadResult parseCatalog(std::string_view xml);
[[nodiscard]] CatalogLoadResult loadCatalog(const std::filesystem::path& path);

}  // namespace ev::catalog
