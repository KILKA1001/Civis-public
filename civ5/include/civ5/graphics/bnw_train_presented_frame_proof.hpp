#pragma once
#include <functional>
#include <memory>
namespace civ5::validation { struct PreviewPresenter; }
namespace civ5::ui {class BnwTrainRendererRefreshReceipt;class BnwOwnedCameraGuiScreen;}
namespace civ5::graphics {
// Stack-only evidence issued by the actual camera paint/present owner AFTER
// successful native presentation and its real Scene/UI/viewport bookends.
class BnwTrainPresentedFrameProof final {
 public:
    BnwTrainPresentedFrameProof(const BnwTrainPresentedFrameProof&)=delete;
    BnwTrainPresentedFrameProof& operator=(const BnwTrainPresentedFrameProof&)=delete;
    BnwTrainPresentedFrameProof(BnwTrainPresentedFrameProof&&)=delete;
    BnwTrainPresentedFrameProof& operator=(BnwTrainPresentedFrameProof&&)=delete;
    void require_current_for_train(const ui::BnwTrainRendererRefreshReceipt&)const;
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> canonical_gui_for_train(
        const ui::BnwTrainRendererRefreshReceipt&)const;
 private:
    friend struct ::civ5::validation::PreviewPresenter;
    struct State;State& state_;
    explicit BnwTrainPresentedFrameProof(State& value):state_(value){}
    static void with_actual_presented_frame(
        const std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt>&,
        const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>&,
        const std::function<void(const BnwTrainPresentedFrameProof&)>&);
};
}
