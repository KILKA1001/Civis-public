#pragma once
#include <cstddef>
#include <memory>
#include "civ5/ui/detail/bnw_positioned_label_run_loan.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutRuntimeDimensions;
class BnwCompletedFontLoan;
class BnwLabelCurrentTextLoan;
class BnwLabelSystemFontBorrower;
class NotificationLayoutButtonState;
class NotificationLayoutEmbeddedLabelFactoryReturn;
class NotificationLayoutEmbeddedLabelStringLayerLoan;
// A distinct actual Button-owned child allocation, not a Tree ControlID, copied
// outer baseline or detached field DTO. The actual caller/allocator/Realm
// bridges remain mandatory: this leaf grants no703120 or5E3A90 full return.
class NotificationLayoutEmbeddedLabelState final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class BnwLabelCurrentTextLoan;
    friend class BnwPositionedLabelRunLoan;
    friend class NotificationLayoutButtonState;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct ConstructorLoan;
    struct InitializationLoan;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutEmbeddedLabelState(std::shared_ptr<Impl>);
    static std::shared_ptr<NotificationLayoutEmbeddedLabelState> allocate_pending_owned(
        const std::shared_ptr<ConstructorLoan>&);
    void construct_literal_prefix_owned(const std::shared_ptr<ConstructorLoan>&);
    void require_ownership_origin_owned(const std::shared_ptr<NotificationLayoutButtonState>&,
        const runtime::BnwWorldViewSourceOperationLoan&,std::size_t actual_parser_layer)const;
    void enter_first_font_setter_owned(const NotificationLayoutRuntimeDimensions&,
        const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>&,
        const std::shared_ptr<const BnwCompletedFontLoan>&,std::size_t)const;
    void parse_post_font_attributes_owned(const NotificationLayoutRuntimeDimensions&,
        const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>&,std::size_t)const;
    void initialize_from_parent_owned(const std::shared_ptr<InitializationLoan>&)const;
    void initialize_common_5761_owned(const std::shared_ptr<InitializationLoan>&)const;
    void require_initialization_font_current_owned(const std::shared_ptr<InitializationLoan>&)const;
    void require_initialization_font_source_owned(const std::shared_ptr<InitializationLoan>&)const;
    std::unique_ptr<BnwLabelCurrentTextLoan> current_position_text_owned(const std::shared_ptr<InitializationLoan>&)const;
    std::unique_ptr<BnwLabelCurrentTextLoan> current_size_text_owned(const std::shared_ptr<InitializationLoan>&)const;
    void generate_text_segments_5a_owned(const std::shared_ptr<InitializationLoan>&)const;
    void position_plain_segments_7505_owned(const std::shared_ptr<InitializationLoan>&)const;
    void calculate_plain_size_5077_owned(const std::shared_ptr<InitializationLoan>&)const;
    void anchor_initialized_4c_owned(const std::shared_ptr<InitializationLoan>&)const;
    void bounds_initialized_94_owned(const std::shared_ptr<InitializationLoan>&)const;
    void rebuild_initialized_layout_owned(const std::shared_ptr<InitializationLoan>&)const;
    std::shared_ptr<BnwPositionedLabelRunLoan> make_positioned_run_pending_owned(const std::shared_ptr<InitializationLoan>&)const;
    void retire_construction_authority_owned()noexcept;
    // No callable read3C writer until original parserContextRealm is lent.
    // GuiPort and Source context-name can never substitute for that operand.
    // Full constructor return is only an opaque FactoryReturn, never a class-wide readiness boolean.
    static constexpr bool subtype_parser_returned=false;
    static constexpr bool native_initialized=false;
public:
    ~NotificationLayoutEmbeddedLabelState();
    NotificationLayoutEmbeddedLabelState(const NotificationLayoutEmbeddedLabelState&)=delete;
    NotificationLayoutEmbeddedLabelState& operator=(const NotificationLayoutEmbeddedLabelState&)=delete;
};
}
