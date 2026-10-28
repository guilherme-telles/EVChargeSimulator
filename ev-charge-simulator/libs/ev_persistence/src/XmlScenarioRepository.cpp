#include "ev/persistence/XmlScenarioRepository.h"

#include <tinyxml2.h>

#include <charconv>
#include <cstring>

#include "ev/domain/Validation.h"
#include "ev/persistence/XmlSupport.h"

namespace ev::persistence {

using namespace ev::domain;
using tinyxml2::XMLElement;

namespace {

ScenarioLoadResult failure(const std::string& path, const std::string& detail) {
    ScenarioLoadResult result;
    result.errors.push_back(makeError(codes::kInvalidFile, {path}, detail));
    return result;
}

std::string versionDetail(const std::string& version) {
    double major = 0.0;
    const auto [ptr, ec] = std::from_chars(version.data(), version.data() + version.size(), major);
    (void)ptr;
    if (ec == std::errc{} && major > 1.0) {
        return "schemaVersion " + version + " é mais recente que a suportada (" + kSchemaVersion +
               "); atualize o aplicativo.";
    }
    return "schemaVersion " + version + " não suportada; versão esperada: " + kSchemaVersion + ".";
}

void readVehicle(const XMLElement* scenario, ChargeRequest& request, XmlFieldReader& reader) {
    const XMLElement* vehicle = scenario->FirstChildElement("vehicle");
    if (vehicle == nullptr) {
        reader.fail("scenario/vehicle", "elemento obrigatório ausente");
        return;
    }
    reader.warnUnknownChildren(vehicle, {"usableCapacity", "maxChargePower", "consumption", "ratedRange"},
                               "scenario/vehicle");
    request.vehicle.id = reader.text(vehicle, "id", "scenario/vehicle@id", false).value_or("manual");
    request.vehicle.name = reader.text(vehicle, "name", "scenario/vehicle@name", false).value_or("");
    if (auto v = reader.numberText(vehicle->FirstChildElement("usableCapacity"), "kWh",
                                   "scenario/vehicle/usableCapacity")) {
        request.battery.usableCapacity = KilowattHours{*v};
    }
    if (auto v = reader.numberText(vehicle->FirstChildElement("maxChargePower"), "kW",
                                   "scenario/vehicle/maxChargePower")) {
        request.vehicleMaxPower = Kilowatts{*v};
    }
    if (auto v = reader.numberText(vehicle->FirstChildElement("consumption"), "kWh/100km",
                                   "scenario/vehicle/consumption")) {
        request.efficiency.consumptionKwhPer100Km = *v;
    }
    if (const XMLElement* range = vehicle->FirstChildElement("ratedRange")) {
        if (auto v = reader.numberText(range, "km", "scenario/vehicle/ratedRange")) {
            request.efficiency.ratedRange = Kilometers{*v};
        }
    }
}

void readCharger(const XMLElement* scenario, ChargeRequest& request, XmlFieldReader& reader) {
    const XMLElement* charger = scenario->FirstChildElement("charger");
    if (charger == nullptr) {
        reader.fail("scenario/charger", "elemento obrigatório ausente");
        return;
    }
    if (auto type = reader.text(charger, "type", "scenario/charger@type")) {
        if (*type == "AC") {
            request.charger.type = ChargerType::AC;
        } else if (*type == "DC") {
            request.charger.type = ChargerType::DC;
        } else {
            reader.fail("scenario/charger@type", "tipo deve ser AC ou DC");
        }
    }
    if (auto v = reader.number(charger, "powerKw", "scenario/charger@powerKw")) {
        request.charger.ratedPower = Kilowatts{*v};
    }
    if (auto v = reader.number(charger, "installationLimitKw", "scenario/charger@installationLimitKw", false)) {
        request.charger.installationLimit = Kilowatts{*v};
    }
}

void readBattery(const XMLElement* scenario, ChargeRequest& request, XmlFieldReader& reader) {
    const XMLElement* battery = scenario->FirstChildElement("battery");
    if (battery == nullptr) {
        reader.fail("scenario/battery", "elemento obrigatório ausente");
        return;
    }
    if (auto v = reader.number(battery, "currentSoc", "scenario/battery@currentSoc")) {
        request.battery.currentSoc = Percent{*v};
    }
    if (auto v = reader.number(battery, "targetSoc", "scenario/battery@targetSoc")) {
        request.battery.targetSoc = Percent{*v};
    }
    if (auto v = reader.number(battery, "efficiency", "scenario/battery@efficiency")) {
        request.battery.efficiency = *v;
    }
}

void readTariff(const XMLElement* scenario, ChargeRequest& request, XmlFieldReader& reader) {
    const XMLElement* tariff = scenario->FirstChildElement("tariff");
    if (tariff == nullptr) {
        reader.fail("scenario/tariff", "elemento obrigatório ausente");
        return;
    }
    if (auto currency = reader.text(tariff, "currency", "scenario/tariff@currency")) {
        if (*currency != "BRL") {
            reader.fail("scenario/tariff@currency", "somente BRL é suportado no MVP");
        }
        request.tariff.currency = *currency;
    }
    if (auto v = reader.number(tariff, "energyRate", "scenario/tariff@energyRate")) {
        request.tariff.energyRatePerKwh = *v;
    }
    if (auto v = reader.number(tariff, "fixedFee", "scenario/tariff@fixedFee", false)) {
        request.tariff.fixedFee = Brl{*v};
    }
    if (auto v = reader.number(tariff, "minuteRate", "scenario/tariff@minuteRate", false)) {
        request.tariff.ratePerMinute = *v;
    }
}

void readCurve(const XMLElement* scenario, ChargeRequest& request, XmlFieldReader& reader) {
    const XMLElement* curve = scenario->FirstChildElement("chargeCurve");
    if (curve == nullptr) {
        return;
    }
    reader.warnUnknownChildren(curve, {"segment"}, "scenario/chargeCurve");
    int index = 0;
    for (auto* seg = curve->FirstChildElement("segment"); seg != nullptr; seg = seg->NextSiblingElement("segment")) {
        const std::string path = "scenario/chargeCurve/segment[" + std::to_string(index++) + "]";
        auto from = reader.number(seg, "fromSoc", path + "@fromSoc");
        auto to = reader.number(seg, "toSoc", path + "@toSoc");
        auto power = reader.number(seg, "powerKw", path + "@powerKw");
        if (from && to && power) {
            request.curve.segments.push_back({Percent{*from}, Percent{*to}, Kilowatts{*power}});
        }
    }
}

}  // namespace

ScenarioLoadResult XmlScenarioRepository::parse(std::string_view xml) {
    if (xml.size() > kMaxFileBytes) {
        return failure("scenario", "arquivo maior que o limite de 1 MB");
    }
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.data(), xml.size()) != tinyxml2::XML_SUCCESS) {
        return failure("scenario", std::string("XML malformado: ") + doc.ErrorStr());
    }
    const XMLElement* root = doc.RootElement();
    if (root == nullptr || std::strcmp(root->Name(), "scenario") != 0) {
        return failure("scenario", "elemento raiz <scenario> ausente");
    }
    if (elementDepth(root) > kMaxElementDepth) {
        return failure("scenario", "profundidade de elementos acima do limite");
    }
    const char* version = root->Attribute("schemaVersion");
    if (version == nullptr) {
        return failure("scenario@schemaVersion", "atributo obrigatório ausente");
    }
    if (std::string(version) != kSchemaVersion) {
        return failure("scenario@schemaVersion", versionDetail(version));
    }

    XmlFieldReader reader;
    reader.warnUnknownChildren(root, {"vehicle", "charger", "battery", "tariff", "chargeCurve"}, "scenario");
    ChargeRequest request;
    readVehicle(root, request, reader);
    readCharger(root, request, reader);
    readBattery(root, request, reader);
    readTariff(root, request, reader);
    readCurve(root, request, reader);

    ScenarioLoadResult result;
    result.warnings = std::move(reader.warnings);
    if (!reader.errors.empty()) {
        result.errors = std::move(reader.errors);
        return result;
    }
    auto semanticErrors = validate(request);
    if (!semanticErrors.empty()) {
        result.errors.push_back(makeError(codes::kInvalidFile, {"scenario"}, "valores inválidos no cenário"));
        result.errors.insert(result.errors.end(), semanticErrors.begin(), semanticErrors.end());
        return result;
    }
    result.scenario = std::move(request);
    return result;
}

