#pragma once
#include "bnw_native_package_xml_parser_owner.hpp"
#include "bnw_native_package_key_source.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include <sstream>
#include <string>

namespace civ5::ui::detail {
//738F00 actual core field writer on SAME privately parsed fresh descriptor.
//Standalone Name/Description consumes SAME real early selected-language input.
//No native ABI/Steam parity or public Loaded/fallback string is asserted.
class BnwNativePackageDescriptorCoreWriter final {
    friend class BnwNativeInitialDescriptorSource;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativeStandalonePackageLoaded;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeGlobalResourceWarmOwner;
    static void need(bool value,const char* error){if(!value)throw std::runtime_error(error);}
    using Parser=BnwNativePackageXmlParserOwner;
    using SelectedInput=runtime::BnwSelectedLanguageInputOwner;
    using Node=Parser::Node;
    using Doc=Parser::Document;
    struct PropertyDefinition {std::string name;std::uint32_t type;bool required;};
    struct Metadata {std::string name,description;};
    struct Fields {
        //6F7D30 actual defaults. An absent GUID remains UNWRITTEN; no zero GUID
        //is manufactured for the native constructor's unspecified +0C bytes.
        std::optional<std::array<std::uint8_t,16>> written_guid;
        std::vector<std::uint32_t> steam_apps;
        const std::uint32_t category_mask=0xffffffffU; //Read-only constructor provenance, NOT current activation.
      private:
        friend class BnwNativeGlobalResourceWarmOwner;
        const std::thread::id activation_creator_thread_=std::this_thread::get_id();
        mutable std::uint32_t live_category_mask_=0xffffffffU; //Actual ctor +48 value, genuine actor writes later.
        // The const Core owns constructor provenance below. Only the genuine
        // installed startup writer may evolve actual +44; const loans cannot.
        mutable std::optional<std::uint32_t> startup_live_flags_;
      public:
        std::uint32_t current_flags()const {need(activation_creator_thread_==std::this_thread::get_id(),
            "Actual package flags getter foreign creator thread");return startup_live_flags_.value_or(flags);}
        std::uint32_t current_category_mask()const {need(activation_creator_thread_==std::this_thread::get_id(),
            "Actual activation getter foreign creator thread");return live_category_mask_;}
        std::uint32_t version=1,flags=4; //Read-only constructor provenance after Core publication.
        std::int32_t priority=0;
        std::vector<PropertyDefinition> definitions;
    };
    static std::string_view range(const Doc& doc,Parser::Range value) {
        need(value.begin<=doc.decoded.size()&&value.size<=doc.decoded.size()-value.begin,
            "Package writer node range escaped SAME owned Buffer");
        return {reinterpret_cast<const char*>(doc.decoded.data()+value.begin),value.size};
    }
    static std::string_view c_prefix(std::string_view value) {
        const auto nul=value.find('\0');return value.substr(0,nul);
    }
    static bool icase(std::string_view left,std::string_view right,const SelectedInput* actual=nullptr) {
        if(actual)return actual->platform_ascii_compare(left,right)==0;
        left=c_prefix(left);right=c_prefix(right);if(left.size()!=right.size())return false;
        //All835330 successful value inputs are ASCII. This models the native
        //_stricmp branch without introducing a foreign Unicode/culture source.
        for(std::size_t i=0;i<left.size();++i) {
            auto a=static_cast<unsigned char>(left[i]),b=static_cast<unsigned char>(right[i]);
            if(a>='A'&&a<='Z')a+='a'-'A';if(b>='A'&&b<='Z')b+='a'-'A';if(a!=b)return false;
            need(left[i]==right[i]||a!='i',"Native writer I/i comparison lacks actual current C-locale source");
        }
        return true;
    }
    static const Node* child(const Doc& doc,const Node& parent,std::string_view name) {
        for(const auto& item:parent.children)if(range(doc,item->name)==name)return item.get();return nullptr;
    }
    static std::optional<std::string_view> ascii_value(const Doc& doc,const Node& node) {
        const auto value=node.value?range(doc,*node.value):std::string_view{};
        for(unsigned char byte:value)if(byte&128)return {}; //Actual835330 ASCII gate.
        return value;
    }
    static std::optional<std::string_view> attribute(const Doc& doc,const Node& node,std::string_view name) {
        for(const auto& item:node.attributes)if(range(doc,item.name)==name) {
            const auto value=range(doc,item.value);for(unsigned char byte:value)if(byte&128)return {};
            return value;
        }
        return {};
    }
    template<class Integer> static std::optional<Integer> integer(std::string_view text,const SelectedInput* actual=nullptr) {
        //836080/836160 write the exact bounded value into a freshly constructed
        //stringstream then formatted-extract int32/uint32. Trailing data need
        //not be consumed; failbit/badbit, not EOF, decide conversion success.
        std::istringstream stream{std::string(text)};if(actual)stream.imbue(actual->platform_stream_locale());Integer value{};stream>>value;
        if(stream.rdstate()&(std::ios_base::failbit|std::ios_base::badbit))return {};
        return value;
    }
    static int hex(unsigned char byte) {
        if(byte>='0'&&byte<='9')return byte-'0';if(byte>='a'&&byte<='f')return byte-'a'+10;
        if(byte>='A'&&byte<='F')return byte-'A'+10;return -1;
    }
    static bool guid(std::string_view text,std::array<std::uint8_t,16>& result) {
        std::size_t cursor=0;
        auto piece=[&](unsigned count,std::uint32_t& value) {
            while(cursor<text.size()&&(text[cursor]=='{'||text[cursor]=='-'||text[cursor]==' '))++cursor;
            value=0;if(cursor==text.size())return false;
            //Native helpers succeed on a short final piece at EOS; every next
            //piece still needs at least one digit. No invented strict GUID regex.
            do {
                const auto digit=hex(static_cast<unsigned char>(text[cursor]));if(digit<0)return false;
                value=(value<<4)+static_cast<std::uint32_t>(digit);++cursor;
            }while(cursor<text.size()&&--count);
            return true;
        };
        std::uint32_t value=0;if(!piece(8,value))return false;
        for(unsigned i=0;i<4;++i)result[i]=static_cast<std::uint8_t>(value>>(8*i));
        if(!piece(4,value))return false;result[4]=value&255;result[5]=(value>>8)&255;
        if(!piece(4,value))return false;result[6]=value&255;result[7]=(value>>8)&255;
        for(unsigned i=8;i<16;++i){if(!piece(2,value))return false;result[i]=static_cast<std::uint8_t>(value);}
        //4BD4D0 skips trailing '}' but never requires full input consumption.
        return true;
    }
    using KeyBytes=native_package_key_detail::Bytes;
    static void uppercase_file(std::string_view text,KeyBytes& output,const SelectedInput* actual=nullptr) {
        if(actual){const auto value=actual->platform_uppercase_ascii(text);output.insert(output.end(),value.begin(),value.end());return;}
        //891400 mode1 uses current native C-locale toupper, not path lookup.
        //835330 already proved ASCII; i is the locale-dependent ASCII branch.
        for(unsigned char byte:text) {
            need(byte!='i',"Native File/checksum toupper i lacks current C-locale producer");
            if(byte>='a'&&byte<='z')byte-=('a'-'A');output.push_back(byte);
        }
    }
    static bool append_property(const Doc& doc,const Node& node,std::string_view value,
                                std::uint32_t type,KeyBytes& output,const SelectedInput* actual) {
        if(type==0)return native_package_key_detail::append_text(value,output);
        if(type==1)return native_package_key_detail::append_hex(value,output);
        if(type==3)return native_package_key_detail::append_guid(value,output);
        need(type==2,"Actual package property type unknown");
        uppercase_file(value,output,actual);
        if(const auto checksum=attribute(doc,node,"Checksum"))uppercase_file(*checksum,output,actual);
        return true; //891400 path/checksum byte append, not external file reads.
    }
    static bool property_bytes_8916a0(const Doc& doc,const Node& root,const Fields& fields,KeyBytes& output,const SelectedInput* actual) {
        bool last=true;
        for(const auto& definition:fields.definitions) {
            bool processed=false;
            if(!definition.name.empty())for(const auto& node:root.children) {
                if(range(doc,node->name)!=definition.name)continue;
                if(const auto value=ascii_value(doc,*node)) {
                    last=append_property(doc,*node,*value,definition.type,output,actual);
                    if(last)processed=true;
                }
                //Native next-sibling query happens before BL check; private
                //tree traversal has no source callbacks or field writes.
                if(!last)break;
            }
            if(definition.required&&!processed)return false;
            //Later successful selectors overwrite last; never accumulate AND.
        }
        return last;
    }
    static void write_key_flag_641a40(const Doc& doc,const Node& root,Fields& fields,const SelectedInput* actual) {
        const auto* key=child(doc,root,"Key");if(!key)return;
        const auto authored=ascii_value(doc,*key);if(!authored)return;
        KeyBytes plain,decoded_key;
        if(!property_bytes_8916a0(doc,root,fields,plain,actual))return;
        if(!native_package_key_detail::append_hex(*authored,decoded_key))return;
        //Only the previously verified arithmetic functions are reused. No
        //existing content::XmlNode, retained XML witness or manifest is read.
        const auto interleaved=native_package_key_detail::interleave(plain);
        const auto computed=native_package_key_detail::md5(interleaved);
        if(decoded_key.size()==computed.size()&&std::equal(computed.begin(),computed.end(),decoded_key.begin()))
            fields.flags|=1; //641A40 actual +44 success bit; mismatch is NOT load failure.
    }
    static std::unique_ptr<Fields> write_738f00_core(const Doc& doc,const SelectedInput* actual=nullptr) {
        if(doc.roots.empty())return {};
        const auto name=range(doc,doc.roots.front()->name);
        //Actual835090 ->834FF0 obtains virtual8358F0 name pointer/full extent,
        //then checks every extent byte for highbit before _stricmp C-prefix.
        //NonASCII is computed load-false, not a terminal source exception;
        //40CD60 may clean up and continue after this real getter-false.
        for(unsigned char byte:name)if(byte&128)return {};
        if(!icase(name,"Civ5Package",actual))return {};
        const auto& root=*doc.roots.front();auto output=std::make_unique<Fields>();
        if(const auto* id=child(doc,root,"GUID"))if(const auto text=ascii_value(doc,*id)) {
            std::array<std::uint8_t,16> written{};if(!guid(*text,written))return {};
            output->written_guid=written;
        }
        for(const auto& item:root.children)if(range(doc,item->name)=="SteamApp")
            if(const auto text=ascii_value(doc,*item))if(const auto value=integer<std::uint32_t>(*text,actual))
                output->steam_apps.push_back(*value); //639EA0 actual ordered +1C vector.
        if(const auto* node=child(doc,root,"Version"))if(const auto text=ascii_value(doc,*node))
            if(const auto value=integer<std::uint32_t>(*text,actual))output->version=*value;
        if(const auto* node=child(doc,root,"Priority"))if(const auto text=ascii_value(doc,*node))
            if(const auto value=integer<std::int32_t>(*text,actual))output->priority=*value;
        if(const auto* node=child(doc,root,"Ownership"))if(const auto text=ascii_value(doc,*node))
            if(icase(*text,"free",actual))output->flags|=0x10;
        if(const auto* node=child(doc,root,"Upgrade"))if(const auto text=ascii_value(doc,*node))
            if(const auto value=integer<std::uint32_t>(*text,actual))if(*value)output->flags|=0x20;
        output->definitions={{"GUID",3,true},{"SteamApp",0,true},{"Version",0,true}};
        if(const auto* tags=child(doc,root,"PTags"))for(const auto& item:tags->children) {
            if(range(doc,item->name)!="Tag")continue;const auto value=ascii_value(doc,*item);if(!value)continue;
            bool duplicate=false;for(const auto& old:output->definitions)if(icase(old.name,*value,actual)){duplicate=true;break;}
            if(duplicate)continue;
            std::uint32_t type=2;
            if(const auto declaration=attribute(doc,*item,"Type")) {
                if(icase(*declaration,"Text",actual))type=0;else if(icase(*declaration,"Hex",actual))type=1;
                else if(icase(*declaration,"GUID",actual))type=3; //File/unknown remain2.
            }
            output->definitions.push_back({std::string(c_prefix(*value)),type,false});
        }
        //Core includes actual Key flag writes before resource eligibility.
        //Unresolved language effects remain a separate phase, never a full load.
        write_key_flag_641a40(doc,root,*output,actual);
        //Stop at the actual bounded core checkpoint. Unconditional890EC0 /
        //497940 settings/Steam calls and Name/Description writes are not run.
        //No full738F00, Loaded, manager label or Category seal is issued here.
        return output;
    }
    static void localized_field_890ec0(const Doc& doc,const Node& root,std::string_view property,
                                     const SelectedInput& input,std::string& output) {
        //Unconditional selected-language input first, even absent property.
        //Standalone adapter replaces497940 Steam input with SAME actual local
        //profile; its bytes/current locale are source-owned, never a default.
        const auto language=input.language();const auto* node=child(doc,root,property);if(!node)return;
        bool selected=false;
        if(!node->children.empty())for(const auto& item:node->children) {
            if(range(doc,item->name)!="Value")continue;
            const auto declaration=attribute(doc,*item,"language");if(!declaration)continue;
            const bool current=icase(*declaration,language,&input);
            if(!current&&(selected||(!icase(*declaration,"en_US",&input)&&!icase(*declaration,"english",&input))))continue;
            //Actual8352A0 return is ignored here. A failed nonASCII getter
            //keeps the prior FString; successful empty text clears it.
            if(const auto value=ascii_value(doc,*item))output=std::string(*value);
            selected=true; //Later current-language Values can replace prior match.
        }
        if(!selected)if(const auto value=ascii_value(doc,*node))output=std::string(*value);
    }
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<Parser> parser;
        std::weak_ptr<const BnwNativePackageDescriptorCoreWriter> self;
        std::atomic<bool> closed{false},failed{false},busy{false};
        const Doc* document_identity=nullptr;
        std::unique_ptr<const Fields> fields;
        std::unique_ptr<const Metadata> metadata;
        std::optional<std::weak_ptr<const SelectedInput>> actual_settings;
        explicit State(const std::shared_ptr<Parser>& actual,const std::shared_ptr<const SelectedInput>& settings):parser(actual) {if(settings)actual_settings=settings;}
        void live()const {
            need(thread==std::this_thread::get_id()&&!closed.load()&&!failed.load()&&!self.expired(),
                "Package core writer retired/foreign thread");
        }
        void check(const std::shared_ptr<Parser>& actual)const {
            live();need(actual&&actual.use_count()>1&&!parser.owner_before(actual)&&!actual.owner_before(parser),
                "Package core writer lost SAME externally owned parser");
            if(actual_settings) {
                const auto selected=actual_settings->lock();need(selected&&selected.use_count()>1,"Actual selected metadata input retired");
                selected->require_current();live();need(selected.use_count()>1,"Actual selected metadata input lost owner in callback");
            }
        }
        void check_completed(const std::shared_ptr<Parser>& actual,const BnwNativeCompletedColdSourceLoan& loan,
            const BnwNativeColdFilesystemOwner& cold)const {
            live();
            const auto parser_current=[&]{
                need(actual&&actual.use_count()>1&&!parser.owner_before(actual)&&!actual.owner_before(parser),
                    "Completed Core lost same external Parser/control");
            };
            parser_current();loan.require_same_cold(cold);live();parser_current();
            if(actual_settings) {
                const auto selected=actual_settings->lock();
                need(selected&&selected.use_count()>1&&!actual_settings->owner_before(selected)&&!selected.owner_before(*actual_settings),
                    "Completed actual selected input retired/substituted");
                selected->require_current();live();
                // Callback-free check IMMEDIATELY after real selected provider,
                // before another Cold/Parser/Reader/provider operation.
                parser_current();
                need(selected.use_count()>1&&!actual_settings->owner_before(selected)&&!selected.owner_before(*actual_settings),
                    "Completed actual selected input lost owner/control during provider");
            }
            parser_current();loan.require_same_cold(cold);live();parser_current();
        }
        struct Scope {
            const std::shared_ptr<State> state;
            explicit Scope(std::shared_ptr<State> actual):state(std::move(actual)) {
                state->live();if(state->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("Package core writer reentry");}
            }
            ~Scope(){state->busy.store(false);}
        };
    };
    class HeldCoreGuard {
        friend class BnwNativePackageDescriptorCoreWriter;
        const std::shared_ptr<State>& state;
        const std::shared_ptr<Parser>& parser;
        const Parser::HeldDocumentGuard& document;
        const BnwNativeCompletedColdSourceLoan* const completed_loan=nullptr;
        const BnwNativeColdFilesystemOwner* const completed_cold=nullptr;
        const std::thread::id creator_thread=std::this_thread::get_id();
        HeldCoreGuard(const std::shared_ptr<State>& actual,const std::shared_ptr<Parser>& parsed,
                      const Parser::HeldDocumentGuard& held):state(actual),parser(parsed),document(held){}
        HeldCoreGuard(const std::shared_ptr<State>& actual,const std::shared_ptr<Parser>& parsed,
            const Parser::HeldDocumentGuard& held,const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)
            :state(actual),parser(parsed),document(held),completed_loan(&loan),completed_cold(&cold){}
      public:
        void require_current()const {
            if(completed_loan) {
                need(creator_thread==std::this_thread::get_id(),"Completed Core guard foreign creator thread");
                need(state->busy.load()&&completed_cold,"Completed Core guard outside actual Core scope");
                state->check_completed(parser,*completed_loan,*completed_cold);document.require_current();
                state->check_completed(parser,*completed_loan,*completed_cold);
            }else {state->check(parser);document.require_current();state->check(parser);}
        }
        std::string platform_lowercase_ascii(std::string_view text)const {
            require_current();need(bool(state->actual_settings),"Selected manager CRC lacks actual retained CRT input");
            const auto selected=state->actual_settings->lock();
            need(selected&&selected.use_count()>1,"Selected manager CRC actual settings source retired");
            auto result=selected->platform_lowercase_ascii(text);require_current();
            need(selected.use_count()>1&&!state->actual_settings->owner_before(selected)&&!selected.owner_before(*state->actual_settings),
                "Selected manager CRC lost SAME settings source in callback");
            return result;
        }
        HeldCoreGuard(const HeldCoreGuard&)=delete;
        HeldCoreGuard&operator=(const HeldCoreGuard&)=delete;
    };
    const std::shared_ptr<State> state_;
    explicit BnwNativePackageDescriptorCoreWriter(const std::shared_ptr<Parser>& actual,const std::shared_ptr<const SelectedInput>& input):state_(std::make_shared<State>(actual,input)){}
    static std::shared_ptr<BnwNativePackageDescriptorCoreWriter> create(const std::shared_ptr<Parser>& actual,
            const std::shared_ptr<const SelectedInput>& input={}) {
        need(bool(actual),"Package core writer lacks genuine fresh parser");
        auto result=std::shared_ptr<BnwNativePackageDescriptorCoreWriter>(new BnwNativePackageDescriptorCoreWriter(actual,input));
        result->state_->self=result;return result;
    }
    bool write_actual_descriptor_core_738f00()const {
        const auto owned=state_;State::Scope operation(owned);const auto parser=owned->parser.lock();
        try {
            owned->check(parser);need(!owned->fields,"Package core writer occurrence replay");
            std::unique_ptr<Fields> output;const Doc* identity=nullptr;
            const auto selected=owned->actual_settings?owned->actual_settings->lock():nullptr;
            parser->with_current_document_before_738f00([&](const Doc& doc){
                owned->check(parser);identity=&doc;output=write_738f00_core(doc,selected.get());owned->check(parser);
            });
            owned->check(parser);if(!output){owned->failed.store(true);return false;}
            owned->fields=std::move(output);owned->document_identity=identity;owned->check(parser);
            return true; //Supported actual core fields ONLY. No Loaded authority.
        }catch(...){owned->failed.store(true);throw;}
    }
    void write_actual_standalone_metadata_738f00(const std::shared_ptr<const SelectedInput>& input)const {
        const auto selected=input;const auto owned=state_;State::Scope operation(owned);const auto parser=owned->parser.lock();
        try {
            owned->check(parser);need(selected&&owned->fields&&!owned->metadata,"Actual standalone metadata requires genuine selected input/core/new occurrence");
            selected->require_current();
            if(owned->actual_settings) {
                need(!owned->actual_settings->owner_before(selected)&&!selected.owner_before(*owned->actual_settings),
                     "Standalone metadata selected input differs from core arithmetic source");
            }
            auto output=std::make_unique<Metadata>();
            parser->with_current_document_before_738f00([&](const Doc& doc){
                owned->check(parser);need(&doc==owned->document_identity,"Standalone metadata lost SAME fresh descriptor");
                localized_field_890ec0(doc,*doc.roots.front(),"Name",*selected,output->name);
                localized_field_890ec0(doc,*doc.roots.front(),"Description",*selected,output->description);
                selected->require_current();owned->check(parser);
            });selected->require_current();owned->check(parser);
            owned->actual_settings=selected;owned->metadata=std::move(output);owned->check(parser);
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer> void with_current_standalone_metadata(Consumer&& consume)const {
        const auto owned=state_;State::Scope operation(owned);const auto parser=owned->parser.lock();
        try {
            owned->check(parser);need(bool(owned->metadata),"Actual standalone Name/Description phase absent");
            parser->with_current_document_before_738f00([&](const Doc& doc,const auto& document_guard){
                owned->check(parser);need(&doc==owned->document_identity,"Standalone metadata lost SAME fresh descriptor");
                const HeldCoreGuard guard(owned,parser,document_guard);
                if constexpr(std::is_invocable_v<Consumer&,const Fields&,const Metadata&,const Doc&,const HeldCoreGuard&>)
                    consume(*owned->fields,*owned->metadata,doc,guard);
                else consume(*owned->fields,*owned->metadata,doc);
                guard.require_current();owned->check(parser);
            });owned->check(parser);
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer> void with_current_core_fields(Consumer&& consume)const {
        const auto owned=state_;State::Scope operation(owned);const auto parser=owned->parser.lock();
        try {
            owned->check(parser);need(bool(owned->fields),"Package actual core fields absent");
            parser->with_current_document_before_738f00([&](const Doc& doc,const auto& document_guard){
                owned->check(parser);need(&doc==owned->document_identity,"Package core fields lost SAME descriptor");
                const HeldCoreGuard guard(owned,parser,document_guard);
                if constexpr(std::is_invocable_v<Consumer&,const Fields&,const Doc&,const HeldCoreGuard&>)
                    consume(*owned->fields,doc,guard);
                else consume(*owned->fields,doc);
                guard.require_current();owned->check(parser);
            });owned->check(parser);
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer>void with_current_standalone_metadata_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);const auto parser=owned->parser.lock();
        try {
            owned->check_completed(parser,loan,cold);loan.require_same_cold(cold);
            need(owned->fields&&owned->metadata,"Completed actual Core/Name/Description phases absent");
            parser->with_current_document_completed(loan,cold,[&](const Doc& doc,const auto& document_guard){
                owned->check_completed(parser,loan,cold);document_guard.require_current();
                need(&doc==owned->document_identity,"Completed metadata lost same original parsed document");
                const HeldCoreGuard guard(owned,parser,document_guard,loan,cold);
                {
                    const auto consume=std::forward<Consumer>(consumer);guard.require_current();owned->check_completed(parser,loan,cold);
                    if constexpr(std::is_invocable_v<decltype(consume)&,const Fields&,const Metadata&,const Doc&,const HeldCoreGuard&>)
                        consume(*owned->fields,*owned->metadata,doc,guard);
                    else consume(*owned->fields,*owned->metadata,doc);
                    guard.require_current();owned->check_completed(parser,loan,cold);
                } // Target destructor inside actual Parser/Reader/Cold guards.
                guard.require_current();owned->check_completed(parser,loan,cold);
            });owned->check_completed(parser,loan,cold);loan.require_same_cold(cold);
        }catch(...){owned->failed.store(true);throw;}
    }
public:
    ~BnwNativePackageDescriptorCoreWriter(){state_->closed.store(true);}
    BnwNativePackageDescriptorCoreWriter(const BnwNativePackageDescriptorCoreWriter&)=delete;
    BnwNativePackageDescriptorCoreWriter&operator=(const BnwNativePackageDescriptorCoreWriter&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativePackageDescriptorCoreWriter>);
} //namespace civ5::ui::detail
