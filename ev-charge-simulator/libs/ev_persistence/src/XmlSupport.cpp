#include "ev/persistence/XmlSupport.h"

#include <tinyxml2.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <system_error>

namespace ev::persistence {

using domain::codes::kInvalidFile;

ParsedNumber parseCanonicalNumber(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\n')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\n')) {
        text.remove_suffix(1);
    }
    if (text.empty()) {
        return {};
    }
    std::string buffer(text);
    const auto commas = std::count(buffer.begin(), buffer.end(), ',');
    const auto dots = std::count(buffer.begin(), buffer.end(), '.');
    bool normalized = false;
    if (commas > 0) {
        if (commas != 1 || dots != 0) {
            return {};
        }
        std::replace(buffer.begin(), buffer.end(), ',', '.');
        normalized = true;
    }
    double value = 0.0;
    const char* begin = buffer.data();
    const char* end = begin + buffer.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end || !std::isfinite(value)) {
        return {};
    }
    return ParsedNumber{value, normalized};
}

std::string formatCanonicalNumber(double value) {
    char buffer[64];
    const auto [ptr, ec] = std::to_chars(std::begin(buffer), std::end(buffer), value);
    return ec == std::errc{} ? std::string(buffer, ptr) : std::string("0");
}

FileReadResult readFileLimited(const std::filesystem::path& path, std::uintmax_t maxBytes) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return {std::nullopt, "arquivo não encontrado ou inacessível"};
    }
    if (size > maxBytes) {
        return {std::nullopt, "arquivo maior que o limite de 1 MB"};
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {std::nullopt, "não foi possível ler o arquivo"};
    }
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return {std::move(content), {}};
}

bool writeFileAtomically(const std::filesystem::path& path, const std::string& content) {
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream stream(tmp, std::ios::binary | std::ios::trunc);
        if (!stream) {
            return false;
        }
        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        stream.flush();
        if (!stream) {
            std::error_code ignored;
            std::filesystem::remove(tmp, ignored);
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::error_code ignored;
        std::filesystem::remove(tmp, ignored);
        return false;
    }
    return true;
}

int elementDepth(const tinyxml2::XMLElement* element) {
    if (element == nullptr) {
        return 0;
    }
    int deepestChild = 0;
    for (auto* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        deepestChild = std::max(deepestChild, elementDepth(child));
        if (deepestChild > kMaxElementDepth) {
            break;
        }
    }
    return 1 + deepestChild;
}

void XmlFieldReader::fail(const std::string& path, const std::string& detail) {
    errors.push_back(domain::makeError(kInvalidFile, {path}, detail));
}

std::optional<double> XmlFieldReader::convert(const char* raw, const std::string& path) {
    const auto parsed = parseCanonicalNumber(raw);
    if (!parsed.value) {
        fail(path, "número inválido ou ambíguo: '" + std::string(raw) + "'");
        return std::nullopt;
    }
    if (parsed.normalizedComma) {
        warnings.push_back({domain::codes::kWarnDecimalComma,
                            "Vírgula decimal normalizada para ponto em " + path + "."});
    }
    return parsed.value;
}

std::optional<double> XmlFieldReader::number(const tinyxml2::XMLElement* element, const char* attribute,
                                             const std::string& path, bool required) {
    const char* raw = element->Attribute(attribute);
    if (raw == nullptr) {
        if (required) {
            fail(path, "atributo obrigatório ausente");
        }
        return std::nullopt;
    }
    return convert(raw, path);
}

std::optional<double> XmlFieldReader::numberText(const tinyxml2::XMLElement* element, const char* expectedUnit,
                                                 const std::string& path) {
    if (element == nullptr) {
        fail(path, "elemento obrigatório ausente");
        return std::nullopt;
    }
    const char* unit = element->Attribute("unit");
    if (unit != nullptr && std::string(unit) != expectedUnit) {
        fail(path + "@unit", "unidade inesperada '" + std::string(unit) + "', esperado '" + expectedUnit + "'");
        return std::nullopt;
    }
    const char* raw = element->GetText();
    if (raw == nullptr) {
        fail(path, "valor ausente");
        return std::nullopt;
    }
    return convert(raw, path);
}

std::optional<std::string> XmlFieldReader::text(const tinyxml2::XMLElement* element, const char* attribute,
                                                const std::string& path, bool required) {
    const char* raw = element->Attribute(attribute);
    if (raw == nullptr) {
        if (required) {
            fail(path, "atributo obrigatório ausente");
        }
        return std::nullopt;
    }
    return std::string(raw);
}

void XmlFieldReader::warnUnknownChildren(const tinyxml2::XMLElement* parent, const std::vector<std::string>& known,
                                         const std::string& path) {
    for (auto* child = parent->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (std::find(known.begin(), known.end(), child->Name()) == known.end()) {
            warnings.push_back({domain::codes::kWarnUnknownElement,
                                "Elemento desconhecido ignorado: " + path + "/" + child->Name() + "."});
        }
    }
}

}  // namespace ev::persistence
