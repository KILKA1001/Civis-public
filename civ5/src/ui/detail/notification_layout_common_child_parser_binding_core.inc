// Inside civ5::ui AFTER actual Core/Gui/Runtime::State. Binding closure only.
// This leaf deliberately has NO native529520/v44-return issuer and grants no
// virtual38 attachment, array78, OR8, flat-loop skip, parser/init or frame readiness.
class BnwWorldViewControlTree::CommonChildParserReturn final {
    friend class BnwWorldViewControlTree;
    friend class CommonChildParserFrame;
    friend class RootOrdinaryParserFrame;
    friend class detail::NotificationLayoutRuntimeDimensions;
    const std::thread::id creator_=std::this_thread::get_id();
    bool consumed_=false;
    const CommonChildParserFrame* const frame_;
    const RootOrdinaryParserFrame* const root_frame_=nullptr;
    const std::shared_ptr<detail::NotificationLayoutRuntimeDimensions::State> child_;
    const std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence_;
    const std::shared_ptr<const Core::FullXmlOccurrence> xml_;
    std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> native_;
    // Sole caller is the actual Image v44 returning branch with typed common return.
    // A DOM type, parsed_source_serial or successful prefix cannot call it.
    CommonChildParserReturn(const CommonChildParserFrame*,
        std::shared_ptr<detail::NotificationLayoutRuntimeDimensions::State>,
        std::shared_ptr<const Core::LayoutReceiverOccurrence>,
        std::shared_ptr<const Core::FullXmlOccurrence>);
    CommonChildParserReturn(const RootOrdinaryParserFrame*,
        std::shared_ptr<detail::NotificationLayoutRuntimeDimensions::State>,
        std::shared_ptr<const Core::LayoutReceiverOccurrence>,
        std::shared_ptr<const Core::FullXmlOccurrence>);
public:
    CommonChildParserReturn(const CommonChildParserReturn&)=delete;
    CommonChildParserReturn& operator=(const CommonChildParserReturn&)=delete;
};

