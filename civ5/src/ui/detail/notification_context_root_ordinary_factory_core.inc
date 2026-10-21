// Inside civ5::ui after actual RootRead cursor and closed ordinary parser frame.
struct detail::NotificationContextRootOrdinaryFactoryReturn::Impl final {
    using Tree=BnwWorldViewControlTree;
    using Root=NotificationContextRootReadChildrenLoan;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Tree::Core> core;
    const std::shared_ptr<const Root> root;
    const Root::Guard read;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::shared_ptr<const Tree::RootOrdinaryParserFrame::ClosedReturn> closed;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<const NotificationLayoutContextRealmRef> realm;
    const std::shared_ptr<const Tree::Core::FullXmlOccurrence> root_xml;
    const Subtype subtype;
    const std::uint64_t id;
    Impl(std::shared_ptr<Tree::Core> c,std::shared_ptr<const Root> r,Root::Guard g,
        const runtime::BnwWorldViewSourceOperationLoan* op,
        std::shared_ptr<const Tree::RootOrdinaryParserFrame::ClosedReturn> result,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> child,
        std::shared_ptr<const NotificationLayoutContextRealmRef> context,
        std::shared_ptr<const Tree::Core::FullXmlOccurrence> xml,Subtype type,std::uint64_t control)
        :core(std::move(c)),root(std::move(r)),read(std::move(g)),operation(op),closed(std::move(result)),
         receiver(std::move(child)),realm(std::move(context)),root_xml(std::move(xml)),subtype(type),id(control){}
    void first()const {Tree::require_root_ordinary_factory_current_without_callbacks(*this);}
    void full()const {first();read.require_source_owned(*operation);first();}
};

void BnwWorldViewControlTree::require_root_ordinary_factory_current_without_callbacks(
    const detail::NotificationContextRootOrdinaryFactoryReturn::Impl& value){
    using Tree=BnwWorldViewControlTree;
    using Subtype=detail::NotificationContextRootOrdinaryFactoryReturn::Subtype;
    const auto& creator=value.creator;
    const auto& core=value.core;
    const auto& root=value.root;
    const auto& read=value.read;
    const auto& operation=value.operation;
    const auto& closed=value.closed;
    const auto& receiver=value.receiver;
    const auto& realm=value.realm;
    const auto& root_xml=value.root_xml;
    const auto& subtype=value.subtype;
    const auto& id=value.id;
        require(creator==std::this_thread::get_id()&&root&&closed&&closed->creator==creator&&
            closed->frame_ended.load(),"Ordinary factory requires genuine parser frame DTOR FIRST");
        read.require_current_without_callbacks();
        const auto actual=core.lock();
        require(actual&&operation&&receiver&&realm&&root_xml,"Ordinary factory actual custody retired");
        operation->require_origin_without_callbacks();actual->layout_origin(true);
        const auto& node=Tree::Core::node(actual->nodes,id);
        require(node.alive&&!node.dynamic&&node.layout_baseline&&node.full_xml_occurrence==closed->xml&&
            node.layout_receiver_occurrence==closed->occurrence&&closed->runtime&&
            !closed->runtime->alive.load()&&!closed->runtime->failed.load()&&closed->runtime->id==id&&
            closed->runtime->operation==operation&&closed->runtime->baseline==&*node.layout_baseline&&
            node.layout_baseline->base_.common_return_owned==closed->common&&
            node.layout_baseline->base_.receiver_owned==receiver&&receiver->impl_&&
            receiver->impl_->canonical.lock()==receiver&&receiver->impl_->core.lock()==actual&&
            receiver->impl_->occurrence.lock()==closed->occurrence&&
            closed->xml->parent.lock()==root_xml&&actual->context_root_parsed_xml_occurrence==root_xml&&
            Tree::Core::same_layout_xml(closed->xml->value,read.actual_declaration_without_callbacks())&&
            read.actual_context_realm_without_callbacks()==realm,
            "Ordinary factory SAME returned receiver/current Root/XML changed");
        const auto expected=[&]() ->std::string_view {
            switch(subtype){case Subtype::image:return "Image";case Subtype::box:return "Box";
                case Subtype::button:return "Button";case Subtype::label:return "Label";
                case Subtype::stack:return "Stack";case Subtype::grid:return "Grid";}
            return {};
        }();
        require(!expected.empty()&&node.layout_baseline->actual_type_==expected&&closed->xml->value.name==expected,
            "Ordinary factory concrete subtype cannot be substituted");
        const auto native=closed->common?closed->common->impl_:nullptr;
        const auto& base=node.layout_baseline->base_;
        require(native&&native->core.lock()==actual&&native->occurrence.lock()==closed->occurrence&&
            native->selected.lock()==closed->xml&&native->receiver.lock()==receiver&&
            native->children.lock()==base.children_78&&base.children_78&&base.children_78->impl_&&
            base.children_78->impl_->ordered.size()==native->count&&native->count==closed->xml->value.children.size()&&
            native->serial==operation->operation_serial()&&(base.flags_40&8u),
            "Ordinary factory physical common return/children78/OR8 changed");
        read.require_current_without_callbacks();
    
}

