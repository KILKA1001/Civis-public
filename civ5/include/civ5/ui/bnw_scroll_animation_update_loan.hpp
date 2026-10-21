#pragma once
#include "civ5/ui/bnw_tech_award_animation.hpp"
#include <atomic>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwWorldViewNativeContextIssuer;class BnwSpClientUpdateLoan;}
namespace civ5::ui {
class UiScreenGlRenderer;class BnwWorldViewControlTree;
// Issued only at the genuine Camera while-loop update boundary, after its
// message batch. A cold preparation sample cannot construct this carrier.
class BnwScrollAnimationLoopTick final {
    friend class UiScreenGlRenderer;
    friend class BnwScrollAnimationUpdateLoan;
    struct State {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true},entered{false};
        mutable std::atomic<bool> failed{false};
        bool consumed=false;
        const std::weak_ptr<const BnwTechAwardAnimationSample> sample;
        const std::uint64_t sequence;
        State(const std::shared_ptr<const BnwTechAwardAnimationSample>&,std::uint64_t);
        std::shared_ptr<const BnwTechAwardAnimationSample> current()const;
        void origin()const;
    };
    const std::shared_ptr<State> state_;
    explicit BnwScrollAnimationLoopTick(const std::shared_ptr<const BnwTechAwardAnimationSample>&);
public:
    ~BnwScrollAnimationLoopTick()noexcept;
    BnwScrollAnimationLoopTick(const BnwScrollAnimationLoopTick&)=delete;
    BnwScrollAnimationLoopTick&operator=(const BnwScrollAnimationLoopTick&)=delete;
    BnwScrollAnimationLoopTick(BnwScrollAnimationLoopTick&&)=delete;
    BnwScrollAnimationLoopTick&operator=(BnwScrollAnimationLoopTick&&)=delete;
};
// Borrowed only inside ACTIVE counted AnimationSources. It retains State,
// not renderer/Scene/UI/Window facades or the original loop carrier.
class BnwScrollAnimationUpdateLoan final {
    friend class UiScreenGlRenderer;
    friend class BnwWorldViewControlTree;
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::runtime::BnwSpClientUpdateLoan;
    const std::shared_ptr<BnwScrollAnimationLoopTick::State> state_;
    const std::thread::id creator_=std::this_thread::get_id();
    std::atomic<bool> alive_{true};
    explicit BnwScrollAnimationUpdateLoan(const std::shared_ptr<BnwScrollAnimationLoopTick::State>&);
    ~BnwScrollAnimationUpdateLoan()noexcept;
    void require_current_without_callbacks()const;
    std::uint64_t tick_sequence_without_callbacks()const;
    float delta_seconds_without_callbacks()const;
    std::weak_ptr<BnwClientSceneFrame> actual_scene_without_callbacks()const;
    BnwScrollAnimationUpdateLoan(const BnwScrollAnimationUpdateLoan&)=delete;
    BnwScrollAnimationUpdateLoan&operator=(const BnwScrollAnimationUpdateLoan&)=delete;
    BnwScrollAnimationUpdateLoan(BnwScrollAnimationUpdateLoan&&)=delete;
    BnwScrollAnimationUpdateLoan&operator=(BnwScrollAnimationUpdateLoan&&)=delete;
};
}
