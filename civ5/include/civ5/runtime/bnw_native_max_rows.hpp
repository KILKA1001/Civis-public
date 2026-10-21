#pragma once

#include "civ5/content/rules_database.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime::detail {
// CvDatabaseUtility::MaxRows uses rowid, not authored ID or Prefetch slots.
// This read result grants no GC collection/initialized owner authority.
inline std::int32_t bnw_native_max_rows(const content::RulesDatabase& rules,
                                      std::string_view table,
                                      std::int32_t allocation_budget=1024*1024) {
    if(table!="Flavors" && table!="BuildingClasses" && table!="UnitClasses" && table!="Traits")
        throw std::invalid_argument("BNW native MaxRows table outside bounded scope");
    std::int32_t result=0;
    const auto report=rules.query_rows({"SELECT max(rowid) from "+std::string(table),{}},
        [&](const content::RulesRowView& row) {
            // GetInt(NULL) is0, so the aggregate over an empty table yields1.
            const auto value=row.integer_at(0);
            if(value<std::numeric_limits<std::int32_t>::min() ||
               value>=std::numeric_limits<std::int32_t>::max())
                throw std::invalid_argument("BNW native MaxRows GetInt/add unavailable");
            result=static_cast<std::int32_t>(value)+1;
        },1);
    if(!report.all_succeeded() || report.rows!=1)
        throw std::invalid_argument("BNW native MaxRows query unavailable");
    if(result<0 || allocation_budget<0 || result>allocation_budget)
        throw std::invalid_argument("BNW native MaxRows allocation unavailable");
    return result;
}
} // namespace civ5::runtime::detail