std::string XmlScenarioRepository::serialize(const ChargeRequest& scenario) {
    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());
    XMLElement* root = doc.NewElement("scenario");
    root->SetAttribute("schemaVersion", kSchemaVersion);
    doc.InsertEndChild(root);

    const auto num = [](double v) { return formatCanonicalNumber(v); };
    const auto addValue = [&](XMLElement* parent, const char* name, const char* unit, double value) {
        XMLElement* e = parent->InsertNewChildElement(name);
        e->SetAttribute("unit", unit);
        e->SetText(num(value).c_str());
    };

    XMLElement* vehicle = root->InsertNewChildElement("vehicle");
    vehicle->SetAttribute("id", scenario.vehicle.id.c_str());
    if (!scenario.vehicle.name.empty()) {
        vehicle->SetAttribute("name", scenario.vehicle.name.c_str());
    }
    addValue(vehicle, "usableCapacity", "kWh", scenario.battery.usableCapacity.value());
    addValue(vehicle, "maxChargePower", "kW", scenario.vehicleMaxPower.value());
    addValue(vehicle, "consumption", "kWh/100km", scenario.efficiency.consumptionKwhPer100Km);
    if (scenario.efficiency.ratedRange) {
        addValue(vehicle, "ratedRange", "km", scenario.efficiency.ratedRange->value());
    }

    XMLElement* charger = root->InsertNewChildElement("charger");
    charger->SetAttribute("type", scenario.charger.type == ChargerType::DC ? "DC" : "AC");
    charger->SetAttribute("powerKw", num(scenario.charger.ratedPower.value()).c_str());
    if (scenario.charger.installationLimit) {
        charger->SetAttribute("installationLimitKw", num(scenario.charger.installationLimit->value()).c_str());
    }

    XMLElement* battery = root->InsertNewChildElement("battery");
    battery->SetAttribute("currentSoc", num(scenario.battery.currentSoc.value()).c_str());
    battery->SetAttribute("targetSoc", num(scenario.battery.targetSoc.value()).c_str());
    battery->SetAttribute("efficiency", num(scenario.battery.efficiency).c_str());

    XMLElement* tariff = root->InsertNewChildElement("tariff");
    tariff->SetAttribute("currency", scenario.tariff.currency.c_str());
    tariff->SetAttribute("energyRate", num(scenario.tariff.energyRatePerKwh).c_str());
    tariff->SetAttribute("fixedFee", num(scenario.tariff.fixedFee.value()).c_str());
    tariff->SetAttribute("minuteRate", num(scenario.tariff.ratePerMinute).c_str());

    if (scenario.curve.enabled()) {
        XMLElement* curve = root->InsertNewChildElement("chargeCurve");
        for (const auto& segment : scenario.curve.segments) {
            XMLElement* seg = curve->InsertNewChildElement("segment");
            seg->SetAttribute("fromSoc", num(segment.fromSoc.value()).c_str());
            seg->SetAttribute("toSoc", num(segment.toSoc.value()).c_str());
            seg->SetAttribute("powerKw", num(segment.maxPower.value()).c_str());
        }
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    return std::string(printer.CStr());
}

std::vector<ValidationError> XmlScenarioRepository::save(const ChargeRequest& scenario,
                                                         const std::filesystem::path& path) const {
    auto errors = validate(scenario);
    if (!errors.empty()) {
        return errors;
    }
    if (!writeFileAtomically(path, serialize(scenario))) {
        return {makeError(codes::kSaveFailed, {}, "falha ao gravar o arquivo")};
    }
    return {};
}

ScenarioLoadResult XmlScenarioRepository::load(const std::filesystem::path& path) const {
    auto file = readFileLimited(path, kMaxFileBytes);
    if (!file.content) {
        return failure("scenario", file.error);
    }
    return parse(*file.content);
}

}  // namespace ev::persistence
