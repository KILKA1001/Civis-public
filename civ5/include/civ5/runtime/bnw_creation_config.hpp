#pragma once
#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::runtime {
class BnwSelectedLanguageInputOwner;
class BnwNewGameSetupRules;
// Requested menu choices are inputs, never a live-world or hook receipt.
struct BnwCreationChoices {
    std::string world, climate, sea_level, speed, era, handicap;
};
struct BnwCreationInfoBinding {
    std::string table, type;
    std::int32_t id = -1;
    std::uint32_t hash = 0;
};
struct BnwCreationSpeedAssignment {
    BnwCreationInfoBinding speed;
    std::optional<std::int32_t> timer_write;
};
struct BnwCreationMetadata {
    // Worlds, Climates, SeaLevels, GameSpeeds, Eras, HandicapInfos.
    std::array<BnwCreationInfoBinding, 6> info;
    std::string rules_sha256;
    // Exact reached setGameSpeed calls, in native sparse Info scan order.
    std::vector<BnwCreationSpeedAssignment> speed_assignments;
    // LAST reached timer WRITE; later speed calls without a write preserve it.
    // Neither descriptor executes a setter or reads a prior unknown timer.
    std::optional<std::int32_t> speed_timer_write;
};
enum class BnwCreationConfigError {
    parent_unavailable, wrong_thread, rules_not_immutable, profile_not_bnw,
    rules_retired, invalid_choice, unsupported_native_metadata
};
class BnwCreationConfigRefusal : public std::runtime_error {
 public:
    BnwCreationConfigRefusal(BnwCreationConfigError, const std::string &);
    BnwCreationConfigError code() const noexcept { return code_; }
 private:
    BnwCreationConfigError code_;
};
class BnwCreationConfig {
 public:
    // Metadata diagnostics retain supplied rules without issuing map/world or
    // genuine early-input authority. Existing isolated metadata callers stay valid.
    static std::shared_ptr<const BnwCreationConfig> from_selected_rules(
        std::shared_ptr<const BnwFreshGameContentResult>, BnwCreationChoices);
    // Strict early-input composition requires the genuine Content factory
    // control and private actual activation receipt before any input callback.
    static std::shared_ptr<const BnwCreationConfig> from_selected_rules(
        std::shared_ptr<const BnwFreshGameContentResult>,BnwCreationChoices,
        std::shared_ptr<const BnwSelectedLanguageInputOwner>);
    void require_current() const;
    BnwCreationMetadata metadata() const;
 private:
    friend class BnwFreshGame;
    friend class BnwNewGameSetupRules;
    friend class BnwFreshPregameSpeed;
    friend class BnwSelectedLocaleOwner;
    std::shared_ptr<const BnwFreshGameContentResult> borrow_activated_content_for_game_setup()const;
    std::string selected_speed_input() const;
    std::string selected_era_input() const;
    const content::RulesDatabase& game_rules() const;
    std::shared_ptr<const BnwSelectedLanguageInputOwner> borrow_selected_language_input()const;
    struct Impl;
    explicit BnwCreationConfig(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
