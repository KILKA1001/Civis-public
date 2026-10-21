#pragma once
#include "civ5/content/manifest.hpp"
#include "bnw_native_color_load_kernel.hpp"
#include "bnw_native_mounted_style_reader.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <type_traits>

namespace civ5::ui::detail {
class BnwNativeGlobalColorRegistryBank;
// Private field component of the SAME registry bank. No public issuer accepts
// copied maps, caller readiness bits or an XML inventory as native authority.
class BnwNativeColorSet64Storage final {
    friend class BnwNativeGlobalColorRegistryBank;
    struct Value {
        native_color_load_detail::ColorSetBytes bytes{};
        std::array<bool,64> known{};
    };
    using Current=std::function<void(const content::XmlNode*,std::uint64_t)>;
    using LookupColor=std::function<std::optional<native_color_load_detail::ColorBits>(std::uint32_t)>;
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        Current current;
        std::map<std::uint32_t,Value> entries;
        std::set<std::uint64_t> loads;
        std::atomic<bool> closed{false},failed{false},busy{false};
        explicit State(Current guard):current(std::move(guard)){}
        static void need(bool value,const char* text){if(!value)throw std::runtime_error(text);}
        struct Scope {
            std::shared_ptr<State> state;
            explicit Scope(std::shared_ptr<State> actual):state(std::move(actual)) {
                need(std::this_thread::get_id()==state->thread,"ColorSet64 foreign thread");
                need(!state->closed.load()&&!state->failed.load(),"ColorSet64 retired");
                if(state->busy.exchange(true)) {
                    state->failed.store(true);throw std::runtime_error("ColorSet64 reentry");
                }
            }
            ~Scope(){state->busy.store(false);}
            Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
        };
        void require_current(const content::XmlNode* document,std::uint64_t occurrence) {
            need(!closed.load()&&!failed.load(),"ColorSet64 source retired during callback");
            try {
                const auto guard=current;
                need(bool(guard),"ColorSet64 actual bank/source lease absent");
                guard(document,occurrence);
                need(!closed.load()&&!failed.load(),"ColorSet64 bank ownership lost during callback");
            } catch(...) {failed.store(true);throw;}
        }
    };
    std::shared_ptr<State> state_;
    static Value constructor_value() {
        Value value;native_color_load_detail::default_color_set(value.bytes);
        for(std::size_t n=0;n!=63;++n)value.known[n]=true;
        return value; //540EB0 leaves byte3F unknown, never a supplied zero.
    }
    static void known(Value& value,std::size_t at,std::size_t count) {
        for(std::size_t n=at;n!=at+count;++n)value.known[n]=true;
    }
    static void canonical(const std::string& text) {
        for(unsigned char c:text)
            State::need(c>=0x20&&c<=0x7e,"ColorSet64 FString source conversion unsupported");
    }
    static void component(const std::shared_ptr<State>& state,Value& value,
            unsigned slot,std::string_view input,const LookupColor& lookup,
            const content::XmlNode* document,std::uint64_t occurrence) {
        using namespace native_color_load_detail;
        auto text=trim(input);if(text.empty())return;
        Tokens cursor{text};const auto first=cursor.take();
        if(!first.empty()) {
            const auto key=hash_query(first);
            state->require_current(document,occurrence);
            State::need(bool(lookup),"ColorSet64 normal5307B0 actual lookup supplier absent");
            const auto found=lookup(key); //Normal hash lookup, not special55E2B0 player route.
            state->require_current(document,occurrence);
            if(found) {
                component_named_value(value.bytes,slot,*found,key);
                known(value,slot*16,16);known(value,0x30+slot*4,4);known(value,0x3c+slot,1);
                if(cursor.next!=std::string_view::npos) {
                    component_named_alpha(value.bytes,slot,cursor.take());
                    known(value,slot*16+12,4);known(value,0x3c+slot,1);
                }
                return;
            }
        }
        //Only genuine normal-lookup MISS reaches numeric fallback. An empty
        //first token skips lookup exactly as76D3C0 does, not arbitrary absence.
        component_literal(value.bytes,slot,text);
        known(value,0x30+slot*4,4);known(value,0x3c+slot,1);known(value,slot*16,16);
    }
    //Only the shared bank can construct this field component. Its guard must
    //bind SAME actual bank epoch/loan/source roster and exact captured XML
    //occurrence; null/0 identifies a storage lookup, never missing source.
    explicit BnwNativeColorSet64Storage(Current current)
        :state_(std::make_shared<State>(std::move(current))) {
        auto state=state_;State::Scope scope(state);state->require_current(nullptr,0);
    }
    //601CA0 Color phase belongs to the SAME bank and must precede this method.
    //Each call consumes its actual registered source occurrence once. No XML
    //parser, Package selection, enabled-slot admission or global-stack grant
    //is made by this private field component.
    void read_from_same_xml(std::shared_ptr<const content::XmlNode> document,
            std::uint64_t occurrence,LookupColor normal_lookup) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(document!=nullptr,"ColorSet64 retained XML absent");
            state->require_current(document.get(),occurrence);
            State::need(state->loads.insert(occurrence).second,"ColorSet64 source occurrence cannot replay");
            for(const auto& node:document->children) {
                if(node.name!="ColorSet")continue;
                std::set<std::string> names;
                const std::string* name=nullptr;
                for(const auto& [key,text]:node.attributes) {
                    State::need(names.insert(key).second,"ColorSet64 duplicate XML conversion unsupported");
                    if(key=="Name")name=&text;
                }
                State::need(name&&!name->empty(),"ColorSet64 Name conversion unavailable");
                canonical(*name);
                const auto hash=native_color_load_detail::hash_length(*name);
                if(state->entries.contains(hash))continue; //601CA0 first hash wins.
                auto value=constructor_value();
                for(unsigned slot=0;slot!=4;++slot) {
                    const auto key=std::string("Color")+std::to_string(slot);
                    for(const auto& [attribute,text]:node.attributes)if(attribute==key) {
                        canonical(text);
                        component(state,value,slot,text,normal_lookup,document.get(),occurrence);
                    }
                }
                state->require_current(document.get(),occurrence);
                state->entries.emplace(hash,std::move(value)); //After all four operands.
            }
            state->require_current(document.get(),occurrence);
        } catch(...) {state->failed.store(true);state->closed.store(true);throw;}
    }
    //Only Bank can call this branch after its actual601CA0 Color pass, with
    //the SAME private reader and its persistent Name/value FString locals.
    //No content::XmlNode projection or arbitrary attribute map is created.
    void read_from_same_owned_reader(const std::shared_ptr<BnwNativeMountedStyleReader>& reader,
        std::uint64_t occurrence,const std::shared_ptr<BnwNativeMountedStyleReader::StringLocal>& name_local,
        const std::shared_ptr<BnwNativeMountedStyleReader::StringLocal>& value_local,LookupColor normal_lookup) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(reader&&name_local&&value_local,"ColorSet actual reader/locals absent");
            const auto root=reader->first_same_owned_element();
            const auto document=root.document.lock();State::need(bool(document),"ColorSet actual parsed root anchor retired");
            const auto check=[&]{document->require_current();state->require_current(nullptr,0);document->require_current();};
            check();State::need(state->loads.insert(occurrence).second,"ColorSet actual selected load replay");
            for(const auto& node:root.same_matching_children_835660_8356c0("ColorSet")) {
                check();
                const auto name=node.first_same_attribute_value_8355c0("Name");
                const auto copied=name.consume_same_string_local(name_local);
                State::need(copied!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                            "ColorSet Name needs unavailable native adjacent bytes");
                //601CA0 ignores missing/failed Name conversion and hashes the
                //actual persistent local, including its retained prior value.
                const auto text=reader->copied_same_string_local(name_local);
                const auto hash=native_color_load_detail::hash_length(text);check();
                if(state->entries.contains(hash))continue;
                auto value=constructor_value();
                for(unsigned slot=0;slot<4;++slot) {
                    const auto attribute=node.first_same_attribute_value_8355c0(std::string("Color")+std::to_string(slot));
                    const auto converted=attribute.consume_same_string_local(value_local);
                    State::need(converted!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                                "ColorSet component needs unavailable native adjacent bytes");
                    if(converted!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::native_true)continue;
                    const auto input=reader->copied_same_string_local(value_local);
                    //Supported native arithmetic kernel requires a bounded
                    //ASCII FString without embedded NUL; do not guess opaque
                    //allocation/cursor semantics for unsupported inputs.
                    canonical(input);check();
                    component(state,value,slot,input,normal_lookup,nullptr,0);check();
                }
                check();state->entries.emplace(hash,std::move(value));check();
            }
            check();
        }catch(...){state->failed.store(true);state->closed.store(true);throw;}
    }
    //717440 mutates SAME stored ColorSet values after the actual bank's
    //source-scheduled Color writers.462C20 invokes this for each retainedSkin
    //slot in reverse stack order, then global. No enabled/final-ready token is
    //issued by this field method and the bank's actual source scheduler owns
    //that complete epoch; Directory/Paks/Style/priority effects remain needed.
    void refresh_named_colors(LookupColor normal_lookup) {
        auto state=state_;State::Scope scope(state);
        try {
            state->require_current(nullptr,0);
            for(auto& [hash,value]:state->entries) {
                for(unsigned slot=0;slot!=3;++slot) { //Never a fourth refresh.
                    const auto key=native_color_load_detail::read_word(value.bytes,0x30+slot*4);
                    if(key==0)continue;
                    const auto saved_alpha=native_color_load_detail::read_word(value.bytes,slot*16+12);
                    std::array<bool,4> saved_alpha_known{};
                    for(unsigned n=0;n!=4;++n)saved_alpha_known[n]=value.known[slot*16+12+n];
                    state->require_current(nullptr,0);
                    State::need(bool(normal_lookup),"ColorSet64 final717440 actual normal5307B0 supplier absent");
                    const auto found=normal_lookup(key);
                    state->require_current(nullptr,0);
                    //Native lookup writes only on success.717440 ignores its
                    //bool result; a genuine miss preserves previous RGBA.
                    if(found) {
                        for(unsigned channel=0;channel!=4;++channel)
                            native_color_load_detail::write_word(value.bytes,slot*16+channel*4,(*found)[channel]);
                        known(value,slot*16,16);
                    }
                    if(value.bytes[0x3c+slot]==3) {
                        native_color_load_detail::write_word(value.bytes,slot*16+12,saved_alpha);
                        for(unsigned n=0;n!=4;++n)value.known[slot*16+12+n]=saved_alpha_known[n];
                    }
                    state->require_current(nullptr,0);
                }
            }
            state->require_current(nullptr,0);
        } catch(...) {state->failed.store(true);state->closed.store(true);throw;}
    }
    std::optional<Value> lookup_key(std::uint32_t key)const {
        auto state=state_;State::Scope scope(state);
        state->require_current(nullptr,0);
        const auto found=state->entries.find(key);
        std::optional<Value> result;
        if(found!=state->entries.end())result=found->second; //Full64bytes+unknown mask.
        state->require_current(nullptr,0);
        return result;
    }
public:
    ~BnwNativeColorSet64Storage(){if(state_)state_->closed.store(true);}
    BnwNativeColorSet64Storage(const BnwNativeColorSet64Storage&)=delete;
    BnwNativeColorSet64Storage&operator=(const BnwNativeColorSet64Storage&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativeColorSet64Storage>);
static_assert(!std::is_copy_constructible_v<BnwNativeColorSet64Storage>);
}
