#include "ev/domain/ValidationError.h"

#include <map>
#include <utility>

namespace ev::domain {

std::string messageFor(const std::string& code) {
    static const std::map<std::string, std::string> kMessages = {
        {codes::kSocOutOfRange, "Informe um nível de bateria entre 0% e 100%."},
        {codes::kTargetNotAboveCurrent, "O nível desejado deve ser maior que o nível atual."},
        {codes::kPowerNotPositive, "Informe uma potência disponível maior que zero."},
        {codes::kEfficiencyOutOfRange, "Informe eficiência entre 50% e 100%."},
        {codes::kConsumptionNotPositive, "Informe um consumo médio maior que zero (kWh/100 km)."},
        {codes::kCapacityNotPositive, "Informe uma capacidade útil de bateria maior que zero."},
        {codes::kNegativeTariff, "Valores de tarifa não podem ser negativos."},
        {codes::kRatedRangeNotPositive, "Informe uma autonomia nominal maior que zero ou deixe em branco."},
        {codes::kInvalidCurve,
         "Faixas da curva de carga inválidas: use faixas entre 0% e 100%, com início menor que o fim e sem "
         "sobreposição."},
        {codes::kInvalidFile, "Não foi possível abrir o cenário. Verifique o formato e a versão do arquivo."},
        {codes::kSaveFailed, "Não foi possível salvar o cenário. O arquivo anterior foi preservado."},
        {codes::kNonFiniteResult, "Não foi possível calcular com os valores informados. Revise os campos."},
    };
    const auto it = kMessages.find(code);
    return it != kMessages.end() ? it->second : std::string{};
}

ValidationError makeError(const std::string& code, std::vector<std::string> fields, std::string detail) {
    return ValidationError{code, std::move(fields), messageFor(code), std::move(detail)};
}

}  // namespace ev::domain
