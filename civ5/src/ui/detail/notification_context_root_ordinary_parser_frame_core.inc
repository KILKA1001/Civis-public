// Inside civ5::ui after actual Core, Gui, Runtime::State and CommonChildParserReturn.
// The file-root parent is a real RootRead loan; no id0 Runtime is synthesized.
class BnwWorldViewControlTree::RootOrdinaryParserFrame final {
    friend class BnwWorldViewControlTree;
    friend class CommonChildParserReturn;
    friend class detail::NotificationLayoutRuntimeDimensions;
    friend class detail::NotificationContextRootOrdinaryFactoryReturn;
    using Runtime=detail::NotificationLayoutRuntimeDimensions;
    using Root=detail::NotificationContextRootReadChildrenLoan;
    using Styles=detail::NotificationContextGuiPort::Impl::SelectedStyleRegistry;
    struct ClosedReturn final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> frame_ended{false};
        const std::shared_ptr<Runtime::State> runtime;
        const std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence;
        const std::shared_ptr<const Core::FullXmlOccurrence> xml;
        const std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> common;
        ClosedReturn(std::shared_ptr<Runtime::State> r,
            std::shared_ptr<const Core::LayoutReceiverOccurrence> o,
            std::shared_ptr<const Core::FullXmlOccurrence> x,
            std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> c)
            :runtime(std::move(r)),occurrence(std::move(o)),xml(std::move(x)),common(std::move(c)){}
    };
    const std::thread::id creator_=std::this_thread::get_id();
    bool alive_=true,closed_=false;
    const std::shared_ptr<Core> core_;
    const std::shared_ptr<const Root> root_;
    const Root::Guard root_guard_;
    const std::shared_ptr<const BnwWorldViewControlTree> tree_owner_;
    const BnwWorldViewControlTree*const tree_;
    const runtime::BnwWorldViewSourceOperationLoan*const operation_;
    const std::uint64_t child_id_;
    const std::shared_ptr<const Core::FullXmlOccurrence> xml_;
    const std::shared_ptr<Styles> styles_;
    const std::map<std::string,content::XmlNode,std::less<>> parser_styles_;
    std::weak_ptr<Runtime::State> child_;
    std::shared_ptr<ClosedReturn> returned_;
    static auto merge_styles(const std::shared_ptr<Styles>& styles,const Root::Guard& read){
        read.require_current_without_callbacks();styles->first();
        std::map<std::string,content::XmlNode,std::less<>> result;
        // The same selected Base, Exp1, Exp2 providers as lookup_global.
        for(const auto& provider:styles->providers)
            for(const auto& [name,value]:provider){
                read.require_current_without_callbacks();styles->first();
                result.insert_or_assign(name,value);
                read.require_current_without_callbacks();styles->first();
            }
        return result;
    }
    RootOrdinaryParserFrame(std::shared_ptr<Core> c,std::shared_ptr<const Root> root,Root::Guard guard,
        std::shared_ptr<const BnwWorldViewControlTree> tree,const runtime::BnwWorldViewSourceOperationLoan* op,
        std::uint64_t id,std::shared_ptr<const Core::FullXmlOccurrence> xml,std::shared_ptr<Styles> styles)
        :core_(std::move(c)),root_(std::move(root)),root_guard_(std::move(guard)),tree_owner_(std::move(tree)),tree_(tree_owner_.get()),
         operation_(op),child_id_(id),xml_(std::move(xml)),styles_(std::move(styles)),parser_styles_(merge_styles(styles_,root_guard_)){}
    void require_root_without_callbacks()const {
        require(creator_==std::this_thread::get_id()&&alive_&&!closed_,
            "Root ordinary frame retired/closed/foreign FIRST");
        root_guard_.require_current_without_callbacks();
        operation_->require_origin_without_callbacks();core_->layout_origin(true);
        require(core_->layout_parser_root_child_frame==this&&xml_&&styles_&&
            core_->context_gui_port&&core_->context_gui_port->impl_->selected_styles==styles_&&
            Core::node(core_->nodes,child_id_).full_xml_occurrence==xml_&&
            Core::same_layout_xml(xml_->value,root_guard_.actual_declaration_without_callbacks()),
            "Root ordinary SAME lexical frame/XML/selected Styles changed");
        styles_->first();root_guard_.require_current_without_callbacks();
    }
    void require_source_owned()const {
        require_root_without_callbacks();root_guard_.require_source_owned(*operation_);require_root_without_callbacks();
        const auto resources=operation_->selected_style_resources_owned();require_root_without_callbacks();
        require(resources.size()==styles_->sheets.size(),"Root ordinary selected Styles count changed");
        for(std::size_t i=0;i<resources.size();++i){
            const auto& now=resources[i];const auto& old=styles_->sheets[i];
            require(now.payload_identity.sha256==old.payload_identity.sha256&&now.bytes==old.bytes&&now.xml==old.xml,
                "Root ordinary selected Bank/VFS/DLC Styles custody changed");
        }
        require_root_without_callbacks();
    }
    void bind_child_runtime_owned(const std::shared_ptr<Runtime::State>& child){
        require_root_without_callbacks();Runtime::require_storage_without_callbacks(child);
        require(child_.expired()&&child->id==child_id_&&child->tree_facade==tree_&&child->operation==operation_,
            "Root ordinary parser bound an unrelated/replayed Runtime");
        child_=child;require_root_without_callbacks();Runtime::require_storage_without_callbacks(child);
    }
    void require_return_without_callbacks(const CommonChildParserReturn& value)const {
        require_root_without_callbacks();
        const auto captured=child_.lock();const auto& node=Core::node(core_->nodes,child_id_);
        require(value.creator_==std::this_thread::get_id()&&!value.consumed_&&!value.frame_&&value.root_frame_==this&&
            captured&&captured==value.child_&&!captured->alive.load()&&!captured->failed.load()&&
            captured->id==child_id_&&captured->operation==operation_&&captured->tree_facade==tree_&&
            !core_->layout_parser_child_frame&&!core_->layout_parser_control_id&&!core_->layout_parser_borrower&&
            !core_->layout_parser_facade&&!core_->layout_parser_occurrence&&node.layout_baseline&&
            captured->baseline==&*node.layout_baseline&&node.layout_receiver_occurrence==value.occurrence_&&
            node.full_xml_occurrence==value.xml_&&value.xml_==xml_&&value.native_&&
            node.layout_baseline->base_.common_return_owned==value.native_&&
            !node.layout_baseline->parser_runtime_dimensions_&&value.native_->impl_,
            "Root ordinary return needs SAME native common result and actual Runtime/Binding DTOR");
        const auto native=value.native_->impl_;const auto& base=node.layout_baseline->base_;
        require(native->creator==std::this_thread::get_id()&&native->core.lock()==core_&&
            native->occurrence.lock()==value.occurrence_&&native->selected.lock()==xml_&&
            native->receiver.lock()==base.receiver_owned&&native->children.lock()==base.children_78&&
            base.receiver_owned&&base.children_78&&base.children_78->impl_&&
            native->serial==operation_->operation_serial()&&native->count==xml_->value.children.size()&&
            base.children_78->impl_->ordered.size()==native->count&&(base.flags_40&8u),
            "Root ordinary actual common suffix/receiver/ordered78/OR8 changed");
    }
    std::shared_ptr<ClosedReturn> close_after_return_owned(CommonChildParserReturn& value){
        try{
            require_return_without_callbacks(value);require_source_owned();require_return_without_callbacks(value);
            auto closed=std::make_shared<ClosedReturn>(value.child_,value.occurrence_,value.xml_,value.native_);
            require_source_owned();require_return_without_callbacks(value);
            require(!returned_,"Root ordinary parser completion replay");returned_=closed;
            value.consumed_=true;alive_=false;closed_=true;
            core_->layout_parser_root_child_frame=nullptr;
            root_guard_.require_source_owned(*operation_);root_guard_.require_current_without_callbacks();
            return closed;
        }catch(...){core_->failed.store(true);if(const auto child=child_.lock())Runtime::fail_storage_without_callbacks(child);throw;}
    }
