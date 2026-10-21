#pragma once
#include "civ5/ui/bnw_alpha_progression_backend.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_research_queue_save_owner.hpp"
#include <filesystem>
#include <memory>
#include <map>
#include <string>

namespace civ5::tests {
// Measurement only. Root authenticates producing source/EXE/process/file pins;
// this text is never an owner capability or continuation authorization.
using BnwResearchRestartMeasurement=std::map<std::string,std::string>;
BnwResearchRestartMeasurement measure_actual_bnw_research_restart(
    const ui::SelectedBnwReadOnlyPreview&);
void write_new_bnw_research_measurement(const std::filesystem::path&,
    const BnwResearchRestartMeasurement&);
BnwResearchRestartMeasurement read_bnw_research_measurement(const std::filesystem::path&);

struct BnwResearchChoiceRestartReport {
    std::size_t atomic_writes=0,normal_loads=0,retained_sections=0;
    bool partial_research_preserved=false;
    bool full_continuation_proven=false,original_bnw_parity_proven=false;
};
// Test observation, never a gameplay issuer. Capture BEFORE the actual typed
// button so the real SaveOwner comparison revision remains the before revision.
class BnwResearchChoiceRestartFixture final {
  public:
    static std::unique_ptr<BnwResearchChoiceRestartFixture> capture_before_action(
        const ui::SelectedBnwReadOnlyPreview&,const std::shared_ptr<const ui::BnwClientFrame>& actual_before_frame,
        const ui::UiContextRegistry& actual_client_registry,const std::filesystem::path& actual_input,
        const std::filesystem::path& actual_game_root,const std::filesystem::path& new_output_directory);
    ~BnwResearchChoiceRestartFixture();
    BnwResearchChoiceRestartFixture(const BnwResearchChoiceRestartFixture&)=delete;
    BnwResearchChoiceRestartFixture& operator=(const BnwResearchChoiceRestartFixture&)=delete;
    // Call AFTER production renderer performed actual paint/present/completion.
    BnwResearchChoiceRestartReport verify_after_presented_choice(
        const ui::SelectedBnwReadOnlyPreview&,const ui::BnwAlphaProgressionBackend&,
        std::shared_ptr<const runtime::BnwResearchChoiceCommit>);
    // Separate genuine failed client fixture; no failure callback is injected.
    void verify_failed_source_refuses_export(const ui::SelectedBnwReadOnlyPreview&,
        const ui::BnwAlphaProgressionBackend&);
    // After a SECOND real typed choice and production presentation on the SAME
    // client, require the original SaveOwner/first receipt to refuse output.
    void verify_stale_owner_refuses_after_next_actual_choice(
        const ui::SelectedBnwReadOnlyPreview&,
        std::shared_ptr<const runtime::BnwResearchChoiceCommit> actual_next_choice);
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwResearchChoiceRestartFixture(std::unique_ptr<Impl>);
};
}
