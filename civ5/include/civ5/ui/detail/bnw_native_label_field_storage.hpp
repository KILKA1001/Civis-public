#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace civ5::ui::detail {
// A subtype field component for the SAME retained graph; never a graph, font
// resolver, native initializer, or renderer. Bindings must be supplied by that
// graph's actual source boundaries. Test bindings remain declared fixtures.
struct BnwLabelFieldBinding {
    std::shared_ptr<const void> graph, control, parent;
    std::size_t control_index = 0;
    std::string source_path;
    std::function<bool()> current;
};
struct BnwLabelFontFieldBinding {
    std::shared_ptr<const void> authored;
    std::string source_sha256;
    std::function<bool()> current;
    // SAME authored font style record +40/+44/+48. Missing supplier is pending;
    // no layer is enabled by a name, alpha, or a guessed default index.
    std::function<std::optional<std::array<std::int32_t,3>>(std::uint8_t)> layers;
};
struct BnwLabelExtendedFieldBinding {
    std::shared_ptr<const void> owner;
    std::function<bool()> current;
};
struct BnwLabelColorFieldBinding {
    std::array<std::array<float,4>,3> values{};
    std::shared_ptr<const void> owner;
    std::function<bool()> current;
};
struct BnwLabelStoredFields {
    // Source constructor 764170 ->540EB0: 3 float4 ones. These are constructor
    // fields, not a selected context/global Color receipt.
    std::array<std::array<float,4>,3> colors{{{1,1,1,1},{1,1,1,1},{1,1,1,1}}};
    std::optional<std::string> text;
    std::optional<BnwLabelFontFieldBinding> font;
    std::optional<BnwLabelExtendedFieldBinding> extended;
    std::optional<std::array<std::int32_t,3>> layer_indices;
    std::uint8_t style = 0;
    std::int16_t wrap = -1, leading = 0, truncate = -1;
    bool color_override = false;
    bool force_ime = false, force_non_ime = false;
    // Modeled02000000 at5E3AC4; supplied context write is still unavailable.
    bool subtype_read_entered = false;
    // This field component owns no initialized receiver or parent completion.
    // There is deliberately no setter that can make these true.
    static constexpr bool native_initialized = false;
    static constexpr bool parent_callbacks_complete = false;
    static constexpr bool native_label_ready = false;
};
struct BnwLabelFieldIdentity {
    std::shared_ptr<const void> graph, control, parent;
    std::size_t control_index = 0;
    std::string source_path;
};
class BnwNativeLabelFieldStorage;
class BnwLabelFieldSnapshot {
 public:
    const BnwLabelStoredFields& fields() const;
    const BnwLabelFieldIdentity& identity() const;
    std::uint64_t revision() const;
    bool current() const noexcept;
 private:
    friend class BnwNativeLabelFieldStorage;
    struct Data;
    explicit BnwLabelFieldSnapshot(std::shared_ptr<const Data>);
    std::shared_ptr<const Data> data_;
};
enum class BnwLabelFieldOperation { set_text, set_font, set_style, set_colors,
                                  xml_wrap, xml_truncate, xml_leading,
                                  xml_force_ime, xml_force_non_ime, xml_read_entry, reset };
struct BnwLabelFieldTransition {
    BnwLabelFieldOperation operation;
    std::uint64_t revision;
    // Modeled source invocation only: 6A2EA0's initialized guard immediately
    // skips work. No original VM/computation or parent callback is executed.
    bool text_update_called = false;
    bool text_update_skipped_uninitialized = false;
    bool native_vm_executed = false;
};
class BnwNativeLabelFieldStorage {
 public:
    explicit BnwNativeLabelFieldStorage(BnwLabelFieldBinding);
    ~BnwNativeLabelFieldStorage();
    BnwNativeLabelFieldStorage(const BnwNativeLabelFieldStorage&) = delete;
    BnwNativeLabelFieldStorage& operator=(const BnwNativeLabelFieldStorage&) = delete;
    BnwLabelFieldSnapshot snapshot() const;
    BnwLabelFieldTransition set_text(std::optional<std::string> owned_text);
    using ExtendedSelector = std::function<std::optional<BnwLabelExtendedFieldBinding>(
                                          const BnwLabelFontFieldBinding&,bool force_ime,bool force_non_ime)>;
    BnwLabelFieldTransition set_font(std::optional<BnwLabelFontFieldBinding>, ExtendedSelector);
    BnwLabelFieldTransition set_style(std::uint8_t);
    BnwLabelFieldTransition set_colors(BnwLabelColorFieldBinding);
    BnwLabelFieldTransition xml_wrap(std::int16_t);
    BnwLabelFieldTransition xml_truncate(std::int16_t);
    BnwLabelFieldTransition xml_leading(std::int16_t);
    BnwLabelFieldTransition xml_force_ime(bool parsed_true);
    BnwLabelFieldTransition xml_force_non_ime(bool parsed_true);
    BnwLabelFieldTransition xml_read_entry();
    BnwLabelFieldTransition reset();
    void close() noexcept;
 private:
    friend class BnwLabelFieldSnapshot;
    struct Cell;
    std::shared_ptr<Cell> cell_;
    BnwLabelFieldTransition mutate(BnwLabelFieldOperation,bool,
                                   std::function<void(Cell&)>);
};
}
