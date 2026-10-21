#pragma once
#include "civ5/content/manifest.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <string>

namespace civ5::ui::detail {
// Private subtype component of the SAME InitialGraph; no second context tree,
// font registry, parser, native initializer or caller-supplied capability bit.
struct BnwRootContextFieldBinding {
    std::shared_ptr<const content::XmlNode> document;
    std::string source_path, xml_sha256;
    std::function<bool()> current;
};
// Source registration event, not native factory/BuildInstance execution.
struct BnwRootInstanceMetadataEntry {
    std::size_t source_ordinal = 0;
    std::string name;
    std::uint32_t hash = 0;
    // Alias into SAME whole retained selected XML, never a detached template.
    std::shared_ptr<const content::XmlNode> first_child;
    bool skipped_no_first_child = false;
};
struct BnwRootContextStoredFields {
    // 40F9C0: source-owned +538=null/+534=empty. There is no font setter here.
    std::shared_ptr<const void> authored_font;
    std::string style;
    // 540EB0 initializes three RGBA values and associated metadata. The
    // trailing structure padding byte is NOT an initialized fourth color.
    std::array<std::array<float,4>,3> colors{{{1,1,1,1},{1,1,1,1},{1,1,1,1}}};
    std::array<std::uint32_t,3> color_words{};
    std::array<std::uint8_t,3> color_flags{};
    bool empty_declaration_read_modeled = false;
    bool instance_metadata_prefix_modeled = false;
    std::size_t next_source_cursor = 0, instance_template_count = 0;
    std::vector<BnwRootInstanceMetadataEntry> instance_metadata_journal;
    static constexpr bool native_build_instance_executed = false;
    static constexpr bool color_padding_known = false;
    static constexpr bool full_native_constructor_complete = false;
    static constexpr bool native_read_executed = false, native_reset_executed = false;
    static constexpr bool child_read_context_available = false;
    static constexpr bool font_registry_available = false;
    static constexpr bool name_registration_complete = false, parent_append_complete = false;
    static constexpr bool native_initialized = false, native_label_ready = false;
};
struct BnwRootContextFieldIdentity {
    // Real local component occurrence; separate from the panel token and XML
    // record identity. Retaining this token never retains mutable authority.
    std::shared_ptr<const void> instance;
    std::shared_ptr<const content::XmlNode> document;
    std::string source_path, xml_sha256;
};
class BnwNativeRootContextFields;
class BnwRootContextInstanceTemplateQuote;
class BnwRootContextFieldSnapshot {
 public:
    const BnwRootContextStoredFields& fields() const;
    const BnwRootContextFieldIdentity& identity() const;
    std::uint64_t revision() const;
    bool current() const noexcept;
    // Checked local map lookup; does NOT execute703B00/native BuildInstance.
    std::optional<BnwRootContextInstanceTemplateQuote> instance_template(std::string_view) const;
 private:
    friend class BnwNativeRootContextFields;
    struct Data;
    explicit BnwRootContextFieldSnapshot(std::shared_ptr<const Data>);
    std::shared_ptr<const Data> data_;
};
class BnwRootContextInstanceTemplateQuote {
 public:
    const BnwRootContextFieldSnapshot& authority() const { return authority_; }
    const BnwRootInstanceMetadataEntry& entry() const { return entry_; }
    bool current() const noexcept { return authority_.current(); }
    static constexpr bool native_build_instance_executed = false;
    static constexpr bool child_read_context_available = false, native_label_ready = false;
 private:
    friend class BnwRootContextFieldSnapshot;
    BnwRootContextInstanceTemplateQuote(BnwRootContextFieldSnapshot authority,
                                       BnwRootInstanceMetadataEntry entry)
        : authority_(std::move(authority)), entry_(std::move(entry)) {}
    BnwRootContextFieldSnapshot authority_;
    BnwRootInstanceMetadataEntry entry_;
};
class BnwNativeRootContextFields {
 public:
    explicit BnwNativeRootContextFields(BnwRootContextFieldBinding);
    ~BnwNativeRootContextFields();
    BnwNativeRootContextFields(const BnwNativeRootContextFields&) = delete;
    BnwNativeRootContextFields& operator=(const BnwNativeRootContextFields&) = delete;
    BnwRootContextFieldSnapshot snapshot() const;
    // Model only the proven absent declaration branches of5D2920. Root attrs
    // and direct ColorSet must be absent. This does not execute native helpers.
    void read_empty_declaration();
    //7A9F63..7AA044: only leading canonical Instance declarations.
    // No Name/conversion means ordinary-factory fallback: refuse here.
    // No first child means source skip and an explicit skipped journal event.
    // Stop BEFORE first ordinary child/Styles (unmodeled source branches).
    void register_instance_metadata_prefix();
    // Civis lifecycle retirement, not native reset or constructor replay.
    void retire_for_graph_reset();
    void close() noexcept;
 private:
    friend class BnwRootContextFieldSnapshot;
    struct Cell;
    std::shared_ptr<Cell> cell_;
};
}
