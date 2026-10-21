#pragma once
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace civ5::content {
enum class GlobalDefineKind { integer, source_float, real, text };
enum class GlobalDefineProfile { bnw, vp542 };
using GlobalDefineValue = std::variant<std::int64_t, double, std::string>;
struct GlobalDefineLookup {
    GlobalDefineValue value = std::int64_t{0};
    bool found = false;
    int sqlite_code = 0;
    std::string error;
};
struct GlobalDefineSpec {
    std::string_view name;
    GlobalDefineKind kind;
    std::int64_t initial_integer = 0;
    double initial_real = 0;
    bool zero_is_none = false;
};
std::span<const GlobalDefineSpec> global_define_specs(GlobalDefineProfile profile);
struct CachedGlobalDefine {
    std::string name;
    GlobalDefineKind kind;
    GlobalDefineLookup lookup;
    bool retained_previous = false;
};
// Owns cached values independently of the database. Native float precision is
// explicit; source_float values are stored in double after source rounding.
// Integer values and indices do not inherit the source GetInt/x86 width.
class GlobalDefineCache {
  public:
    GlobalDefineProfile profile() const { return profile_; }
    std::span<const CachedGlobalDefine> entries() const { return entries_; }
    const CachedGlobalDefine *find(std::string_view name) const;
    std::uint64_t missing() const;
    std::uint64_t errors() const;
  private:
    friend class RulesDatabase;
    GlobalDefineProfile profile_ = GlobalDefineProfile::bnw;
    std::vector<CachedGlobalDefine> entries_;
    std::map<std::string, std::size_t, std::less<>> names_;
};
} // namespace civ5::content
