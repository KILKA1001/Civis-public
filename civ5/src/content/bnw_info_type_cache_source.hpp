#pragma once
#include "civ5/content/rules_database.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <thread>

namespace civ5::content::info_type_cache_detail {
// Source algorithm only. Neither this state nor its journal grants native GC
// stage permission. The genuine preceding Validate owner is still required.
struct TableVisit {
    enum class Failure { none, prepare, step };
    std::string name, sql, error;
    std::uint64_t rows = 0;
    bool checked_columns = false, eligible = false;
    Failure failure = Failure::none;
};
struct EnumerationReport {
    bool reset_performed = false, inventory_exhausted = false;
    TableVisit::Failure inventory_failure = TableVisit::Failure::none;
    std::string inventory_error;
    std::uint64_t rows = 0, null_types = 0;
    std::vector<TableVisit> tables;
};
struct Budget {
    std::uint64_t tables = 1024 * 1024, rows = 16 * 1024 * 1024;
};
class Operation;
class LeafState final {
  public:
    LeafState() = default;
    LeafState(const LeafState&) = delete;
    LeafState& operator=(const LeafState&) = delete;
    LeafState(LeafState&&) = delete;
    LeafState& operator=(LeafState&&) = delete;
    std::int32_t lookup(const char* name) const;
    std::int32_t lookup(std::string_view name) const;
    std::int32_t lookup_hash(std::uint32_t hash) const;
    const std::map<std::string,InfoTypeEntry,std::less<>>& names() const;
    const std::map<std::uint32_t,InfoTypeEntry>& hashes() const;
    const std::vector<InfoTypeEntry>& writes() const;
    const EnumerationReport& report() const;
  private:
    friend class Operation;
    void check() const;
    const std::thread::id thread_ = std::this_thread::get_id();
    mutable std::atomic<bool> poisoned_{false};
    bool active_ = false;
    std::shared_ptr<const RulesDatabase> database_;
    std::function<bool()> current_;
    std::map<std::string,InfoTypeEntry,std::less<>> names_;
    std::map<std::uint32_t,InfoTypeEntry> hashes_;
    std::vector<InfoTypeEntry> writes_;
    EnumerationReport report_;
};
// SQL facade is retained by value and then by the leaf, including partial
// failure. No external callback or SQL row reference crosses this operation.
// SQL prepare/step failure retains source writes and continues later tables;
// host budgets/allocations/currentness failures propagate without rollback.
void reset_and_enumerate(std::shared_ptr<const RulesDatabase>, LeafState&, const Budget& = {});
}