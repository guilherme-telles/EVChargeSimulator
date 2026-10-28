#include "ev/catalog/XmlVehicleCatalog.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cstring>
#include <set>

#include "ev/persistence/XmlSupport.h"

namespace ev::catalog {

using namespace ev::domain;
using persistence::XmlFieldReader;

namespace {

CatalogLoadResult failure(const std::string& path, const std::string& detail) {
    CatalogLoadResult result;
    result.errors.push_back(makeError(codes::kInvalidFile, {path}, detail));
    return result;
}

std::optional<double> positive(XmlFieldReader& reader, const tinyxml2::XMLElement* e, const char* attr,
                               const std::string& path) {
    auto value = reader.number(e, attr, path + "@" + attr);
    if (value && *value <= 0.0) {
        reader.fail(path + "@" + attr, "valor deve ser maior que zero");
        return std::nullopt;
    }
    return value;
}

}  // namespace

std::optional<Vehicle> XmlVehicleCatalog::find(const std::string& id) const {
    const auto it = std::find_if(vehicles_.begin(), vehicles_.end(), [&](const Vehicle& v) { return v.id == id; });
    return it != vehicles_.end() ? std::optional<Vehicle>(*it) : std::nullopt;
}

CatalogLoadResult parseCatalog(std::string_view xml) {
    if (xml.size() > persistence::kMaxFileBytes) {
        return failure("vehicleCatalog", "arquivo maior que o limite de 1 MB");
    }
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.data(), xml.size()) != tinyxml2::XML_SUCCESS) {
        return failure("vehicleCatalog", std::string("XML malformado: ") + doc.ErrorStr());
    }
    const auto* root = doc.RootElement();
    if (root == nullptr || std::strcmp(root->Name(), "vehicleCatalog") != 0) {
        return failure("vehicleCatalog", "elemento raiz <vehicleCatalog> ausente");
    }
    if (persistence::elementDepth(root) > persistence::kMaxElementDepth) {
        return failure("vehicleCatalog", "profundidade de elementos acima do limite");
    }
    const char* version = root->Attribute("schemaVersion");
    if (version == nullptr || std::string(version) != "1.0") {
        return failure("vehicleCatalog@schemaVersion", "schemaVersion ausente ou não suportada");
    }

    XmlFieldReader reader;
    std::vector<Vehicle> vehicles;
    std::set<std::string> ids;
    std::size_t index = 0;
    for (auto* e = root->FirstChildElement("vehicle"); e != nullptr; e = e->NextSiblingElement("vehicle"), ++index) {
        if (index >= kMaxCatalogVehicles) {
            return failure("vehicleCatalog", "quantidade de veículos acima do limite");
        }
        const std::string path = "vehicleCatalog/vehicle[" + std::to_string(index) + "]";
        Vehicle v;
        v.id = reader.text(e, "id", path + "@id").value_or("");
        v.name = reader.text(e, "name", path + "@name").value_or("");
        v.source = reader.text(e, "source", path + "@source", false).value_or("");
        if (!v.id.empty() && !ids.insert(v.id).second) {
            reader.fail(path + "@id", "id duplicado: " + v.id);
        }
        v.usableCapacity = KilowattHours{positive(reader, e, "usableCapacityKWh", path).value_or(0.0)};
        v.maxAcPower = Kilowatts{positive(reader, e, "maxAcKw", path).value_or(0.0)};
        v.maxDcPower = Kilowatts{positive(reader, e, "maxDcKw", path).value_or(0.0)};
        v.consumptionKwhPer100Km = positive(reader, e, "consumptionKWh100Km", path).value_or(0.0);
        v.ratedRange = Kilometers{positive(reader, e, "ratedRangeKm", path).value_or(0.0)};
        vehicles.push_back(std::move(v));
    }
    if (!reader.errors.empty()) {
        CatalogLoadResult result;
        result.errors = std::move(reader.errors);
        return result;
    }
    return CatalogLoadResult{XmlVehicleCatalog{std::move(vehicles)}, {}};
}

CatalogLoadResult loadCatalog(const std::filesystem::path& path) {
    auto file = persistence::readFileLimited(path, persistence::kMaxFileBytes);
    if (!file.content) {
        return failure("vehicleCatalog", file.error);
    }
    return parseCatalog(*file.content);
}

}  // namespace ev::catalog
