#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutRuntimeDimensions;
class BnwLabelProcessModuleOwner;
class BnwLabelCurrentTextLoan;
class BnwStandalonePositionedLabelRunLoan;
// A distinct actual selected-node allocation. It is NEVER Button178 or a tag
// converted to a Label receipt. The native common/parser/initializer suffix is
// mandatory and remains closed until its real caller returns.
class NotificationLayoutStandaloneLabelState final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class BnwLabelProcessModuleOwner;
    friend class BnwLabelCurrentTextLoan;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class BnwNormalLabelPaintInvocation;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct ConstructionLoan;
    struct ConstructorReturn;
    struct ParserLoan;
    struct FieldsReturn;
    struct WrapperReturn;
    struct InitializationLoan;
    struct InitializerFieldsReturn;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutStandaloneLabelState(std::shared_ptr<Impl>);
    static std::shared_ptr<NotificationLayoutStandaloneLabelState> allocate_pending_owned(
        const std::shared_ptr<ConstructionLoan>&);
    void construct_literal_owned(const std::shared_ptr<ConstructionLoan>&);
    void initialize_slot10_prefix_owned(const std::shared_ptr<InitializationLoan>&);
    void initialize_slot10_body_owned(const std::shared_ptr<InitializationLoan>&);
    void initialize_common_5761_owned(const std::shared_ptr<InitializationLoan>&)const;
    void anchor_slot4c_owned(const std::shared_ptr<InitializationLoan>&)const;
    void bounds_slot94_owned(const std::shared_ptr<InitializationLoan>&)const;
    void generate_text_segments_5a_owned(const std::shared_ptr<InitializationLoan>&)const;
    void position_plain_segments_7505_owned(const std::shared_ptr<InitializationLoan>&)const;
    void calculate_plain_size_5077_owned(const std::shared_ptr<InitializationLoan>&)const;
    std::unique_ptr<BnwLabelCurrentTextLoan> current_text_owned(const std::shared_ptr<InitializationLoan>&,bool)const;
    std::shared_ptr<BnwStandalonePositionedLabelRunLoan> pending_positioned_run_owned(const std::shared_ptr<InitializationLoan>&);
    void parse_fields_5e_owned(const NotificationLayoutRuntimeDimensions&,const std::shared_ptr<ParserLoan>&);
public:
    ~NotificationLayoutStandaloneLabelState();
    NotificationLayoutStandaloneLabelState(const NotificationLayoutStandaloneLabelState&)=delete;
    NotificationLayoutStandaloneLabelState& operator=(const NotificationLayoutStandaloneLabelState&)=delete;
};
}
