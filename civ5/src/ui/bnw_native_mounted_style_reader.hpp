#pragma once
#include "bnw_native_fpk_mounted_catalog.hpp"
#include <limits>
#include <string_view>
#include "bnw_native_owned_skin_selected_input.hpp"
#include "bnw_native_style_parser_byte_tables.hpp"

namespace civ5::ui::detail {
// Actual mounted branch of8348C0: retained virtual-stream bytes +terminator.
// The Bank must resolve820410/8211F0 namespace/current manager first. This
// component cannot open an arbitrary path or accept a descriptor DOM/buffer.
class BnwNativeMountedStyleReader final {
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeOwnedUiSkinPoststageInstance;
    friend class BnwNativeColorSet64Storage;
    using Mount=BnwNativeFpkMountedCatalog;
    struct ParsedDocument;
    struct State {
        std::shared_ptr<const BnwNativeOwnedSkinSelectedInput> selected_input;
        std::weak_ptr<Mount::State> mount;
        std::shared_ptr<const Mount::Entry> entry;
        std::shared_ptr<std::vector<std::uint8_t>> source_buffer4;
        std::shared_ptr<const ParsedDocument> parsed_document;
        std::atomic<bool> parser_busy{false};
        const std::thread::id thread=std::this_thread::get_id();
        std::atomic<bool> closed{false},failed{false};
        void check() {
            Mount::State::need(thread==std::this_thread::get_id(),"Style reader foreign thread");
            Mount::State::need(!closed.load()&&!failed.load(),"Style reader retired");
            if(selected_input) {
                const auto actual=selected_input;actual->require_current();
                Mount::State::need(!closed.load()&&!failed.load()&&selected_input==actual,"Style actual selected input changed");
                actual->require_current();return;
            }
            auto owner=mount.lock();Mount::State::need(bool(owner),"Style mount expired");
            Mount::State::Scope scope(owner);
            owner->check_source();
            const auto found=owner->entries.find(entry->hash);
            Mount::State::need(found!=owner->entries.end()&&found->second==entry,"Style actual entry changed");
            Mount::State::need(!closed.load()&&!failed.load(),"Style reader lost during source callback");
            owner->check_source();
            Mount::State::need(!closed.load()&&!failed.load(),"Style reader retired after source callback");
        }
    };
    // Only the parser retained by this reader may borrow this quote. It keeps
    // the actual source buffer alive, but cannot extend facade/source lifetime.
    struct SourceQuote {
        std::shared_ptr<State> state;
        std::shared_ptr<const std::vector<std::uint8_t>> buffer;
        void require_current()const {
            // The callback can release storage containing this quote itself.
            const auto owned_state=state;const auto owned_buffer=buffer;
            try {owned_state->check();Mount::State::need(!owned_state->parser_busy.load(),"Style parser owns mutable source buffer");Mount::State::need(owned_state->source_buffer4==owned_buffer,"Style buffer replaced");}
            catch(...){owned_state->failed.store(true);throw;}
        }
    };
    std::shared_ptr<State> state_=std::make_shared<State>();
    explicit BnwNativeMountedStyleReader(std::shared_ptr<Mount> genuine_mounted_owner,
            std::shared_ptr<Mount::Stream> genuine_fresh_stream) {
        auto state=state_;
        try {
            Mount::State::need(genuine_mounted_owner&&genuine_fresh_stream,"Style actual stream absent");
            auto mount=genuine_mounted_owner->state_;
            // Authenticate immutable mount owner thread before reading ANY
            // mutable stream cursor; reader creator thread is insufficient.
            Mount::State::need(mount->thread==std::this_thread::get_id(),"Style actual mount foreign thread");
            Mount::State::need(genuine_fresh_stream->owner.lock()==mount&&genuine_fresh_stream->entry,
                               "Style stream foreign ResourceManager mount");
            Mount::State::need(genuine_fresh_stream->position==0,"Style reader requires genuine fresh stream");
            state->mount=mount;state->entry=genuine_fresh_stream->entry;
            state->check();
            const auto size=state->entry->length; //83C8C0 same Entry+4, not caller length.
            Mount::State::need(size<UINT32_MAX,"Style native size+1 wrap unsupported");
            auto buffer=std::make_shared<std::vector<std::uint8_t>>(std::size_t(size)+1);
            //8348C0 repeatedly reads remaining count from SAME stream until
            //full length, then appends NUL. Original's no-progress loop is
            //not emulated as successful input: failure poisons this occurrence.
            std::uint32_t received=0;
            do {
                auto chunk=std::make_shared<std::vector<std::uint8_t>>(size-received);
                const auto got=genuine_mounted_owner->read_same_mount(genuine_fresh_stream,chunk,size-received);
                state->check();Mount::State::need(got<=size-received&&(got!=0||size==0),"Style source read made no progress");
                std::copy_n(chunk->begin(),got,buffer->begin()+received);received+=got;
            } while(received<size);
            (*buffer)[size]=0;state->check();state->source_buffer4=std::move(buffer);
            state->check();
        } catch(...){state->failed.store(true);throw;}
    }
    //Private genuine directOS8348C0 lane. The only input constructor accepts
    //a closed Parent-issued actual54B8D0 read receipt, never caller bytes/path.
    explicit BnwNativeMountedStyleReader(std::shared_ptr<const BnwNativeOwnedSkinSelectedInput> actual_input) {
        const auto state=state_;
        try {
            Mount::State::need(bool(actual_input),"Style actual selected input absent");
            actual_input->require_current();state->selected_input=std::move(actual_input);state->check();
            std::size_t callbacks=0;
            state->selected_input->with_actual_bytes([&](std::span<const std::uint8_t> actual_bytes) {
                state->check();Mount::State::need(callbacks++==0,"Style selected read repeated actual bytes");
                Mount::State::need(actual_bytes.size()<UINT32_MAX,"Style native size+1 wrap unsupported");
                auto buffer=std::make_shared<std::vector<std::uint8_t>>(actual_bytes.size()+1);
                std::copy(actual_bytes.begin(),actual_bytes.end(),buffer->begin());
                (*buffer)[actual_bytes.size()]=0;state->check();state->source_buffer4=std::move(buffer);state->check();
            });
            Mount::State::need(callbacks==1&&state->source_buffer4,"Style selected read did not return actual bytes");
            state->check();
        }catch(...){state->failed.store(true);throw;}
    }
    SourceQuote source_for_same_parser()const {
        auto state=state_;
        try {
            state->check();Mount::State::need(bool(state->source_buffer4),"Style retained source buffer absent");
            SourceQuote quote{state,state->source_buffer4};quote.require_current();return quote;
        } catch(...){state->failed.store(true);throw;}
    }
    struct ParsedDocument {
        struct Slice {std::size_t offset=0;std::uint32_t length=0;std::size_t native_end_offset=0;};
        struct Attribute {Slice name,value;};
        struct Node {
            Slice name,value;
            bool has_value=false;
            const Node* parent=nullptr;
            std::vector<Attribute> attributes;
            std::vector<std::unique_ptr<const Node>> children;
        };
        std::weak_ptr<State> reader;
        std::shared_ptr<const std::vector<std::uint8_t>> buffer;
        std::vector<std::unique_ptr<const Node>> roots;
        void require_current()const {
            const auto owned_reader=reader.lock();const auto owned_buffer=buffer;
            const auto identity=this;
            Mount::State::need(bool(owned_reader),"Style parsed reader expired");
            try {
                owned_reader->check();
                Mount::State::need(!owned_reader->parser_busy.load(),"Style parse in progress");
                Mount::State::need(owned_reader->parsed_document.get()==identity&&owned_reader->source_buffer4==owned_buffer,
                                   "Style parsed occurrence changed");
                owned_reader->check();
            } catch(...){owned_reader->failed.store(true);throw;}
        }
        bool contains(const Node* target)const {
            const auto visit=[&](const auto& self,const Node& node)->bool {
                if(&node==target)return true;
                for(const auto& child:node.children)if(self(self,*child))return true;
                return false;
            };
            for(const auto& root:roots)if(visit(visit,*root))return true;
            return false;
        }
    };
    // Private port storage for the persistent FStringA locals in601CA0.
    // Only this SAME parsed-reader occurrence can initialize it. It does not
    // assert Original FString allocation/layout or activate the Skin caller.
    struct BorrowedNode;
    struct StringLocal {
    private:
        friend class BnwNativeMountedStyleReader;
        friend struct BorrowedNode;
        std::weak_ptr<const ParsedDocument> document;
        const std::thread::id thread=std::this_thread::get_id();
        std::atomic<bool> busy{false};
        std::string value;
        explicit StringLocal(const std::shared_ptr<const ParsedDocument>& owner):document(owner){}
    };
    struct BorrowedNode {
        std::weak_ptr<const ParsedDocument> document;
        const ParsedDocument::Node* node=nullptr;
        const ParsedDocument::Attribute* attribute=nullptr; //actual retained attribute node, never projected DOM
        struct NativeTextMetadata {
            //835490 returns Node+4 pointer and Node+C DWORD without a range
            //check. Metadata is NOT a bounded std::span/string capability.
            bool static_empty=false;
            std::size_t pointer_offset=0,native_end_offset=0;
            std::uint32_t native_length=0;
        };
        enum class StringConsume {native_false,native_true,incomplete_adjacent_bytes};
        StringConsume consume_same_string_local(const std::shared_ptr<StringLocal>& destination)const {
            // Retain receiver/source/destination before a source callback can
            // release their containing facade. No external bytes/provider.
            const auto owned=document.lock();const auto borrowed=node;
            const auto target=destination;
            Mount::State::need(owned&&target,"Style string source/destination expired");
            Mount::State::need(target->thread==std::this_thread::get_id(),"Style string foreign thread");
            Mount::State::need(target->document.lock()==owned,"Style string foreign parsed occurrence");
            if(target->busy.exchange(true)) {
                if(auto reader=owned->reader.lock())reader->failed.store(true);
                throw std::runtime_error("Style string consumer reentry");
            }
            struct End {std::shared_ptr<StringLocal> target;~End(){target->busy.store(false);}} end{target};
            try {
                owned->require_current();
                auto finish=[&](StringConsume result){owned->require_current();return result;};
                if(!borrowed)return finish(StringConsume::native_false); //835490 false.
                Mount::State::need(owned->contains(borrowed),"Style string node foreign occurrence");
                if(attribute) {
                    bool actual=false;for(const auto& candidate:borrowed->attributes)if(&candidate==attribute){actual=true;break;}
                    Mount::State::need(actual,"Color value foreign actual attribute");
                }
                if(!attribute&&!borrowed->has_value) { //835490 real node/static-empty success.
                    owned->require_current();target->value.clear();
                    return finish(StringConsume::native_true);
                }
                const auto slice=attribute?attribute->value:borrowed->value;const auto buffer=owned->buffer;
                Mount::State::need(slice.offset<=buffer->size(),"Style value pointer outside retained buffer");
                const auto available=buffer->size()-slice.offset;
                const auto count=std::min<std::uint64_t>(slice.length,available);
                //835330 scans raw DWORD length FIRST, without a NUL stop.
                // A highbit encountered in genuine bytes proves false even
                // when the later native end would leave this allocation.
                for(std::uint64_t i=0;i<count;++i)
                    if((*buffer)[slice.offset+static_cast<std::size_t>(i)]&0x80u)
                        return finish(StringConsume::native_false);
                if(count!=slice.length)
                    return finish(StringConsume::incomplete_adjacent_bytes);
                //8352A0 signed guard occurs ONLY after835330 true. Preserve
                // destination on negative raw DWORD; do not guess that guard
                // bypasses a scan outside the actual retained allocation.
                if(slice.length&0x80000000u)return finish(StringConsume::native_true);
                const auto source=reinterpret_cast<const char*>(buffer->data()+slice.offset);
                if(source==target->value.data())return finish(StringConsume::native_true);
                // Successful zero length clears; positive copies exact bytes,
                // including embedded NUL, into this distinct private owner.
                // std::string supplies the final terminator. Native81EFF0
                // allocator/object ABI remains a separate unimplemented seam.
                std::string replacement(source,slice.length);
                owned->require_current();target->value.swap(replacement);
                return finish(StringConsume::native_true);
            } catch(...) {
                if(auto reader=owned->reader.lock())reader->failed.store(true);
                throw;
            }
        }
        std::optional<NativeTextMetadata> copied_native_text_metadata()const {
            const auto owned=document.lock();const auto borrowed=node;
            Mount::State::need(bool(owned),"Style borrowed document expired");
            owned->require_current();std::optional<NativeTextMetadata> result;
            if(borrowed) {
                Mount::State::need(owned->contains(borrowed),"Style text node foreign occurrence");
                if(!borrowed->has_value)result=NativeTextMetadata{true,0,0,0};
                else result=NativeTextMetadata{false,borrowed->value.offset,borrowed->value.native_end_offset,borrowed->value.length};
            } // NULL native element returns false; real no-text node returns true/static empty.
            owned->require_current();return result;
        }
        // This port-only availability result does NOT substitute835490's
        // success or invent a native failure. Wrapped metadata remains above.
        // Downstream native string/Color consumers still require exact review.
        std::optional<std::string> copied_bounded_text_if_available()const {
            const auto owned=document.lock();const auto borrowed=node;
            Mount::State::need(bool(owned),"Style borrowed document expired");
            owned->require_current();std::optional<std::string> result;
            if(borrowed) {
                Mount::State::need(owned->contains(borrowed),"Style text node foreign occurrence");
                if(!borrowed->has_value)result.emplace();
                else {
                    const auto slice=borrowed->value;
                    if(slice.native_end_offset>=slice.offset&&slice.offset<=owned->buffer->size()&&
                       slice.length<=owned->buffer->size()-slice.offset&&
                       slice.native_end_offset-slice.offset==slice.length)
                        result.emplace(reinterpret_cast<const char*>(owned->buffer->data()+slice.offset),slice.length);
                }
            }
            owned->require_current();return result;
        }
        BorrowedNode first_same_attribute_value_8355c0(const std::string& key)const {
            const auto owned=document.lock();const auto borrowed=node;const auto copied_key=key;
            Mount::State::need(bool(owned),"Color attribute actual parser retired");
            owned->require_current();BorrowedNode result{owned,nullptr};
            if(borrowed) {
                Mount::State::need(owned->contains(borrowed),"Color attribute foreign actual node");
                for(const auto& actual:borrowed->attributes) {
                    const auto& name=actual.name;
                    if(name.length==copied_key.size()&&std::memcmp(owned->buffer->data()+name.offset,copied_key.data(),name.length)==0) {
                        result={owned,borrowed,&actual};break; //835B80 first exact length/bytes match.
                    }
                }
            }
            owned->require_current();return result;
        }
        std::vector<BorrowedNode> same_matching_children_835660_8356c0(const std::string& key)const {
            const auto owned=document.lock();const auto borrowed=node;const auto copied_key=key;
            Mount::State::need(bool(owned),"Color child actual parser retired");
            owned->require_current();std::vector<BorrowedNode> result;
            if(borrowed) {
                Mount::State::need(owned->contains(borrowed),"Color child foreign actual root");
                for(const auto& actual:borrowed->children) {
                    const auto& name=actual->name;
                    if(name.length==copied_key.size()&&std::memcmp(owned->buffer->data()+name.offset,copied_key.data(),name.length)==0)
                        result.push_back({owned,actual.get()});
                }
            }
            owned->require_current();return result;
        }
        std::string copied_name()const {
            const auto owned=document.lock();const auto borrowed=node;
            Mount::State::need(owned&&borrowed,"Style borrowed node expired");
            owned->require_current();
            Mount::State::need(owned->contains(borrowed),"Style node foreign occurrence");
            const auto slice=borrowed->name;
            std::string result(reinterpret_cast<const char*>(owned->buffer->data()+slice.offset),slice.length);
            owned->require_current();return result;
        }
        std::optional<std::string> copied_first_attribute(const std::string& key)const {
            const auto owned=document.lock();const auto borrowed=node;const auto copied_key=key;
            Mount::State::need(owned&&borrowed,"Style borrowed node expired");
            owned->require_current();Mount::State::need(owned->contains(borrowed),"Style node foreign occurrence");
            std::optional<std::string> result;
            for(const auto& attr:borrowed->attributes) {
                const std::string name(reinterpret_cast<const char*>(owned->buffer->data()+attr.name.offset),attr.name.length);
                if(name==copied_key) {
                    result.emplace(reinterpret_cast<const char*>(owned->buffer->data()+attr.value.offset),attr.value.length);
                    break; // Ordered duplicate attributes retained; no map overwrite.
                }
            }
            owned->require_current();return result;
        }
    };
    class NativeParseFailure final:public std::runtime_error {
      public:explicit NativeParseFailure(const char* text):std::runtime_error(text){}
    };
    struct Parser {
        using Tables=BnwNativeStyleParserByteTables;
        using Node=ParsedDocument::Node;using Slice=ParsedDocument::Slice;
        std::shared_ptr<State> reader;
        std::shared_ptr<std::vector<std::uint8_t>> bytes;
        std::shared_ptr<ParsedDocument> result;
        std::size_t position=0;
        explicit Parser(std::shared_ptr<State> owner):reader(std::move(owner)),bytes(reader->source_buffer4),result(std::make_shared<ParsedDocument>()) {
            result->reader=reader;result->buffer=bytes;
        }
        std::uint8_t at(std::size_t p)const {Mount::State::need(p<bytes->size(),"Style source cursor outside retained buffer");return (*bytes)[p];}
        std::uint8_t current()const{return at(position);}
        bool starts(const char* text)const {
            const auto n=std::strlen(text);if(n>bytes->size()-position)return false;
            return std::memcmp(bytes->data()+position,text,n)==0;
        }
        void need(bool v,const char* error)const {
            if(v)return;
            //Only actual parser syntax-error operands enter834B30 native
            //false. Unsupported extent/depth/numeric-domain/source failures
            //keep their throwing port frontier; no catch-all false conversion.
            constexpr std::string_view actual_syntax_errors[]={
                "Style native entity expected semicolon","Style native unexpected end of data",
                "Style native unexpected end of DOCTYPE","Style native unexpected end of DOCTYPE subset",
                "Style native expected element name","Style native expected attribute name",
                "Style native expected equals","Style native expected attribute quote",
                "Style native expected closing attribute quote","Style native expected greater-than",
                "Style native unexpected end of element","Style native expected closing greater-than",
                "Style native expected less-than"};
            for(const auto actual:actual_syntax_errors)if(actual==error)throw NativeParseFailure(error);
            Mount::State::need(false,error);
        }
        void whitespace(){while(Tables::space[current()])++position;}
        Slice name(const std::array<std::uint8_t,256>& table) {
            const auto begin=position;while(table[current()])++position;
            return {begin,static_cast<std::uint32_t>(position-begin),position};
        }
        void write_utf8(std::uint32_t value,std::size_t& write) {
            need(value<0x110000u,"Style native numeric entity outside UTF8 domain");
            auto put=[&](std::uint8_t c){need(write<bytes->size(),"Style decoded write outside source");(*bytes)[write++]=c;};
            //8383B0 accepts surrogate codepoints too; do not invent Unicode validation.
            if(value<0x80u)put(static_cast<std::uint8_t>(value));
            else if(value<0x800u){put(static_cast<std::uint8_t>((value>>6)|0xc0));put(static_cast<std::uint8_t>((value&63)|0x80));}
            else if(value<0x10000u){put(static_cast<std::uint8_t>((value>>12)|0xe0));put(static_cast<std::uint8_t>(((value>>6)&63)|0x80));put(static_cast<std::uint8_t>((value&63)|0x80));}
            else {put(static_cast<std::uint8_t>((value>>18)|0xf0));put(static_cast<std::uint8_t>(((value>>12)&63)|0x80));put(static_cast<std::uint8_t>(((value>>6)&63)|0x80));put(static_cast<std::uint8_t>((value&63)|0x80));}
        }
        Slice decoded(const std::array<std::uint8_t,256>& allowed) {
            const auto begin=position;auto write=position;
            while(allowed[current()]) {
                if(current()=='&'&&position+1<bytes->size()&&at(position+1)=='#') {
                    position+=2;const bool hex=current()=='x';if(hex)++position;
                    std::uint32_t value=0;
                    //837930/837B70/838470 use SAME hex-digit table for BOTH
                    //radices. uint32 accumulation wraps; uppercase X is not x.
                    while(Tables::digit[current()]!=255u) {
                        value=value*(hex?16u:10u)+Tables::digit[current()];++position;
                    }
                    write_utf8(value,write);need(current()==';',"Style native entity expected semicolon");++position;continue;
                }
                bool entity=false;
                if(current()=='&') {
                    struct Named {const char* text;std::uint8_t byte;};
                    constexpr Named names[]={{"&amp;",'&'},{"&apos;",'\''},{"&gt;",'>'},{"&lt;",'<'},{"&quot;",'"'}};
                    for(const auto& n:names)if(starts(n.text)) {(*bytes)[write++]=n.byte;position+=std::strlen(n.text);entity=true;break;}
                }
                if(!entity)(*bytes)[write++]=(*bytes)[position++]; //unknown entity remains literal.
            }
            return {begin,static_cast<std::uint32_t>(write-begin),write}; // actual raw pointer/end and DWORD.
        }
        void skip_until(const char* end) {
            while(!starts(end)){need(current()!=0,"Style native unexpected end of data");++position;}
            position+=std::strlen(end);
        }
        void doctype() {
            while(current()!='>') {
                need(current()!=0,"Style native unexpected end of DOCTYPE");
                if(current()!='['){++position;continue;}
                ++position;std::uint32_t nesting=1;
                while(nesting) {
                    need(current()!=0,"Style native unexpected end of DOCTYPE subset");
                    if(current()=='[')++nesting;else if(current()==']')--nesting;
                    ++position;
                }
            }
            ++position;
        }
        std::unique_ptr<Node> dispatch(Node* parent,std::size_t depth) {
            //8361D0 called AFTER '<'. Comments/declarations/CDATA/DOCTYPE
            //produce no element, including CDATA inside an element.
            if(current()=='?'){++position;skip_until("?>");return {};}
            if(current()=='!') {
                if(starts("!--")){position+=3;skip_until("-->");return {};}
                if(starts("![CDATA[")){position+=8;skip_until("]]>");return {};}
                if(starts("!DOCTYPE")&&position+8<bytes->size()&&Tables::space[at(position+8)]) {position+=9;doctype();return {};}
                ++position;skip_until(">");return {}; //native unknown bang declaration.
            }
            need(depth<=512,"Style native recursion depth outside supported port domain");
            auto node=std::make_unique<Node>();node->parent=parent;node->name=name(Tables::element_name);
            need(node->name.length!=0,"Style native expected element name");whitespace();
            while(Tables::attribute_name[current()]) {
                ParsedDocument::Attribute attr;attr.name=name(Tables::attribute_name);
                need(attr.name.length!=0,"Style native expected attribute name");whitespace();
                need(current()=='=',"Style native expected equals");++position;whitespace();
                const auto quote=current();need(quote=='\''||quote=='"',"Style native expected attribute quote");++position;
                attr.value=decoded(quote=='\''?Tables::single_value:Tables::double_value);
                need(current()==quote,"Style native expected closing attribute quote");++position;
                node->attributes.push_back(attr);whitespace();
            }
            if(current()=='/') {++position;need(current()=='>',"Style native expected greater-than");++position;return node;}
            need(current()=='>',"Style native expected greater-than");++position;
            while(true) {
                whitespace();need(current()!=0,"Style native unexpected end of element");
                if(current()!='<') {
                    auto text=decoded(Tables::text_value);
                    //837DB0 trims backwards WITHOUT a begin bound. Retain
                    //native pointer/end and modulo32 SUB even when end<begin.
                    auto native_end=text.native_end_offset;
                    while(true) {
                        need(native_end!=0,"Style native trim outside retained buffer; parity unfinished");
                        if(!Tables::space[(*bytes)[native_end-1]])break;
                        --native_end;
                    }
                    text.native_end_offset=native_end;
                    text.length=static_cast<std::uint32_t>(native_end-text.offset);
                    //837DB0 only records first nonempty text; later text is
                    //still decoded, but does not concatenate into that field.
                    // Native checks the stored pointer's FIRST BYTE, not
                    // its trimmed length (a zero-length space entity differs).
                    if(!node->has_value||(*bytes)[node->value.offset]==0) {node->value=text;node->has_value=true;}
                    continue;
                }
                ++position;
                if(current()=='/') {
                    ++position;name(Tables::element_name);whitespace();
                    //8369B0 consumes the closing name WITHOUT comparing it
                    //to the opening name. Preserve this actual native quirk.
                    need(current()=='>',"Style native expected closing greater-than");++position;return node;
                }
                if(auto child=dispatch(node.get(),depth+1))node->children.push_back(std::move(child));
            }
        }
        void run() {
            if(bytes->size()>=4&&at(0)==0xef&&at(1)==0xbb&&at(2)==0xbf)position=3;
            while(true) {
                whitespace();if(current()==0)return;
                need(current()=='<',"Style native expected less-than");++position;
                if(auto node=dispatch(nullptr,0))result->roots.push_back(std::move(node));
            }
        }
    };
    bool try_parse_same_owned_source_834b30()const {
        const auto state=state_;
        Mount::State::need(state->thread==std::this_thread::get_id(),"Style native parser foreign creator thread");
        if(state->parser_busy.exchange(true)) {
            state->failed.store(true);throw std::runtime_error("Style native parser reentry");
        }
        struct End {std::shared_ptr<State> state;~End(){state->parser_busy.store(false);}} end{state};
        try {
            state->check();Mount::State::need(state->source_buffer4&&!state->parsed_document,"Style native parser source absent/already consumed");
            Parser parser(state);bool native_success=true;
            try {parser.run();}catch(const NativeParseFailure&){native_success=false;}
            state->check();state->parsed_document=std::move(parser.result);state->check();
            //Authentic partial parser storage stays with this reader. False
            //does not call834B80 or admit its root as a successful load.
            return native_success;
        }catch(...){state->failed.store(true);throw;}
    }
    std::shared_ptr<const ParsedDocument> parse_same_owned_source()const {
        auto state=state_;
        Mount::State::need(state->thread==std::this_thread::get_id(),"Style parser foreign creator thread");
        if(state->parser_busy.exchange(true)) {
            state->failed.store(true);throw std::runtime_error("Style parser reentry");
        }
        struct End {std::shared_ptr<State> state;~End(){state->parser_busy.store(false);}} end{state};
        try {
            state->check();Mount::State::need(state->source_buffer4&&!state->parsed_document,"Style parser source absent/already consumed");
            Parser parser(state);parser.run();state->check();
            state->parsed_document=std::move(parser.result);return state->parsed_document;
        } catch(...){state->failed.store(true);throw;}
    }
    BorrowedNode first_same_owned_element()const {
        auto state=state_;
        // parsed_document is mutable publication owned by reader thread.
        // Check that immutable owner BEFORE reading the shared_ptr field.
        Mount::State::need(state->thread==std::this_thread::get_id(),"Style parsed owner foreign thread");
        state->check();
        auto document=state->parsed_document;Mount::State::need(bool(document),"Style parsed owner absent");
        document->require_current();BorrowedNode result;
        result={document,document->roots.empty()?nullptr:document->roots.front().get()};
        document->require_current();return result;
    }
    std::shared_ptr<StringLocal> make_same_owned_string_local()const {
        const auto state=state_;
        Mount::State::need(state->thread==std::this_thread::get_id(),"Style string owner foreign thread");
        state->check();const auto document=state->parsed_document;
        Mount::State::need(bool(document),"Style string actual parsed owner absent");
        document->require_current();
        auto result=std::shared_ptr<StringLocal>(new StringLocal(document));
        document->require_current();return result;
    }
    std::string copied_same_string_local(const std::shared_ptr<StringLocal>& destination)const {
        const auto state=state_;const auto target=destination;
        Mount::State::need(state->thread==std::this_thread::get_id(),"Style string reader foreign thread");
        Mount::State::need(target&&target->thread==std::this_thread::get_id(),"Style string destination foreign thread");
        const auto document=target->document.lock();
        Mount::State::need(bool(document),"Style string parsed owner expired");
        if(target->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("Style string read reentry");}
        struct End {std::shared_ptr<StringLocal> target;~End(){target->busy.store(false);}} end{target};
        try {
            state->check();Mount::State::need(state->parsed_document==document,"Style string foreign reader");
            document->require_current();std::string result=target->value;
            document->require_current();return result;
        } catch(...){state->failed.store(true);throw;}
    }
    BnwNativeMountedStyleReader(const BnwNativeMountedStyleReader&)=delete;
    BnwNativeMountedStyleReader&operator=(const BnwNativeMountedStyleReader&)=delete;
public:
    ~BnwNativeMountedStyleReader(){state_->closed.store(true);}
};
} //namespace civ5::ui::detail
