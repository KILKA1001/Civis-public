#pragma once
#include "civ5/ui/frontend_content.hpp"
#include <memory>
#include <stdexcept>
#include <thread>

namespace civ5::ui {
// Menu/process adapter for an existing application executor. It neither creates
// a Bank nor launches another process. Graphics supplies the actual same-client
// owner after its Window/scene suppliers are connected. Read-only preview,
// metadata admission and process creation must never be reported as performed.
class FrontendProcessBackend final : public FrontendBackend {
    const FrontendCatalogContentAdapter &content_;
    std::shared_ptr<FrontendBackend> application_;
    const std::thread::id creator_ = std::this_thread::get_id();
    mutable bool callback_ = false;
    bool uncertain_ = false;

    struct Guard {
        bool &active;
        explicit Guard(bool &value) : active(value) { active = true; }
        ~Guard() { active = false; }
    };
    static bool network_or_store(std::string_view operation) {
        return operation == "start_multiplayer" || operation == "reconnect" ||
            operation == "open_workshop" || operation == "open_beyond_earth_store";
    }
    static bool launch(std::string_view operation) {
        return operation == "load_game" || operation == "start_game" ||
            operation == "quick_game" || operation == "start_scenario" ||
            operation == "start_tutorial" || operation == "load_custom_mod";
    }
    static UiCommandResult absent(std::string_view operation) {
        std::string reason = "This feature is not implemented yet";
        if (operation == "load_game") reason = "The party window and decoded-scene executor is not connected";
        else if (operation == "start_game" || operation == "quick_game")
            reason = "The new-game generator and party executor is not connected";
        else if (operation == "apply_options" || operation == "reset_options")
            reason = "The settings apply and Civis profile persistence owner is not connected";
        else if (operation == "apply_resolution") reason = "The window settings executor is not connected";
        else if (operation == "start_scenario") reason = "Starting a scenario is not connected yet";
        else if (operation == "start_tutorial") reason = "Starting a tutorial is not connected yet";
        else if (operation == "randomize_setup" || operation == "reset_setup" || operation == "add_ai")
            reason = "Changing game setup is not connected yet";
        return {UiCommandStatus::unavailable, std::move(reason)};
    }
    UiCommandResult context() const {
        if (std::this_thread::get_id() != creator_)
            return {UiCommandStatus::unavailable, "Frontend executor belongs to another thread"};
        if (callback_) return {UiCommandStatus::unavailable, "Frontend executor callback is already in progress"};
        if (should_close) return {UiCommandStatus::unavailable, "Frontend process is closing"};
        return {UiCommandStatus::ready, {}};
    }
public:
    explicit FrontendProcessBackend(const FrontendCatalogContentAdapter &content) : content_(content) {}
    bool should_close = false;

    // One concrete existing executor, owned through every callback. The factory
    // and actual Bank/Window/scene assembly are the graphics owner's seam.
    void connect_application(std::shared_ptr<FrontendBackend> application) {
        const auto state = context();
        if (state.status != UiCommandStatus::ready || uncertain_ || application_ ||
            !application || application.get() == this)
            throw std::logic_error("Frontend application connection is unavailable or already occupied");
        application_ = std::move(application);
    }
    UiCommandResult availability(const FrontendRequest &request) const override {
        const auto state = context();
        if (state.status != UiCommandStatus::ready) return state;
        if (request.operation == "exit_game") return {UiCommandStatus::ready, {}};
        if (network_or_store(request.operation))
            return {UiCommandStatus::unavailable, "Network and Steam services are not connected"};
        if (uncertain_)
            return {UiCommandStatus::unavailable, "The previous application action has an uncertain result; reopen the client"};
        if (launch(request.operation) && request.operation != "load_game" && request.save)
            return {UiCommandStatus::invalid, "A saved-game input cannot be used for a new-game operation"};
        if (request.operation == "load_game" && (!request.save || !request.draft.save ||
                request.save->id != *request.draft.save))
            return {UiCommandStatus::unavailable, "Select a saved game"};
        if (!application_) return absent(request.operation);
        const auto owner = application_;
        Guard guard(callback_);
        try {
            const auto result = owner->availability(request);
            if (result.status == UiCommandStatus::ready || result.status == UiCommandStatus::unavailable ||
                result.status == UiCommandStatus::invalid || result.status == UiCommandStatus::pending_refresh)
                return result;
            return {UiCommandStatus::unavailable, "Application capability query returned an effect status"};
        }
        catch (const std::exception &error) { return {UiCommandStatus::unavailable, error.what()}; }
    }
    FrontendContentCheck check_selection(const FrontendRequest &request) const override {
        const auto state = context();
        if (state.status != UiCommandStatus::ready) return {state, {}, false};
        return content_.check_selection(request);
    }
    FrontendContentCheck check_content(const FrontendRequest &request) const override {
        const auto state = context();
        if (state.status != UiCommandStatus::ready) return {state, {}, false};
        return content_.check_content(request);
    }
    UiCommandResult execute(const FrontendRequest &request) override {
        const auto ready = availability(request);
        if (ready.status != UiCommandStatus::ready) return ready;
        if (request.operation == "exit_game") {
            should_close = true;
            return {UiCommandStatus::performed, {}};
        }
        // Direct backend callers also pass admission. The application owner
        // still captures Source3 input and executes Source22 itself; this check
        // is neither input custody nor gameplay authority.
        if (launch(request.operation)) {
            const auto admitted = content_.check_content(request);
            if (admitted.result.status != UiCommandStatus::ready) return admitted.result;
            if (!admitted.requirements_known)
                return {UiCommandStatus::unavailable, "Content admission has no verified save requirements"};
        }
        const auto owner = application_;
        Guard guard(callback_);
        try {
            const auto result = owner->execute(request);
            if (result.status == UiCommandStatus::ready) {
                uncertain_ = true;
                return {UiCommandStatus::uncertain, "Application executor returned an invalid completion status"};
            }
            if (result.status == UiCommandStatus::uncertain) uncertain_ = true;
            return result;
        } catch (const std::exception &error) {
            // An effectful owner may throw after an irreversible transition.
            // Preserve the menu data, but never automatically repeat its call.
            uncertain_ = true;
            return {UiCommandStatus::uncertain, std::string("Application executor result is uncertain: ") + error.what()};
        } catch (...) {
            uncertain_ = true;
            return {UiCommandStatus::uncertain, "Application executor result is uncertain"};
        }
    }
};
} // namespace civ5::ui