detail::NotificationContextRootOrdinaryFactoryReturn::NotificationContextRootOrdinaryFactoryReturn(
    std::shared_ptr<const Impl> value):impl_(std::move(value)){}
detail::NotificationContextRootOrdinaryFactoryReturn::~NotificationContextRootOrdinaryFactoryReturn()=default;
detail::NotificationContextRootOrdinaryFactoryReturn::Guard::Guard(std::shared_ptr<const Impl> value):impl_(std::move(value)){}
void detail::NotificationContextRootOrdinaryFactoryReturn::Guard::require_current_without_callbacks()const {impl_->first();}
void detail::NotificationContextRootOrdinaryFactoryReturn::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const {
    impl_->first();require(impl_->operation==&op,"Ordinary factory offered a different Source operation");impl_->full();
}
void detail::NotificationContextRootOrdinaryFactoryReturn::Guard::require_same_root_read_without_callbacks(
    const NotificationContextRootReadChildrenLoan& root)const {
    impl_->first();const auto current=root.capture_guard_without_callbacks();current.require_current_without_callbacks();
    require(impl_->root.get()==&root&&impl_->read.impl_==current.impl_,"Ordinary factory offered another RootRead loan");
}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
detail::NotificationContextRootOrdinaryFactoryReturn::Guard::actual_receiver_without_callbacks()const {impl_->first();return impl_->receiver;}
detail::NotificationContextRootOrdinaryFactoryReturn::Subtype
detail::NotificationContextRootOrdinaryFactoryReturn::Guard::actual_subtype_without_callbacks()const {impl_->first();return impl_->subtype;}
std::uint64_t detail::NotificationContextRootOrdinaryFactoryReturn::Guard::actual_control_id_without_callbacks()const {impl_->first();return impl_->id;}
const content::XmlNode& detail::NotificationContextRootOrdinaryFactoryReturn::Guard::actual_declaration_without_callbacks()const {
    impl_->first();return impl_->read.actual_declaration_without_callbacks();
}
std::shared_ptr<const detail::NotificationLayoutContextRealmRef>
detail::NotificationContextRootOrdinaryFactoryReturn::Guard::actual_context_realm_without_callbacks()const {impl_->first();return impl_->realm;}
detail::NotificationContextRootOrdinaryFactoryReturn::Guard
detail::NotificationContextRootOrdinaryFactoryReturn::capture_guard_for_controls_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootReadChildrenLoan& root)const {
    impl_->first();Guard guard(impl_);guard.require_same_root_read_without_callbacks(root);guard.require_source_owned(op);return guard;
}

