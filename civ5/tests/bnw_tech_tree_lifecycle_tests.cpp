#include "civ5/ui/bnw_tech_tree_lifecycle.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "bnw_client_scene_frame_fixture.hpp"
#include <filesystem>
#include <iostream>
#include <thread>

namespace {
using namespace civ5;
using namespace civ5::ui;
using bnw_frame_fixture::check;
using bnw_frame_fixture::rejects;
constexpr TopPanelActionEvent science{TopPanelAction::open_technology_tree,TopPanelActionRoute::tech_tree_popup};
UiContextRegistry registry() {
    // Declared small registry fixture; never an executed native Lua context.
    return UiContextRegistry::from_documents(content::read_xml_metadata(
        R"(<Context><LuaContext ID="WorldView" FileName="WorldView"/><LuaContext ID="TechTree" FileName="TechTree" Hidden="1"/><LuaContext ID="Parent" FileName="Parent" Hidden="1"/><LuaContext ID="TestModal" FileName="Assets/UI/InGame/Popups/TestModal" Hidden="1" Modal="1"/></Context>)"),
        content::read_xml_metadata("<Context/>"));
}
std::shared_ptr<assets::LooseResourceIndex> original_sources(const std::filesystem::path &root) {
    auto index=std::make_shared<assets::LooseResourceIndex>(root);
    for (const auto *path:{"Assets/DLC/Expansion2/UI/InGame/TopPanel.lua","Assets/DLC/Expansion2/UI/InGame/TopPanel.xml",
        "Assets/DLC/Expansion2/UI/TechTree/TechTree.lua","Assets/DLC/Expansion2/UI/InGame/TechTree/TechTree.xml",
        "Assets/DLC/Expansion2/UI/InGame/InGame.xml"}) index->add_file(root/path);
    return index;
}
struct Harness {
    runtime::LiveSession session=bnw_frame_fixture::session();
    UiScreenController screens{registry()};
    assets::ResourceMountTable &resources;
    bool base_current=true;
    std::function<void()> preflight;
    BnwTechTreeLifecycle lifecycle;
    BnwClientFrameOwner client;
    LiveUiHost host;
    std::shared_ptr<const BnwClientFrame> frame;
    std::optional<LiveUiFrame> ui;
    LiveSessionUiSources sources() {
        LiveSessionUiSources result;
        // Root-owned trailing callback required for the coherent test build.
        // This runs inside actual publication, never by const_cast/substitution.
        result.augment_snapshot=[this](const runtime::LiveSession &s,UiFrameSnapshot snapshot) {
            auto fences=std::make_shared<UiOwnerFences>();
            if (const auto *current=snapshot.find<UiOwnerFences>()) *fences=*current;
            fences->checks.push_back([this]{if (auto callback=preflight) callback();return base_current;});
            return lifecycle.augment_snapshot(s,snapshot.with(std::shared_ptr<const UiOwnerFences>(fences)));
        };
        return result;
    }
    LiveUiSources host_sources() {
        LiveUiSources result;
        result.progression[static_cast<std::size_t>(ProgressionScreen::technologies)]=&lifecycle.source();
        result.active_tech_tree_event=[this]{return lifecycle.active_event(session);};
        result.tech_tree_processed=[this](const ProgressionTechnologyTreeEvent &e){return lifecycle.process(e);};
        result.tech_tree_local_dequeue_observed=[this](const ProgressionTechnologyTreeEvent &e){return lifecycle.local_dequeue_observed(e);};
        return result;
    }
    explicit Harness(assets::ResourceMountTable &r,BnwTechTreeLifecycle::Observer observer={})
        :resources(r),lifecycle(session,7,screens,r,std::move(observer)),
         client(session,"DECLARED-SMALL-LIFECYCLE-FIXTURE",7,sources()),
         host(screens,client.ui_provider(),nullptr,host_sources()) {publish();}
    void publish() {frame=client.capture({800,600,800,600,96},{});ui=host.begin_frame(800,600);}
    UiCommandResult open(const TopPanelActionEvent &action=science) {
        const auto result=lifecycle.admit_science_action(session,client,frame,host,*ui,action);
        // Actual scene dispatch must do this in production. Explicit fixture
        // boundary, not an alternative event producer.
        if (result.status==UiCommandStatus::performed || result.status==UiCommandStatus::unavailable)
            client.notify_world_changed();
        return result;
    }
    ProgressionTechnologyTreeEvent event() {
        const auto event=lifecycle.active_event(session);check(bool(event),"Current lifecycle event absent");return *event;
    }
};
void lifecycle_gate(assets::ResourceMountTable &resources) {
    Harness h(resources);
    check(h.lifecycle.matches_snapshot(h.session,h.frame->ui),"Absent-event publication is not owned");
    check(!h.lifecycle.active_event(h.session),"Construction invented a popup occurrence");
    const auto absent=h.frame;
    check(h.screens.dispatch(UiOpenScreen{"Parent"},h.ui->snapshot).status==UiCommandStatus::navigation,"Parent fixture open failed");
    h.publish();
    check(h.open().status==UiCommandStatus::performed,"Current actual shared-frame science action rejected");
    const auto event=h.event();
    check(h.screens.visible("TechTree") && h.screens.focused_screen()==std::optional<std::string>{"TechTree"},"Shared queue effect missing");
    check(h.lifecycle.semaphore()==1 && !h.lifecycle.matches_snapshot(h.session,absent->ui),"Queue did not retire absent-event epoch");
    check(h.open().status==UiCommandStatus::invalid,"Duplicate/stale admission was accepted");
    h.publish();
    const auto active=h.frame;
    const auto *published=h.frame->ui.find<ProgressionTechnologyTreeEvent>();
    check(published && published->token==event.token && h.lifecycle.matches_snapshot(h.session,h.frame->ui),"Exact owned occurrence not published");
    auto wrong=event;++wrong.token;
    check(h.lifecycle.process(wrong).status==UiCommandStatus::invalid,"Foreign token dequeued real occurrence");
    check(h.ui->technology_tree_close_ready,"Actual source/event did not enable authored close route");
    check(h.client.dispatch(h.host,*h.ui,h.frame,progression_tree_close_command(event.token)).status==UiCommandStatus::navigation,
        "Shared client/host post-ack reconciliation double-popped actual owner dequeue");
    check(!h.screens.visible("TechTree") && h.screens.focused_screen()==std::optional<std::string>{"Parent"},"Dequeue did not restore actual parent");
    check(!h.lifecycle.active_event(h.session) && h.lifecycle.semaphore()==0,"Processed did not retire occurrence/balance semaphore");
    // Outer client dispatch has retired publication after host checked the ack.
    check(!h.lifecycle.local_dequeue_observed(event) && !h.lifecycle.local_dequeue_observed(wrong),"Retired publication retained usable post-ack capability");
    check(!h.lifecycle.matches_snapshot(h.session,active->ui),"Processed did not retire captured event");
    check(h.lifecycle.process(event).status==UiCommandStatus::invalid,"Processed occurrence retried");
    const auto journal=h.lifecycle.journal();
    const std::vector<BnwTechTreeEffect> order{BnwTechTreeEffect::queued,BnwTechTreeEffect::show_semaphore_incremented,
        BnwTechTreeEffect::popup_shown,BnwTechTreeEffect::dequeued,BnwTechTreeEffect::hide_semaphore_decremented,
        BnwTechTreeEffect::popup_processed,BnwTechTreeEffect::open_flag_cleared};
    check(journal.size()==order.size(),"Local effects journal count differs");
    for (std::size_t i=0;i<order.size();++i) check(journal[i].effect==order[i] && journal[i].occurrence.token==event.token &&
        !journal[i].native_vm_executed && !journal[i].refresh_display_executed && journal[i].data2==-1 && journal[i].processed_result==0,
        "Journal mislabeled source order/scope/occurrence");
    h.client.notify_world_changed();h.publish();
    check(h.open().status==UiCommandStatus::performed && h.event().token!=event.token,"New occurrence reused old token");
    h.publish();
    check(h.lifecycle.process(h.event()).status==UiCommandStatus::performed,"Second actual occurrence close failed");
    check(h.lifecycle.local_dequeue_observed(h.lifecycle.journal().back().occurrence),"Current completed local dequeue proof missing");
}
void package_source_basis_gate(const std::filesystem::path &root) {
    const auto package_root=root/"Assets/DLC/Expansion2";
    auto package=std::make_shared<assets::LooseResourceIndex>(package_root);
    for (const auto *path:{"UI/InGame/TopPanel.lua","UI/InGame/TopPanel.xml","UI/TechTree/TechTree.lua",
        "UI/InGame/TechTree/TechTree.xml","UI/InGame/InGame.xml"}) package->add_file(package_root/path);
    assets::ResourceMountTable resources(assets::resource_lookup(package));
    {
        Harness h(resources);
        check(h.lifecycle.source().requested_key=="Assets/UI/InGame/TechTree/TechTree.xml" &&
            h.lifecycle.source().selected_source_path=="UI/InGame/TechTree/TechTree.xml",
            "Package-root source was rewritten or did not preserve the actual selected XML");
        check(h.open().status==UiCommandStatus::performed,"BNW package-root normal science action refused");
        h.publish();
        check(h.lifecycle.process(h.event()).status==UiCommandStatus::performed,
            "BNW package-root normal science close refused");
        for (const auto &effect:h.lifecycle.journal())
            check(effect.data2==-1 && !effect.native_vm_executed && !effect.refresh_display_executed,
                "BNW package-root fixture claimed an unsupported Lua path");
    }
    // A real Base producer is not an additional accepted BNW Lua variant.
    auto base=std::make_shared<assets::LooseResourceIndex>(root);
    for (const auto *path:{"Assets/DLC/Expansion2/UI/InGame/TopPanel.lua","Assets/DLC/Expansion2/UI/InGame/TopPanel.xml",
        "Assets/UI/InGame/TechTree/TechTree.lua","Assets/DLC/Expansion2/UI/InGame/TechTree/TechTree.xml",
        "Assets/DLC/Expansion2/UI/InGame/InGame.xml"}) base->add_file(root/path);
    assets::ResourceMountTable wrong_variant(assets::resource_lookup(base));
    bool rejected=false;
    try {Harness h(wrong_variant);}
    catch (const std::runtime_error &error) {
        const std::string message=error.what();
        rejected=message.find("TechTree selected source is outside the inspected stock contract")!=std::string::npos &&
            message.find("key=[Assets/UI/InGame/TechTree/TechTree.lua]")!=std::string::npos &&
            message.find("actual_sha=[efe3401e811cc98466a5e103798de7cb39b6a20e735589610e50a39a1a81be84]")!=std::string::npos;
    }
    check(rejected,"Base Lua producer did not fail the exact BNW source contract");
}
void admission_negatives(assets::ResourceMountTable &resources) {
    Harness h(resources);
    check(h.open({TopPanelAction::open_technology_tree,TopPanelActionRoute::choose_policy_popup}).status==UiCommandStatus::invalid,"Wrong typed route admitted");
    auto altered=*h.ui;altered.owner=nullptr;
    check(h.lifecycle.admit_science_action(h.session,h.client,h.frame,h.host,altered,science).status==UiCommandStatus::invalid,"Foreign host frame admitted");
    const auto original=*h.ui;h.host.begin_frame(800,600);
    check(h.lifecycle.admit_science_action(h.session,h.client,h.frame,h.host,original,science).status==UiCommandStatus::invalid,"Retired host sequence admitted");
    h.publish();h.base_current=false;
    check(h.open().status==UiCommandStatus::invalid && h.lifecycle.journal().empty(),"Unrelated base fence was removed");
    h.base_current=true;h.publish();
    auto extra=*h.ui;auto fences=std::make_shared<UiOwnerFences>(*extra.snapshot.find<UiOwnerFences>());
    fences->checks.push_back([]{return true;});extra.snapshot=extra.snapshot.with(std::shared_ptr<const UiOwnerFences>(fences));
    check(h.lifecycle.admit_science_action(h.session,h.client,h.frame,h.host,extra,science).status==UiCommandStatus::invalid,
        "Foreign additional fences were silently sanitized");
    // Actual registry classifies the /Popups/ source as modal; it does not
    // infer modal ownership from the otherwise ignored fixture Modal attr.
    const auto *modal=h.screens.registry().find("TestModal");
    check(modal && modal->modal,"Declared modal fixture did not select a real registry modal route");
    check(h.screens.dispatch(UiOpenScreen{"TestModal"},h.ui->snapshot).status==UiCommandStatus::navigation &&
        h.screens.focused_screen()==std::optional<std::string>{"TestModal"},"Actual modal fixture did not own focus");
    check(h.open().status==UiCommandStatus::invalid && h.lifecycle.journal().empty(),"Science crossed actual modal focus");
    rejects([&]{h.lifecycle.augment_snapshot(h.session,h.frame->ui);});
    rejects([&]{h.lifecycle.augment_snapshot(h.session,UiFrameSnapshot(89,7,12).with(std::make_shared<const ProgressionTechnologyTreeEvent>(ProgressionTechnologyTreeEvent{7,12,991})));});
    bool rejected=false;
    std::thread worker([&]{rejected=h.lifecycle.admit_science_action(h.session,h.client,h.frame,h.host,*h.ui,science).status==UiCommandStatus::invalid &&
        !h.lifecycle.active_event(h.session) && !h.lifecycle.matches_snapshot(h.session,h.frame->ui);});worker.join();
    check(rejected,"Foreign-thread ownership check accepted");
    check(h.lifecycle.journal().empty(),"Rejected paths emitted effects");
}
void replacements_and_sources(assets::ResourceMountTable &resources,std::shared_ptr<assets::LooseResourceIndex> original) {
    Harness h(resources);const auto checkpoint=h.session.checkpoint();
    check(h.open().status==UiCommandStatus::performed,"Restore fixture initial open failed");const auto old=h.event();
    h.session.restore(checkpoint);
    check(!h.lifecycle.active_event(h.session) && h.lifecycle.process(old).status==UiCommandStatus::invalid,"Identical restore reused borrowed lifecycle");
    // Explicit fresh shared controller and owner after intentional restore.
    UiScreenController screens(registry());BnwTechTreeLifecycle rebound(h.session,7,screens,resources);
    auto snapshot=rebound.augment_snapshot(h.session,UiFrameSnapshot(77,7,12));
    check(rebound.matches_snapshot(h.session,snapshot) && !h.lifecycle.matches_snapshot(h.session,snapshot),"Rebound owner confused private evidence");
    LiveSessionUiSources rebound_sources;
    rebound_sources.augment_snapshot=[&](const runtime::LiveSession &s,UiFrameSnapshot f){return rebound.augment_snapshot(s,f);};
    BnwClientFrameOwner rebound_client(h.session,"DECLARED-REBIND-FIXTURE",7,rebound_sources);
    LiveUiHost rebound_host(screens,rebound_client.ui_provider(),nullptr);
    const auto rebound_frame=rebound_client.capture({800,600,800,600,96},{});
    const auto rebound_ui=rebound_host.begin_frame(800,600);
    check(rebound.admit_science_action(h.session,rebound_client,rebound_frame,rebound_host,rebound_ui,science).status==UiCommandStatus::performed,
        "Explicit generation rebind did not admit actual client publication");
    const auto fresh=rebound.active_event(h.session);
    check(fresh && fresh->token!=old.token,"Rebound owner restarted occurrence token counter");
    rebound_client.notify_world_changed();rebound_client.capture({800,600,800,600,96},{});
    check(rebound.process(*fresh).status==UiCommandStatus::performed,"Rebound shared dequeue failed");
    snapshot=rebound.augment_snapshot(h.session,UiFrameSnapshot(78,7,12));
    h.session.restore(checkpoint);check(!rebound.matches_snapshot(h.session,snapshot),"Repeated restore kept rebound snapshot current");
    Harness source_guard(resources);
    resources.mount({777,0,9,assets::resource_lookup(original),true,true});
    check(source_guard.open().status==UiCommandStatus::invalid && source_guard.lifecycle.journal().empty(),"Same-byte changed mount identity accepted");
    resources.unmount(777);
    resources.set_default_enabled(false);
    check(!source_guard.lifecycle.matches_snapshot(source_guard.session,source_guard.frame->ui),"Missing selected stock source kept frame current");
    resources.set_default_enabled(true);
    // In-place missed active-player transition, without generation change.
    Harness active_guard(resources);
    auto &state=active_guard.session.mutable_state();runtime::rebuild_bnw_live_plot_visibility(state);
    active_guard.publish();state.world.pregame->archive.integers.erase("active_player");
    check(active_guard.open().status==UiCommandStatus::invalid && !active_guard.lifecycle.active_event(active_guard.session),"Missing live active-player owner accepted");
    assets::ResourceMountTable ambiguous([](const assets::AssetKey &key)->assets::ResourceMountHit {
        return {assets::AmbiguousAsset{key,{assets::SourceKind::loose,"declared-test-ambiguity","",{}},{}},true};
    });
    auto s=bnw_frame_fixture::session();UiScreenController c(registry());
    rejects([&]{BnwTechTreeLifecycle owner(s,7,c,ambiguous);});
}
void uncertain_and_reentry(assets::ResourceMountTable &resources) {
    Harness *current=nullptr;bool reentry_rejected=false;
    Harness reentry(resources,[&](const BnwTechTreeEffectObservation &observation) {
        reentry_rejected=current && current->lifecycle.process(observation.occurrence).status==UiCommandStatus::invalid &&
            !current->lifecycle.active_event(current->session);
    });current=&reentry;
    check(reentry.open().status==UiCommandStatus::performed && reentry_rejected,"Reentry manufactured a second transition");
    reentry.publish();
    check(reentry.lifecycle.process(reentry.event()).status==UiCommandStatus::performed,"Reentry rejection broke genuine transition");
    Harness fail_show(resources,[](const BnwTechTreeEffectObservation &o){if(o.effect==BnwTechTreeEffect::popup_shown) throw std::runtime_error("Declared observer failure");});
    check(fail_show.open().status==UiCommandStatus::unavailable && fail_show.lifecycle.uncertain() && fail_show.screens.visible("TechTree") &&
        !fail_show.lifecycle.active_event(fail_show.session),"Uncertain actual show was accepted/rolled back as if it never happened");
    check(fail_show.open().status==UiCommandStatus::invalid,"Uncertain occurrence admission retried");
    Harness fail_processed(resources,[](const BnwTechTreeEffectObservation &o){if(o.effect==BnwTechTreeEffect::popup_processed) throw std::runtime_error("Declared processed uncertainty");});
    check(fail_processed.open().status==UiCommandStatus::performed,"Processed uncertainty fixture open failed");const auto event=fail_processed.event();
    fail_processed.publish();
    check(fail_processed.lifecycle.process(event).status==UiCommandStatus::unavailable && fail_processed.lifecycle.uncertain() && !fail_processed.screens.visible("TechTree") &&
        fail_processed.lifecycle.semaphore()==0 && !fail_processed.lifecycle.active_event(fail_processed.session),"Actual partial close was relabeled success");
    check(fail_processed.lifecycle.process(event).status==UiCommandStatus::invalid,"Uncertain processed occurrence retried");
    check(!fail_processed.lifecycle.local_dequeue_observed(event),"Uncertain event absence became post-ack authority");
    Harness *changed=nullptr;
    Harness publication(resources,[&](const BnwTechTreeEffectObservation &o){if(o.effect==BnwTechTreeEffect::popup_shown) changed->client.notify_world_changed();});changed=&publication;
    check(publication.open().status==UiCommandStatus::unavailable && publication.lifecycle.uncertain(),"Callback replaced current client but admission succeeded");
    Harness *fenced=nullptr;
    Harness unrelated(resources,[&](const BnwTechTreeEffectObservation &o){if(o.effect==BnwTechTreeEffect::popup_shown) fenced->base_current=false;});fenced=&unrelated;
    check(unrelated.open().status==UiCommandStatus::unavailable,"Post-check removed an unrelated retired fence");
    Harness *restored=nullptr;std::optional<runtime::LiveSession::Checkpoint> checkpoint;
    Harness restore_during_show(resources,[&](const BnwTechTreeEffectObservation &o){
        if(o.effect==BnwTechTreeEffect::popup_shown) restored->session.restore(*checkpoint);
    });restored=&restore_during_show;checkpoint=restored->session.checkpoint();
    check(restore_during_show.open().status==UiCommandStatus::unavailable && restore_during_show.lifecycle.uncertain(),
        "Identical restore during observed show escaped post-generation guard");
}
void close_reconciliation_negatives(assets::ResourceMountTable &resources) {
    Harness liar(resources);check(liar.open().status==UiCommandStatus::performed,"Lying ack fixture open failed");liar.publish();
    // A deliberately lying external callback cannot authorize unchanged local
    // controller just because it says performed and stops exposing its token.
    bool pending=true;
    LiveUiSources lying;
    lying.progression[static_cast<std::size_t>(ProgressionScreen::technologies)]=&liar.lifecycle.source();
    const auto event=liar.event();
    lying.active_tech_tree_event=[&]()->std::optional<ProgressionTechnologyTreeEvent>{return pending?std::optional{event}:std::nullopt;};
    lying.tech_tree_processed=[&](const auto &){pending=false;return UiCommandResult{UiCommandStatus::performed,"Declared lying test callback"};};
    lying.tech_tree_local_dequeue_observed=[](const auto &){return true;};
    LiveUiHost lying_host(liar.screens,liar.client.ui_provider(),nullptr,lying);
    auto frame=lying_host.begin_frame(800,600);
    check(frame.technology_tree_close_ready,"Declared lying callback test lacks actual source/event");
    check(lying_host.dispatch(progression_tree_close_command(event.token),frame).status==UiCommandStatus::unavailable && liar.screens.visible("TechTree"),
        "Lying true ack caused fallback pop of unchanged real controller");
    // Foreign controller receives the dequeue; shared controller must stay open.
    pending=true;UiScreenController foreign(registry());foreign.dispatch(UiOpenScreen{"TechTree"},liar.frame->ui);
    lying.tech_tree_processed=[&](const auto &){foreign.dispatch(UiCloseScreen{"TechTree"},liar.frame->ui);pending=false;
        return UiCommandResult{UiCommandStatus::performed,"Declared foreign controller callback"};};
    LiveUiHost foreign_host(liar.screens,liar.client.ui_provider(),nullptr,lying);frame=foreign_host.begin_frame(800,600);
    check(foreign_host.dispatch(progression_tree_close_command(event.token),frame).status==UiCommandStatus::unavailable && liar.screens.visible("TechTree"),
        "Foreign controller true ack closed shared context");
    for (const bool throws:{false,true}) {
        Harness failed_ack(resources);check(failed_ack.open().status==UiCommandStatus::performed,"Failed capability fixture open failed");failed_ack.publish();
        auto source=failed_ack.host_sources();
        source.tech_tree_local_dequeue_observed=[throws](const auto &)->bool {
            if (throws) throw std::runtime_error("Declared acknowledgement failure");return false;
        };
        LiveUiHost rejecting(failed_ack.screens,failed_ack.client.ui_provider(),nullptr,source);
        const auto current=rejecting.begin_frame(800,600);const auto occurrence=failed_ack.event();
        check(rejecting.dispatch(progression_tree_close_command(occurrence.token),current).status==UiCommandStatus::unavailable &&
            !failed_ack.screens.visible("TechTree") && failed_ack.lifecycle.semaphore()==0,
            "Failed post-ack capability attempted fallback second pop/pretended success");
    }
    for (const auto effect:{BnwTechTreeEffect::dequeued,BnwTechTreeEffect::open_flag_cleared}) {
        Harness *changed=nullptr;
        Harness reopened(resources,[&](const BnwTechTreeEffectObservation &o){
            if(o.effect==effect) changed->screens.dispatch(UiOpenScreen{effect==BnwTechTreeEffect::dequeued?"Parent":"TechTree"},changed->ui->snapshot);
        });changed=&reopened;
        check(reopened.open().status==UiCommandStatus::performed,"Controller mutation fixture open failed");reopened.publish();const auto e=reopened.event();
        check(reopened.lifecycle.process(e).status==UiCommandStatus::unavailable && !reopened.lifecycle.local_dequeue_observed(e),
            "Wrong restored parent/reopened screen became successful local ack");
    }
    Harness source_changed(resources);check(source_changed.open().status==UiCommandStatus::performed,"Source mutation close fixture failed");source_changed.publish();
    const auto e=source_changed.event();check(source_changed.lifecycle.process(e).status==UiCommandStatus::performed,"Source mutation fixture close failed");
    resources.set_default_enabled(false);
    check(!source_changed.lifecycle.local_dequeue_observed(e),"Post-ack source mutation kept local effect capability current");
    resources.set_default_enabled(true);
    Harness base_changed(resources);check(base_changed.open().status==UiCommandStatus::performed,"Base mutation close fixture failed");base_changed.publish();const auto b=base_changed.event();
    check(base_changed.lifecycle.process(b).status==UiCommandStatus::performed,"Base mutation fixture close failed");base_changed.base_current=false;
    check(!base_changed.lifecycle.local_dequeue_observed(b),"Post-ack ignored unrelated base owner fence");
}
void lifetime_gate(assets::ResourceMountTable &resources) {
    auto session=bnw_frame_fixture::session();UiScreenController screens(registry());
    auto owner=std::make_unique<BnwTechTreeLifecycle>(session,7,screens,resources);
    const auto snapshot=owner->augment_snapshot(session,UiFrameSnapshot(1,7,12));
    check(ui_owners_current(snapshot),"Live owner weak guard unexpectedly failed");
    owner.reset();check(!ui_owners_current(snapshot),"Destroyed lifecycle left borrowed-this fence callable");
}

void pinned_arguments_gate(assets::ResourceMountTable &resources) {
    Harness telemetry(resources);
    std::vector<TopPanelActionEvent> events{science};
    bool reallocated=false;
    {
        assets::ScopedResourceResolveObserver observer(resources,[&](const auto &,const auto &,bool) {
            if (reallocated) return;
            reallocated=true;
            events.reserve(4096); // Invalidates the originally borrowed element.
            events.clear();
            events.push_back({TopPanelAction::open_game_menu,TopPanelActionRoute::queue_game_menu});
        });
        const auto result=telemetry.lifecycle.admit_science_action(telemetry.session,telemetry.client,
            telemetry.frame,telemetry.host,*telemetry.ui,events.front());
        check(reallocated && result.status==UiCommandStatus::performed,
            "Preflight vector reallocation substituted/freed the accepted science action");
    }
    check(telemetry.event().token!=0 && telemetry.screens.visible("TechTree"),
        "Pinned actual science action failed to produce its local occurrence");
    telemetry.client.notify_world_changed();telemetry.publish();
    check(telemetry.lifecycle.process(telemetry.event()).status==UiCommandStatus::performed,
        "Pinned action occurrence did not close normally");

    Harness ui_alias(resources);bool ui_replaced=false;
    const auto old_ui=*ui_alias.ui;
    ui_alias.preflight=[&] {
        if (ui_replaced) return;
        ui_replaced=true;
        ui_alias.ui=ui_alias.host.begin_frame(800,600);
    };
    check(ui_alias.lifecycle.admit_science_action(ui_alias.session,ui_alias.client,ui_alias.frame,
              ui_alias.host,*ui_alias.ui,science).status==UiCommandStatus::invalid && ui_replaced &&
              ui_alias.ui->sequence!=old_ui.sequence && ui_alias.lifecycle.journal().empty(),
          "Preflight optional replacement substituted a new host frame for old input");

    Harness publication_alias(resources);bool frame_replaced=false;
    const auto old_frame=publication_alias.frame; // Retain original immutable payload during callback.
    publication_alias.preflight=[&] {
        if (frame_replaced) return;
        frame_replaced=true;
        publication_alias.client.notify_world_changed();
        publication_alias.publish();
    };
    check(publication_alias.lifecycle.admit_science_action(publication_alias.session,publication_alias.client,
              publication_alias.frame,publication_alias.host,*publication_alias.ui,science).status==UiCommandStatus::invalid &&
              frame_replaced && publication_alias.frame!=old_frame && publication_alias.lifecycle.journal().empty(),
          "Preflight publication replacement substituted a new client frame for old input");
}

void augmentation_base_gate() {
    auto session=bnw_frame_fixture::session();
    std::optional<std::int64_t> selection;
    bool called=false;
    LiveSessionUiSources sources;
    sources.selected_unit=[&]{return selection;};
    sources.augment_snapshot=[&](const runtime::LiveSession &,UiFrameSnapshot frame) {
        called=true;selection=21; // Actual original selection fence now fails.
        return frame.with(std::make_shared<const UiOwnerFences>());
    };
    rejects([&]{capture_live_session_ui_frame(session,1,7,sources);});
    check(called && selection==21,"Original-base-fence regression did not reach augmentation");
}
}
int main(int argc,char **argv) {
    try {
        if (argc!=2) throw std::invalid_argument("Expected original game-root argument; no fixture-original claim");
        const std::string root=argv[1];
        const auto original=original_sources(std::filesystem::path(std::u8string(root.begin(),root.end())));
        assets::ResourceMountTable resources(assets::resource_lookup(original));
        package_source_basis_gate(std::filesystem::path(std::u8string(root.begin(),root.end())));
        lifecycle_gate(resources);admission_negatives(resources);replacements_and_sources(resources,original);
        uncertain_and_reentry(resources);close_reconciliation_negatives(resources);lifetime_gate(resources);augmentation_base_gate();pinned_arguments_gate(resources);
        std::cout<<"PASS actual mounted stock source pins + declared shared-controller lifecycle fixtures; nativeVM=false RefreshDisplay=false\n";
        return 0;
    } catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
