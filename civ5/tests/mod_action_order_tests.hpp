#pragma once
void mod_action_order_tests() {
    using namespace civ5::content;
    Catalog catalog;
    const auto add = [&](const std::string &id, const std::string &relations = "", std::uint64_t version = 1) {
        PackageManifest p{}; p.kind=PackageKind::mod; p.id=id; p.version=version;
        p.document=read_xml_metadata("<Mod>"+relations+"</Mod>");
        catalog.packages.push_back(std::move(p));
    };
    const auto dependencies = [](const std::string &id) { return "<Dependencies><Mod id='"+id+"'/></Dependencies>"; };
    add("core"); add("expansion",dependencies("core")); add("independent");
    std::vector<std::size_t> active{0,1,2};
    check(order_mod_actions(active,catalog)==std::vector<std::size_t>{0,2,1},
          "Retail mod actions visit independent roots before dependent expansion");
    add("addon",dependencies("expansion")); active.push_back(3);
    check(order_mod_actions(active,catalog)==std::vector<std::size_t>{0,2,1,3},
          "Transitive relationship counters emit referenced expansion before fallback addon");
    catalog.packages[2].document=read_xml_metadata("<Mod><References><Mod id='EXPANSION'/></References></Mod>");
    check(order_mod_actions(active,catalog)==std::vector<std::size_t>{0,1,2,3},
          "Selected optional references participate case insensitively in action order");
    catalog.packages[2].document=read_xml_metadata("<Mod><References><Mod id='absent'/><Mod id='expansion' minversion='2'/><Mod id='core' maxversion='0'/></References></Mod>");
    check(order_mod_actions(active,catalog)==std::vector<std::size_t>{0,2,1,3},
          "Unselected and out-of-range references do not add content or ordering edges");
    catalog.packages[0].document=read_xml_metadata("<Mod>"+dependencies("addon")+"</Mod>");
    rejects([&]{order_mod_actions(active,catalog);});
    catalog.packages[0].document=read_xml_metadata("<Mod/>");
    rejects([&]{order_mod_actions(std::vector<std::size_t>{0,0},catalog);});
    Catalog wide;
    std::swap(catalog,wide);
    const auto prefix=std::string(130,'x');
    add(prefix+"a","",4294967300ULL);add(prefix+"b");
    add("wide-reference","<References><Mod id='"+prefix+"a' minversion='4294967300' maxversion='4294967300'/></References>");
    check(order_mod_actions(std::vector<std::size_t>{2,0,1},catalog)==std::vector<std::size_t>{0,1,2},
          "Full mod IDs and versions remain distinct beyond native 64-byte/int32 limits");
    // This asymmetric graph exercises roots, the count scan and stable fallback.
    catalog.packages.clear();
    add("a",dependencies("b"));add("b",dependencies("c"));add("c");add("d");
    check(order_mod_actions(std::vector<std::size_t>{0,1,2,3},catalog)==std::vector<std::size_t>{2,3,1,0},
          "Retail action sorter preserves roots, single count scan, and stable remaining mods");
    catalog.packages[0].document=read_xml_metadata("<Mod><Dependencies><Mod id='b'/><Mod id='b'/></Dependencies><References><Mod id='b'/></References></Mod>");
    check(order_mod_actions(std::vector<std::size_t>{0,1,2,3},catalog)==std::vector<std::size_t>{2,3,1,0},
          "Repeated dependency and reference records retain native closure and adjacent deduplication");
    catalog.packages.clear();
    for(std::size_t i=0;i<260;++i)add("deep-"+std::to_string(i),i?dependencies("deep-"+std::to_string(i-1)):"");
    active.clear();for(std::size_t i=260;i-->0;)active.push_back(i);
    const auto ordered=order_mod_actions(active,catalog);
    std::vector<std::size_t> expected;
    for(std::size_t i=0;i<130;++i)expected.push_back(i);
    for(std::size_t i=260;i-->130;)expected.push_back(i);
    check(ordered==expected,
          "Deep native count scan retains its unsorted fallback and uses a host-owned stack");
}
