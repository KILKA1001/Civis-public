#pragma once
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>

namespace civ5::ui::detail {
// Local authorization only. Shared by SAME graph forks; not native context,
// initialized state, source identity, or a mutation/publication receipt.
class BnwNativeLabelXmlPrefixGuard {
    struct State {
        const std::thread::id thread = std::this_thread::get_id();
        std::atomic<bool> busy{false}, failed{false};
    };
    std::shared_ptr<State> state_ = std::make_shared<State>();
 public:
    class Scope {
        friend class BnwNativeLabelXmlPrefixGuard;
        std::shared_ptr<State> state_;
        explicit Scope(std::shared_ptr<State> state) : state_(std::move(state)) {
            // Foreign access must not read or poison same-thread authorization.
            if (std::this_thread::get_id() != state_->thread)
                throw std::runtime_error("Native Label XML prefix foreign thread");
            if (state_->failed.load())
                throw std::runtime_error("Native Label XML prefix retired authorization");
            if (state_->busy.exchange(true)) {
                state_->failed.store(true);
                throw std::runtime_error("Native Label XML prefix reentry");
            }
        }
     public:
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;
        ~Scope() {
            if (std::this_thread::get_id() != state_->thread)
                state_->failed.store(true);
            state_->busy.store(false);
        }
        void check() const {
            if (std::this_thread::get_id() != state_->thread ||
                state_->failed.load() || !state_->busy.load())
                throw std::runtime_error("Native Label XML prefix lost authorization");
        }
    };
    Scope enter() const { return Scope(state_); }
    bool healthy() const noexcept {
        return std::this_thread::get_id() == state_->thread &&
               !state_->failed.load();
    }
};
}
