#include "bnw_globals_cache_source_owner.hpp"
#include <bit>
#include <exception>
#include <stdexcept>
namespace civ5::content {
namespace globals_cache_detail {
std::int32_t source_int(std::int64_t value) noexcept {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}
class LeafOperation final {
    LeafState& state_;
  public:
    explicit LeafOperation(LeafState& state) : state_(state) {
        // Thread first: a foreign caller never reads active_ or public fields.
        if (state_.thread_ != std::this_thread::get_id()) {
            poison();
            throw std::runtime_error("GC source leaf originating thread changed");
        }
        require_current();
        if (state_.active_) {
            poison();
            throw std::runtime_error("GC source leaf reentry is forbidden");
        }
        state_.active_ = true;
    }
    ~LeafOperation() { state_.active_ = false; }
    LeafOperation(const LeafOperation&) = delete;
    LeafOperation& operator=(const LeafOperation&) = delete;
    void poison() noexcept { state_.poisoned_.store(true); }
    void require_current() const {
        if (state_.poisoned_.load())
            throw std::runtime_error("GC source leaf was invalidated by mutation/reentry/thread use");
    }
};
namespace {
bool same_value(const GlobalDefineValue& a, const GlobalDefineValue& b) noexcept {
    if (a.valueless_by_exception() || b.valueless_by_exception())
        return a.valueless_by_exception() && b.valueless_by_exception();
    if (a.index() != b.index()) return false;
    if (const auto* v = std::get_if<std::int64_t>(&a)) return *v == std::get<std::int64_t>(b);
    if (const auto* v = std::get_if<double>(&a))
        return std::bit_cast<std::uint64_t>(*v) == std::bit_cast<std::uint64_t>(std::get<double>(b));
    return std::get<std::string>(a) == std::get<std::string>(b);
}
bool same_entry(const CachedGlobalDefine& a, const CachedGlobalDefine& b) noexcept {
    return a.name == b.name && a.kind == b.kind && a.retained_previous == b.retained_previous &&
        a.lookup.found == b.lookup.found && a.lookup.sqlite_code == b.lookup.sqlite_code &&
        a.lookup.error == b.lookup.error && same_value(a.lookup.value, b.lookup.value);
}
struct LeafSnapshot {
    std::int32_t heavy;
    std::map<std::string, CachedGlobalDefine, std::less<>> values;
    std::vector<std::string> writes, corrections;
    explicit LeafSnapshot(const LeafState& state)
        : heavy(state.heavy), values(state.values), writes(state.write_order),
          corrections(state.policy_correction_order) {}
    bool matches(const LeafState& state) const noexcept {
        if (heavy != state.heavy || writes != state.write_order ||
            corrections != state.policy_correction_order || values.size() != state.values.size()) return false;
        auto actual = state.values.begin();
        for (const auto& expected : values) {
            if (expected.first != actual->first || !same_entry(expected.second, actual->second)) return false;
            ++actual;
        }
        return true;
    }
};
void write_leaf(const RulesDatabase& database, LeafState& state, LeafOperation& operation,
                const WriteObserver* observer) {
    const auto current = database.immutable_preview_fingerprint_currentness();
    const auto check = [&] {
        operation.require_current();
        if (!current || !current()) throw std::runtime_error("GC source leaf requires SAME current sealed SQL owner/thread");
        operation.require_current();
    };
    check();
    const auto specs = global_define_specs(GlobalDefineProfile::bnw);
    for (std::size_t i = 0; i < specs.size(); ++i) {
        check();
        const auto& spec = specs[i];
        auto lookup = database.global_define(spec.name, spec.kind);
        check();
        if (spec.kind == GlobalDefineKind::integer) {
            lookup.value = std::int64_t(source_int(std::get<std::int64_t>(lookup.value)));
        } else if (spec.kind != GlobalDefineKind::source_float) {
            throw std::runtime_error("BNW cacheGlobals non-source scalar kind");
        }
        // Source miss/error still stores initialized zero, immediately and in
        // the unchanged 1689-spec order. Unreached fields retain prior values.
        CachedGlobalDefine entry{std::string(spec.name), spec.kind, std::move(lookup), false};
        const auto key = entry.name;
        {
            auto [where, inserted] = state.values.insert_or_assign(key, std::move(entry));
            (void)inserted;
            state.write_order.push_back(where->first);
            if (where->first == "HEAVY_RESOURCE_THRESHOLD")
                state.heavy = source_int(std::get<std::int64_t>(where->second.lookup.value));
        } // No iterator or map-entry reference survives a callback boundary.
        if (observer && *observer) {
            const LeafSnapshot before(state);
            const CachedGlobalDefine copied_entry = state.values.at(key);
            std::exception_ptr failure;
            try { (*observer)(i, copied_entry); }
            catch (...) { failure = std::current_exception(); }
            if (!before.matches(state)) {
                operation.poison();
                throw std::runtime_error("GC observer changed the source leaf state");
            }
            check(); // Caught nested calls and SQL poison cannot be hidden.
            // An unchanged throwing observer retains the completed prefix,
            // with no poison: a later no-observer call may resume/requery it.
            if (failure) std::rethrow_exception(failure);
        }
        check();
        if (spec.zero_is_none) {
            const auto where = state.values.find(key); // Fresh after all checks.
            if (where == state.values.end()) {
                operation.poison();
                throw std::runtime_error("GC source leaf store disappeared");
            }
            if (std::get<std::int64_t>(where->second.lookup.value) == 0) {
                where->second.lookup.value = std::int64_t{-1};
                state.policy_correction_order.push_back(key);
            }
        }
    }
}
} // namespace
void ordered_lookup_write(const RulesDatabase& database, LeafState& state) {
    LeafOperation operation(state);
    write_leaf(database, state, operation, nullptr);
}
void ordered_lookup_write(std::shared_ptr<const RulesDatabase> database,
                          std::shared_ptr<LeafState> state, const WriteObserver& observer) {
    if (!database || !state) throw std::invalid_argument("GC observer requires genuine shared SQL/leaf owners");
    LeafOperation operation(*state);
    const WriteObserver copied_observer = observer;
    write_leaf(*database, *state, operation, &copied_observer);
}

}
BnwGlobalsCacheSourceOwner::BnwGlobalsCacheSourceOwner(std::shared_ptr<const RulesDatabase> database)
    : database_(std::move(database)), thread_(std::this_thread::get_id()) {
    if (!database_) throw std::invalid_argument("GC source owner requires owned SQL wrapper");
    current_ = database_->immutable_preview_fingerprint_currentness();
    require_current(*database_);
}
void BnwGlobalsCacheSourceOwner::require_current(const RulesDatabase& database) const {
    if (&database != database_.get() || thread_ != std::this_thread::get_id() || !current_ || !current_())
        throw std::runtime_error("GC source owner SQL identity/generation/thread is stale");
}
void BnwGlobalsCacheSourceOwner::cache_game_database_data() {
    require_current(*database_);
    if (!needs_caching_) return;
    // The source first executes ValidateGameDatabase (its result ignored),
    // then info-type reset/enumeration. Neither may be replaced by final SQL.
    throw std::runtime_error("GC CacheGameDatabaseData unavailable: actual ShouldValidate/config load/log effects and info-type source prefix are not bound");
}
void BnwGlobalsCacheSourceOwner::flush_game_database_data() {
    require_current(*database_);
    needs_caching_ = true;
}
// No needs_caching_=false transition exists: actual PrefetchGameData,
// UpdatePlayableCivilizationCounts, AcquireTypes and SetGlobalActionInfo must
// first be owned and executed in source order (non-short-circuit &=).
}
