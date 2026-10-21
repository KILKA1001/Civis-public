#pragma once
#include "bnw_native_package_descriptor_byte_reader.hpp"
#include <array>
#include <optional>
#include <string_view>

namespace civ5::ui::detail {
//Owned fresh-byte834B30/835EB0 grammar. Native object layout/allocator and
//738F00 descriptor validation are separate; this owner cannot issue Loaded.
class BnwNativePackageXmlParserOwner final {
    friend class BnwNativeInitialDescriptorSource;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativePackageDescriptorCoreWriter;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeSkinDocumentRegistration;
    using Reader=BnwNativePackageDescriptorByteReader;
    struct Range {std::size_t begin=0,size=0;};
    struct Attribute {Range name,value;};
    struct Node {
        Range name;
        std::optional<Range> value;
        std::vector<Attribute> attributes;
        std::vector<std::unique_ptr<Node>> children;
    };
    struct ParseError {std::string_view message;std::size_t offset;};
    struct Document {
        //SAME immutable8348C0 Buffer remains owned. All node ranges address
        //this document's in-place decoded copy of those exact fresh bytes.
        const std::shared_ptr<const Reader::Buffer> input;
        std::vector<std::uint8_t> decoded;
        std::vector<std::unique_ptr<Node>> roots;
        explicit Document(std::shared_ptr<const Reader::Buffer> actual)
            :input(std::move(actual)),decoded(input->nul_terminated){}
    };
    struct Kernel {
        Document& doc;
        std::size_t cursor=0,nodes=0;
        std::optional<ParseError> first_error;
        struct SyntaxFailure {};
        explicit Kernel(Document& actual):doc(actual){}
        static constexpr std::array<std::uint8_t,256> tb75810={
            0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        };
        static constexpr std::array<std::uint8_t,256> tb75950={
            0,1,1,1,1,1,1,1,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb75a50={
            0,1,1,1,1,1,1,1,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb75b50={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb75c50={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb75d50={
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,1,2,3,4,5,6,7,8,9,255,255,255,255,255,255,
            255,10,11,12,13,14,15,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,10,11,12,13,14,15,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
        };
        static constexpr std::array<std::uint8_t,256> tb75e78={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb75f78={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb76078={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };
        static constexpr std::array<std::uint8_t,256> tb76178={
            0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        };

        std::uint8_t at(std::size_t p)const {
            if(p>=doc.decoded.size())throw std::runtime_error("Native XML cursor escaped owned buffer");
            return doc.decoded[p];
        }
        [[noreturn]] void error(std::string_view message,std::size_t at) {
            if(!first_error)first_error=ParseError{message,at};throw SyntaxFailure{};
        }
        void whitespace(){while(tb75810[at(cursor)])++cursor;}
        bool starts(std::string_view value)const {
            if(value.size()>doc.decoded.size()-cursor)return false;
            for(std::size_t i=0;i<value.size();++i)
                if(at(cursor+i)!=static_cast<std::uint8_t>(value[i]))return false;
            return true;
        }
        void skip_until(std::string_view end) {
            while(!starts(end)){if(!at(cursor))error("unexpected end of data",cursor);++cursor;}
            cursor+=end.size();
        }
        void emit(std::size_t& output,std::uint8_t byte) {
            if(output>=doc.decoded.size())throw std::runtime_error("Native XML decoded extent escaped Buffer");
            doc.decoded[output++]=byte;
        }
        void codepoint(std::size_t& output,std::uint32_t value) {
            if(value<0x80)emit(output,static_cast<std::uint8_t>(value));
            else if(value<0x800){emit(output,0xc0|(value>>6));emit(output,0x80|(value&63));}
            else if(value<0x10000){emit(output,0xe0|(value>>12));emit(output,0x80|((value>>6)&63));emit(output,0x80|(value&63));}
            else if(value<0x110000){emit(output,0xf0|(value>>18));emit(output,0x80|((value>>12)&63));emit(output,0x80|((value>>6)&63));emit(output,0x80|(value&63));}
            else error("invalid numeric character",cursor);
            //8383B0 accepts surrogate values and zero, as the native emitter does.
        }
        std::size_t translated(const std::array<std::uint8_t,256>& stop) {
            std::size_t output=cursor;
            while(stop[at(cursor)]) {
                if(at(cursor)!='&'){emit(output,at(cursor++));continue;}
                struct Entity {std::string_view spelling;std::uint8_t byte;};
                static constexpr Entity entities[]={{"&amp;",'&'},{"&apos;",'\''},{"&gt;",'>'},{"&lt;",'<'},{"&quot;",'"'}};
                bool matched=false;
                for(const auto& item:entities)if(starts(item.spelling)) {
                    cursor+=item.spelling.size();emit(output,item.byte);matched=true;break;
                }
                if(matched)continue;
                if(starts("&#")) {
                    cursor+=2;std::uint32_t value=0;
                    const bool hex=at(cursor)=='x';if(hex)++cursor;
                    while(tb75d50[at(cursor)]!=255) {
                        const auto digit=tb75d50[at(cursor++)];
                        value=hex?(value<<4)+digit:value*std::uint32_t(10)+digit;
                    }
                    codepoint(output,value); //Native emits BEFORE checking ';'.
                    if(at(cursor)!=';')error("expected ;",cursor);
                    ++cursor;continue;
                }
                emit(output,at(cursor++)); //Unknown entities are literal, not errors.
            }
            return output;
        }
        void attributes(Node& node) {
            while(tb75a50[at(cursor)]) {
                const auto start=cursor;while(tb75a50[at(cursor)])++cursor;
                const Range name{start,cursor-start};whitespace();
                if(at(cursor)!='=')error("expected =",cursor);
                ++cursor;whitespace();const auto quote=at(cursor);
                if(quote!='\''&&quote!='"')error("expected ' or \"",cursor);
                ++cursor;const auto value_start=cursor;
                const auto end=translated(quote=='\''?tb75b50:tb75c50);
                if(at(cursor)!=quote)error("unexpected end of data",cursor);
                node.attributes.push_back({name,{value_start,end-value_start}});
                ++cursor;whitespace();
            }
        }
        void contents(Node& node,std::size_t depth) {
            for(;;) {
                whitespace();
                if(!at(cursor))error("unexpected end of data",cursor);
                if(at(cursor)!='<') {
                    const auto start=cursor;auto end=translated(tb75e78);
                    //Native trailing trim; bound the zero-length case rather
                    //than reproduce a pointer underflow outside owned Buffer.
                    while(end>start&&tb75810[at(end-1)])--end;
                    if(!node.value||at(node.value->begin)==0)node.value=Range{start,end-start};
                    continue;
                }
                if(cursor+1<doc.decoded.size()&&at(cursor+1)=='/') {
                    cursor+=2;while(tb75950[at(cursor)])++cursor;whitespace();
                    if(at(cursor)!='>')error("expected >",cursor);
                    ++cursor;return; //Native does NOT compare closing/opening names.
                }
                ++cursor;if(auto child=dispatch(depth+1))node.children.push_back(std::move(child));
            }
        }
        std::unique_ptr<Node> element(std::size_t depth) {
            if(depth>512||++nodes>1000000)throw std::runtime_error("Native XML supported node/depth budget exceeded");
            auto node=std::make_unique<Node>();const auto start=cursor;
            while(tb75950[at(cursor)])++cursor;
            if(start==cursor)error("expected element name",cursor);
            node->name={start,cursor-start};whitespace();attributes(*node);
            if(at(cursor)=='>'){++cursor;contents(*node,depth);}
            else if(at(cursor)=='/'){++cursor;if(at(cursor)!='>')error("expected >",cursor);++cursor;}
            else error("expected >",cursor);
            return node;
        }
        std::unique_ptr<Node> dispatch(std::size_t depth) {
            if(at(cursor)=='!') {
                ++cursor;
                if(starts("--")){cursor+=2;skip_until("-->");return {};}
                if(starts("[CDATA[")){cursor+=7;skip_until("]]>");return {};}
                if(starts("DOCTYPE")&&cursor+7<doc.decoded.size()&&tb75810[at(cursor+7)]) {
                    cursor+=8;
                    for(;;) {
                        if(!at(cursor))error("unexpected end of data",cursor);
                        if(at(cursor)=='>'){++cursor;return {};}
                        if(at(cursor++)=='[') {
                            std::size_t brackets=1;
                            while(brackets) {
                                const auto byte=at(cursor);if(!byte)error("unexpected end of data",cursor);
                                ++cursor;if(byte=='[')++brackets;else if(byte==']')--brackets;
                            }
                        }
                    }
                }
                skip_until(">");return {};
            }
            if(at(cursor)=='?'){++cursor;skip_until("?>");return {};}
            return element(depth);
        }
        bool parse_835eb0() {
            try {
                if(starts("\xef\xbb\xbf"))cursor=3;
                for(;;) {
                    whitespace();if(!at(cursor))return true;
                    if(at(cursor)!='<')error("expected <",cursor);
                    ++cursor;if(auto root=dispatch(1))doc.roots.push_back(std::move(root));
                }
            }catch(const SyntaxFailure&){return false;}
        }
    };
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<Reader> reader;
        std::weak_ptr<const BnwNativePackageXmlParserOwner> self;
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::unique_ptr<const Document> document;
        std::optional<ParseError> syntax_error;
        explicit State(const std::shared_ptr<Reader>& actual):reader(actual){}
        void live()const {
            Reader::State::need(thread==std::this_thread::get_id()&&!closed.load()&&!failed.load()&&!self.expired(),
                "Package owned XML parser retired/foreign thread");
        }
        void check(const std::shared_ptr<Reader>& actual)const {
            live();Reader::State::need(actual&&actual.use_count()>1&&!reader.owner_before(actual)&&!actual.owner_before(reader),
                "Package XML parser lost SAME externally owned byte reader");
        }
        struct Scope {
            const std::shared_ptr<State> value;
            explicit Scope(std::shared_ptr<State> actual):value(std::move(actual)) {
                value->live();if(value->busy.exchange(true)){value->failed.store(true);throw std::runtime_error("Package XML parser reentry");}
            }
            ~Scope(){value->busy.store(false);}
        };
    };
    class HeldDocumentGuard {
        friend class BnwNativePackageXmlParserOwner;
        const std::shared_ptr<State>& state;
        const std::shared_ptr<Reader>& reader;
        const Reader::HeldSourceGuard& source;
        HeldDocumentGuard(const std::shared_ptr<State>& actual,const std::shared_ptr<Reader>& bytes,
                          const Reader::HeldSourceGuard& native):state(actual),reader(bytes),source(native){}
      public:
        void require_current()const {state->check(reader);source.require_current();state->check(reader);}
        HeldDocumentGuard(const HeldDocumentGuard&)=delete;
        HeldDocumentGuard&operator=(const HeldDocumentGuard&)=delete;
    };
    const std::shared_ptr<State> state_;
    explicit BnwNativePackageXmlParserOwner(const std::shared_ptr<Reader>& actual):state_(std::make_shared<State>(actual)){}
    static std::shared_ptr<BnwNativePackageXmlParserOwner> create(const std::shared_ptr<Reader>& actual) {
        Reader::State::need(bool(actual),"Package XML parser lacks actual byte reader");
        auto result=std::shared_ptr<BnwNativePackageXmlParserOwner>(new BnwNativePackageXmlParserOwner(actual));
        result->state_->self=result;return result;
    }
    bool parse_834b30()const {
        const auto owned=state_;State::Scope operation(owned);const auto reader=owned->reader.lock();
        try {
            owned->check(reader);Reader::State::need(!owned->document&&!owned->syntax_error,"Package parser occurrence replay");
            std::unique_ptr<Document> result;bool parsed=false;
            reader->with_current_bytes_before_834b30([&](auto bytes,std::uint32_t size){
                owned->check(reader);
                const auto actual=reader->state_->buffer;
                Reader::State::need(actual&&bytes.data()==actual->nul_terminated.data()&&size==actual->native_size,
                    "Package XML parser byte Buffer identity changed");
                result=std::make_unique<Document>(actual);Kernel kernel(*result);
                parsed=kernel.parse_835eb0();owned->syntax_error=kernel.first_error;owned->check(reader);
            });
            owned->check(reader);reader->close_after_actual_parser_8349aa();owned->check(reader);
            if(parsed)owned->document=std::move(result);
            return parsed; //Computed grammar result only;738F00 still required.
        }catch(...) {
            owned->failed.store(true);if(reader)reader->retire_after_parser_failure();throw;
        }
    }
    template<class Consumer> void with_current_document_before_738f00(Consumer&& consume)const {
        const auto owned=state_;State::Scope operation(owned);const auto reader=owned->reader.lock();
        try {
            owned->check(reader);Reader::State::need(bool(owned->document),"Actual package XML syntax success absent");
            reader->with_current_bytes_before_834b30([&](auto bytes,std::uint32_t size,const auto& native_source){
                owned->check(reader);const auto& doc=*owned->document;
                Reader::State::need(bytes.data()==doc.input->nul_terminated.data()&&size==doc.input->native_size,
                    "Package parsed node ranges detached from SAME original Buffer");
                const HeldDocumentGuard guard(owned,reader,native_source);
                if constexpr(std::is_invocable_v<Consumer&,const Document&,const HeldDocumentGuard&>)consume(doc,guard);
                else consume(doc);
                guard.require_current();owned->check(reader);
            });
            owned->check(reader);
        }catch(...){owned->failed.store(true);if(reader)reader->retire_after_parser_failure();throw;}
    }
    template<class Consumer>void with_current_document_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);const auto reader=owned->reader.lock();
        try {
            owned->check(reader);loan.require_same_cold(cold);
            Reader::State::need(bool(owned->document),"Completed actual XML syntax success absent");
            reader->with_current_bytes_completed(loan,cold,[&](auto bytes,std::uint32_t size,const auto& native_source){
                owned->check(reader);native_source.require_current();const auto& doc=*owned->document;
                Reader::State::need(bytes.data()==doc.input->nul_terminated.data()&&size==doc.input->native_size,
                    "Completed parsed node ranges detached from original byte Buffer");
                const HeldDocumentGuard guard(owned,reader,native_source);
                {
                    const auto consume=std::forward<Consumer>(consumer);guard.require_current();owned->check(reader);
                    if constexpr(std::is_invocable_v<decltype(consume)&,const Document&,const HeldDocumentGuard&>)consume(doc,guard);
                    else consume(doc);
                    guard.require_current();owned->check(reader);
                } // Application target destructor still inside actual Reader source guard.
                guard.require_current();owned->check(reader);
            });owned->check(reader);loan.require_same_cold(cold);
        }catch(...){owned->failed.store(true);if(reader)reader->retire_after_parser_failure();throw;}
    }
public:
    ~BnwNativePackageXmlParserOwner(){state_->closed.store(true);}
    BnwNativePackageXmlParserOwner(const BnwNativePackageXmlParserOwner&)=delete;
    BnwNativePackageXmlParserOwner&operator=(const BnwNativePackageXmlParserOwner&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativePackageXmlParserOwner>);
} //namespace civ5::ui::detail
