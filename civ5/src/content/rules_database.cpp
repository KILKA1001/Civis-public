#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <filesystem>
#include <random>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include "civ5/runtime/file_identity.hpp"

namespace civ5::content {
namespace {
std::string quote(const std::string &value, char delimiter) {
    if (value.find('\0') != std::string::npos)
        throw std::runtime_error("NUL in SQL identifier or literal");
    std::string result(1, delimiter);
    for (const auto c : value) {
        result += c;
        if (c == delimiter)
            result += c;
    }
    return result + delimiter;
}
std::string identifier(const std::string &value) {
    if (value.empty())
        throw std::runtime_error("Empty SQL identifier");
    return quote(value, '"');
}
void attributes(const XmlNode &node, std::initializer_list<std::string_view> allowed) {
    for (const auto &[name, value] : node.attributes) {
        (void)value;
        if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
            throw std::runtime_error("Unsupported " + node.name + " attribute: " + name);
    }
}
void whitespace(const std::string &value) {
    if (value.find_first_not_of(" \t\n\r") != std::string::npos)
        throw std::runtime_error("Unexpected mixed GameData XML text");
}
bool enabled(const XmlNode &node, const std::string &key) {
    const auto value = runtime::ascii_lower(node.attribute(key));
    if (value.empty() || value == "false" || value == "0")
        return false;
    if (value == "true" || value == "1")
        return true;
    throw std::runtime_error("Invalid boolean attribute " + key);
}
using Fields = std::vector<std::pair<std::string, DatabaseValue>>;
Fields fields(const XmlNode &node) {
    // FXmlTraverser enumerates attributes and child elements here. Container
    // character data is not a field: shipped CIV5Automates.xml has a stray '>'
    // between two Row columns. Leaf values below still preserve their full text.
    Fields result;
    std::set<std::string> seen;
    const auto add = [&](const auto &name, const auto &value) {
        if (!seen.insert(runtime::ascii_lower(name)).second)
            throw std::runtime_error("Duplicate field " + name);
        result.emplace_back(name, game_data_value(value));
    };
    for (const auto &[name, value] : node.attributes)
        add(name, value);
    for (const auto &field : node.children) {
        if (!field.attributes.empty() || !field.children.empty())
            throw std::runtime_error("Nested GameData field " + field.name);
        add(field.name, field.text);
    }
    return result;
}
void assignments(DatabaseStatement &statement, const Fields &values, const char *separator) {
    bool first = true;
    for (const auto &[name, value] : values) {
        if (!first)
            statement.sql += separator;
        first = false;
        statement.sql += identifier(name) + " = ?";
        statement.parameters.push_back(value);
    }
}
std::string reference(const std::string &value) {
    const auto open = value.find('(');
    if (open == std::string::npos || value.back() != ')' || value.find('(', open + 1) != std::string::npos)
        throw std::runtime_error("Unsupported column reference: " + value);
    return identifier(value.substr(0, open)) + "(" +
           identifier(value.substr(open + 1, value.size() - open - 2)) + ")";
}
double real(const std::string &text) {
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value))
        throw std::runtime_error("Invalid or overflowing GameData real");
    return value;
}
DatabaseStatement table(const XmlNode &node) {
    attributes(node, {"name"});
    whitespace(node.text);
    DatabaseStatement result{"CREATE TABLE " + identifier(node.attribute("name")) + " (", {}};
    if (node.children.empty())
        throw std::runtime_error("Table has no columns");
    bool first = true;
    for (const auto &column : node.children) {
        if (column.name != "Column" || !column.children.empty())
            throw std::runtime_error("Unsupported table declaration");
        whitespace(column.text);
        attributes(column, {"name", "type", "primarykey", "autoincrement", "unique", "notnull", "default",
                            "reference"});
        const auto type = runtime::ascii_lower(column.attribute("type"));
        if (type != "integer" && type != "int" && type != "text" && type != "string" && type != "boolean" &&
            type != "bool" && type != "real" && type != "float" && type != "numeric" && type != "blob" &&
            type != "variant")
            throw std::runtime_error("Unsupported GameData column type: " + type);
        if (!first)
            result.sql += ",";
        first = false;
        result.sql += identifier(column.attribute("name")) + " " + type;
        if (enabled(column, "primarykey"))
            result.sql += " PRIMARY KEY";
        if (enabled(column, "autoincrement"))
            result.sql += " AUTOINCREMENT";
        if (enabled(column, "unique"))
            result.sql += " UNIQUE";
        if (enabled(column, "notnull"))
            result.sql += " NOT NULL";
        for (const auto &[key, value] : column.attributes)
            if (key == "default") {
                result.sql += " DEFAULT ";
                const auto lower = runtime::ascii_lower(value);
                if (value == "NULL")
                    result.sql += "NULL";
                else if (lower == "true" || lower == "false")
                    result.sql += lower == "true" ? "1" : "0";
                else if (type == "integer" || type == "real" || type == "float" || type == "numeric") {
                    (void)real(value);
                    result.sql += value;
                } else
                    result.sql += quote(value, '\'');
            }
        const auto ref = column.attribute("reference");
        if (!ref.empty())
            result.sql += " REFERENCES " + reference(ref);
    }
    result.sql += ")";
    return result;
}
DatabaseStatement index(const XmlNode &node) {
    // Port the explicit name/table/Column grammar present in the vanilla source.
    // More permissive native attribute and collation behavior is not inferred.
    attributes(node, {"name", "table", "unique"});
    whitespace(node.text);
    DatabaseStatement result{"CREATE ", {}};
    if (enabled(node, "unique"))
        result.sql += "UNIQUE ";
    result.sql +=
        "INDEX " + identifier(node.attribute("name")) + " ON " + identifier(node.attribute("table")) + " (";
    if (node.children.empty())
        throw std::runtime_error("Index needs columns");
    bool first = true;
    for (const auto &column : node.children) {
        if (column.name != "Column" || !column.children.empty())
            throw std::runtime_error("Unsupported Index child");
        attributes(column, {"name", "sort"});
        whitespace(column.text);
        const auto sort = runtime::ascii_lower(column.attribute("sort"));
        if (!sort.empty() && sort != "asc" && sort != "desc")
            throw std::runtime_error("Unsupported Index sort");
        if (!first)
            result.sql += ",";
        first = false;
        result.sql += identifier(column.attribute("name"));
        if (!sort.empty())
            result.sql += " " + sort;
    }
    result.sql += ")";
    return result;
}
} // namespace

DatabaseValue game_data_value(const std::string &text) {
    const auto lower = runtime::ascii_lower(text);
    if (lower.starts_with("true"))
        return std::int64_t{1};
    if (lower.starts_with("false"))
        return std::int64_t{0};
    if (text.empty() || text.starts_with("NULL"))
        return std::monostate{};
    bool dot = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] >= '0' && text[i] <= '9')
            continue;
        if (text[i] == '.' && !dot && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9') {
            dot = true;
            continue;
        }
        return text;
    }
    if (dot) {
        // Original Results::Bind(float) rounding is a numeric rule, not an
        // address-space limit.
        const auto value = static_cast<float>(real(text));
        if (!std::isfinite(value))
            throw std::runtime_error("GameData float overflow");
        return static_cast<double>(value);
    }
    std::int64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::runtime_error("GameData signed-64 integer overflow");
    return value;
}
GameDataPackageTag dlc_game_data_tag(const std::string &guid) {
    GameDataPackageTag result{{}, {"Civilizations", "Leaders"}};
    for (const char c : canonical_guid(guid)) {
        if (c != '-')
            result.value += c >= 'a' && c <= 'f' ? static_cast<char>(c - 'a' + 'A') : c;
    }
    return result;
}
std::vector<GameDataOperation> compile_game_data(const XmlNode &document,
                                                 const GameDataPackageTag &package_tag) {
    if (document.name != "GameData")
        throw std::runtime_error("Expected GameData root");
    attributes(document, {});
    // The native serializer iterates child elements via FXmlTraverser.
    // Container text is not an action; C/SQL comment markers do not hide rows.
    std::vector<GameDataOperation> statements;
    for (const auto &node : document.children) {
        if (node.name == "Table") {
            statements.push_back(table(node));
            continue;
        }
        if (node.name == "Index") {
            statements.push_back(index(node));
            continue;
        }
        if (node.name == "DeleteMissingReferences") {
            attributes(node, {"table", "column"});
            whitespace(node.text);
            if (!node.children.empty())
                throw std::runtime_error("DeleteMissingReferences cannot contain child elements");
            DeleteMissingReferences cleanup{node.attribute("table"), node.attribute("column")};
            // Validate names now; resolve incoming foreign keys at execution time.
            identifier(cleanup.table);
            identifier(cleanup.column);
            statements.emplace_back(std::move(cleanup));
            continue;
        }
        attributes(node, {});
        for (const auto &action : node.children) {
            DatabaseStatement statement;
            if (action.name == "Update") {
                attributes(action, {});
                whitespace(action.text);
                for (const auto &part : action.children)
                    if (part.name != "Set" && part.name != "Where")
                        throw std::runtime_error("Unsupported Update child " + part.name);
                const auto *set = action.child("Set");
                if (!set)
                    throw std::runtime_error("Update needs Set");
                const auto set_values = fields(*set);
                if (set_values.empty())
                    throw std::runtime_error("Empty Set");
                statement.sql = "UPDATE " + identifier(node.name) + " SET ";
                assignments(statement, set_values, ",");
                if (const auto *where = action.child("Where")) {
                    const auto values = fields(*where);
                    if (!values.empty()) {
                        statement.sql += " WHERE ";
                        assignments(statement, values, " AND ");
                    }
                }
            } else if (action.name == "Delete") {
                statement.sql = "DELETE FROM " + identifier(node.name);
                const auto values = fields(action);
                if (!values.empty()) {
                    statement.sql += " WHERE ";
                    assignments(statement, values, " AND ");
                }
            } else {
                if (action.name == "Row")
                    statement.sql = "INSERT INTO ";
                else if (action.name == "InsertOrAbort")
                    statement.sql = "INSERT OR ABORT INTO ";
                else if (action.name == "InsertOrIgnore")
                    statement.sql = "INSERT OR IGNORE INTO ";
                else if (action.name == "Replace")
                    statement.sql = "INSERT OR REPLACE INTO ";
                else
                    throw std::runtime_error("Unsupported GameData action " + action.name);
                auto values = fields(action);
                if (values.empty())
                    throw std::runtime_error("Insert needs values");
                // DLL 0x100743A3 calls the tagging helper after the final child
                // field. Attribute-only rows, Update and Delete do not call it.
                // Table comparison is case-sensitive std::string::compare.
                if (!action.children.empty() &&
                    std::find(package_tag.tables.begin(), package_tag.tables.end(), node.name) !=
                        package_tag.tables.end())
                    values.emplace_back("PackageID", game_data_value(package_tag.value));
                statement.sql += identifier(node.name) + " (";
                for (const auto &[name, value] : values) {
                    if (!statement.parameters.empty())
                        statement.sql += ",";
                    statement.sql += identifier(name);
                    statement.parameters.push_back(value);
                }
                statement.sql += ") VALUES (";
                for (std::size_t i = 0; i < values.size(); ++i)
                    statement.sql += i ? ",?" : "?";
                statement.sql += ")";
            }
            if (action.name == "InsertOrAbort")
                statements.push_back(GameDataTryInsert{std::move(statement)});
            else
                statements.push_back(std::move(statement));
        }
    }
    return statements;
}

