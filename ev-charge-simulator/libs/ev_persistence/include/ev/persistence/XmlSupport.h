#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ev/domain/Model.h"
#include "ev/domain/ValidationError.h"

namespace tinyxml2 {
class XMLElement;
}

namespace ev::persistence {

inline constexpr std::uintmax_t kMaxFileBytes = 1024U * 1024U;  ///< 1 MB limit (spec 6.3).
inline constexpr int kMaxElementDepth = 8;

/// Locale-independent number parsing. "7,4" is normalized to 7.4 (ADR-0005); anything ambiguous
/// such as "1,234.5" or "1,2,3" is rejected.
struct ParsedNumber {
    std::optional<double> value;
    bool normalizedComma = false;
};
[[nodiscard]] ParsedNumber parseCanonicalNumber(std::string_view text);

/// Shortest round-trip representation with '.' as decimal separator.
[[nodiscard]] std::string formatCanonicalNumber(double value);

struct FileReadResult {
    std::optional<std::string> content;
    std::string error;
};
/// Rejects files larger than maxBytes before reading them (IT-006).
[[nodiscard]] FileReadResult readFileLimited(const std::filesystem::path& path, std::uintmax_t maxBytes);

/// Writes to `<path>.tmp` and renames over `path`, so a failed write never corrupts the previous file.
[[nodiscard]] bool writeFileAtomically(const std::filesystem::path& path, const std::string& content);

[[nodiscard]] int elementDepth(const tinyxml2::XMLElement* element);

/// Reads attributes/text from XML, collecting IO-001 errors (with the XML path) and warnings.
class XmlFieldReader {
public:
    std::optional<double> number(const tinyxml2::XMLElement* element, const char* attribute,
                                 const std::string& path, bool required = true);
    std::optional<double> numberText(const tinyxml2::XMLElement* element, const char* expectedUnit,
                                     const std::string& path);
    std::optional<std::string> text(const tinyxml2::XMLElement* element, const char* attribute,
                                    const std::string& path, bool required = true);
    void fail(const std::string& path, const std::string& detail);
    void warnUnknownChildren(const tinyxml2::XMLElement* parent, const std::vector<std::string>& known,
                             const std::string& path);

    std::vector<domain::ValidationError> errors;
    std::vector<domain::Warning> warnings;

private:
    std::optional<double> convert(const char* raw, const std::string& path);
};

}  // namespace ev::persistence
