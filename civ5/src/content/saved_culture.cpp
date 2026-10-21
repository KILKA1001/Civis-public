#include "civ5/content/saved_culture.hpp"
#include <stdexcept>

namespace civ5::content {
std::optional<std::int64_t> saved_great_work_class(const RulesDatabase &rules,
                                                   const saves::GreatWorkObservation &work) {
    if (work.class_type)
        return work.class_type;
    std::optional<std::int64_t> result;
    // CultureHelpers::GetGreatWorkClass reads the first matching row. Keep
    // both the bound type and result in host width, without the original int cast.
    rules.query({"SELECT GreatWorkClasses.ID from GreatWorkClasses inner join GreatWorks "
                 "on GreatWorkClassType = GreatWorkClasses.Type where GreatWorks.ID = ?",
                 {work.type}},
                [&](auto row) {
                    if (result)
                        return;
                    const auto id = std::get_if<std::int64_t>(&row[0]);
                    if (!id)
                        throw std::runtime_error("Non-integer great work class in selected rules");
                    result = *id;
                });
    return result;
}
} // namespace civ5::content
