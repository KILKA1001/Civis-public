#pragma once
#include "civ5/content/research_rules.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <span>
#include <stdexcept>
#include <string>
#include <variant>

namespace civ5::runtime::research_binding_detail {
inline std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty()||!std::holds_alternative<std::int64_t>(entry->lookup.value))
        throw std::runtime_error("Unresolved research define: "+std::string(name));
    return std::get<std::int64_t>(entry->lookup.value);
}
// Exact signed average without overflowing the sum, even at int64 extremes.
inline std::int64_t average(std::span<const std::int64_t> values) {
    if(values.empty()||values.size()>INT64_MAX)throw std::runtime_error("Invalid handicap average size");
    const auto n=static_cast<std::int64_t>(values.size());
    std::int64_t q=0,r=0;
    for(auto v:values) {
        q=wide::add(q,v/n);
        const auto s=v%n;
        if(r>=0&&s>=0&&r>=n-s){++q;r-=n-s;}
        else if(r<=0&&s<=0&&r<=-n-s){--q;r+=n+s;}
        else r+=s;
    }
    if(q>0&&r<0)--q;
    else if(q<0&&r>0)++q;
    return q;
}
} // namespace civ5::runtime::research_binding_detail
