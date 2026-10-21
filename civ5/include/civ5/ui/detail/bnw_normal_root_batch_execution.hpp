#pragma once
#include <memory>
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
#include "civ5/ui/detail/bnw_windowed_renderer_matrix_data_loan.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
// Original enclosing Root execution. Its closed producer executes one real
// Matrix and original Prefix before Common; the original Prefix facade remains
// alive until genuine manager consumption ends inside the SAME Ui Root scope.
class BnwNormalRootBatchExecution final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalRootBatchExecution(std::shared_ptr<Impl>,const IssuerKey&);
    BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
    std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> actual_matrix_owned_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::array<std::uint64_t,4> completed_queue_observations_without_callbacks()const;
public:
    ~BnwNormalRootBatchExecution(); // Original Prefix retirement FIRST.
    BnwNormalRootBatchExecution(const BnwNormalRootBatchExecution&)=delete;
    BnwNormalRootBatchExecution&operator=(const BnwNormalRootBatchExecution&)=delete;
};
}