public:
    ~RootOrdinaryParserFrame()noexcept{
        if(closed_){if(returned_)returned_->frame_ended.store(true);return;}
        alive_=false;core_->failed.store(true);
        if(const auto child=child_.lock())Runtime::fail_storage_without_callbacks(child);
        if(creator_==std::this_thread::get_id()&&core_->layout_parser_root_child_frame==this)
            core_->layout_parser_root_child_frame=nullptr;
    }
};

BnwWorldViewControlTree::CommonChildParserReturn::CommonChildParserReturn(
    const RootOrdinaryParserFrame* frame,std::shared_ptr<detail::NotificationLayoutRuntimeDimensions::State> child,
    std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence,std::shared_ptr<const Core::FullXmlOccurrence> xml)
    :frame_(nullptr),root_frame_(frame),child_(std::move(child)),occurrence_(std::move(occurrence)),xml_(std::move(xml)){
    require(root_frame_&&child_&&occurrence_&&xml_,"Root ordinary returning branch custody absent");
    root_frame_->require_root_without_callbacks();
    detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(child_);
    native_=child_->baseline->base_.common_return_owned;
    require(root_frame_->child_.lock()==child_&&child_->id==root_frame_->child_id_&&
        child_->operation==root_frame_->operation_&&child_->tree_facade==root_frame_->tree_&&
        !child_->baseline->parser_runtime_dimensions_&&native_&&native_->impl_&&
        native_->impl_->serial==child_->serial&&xml_==root_frame_->xml_,
        "Root ordinary returning branch cannot be synthesized from cold layout fields");
}

void BnwWorldViewControlTree::require_ordinary_root_read_origin_without_callbacks(
    const std::shared_ptr<Core>& core){
    require(core!=nullptr,"Ordinary Root read origin Core retired");
    if(core->layout_parser_root_child_frame)core->layout_parser_root_child_frame->require_root_without_callbacks();
}
