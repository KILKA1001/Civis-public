#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace civ5::content {
// Borrowed current Database::Results row, valid only during the visitor call.
// Named columns are case-sensitive. Missing values use native getter defaults.
class RulesRowView {
  public:
    RulesRowView(const RulesRowView &) = delete;
    RulesRowView &operator=(const RulesRowView &) = delete;
    bool has_column(std::string_view name) const;
    std::int64_t integer(std::string_view name) const;
    double real(std::string_view name) const;
    std::optional<std::string> text(std::string_view name) const;
    // INTEGER: nonzero; TEXT: first byte t/T; FLOAT/BLOB/NULL/missing: false.
    // Integer conversion is widened from the source's GetInt to int64.
    bool boolean(std::string_view name) const;
    std::int64_t integer_at(int column) const;

  private:
    friend class RulesDatabase;
    explicit RulesRowView(void *statement);
    int column(std::string_view name) const;
    void *statement_;
    std::map<std::string, int, std::less<>> columns_;
};
struct InfoCollectionBudget {
    // Bounds work, never MAX(ID) or the collection's logical span.
    std::uint64_t rows = 16 * 1024 * 1024;
};
struct InfoCollectionReport {
    std::uint64_t rows = 0, slots = 0, holes = 0;
    std::uint64_t overlapping_ids = 0, cache_failures = 0;
    int sqlite_code = 0;
    std::string error;
    bool all_succeeded() const { return error.empty() && cache_failures == 0; }
};
} // namespace civ5::content
