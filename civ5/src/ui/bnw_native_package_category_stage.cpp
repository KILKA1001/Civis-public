#include "bnw_native_package_category_stage.hpp"
#include <stdexcept>
namespace civ5::ui::detail {
CategoryReplaceDecision category_record_replacement(std::uint32_t flags,
    const CategoryCachedArchiveInput& old,std::optional<std::int64_t> incoming){
    if(!old.old_is_archive)return CategoryReplaceDecision::unknown_mandatory_input;
    if((flags&1U) && !*old.old_is_archive)return CategoryReplaceDecision::replace;
    const auto masked=flags&14U;
    if(masked==0)return CategoryReplaceDecision::preserve;
    if(masked==4)return *old.old_is_archive?CategoryReplaceDecision::replace:CategoryReplaceDecision::preserve;
    if(masked!=6)return CategoryReplaceDecision::replace;
    if(!*old.old_is_archive)return CategoryReplaceDecision::preserve;
    if(!old.source_cache_present)return CategoryReplaceDecision::unknown_mandatory_input;
    if(!*old.source_cache_present)return CategoryReplaceDecision::replace;
    if(!old.archive_found)return CategoryReplaceDecision::unknown_mandatory_input;
    if(!*old.archive_found)return CategoryReplaceDecision::replace;
    if(!incoming || !old.original_cached_mtime)return CategoryReplaceDecision::unknown_mandatory_input;
    //83B17F signed high DWORD, then unsigned low DWORD including equality.
    return *incoming>=*old.original_cached_mtime?CategoryReplaceDecision::replace:CategoryReplaceDecision::preserve;
}
CategoryLoosePayloadFields require_category_loose_fields(const CategoryLooseStatInput& stat){
    if(stat.result!=0 || !stat.original_size || !stat.original_mtime)
        throw std::runtime_error("Native unchecked loose stat outputs unknown: mandatory category payload fields unavailable");
    return {*stat.original_size,*stat.original_mtime};
}
std::vector<CategorySourceStep> category_source_plan(const CategorySourcePlanInput& input){
    if(input.category>1)throw std::runtime_error("Native category0/1 source scope required");
    if(input.invalid_flag40)return {};
    if(input.existing_pair)return {{CategorySourcePhase::enable_existing_pair,1}};
    if(input.low_quality)throw std::runtime_error("Native category second DX9_LOW source/cache stage required");
    std::vector<CategorySourceStep> out{{CategorySourcePhase::create_primary_even_empty_paks,0},
        {CategorySourcePhase::load_declared_paks_dx9_then_common,4},
        {CategorySourcePhase::collect_other_category_exclusions,0}};
    if(input.declared_directory_count)
        out.push_back({CategorySourcePhase::scan_each_declared_directory_loose_then_top_level_fpks,7});
    else if(input.category==0)
        out.push_back({CategorySourcePhase::scan_package_root_loose_then_top_level_fpks_for_empty_common,7});
    out.push_back({CategorySourcePhase::scan_explicit_files,7});
    out.push_back({CategorySourcePhase::set_archive_preferences_false,0});
    out.push_back({CategorySourcePhase::publish_pair_name_group0_priorities01,0});
    return out;
}
}