class BnwWorldViewControlTree::CommonChildParserFrame final {
    friend class BnwWorldViewControlTree;
    friend class CommonChildParserReturn;
    friend class detail::NotificationLayoutRuntimeDimensions;
    using Runtime=detail::NotificationLayoutRuntimeDimensions;
    const std::thread::id creator_=std::this_thread::get_id();
    bool alive_=true,closed_=false;
    const std::shared_ptr<Core> core_;
    const std::shared_ptr<Runtime::State> parent_;
    const std::shared_ptr<const Core::LayoutReceiverOccurrence> parent_occurrence_;
    const std::shared_ptr<const Core::FullXmlOccurrence> parent_xml_,child_xml_;
    mutable std::weak_ptr<Runtime::State> child_;
    const CommonChildParserFrame* const previous_;
    const std::map<std::string,content::XmlNode,std::less<>>* const parent_styles_;
    const std::uint64_t child_id_;
    CommonChildParserFrame(std::shared_ptr<Core> core,std::shared_ptr<Runtime::State> parent,
        std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence,
        std::shared_ptr<const Core::FullXmlOccurrence> parent_xml,
        std::shared_ptr<const Core::FullXmlOccurrence> child_xml,std::uint64_t child_id)
        :core_(std::move(core)),parent_(std::move(parent)),parent_occurrence_(std::move(occurrence)),
         parent_xml_(std::move(parent_xml)),child_xml_(std::move(child_xml)),
         previous_(core_->layout_parser_child_frame),parent_styles_(core_->layout_parser_styles),child_id_(child_id){}
    void require_parent_storage_without_callbacks()const {
        require(creator_==std::this_thread::get_id()&&alive_&&!closed_&&parent_&&
            parent_->alive.load()&&!parent_->failed.load(),
            "Common child frame parent retired/foreign/failed FIRST");
        BnwWorldViewControlTree::require_ordinary_root_read_origin_without_callbacks(core_);
        core_->layout_origin(true);parent_->operation->require_origin_without_callbacks();
        require(&core_->source->current_operation_loan()==parent_->operation&&
            parent_->serial==parent_->operation->operation_serial()&&core_->topology_revision==parent_->topology,
            "Common child frame SAME lexical Source/topology changed");
        const auto source=parent_xml_->source.lock(),child_source=child_xml_->source.lock();
        require(source&&child_source&&source.get()==core_->source.get()&&child_source.get()==source.get()&&
            !source.owner_before(core_->source)&&!core_->source.owner_before(source)&&
            !source.owner_before(child_source)&&!child_source.owner_before(source),
            "Common child frame actual selected XML Source controls changed");
        const auto& parent_node=Core::node(core_->nodes,parent_->id);
        const auto& child_node=Core::node(core_->nodes,child_id_);
        const auto direct_parent=child_xml_->parent.lock();
        require(parent_node.alive&&child_node.alive&&!parent_node.dynamic&&!child_node.dynamic&&
            parent_node.layout_baseline&& &*parent_node.layout_baseline==parent_->baseline&&
            parent_node.layout_receiver_occurrence.get()==parent_occurrence_.get()&&
            !parent_node.layout_receiver_occurrence.owner_before(parent_occurrence_)&&
            !parent_occurrence_.owner_before(parent_node.layout_receiver_occurrence)&&
            parent_node.full_xml_occurrence.get()==parent_xml_.get()&&
            !parent_node.full_xml_occurrence.owner_before(parent_xml_)&&!parent_xml_.owner_before(parent_node.full_xml_occurrence)&&
            child_node.full_xml_occurrence.get()==child_xml_.get()&&
            !child_node.full_xml_occurrence.owner_before(child_xml_)&&!child_xml_.owner_before(child_node.full_xml_occurrence)&&
            direct_parent&&direct_parent.get()==parent_xml_.get()&&
            !direct_parent.owner_before(parent_xml_)&&!parent_xml_.owner_before(direct_parent)&&
            child_node.native_initial_parent==parent_->id,
            "Common child frame genuine direct ordered occurrences/allocations changed");
        core_->layout_origin(true);parent_->operation->require_origin_without_callbacks();
    }
    void require_suspended_without_callbacks()const {
        require_parent_storage_without_callbacks();
        require(parent_->common_child_suspended&&core_->layout_parser_child_frame==this&&
            (!core_->layout_parser_control_id||*core_->layout_parser_control_id==child_id_),
            "Common child frame is not SAME entered stack top");
    }
    void bind_child_runtime_owned(const std::shared_ptr<Runtime::State>& child)const {
        require_suspended_without_callbacks();Runtime::require_storage_without_callbacks(child);
        require(child_.expired()&&child->id==child_id_&&child->tree_facade==parent_->tree_facade&&
            child->operation==parent_->operation,
            "Common child actual Runtime capture reused/substituted State");
        child_=child; //Nonallocating weak SAME actual issuance before any Context callback.
        require_suspended_without_callbacks();Runtime::require_storage_without_callbacks(child);
    }
    static std::unique_ptr<CommonChildParserFrame> enter_owned(
        BnwWorldViewControlTree& tree,const Runtime& parent_runtime,std::size_t actual_direct_ordinal){
        const auto parent=parent_runtime.state_;Runtime::require_storage_without_callbacks(parent);
        require(parent->id!=0&&parent->baseline->parser_runtime_dimensions_==&parent_runtime&&
            !parent->baseline->parsed_source_serial_,
            "Common child entry requires genuine still-entered ordinary parent parser");
        const auto core=parent->core.lock();require(core&&core.get()==tree.core_.get()&&parent->tree_facade==&tree,
            "Common child entry substituted Tree facade");
        const auto& node=Core::node(core->nodes,parent->id);const auto xml=node.full_xml_occurrence;
        require(xml&&!node.dynamic&&!xml->dynamic&&actual_direct_ordinal<xml->value.children.size(),
            "Common child entry lacks selected full ordinary XML occurrence");
        require(parent->baseline->parser_layers_.size()==1&&xml->value.attribute("Style").empty(),
            "Common child Style-layer factory requires its distinct genuine selected occurrence producer");
        std::shared_ptr<const Core::FullXmlOccurrence> child_xml;std::uint64_t child_id=0;
        for(const auto& candidate:core->nodes){
            if(candidate.native_initial_parent!=parent->id||!candidate.full_xml_occurrence)continue;
            const auto candidate_xml=candidate.full_xml_occurrence;
            const auto owner=candidate_xml->parent.lock();
            if(candidate_xml->ordinal==actual_direct_ordinal&&owner&&owner.get()==xml.get()&&
                !owner.owner_before(xml)&&!xml.owner_before(owner)){
                require(!child_id,"Common direct child Source occurrence duplicated");
                child_id=candidate.id;child_xml=candidate_xml;
            }
        }
        // Missing locator is not a native factory NULL. Instance/Lua/declaration
        // branches and unknown diagnostic path require their genuine producers.
        require(child_id&&child_xml&&!Core::node(core->nodes,child_id).layout_baseline&&
            !Core::node(core->nodes,child_id).layout_receiver_occurrence,
            "Common child actual factory locator missing/already constructed; no NULL/skip inference");
        const auto occurrence=parent->occurrence.lock();require(bool(occurrence),"Common parent occurrence retired");
        auto result=std::unique_ptr<CommonChildParserFrame>(new CommonChildParserFrame(
            core,parent,occurrence,xml,child_xml,child_id));
        Runtime::require_storage_without_callbacks(parent);
        parent->operation->require_current();Runtime::require_storage_without_callbacks(parent);
        result->require_parent_storage_without_callbacks();
        require(core->layout_parser_child_frame==result->previous_,"Common child stack changed during allocation");
        // Nonallocating role transition. Parent FIRST now fails before Source.
        require(result->parent_styles_,"Actual parent Style lexical scope absent");
        parent->common_child_suspended=true;core->layout_parser_styles=nullptr;
        core->layout_parser_borrower=nullptr;core->layout_parser_facade=nullptr;
        core->layout_parser_occurrence.reset();core->layout_parser_control_id.reset();
        core->layout_parser_child_frame=result.get();result->require_suspended_without_callbacks();
        return result;
    }
    void require_child_return_without_callbacks(const CommonChildParserReturn& returned)const {
        require(returned.creator_==std::this_thread::get_id()&&!returned.consumed_&&returned.frame_==this&&
            returned.child_&&!returned.child_->alive.load()&&!returned.child_->failed.load(),
            "Common child closure needs real child Runtime DTOR and exact unconsumed returning branch");
        require_suspended_without_callbacks();
        require(!core_->layout_parser_control_id&&!core_->layout_parser_occurrence&&
            !core_->layout_parser_borrower&&!core_->layout_parser_facade,
            "Common child closure before actual child Binding DTOR");
        const auto& child_node=Core::node(core_->nodes,child_id_);
        require(returned.native_&&returned.native_->impl_&&child_node.layout_baseline&&
            child_node.layout_baseline->base_.common_return_owned==returned.native_,
            "Child DTOR closure lost exact native common return");
        const auto completed=returned.native_->impl_;
        const auto native_core=completed->core.lock();const auto native_occurrence=completed->occurrence.lock();
        const auto native_xml=completed->selected.lock();const auto native_receiver=completed->receiver.lock();
        const auto native_children=completed->children.lock();
        require(completed->creator==std::this_thread::get_id()&&native_core&&native_core.get()==core_.get()&&
            !native_core.owner_before(core_)&&!core_.owner_before(native_core)&&native_occurrence&&
            native_occurrence==child_node.layout_receiver_occurrence&&native_xml==child_xml_&&native_receiver&&
            native_receiver==child_node.layout_baseline->base_.receiver_owned&&native_children&&
            native_children==child_node.layout_baseline->base_.children_78&&completed->serial==parent_->serial&&
            native_children->impl_->ordered.size()==completed->count&&completed->count==child_xml_->value.children.size()&&
            (child_node.layout_baseline->base_.flags_40&8u),"Child native return current physical identity/suffix changed");
        const auto captured_child=child_.lock();
        require(captured_child&&captured_child.get()==returned.child_.get()&&
            !captured_child.owner_before(returned.child_)&&!returned.child_.owner_before(captured_child)&&
            returned.child_->id==child_id_&&returned.child_->tree_facade==parent_->tree_facade&&
            returned.child_->operation==parent_->operation&&child_node.layout_baseline&&
            returned.child_->baseline==&*child_node.layout_baseline&&
            returned.occurrence_.get()==child_node.layout_receiver_occurrence.get()&&
            !returned.occurrence_.owner_before(child_node.layout_receiver_occurrence)&&
            !child_node.layout_receiver_occurrence.owner_before(returned.occurrence_)&&
            returned.xml_.get()==child_xml_.get()&&!returned.xml_.owner_before(child_xml_)&&
            !child_xml_.owner_before(returned.xml_),"Common child closure exact actual return changed");
    }
    void close_after_child_return_owned(CommonChildParserReturn& returned){
        try{
        require_child_return_without_callbacks(returned);
        parent_->operation->require_current();require_child_return_without_callbacks(returned);
        require(!core_->layout_parser_control_id&&!core_->layout_parser_borrower&&!core_->layout_parser_occurrence,
            "Common child binding revived across full DTOR POST");
        // Retire binding BEFORE restoring parent role. No allocation/callback.
        returned.consumed_=true;alive_=false;closed_=true;
        core_->layout_parser_child_frame=previous_;core_->layout_parser_styles=parent_styles_;
        core_->layout_parser_control_id=parent_->id;core_->layout_parser_occurrence=parent_occurrence_;
        core_->layout_parser_facade=parent_->tree_facade;core_->layout_parser_borrower=parent_->facade;
        parent_->common_child_suspended=false;
        Runtime::require_storage_without_callbacks(parent_);
        parent_->operation->require_current();Runtime::require_storage_without_callbacks(parent_);
        }catch(...){
            Runtime::fail_storage_without_callbacks(parent_);
            Runtime::fail_storage_without_callbacks(returned.child_);
            throw;
        }
    }
public:
    ~CommonChildParserFrame()noexcept {
        if(closed_)return;
        alive_=false;Runtime::fail_storage_without_callbacks(parent_);
        if(const auto child=child_.lock())Runtime::fail_storage_without_callbacks(child);
        // Failed/unreturned child NEVER reactivates a suspended parent.
        if(creator_==std::this_thread::get_id()&&core_->layout_parser_child_frame==this){
            core_->layout_parser_borrower=nullptr;core_->layout_parser_facade=nullptr;
            core_->layout_parser_occurrence.reset();core_->layout_parser_control_id.reset();
            core_->layout_parser_styles=nullptr;core_->layout_parser_child_frame=previous_;
        }
    }
    CommonChildParserFrame(const CommonChildParserFrame&)=delete;
    CommonChildParserFrame& operator=(const CommonChildParserFrame&)=delete;
};

