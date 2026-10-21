#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/content/info_collections.hpp"
#include "civ5/content/global_defines.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace civ5::runtime {struct BnwFreshGameContentResult;}
namespace civ5::content {
using DatabaseValue = std::variant<std::monostate, std::int64_t, double, std::string>;
using DatabaseScriptValue=std::variant<std::monostate,bool,std::int64_t,double,std::string>;
using DatabaseScriptRow=std::vector<std::pair<std::string,DatabaseScriptValue>>;
class RulesScriptCursor {
  public:
    ~RulesScriptCursor();
    // Lazy Step. Exhaustion (including a SQL step error) resets the statement
    // and clears its bindings, as Database::Results::Reset does.
    // Rows own their values. BLOB/NULL become nil, as in PushDatabaseRow.
    std::optional<DatabaseScriptRow> next();
    // Raw Step for GameInfo, whose closures decide when to Reset/Release.
    bool step();
    DatabaseScriptRow row() const;
    // Native cache keys use SQLite GetInt/GetText, independently of row types.
    // The boolean distinguishes an absent column from a present NULL column.
    std::pair<bool,DatabaseScriptValue> cache_key(bool id) const;
    void reset();
    void bind(std::span<const DatabaseScriptValue> parameters,bool ignore_errors=false);
    void release();
    std::string_view error() const;
  private:
    friend class RulesDatabase;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit RulesScriptCursor(std::unique_ptr<Impl>);
};
struct DatabaseStatement {
    std::string sql;
    std::vector<DatabaseValue> parameters;
};
struct RulesQueryReport {
    std::uint64_t rows = 0;
    int sqlite_code = 0;
    std::string error;
    bool all_succeeded() const { return error.empty(); }
};
// Recovered XMLSerializer lexical rules, with integer storage widened to 64
// bits.
DatabaseValue game_data_value(const std::string &text);
struct DeleteMissingReferences {
    std::string table, column;
};
// XML InsertOrAbort uses TryExecute/TryReset, unlike a raw SQL OR ABORT.
// Preparation/binding still fail normally; step ABORT/CONSTRAINT may continue.
struct GameDataTryInsert {
    DatabaseStatement statement;
};
// Cleanup must inspect the schema at its position in the XML stream, including
// tables created earlier in that same file. It cannot be precompiled to SQL.
using GameDataOperation = std::variant<DatabaseStatement, DeleteMissingReferences, GameDataTryInsert>;
struct GameDataPackageTag {
    std::string value;
    std::vector<std::string> tables;
};
// EXE DLC loader registers Civilizations/Leaders and formats the package GUID
// as uppercase hexadecimal without separators. It is unrelated to pointer width.
GameDataPackageTag dlc_game_data_tag(const std::string &guid);
std::vector<GameDataOperation> compile_game_data(const XmlNode &document,
                                                 const GameDataPackageTag &package_tag = {});
struct ModSqlResult {
    // ExecuteMultiple's result. Native EndTransaction errors are logged
    // separately and do not replace this return value.
    bool success = true;
    std::uint64_t completed_statements = 0;
    int error_code = 0;
    std::string error;
    std::string release_error;
    bool transaction_active_after = false;
};
struct IdRemapError {
    std::string stage, message;
    int sqlite_code = 0;
};
struct IdRemapTable {
    std::string name;
    std::uint64_t rows = 0, collected_ids = 0, attempted_updates = 0, completed_updates = 0;
    bool eligible = false, negative_id_seen = false;
    std::vector<IdRemapError> errors;
};
struct IdRemapReport {
    bool all_succeeded = true;
    std::vector<IdRemapTable> tables;
    std::vector<IdRemapError> errors;
};
struct IdRemapBudget {
    std::uint64_t table_entries = 1024 * 1024;
    std::uint64_t ids_per_table = 16 * 1024 * 1024;
};

enum class GameplayPostprocessProfile { bnw, vp542 };
struct GameplayDefine {
    std::string name;
    std::int64_t value = 0;
};
struct GameplayPostprocessError {
    std::string stage, message;
    int sqlite_code = 0;
};
struct PostDefineResult {
    std::string name, key, table;
    std::optional<std::int64_t> rowid;
    bool applied = false;
    std::vector<GameplayPostprocessError> errors;
};
struct GameplayPostprocessReport {
    bool all_succeeded = true, transaction_released = false;
    bool strategic_resources_applied = false, luxury_resources_applied = false;
    std::uint64_t runtime_defines_applied = 0;
    std::vector<PostDefineResult> post_defines;
    std::vector<GameplayPostprocessError> errors;
};
struct GameplayPostprocessBudget {
    std::uint64_t post_defines = 1024 * 1024;
};
struct RulesFingerprintNormalization {
    std::string_view table, stable_key, generated_id;
};

// An isolated in-memory rules database. Source SQL cannot attach files, load
// extensions. Atomic APIs own their transaction; mod APIs reproduce the
// recovered UpdateDatabase transaction behavior. No original cache is opened.
class RulesDatabase {
  public:
    RulesDatabase();
    ~RulesDatabase();
    // Saved-game SQLite lives in a private temporary file, so each Lua open
    // gets its own SQLite connection/transaction while sharing committed data.
    // Never opens or modifies the original game's cache. Empty images reject.
    static std::shared_ptr<RulesDatabase> from_saved_image(std::span<const std::uint8_t> image);
    // New per-mod user data starts with the retail SimpleValues schema. It
    // shares the same private, multi-connection storage as imported save data.
    static std::shared_ptr<RulesDatabase> new_saved_values();
    std::unique_ptr<RulesDatabase> reopen_saved() const;
    // Copy an activated gameplay database for mod-script diagnostics. The main
    // schema and any attached Localization schema are private writable copies;
    // Lua DB.Query writes cannot change the authoritative rules database.
    std::unique_ptr<RulesDatabase> clone_gameplay() const;
    // Call at the session's save boundary. Rejects unfinished transactions on
    // any opened connection instead of silently dropping uncommitted mod data.
    std::vector<std::uint8_t> saved_image() const;
    // Export-only preflight: no implicit COMMIT/ROLLBACK/reset/finalize.
    // Completed inactive cursors are permitted; busy cursors and ANY unfinished
    // connection transaction/savepoint refuse. Legacy saved_image is unchanged.
    void require_saved_checkpoint_boundary() const;
    // Canonical logical schema/rows of the isolated activated gameplay DB.
    // Includes rowid order where present. A selected GameCore profile may
    // normalize a generated ID that changes while the stable keyed rules do
    // not. Physical SQLite page/header changes do not affect the result.
    std::vector<std::uint8_t> gameplay_fingerprint_data(
        std::span<const RulesFingerprintNormalization> normalizations={},
        std::uint64_t maximum_bytes=1024ULL*1024*1024) const;
    // Explicit Civis immutable preview capability, tied to this SAME Impl.
    // One-way: activated unsaved DB, no transaction/busy statement, one thread.
    // Any attempted writer or foreign-thread use permanently poisons the scope;
    // cached/currentness reads then refuse. Unsealed mutable APIs are unchanged.
    void seal_immutable_preview_fingerprints() const;
    bool immutable_preview_fingerprints_current() const;
    // Retains only SAME Impl currentness; empty outside explicit preview scope.
    std::function<bool()> immutable_preview_fingerprint_currentness() const;
    std::uint64_t immutable_preview_fingerprint_serializations() const;
    // Exact existing canonical bytes hashed by the existing SHA256 helper.
    // Cache only inside the explicitly sealed scope, default/normalized apart.
    std::string gameplay_fingerprint_sha256(
        std::span<const RulesFingerprintNormalization> normalizations={},
        std::uint64_t maximum_bytes=1024ULL*1024*1024) const;
    RulesDatabase(const RulesDatabase &) = delete;
    RulesDatabase &operator=(const RulesDatabase &) = delete;
    void apply_xml(const XmlNode &document);
    void apply_sql(const std::string &sql);
    void apply_statements(std::span<const DatabaseStatement> statements);
    // XML: SAVEPOINT _, release on success, whole-transaction rollback on error.
    void apply_mod_xml(const XmlNode &document, const GameDataPackageTag &package_tag = {});
    // SQL: SAVEPOINT _, execute until the first error, RELEASE _ even on error.
    // Earlier successful statements can survive a failed action. Source SQL
    // transactions/savepoints and SQLite conflict modes retain their semantics.
    ModSqlResult apply_mod_sql(const std::string &sql);
    // Explicit CP/VP post-load step. Uses the authored Remapper table, ordered
    // original IDs and individual UPDATEs; failures do not roll back prior
    // updates or skip later entries. Does not begin/commit a transaction.
    // It is not a vanilla rule and does not perform PostDefines or other cache
    // initialization. Budgets bound diagnostics, not gameplay capacities.
    IdRemapReport remap_vp_ids(const IdRemapBudget &budget = {});
    // After source-specific ID remapping: PostDefines -> ResourceUsage ->
    // caller-owned runtime constants, in one source SAVEPOINT/RELEASE scope.
    // SQL failures are reported and later rows/stages still run. An applied
    // statement can subsequently be undone by a source ROLLBACK trigger.
    // Runtime constants are supplied by the x64 session, not compiled x86 caps.
    GameplayPostprocessReport postprocess_gameplay(
        GameplayPostprocessProfile profile, std::span<const GameplayDefine> runtime_defines,
        const GameplayPostprocessBudget &budget = {});
    // VP CustomMods::prefetchCache: CustomModPostDefines.Type -> ROWID ->
    // CustomModOptions, in source order and its own SAVEPOINT/RELEASE scope.
    GameplayPostprocessReport prefetch_custom_mod_options(const GameplayPostprocessBudget &budget = {});
    // GC cache: main-schema tables in sqlite_master order, SELECT * rows in
    // source order, exact ID/Type columns. SQL errors retain the partial map
    // and continue with later tables; host budget failures throw.
    InfoTypeCatalog cache_info_types(const InfoTypeBudget &budget = {}) const;
    // Source PrefetchCollection: SELECT * WHERE ID > -1 ORDER BY ID; holes
    // advance the logical index, and overlapping IDs still append a new slot.
    // No array through MAX(ID) is allocated. The callback is synchronous and
    // may perform nested lookups. A false CacheResults result is counted but
    // still appends, as in GameCore. SQL errors retain the visited prefix;
    // host budget/overflow and callback exceptions propagate to the caller.
    using InfoVisitor = std::function<bool(std::uint64_t slot, std::int64_t authored_id,
                                          const RulesRowView &row)>;
    InfoCollectionReport prefetch_info(const std::string &table, const InfoVisitor &visitor = {},
                                      const InfoCollectionBudget &budget = {}) const;
    // Source LIMIT 1 lookup with SQLite comparison/conversion semantics. A
    // prepared statement is reused, but values are reread on every call.
    // Missing values have a typed zero/empty result; SQL failures are separate.
    GlobalDefineLookup global_define(std::string_view name, GlobalDefineKind kind) const;
    // Complete source cache contract. VP missing values retain constructor or
    // previous values; BNW getters reset to zero on each load. No database write.
    GlobalDefineCache cache_global_defines(GlobalDefineProfile profile,
                                           const GlobalDefineCache *previous = nullptr) const;
    // Original localization schemas include PRAGMA user_version. Only this
    // bootstrap API permits that pragma; source SQL still cannot attach files.
    void apply_localization_schema(const std::string &sql);
    // EXE 0x71E9F0/0x66A560: use Languages metadata to migrate physical language
    // tables to LocalizedText and writable language views.
    std::uint64_t convert_localization_tables();
    // Merge a selected, already built gameplay DLC text database, then mark
    // subsequent text changes as Mods in ScannedFiles (EXE 0x6714A0).
    void merge_localization(const RulesDatabase &package);
    void track_localization_mod_changes();
    // Attach a private in-memory snapshot as Localization. Main and text data
    // then share SQL transaction/rollback behavior, without touching a cache DB.
    void attach_localization(const RulesDatabase &localization);
    // Rows are streamed to the caller, including integers outside signed 32 bits.
    using RowVisitor = std::function<void(std::span<const DatabaseValue>)>;
    std::uint64_t query(const DatabaseStatement &statement, const RowVisitor &visitor) const;
    // Retail DB.Query: first SQL statement only, missing binds NULL, extra
    // binds ignored, numbers double, booleans integer. No eager Step or
    // read-only substitution: writes target this isolated in-memory database.
    // SQL preparation failure returns null (an invalid native Lua iterator).
    // Cursor keeps the connection alive. File/extension access stays denied.
    std::unique_ptr<RulesScriptCursor> script_query(std::string_view sql,
                                                   std::span<const DatabaseScriptValue> parameters) const;
    // Database::Results conversions for linked info tables. SQL failures retain
    // the visited prefix; visitor exceptions and host budget failures propagate.
    RulesQueryReport query_rows(const DatabaseStatement &statement,
                                const std::function<void(const RulesRowView &)> &visitor,
                                std::uint64_t maximum_rows = 16 * 1024 * 1024) const;
    std::int64_t script_statement_count() const;
    std::optional<std::string> script_statement_sql(std::int64_t zero_based_index) const;
    static std::string sqlite_version();
    static std::string sqlite_source_id();

  private:
    friend class RulesScriptCursor;
    friend struct civ5::runtime::BnwFreshGameContentResult;
    // Private storage identity only, including before immutable preview seal.
    std::function<bool(const RulesDatabase&)> capture_actual_impl_identity() const;
    std::vector<std::uint8_t> uncached_gameplay_fingerprint_data(
        std::span<const RulesFingerprintNormalization>,std::uint64_t) const;
    GameplayPostprocessReport postprocess_defines(GameplayPostprocessProfile profile,
        std::span<const GameplayDefine> runtime_defines, const GameplayPostprocessBudget &budget, bool custom);
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::content
