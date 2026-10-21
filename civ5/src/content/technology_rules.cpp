#include "civ5/content/technology_rules.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::content {
namespace {
#define TECH_INT(name) TechnologyField{name, TechnologyFieldKind::integer},
#define TECH_BOOL(name) TechnologyField{name, TechnologyFieldKind::boolean},
constexpr TechnologyField bnw_fields[]{
#include "technology_fields_bnw.inc"
};
constexpr TechnologyField vp_fields[]{
#include "technology_fields_vp542.inc"
};
#undef TECH_INT
#undef TECH_BOOL
}
std::span<const TechnologyField> technology_fields(TechnologyProfile profile) {
    switch (profile) {
    case TechnologyProfile::bnw: return bnw_fields;
    case TechnologyProfile::vp542: return vp_fields;
    }
    throw std::runtime_error("Unknown technology profile");
}
std::int64_t RuleValues::at(std::uint64_t index) const {
    if (index >= extent) throw std::out_of_range("Rules array index");
    const auto value = values.find(index);
    return value == values.end() ? default_value : value->second;
}
std::int64_t RuleMatrix::at(std::uint64_t row, std::uint64_t column) const {
    if (row >= rows || column >= columns) throw std::out_of_range("Rules matrix index");
    const auto value = values.find({row, column});
    return value == values.end() ? 0 : value->second;
}
const TechnologyInfo *TechnologyRules::find(std::uint64_t slot) const {
    const auto value = entries.find(slot);
    return value == entries.end() ? nullptr : &value->second;
}
TechnologyRules load_technology_rules(const RulesDatabase &rules, const InfoTypeCatalog &types,
                                      TechnologyProfile profile, std::uint64_t minimum_domains,
                                      const InfoLocalizer &localizer, const TechnologyBudget &budget) {
    const auto fields = technology_fields(profile);
    if (!types.report().all_succeeded()) throw std::runtime_error("Technologies need a complete type cache");
    TechnologyRules result;
    result.profile = profile;
    result.localization_supplied = bool(localizer);
    const auto error = [&](std::uint64_t slot, const std::string &stage, const std::string &message, int code = 0) {
        ++result.errors;
        if (result.error_samples.size() < budget.error_samples)
            result.error_samples.push_back({slot, stage, message, code});
    };
    const auto extent = [&](std::uint64_t slot, const std::string &table) {
        std::uint64_t count = 0;
        // These names are internal contract constants. rowid is intentionally
        // distinct from authored ID and PrefetchCollection's logical span.
        const auto query = rules.query_rows({"SELECT max(rowid) FROM " + table, {}}, [&](const auto &row) {
            const auto maximum = row.integer_at(0);
            count = maximum < 0 ? static_cast<std::uint64_t>(maximum + 1)
                                : static_cast<std::uint64_t>(maximum) + 1;
        }, 1);
        if (!query.all_succeeded()) error(slot, "MaxRows:" + table, query.error, query.sqlite_code);
        return count;
    };
    result.collection = rules.prefetch_info("Technologies", [&](auto slot, auto, const RulesRowView &row) {
        const auto errors_before = result.errors;
        TechnologyInfo info;
        info.base = read_info_metadata(row, localizer);
        for (const auto &field : fields) {
            if (field.kind == TechnologyFieldKind::integer)
                info.integers.emplace(field.column, row.integer(field.column));
            else
                info.booleans.emplace(field.column, row.boolean(field.column));
        }
        const auto reference = [&](std::string_view column) {
            const auto name = row.text(column);
            const auto *entry = name ? types.find(*name) : nullptr;
            return entry ? entry->id : std::int64_t{-1};
        };
        info.era = reference("Era");
        info.first_free_unit_class = reference("FirstFreeUnitClass");
        info.quote.key = row.text("Quote").value_or("");
        if (localizer) info.quote.value = localizer(info.quote.key);
        info.audio_intro = row.text("AudioIntro").value_or("");
        info.audio_intro_header = row.text("AudioIntroHeader").value_or("");
        const auto read = [&](const std::string &stage, const std::string &sql, const auto &visit) {
            const auto query = rules.query_rows({sql, {info.base.type}}, [&](const auto &linked) {
                if (result.relation_rows >= budget.relation_rows)
                    throw std::runtime_error("Technology relations exceed host budget");
                ++result.relation_rows;
                visit(linked);
            }, budget.relation_rows - result.relation_rows);
            if (!query.all_succeeded()) error(slot, stage, query.error, query.sqlite_code);
        };
        const auto write = [&](RuleValues &array, std::int64_t id, std::int64_t value,
                               const std::string &stage, std::uint64_t source_extent) {
            if (id < 0) { error(slot, stage, "Negative source array index"); return; }
            const auto index = static_cast<std::uint64_t>(id);
            if (index >= source_extent) error(slot, stage, "Source array index exceeds MaxRows/minimum");
            array.extent = std::max(array.extent, index + 1);
            if (value == array.default_value) array.values.erase(index);
            else array.values[index] = value;
        };
        const auto values = [&](const std::string &type_table, const std::string &data_table,
                                const std::string &type_column, const std::string &value_column,
                                std::uint64_t minimum = 0) {
            RuleValues array;
            array.extent = std::max(extent(slot, type_table), minimum);
            const auto source_extent = array.extent;
            const auto sql = "select " + type_table + ".ID, " + value_column + " from " + data_table +
                " inner join " + type_table + " on " + type_column + " = " + type_table + ".Type where TechType = ?";
            read(data_table, sql, [&](const auto &linked) {
                write(array, linked.integer_at(0), linked.integer_at(1), data_table, source_extent);
            });
            return array;
        };
        info.domain_extra_moves = values("Domains", "Technology_DomainExtraMoves", "DomainType", "Moves", minimum_domains);
        info.trade_route_domain_extra_range = values("Domains", "Technology_TradeRouteDomainExtraRange", "DomainType", "Range", minimum_domains);
        info.free_promotions.extent = extent(slot, "UnitPromotions");
        const auto promotion_extent = info.free_promotions.extent;
        read("Technology_FreePromotions", "select UnitPromotions.ID from Technology_FreePromotions inner join UnitPromotions on PromotionType = UnitPromotions.Type where TechType = ?", [&](const auto &linked) {
            write(info.free_promotions, linked.integer_at(0), 1, "Technology_FreePromotions", promotion_extent);
        });
        info.flavors = values("Flavors", "Technology_Flavors", "FlavorType", "Flavor");
        const auto tech_extent = extent(slot, "Technologies");
        const auto prerequisites = [&](const std::string &table) {
            RuleValues array;
            array.extent = tech_extent;
            array.default_value = -1;
            std::uint64_t index = 0;
            read(table, "select Technologies.ID from " + table + " inner join Technologies on Technologies.Type = PrereqTech where TechType = ?;", [&](const auto &linked) {
                if (index == UINT64_MAX) throw std::runtime_error("Technology prerequisite count overflow");
                if (index >= tech_extent) error(slot, table, "Source prerequisite list exceeds MaxRows");
                const auto id = linked.integer_at(0);
                if (id != -1) array.values[index] = id;
                ++index;
                array.extent = std::max(array.extent, index);
            });
            return array;
        };
        info.or_prerequisites = prerequisites("Technology_ORPrereqTechs");
        info.and_prerequisites = prerequisites("Technology_PrereqTechs");
        if (profile == TechnologyProfile::vp542) {
            auto &matrix = info.specialist_yield_changes.emplace();
            matrix.rows = extent(slot, "Specialists");
            matrix.columns = extent(slot, "Yields");
            const auto source_rows = matrix.rows, source_columns = matrix.columns;
            if (!source_rows || !source_columns) error(slot, "Tech_SpecialistYieldChanges", "Zero source matrix dimension");
            read("Tech_SpecialistYieldChanges", "select Specialists.ID as SpecialistID, Yields.ID as YieldID, Yield from Tech_SpecialistYieldChanges inner join Specialists on Specialists.Type = SpecialistType inner join Yields on Yields.Type = YieldType where TechType = ?", [&](const auto &linked) {
                const auto specialist = linked.integer_at(0), yield = linked.integer_at(1), value = linked.integer_at(2);
                if (specialist < 0 || yield < 0) {
                    error(slot, "Tech_SpecialistYieldChanges", "Negative source matrix index");
                    return;
                }
                const auto i = static_cast<std::uint64_t>(specialist), j = static_cast<std::uint64_t>(yield);
                if (i >= source_rows || j >= source_columns)
                    error(slot, "Tech_SpecialistYieldChanges", "Source matrix index exceeds MaxRows");
                matrix.rows = std::max(matrix.rows, i + 1);
                matrix.columns = std::max(matrix.columns, j + 1);
                if (value == 0) matrix.values.erase({i, j});
                else matrix.values[{i, j}] = value;
            });
        }
        result.entries.emplace(slot, std::move(info));
        return result.errors == errors_before;
    }, {budget.technologies});
    return result;
}
} // namespace civ5::content
