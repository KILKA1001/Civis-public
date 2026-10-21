    // Actual WorldView render startup calls the OWN Gameplay loader (state0).
    // Mount publication already returned; Styles then Colors above really ran.
    // Frontend state1 is not earned by this caller and is never enabled here.
    state->first(operation);operation.require_render_operation_owned();state->first(operation);
    const auto bank=operation.selected_provider_startup_bank_owned();state->first(operation);
    auto recipe=bank->source_recipe();state->first(operation);operation.require_current();state->first(operation);
    require(recipe.profile.enabled_mods.empty()&&!recipe.recipe_identity.empty()&&!recipe.mounts.empty(),
        "Gameplay startup lacks genuine published zero-mod mount recipe");
    const auto styles=state->selected_styles;const auto colors=state->selected_color_sets;
    styles->first();colors->first();
    require(styles->source_serial==operation.operation_serial()&&colors->construction_serial==styles->source_serial,
        "Gameplay startup Style/Color setup belongs to another Source occurrence");
    std::optional<std::array<std::size_t,3>> selected;
    for(const auto package_index:recipe.profile.enabled_dlc){
        require(package_index<recipe.catalog.packages.size(),"Gameplay startup selected package index invalid");
        const auto& package=recipe.catalog.packages[package_index];
        for(std::size_t skin_index=0;skin_index<package.document.children.size();++skin_index){
            const auto& skin=package.document.children[skin_index];
            if(skin.name!="UISkin"||skin.attribute("platform")!="Common")continue;
            for(std::size_t child_index=0;child_index<skin.children.size();++child_index){
                const auto& block=skin.children[child_index];if(block.name!="GameplaySkin")continue;
                for(const auto& row:block.children)if(row.name=="Color"){
                    require(package.path.parent_path().filename()=="Expansion2"&&row.text=="UI/ColorAtlas_Expansion2.xml"&&!selected,
                        "Gameplay startup active Color provider unsupported/ambiguous");
                    require(skin.attribute("name")=="Expansion2Primary"&&skin.attribute("set")=="Expansion2",
                        "Gameplay startup selected Common provider occurrence differs");
                    require(std::count_if(block.children.begin(),block.children.end(),[](const auto& x){
                        return x.name=="Style"&&x.text=="UI/Styles_Expansion2.xml";})==1,
                        "Gameplay startup missing actual selected Style loader input");
                    selected=std::array<std::size_t,3>{package_index,skin_index,child_index};
                }
            }
        }
    }
    state->first(operation);operation.require_current();state->first(operation);styles->first();colors->first();
    require(selected&&std::any_of(recipe.mounts.begin(),recipe.mounts.end(),[](const auto& mount){return mount.enabled&&mount.preferred;}),
        "Gameplay startup required selected active provider/mount publication absent");
    // Inspect only genuine same-Bank outputs. Post allocation/full Source cuts
    // precede every subsequent registry read and the final activation store.
    const auto capability=bank->capability();state->first(operation);
    constexpr std::array<std::string_view,3> style_keys{"Assets/UI/Styles.xml","Assets/DLC/Expansion/UI/Styles_Expansion1.xml","Assets/DLC/Expansion2/UI/Styles_Expansion2.xml"};
    constexpr std::array<std::string_view,2> color_keys{"Assets/UI/ColorAtlas.xml","Assets/DLC/Expansion2/UI/ColorAtlas_Expansion2.xml"};
    const auto check=[&]{state->first(operation);styles->first();colors->first();bank->require_current();
        operation.require_current();state->first(operation);styles->first();colors->first();};
    for(std::size_t i=0;i<style_keys.size();++i){
        check();const assets::AssetKey key{std::string(style_keys[i])};check();const auto quote=capability.loaded(key);check();
        require(quote.payload_identity.sha256==styles->sheets[i].payload_identity.sha256&&quote.bytes.get()==styles->sheets[i].bytes.get()&&
            !quote.bytes.owner_before(styles->sheets[i].bytes)&&!styles->sheets[i].bytes.owner_before(quote.bytes)&&quote.xml.get()==styles->sheets[i].xml.get()&&
            !quote.xml.owner_before(styles->sheets[i].xml)&&!styles->sheets[i].xml.owner_before(quote.xml),"Gameplay startup SAME Bank Style result changed");
    }
    for(std::size_t i=0;i<color_keys.size();++i){
        check();const assets::AssetKey key{std::string(color_keys[i])};check();const auto quote=capability.loaded(key);check();
        require(quote.payload_identity.sha256==colors->atlas[i].payload_identity.sha256&&quote.bytes.get()==colors->atlas[i].bytes.get()&&
            !quote.bytes.owner_before(colors->atlas[i].bytes)&&!colors->atlas[i].bytes.owner_before(quote.bytes)&&quote.xml.get()==colors->atlas[i].xml.get()&&
            !quote.xml.owner_before(colors->atlas[i].xml)&&!colors->atlas[i].xml.owner_before(quote.xml),"Gameplay startup SAME Bank Color result changed");
    }
    check();auto completed_capability=std::unique_ptr<assets::CivisResourceBankCapability>(
        new assets::CivisResourceBankCapability(bank->capability()));check();
    auto current_recipe=bank->source_recipe();check();
    require(current_recipe.recipe_identity==recipe.recipe_identity&&current_recipe.save_path==recipe.save_path&&
        current_recipe.game_root==recipe.game_root&&current_recipe.canonical_payload_root==recipe.canonical_payload_root&&
        Impl::SelectedProviderActivation::same_mounts(current_recipe.mounts,recipe.mounts),
        "Gameplay startup published mount recipe changed during loaders");
    check();auto activation=std::shared_ptr<Impl::SelectedProviderActivation>(new Impl::SelectedProviderActivation);check();
    activation->bank_=bank;activation->gui_=state;activation->styles_=styles;activation->colors_=colors;
    activation->published_recipe_.emplace(std::move(recipe));
    check(); // Owned recipe move/optional boundary before further registry reads.
    activation->selected_package_=(*selected)[0];activation->skin_occurrence_=(*selected)[1];activation->state_occurrence_=(*selected)[2];
    activation->source_serial_=styles->source_serial;
    activation->self_=activation;
    activation->capability_=std::move(completed_capability);
    for(std::size_t i=0;i<styles->sheets.size();++i){activation->style_bytes_[i]=styles->sheets[i].bytes;activation->style_xml_[i]=styles->sheets[i].xml;}
    for(std::size_t i=0;i<colors->atlas.size();++i){activation->color_bytes_[i]=colors->atlas[i].bytes;activation->color_xml_[i]=colors->atlas[i].xml;}
    check(); // Genuine normal completion precedes native-equivalent enabled +24.
    activation->state_=Impl::SelectedProviderActivation::State::completed_gameplay;
    activation->setup_returned_=true;activation->first(state);
    state->selected_provider_activation=activation; // Nonalloc actual startup publication.
    activation->published_=true;
    state->first(operation);operation.require_current();state->first(operation);activation->first(state);
