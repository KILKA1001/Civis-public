#include "civ5/graphics/bnw_ui_source_observation.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
using namespace civ5;
void check(bool v,const char *text) {if(!v)throw std::runtime_error(text);}
void write(const std::filesystem::path &p,std::string_view text) {
    std::filesystem::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary);out<<text;
    if(!out)throw std::runtime_error("Synthetic resource write failed");
}
void run(const std::filesystem::path &root) {
    write(root/"Stock/TopPanel.xml","<stock/>");write(root/"EUI/TopPanel.xml","<eui__/>");
    auto stock=std::make_shared<assets::LooseResourceIndex>(root);stock->add_file(root/"Stock/TopPanel.xml");
    auto override=std::make_shared<assets::LooseResourceIndex>(root);override->add_file(root/"EUI/TopPanel.xml");
    assets::ResourceMountTable table(assets::resource_lookup(stock),assets::resource_lookup_all(stock));
    table.mount({42,0,0,assets::resource_lookup(override),true,true,assets::resource_lookup_all(override),{}});
    const assets::AssetKey key("TopPanel.xml");
    const auto original=table.resolve(key);
    check(original.mount_id==42,"Fixture does not exercise preferred mount");
    graphics::BnwUiSourceObservation observation;int calls=0;
    {
        assets::ScopedResourceResolveObserver observer(table,[&](const auto &requested,auto hits,bool all) {
            ++calls;observation.observe(requested,hits,all);
            // A nested lookup must keep its result and suppress observer recursion.
            check(table.resolve(key).mount_id==42,"Nested resolve changed mount identity");
        });
        const auto selected=table.resolve(key);
        check(selected.mount_id==original.mount_id && std::get<assets::AssetHandle>(selected.asset).candidate().original_name=="EUI/TopPanel.xml","Instrumentation changed selected source");
        const auto all=table.resolve_all(key);
        check(all.size()==2&&!all[0].mount_id&&all[1].mount_id==42,"Instrumentation changed resolve_all order/identity");
        auto other=table;other.resolve(key);
        std::thread worker([&] {table.resolve(key);});worker.join();
        check(calls==2,"Observer leaked into other table/thread or recursed");
        check(!observer.failed(),"Successful observer marked failed");
    }
    table.resolve(key);check(calls==2,"Expired observer was invoked");
    const auto json=observation.snapshot_json();
    check(json.find("\"mount_id\":42")!=std::string::npos&&json.find("EUI/TopPanel.xml")!=std::string::npos&&json.find("\"operation\":\"resolve_all\"")!=std::string::npos,"Actual source receipt lost selection provenance");
    check(json.find("\"immutable_recheck_passed\":true")!=std::string::npos,"Unmodified source failed digest recheck");
    observation.stop_recording();
    {
        assets::ScopedResourceResolveObserver frozen(table,[&](const auto &k,auto hits,bool all){observation.observe(k,hits,all);});
        table.resolve(key);table.resolve_all(key);table.resolve(assets::AssetKey("after-publication.xml"));
    }
    check(observation.snapshot_json()==json,"One-shot source receipt kept accumulating after publication");
    write(root/"EUI/TopPanel.xml","<edit_/>");
    check(observation.snapshot_json().find("\"immutable_recheck_passed\":false")!=std::string::npos,"Same-length overwritten source became a new baseline");
    observation.mark_observer_failed();
    check(observation.snapshot_json().find("\"observer_failed\":true")!=std::string::npos,"Frozen source receipt lost sticky failure state");
    {
        assets::ScopedResourceResolveObserver throwing(table,[](const auto &,auto,bool) {throw std::runtime_error("observer failure");});
        check(table.resolve(key).mount_id==42&&throwing.failed(),"Observer exception changed resolution or was hidden from proof");
    }
    int dead_calls=0;
    auto expired=std::make_unique<assets::ScopedResourceResolveObserver>(table,[&](const auto &,auto,bool){++dead_calls;});
    std::thread destroy([scope=std::move(expired)]() mutable {scope.reset();});destroy.join();
    table.resolve(key);check(dead_calls==0,"Observer destroyed on another thread remained active");
    graphics::BnwUiSourceObservation missing;
    assets::ScopedResourceResolveObserver recorder(table,[&](const auto &k,auto hits,bool all){missing.observe(k,hits,all);});
    table.resolve(assets::AssetKey("missing.xml"));
    check(missing.snapshot_json().find("\"status\":\"missing\"")!=std::string::npos,"Missing lookup was hidden");
    auto ambiguous=std::get<assets::AssetHandle>(table.resolve(key).asset).candidate();
    assets::ResourceMountTable ambiguous_table([&](const assets::AssetKey &k) {return assets::ResourceMountHit{assets::AmbiguousAsset{k,ambiguous.provenance,{ambiguous,ambiguous}},true};});
    graphics::BnwUiSourceObservation ambiguity;
    assets::ScopedResourceResolveObserver ambiguity_recorder(ambiguous_table,[&](const auto &k,auto hits,bool all){ambiguity.observe(k,hits,all);});
    check(std::holds_alternative<assets::AmbiguousAsset>(ambiguous_table.resolve(key).asset),"Ambiguity changed under observation");
    check(ambiguity.snapshot_json().find("\"status\":\"ambiguous\"")!=std::string::npos,"Ambiguous candidate proof was hidden");
    missing.mark_observer_failed();
    check(missing.snapshot_json().find("\"observer_failed\":true")!=std::string::npos,"Observer failure was not sticky");
    write(root/"observation.json",json);
}
}
int main(int argc,char **argv) {try {
    const auto base=argc==2?std::filesystem::path(argv[1]):std::filesystem::current_path();
    run(base/("synthetic-ui-sources-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
    std::cout<<"PASS actual UI source observation\n";return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
