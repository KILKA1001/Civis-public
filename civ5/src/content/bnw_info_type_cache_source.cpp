#include "bnw_info_type_cache_source.hpp"
#include <bit>
#include <stdexcept>

namespace civ5::content::info_type_cache_detail {
namespace {
constexpr std::string_view inventory_sql = "select name from sqlite_master where type = 'table' ";
std::string select_all(std::string_view table) {
    // Database SelectAll uses sqlite3_snprintf(0x400, ..., %q): escaping
    // apostrophes, WITHOUT adding identifier quotes, then byte truncation.
    std::string sql = "select * from ";
    const auto append = [&](char byte) { if (sql.size() < 1023) sql.push_back(byte); };
    for (const char byte : table.substr(0, table.find('\0'))) {
        append(byte);
        if (byte == '\'') append(byte);
    }
    append(' ');
    return sql;
}
std::int32_t get_int(const DatabaseScriptValue& value) {
    const auto raw = std::get<std::int64_t>(value);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(raw));
}
}
void LeafState::check() const {
    // Do not inspect mutable state on a foreign thread.
    if (std::this_thread::get_id() != thread_) {
        poisoned_.store(true);
        throw std::runtime_error("Info type leaf originating thread differs");
    }
    if (poisoned_.load()) throw std::runtime_error("Info type leaf is poisoned");
    if (current_ && !current_()) {
        poisoned_.store(true);
        throw std::runtime_error("Info type leaf SAME immutable SQL scope is stale");
    }
    if (poisoned_.load()) throw std::runtime_error("Info type leaf is poisoned");
}
std::int32_t LeafState::lookup(const char* name) const {
    check();
    return name ? lookup(std::string_view(name)) : -1;
}
std::int32_t LeafState::lookup(std::string_view name) const {
    check();
    name = name.substr(0,name.find('\0'));
    const auto found = names_.find(name);
    return found == names_.end() ? -1 : static_cast<std::int32_t>(found->second.id);
}
std::int32_t LeafState::lookup_hash(std::uint32_t hash) const {
    check(); const auto found = hashes_.find(hash);
    return found == hashes_.end() ? -1 : static_cast<std::int32_t>(found->second.id);
}
const std::map<std::string,InfoTypeEntry,std::less<>>& LeafState::names() const { check(); return names_; }
const std::map<std::uint32_t,InfoTypeEntry>& LeafState::hashes() const { check(); return hashes_; }
const std::vector<InfoTypeEntry>& LeafState::writes() const { check(); return writes_; }
const EnumerationReport& LeafState::report() const { check(); return report_; }

class Operation final {
  public:
    explicit Operation(LeafState& state) : state_(state) {
        state_.check();
        if (state_.active_) {
            state_.poisoned_.store(true);
            throw std::runtime_error("Info type leaf reentry");
        }
        state_.active_ = true;
    }
    ~Operation() { state_.active_ = false; }
    Operation(const Operation&) = delete;
    Operation& operator=(const Operation&) = delete;
    void run(std::shared_ptr<const RulesDatabase> database, const Budget& budget) {
        if (!database) throw std::runtime_error("Info type leaf SQL owner absent");
        auto current = database->immutable_preview_fingerprint_currentness();
        if (current && !current()) throw std::runtime_error("Info type leaf supplied SQL scope is stale");
        state_.database_ = std::move(database);
        state_.current_ = std::move(current);
        // GC.infoTypeFromStringReset: name map, then hash map. A later failure
        // never restores the old registry or turns this into atomic replacement.
        state_.names_.clear(); state_.hashes_.clear();
        state_.writes_.clear(); state_.report_ = {};
        state_.report_.reset_performed = true;
        state_.check();
        auto inventory = state_.database_->script_query(inventory_sql, {});
        if (!inventory) {
            state_.report_.inventory_failure = TableVisit::Failure::prepare;
            return;
        }
        while (inventory->step()) {
            state_.check();
            if (state_.report_.tables.size() >= budget.tables)
                throw std::runtime_error("Info type source table budget exhausted");
            const auto row = inventory->row();
            const auto name = std::get<std::string>(row.at(0).second);
            state_.report_.tables.push_back({name,select_all(name),{},0,false,false,TableVisit::Failure::none});
            auto& visit = state_.report_.tables.back();
            auto rows = state_.database_->script_query(visit.sql, {});
            if (!rows) { visit.failure = TableVisit::Failure::prepare; continue; }
            bool first = true;
            while (rows->step()) {
                state_.check();
                if (state_.report_.rows >= budget.rows)
                    throw std::runtime_error("Info type source row budget exhausted");
                ++state_.report_.rows; ++visit.rows;
                const auto id = rows->cache_key(true);
                if (first && !id.first) { visit.checked_columns = true; break; }
                const auto type = rows->cache_key(false);
                if (first) {
                    visit.checked_columns = true;
                    if (!id.first || !type.first) break;
                    first = false; visit.eligible = true;
                }
                if (std::holds_alternative<std::monostate>(type.second)) {
                    ++state_.report_.null_types; continue;
                }
                // Existing cursor owns GetText's C-string prefix before maps
                // or any subsequent Step can invalidate native SQLite storage.
                const auto text = std::get<std::string>(type.second);
                const auto integer = get_int(id.second);
                InfoTypeEntry entry{text,name,integer,saved_info_type_hash(text)};
                state_.names_.insert_or_assign(entry.type,entry);
                state_.hashes_.insert_or_assign(entry.hash,entry);
                state_.writes_.push_back(std::move(entry));
            }
            if (!rows->error().empty()) {
                visit.failure = TableVisit::Failure::step;
                visit.error = rows->error();
            }
            state_.check();
        }
        if (!inventory->error().empty()) {
            state_.report_.inventory_failure = TableVisit::Failure::step;
            state_.report_.inventory_error = inventory->error();
        } else state_.report_.inventory_exhausted = true;
        state_.check();
    }
  private:
    LeafState& state_;
};
void reset_and_enumerate(std::shared_ptr<const RulesDatabase> database, LeafState& state, const Budget& budget) {
    Operation operation(state);
    operation.run(std::move(database),budget);
}
}