BnwWorldViewControlTree::CommonChildParserReturn::CommonChildParserReturn(
    const CommonChildParserFrame* frame,std::shared_ptr<detail::NotificationLayoutRuntimeDimensions::State> child,
    std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence,std::shared_ptr<const Core::FullXmlOccurrence> xml)
    :frame_(frame),child_(std::move(child)),occurrence_(std::move(occurrence)),xml_(std::move(xml)){
    require(frame_&&child_&&occurrence_&&xml_,"Common returning branch actual owners absent");
    frame_->require_suspended_without_callbacks();
    detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(child_);
    native_=child_->baseline->base_.common_return_owned;
    require(native_&&native_->impl_&&native_->impl_->serial==child_->serial,
        "Image child returning branch requires actual native common completion receipt");
    const auto captured=frame_->child_.lock();
    require(captured&&captured.get()==child_.get()&&!captured.owner_before(child_)&&!child_.owner_before(captured)&&
        child_->id==frame_->child_id_&&child_->baseline->base_.common_return_owned&&child_->baseline->parser_runtime_dimensions_==nullptr&&
        child_->operation==frame_->parent_->operation&&xml_.get()==frame_->child_xml_.get()&&
        !xml_.owner_before(frame_->child_xml_)&&!frame_->child_xml_.owner_before(xml_),
        "Common returning branch requires actual finished child receiver, never parent scalar alias");
}
