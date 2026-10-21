#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace civ5::ui::detail {
//PRIVATE source algorithm/fixtures; scalar inputs NEVER authenticate an actual
//archive cache, namespace, original stat owner, FIRST map or resource table.
enum class CategoryReplaceDecision { preserve,replace,unknown_mandatory_input };
struct CategoryCachedArchiveInput {
    std::optional<bool> old_is_archive,source_cache_present,archive_found;
    std::optional<std::int64_t> original_cached_mtime;
};
CategoryReplaceDecision category_record_replacement(std::uint32_t flags,
    const CategoryCachedArchiveInput&,std::optional<std::int64_t> original_incoming_mtime);
struct CategoryLooseStatInput {
    int result=-1;
    std::optional<std::int32_t> original_size;
    std::optional<std::int64_t> original_mtime;
};
struct CategoryLoosePayloadFields {std::int32_t size;std::int64_t mtime;};
CategoryLoosePayloadFields require_category_loose_fields(const CategoryLooseStatInput&);
constexpr std::uint32_t category_loose_flags(std::uint32_t original_flags,bool override_pattern_matches){
    return override_pattern_matches?(original_flags&0xfffffff2U)|2U:original_flags;
}
enum class CategorySourcePhase {
    create_primary_even_empty_paks,load_declared_paks_dx9_then_common,
    collect_other_category_exclusions,scan_each_declared_directory_loose_then_top_level_fpks,
    scan_package_root_loose_then_top_level_fpks_for_empty_common,
    scan_explicit_files,set_archive_preferences_false,publish_pair_name_group0_priorities01,
    enable_existing_pair
};
struct CategorySourceStep {CategorySourcePhase phase;std::uint32_t flags;};
struct CategorySourcePlanInput {
    unsigned category=0;bool invalid_flag40=false,existing_pair=false,low_quality=false;
    std::size_t declared_directory_count=0;
};
//Pure literal source program; publication is an instruction in the plan only,
//not an operation on a live table and not a completed source-owner receipt.
std::vector<CategorySourceStep> category_source_plan(const CategorySourcePlanInput&);
}