std::shared_ptr<const detail::NotificationContextRootOrdinaryFactoryReturn>
BnwWorldViewControlTree::construct_root_ordinary_child_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>& root){
    using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
    using Base=detail::NotificationLayoutBaseline;
    require(root!=nullptr,"Ordinary529520 requires the real lexical RootRead loan");
    const auto read=root->capture_guard_without_callbacks();read.require_source_owned(operation);
    const auto evidence=root->impl_->evidence;const auto actual=evidence->core.lock();
    require(actual&&actual==core_&&evidence->operation==&operation,"Ordinary529520 substituted actual parent Core");
    const auto*const declaration=&read.actual_declaration_without_callbacks();
    const auto tree=actual->layout_resource_tree.lock();
    require(tree&&tree.get()==this,"Ordinary factory canonical Tree custody absent");
    const auto document=evidence->document;const auto realm=read.actual_context_realm_without_callbacks();
    const auto first=[&]{read.require_current_without_callbacks();
        require(evidence->core.lock()==actual&&evidence->operation==&operation&&
            &read.actual_declaration_without_callbacks()==declaration&&read.actual_context_realm_without_callbacks()==realm&&
            actual->context_root_children_evidence.lock()==evidence,"Ordinary529520 actual Root declaration/realm changed");};
    const auto full=[&]{first();read.require_source_owned(operation);first();};
    full();
    const auto type=[&]{
        if(declaration->name=="Image")return Factory::Subtype::image;
        if(declaration->name=="Box")return Factory::Subtype::box;
        if(declaration->name=="Button")return Factory::Subtype::button;
        if(declaration->name=="Label")return Factory::Subtype::label;
        if(declaration->name=="Grid")return Factory::Subtype::grid;
        if(declaration->name=="Stack")return Factory::Subtype::stack;
        // Their actual ordinary Receiver and common returning suffix are
        // separate producers; a successful generic parser cannot replace them.
        throw runtime::ScriptError("Ordinary529520 selected subtype needs its genuine constructor/v44/common return producer");
    }();
    try{
        require(document&&document->storage&&document->storage->document&&actual->root_full_xml_occurrence&&
            Core::same_layout_xml(actual->root_full_xml_occurrence->value,*document->storage->document),
            "Ordinary529520 newly parsed XML84 differs from selected Source locator arena");
        const auto old_root=actual->root_full_xml_occurrence;
        // Cold nodes are ONLY a locator. The following arena is copied from
        // the actual new parsed XML84 and is not a native factory receipt.
        if(!actual->context_root_parsed_xml_occurrence){
            auto fresh=std::make_shared<const Core::FullXmlOccurrence>(*document->storage->document,actual->source,nullptr,0,false);
            full();require(!actual->context_root_parsed_xml_occurrence,"Ordinary actual parsed arena replay");
            actual->context_root_parsed_xml_occurrence=std::move(fresh);
        }
        const auto fresh_root=actual->context_root_parsed_xml_occurrence;full();
        require(Core::same_layout_xml(fresh_root->value,*document->storage->document),"Ordinary actual parsed root arena replaced");
        std::uint64_t id=0;
        for(const auto& node:actual->nodes){
            if(node.native_initial_parent||node.dynamic||!node.full_xml_occurrence)continue;
            const auto old=node.full_xml_occurrence;const auto parent=old->parent.lock();
            if(parent==old_root&&old->ordinal==evidence->ordinal){
                require(!id&&Core::same_layout_xml(old->value,*declaration),"Ordinary current declaration locator duplicated/mismatched");id=node.id;
            }
        }
        require(id&&!Core::node(actual->nodes,id).layout_baseline&&!Core::node(actual->nodes,id).layout_receiver_occurrence,
            "Ordinary529520 real unconstructed locator missing; no NULL factory inferred");
        const auto rebind=[&](auto&& self,std::uint64_t control,
            std::shared_ptr<const Core::FullXmlOccurrence> parent,std::size_t ordinal,const content::XmlNode& xml)->void {
            first();const auto old=Core::node(actual->nodes,control).full_xml_occurrence;
            require(old&&Core::same_layout_xml(old->value,xml)&&!Core::node(actual->nodes,control).layout_baseline&&
                !Core::node(actual->nodes,control).layout_receiver_occurrence,"Ordinary fresh XML occurrence cannot overwrite a constructed receiver");
            auto fresh=std::make_shared<const Core::FullXmlOccurrence>(xml,actual->source,parent,ordinal,false);full();
            require(Core::node(actual->nodes,control).full_xml_occurrence==old,"Ordinary XML locator changed during allocation");
            Core::mutable_node_owned(actual->nodes,control).full_xml_occurrence=fresh;
            for(std::size_t n=0;n<xml.children.size();++n){
                const auto& child=xml.children[n];std::uint64_t found=0;
                // Declarations are retained data. Their native branches may
                // not be manufactured here; actual Common owns their handling.
                if(child.name=="LuaContext"||child.name=="Instance"||child.name=="InstanceData")continue;
                for(const auto& candidate:actual->nodes)
                    if(candidate.native_initial_parent==control&&candidate.full_xml_occurrence&&
                        candidate.full_xml_occurrence->parent.lock()==old&&candidate.full_xml_occurrence->ordinal==n){
                        require(!found,"Ordinary fresh direct child locator duplicated");found=candidate.id;
                    }
                require(found!=0,"Ordinary fresh direct child locator absent");self(self,found,fresh,n,child);full();
            }
        };
        rebind(rebind,id,fresh_root,evidence->ordinal,*declaration);full();
        const auto xml=Core::node(actual->nodes,id).full_xml_occurrence;
        const auto gui=actual->context_gui_port;require(gui&&gui->impl_&&gui->impl_->selected_styles,
            "Ordinary actual selected Gui/Styles owner missing");
        const auto styles=gui->impl_->selected_styles;styles->first();full();
        require(!actual->layout_parser_root_child_frame&&!actual->layout_parser_child_frame&&
            !actual->layout_parser_control_id&&!actual->layout_parser_borrower&&!actual->layout_parser_styles,
            "Ordinary actual Root parser entered/replayed before factory");
        auto frame=std::unique_ptr<RootOrdinaryParserFrame>(new RootOrdinaryParserFrame(actual,root,read,tree,&operation,id,xml,styles));
        full();require(!actual->layout_parser_root_child_frame&&!actual->layout_parser_child_frame,
            "Ordinary parser stack changed during construction");
        actual->layout_parser_root_child_frame=frame.get();frame->require_source_owned();
        auto returned=capture_layout_node_canonical_owned(id,frame->parser_styles_,operation.operation_serial());
        require(returned!=nullptr,"Ordinary actual selected v44 did not produce a real returning branch");
        auto closed=frame->close_after_return_owned(*returned);
        frame.reset(); // Real lexical parser frame DTOR precedes factory return.
        full();require(closed->frame_ended.load(),"Ordinary factory before real parser frame DTOR");
        const auto& node=Core::node(actual->nodes,id);require(node.layout_baseline&&node.layout_baseline->base_.receiver_owned&&
            node.layout_baseline->base_.parent_38.expired(),"Ordinary factory must precede genuine parent38 attachment");
        const auto receiver=node.layout_baseline->base_.receiver_owned;
        const auto require_native_return=[&]{
            if(type==Factory::Subtype::button)
                require_button_native_common_return_owned(actual,id,operation,true);
            if(type==Factory::Subtype::stack)
                require_stack_native_common_return_owned(actual,id,operation,true);
        };
        require_native_return();
        auto fields=std::make_shared<const Factory::Impl>(actual,root,read,&operation,closed,receiver,realm,fresh_root,type,id);
        full();fields->first();require_native_return();
        auto result=std::shared_ptr<const Factory>(new Factory(fields));full();fields->first();
        require_native_return();return result;
    }catch(...){actual->failed.store(true);throw;}
}
