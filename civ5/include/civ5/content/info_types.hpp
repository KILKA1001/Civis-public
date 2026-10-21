#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::content {
// FStringA::Hash is a wire identity, not a host index: CRC32 with initial
// all-ones and no final complement. Embedded NUL is rejected by this API.
std::uint32_t saved_info_type_hash(std::string_view type);
struct InfoTypeEntry {
    std::string type, table;
    std::int64_t id = 0;
    std::uint32_t hash = 0;
};
struct InfoTypeCollision {
    InfoTypeEntry previous, replacement;
};
struct InfoTypeReadError {
    std::string table, message;
    int sqlite_code = 0;
};
struct InfoTypeReport {
    std::uint64_t tables = 0, eligible_tables = 0, rows = 0, null_types = 0;
    std::uint64_t name_overwrites = 0, hash_collisions = 0;
    std::vector<InfoTypeCollision> collision_samples;
    std::vector<InfoTypeReadError> errors;
    bool all_succeeded() const { return errors.empty(); }
};
struct InfoTypeBudget {
    std::uint64_t tables = 1024 * 1024, rows = 16 * 1024 * 1024;
    std::uint64_t collision_samples = 16;
};
// Immutable snapshot of GC's cross-table maps. Both name duplicates and CRC
// collisions keep the last visited ID. No allocation depends on MAX(ID).
// Rebuild after changing rules; retained snapshots keep their prior values.
class InfoTypeCatalog {
  public:
    const InfoTypeEntry *find(std::string_view type) const;
    const InfoTypeEntry *find(const char *type) const { return type ? find(std::string_view(type)) : nullptr; }
    const InfoTypeEntry *find_hash(std::uint64_t hash) const;
    const InfoTypeReport &report() const { return report_; }
    std::uint64_t names() const { return names_.size(); }
    std::uint64_t hashes() const { return hashes_.size(); }
    const std::map<std::string,InfoTypeEntry,std::less<>> &entries() const { return names_; }

  private:
    friend class RulesDatabase;
    void insert(InfoTypeEntry entry, const InfoTypeBudget &budget);
    InfoTypeReport report_;
    std::map<std::string, InfoTypeEntry, std::less<>> names_;
    std::map<std::uint32_t, InfoTypeEntry> hashes_;
};
} // namespace civ5::content
