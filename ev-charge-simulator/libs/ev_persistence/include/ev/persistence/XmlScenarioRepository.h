#pragma once

#include <string>
#include <string_view>

#include "ev/domain/Ports.h"

namespace ev::persistence {

inline constexpr const char* kSchemaVersion = "1.0";

/// Versioned XML scenario persistence (RF-009, spec 6.2/6.3) using TinyXML-2.
class XmlScenarioRepository final : public domain::IScenarioRepository {
public:
    [[nodiscard]] std::vector<domain::ValidationError> save(const domain::ChargeRequest& scenario,
                                                            const std::filesystem::path& path) const override;
    [[nodiscard]] domain::ScenarioLoadResult load(const std::filesystem::path& path) const override;

    /// Parses into a temporary object; the caller's active scenario is never touched (RNF-002).
    [[nodiscard]] static domain::ScenarioLoadResult parse(std::string_view xml);
    [[nodiscard]] static std::string serialize(const domain::ChargeRequest& scenario);
};

}  // namespace ev::persistence
