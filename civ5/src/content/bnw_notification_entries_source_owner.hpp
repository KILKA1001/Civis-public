#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/content/info_types.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::content {
// The actual Notifications PrefetchCollection rows. ID selects storage slots;
// NotificationTypes publishes the signed32 bits of FString::Hash(type_name).
struct BnwNotificationEntry final {
    std::uint64_t slot;
    std::int32_t authored_id;
    std::string type_name;
    std::uint32_t hash;
    std::int32_t lua_value;
};
class BnwNotificationEntriesSourceOwner final {
  public:
    ~BnwNotificationEntriesSourceOwner() = default;
    BnwNotificationEntriesSourceOwner(const BnwNotificationEntriesSourceOwner&) = delete;
    BnwNotificationEntriesSourceOwner& operator=(const BnwNotificationEntriesSourceOwner&) = delete;
    void require_current(const RulesDatabase& database) const {
        if (creator_ != std::this_thread::get_id() || &database != database_.get())
            throw std::runtime_error("NotificationEntries SAME rules/creator changed FIRST");
        const auto host = host_.lock();
        if (!host || host.get() != host_get_ || host_.owner_before(host) || host.owner_before(host_) ||
            !current_ || !current_())
            throw std::runtime_error("NotificationEntries actual Host/rules lifetime retired");
    }
    std::uint64_t slot_count_owned(const RulesDatabase& database) const {
        require_current(database); return report_.slots;
    }
    const InfoCollectionReport& report_owned(const RulesDatabase& database) const {
        require_current(database); return report_;
    }
    const std::vector<BnwNotificationEntry>& rows_owned(const RulesDatabase& database) const {
        require_current(database); return rows_;
    }
    const BnwNotificationEntry* entry_at_owned(const RulesDatabase& database, std::uint64_t slot) const {
        require_current(database);
        if (slot >= report_.slots) return nullptr;
        const auto it = std::lower_bound(rows_.begin(), rows_.end(), slot,
            [](const BnwNotificationEntry& row, std::uint64_t value) { return row.slot < value; });
        return it != rows_.end() && it->slot == slot ? &*it : nullptr;
    }
  private:
    friend class civ5::ui::SelectedBnwPreviewHost;
    const std::shared_ptr<const RulesDatabase> database_;
    const std::thread::id creator_ = std::this_thread::get_id();
    const std::weak_ptr<const void> host_;
    const void* const host_get_;
    std::function<bool()> current_;
    std::vector<BnwNotificationEntry> rows_;
    InfoCollectionReport report_;
    BnwNotificationEntriesSourceOwner(std::shared_ptr<const RulesDatabase> database,
                                     const std::shared_ptr<const void>& actual_host,
                                     const std::function<void()>& current)
        : database_(std::move(database)), host_(actual_host), host_get_(actual_host.get()) {
        if (!database_ || !actual_host)
            throw std::invalid_argument("NotificationEntries genuine owned rules/Host absent");
        current_ = database_->immutable_preview_fingerprint_currentness();
        if (current) current();
        require_current(*database_);
        report_ = database_->prefetch_info("Notifications",
            [this, &current](std::uint64_t slot, std::int64_t id, const RulesRowView& row) {
                require_current(*database_);
                // The existing reader widens IDs. Refuse unsupported x86-wrap
                // inputs; accepted rows/counts preserve the native signed32 span.
                if (id < 0 || id > std::numeric_limits<std::int32_t>::max() ||
                    slot >= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                    throw std::runtime_error("NotificationEntries native signed32 ID/span unsupported");
                if (!row.has_column("ID") || !row.has_column("NotificationType"))
                    throw std::runtime_error("NotificationEntries actual source columns absent");
                // CvString::operator=(const char*) clears on NULL; GetText and
                // RulesRowView::text both copy only the first-NUL C-string prefix.
                auto text = row.text("NotificationType");
                std::string name = text ? std::move(*text) : std::string{};
                if (current) current();
                require_current(*database_);
                const auto hash = saved_info_type_hash(name);
                const auto signed_hash = hash <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
                    ? static_cast<std::int32_t>(hash)
                    : static_cast<std::int32_t>(static_cast<std::int64_t>(hash) - 0x100000000ll);
                rows_.push_back({slot, static_cast<std::int32_t>(id), std::move(name), hash, signed_hash});
                if (current) current();
                require_current(*database_);
                return true;
            });
        if (current) current();
        require_current(*database_);
        if (!report_.all_succeeded() || report_.rows != rows_.size())
            throw std::runtime_error("NotificationEntries actual SQL/cache producer did not return");
    }
};
} // namespace civ5::content
