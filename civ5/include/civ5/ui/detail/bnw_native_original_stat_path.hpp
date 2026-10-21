#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
namespace civ5::ui::detail {
// Lexical guard ONLY; no filesystem probes, canonicalization or alias expansion.
// Metadata authority still derives from the private actual original input lease.
inline std::filesystem::path confined_original_stat_path(
    const std::filesystem::path& original_root,const std::filesystem::path& relative){
    const auto refuse=[](){throw std::runtime_error("Native loose stat requires confined original relative source path");};
    if(!original_root.is_absolute() || relative.empty() || relative.has_root_path())refuse();
    for(const auto& component:relative){
        const auto raw=component.native();
        if(component==std::filesystem::path(".."))refuse();
        if(component==std::filesystem::path("."))continue;
        if(raw.empty())continue; //Only a lexical separator; final target still checked.
        if(raw.find(std::filesystem::path::value_type{0})!=decltype(raw)::npos ||
           raw.find(static_cast<std::filesystem::path::value_type>(':'))!=decltype(raw)::npos ||
           raw.back()==static_cast<std::filesystem::path::value_type>('.') ||
           raw.back()==static_cast<std::filesystem::path::value_type>(' '))refuse();
    }
    auto root=original_root.lexically_normal();
    if(root.has_relative_path() && root.filename().empty())root=root.parent_path();
    const auto target=(root/relative).lexically_normal();
    auto candidate=target.begin();
    for(auto expected=root.begin();expected!=root.end();++expected,++candidate)
        if(candidate==target.end() || *candidate!=*expected)refuse();
    if(candidate==target.end())refuse();
    return target;
}
}
