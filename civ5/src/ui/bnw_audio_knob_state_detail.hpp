#pragma once

#include "civ5/content/manifest.hpp"
#include "civ5/ui/bnw_audio_knob_direct_store.hpp"
#include <array>
#include <string>
#include <vector>

namespace civ5::ui::audio_settings_detail {
// Internal numeric state/model only. The future retained audio producer must
// supply its authenticated SoundTypes stream, settings and actual native window
// event owner. This type provides no source authority or live gain permission.
// Default-value construction: actual native85b030 builds a 28-byte stack
// record, then85b220/85b350 and reached fill helpers copy it by 28-byte stride.
// v48 proves float constant1 and field offsets; v49 proves the scalar copies.
// parent_index=-1 is this model's logical null, not the native zero pointer.
// These initial fields do not attest current MASTER or live source ownership.
struct KnobNumericFields {
    float current = 1.0f;             // Native record+0
    float target = 1.0f;              // +4
    float slope = 0.0f;               // +8
    std::int32_t transition_mode = 0; // +12
    float cached_effective = 1.0f;    // +16, not recomputed by direct setter
    std::int32_t parent_index = -1;   // Logical index, not a native pointer/ABI
    bool inherited_flag = false;     // Native byte+24
};
class KnobNumericGraph final {
    struct Entry { std::string name; KnobNumericFields fields; };
    std::vector<Entry> entries_;
    static void need(bool ok, const char* reason) {
        if (!ok) throw std::invalid_argument(reason);
    }
    static bool whitespace_only(std::string_view text) {
        for (const auto c : text) if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
        return true;
    }
    std::size_t index(std::string_view name) const {
        for (std::size_t i = 0; i < entries_.size(); ++i) if (entries_[i].name == name) return i;
        throw std::invalid_argument("BNW audio numeric graph unknown category");
    }
    void append(const content::XmlNode& row, std::int32_t parent, std::size_t depth) {
        // Synthetic MASTER occupies slot0;127 authored nodes occupy slots1..127.
        need(depth < 32 && entries_.size() < 128, "BNW audio numeric hierarchy budget exceeded");
        need(row.name == "SoundType" && whitespace_only(row.text) && row.attributes.size() == 1 &&
             row.attributes[0].first == "name" && !row.attributes[0].second.empty(),
             "BNW audio numeric SoundType subset unsupported");
        const auto& name = row.attributes[0].second;
        // The source uses symbol CRC IDs. This name-based model neither claims
        // those IDs nor admits duplicate names/unsupported spelling branches.
        for (const auto c : name) need((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_',
                                      "BNW audio numeric category spelling unsupported");
        for (const auto& entry : entries_) need(entry.name != name, "BNW audio duplicate category unsupported");
        const auto own = static_cast<std::int32_t>(entries_.size());
        entries_.push_back({name, {}}); entries_.back().fields.parent_index = parent;
        for (const auto& child : row.children) append(child, own, depth + 1);
    }
    float effective(std::size_t own) const {
        const auto& fields = entries_[own].fields;
        need(fields.transition_mode == 0, "BNW audio interpolated knob branch unsupported");
        float result = fields.current;
        if (fields.parent_index >= 0) {
            // Native85edb0 recursively rounds the parent's result, multiplies
            // by this current value, then stores binary32 at every level.
            const float parent = effective(static_cast<std::size_t>(fields.parent_index));
            volatile float rounded = parent * result; result = rounded;
        }
        if (result < 0.0f) result = 0.0f;
        if (result > 1.0f) result = 1.0f;
        return result;
    }
  public:
    explicit KnobNumericGraph(const content::XmlNode& sound_types) {
        need(sound_types.name == "SoundTypes" && whitespace_only(sound_types.text) && sound_types.attributes.empty(),
             "BNW audio numeric hierarchy requires SoundTypes");
        entries_.push_back({"MASTER", {}});
        for (const auto& row : sound_types.children) append(row, 0, 0);
    }
    std::size_t size() const noexcept { return entries_.size(); }
    KnobNumericFields fields(std::string_view name) const { return entries_[index(name)].fields; }
    std::string name(std::size_t i) const { return entries_.at(i).name; }
    void direct(std::string_view name, float incoming) {
        auto& value = entries_[index(name)].fields;
        apply_knob_direct_numeric(value.current, value.transition_mode, incoming);
    }
    // A complete four-setting transaction: source numeric checks precede
    // publication, and missing categories leave every original field intact.
    void load_four_saved_percentages(const std::array<std::int32_t, 4>& percentages) {
        constexpr std::array<std::string_view, 4> names{
            "USER_VOLUME_MUSIC", "USER_VOLUME_SFX", "USER_VOLUME_AMBIENCE", "USER_VOLUME_SPEECH"};
        std::array<std::size_t, 4> selected{}; std::array<float, 4> numeric{};
        for (std::size_t i = 0; i < names.size(); ++i) {
            selected[i] = index(names[i]); numeric[i] = knob_from_saved_volume_percent(percentages[i]);
        }
        for (std::size_t i = 0; i < names.size(); ++i) {
            auto& f = entries_[selected[i]].fields;
            apply_knob_direct_numeric(f.current, f.transition_mode, numeric[i]);
        }
    }
    float category_effective(std::string_view name) const {
        require_saved_volume_numeric_rounding(); return effective(index(name));
    }
};
} // namespace civ5::ui::audio_settings_detail
