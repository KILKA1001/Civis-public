#include "civ5/ui/bnw_info_default_form.hpp"
#include <stdexcept>
namespace civ5::ui {
std::string bnw_info_default_form(const TopPanelLocalizationCatalog &catalog,std::string_view key) {
    const auto found=catalog.rows.find(std::string(key));
    if (found==catalog.rows.end() || found->second.empty())
        throw std::runtime_error("Selected Info Description localization is absent");
    const auto &source=found->second;
    // SDK CvLocalization.h310/423 defaults UTF8/Compose to form1. The
    // selected EN/RU Info rows encode grammatical alternatives with '|'.
    // Only literal no-argument names are owned by this bounded reader.
    if (source.find('{')!=source.npos || source.find('}')!=source.npos)
        throw std::runtime_error("Selected Info Description needs unsupported argument composition");
    auto result=source.substr(0,source.find('|'));
    if (result.empty() || result==key || result.starts_with("TXT_KEY_"))
        throw std::runtime_error("Selected Info Description first form is unresolved");
    return result;
}
} // namespace civ5::ui