struct RulesDatabase::Impl {
    const std::thread::id identity_creator=std::this_thread::get_id();
    struct SavedStorage {
        std::filesystem::path directory, file;
        std::set<sqlite3*> connections;
        SavedStorage() {
            std::random_device random;
            for (unsigned attempt=0;attempt<64;++attempt) {
                const auto candidate=std::filesystem::temp_directory_path()/
                    ("civis-save-db-"+std::to_string(random())+"-"+std::to_string(random()));
                if (std::filesystem::create_directory(candidate)) {
                    directory=candidate;file=directory/"session.db";return;
                }
            }
            throw std::runtime_error("Cannot create private saved database directory");
        }
        ~SavedStorage() {
            // Only files within our exclusively created directory; no recursive
            // cleanup or path supplied by a mod/original save is ever used.
            std::error_code ignored;
            for (const auto *name:{"session.db-journal","session.db-wal","session.db-shm","session.db"})
                std::filesystem::remove(directory/name,ignored);
            std::filesystem::remove(directory,ignored);
        }
    };
    std::shared_ptr<SavedStorage> saved_storage;
    using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
    struct SqlError : std::runtime_error {
        int code;
        SqlError(int value, const std::string &message) : std::runtime_error(message), code(value) {}
    };
    sqlite3 *db = nullptr;
    mutable Statement global_lookup{nullptr, sqlite3_finalize};
    struct ConnectionGuard {
        sqlite3_mutex *mutex;
        explicit ConnectionGuard(sqlite3 *connection):mutex(sqlite3_db_mutex(connection)) {
            if(!mutex) throw std::runtime_error("Immutable rules need SQLite FULLMUTEX");
            sqlite3_mutex_enter(mutex);
        }
        ~ConnectionGuard() {sqlite3_mutex_leave(mutex);}
        ConnectionGuard(const ConnectionGuard &)=delete;
        ConnectionGuard &operator=(const ConnectionGuard &)=delete;
    };
    using FingerprintKey=std::vector<std::array<std::string,3>>;
    struct FingerprintEntry {
        FingerprintKey key;
        std::vector<std::uint8_t> bytes;
        std::string sha256;
    };
    bool preview_sealed=false;
    mutable bool preview_poisoned=false;
    std::thread::id preview_thread;
    mutable bool fingerprint_building=false;
    mutable std::uint64_t fingerprint_serializations=0;
    mutable std::vector<FingerprintEntry> fingerprints;
    void poison_preview() const noexcept {preview_poisoned=true;fingerprints.clear();}
    void preview_access() const {
        if(!preview_sealed) return;
        if(preview_thread!=std::this_thread::get_id()) {
            poison_preview();
            throw std::runtime_error("Immutable preview rules used from foreign thread");
        }
        if(preview_poisoned) throw std::runtime_error("Immutable preview rules scope is poisoned");
    }
    void preview_cleanup_access() const {
        if(preview_sealed && preview_thread!=std::this_thread::get_id()) {
            poison_preview();
            throw std::runtime_error("Immutable preview rules cleanup from foreign thread");
        }
    }
    int step(sqlite3_stmt *statement) const {
        ConnectionGuard guard(db);
        preview_access();
        if(preview_sealed && !sqlite3_stmt_readonly(statement)) {
            poison_preview();
            throw std::runtime_error("Immutable preview rules writer attempted");
        }
        return sqlite3_step(statement);
    }
    static FingerprintKey pin_fingerprint_key(std::span<const RulesFingerprintNormalization> input) {
        FingerprintKey result;result.reserve(input.size());
        for(const auto &item:input) result.push_back({std::string(item.table),
            std::string(item.stable_key),std::string(item.generated_id)});
        return result;
    }
    static std::vector<RulesFingerprintNormalization> key_views(const FingerprintKey &key) {
        std::vector<RulesFingerprintNormalization> result;result.reserve(key.size());
        for(const auto &item:key) result.push_back({item[0],item[1],item[2]});
        return result;
    }
    static bool cacheable_key(const FingerprintKey &key) {
        constexpr std::size_t key_budget=64*1024;
        if(key.size()>key_budget/sizeof(std::array<std::string,3>)) return false;
        std::size_t retained=key.size()*sizeof(std::array<std::string,3>);
        for(const auto &item:key) for(const auto &text:item) {
            if(text.size()>key_budget-retained) return false;
            retained+=text.size();
        }
        return true;
    }
    const FingerprintEntry *cached_fingerprint(const FingerprintKey &key,std::uint64_t budget) const {
        for(const auto &entry:fingerprints) if(entry.key==key) {
            if(entry.bytes.size()>budget) throw std::length_error("Gameplay fingerprint exceeds host byte budget");
            return &entry;
        }
        return nullptr;
    }
    bool internal_transaction = false;
    bool mod_sql = false;
    bool internal_foreign_keys = false;
    bool internal_table_info = false;
    bool localization_schema = false;
    bool internal_memory_attachment = false;
    bool internal_serialization = false;
    static int authorize(void *context, int operation, const char *name, const char *, const char *,
                         const char *) {
        const auto &self = *static_cast<Impl *>(context);
        // Runs under SQLite's connection mutex. Only pure reads/functions and
        // the existing internally-owned read PRAGMAs survive the sealed scope.
        if(self.preview_sealed) {
            if(self.preview_poisoned || self.preview_thread!=std::this_thread::get_id()) {
                self.poison_preview();return SQLITE_DENY;
            }
            const bool read=operation==SQLITE_SELECT || operation==SQLITE_READ ||
                operation==SQLITE_FUNCTION || operation==SQLITE_RECURSIVE;
            const bool read_pragma=operation==SQLITE_PRAGMA && name &&
                ((self.internal_foreign_keys && std::string_view(name)=="foreign_key_list") ||
                 (self.internal_table_info && std::string_view(name)=="table_info") ||
                 (self.internal_serialization && std::string_view(name)=="page_count"));
            if(!read && !read_pragma) {self.poison_preview();return SQLITE_DENY;}
        }
        if (operation == SQLITE_PRAGMA && self.internal_serialization && name && std::string_view(name)=="page_count")
            return SQLITE_OK;
        if (operation == SQLITE_PRAGMA && self.internal_foreign_keys && name &&
            std::string_view(name) == "foreign_key_list")
            return SQLITE_OK;
        if (operation == SQLITE_PRAGMA && self.internal_table_info && name &&
            std::string_view(name) == "table_info")
            return SQLITE_OK;
        if (operation == SQLITE_PRAGMA && self.localization_schema && name &&
            std::string_view(name) == "user_version")
            return SQLITE_OK;
        if (self.internal_memory_attachment && name &&
            ((operation == SQLITE_ATTACH && std::string_view(name) == ":memory:") ||
             (operation == SQLITE_DETACH &&
              (std::string_view(name) == "ContentPackage" || std::string_view(name) == "Localization"))))
            return SQLITE_OK;
        if (operation == SQLITE_ATTACH || operation == SQLITE_DETACH || operation == SQLITE_PRAGMA)
            return SQLITE_DENY;
        if ((operation == SQLITE_TRANSACTION || operation == SQLITE_SAVEPOINT) &&
            !self.internal_transaction && !self.mod_sql)
            return SQLITE_DENY;
        return SQLITE_OK;
    }
    explicit Impl(std::shared_ptr<SavedStorage> saved={}) : saved_storage(std::move(saved)) {
        static_assert(sizeof(void *) == 8 && sizeof(sqlite3_int64) == 8);
        const auto encoded=saved_storage?saved_storage->file.u8string():std::u8string(u8":memory:");
        const auto status = sqlite3_open_v2(
            reinterpret_cast<const char*>(encoded.c_str()), &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (status != SQLITE_OK) {
            const auto error = db ? std::string(sqlite3_errmsg(db)) : "allocation failure";
            if (db)
                sqlite3_close(db);
            db = nullptr;
            throw std::runtime_error(error);
        }
        sqlite3_extended_result_codes(db, 1);
        sqlite3_set_authorizer(db, authorize, this);
        if(saved_storage)try {saved_storage->connections.insert(db);}
        catch(...) {sqlite3_close(db);db=nullptr;throw;}
    }
    ~Impl() {
        global_lookup.reset();
        if(saved_storage)saved_storage->connections.erase(db);
        if (db)
            sqlite3_close(db);
    }
    void check(int status) const {
        if (status != SQLITE_OK)
            throw SqlError(status, "Rules SQLite: " + std::string(sqlite3_errmsg(db)));
    }
    Statement prepare(const std::string &sql, const char **tail = nullptr) const {
        ConnectionGuard guard(db);preview_access();
        if (sql.find('\0') != std::string::npos)
            throw std::runtime_error("NUL in SQL source");
        sqlite3_stmt *raw = nullptr;
        const auto status = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, tail);
        Statement result(raw, sqlite3_finalize);
        check(status);
        return result;
    }
    void bind(sqlite3_stmt *statement, std::span<const DatabaseValue> values) const {
        ConnectionGuard guard(db);preview_access();
        if (values.size() != static_cast<std::size_t>(sqlite3_bind_parameter_count(statement)))
            throw std::runtime_error("SQL parameter count mismatch");
        for (std::size_t i = 0; i < values.size(); ++i) {
            const auto index =
                static_cast<int>(i + 1); // SQLite limits columns; never an object ID or byte offset.
            const auto &value = values[i];
            int status = SQLITE_ERROR;
            if (std::holds_alternative<std::monostate>(value))
                status = sqlite3_bind_null(statement, index);
            else if (const auto *integer = std::get_if<std::int64_t>(&value))
                status = sqlite3_bind_int64(statement, index, *integer);
            else if (const auto *number = std::get_if<double>(&value))
                status = sqlite3_bind_double(statement, index, *number);
            else {
                const auto &text = std::get<std::string>(value);
                status = sqlite3_bind_text64(statement, index, text.data(), text.size(), SQLITE_TRANSIENT,
                                             SQLITE_UTF8);
            }
            check(status);
        }
    }
    void run(sqlite3_stmt *statement) const {
        int status;
        do {
            status = step(statement);
        } while (status == SQLITE_ROW);
        if (status != SQLITE_DONE)
            check(status);
    }
    void transaction(const char *sql) {
        ConnectionGuard guard(db);preview_access();
        internal_transaction = true;
        try {
            auto statement = prepare(sql);
            internal_transaction = false;
            run(statement.get());
        } catch (...) {
            internal_transaction = false;
            throw;
        }
    }
    void rollback_if_active() noexcept {
        if (!sqlite3_get_autocommit(db)) {
            try {
                transaction("ROLLBACK");
            } catch (...) { /* Preserve the original execution error. */
            }
        }
    }
    void run_sql(const std::string &sql, bool allow_unbound, std::uint64_t &completed) {
        ConnectionGuard guard(db);preview_access();
        const char *next = sql.c_str();
        while (*next) {
            sqlite3_stmt *raw = nullptr;
            const char *tail = nullptr;
            const auto status = sqlite3_prepare_v2(db, next, -1, &raw, &tail);
            Statement statement(raw, sqlite3_finalize);
            check(status);
            if (tail <= next)
                throw std::runtime_error("SQL parser made no progress");
            next = tail;
            if (statement) {
                if (!allow_unbound && sqlite3_bind_parameter_count(statement.get()) != 0)
                    throw std::runtime_error("Unbound parameter in SQL source");
                run(statement.get());
                ++completed;
            }
        }
    }
    Statement single(const DatabaseStatement &source) const {
        const char *tail = nullptr;
        auto statement = prepare(source.sql, &tail);
        if (!statement)
            throw std::runtime_error("Empty SQL statement");
        auto extra = prepare(tail);
        if (extra)
            throw std::runtime_error("Expected exactly one SQL statement");
        bind(statement.get(), source.parameters);
        return statement;
    }
    std::string column_text(sqlite3_stmt *statement, int index) const {
        const auto *text = sqlite3_column_text(statement, index);
        if (!text) {
            if (sqlite3_column_type(statement, index) == SQLITE_NULL)
                return {};
            check(sqlite3_errcode(db));
            throw std::runtime_error("SQLite text conversion failed");
        }
        return {reinterpret_cast<const char *>(text),
                static_cast<std::size_t>(sqlite3_column_bytes(statement, index))};
    }
    Statement foreign_keys(const std::string &table) {
        ConnectionGuard guard(db);preview_access();
        internal_foreign_keys = true;
        try {
            auto result = prepare("PRAGMA foreign_key_list(" + identifier(table) + ");");
            internal_foreign_keys = false;
            return result;
        } catch (...) {
            internal_foreign_keys = false;
            throw;
        }
    }
    Statement table_info(const std::string &table) {
        ConnectionGuard guard(db);preview_access();
        internal_table_info = true;
        try {
            auto result = prepare("PRAGMA table_info(" + identifier(table) + ");");
            internal_table_info = false;
            return result;
        } catch (...) {
            internal_table_info = false;
            throw;
        }
    }
    bool next(sqlite3_stmt *statement) const {
        const auto status = step(statement);
        if (status == SQLITE_ROW)
            return true;
        if (status != SQLITE_DONE)
            check(status);
        return false;
    }
    void delete_missing_references(const DeleteMissingReferences &target) {
        // DLL 0x10072240: all main-schema tables, in sqlite_master order. Each
        // foreign-key row is checked independently, including composite keys.
        // NOT IN is intentional: replacing it with NOT EXISTS changes NULLs.
        auto tables = prepare("select name from sqlite_master where type = 'table'");
        transaction("SAVEPOINT _;");
        try {
            while (next(tables.get())) {
                const auto table = column_text(tables.get(), 0);
                auto keys = foreign_keys(table);
                while (next(keys.get())) {
                    const auto referenced_table = column_text(keys.get(), 2);
                    const auto referenced_column = column_text(keys.get(), 4);
                    if (runtime::ascii_lower(referenced_table) != runtime::ascii_lower(target.table) ||
                        runtime::ascii_lower(referenced_column) != runtime::ascii_lower(target.column))
                        continue;
                    const auto from = column_text(keys.get(), 3);
                    auto deletion =
                        prepare("DELETE FROM " + identifier(table) + " WHERE " + identifier(from) +
                                " NOT IN (SELECT " + identifier(referenced_column) + " FROM " +
                                identifier(target.table) + ")");
                    run(deletion.get());
                }
            }
            transaction("RELEASE _;");
        } catch (...) {
            rollback_if_active();
            throw;
        }
    }
    void run_game_data(std::span<const GameDataOperation> operations) {
        for (const auto &operation : operations) {
            if (const auto *source = std::get_if<DatabaseStatement>(&operation)) {
                auto statement = single(*source);
                run(statement.get());
            } else if (const auto *attempt = std::get_if<GameDataTryInsert>(&operation)) {
                // DLL 0x1007483F/0x10006F30: only XML InsertOrAbort takes this
                // path. Keep actual OR ABORT SQL: replacing it with OR IGNORE
                // changes trigger side effects and statement rollback.
                auto statement = single(attempt->statement);
                const auto status = step(statement.get());
                const auto primary = status & 0xff;
                if (status != SQLITE_DONE && primary != SQLITE_ABORT && primary != SQLITE_CONSTRAINT)
                    check(status);
                // Native TryReset ignores sqlite3_reset's last-step status;
                // it returns whether clearing bindings succeeded.
                sqlite3_reset(statement.get());
                check(sqlite3_clear_bindings(statement.get()));
            } else {
                delete_missing_references(std::get<DeleteMissingReferences>(operation));
            }
        }
    }
    void memory_attachment_sql(const std::string &sql) {
        ConnectionGuard guard(db);preview_access();
        internal_memory_attachment = true;
        try {
            auto statement = prepare(sql);
            run(statement.get());
            internal_memory_attachment = false;
        } catch (...) {
            internal_memory_attachment = false;
            throw;
        }
    }
    void attach_snapshot(const Impl &source, const char *schema, const char *source_schema="main") {
        ConnectionGuard source_guard(source.db);source.preview_access();
        ConnectionGuard target_guard(db);preview_access();
        if (&source == this || !sqlite3_get_autocommit(db) || !sqlite3_get_autocommit(source.db))
            throw std::runtime_error("Memory snapshot needs distinct databases without pending transactions");
        memory_attachment_sql("ATTACH ':memory:' AS " + identifier(schema));
        try {
            // No source filename, pointer-size serialization or original BinaryIO
            // cache format. SQLite copies schema, views, triggers and 64-bit values.
            auto *backup = sqlite3_backup_init(db, schema, source.db, source_schema);
            if (!backup)
                check(sqlite3_errcode(db));
            int status;
            do {
                status = sqlite3_backup_step(backup, 256);
            } while (status == SQLITE_OK);
            const auto finish = sqlite3_backup_finish(backup);
            if (status != SQLITE_DONE)
                check(status);
            check(finish);
        } catch (...) {
            try {
                memory_attachment_sql("DETACH " + identifier(schema));
            } catch (...) {
            }
            throw;
        }
    }
};
RulesDatabase::RulesDatabase() : impl_(std::make_shared<Impl>()) {}
RulesDatabase::~RulesDatabase() = default;
std::function<bool(const RulesDatabase&)> RulesDatabase::capture_actual_impl_identity() const {
    if(!impl_||impl_->identity_creator!=std::this_thread::get_id())
        throw std::runtime_error("Actual rules identity capture belongs to another thread");
    return [actual=impl_,creator=impl_->identity_creator](const RulesDatabase& candidate) noexcept {
        // Immutable creator check precedes any candidate pimpl access.
        if(creator!=std::this_thread::get_id())return false;
        const auto& supplied=candidate.impl_;
        return supplied&&supplied.get()==actual.get()&&
            !supplied.owner_before(actual)&&!actual.owner_before(supplied);
    };
}
std::shared_ptr<RulesDatabase> RulesDatabase::new_saved_values() {
    auto result=std::make_shared<RulesDatabase>();
    result->impl_=std::make_shared<Impl>(std::make_shared<Impl::SavedStorage>());
    auto schema=result->impl_->prepare("CREATE TABLE SimpleValues(Name TEXT PRIMARY KEY,Value)");
    result->impl_->run(schema.get());
    return result;
}
std::shared_ptr<RulesDatabase> RulesDatabase::from_saved_image(std::span<const std::uint8_t> image) {
    constexpr std::string_view magic("SQLite format 3\0",16);
    if(image.size()<100||image.size()>static_cast<std::uint64_t>(INT64_MAX)||
       !std::equal(magic.begin(),magic.end(),image.begin()))
        throw std::runtime_error("Invalid saved SQLite image");
    // Borrow only while the validator is alive, and keep the source immutable.
    auto source=std::make_unique<Impl>();
    sqlite3_set_authorizer(source->db,nullptr,nullptr);
    source->check(sqlite3_deserialize(source->db,"main",const_cast<unsigned char*>(image.data()),
        static_cast<sqlite3_int64>(image.size()),static_cast<sqlite3_int64>(image.size()),SQLITE_DESERIALIZE_READONLY));
    sqlite3_db_config(source->db,SQLITE_DBCONFIG_DEFENSIVE,1,nullptr);
    sqlite3_db_config(source->db,SQLITE_DBCONFIG_TRUSTED_SCHEMA,0,nullptr);
    auto check=source->prepare("PRAGMA quick_check");
    std::uint64_t batches=100000;
    sqlite3_progress_handler(source->db,1000,[](void *p){
        auto &remaining=*static_cast<std::uint64_t*>(p);
        if(!remaining)return 1;
        --remaining;return 0;
    },&batches);
    if(!source->next(check.get())||source->column_text(check.get(),0)!="ok"||source->next(check.get()))
        throw std::runtime_error("Saved SQLite integrity check failed");
    check.reset();sqlite3_progress_handler(source->db,0,nullptr,nullptr);
    auto result=std::make_shared<RulesDatabase>();
    result->impl_=std::make_shared<Impl>(std::make_shared<Impl::SavedStorage>());
    auto *backup=sqlite3_backup_init(result->impl_->db,"main",source->db,"main");
    if(!backup)throw std::runtime_error("Cannot copy saved SQLite image");
    const auto copied=sqlite3_backup_step(backup,-1),finished=sqlite3_backup_finish(backup);
    if(copied!=SQLITE_DONE)result->impl_->check(copied);
    result->impl_->check(finished);return result;
}
std::unique_ptr<RulesDatabase> RulesDatabase::reopen_saved() const {
    if(!impl_->saved_storage)throw std::runtime_error("Database has no saved-session backing");
    auto result=std::make_unique<RulesDatabase>();
    result->impl_=std::make_shared<Impl>(impl_->saved_storage);return result;
}
std::unique_ptr<RulesDatabase> RulesDatabase::clone_gameplay() const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    if(impl_->saved_storage||!sqlite3_get_autocommit(impl_->db))
        throw std::runtime_error("Gameplay clone requires activated rules outside a transaction");
    auto result=std::make_unique<RulesDatabase>();
    auto *backup=sqlite3_backup_init(result->impl_->db,"main",impl_->db,"main");
    if(!backup)throw std::runtime_error("Cannot clone gameplay database");
    const auto copied=sqlite3_backup_step(backup,-1),finished=sqlite3_backup_finish(backup);
    if(copied!=SQLITE_DONE)result->impl_->check(copied);
    result->impl_->check(finished);
    if(sqlite3_db_readonly(impl_->db,"Localization")!=-1)
        result->impl_->attach_snapshot(*impl_,"Localization","Localization");
    return result;
}
std::vector<std::uint8_t> RulesDatabase::saved_image() const {
    if(!impl_->saved_storage||!sqlite3_get_autocommit(impl_->db))
        throw std::runtime_error("Saved image requires a saved-session connection outside a transaction");
    for(auto *connection:impl_->saved_storage->connections)
        if(!sqlite3_get_autocommit(connection))
            throw std::runtime_error("Saved database has an unfinished transaction on another connection");
    sqlite3_int64 size=0;
    impl_->internal_serialization=true;
    std::unique_ptr<unsigned char,decltype(&sqlite3_free)> data(sqlite3_serialize(impl_->db,"main",&size,0),sqlite3_free);
    impl_->internal_serialization=false;
    if(!data||size<100)throw std::runtime_error("Cannot serialize saved SQLite image");
    return {data.get(),data.get()+static_cast<std::size_t>(size)};
}
void RulesDatabase::require_saved_checkpoint_boundary() const {
    if(!impl_->saved_storage)
        throw std::runtime_error("Saved checkpoint requires actual saved-session storage");
    for(auto *connection:impl_->saved_storage->connections) {
        Impl::ConnectionGuard guard(connection);
        if(!sqlite3_get_autocommit(connection))
            throw std::runtime_error("Saved checkpoint has an unfinished transaction/savepoint");
        for(auto *statement=sqlite3_next_stmt(connection,nullptr);statement;
            statement=sqlite3_next_stmt(connection,statement))
            if(sqlite3_stmt_busy(statement))
                throw std::runtime_error("Saved checkpoint has an unfinished query");
    }
}
void RulesDatabase::seal_immutable_preview_fingerprints() const {
    Impl::ConnectionGuard guard(impl_->db);
    if(impl_->preview_sealed) throw std::runtime_error("Immutable preview rules scope already sealed");
    if(impl_->saved_storage || !sqlite3_get_autocommit(impl_->db))
        throw std::runtime_error("Immutable preview requires activated unsaved rules outside a transaction");
    for(auto *statement=sqlite3_next_stmt(impl_->db,nullptr);statement;
        statement=sqlite3_next_stmt(impl_->db,statement))
        if(sqlite3_stmt_busy(statement))
            throw std::runtime_error("Immutable preview refuses an active prepared statement");
    impl_->preview_thread=std::this_thread::get_id();
    impl_->preview_sealed=true;
    // The bundled sqlite3_set_authorizer expires all preexisting statements.
    // Actual step also checks readonly; inactive preprepared writes cannot slip
    // past a prepare-only authorizer when they are later used or reprepared.
    const auto status=sqlite3_set_authorizer(impl_->db,Impl::authorize,impl_.get());
    if(status!=SQLITE_OK) {impl_->poison_preview();impl_->check(status);}
}
bool RulesDatabase::immutable_preview_fingerprints_current() const {
    Impl::ConnectionGuard guard(impl_->db);
    return impl_->preview_sealed && !impl_->preview_poisoned &&
        impl_->preview_thread==std::this_thread::get_id();
}
std::function<bool()> RulesDatabase::immutable_preview_fingerprint_currentness() const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    if(!impl_->preview_sealed) return {};
    return [owner=impl_]() noexcept {
        try {
            Impl::ConnectionGuard retained_guard(owner->db);
            return !owner->preview_poisoned && owner->preview_thread==std::this_thread::get_id();
        } catch(...) {return false;}
    };
}
std::uint64_t RulesDatabase::immutable_preview_fingerprint_serializations() const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    return impl_->fingerprint_serializations;
}
std::vector<std::uint8_t> RulesDatabase::gameplay_fingerprint_data(
    std::span<const RulesFingerprintNormalization> normalizations,std::uint64_t maximum_bytes) const {
    const auto key=Impl::pin_fingerprint_key(normalizations);
    const auto views=Impl::key_views(key);
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    if(!impl_->preview_sealed) return uncached_gameplay_fingerprint_data(views,maximum_bytes);
    if(const auto *cached=impl_->cached_fingerprint(key,maximum_bytes)) return cached->bytes;
    if(impl_->fingerprint_building) throw std::runtime_error("Reentered immutable rules fingerprint build");
    impl_->fingerprint_building=true;
    struct Reset {bool &value;~Reset(){value=false;}} reset{impl_->fingerprint_building};
    auto data=uncached_gameplay_fingerprint_data(views,maximum_bytes);
    impl_->preview_access();
    ++impl_->fingerprint_serializations;
    // Bounded cache: at most default plus one normalized key, and128MiB total.
    std::uint64_t retained=0;for(const auto &entry:impl_->fingerprints) retained+=entry.bytes.size();
    constexpr std::uint64_t cache_budget=128ULL*1024*1024;
    const bool normalized_cached=std::any_of(impl_->fingerprints.begin(),impl_->fingerprints.end(),
        [](const auto &entry){return !entry.key.empty();});
    if(Impl::cacheable_key(key) && (key.empty() || !normalized_cached) && impl_->fingerprints.size()<2 &&
       data.size()<=cache_budget-retained) {
        auto digest=runtime::identify_bytes(data).sha256;
        impl_->preview_access();
        impl_->fingerprints.push_back({key,data,std::move(digest)});
    }
    return data;
}
std::string RulesDatabase::gameplay_fingerprint_sha256(
    std::span<const RulesFingerprintNormalization> normalizations,std::uint64_t maximum_bytes) const {
    const auto key=Impl::pin_fingerprint_key(normalizations);
    const auto views=Impl::key_views(key);
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    if(impl_->preview_sealed)
        if(const auto *cached=impl_->cached_fingerprint(key,maximum_bytes)) return cached->sha256;
    const auto data=gameplay_fingerprint_data(views,maximum_bytes);
    if(impl_->preview_sealed)
        if(const auto *cached=impl_->cached_fingerprint(key,maximum_bytes)) return cached->sha256;
    return runtime::identify_bytes(data).sha256;
}
std::vector<std::uint8_t> RulesDatabase::uncached_gameplay_fingerprint_data(
    std::span<const RulesFingerprintNormalization> normalizations,std::uint64_t maximum_bytes) const {
    if(impl_->saved_storage||!sqlite3_get_autocommit(impl_->db))
        throw std::runtime_error("Gameplay fingerprint requires an activated database outside a transaction");
    std::vector<std::uint8_t> data;
    const auto raw=[&](std::span<const std::uint8_t> bytes){
        if(data.size()>maximum_bytes||bytes.size()>maximum_bytes-data.size())
            throw std::length_error("Gameplay fingerprint exceeds host byte budget");
        data.insert(data.end(),bytes.begin(),bytes.end());
    };
    const auto byte=[&](std::uint8_t value){raw(std::span(&value,1));};
    const auto number=[&](std::uint64_t value){for(unsigned i=0;i<8;++i)byte(static_cast<std::uint8_t>(value>>(8*i)));};
    const auto string=[&](std::string_view value){
        number(value.size());
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    };
    const auto cell=[&](sqlite3_stmt *statement,int column){
        const auto kind=sqlite3_column_type(statement,column);
        byte(static_cast<std::uint8_t>(kind));
        switch(kind){
        case SQLITE_NULL:break;
        case SQLITE_INTEGER:number(std::bit_cast<std::uint64_t>(sqlite3_column_int64(statement,column)));break;
        case SQLITE_FLOAT:number(std::bit_cast<std::uint64_t>(sqlite3_column_double(statement,column)));break;
        case SQLITE_TEXT:
        case SQLITE_BLOB:{
            const auto *value=kind==SQLITE_TEXT?sqlite3_column_text(statement,column):
                static_cast<const unsigned char*>(sqlite3_column_blob(statement,column));
            const auto size=sqlite3_column_bytes(statement,column);
            if(size<0||(size>0&&!value))throw std::runtime_error("Invalid gameplay fingerprint field");
            number(static_cast<std::uint64_t>(size));
            if(size)raw({value,static_cast<std::size_t>(size)});
            break;
        }
        default:throw std::runtime_error("Unsupported gameplay fingerprint SQLite value");
        }
    };
    string("CivisGameplayRules01");
    std::vector<std::string> tables;
    auto schema=impl_->prepare("SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name,tbl_name,sql");
    while(impl_->next(schema.get())){
        byte(1);
        for(int i=0;i<4;++i)cell(schema.get(),i);
        if(sqlite3_column_type(schema.get(),0)==SQLITE_TEXT&&
           std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(schema.get(),0)))=="table")
            tables.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(schema.get(),1)));
    }
    schema.reset();byte(0);
    for(const auto &table:tables){
        byte(2);string(table);
        const RulesFingerprintNormalization *normalization=nullptr;
        for(const auto &candidate:normalizations)
            if(candidate.table==table){
                if(normalization||candidate.stable_key.empty()||candidate.generated_id.empty()||
                   candidate.stable_key==candidate.generated_id)
                    throw std::runtime_error("Invalid gameplay fingerprint normalization");
                normalization=&candidate;
            }
        byte(normalization?1:0);
        if(normalization){string(normalization->stable_key);string(normalization->generated_id);}
        const auto source=normalization?
            "SELECT * FROM "+identifier(table)+" ORDER BY "+identifier(std::string(normalization->stable_key))+" COLLATE BINARY":
            "SELECT rowid,* FROM "+identifier(table)+" ORDER BY rowid";
        sqlite3_stmt *statement=nullptr;
        auto status=sqlite3_prepare_v2(impl_->db,source.c_str(),-1,&statement,nullptr);
        if(status!=SQLITE_OK){
            if(statement)sqlite3_finalize(statement);
            if(normalization)impl_->check(status);
            const auto fallback="SELECT * FROM "+identifier(table);
            status=sqlite3_prepare_v2(impl_->db,fallback.c_str(),-1,&statement,nullptr);
            impl_->check(status);
        }
        std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> rows(statement,sqlite3_finalize);
        const auto columns=sqlite3_column_count(rows.get());
        int key_column=-1,ignored_column=-1;
        if(normalization){
            for(int i=0;i<columns;++i){
                const std::string_view name=sqlite3_column_name(rows.get(),i);
                if(name==normalization->stable_key)key_column=i;
                if(name==normalization->generated_id)ignored_column=i;
            }
            if(key_column<0||ignored_column<0)
                throw std::runtime_error("Gameplay fingerprint normalization column is missing");
        }
        number(static_cast<std::uint64_t>(columns-(normalization?1:0)));
        std::string previous_key;
        bool has_previous=false;
        while(impl_->next(rows.get())){
            if(normalization){
                if(sqlite3_column_type(rows.get(),key_column)!=SQLITE_TEXT)
                    throw std::runtime_error("Gameplay fingerprint stable key is not text");
                const auto *key=sqlite3_column_text(rows.get(),key_column);
                const auto size=sqlite3_column_bytes(rows.get(),key_column);
                const std::string current(reinterpret_cast<const char*>(key),static_cast<std::size_t>(size));
                if(has_previous&&current==previous_key)
                    throw std::runtime_error("Gameplay fingerprint stable key is duplicated");
                previous_key=current;has_previous=true;
            }
            byte(3);
            for(int i=0;i<columns;++i)if(i!=ignored_column)cell(rows.get(),i);
        }
        byte(4);
    }
    return data;
}
struct RulesScriptCursor::Impl {
    std::shared_ptr<RulesDatabase::Impl> owner;
    RulesDatabase::Impl::Statement statement{nullptr,sqlite3_finalize};
    std::string error;
    bool current=false;
};
RulesScriptCursor::RulesScriptCursor(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
RulesScriptCursor::~RulesScriptCursor()=default;
std::string_view RulesScriptCursor::error()const{return impl_->error;}
std::optional<DatabaseScriptRow> RulesScriptCursor::next(){
    if(!step()){reset();return {};}
    return row();
}
void RulesScriptCursor::reset(){
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_cleanup_access();

    impl_->current=false;
    if(auto *statement=impl_->statement.get()){
        // Results::Reset calls sqlite3_reset, then clears binds only on success.
        if(sqlite3_reset(statement)==SQLITE_OK)sqlite3_clear_bindings(statement);
    }
}
void RulesScriptCursor::release(){
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_cleanup_access();
impl_->current=false;impl_->statement.reset();}
bool RulesScriptCursor::step(){
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_access();
    auto *statement=impl_->statement.get();auto &owner=*impl_->owner;
    impl_->error.clear();impl_->current=false;
    if(!statement)return false;
    const bool previous=owner.mod_sql;owner.mod_sql=true;
    const int status=owner.step(statement);owner.mod_sql=previous;
    if(status!=SQLITE_ROW){
        if(status!=SQLITE_DONE)impl_->error=sqlite3_errmsg(owner.db);
        return false;
    }
    impl_->current=true;return true;
}
DatabaseScriptRow RulesScriptCursor::row()const{
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_access();
    if(!impl_->current)throw std::runtime_error("Script query has no current row");
    auto *statement=impl_->statement.get();
    DatabaseScriptRow row;
    for(int column=0;column<sqlite3_column_count(statement);++column){
        DatabaseScriptValue value;
        switch(sqlite3_column_type(statement,column)){
        case SQLITE_INTEGER:{
            const auto integer=static_cast<std::int64_t>(sqlite3_column_int64(statement,column));
            const char *declared=sqlite3_column_decltype(statement,column);
            if(declared&&std::string_view(declared)=="boolean")value=integer!=0;
            else value=integer;
            break;
        }
        case SQLITE_FLOAT:value=sqlite3_column_double(statement,column);break;
        case SQLITE_TEXT:{
            const auto *text=sqlite3_column_text(statement,column);
            if(!text)throw std::runtime_error("SQLite script text conversion failed");
            // Retail PushDatabaseRow uses lua_pushstring, including C-string
            // termination rather than preserving an embedded SQL text NUL.
            value=std::string(reinterpret_cast<const char*>(text));break;
        }
        default:break; // Native BLOB and NULL are nil, but their column exists.
        }
        row.emplace_back(sqlite3_column_name(statement,column),std::move(value));
    }
    return row;
}
std::pair<bool,DatabaseScriptValue> RulesScriptCursor::cache_key(bool id)const{
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_access();
    if(!impl_->current)throw std::runtime_error("Script query has no current row");
    auto *statement=impl_->statement.get();
    for(int column=0;column<sqlite3_column_count(statement);++column){
        if(std::string_view(sqlite3_column_name(statement,column))!=(id?"ID":"Type"))continue;
        if(id)return {true,static_cast<std::int64_t>(sqlite3_column_int64(statement,column))};
        const auto *text=sqlite3_column_text(statement,column);
        if(!text)return {true,std::monostate{}};
        return {true,std::string(reinterpret_cast<const char*>(text))};
    }
    return {false,std::monostate{}};
}
std::unique_ptr<RulesScriptCursor> RulesDatabase::script_query(std::string_view sql,
                                  std::span<const DatabaseScriptValue> parameters)const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    auto cursor=std::make_unique<RulesScriptCursor::Impl>();cursor->owner=impl_;
    const bool previous=impl_->mod_sql;impl_->mod_sql=true;
    try {cursor->statement=impl_->prepare(std::string(sql.substr(0,sql.find('\0'))));}
    catch(const Impl::SqlError&){impl_->mod_sql=previous;return {};}
    catch(...){impl_->mod_sql=previous;throw;}
    impl_->mod_sql=previous;
    if(!cursor->statement)return {};
    auto result=std::unique_ptr<RulesScriptCursor>(new RulesScriptCursor(std::move(cursor)));
    result->bind(parameters);return result;
}
std::int64_t RulesDatabase::script_statement_count()const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    std::int64_t count=0;
    for(auto* statement=sqlite3_next_stmt(impl_->db,nullptr);statement;statement=sqlite3_next_stmt(impl_->db,statement))++count;
    return count; // actual DLL100012E0, same retained connection
}
std::optional<std::string> RulesDatabase::script_statement_sql(std::int64_t index)const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    auto* statement=sqlite3_next_stmt(impl_->db,nullptr);std::int64_t current=0;
    while(statement){
        if(current==index){const char*const sql=sqlite3_sql(statement);return sql?std::optional<std::string>(sql):std::nullopt;}
        ++current;statement=sqlite3_next_stmt(impl_->db,statement);
    }
    return {}; // original zero-based/negative/out-of-range returns NULL
}
void RulesScriptCursor::bind(std::span<const DatabaseScriptValue> parameters,bool ignore_errors){
    RulesDatabase::Impl::ConnectionGuard guard(impl_->owner->db);impl_->owner->preview_access();
    if(!impl_->statement)throw std::runtime_error("Script query has been released");
    const auto available=static_cast<std::size_t>(sqlite3_bind_parameter_count(impl_->statement.get()));
    for(std::size_t i=0;i<std::min(available,parameters.size());++i){
        auto *statement=impl_->statement.get();const int index=static_cast<int>(i+1);const auto &value=parameters[i];
        int status=SQLITE_OK;
        switch(value.index()){
        case 0:status=sqlite3_bind_null(statement,index);break;
        case 1:status=sqlite3_bind_int64(statement,index,std::get<bool>(value)?1:0);break;
        case 2:status=sqlite3_bind_int64(statement,index,std::get<std::int64_t>(value));break;
        case 3:status=sqlite3_bind_double(statement,index,std::get<double>(value));break;
        case 4:{
            const auto &text=std::get<std::string>(value);
            const auto end=text.find('\0'),size=end==std::string::npos?text.size():end;
            status=sqlite3_bind_text64(statement,index,text.data(),size,SQLITE_TRANSIENT,SQLITE_UTF8);break;
        }
        }
        if(!ignore_errors)impl_->owner->check(status);
    }
}
void RulesDatabase::apply_xml(const XmlNode &document) {
    const auto operations = compile_game_data(document);
    impl_->transaction("BEGIN");
    try {
        impl_->run_game_data(operations);
        impl_->transaction("COMMIT");
    } catch (...) {
        impl_->rollback_if_active();
        throw;
    }
}
void RulesDatabase::apply_statements(std::span<const DatabaseStatement> statements) {
    impl_->transaction("BEGIN");
    try {
        for (const auto &source : statements) {
            auto statement = impl_->single(source);
            impl_->run(statement.get());
        }
        impl_->transaction("COMMIT");
    } catch (...) {
        impl_->rollback_if_active();
        throw;
    }
}
void RulesDatabase::apply_sql(const std::string &sql) {
    if (sql.find('\0') != std::string::npos)
        throw std::runtime_error("NUL in SQL source");
    impl_->transaction("BEGIN");
    try {
        std::uint64_t completed = 0;
        impl_->run_sql(sql, false, completed);
        impl_->transaction("COMMIT");
    } catch (...) {
        impl_->rollback_if_active();
        throw;
    }
}
void RulesDatabase::apply_localization_schema(const std::string &sql) {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    impl_->localization_schema = true;
    try {
        apply_sql(sql);
        impl_->localization_schema = false;
    } catch (...) {
        impl_->localization_schema = false;
        throw;
    }
}
std::uint64_t RulesDatabase::convert_localization_tables() {
    std::vector<DatabaseStatement> statements;
    const auto count = query(
        {"select Type, TableName, TagColumnName, TextColumnName, PluralityColumnName, GenderColumnName "
         "from Languages",
         {}},
        [&](auto row) {
            const auto &language = std::get<std::string>(row[0]);
            const auto &table_name = std::get<std::string>(row[1]);
            const auto table = identifier(table_name);
            const auto tag = identifier(std::get<std::string>(row[2]));
            const auto text = identifier(std::get<std::string>(row[3]));
            const auto plurality = identifier(std::get<std::string>(row[4]));
            const auto gender = identifier(std::get<std::string>(row[5]));
            const auto value = quote(language, '\'');
            statements.push_back({"INSERT OR IGNORE INTO LanguagePriorities(LanguageSet, Language, "
                                  "Priority) VALUES(?, ?, 100)",
                                  {language, language}});
            statements.push_back({"INSERT INTO LocalizedText(Language, Tag, Text, Gender, Plurality) "
                                  "SELECT " +
                                      value + "," + tag + "," + text + "," + gender + "," + plurality +
                                      " FROM " + table,
                                  {}});
            statements.push_back({"DROP TABLE " + table, {}});
            statements.push_back({"CREATE VIEW " + table +
                                      " AS SELECT Tag, Text, Gender, Plurality "
                                      "from LocalizedText where Language = " +
                                      value,
                                  {}});
            statements.push_back(
                {"CREATE TRIGGER " + identifier(table_name + "_insert") + " INSTEAD OF INSERT ON " + table +
                     " BEGIN insert into LocalizedText(Language, Tag, Text, Gender, Plurality) "
                     "VALUES(" +
                     value + ", NEW." + tag + ", NEW." + text + ", NEW." + gender + ", NEW." + plurality +
                     "); END",
                 {}});
            // Native uses NEW.Tag in the predicate: a tag rename does not move
            // the old row. Do not silently substitute OLD.Tag here.
            statements.push_back({"CREATE TRIGGER " + identifier(table_name + "_update") +
                                      " INSTEAD OF UPDATE ON " + table +
                                      " BEGIN UPDATE LocalizedText SET Text = NEW." + text +
                                      ", Gender = NEW." + gender + ", Plurality = NEW." + plurality +
                                      " WHERE Language = " + value + " and Tag = NEW." + tag + "; END",
                                  {}});
            statements.push_back({"CREATE TRIGGER " + identifier(table_name + "_delete") +
                                      " INSTEAD OF DELETE ON " + table +
                                      " BEGIN DELETE from LocalizedText where Language = " + value +
                                      " and Tag = OLD." + tag + "; END",
                                  {}});
        });
    apply_statements(statements);
    return count;
}
void RulesDatabase::merge_localization(const RulesDatabase &package) {
    impl_->attach_snapshot(*package.impl_, "ContentPackage");
    try {
        apply_sql("INSERT OR REPLACE INTO LocalizedText SELECT * FROM ContentPackage.LocalizedText;");
        impl_->memory_attachment_sql("DETACH ContentPackage");
    } catch (...) {
        // apply_sql owns and closes its transaction before detaching.
        try {
            impl_->memory_attachment_sql("DETACH ContentPackage");
        } catch (...) {
        }
        throw;
    }
}
void RulesDatabase::track_localization_mod_changes() {
    apply_sql("CREATE TRIGGER If Not Exists ModText_Insert AFTER INSERT ON LocalizedText BEGIN "
              "Insert or Ignore into ScannedFiles(Path, DateTime) VALUES('Mods', 0); END;"
              "CREATE TRIGGER If Not Exists ModText_Delete AFTER DELETE ON LocalizedText BEGIN "
              "Insert or Ignore into ScannedFiles(Path, DateTime) VALUES('Mods', 0); END;"
              "CREATE TRIGGER If Not Exists ModText_Update AFTER UPDATE ON LocalizedText BEGIN "
              "Insert or Ignore into ScannedFiles(Path, DateTime) VALUES('Mods', 0); END;");
}
void RulesDatabase::attach_localization(const RulesDatabase &localization) {
    impl_->attach_snapshot(*localization.impl_, "Localization");
}
void RulesDatabase::apply_mod_xml(const XmlNode &document, const GameDataPackageTag &package_tag) {
    const auto operations = compile_game_data(document, package_tag);
    impl_->transaction("SAVEPOINT _;");
    try {
        impl_->run_game_data(operations);
        impl_->transaction("RELEASE _;");
    } catch (...) {
        impl_->rollback_if_active();
        throw;
    }
}
ModSqlResult RulesDatabase::apply_mod_sql(const std::string &sql) {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    if (sql.find('\0') != std::string::npos)
        throw std::runtime_error("NUL in SQL source");
    ModSqlResult result;
    impl_->transaction("SAVEPOINT _;");
    try {
        impl_->mod_sql = true;
        try {
            // Native ExecuteMultiple prepares/steps directly, without binding.
            impl_->run_sql(sql, true, result.completed_statements);
        } catch (const Impl::SqlError &error) {
            result.success = false;
            result.error_code = error.code;
            result.error = error.what();
        }
        impl_->mod_sql = false;
        try {
            // The original SQL action does this unconditionally. In particular,
            // RAISE(ROLLBACK) has already removed the savepoint: log the release
            // error without committing anything or masking the execution error.
            impl_->transaction("RELEASE _;");
        } catch (const Impl::SqlError &error) {
            result.release_error = error.what();
        }
    } catch (...) {
        impl_->mod_sql = false;
        impl_->rollback_if_active();
        throw;
    }
    result.transaction_active_after = !sqlite3_get_autocommit(impl_->db);
    return result;
}
IdRemapReport RulesDatabase::remap_vp_ids(const IdRemapBudget &budget) {
    IdRemapReport report;
    const auto error = [&](std::vector<IdRemapError> &errors, const char *stage,
                           const Impl::SqlError &failure) {
        report.all_succeeded = false;
        errors.push_back({stage, failure.what(), failure.code});
    };
    Impl::Statement tables(nullptr, sqlite3_finalize);
    try {
        tables = impl_->prepare("SELECT name FROM Remapper");
    } catch (const Impl::SqlError &failure) {
        error(report.errors, "remapper", failure);
        return report;
    }
    for (;;) {
        try {
            if (!impl_->next(tables.get()))
                break;
        } catch (const Impl::SqlError &failure) {
            error(report.errors, "remapper", failure);
            break;
        }
        if (report.tables.size() >= budget.table_entries)
            throw std::runtime_error("ID remapper exceeds table entry budget");
        report.tables.push_back({});
        auto &table = report.tables.back();
        table.name = impl_->column_text(tables.get(), 0);
        if (table.name.empty() || table.name.find('\0') != std::string::npos) {
            report.all_succeeded = false;
            table.errors.push_back({"table_name", "Empty or NUL-containing remapper table name", 0});
            continue;
        }
        const auto quoted = identifier(table.name);
        const char *stage = "count";
        try {
            // The original Count() is signed-32. Preserve its > 0 predicate
            // with SQLite's full 64-bit count instead of wrapping large tables.
            auto count = impl_->prepare("SELECT count(*) FROM " + quoted);
            if (!impl_->next(count.get()))
                throw std::runtime_error("ID remapper count returned no row");
            const auto rows = sqlite3_column_int64(count.get(), 0);
            if (rows <= 0)
                continue;
            table.rows = static_cast<std::uint64_t>(rows);
            count.reset();
            stage = "table_info";
            {
                auto columns = impl_->table_info(table.name);
                while (impl_->next(columns.get())) {
                    // Exact name and first PK position, including composite PK.
                    if (impl_->column_text(columns.get(), 1) == "ID" &&
                        sqlite3_column_int64(columns.get(), 5) == 1) {
                        table.eligible = true;
                        break;
                    }
                }
            }
            if (!table.eligible)
                continue;
            stage = "collect_ids";
            std::vector<std::int64_t> ids;
            auto query = impl_->prepare("SELECT ID FROM " + quoted + " ORDER BY ID");
            for (;;) {
                try {
                    if (!impl_->next(query.get()))
                        break;
                } catch (const Impl::SqlError &failure) {
                    error(table.errors, stage, failure);
                    break; // Native Step stops; an already collected prefix remains.
                }
                const auto id = sqlite3_column_int64(query.get(), 0);
                if (id < 0) {
                    table.negative_id_seen = true;
                    break;
                }
                if (ids.size() >= budget.ids_per_table)
                    throw std::runtime_error("ID remapper exceeds per-table ID budget");
                ids.push_back(id);
            }
            table.collected_ids = ids.size();
            // Keep the source query alive through updates, as the native Results
            // object does (including an early break on a negative ID).
            stage = "prepare_update";
            auto update = impl_->prepare("UPDATE " + quoted + " SET ID = ? WHERE ID = ?");
            for (std::uint64_t index = 0; index < ids.size(); ++index) {
                if (index > static_cast<std::uint64_t>(INT64_MAX))
                    throw std::runtime_error("ID remapper ordinal exceeds signed-64 storage");
                const auto next_id = static_cast<std::int64_t>(index);
                if (ids[static_cast<std::size_t>(index)] == next_id)
                    continue;
                ++table.attempted_updates;
                try {
                    impl_->check(sqlite3_bind_int64(update.get(), 1, next_id));
                    impl_->check(sqlite3_bind_int64(update.get(), 2, ids[static_cast<std::size_t>(index)]));
                    impl_->run(update.get());
                    ++table.completed_updates;
                } catch (const Impl::SqlError &failure) {
                    error(table.errors, "update", failure);
                }
                // DLL 0x104B4718..0x104B472A ignores Execute/Reset results and
                // advances to the next original ID. Rebind both values each time.
                sqlite3_reset(update.get());
                impl_->check(sqlite3_clear_bindings(update.get()));
            }
        } catch (const Impl::SqlError &failure) {
            error(table.errors, stage, failure);
        }
    }
    return report;
}
GameplayPostprocessReport RulesDatabase::postprocess_gameplay(
    GameplayPostprocessProfile profile, std::span<const GameplayDefine> runtime_defines,
    const GameplayPostprocessBudget &budget) {
    return postprocess_defines(profile, runtime_defines, budget, false);
}
GameplayPostprocessReport RulesDatabase::prefetch_custom_mod_options(const GameplayPostprocessBudget &budget) {
    return postprocess_defines(GameplayPostprocessProfile::vp542, {}, budget, true);
}
GameplayPostprocessReport RulesDatabase::postprocess_defines(
    GameplayPostprocessProfile profile, std::span<const GameplayDefine> runtime_defines,
    const GameplayPostprocessBudget &budget, bool custom) {
    if (profile != GameplayPostprocessProfile::bnw && profile != GameplayPostprocessProfile::vp542)
        throw std::runtime_error("Unknown gameplay postprocessing profile");
    for (const auto &define : runtime_defines)
        if (define.name.empty() || define.name.find('\0') != std::string::npos)
            throw std::runtime_error("Invalid runtime define name");
    GameplayPostprocessReport report;
    const auto error = [&](std::vector<GameplayPostprocessError> &errors, const char *stage,
                           const Impl::SqlError &failure) {
        report.all_succeeded = false;
        errors.push_back({stage, failure.what(), failure.code});
    };
    const auto insert_sql = custom ? "INSERT OR REPLACE INTO CustomModOptions(Name, Value) VALUES(?, ?)"
                                   : "INSERT OR REPLACE INTO Defines(Name, Value) VALUES(?, ?)";
    impl_->transaction("SAVEPOINT _;");
    try {
        Impl::Statement insert(nullptr, sqlite3_finalize), rows(nullptr, sqlite3_finalize);
        try { insert = impl_->prepare(insert_sql); }
        catch (const Impl::SqlError &failure) { error(report.errors, "prepare_insert", failure); }
        std::array<int, 3> columns{-1, -1, -1};
        try {
            rows = impl_->prepare(custom ? "SELECT * FROM CustomModPostDefines" : "SELECT * FROM PostDefines");
            const std::array names{"Name", custom ? "Type" : "Key", "Table"};
            for (int i = 0; i < sqlite3_column_count(rows.get()); ++i)
                for (std::size_t n = 0; n < names.size(); ++n)
                    if (std::string_view(sqlite3_column_name(rows.get(), i)) == names[n])
                        columns[n] = i;
            if (std::any_of(columns.begin(), columns.end(), [](int i) { return i < 0; }))
                throw Impl::SqlError(SQLITE_ERROR, "Missing PostDefines column");
        } catch (const Impl::SqlError &failure) {
            error(report.errors, "post_defines", failure);
            rows.reset();
        }
        while (rows) {
            try { if (!impl_->next(rows.get())) break; }
            catch (const Impl::SqlError &failure) { error(report.errors, "post_defines", failure); break; }
            if (report.post_defines.size() >= budget.post_defines)
                throw std::runtime_error("PostDefines exceeds host entry budget");
            report.post_defines.push_back({});
            auto &entry = report.post_defines.back();
            entry.name = impl_->column_text(rows.get(), columns[0]);
            entry.key = impl_->column_text(rows.get(), columns[1]);
            entry.table = impl_->column_text(rows.get(), columns[2]);
            if (entry.table.empty() || entry.table.find('\0') != std::string::npos ||
                entry.name.find('\0') != std::string::npos || entry.key.find('\0') != std::string::npos) {
                report.all_succeeded = false;
                entry.errors.push_back({"metadata", "Invalid PostDefines identifier or text", 0});
                continue;
            }
            const char *stage = "lookup";
            try {
                auto sql = "SELECT ROWID FROM " + identifier(entry.table) + " WHERE Type = ";
                // Retail BNW embeds the authored key in a SQL literal without
                // escaping. Retain its syntax errors/expressions; do not silently
                // repair a mod. VP binds the key instead. Neither uses a fixed buffer.
                sql += profile == GameplayPostprocessProfile::bnw ? "'" + entry.key + "' LIMIT 1"
                                                                  : "? LIMIT 1";
                DatabaseStatement lookup_source{sql, {}};
                if (profile == GameplayPostprocessProfile::vp542) {
                    if (sqlite3_column_type(rows.get(), columns[1]) == SQLITE_NULL)
                        lookup_source.parameters.push_back(std::monostate{});
                    else
                        lookup_source.parameters.push_back(entry.key);
                }
                auto lookup = impl_->single(lookup_source);
                if (!impl_->next(lookup.get())) continue;
                entry.rowid = sqlite3_column_int64(lookup.get(), 0);
                if (!insert) continue;
                stage = "insert";
                const std::array<DatabaseValue, 2> values{
                    sqlite3_column_type(rows.get(), columns[0]) == SQLITE_NULL
                        ? DatabaseValue(std::monostate{}) : DatabaseValue(entry.name),
                    *entry.rowid};
                impl_->bind(insert.get(), values);
                impl_->run(insert.get());
                entry.applied = true;
            } catch (const Impl::SqlError &failure) { error(entry.errors, stage, failure); }
            if (insert) {
                sqlite3_reset(insert.get());
                impl_->check(sqlite3_clear_bindings(insert.get()));
            }
        }
        const auto update = [&](const std::string &sql, const char *stage, bool &applied) {
            try {
                auto statement = impl_->prepare(sql);
                impl_->run(statement.get());
                applied = true;
            } catch (const Impl::SqlError &failure) { error(report.errors, stage, failure); }
        };
        std::string strategic = "UPDATE Resources SET ResourceUsage = 1 WHERE EXISTS "
                                "(SELECT * FROM Unit_ResourceQuantityRequirements WHERE ResourceType = Type)";
        if (profile == GameplayPostprocessProfile::vp542)
            strategic += " OR EXISTS (SELECT * FROM Building_ResourceQuantityRequirements WHERE ResourceType = Type)";
        if (!custom) {
            update(strategic, "strategic_resources", report.strategic_resources_applied);
            update("UPDATE Resources SET ResourceUsage = 2 WHERE Happiness > 0", "luxury_resources",
                   report.luxury_resources_applied);
        }
        // The original prepares a fresh statement for this final phase.
        if (!custom) try {
            auto runtime_insert = impl_->prepare(insert_sql);
            for (const auto &define : runtime_defines) {
                try {
                    impl_->bind(runtime_insert.get(), std::array<DatabaseValue, 2>{define.name, define.value});
                    impl_->run(runtime_insert.get());
                    ++report.runtime_defines_applied;
                } catch (const Impl::SqlError &failure) { error(report.errors, "runtime_define", failure); }
                sqlite3_reset(runtime_insert.get());
                impl_->check(sqlite3_clear_bindings(runtime_insert.get()));
            }
        } catch (const Impl::SqlError &failure) { error(report.errors, "runtime_defines", failure); }
        try {
            impl_->transaction("RELEASE _;");
            report.transaction_released = true;
        } catch (const Impl::SqlError &failure) { error(report.errors, "release", failure); }
    } catch (...) {
        impl_->rollback_if_active();
        throw;
    }
    return report;
}
RulesRowView::RulesRowView(void *statement) : statement_(statement) {
    auto *row = static_cast<sqlite3_stmt *>(statement_);
    for (int i = 0; i < sqlite3_column_count(row); ++i) {
        const auto *name = sqlite3_column_name(row, i);
        if (!name) throw std::runtime_error("Rules column name allocation failed");
        columns_[name] = i;
    }
}
int RulesRowView::column(std::string_view name) const {
    const auto found = columns_.find(name.substr(0, name.find('\0')));
    return found == columns_.end() ? -1 : found->second;
}
bool RulesRowView::has_column(std::string_view name) const { return column(name) >= 0; }
std::int64_t RulesRowView::integer(std::string_view name) const {
    return integer_at(column(name));
}
std::int64_t RulesRowView::integer_at(int index) const {
    auto *row = static_cast<sqlite3_stmt *>(statement_);
    return index < 0 || index >= sqlite3_column_count(row) ? 0 : sqlite3_column_int64(row, index);
}
double RulesRowView::real(std::string_view name) const {
    const auto index = column(name);
    return index < 0 ? 0.0 : sqlite3_column_double(static_cast<sqlite3_stmt *>(statement_), index);
}
std::optional<std::string> RulesRowView::text(std::string_view name) const {
    const auto index = column(name);
    auto *row = static_cast<sqlite3_stmt *>(statement_);
    if (index < 0 || sqlite3_column_type(row, index) == SQLITE_NULL) return {};
    const auto *value = sqlite3_column_text(row, index);
    if (!value) throw std::runtime_error("Rules column text allocation failed");
    // GetText consumers use a C string, including the first-NUL boundary.
    return std::string(reinterpret_cast<const char *>(value));
}
bool RulesRowView::boolean(std::string_view name) const {
    const auto index = column(name);
    auto *row = static_cast<sqlite3_stmt *>(statement_);
    if (index < 0) return false;
    switch (sqlite3_column_type(row, index)) {
    case SQLITE_INTEGER: return sqlite3_column_int64(row, index) != 0;
    case SQLITE_TEXT: {
        const auto *value = sqlite3_column_text(row, index);
        if (!value) throw std::runtime_error("Rules column text allocation failed");
        return value[0] == 't' || value[0] == 'T';
    }
    default: return false;
    }
}
InfoCollectionReport RulesDatabase::prefetch_info(const std::string &table, const InfoVisitor &visitor,
                                                  const InfoCollectionBudget &budget) const {
    InfoCollectionReport report;
    Impl::Statement rows{nullptr, sqlite3_finalize};
    const auto failure = [&](const Impl::SqlError &error) {
        report.sqlite_code = error.code;
        report.error = error.what();
    };
    try { rows = impl_->prepare("SELECT * FROM " + identifier(table) + " WHERE ID > -1 ORDER BY ID"); }
    catch (const Impl::SqlError &error) { failure(error); return report; }
    const RulesRowView row(rows.get());
    for (;;) {
        // Catch only our own SQL step. A nested callback's exception must not
        // be mistaken for a source SelectWhere/Step failure.
        try { if (!impl_->next(rows.get())) break; }
        catch (const Impl::SqlError &error) { failure(error); break; }
        if (report.rows >= budget.rows)
            throw std::runtime_error("Info collection rows exceed host budget");
        const auto id = row.integer("ID");
        // The source casts GetInt to size_t, including coercible text IDs.
        // Widen both sides to 64 bits without copying x86 wraparound.
        const auto slot = std::max(report.slots, static_cast<std::uint64_t>(id));
        if (slot == UINT64_MAX)
            throw std::runtime_error("Info collection logical span exceeds uint64");
        report.holes += slot - report.slots;
        if (static_cast<std::uint64_t>(id) < report.slots) ++report.overlapping_ids;
        if (visitor && !visitor(slot, id, row)) ++report.cache_failures;
        ++report.rows;
        report.slots = slot + 1;
    }
    return report;
}
RulesQueryReport RulesDatabase::query_rows(const DatabaseStatement &statement,
                                           const std::function<void(const RulesRowView &)> &visitor,
                                           std::uint64_t maximum_rows) const {
    RulesQueryReport report;
    Impl::Statement rows{nullptr, sqlite3_finalize};
    const auto failure = [&](const Impl::SqlError &error) {
        report.sqlite_code = error.code;
        report.error = error.what();
    };
    try {
        rows = impl_->prepare(statement.sql);
        if (!rows) throw std::runtime_error("Empty rules row query");
        impl_->bind(rows.get(), statement.parameters);
    } catch (const Impl::SqlError &error) { failure(error); return report; }
    const RulesRowView row(rows.get());
    for (;;) {
        try { if (!impl_->next(rows.get())) break; }
        catch (const Impl::SqlError &error) { failure(error); break; }
        if (report.rows >= maximum_rows) throw std::runtime_error("Rules query rows exceed host budget");
        visitor(row);
        ++report.rows;
    }
    return report;
}
InfoTypeCatalog RulesDatabase::cache_info_types(const InfoTypeBudget &budget) const {
    InfoTypeCatalog result;
    auto &report = result.report_;
    const auto error = [&](const std::string &table, const Impl::SqlError &failure) {
        report.errors.push_back({table, failure.what(), failure.code});
    };
    try {
        auto tables = impl_->prepare("SELECT name FROM main.sqlite_master WHERE type='table'");
        while (impl_->next(tables.get())) {
            if (report.tables >= budget.tables)
                throw std::runtime_error("Info type tables exceed host budget");
            ++report.tables;
            const auto table = impl_->column_text(tables.get(), 0);
            try {
                // SelectAll uses ordinary SQLite name resolution. An authored
                // temporary object can shadow a table from the main inventory.
                auto rows = impl_->prepare("SELECT * FROM " + identifier(table));
                // Native checks columns only after the first Step succeeds.
                if (!impl_->next(rows.get())) continue;
                int id_column = -1, type_column = -1;
                for (int i = 0; i < sqlite3_column_count(rows.get()); ++i) {
                    const std::string_view name(sqlite3_column_name(rows.get(), i));
                    if (name == "ID") id_column = i;
                    if (name == "Type") type_column = i;
                }
                if (id_column < 0 || type_column < 0) continue;
                ++report.eligible_tables;
                do {
                    if (report.rows >= budget.rows)
                        throw std::runtime_error("Info type rows exceed host budget");
                    ++report.rows;
                    if (sqlite3_column_type(rows.get(), type_column) == SQLITE_NULL) {
                        ++report.null_types;
                        continue;
                    }
                    auto type = impl_->column_text(rows.get(), type_column);
                    // Native GetText returns a C string; only its prefix is used.
                    if (const auto nul = type.find('\0'); nul != std::string::npos) type.resize(nul);
                    const auto hash = saved_info_type_hash(type);
                    result.insert({std::move(type), table,
                                   sqlite3_column_int64(rows.get(), id_column), hash}, budget);
                } while (impl_->next(rows.get()));
            } catch (const Impl::SqlError &failure) { error(table, failure); }
        }
    } catch (const Impl::SqlError &failure) { error("sqlite_master", failure); }
    return result;
}
GlobalDefineLookup RulesDatabase::global_define(std::string_view name, GlobalDefineKind kind) const {
    Impl::ConnectionGuard guard(impl_->db);impl_->preview_access();
    GlobalDefineLookup result;
    switch (kind) {
    case GlobalDefineKind::integer: result.value = std::int64_t{0}; break;
    case GlobalDefineKind::source_float:
    case GlobalDefineKind::real: result.value = 0.0; break;
    case GlobalDefineKind::text: result.value = std::string{}; break;
    default: throw std::runtime_error("Unknown global define kind");
    }
    try {
        if (!impl_->global_lookup)
            impl_->global_lookup = impl_->prepare("SELECT Value from Defines where Name = ? LIMIT 1");
        auto *statement = impl_->global_lookup.get();
        const std::array<DatabaseValue, 1> names{std::string(name.substr(0, name.find('\0')))};
        impl_->bind(statement, names);
        if (impl_->next(statement)) {
            switch (kind) {
            case GlobalDefineKind::integer:
                result.value = sqlite3_column_int64(statement, 0);
                break;
            case GlobalDefineKind::source_float: {
                const auto real = sqlite3_column_double(statement, 0);
                // Defined handling of source float overflow, without invoking
                // an out-of-range C++ floating conversion.
                static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits == 24);
                const auto limit = static_cast<double>(std::numeric_limits<float>::max());
                const auto overflow = limit + std::ldexp(1.0, 103);
                if (std::abs(real) >= overflow)
                    result.value = std::copysign(std::numeric_limits<double>::infinity(), real);
                else if (std::abs(real) > limit)
                    result.value = std::copysign(limit, real);
                else
                    result.value = static_cast<double>(static_cast<float>(real));
                break;
            }
            case GlobalDefineKind::real:
                result.value = sqlite3_column_double(statement, 0);
                break;
            case GlobalDefineKind::text: {
                auto value = impl_->column_text(statement, 0);
                if (const auto nul = value.find('\0'); nul != std::string::npos) value.resize(nul);
                result.value = std::move(value);
                break;
            }
            }
            result.found = true;
        }
    } catch (const Impl::SqlError &failure) {
        result.sqlite_code = failure.code;
        result.error = failure.what();
    } catch (...) {
        if (impl_->global_lookup) sqlite3_reset(impl_->global_lookup.get());
        throw;
    }
    if (impl_->global_lookup) sqlite3_reset(impl_->global_lookup.get());
    return result;
}
GlobalDefineCache RulesDatabase::cache_global_defines(GlobalDefineProfile profile,
                                                       const GlobalDefineCache *previous) const {
    const auto specs = global_define_specs(profile);
    if (previous && previous->profile() != profile)
        throw std::runtime_error("Cannot reuse global defines from another source profile");
    GlobalDefineCache result;
    result.profile_ = profile;
    result.entries_.reserve(specs.size());
    for (const auto &spec : specs) {
        CachedGlobalDefine entry{std::string(spec.name), spec.kind, global_define(spec.name, spec.kind)};
        if (!entry.lookup.found && profile == GlobalDefineProfile::vp542) {
            const auto *old = previous ? previous->find(spec.name) : nullptr;
            if (old) {
                entry.lookup.value = old->lookup.value;
                entry.retained_previous = true;
            } else if (spec.kind == GlobalDefineKind::integer) {
                entry.lookup.value = spec.initial_integer;
            } else {
                entry.lookup.value = spec.initial_real;
            }
        }
        if (spec.zero_is_none && std::get<std::int64_t>(entry.lookup.value) == 0)
            entry.lookup.value = std::int64_t{-1};
        result.names_.emplace(entry.name, result.entries_.size());
        result.entries_.push_back(std::move(entry));
    }
    return result;
}
std::uint64_t RulesDatabase::query(const DatabaseStatement &source, const RowVisitor &visitor) const {
    auto statement = impl_->single(source);
    if (!sqlite3_stmt_readonly(statement.get()))
        throw std::runtime_error("Query must be read-only");
    std::uint64_t rows = 0;
    int status;
    while ((status = impl_->step(statement.get())) == SQLITE_ROW) {
        std::vector<DatabaseValue> values;
        for (int col = 0; col < sqlite3_column_count(statement.get()); ++col) {
            switch (sqlite3_column_type(statement.get(), col)) {
            case SQLITE_NULL:
                values.emplace_back(std::monostate{});
                break;
            case SQLITE_INTEGER:
                values.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), col)));
                break;
            case SQLITE_FLOAT:
                values.emplace_back(sqlite3_column_double(statement.get(), col));
                break;
            case SQLITE_TEXT: {
                const auto *text = sqlite3_column_text(statement.get(), col);
                const auto bytes = sqlite3_column_bytes(statement.get(), col);
                if (!text)
                    throw std::runtime_error("SQLite text conversion failed");
                values.emplace_back(
                    std::string(reinterpret_cast<const char *>(text), static_cast<std::size_t>(bytes)));
                break;
            }
            default:
                throw std::runtime_error("BLOB query values are not yet exposed");
            }
        }
        visitor(values);
        ++rows;
    }
    if (status != SQLITE_DONE)
        impl_->check(status);
    return rows;
}
std::string RulesDatabase::sqlite_version() { return sqlite3_libversion(); }
std::string RulesDatabase::sqlite_source_id() { return sqlite3_sourceid(); }
} // namespace civ5::content
