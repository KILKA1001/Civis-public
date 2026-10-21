#include "civ5/ui/detail/bnw_native_label_field_storage.hpp"
#include "civ5/ui/detail/bnw_native_label_xml_prefix_guard.hpp"
#include <atomic>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace civ5::ui::detail;
void expect(bool ok,const char*s){if(!ok)throw std::runtime_error(s);}
template<class F> void reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}expect(rejected,"Expected rejection");}
auto identity(){return std::make_shared<const int>(0);}
BnwLabelFieldBinding binding(std::function<bool()>current=[](){return true;}){return {identity(),identity(),identity(),17,"fixture/SAME-right-tree/Label",std::move(current)};}
BnwLabelFontFieldBinding font(std::function<bool()> current=[](){return true;}){
    return {identity(),std::string(64,'a'),std::move(current),[](std::uint8_t s){return std::optional<std::array<std::int32_t,3>>{{s,-1,7}};}};
}
int main(){
    // These are independently byte-pinned constructor/setter state fixtures;
    // callback tokens are local fixtures, not an actual native Label producer.
    BnwNativeLabelFieldStorage label(binding());auto ctor=label.snapshot();
    expect(!ctor.fields().text&&!ctor.fields().font&&!ctor.fields().extended&&!ctor.fields().layer_indices,"764170 null fields");
    expect(ctor.fields().style==0&&ctor.fields().wrap==-1&&ctor.fields().truncate==-1&&ctor.fields().leading==0,"764170 signed constructor fields");
    for(auto rgba:ctor.fields().colors)for(float v:rgba)expect(std::bit_cast<std::uint32_t>(v)==0x3f800000,"540EB0 9D144C color constant");
    auto style_no_font=label.set_style(9);expect(!style_no_font.text_update_called&&label.snapshot().fields().style==0,"686AE0 no font no-op");
    std::string text="ASCII + \xd0\xaf [raw markup]";label.set_text(text);text.assign("changed");auto first=label.snapshot();
    expect(first.fields().text=="ASCII + \xd0\xaf [raw markup]"&&!ctor.current(),"SetText owns complete raw text and retires snapshot");
    label.set_text(*first.fields().text);expect(!first.current(),"Identical source text retires old Civis revision");
    std::string aliased_text="before-source-callback";bool alter_input=false;
    BnwNativeLabelFieldStorage alias_owner(binding([&]{if(alter_input)aliased_text="after-source-callback";return true;}));
    alter_input=true;alias_owner.set_text(aliased_text);
    expect(alias_owner.snapshot().fields().text=="before-source-callback"&&aliased_text=="after-source-callback","SetText argument copied before source callbacks");
    label.set_text(std::nullopt);expect(label.snapshot().fields().text=="","611930 null normalizes allocated empty text");
    reject([&]{label.set_text(std::string(0x4001,'a'));});reject([&]{label.set_text(std::string("a\0b",3));});
    label.set_text(std::string(0x4000,'a'));expect(label.snapshot().fields().text->size()==0x4000,"Source SetText bound differs from410900 bound");
    auto f=font();auto ext=identity();int selected=0;
    auto select=[&](const BnwLabelFontFieldBinding&,bool,bool){++selected;return std::optional<BnwLabelExtendedFieldBinding>{{ext,[]{return true;}}};};
    label.set_font(f,select);expect(selected==1&&label.snapshot().fields().extended->owner==ext,"6F3CD0 current font supplier");
    label.set_font(f,select);expect(selected==1,"Same +CC pointer must not reselect +170");
    label.set_font(std::nullopt,select);expect(selected==2&&label.snapshot().fields().font->authored==f.authored,"NULL incoming preserves +CC but reselects +170");
    label.xml_force_ime(true);label.xml_force_non_ime(true);label.xml_force_ime(false);
    expect(label.snapshot().fields().force_ime&&label.snapshot().fields().force_non_ime,"5E3A90 force flags OR only, false XML input does not clear");
    label.set_font(std::nullopt,[](const auto&,bool ime,bool non_ime){expect(ime&&non_ime,"Actual force flags passed to SAME policy supplier");return std::optional<BnwLabelExtendedFieldBinding>{};});
    label.set_style(2);expect(label.snapshot().fields().layer_indices==std::array<std::int32_t,3>{2,-1,7},"Same current authored style indices survive negative layer");
    BnwLabelColorFieldBinding color{{{{2,-1,0,.7f},{1,0,0,.5f},{0,1,0,0}}},identity(),[]{return true;}};
    label.set_colors(color);label.xml_wrap(31);expect(label.snapshot().fields().truncate==-1&&label.snapshot().fields().wrap==31,"XML wrap retires truncate");
    label.xml_truncate(8);label.xml_leading(-3);auto before_reset=label.snapshot();label.reset();auto reset=label.snapshot();
    expect(!before_reset.current()&&!reset.fields().text&&!reset.fields().font&&reset.fields().wrap==-1&&reset.fields().leading==0,"64C8C0 field retirement");
    expect(reset.fields().style==2&&reset.fields().colors==color.values&&reset.fields().color_override,"Reset preserves source style/colors, not constructor replay");
    expect(!reset.fields().native_initialized&&!reset.fields().parent_callbacks_complete&&!reset.fields().native_label_ready,"No initialization authority");
    // Source5E3AC4 OR02000000 is separate from unavailable actualContext+3C.
    BnwNativeLabelFieldStorage read_receiver(binding());auto before_read=read_receiver.snapshot();
    auto read_transition=read_receiver.xml_read_entry();auto after_read=read_receiver.snapshot();
    expect(!before_read.current()&&after_read.current()&&after_read.fields().subtype_read_entered,"XML source OR retires the SAME constructor occurrence");
    expect(!read_transition.text_update_called&&!read_transition.text_update_skipped_uninitialized&&!read_transition.native_vm_executed&&!after_read.fields().font&&!after_read.fields().text,"Read entry cannot invent later Font/String/TextUpdate");
    read_receiver.reset();expect(read_receiver.snapshot().fields().subtype_read_entered,"64C8C0 must preserve02000000 read bit");
    bool poison_read=false;int read_checks=0;
    BnwNativeLabelFieldStorage changed_during_read(binding([&]{return !poison_read||++read_checks<2;}));
    auto before_poison=changed_during_read.snapshot();poison_read=true;
    reject([&]{changed_during_read.xml_read_entry();});expect(!before_poison.current(),"Post-mutation source failure cannot retain preceding XML occurrence");
    reject([&]{changed_during_read.snapshot();});
    // SAME prefix guard is shared across graph facades. Query callbacks run
    // before field mutation, so cell-level guards alone cannot catch reentry.
    auto prefix_guard=std::make_shared<BnwNativeLabelXmlPrefixGuard>();auto alternate_fork_guard=prefix_guard;
    BnwNativeLabelFieldStorage prefix_fields(binding([prefix_guard]{return prefix_guard->healthy();}));auto pre_callback=prefix_fields.snapshot();
    {auto scope=prefix_guard->enter();reject([&]{auto nested=alternate_fork_guard->enter();});reject([&]{scope.check();});}
    expect(!pre_callback.current(),"Caught alternate-fork recursion cannot retain field authorization before mutation");
    reject([&]{auto scope=prefix_guard->enter();});
    auto thread_guard=std::make_shared<BnwNativeLabelXmlPrefixGuard>();{auto scope=thread_guard->enter();std::atomic<bool> denied{false};std::thread wrong([&]{reject([&]{auto nested=thread_guard->enter();});denied=!thread_guard->healthy();});wrong.join();expect(denied,"Foreign prefix query must refuse");scope.check();}
    expect(thread_guard->healthy(),"Foreign query poisoned actual thread authorization");
    std::weak_ptr<BnwNativeLabelXmlPrefixGuard> disappearing_guard=thread_guard;
    {auto scope=thread_guard->enter();thread_guard.reset();expect(disappearing_guard.expired(),"Guard scope invented facade lifetime");scope.check();}
    auto published_guard=std::make_shared<BnwNativeLabelXmlPrefixGuard>();BnwNativeLabelFieldStorage publishing(binding([published_guard]{return published_guard->healthy();}));auto pre_publish=publishing.snapshot();
    {auto scope=published_guard->enter();publishing.xml_read_entry();auto candidate=publishing.snapshot();reject([&]{auto nested=published_guard->enter();});expect(!candidate.current()&&!pre_publish.current(),"Caught recursion after mutation retires immutable candidate");reject([&]{scope.check();});}
    // Actual graph catch closes the SAME cell when final source validation or
    // publication throws. Exercise that storage retirement without fake init.
    auto uncertain=std::make_shared<BnwNativeLabelFieldStorage>(binding());auto uncertain_before=uncertain->snapshot();
    reject([&]{try{uncertain->xml_read_entry();throw std::bad_alloc();}catch(...){uncertain->close();throw;}});
    expect(!uncertain_before.current(),"Publication allocation throw retained preceding occurrence");reject([&]{uncertain->snapshot();});
    bool base_current=true;BnwNativeLabelFieldStorage other(binding([&]{return base_current;}));auto old=other.snapshot();base_current=false;
    expect(!old.current(),"SAME base/parent/source changed");reject([&]{other.set_text("new");});
    bool font_current=true;BnwNativeLabelFieldStorage current_font(binding());current_font.set_font(font([&]{return font_current;}),[](const auto&,bool,bool){return std::optional<BnwLabelExtendedFieldBinding>{};});auto font_frame=current_font.snapshot();font_current=false;
    expect(!font_frame.current(),"Authored font owner mutation invalidates fields");
    bool base_alive=true,mutate_base=false;
    BnwNativeLabelFieldStorage dependent(binding([&]{return base_alive;}));
    auto base_changing_font=font([&]{if(mutate_base)base_alive=false;return true;});
    dependent.set_font(base_changing_font,[](const auto&,bool,bool){return std::optional<BnwLabelExtendedFieldBinding>{};});
    auto base_frame=dependent.snapshot();mutate_base=true;
    expect(!base_frame.current(),"Returning-true font callback cannot erase changed original base fence");
    BnwNativeLabelFieldStorage missing_selector(binding());reject([&]{missing_selector.set_font(font(),{});});reject([&]{missing_selector.snapshot();});
    BnwNativeLabelFieldStorage*recursive=nullptr;bool recurse=false;
    BnwNativeLabelFieldStorage recursive_owner(binding([&]{if(recurse){try{recursive->set_text("nested");}catch(const std::exception&){} }return true;}));recursive=&recursive_owner;auto preceding=recursive_owner.snapshot();recurse=true;
    reject([&]{recursive_owner.set_text("outer");});expect(!preceding.current(),"Caught recursive callback latches failure/retirement");
    auto lifetime=std::make_unique<BnwNativeLabelFieldStorage>(binding());auto retained=lifetime->snapshot();lifetime.reset();expect(!retained.current()&&!retained.fields().text,"Snapshot owns fields but not dead authority");
    struct GraphFixture {
        // SAME source factory inventory; graph owns field components only.
        std::shared_ptr<std::array<int,2>> right=std::make_shared<std::array<int,2>>();
        std::shared_ptr<const int> token=identity();
        std::shared_ptr<BnwNativeLabelFieldStorage> label;
    };
    auto graph=std::make_shared<GraphFixture>();auto retained_right=graph->right;
    graph->label=std::make_shared<BnwNativeLabelFieldStorage>(BnwLabelFieldBinding{
        graph->token,std::shared_ptr<const void>(retained_right,&(*retained_right)[1]),
        std::shared_ptr<const void>(retained_right,&(*retained_right)[0]),1,"fixture/right/Label",[retained_right]{return static_cast<bool>(retained_right);}});
    std::weak_ptr<GraphFixture> weak_graph=graph;auto graph_snapshot=graph->label->snapshot();
    auto fork=std::make_shared<GraphFixture>(*graph);graph.reset();
    expect(graph_snapshot.identity().control.get()==&(*retained_right)[1]&&graph_snapshot.identity().parent.get()==&(*retained_right)[0],"Source identities alias SAME inventory records, not actual native AddChild");
    expect(weak_graph.expired()&&graph_snapshot.current(),"Snapshots do not retain graph; SAME fork retains SAME field cell");
    fork.reset();expect(!graph_snapshot.current()&&retained_right,"Final graph teardown retires fields although source records still live");
    bool drop_owner=false;std::unique_ptr<BnwNativeLabelFieldStorage> disappearing;
    disappearing=std::make_unique<BnwNativeLabelFieldStorage>(binding([&]{if(drop_owner)disappearing.reset();return true;}));
    auto dying_snapshot=disappearing->snapshot();auto*dying_receiver=disappearing.get();drop_owner=true;
    reject([&]{dying_receiver->set_text("owned-before-death");});
    expect(!disappearing&&!dying_snapshot.current(),"Last owner deletion inside callback has retained private cell and fails closed");
    BnwNativeLabelFieldStorage thread_owner(binding());auto thread_frame=thread_owner.snapshot();std::atomic<bool> foreign_rejected{false};
    std::thread foreign([&]{foreign_rejected=!thread_frame.current();reject([&]{thread_owner.set_style(1);});});foreign.join();expect(foreign_rejected&&thread_frame.current(),"Foreign thread cannot mutate or poison same-thread owner");
    thread_owner.close();expect(!thread_frame.current(),"Explicit close retires occurrence");
    std::cout<<"PASS: source field fixtures, raw text/font/style/reset, immutable revisions, source/lifetime/thread/reentry; nativeLabelReady=false\n";
}
