// Nested only in actual canonical Gui::Impl, AFTER completed Styles/Colors.
// This closed own x64 loader return is not an Original startup callback token.
class SelectedProviderActivation final {
    friend class detail::NotificationContextGuiPort;
    const std::thread::id creator_=std::this_thread::get_id();
    mutable std::atomic<bool> alive_{true},failed_{false};
    std::weak_ptr<const assets::CivisResourceBankOwner> bank_;
    std::weak_ptr<Impl> gui_;
    std::weak_ptr<SelectedStyleRegistry> styles_;
    std::weak_ptr<SelectedColorSetRegistry> colors_;
    std::weak_ptr<SelectedProviderActivation> self_;
    std::unique_ptr<assets::CivisResourceBankCapability> capability_;
    std::optional<assets::CivisResourceBankSourceRecipe> published_recipe_;
    std::array<std::shared_ptr<const assets::Bytes>,3> style_bytes_;
    std::array<std::shared_ptr<const content::XmlNode>,3> style_xml_;
    std::array<std::shared_ptr<const assets::Bytes>,2> color_bytes_;
    std::array<std::shared_ptr<const content::XmlNode>,2> color_xml_;
    std::size_t selected_package_=0,skin_occurrence_=0,state_occurrence_=0;
    std::uint64_t source_serial_=0;
    enum class State {disabled,completed_gameplay};
    State state_=State::disabled;
    bool setup_returned_=false;
    bool published_=false;
    SelectedProviderActivation()=default; // Allocates NO borrowed-state initializer.
    static bool same_mounts(const std::vector<assets::CivisResourceMountDescriptor>& a,
                            const std::vector<assets::CivisResourceMountDescriptor>& b){
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i)if(a[i].id!=b[i].id||a[i].group!=b[i].group||a[i].priority!=b[i].priority||
            a[i].enabled!=b[i].enabled||a[i].preferred!=b[i].preferred)return false;
        return true;
    }
 public:
    ~SelectedProviderActivation(){retire();}
    void retire()noexcept{alive_.store(false);} // Before Style/Color/Gui destruction.
    void first(const std::shared_ptr<Impl>& gui)const{
        try{
            require(creator_==std::this_thread::get_id()&&alive_.load()&&!failed_.load(),"Gameplay activation retired/foreign/failed FIRST");
            const auto actual=gui_.lock();const auto bank=bank_.lock();const auto styles=styles_.lock();const auto colors=colors_.lock();
            require(actual&&gui&&actual.get()==gui.get()&&!actual.owner_before(gui)&&!gui.owner_before(actual)&&bank&&styles&&colors,
                "Gameplay activation actual owners expired/substituted");
            require(gui->facade_alive.load()&&!gui->failed.load()&&!gui->source.expired(),"Gameplay activation Gui/Source retired");
            if(published_){const auto self=self_.lock();require(self&&self.get()==this&&gui->selected_provider_activation.get()==this&&
                !self.owner_before(gui->selected_provider_activation)&&!gui->selected_provider_activation.owner_before(self),
                "Gameplay activation actual published receipt substituted");}
            styles->first();colors->first();
            require(gui->selected_styles.get()==styles.get()&&!gui->selected_styles.owner_before(styles)&&!styles.owner_before(gui->selected_styles)&&
                gui->selected_color_sets.get()==colors.get()&&!gui->selected_color_sets.owner_before(colors)&&!colors.owner_before(gui->selected_color_sets)&&
                published_recipe_&&source_serial_&&setup_returned_&&styles->source_serial==source_serial_&&colors->construction_serial==source_serial_,
                "Gameplay activation lost genuine selected setup returns");
            require(bool(capability_),"Gameplay activation completed Bank epoch absent");
            capability_->require_current();
            bank->require_current();
            for(std::size_t i=0;i<style_bytes_.size();++i)require(styles->sheets[i].bytes.get()==style_bytes_[i].get()&&
                !styles->sheets[i].bytes.owner_before(style_bytes_[i])&&!style_bytes_[i].owner_before(styles->sheets[i].bytes)&&
                styles->sheets[i].xml.get()==style_xml_[i].get()&&!styles->sheets[i].xml.owner_before(style_xml_[i])&&!style_xml_[i].owner_before(styles->sheets[i].xml),
                "Gameplay activation SAME completed Style control changed");
            for(std::size_t i=0;i<color_bytes_.size();++i)require(colors->atlas[i].bytes.get()==color_bytes_[i].get()&&
                !colors->atlas[i].bytes.owner_before(color_bytes_[i])&&!color_bytes_[i].owner_before(colors->atlas[i].bytes)&&
                colors->atlas[i].xml.get()==color_xml_[i].get()&&!colors->atlas[i].xml.owner_before(color_xml_[i])&&!color_xml_[i].owner_before(colors->atlas[i].xml),
                "Gameplay activation SAME completed Color control changed");
        }catch(...){alive_.store(false);failed_.store(true);throw;}
    }
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan& op,const std::shared_ptr<Impl>& gui)const{
        try{
            const auto pure=[&]{first(gui);gui->first(op);first(gui);};
            require(!gui->canonical_gui.expired(),"Gameplay activation actual canonical Gui has not returned");
            pure();const auto serial=op.operation_serial();pure();op.require_operation(serial);pure();
            // The closed SAME Bank capability authenticates its immutable
            // published namespace/epoch. Bank::load can add cached payloads,
            // never replace mounts/recipe; no repeated borrowed catalog copy.
            pure();op.require_operation(serial);pure();
        }catch(...){alive_.store(false);failed_.store(true);throw;}
    }
    bool gameplay_enabled_without_callbacks(const std::shared_ptr<Impl>& gui)const{
        first(gui);return state_==State::completed_gameplay;
    }
};
std::shared_ptr<SelectedProviderActivation> selected_provider_activation;
