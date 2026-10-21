#pragma once
#include "bnw_native_initial_descriptor_source.hpp"
#include "bnw_native_ui_skin_retained_occurrence.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_map>

namespace civ5::ui::native_ui_skin_text_detail {
inline void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
// Validate the captured bytes themselves before issuing retained inputs.
// File before/after pins and normalized DOM equality alone are insufficient.
inline void require_captured_bytes(std::string_view raw, const runtime::FileIdentity& expected) {
    const auto captured = runtime::identify_bytes(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(raw.data()),raw.size()));
    need(captured.bytes == expected.bytes && captured.sha256 == expected.sha256,
         "Native UISkin captured original bytes differ from SAME descriptor pin");
}
inline bool space(unsigned char c) { return c == 9 || c == 10 || c == 13 || c == 32; }
inline unsigned digit(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 255;
}
// Exact8383B0 byte emitter, including zero and surrogate DWORD values.
inline void emit(std::string& out, std::uint32_t value) {
    need(value < 0x110000U, "Native UISkin numeric value marks descriptor parse failure");
    if (value < 0x80) out.push_back(char(value));
    else if (value < 0x800) {
        out.push_back(char(0xc0 | (value >> 6))); out.push_back(char(0x80 | (value & 63)));
    } else if (value < 0x10000) {
        out.push_back(char(0xe0 | (value >> 12))); out.push_back(char(0x80 | ((value >> 6) & 63)));
        out.push_back(char(0x80 | (value & 63)));
    } else {
        out.push_back(char(0xf0 | (value >> 18))); out.push_back(char(0x80 | ((value >> 12) & 63)));
        out.push_back(char(0x80 | ((value >> 6) & 63))); out.push_back(char(0x80 | (value & 63)));
    }
}
// Pure source kernel: never a Package/token issuer. Input is one literal text
// run ending before raw '<'; decoded '<' remains a value byte.
inline std::string transform(std::string_view raw) {
    std::string out;
    for (std::size_t p = 0; p < raw.size();) {
        if (raw[p] == '&') {
            bool named = false;
            for (const auto& [spelling, value] : std::array<std::pair<std::string_view,char>,5>{
                     {{"&amp;",'&'},{"&apos;",'\''},{"&quot;",'"'},{"&gt;",'>'},{"&lt;",'<'}}}) {
                if (raw.substr(p, spelling.size()) == spelling) {
                    out.push_back(value); p += spelling.size(); named = true; break;
                }
            }
            if (named) continue;
            if (p + 1 < raw.size() && raw[p + 1] == '#') {
                auto at = p + 2;
                const bool hex = at < raw.size() && raw[at] == 'x';
                if (hex) ++at;
                std::uint32_t value = 0;
                while (at < raw.size() && digit(static_cast<unsigned char>(raw[at])) != 255) {
                    value = value * (hex ? 16U : 10U) + digit(static_cast<unsigned char>(raw[at++]));
                }
                // The native emitter runs before this delimiter test. Any
                // failure poisons reader parse; a private result is not issued.
                emit(out, value);
                need(at < raw.size() && raw[at] == ';', "Native UISkin numeric entity marks parse failure");
                p = at + 1; continue;
            }
        }
        out.push_back(raw[p++]);
    }
    return out;
}
enum class RunKind { text, cdata };
struct Run { const content::XmlNode* node; std::size_t begin, length; RunKind kind; };
struct Value { std::string bytes; bool first_buffer_byte_nonzero = false; };
struct LexicalDocument {
    std::vector<Run> runs;
    std::unordered_map<const content::XmlNode*, Value> values;
};
// Retains literal offsets and maps elements to the SAME already parsed DOM.
// This does not replace XmlLite validation or change its concatenated text.
inline LexicalDocument retain_runs(const content::XmlNode& root, std::string_view raw) {
    struct Frame { const content::XmlNode* node; std::size_t child = 0; };
    LexicalDocument result;
    std::vector<Frame> stack;
    bool root_seen = false;
    std::size_t p = raw.starts_with("\xef\xbb\xbf") ? 3 : 0;
    const auto finish = [&] {
        need(!stack.empty() && stack.back().child == stack.back().node->children.size(),
             "Native UISkin raw source/retained DOM children differ");
        stack.pop_back();
    };
    while (p < raw.size()) {
        if (raw[p] != '<') {
            const auto end = raw.find('<', p);
            const auto stop = end == std::string_view::npos ? raw.size() : end;
            if (!stack.empty()) {
                const auto* node = stack.back().node;
                result.runs.push_back({node, p, stop - p, RunKind::text});
                auto begin = p;
                while (begin < stop && space(static_cast<unsigned char>(raw[begin]))) ++begin;
                if (begin < stop) {
                    auto text = transform(raw.substr(begin, stop - begin));
                    auto& current = result.values.at(node);
                    if (!current.first_buffer_byte_nonzero) {
                        //837DB0 tests value-buffer[0], not trimmed metadata
                        // length. An entity producing space can lock length0.
                        current.first_buffer_byte_nonzero = !text.empty() && text.front() != '\0';
                        while (!text.empty() && space(static_cast<unsigned char>(text.back()))) text.pop_back();
                        current.bytes = std::move(text);
                    }
                }
            } else {
                need(std::all_of(raw.begin()+p, raw.begin()+stop, [](unsigned char c){return space(c);}),
                     "Native UISkin text outside retained root");
            }
            p = stop; continue;
        }
        const auto skip = [&](std::string_view end, std::size_t from) {
            const auto stop = raw.find(end, from);
            need(stop != std::string_view::npos, "Native UISkin incomplete raw markup");
            return stop;
        };
        if (raw.substr(p,4) == "<!--") { p = skip("-->",p+4)+3; continue; }
        if (raw.substr(p,2) == "<?") { p = skip("?>",p+2)+2; continue; }
        if (raw.substr(p,9) == "<![CDATA[") {
            const auto end = skip("]]>",p+9);
            if (!stack.empty()) result.runs.push_back({stack.back().node,p+9,end-(p+9),RunKind::cdata});
            p = end+3; continue; // Exact836510: no child or scalar value.
        }
        // The SAME XmlLite document already prohibits DTDs. This is a
        // consistency failure, not a new supported-input restriction.
        need(raw.substr(p,2) != "<!", "Native UISkin raw declaration differs from retained XmlLite document");
        const bool closing = p+1 < raw.size() && raw[p+1] == '/';
        auto name_begin = p+(closing?2:1), at = name_begin;
        while (at < raw.size() && !space(static_cast<unsigned char>(raw[at])) && raw[at]!='/' && raw[at]!='>') ++at;
        const auto name = raw.substr(name_begin,at-name_begin);
        need(!name.empty(), "Native UISkin empty raw node name");
        char quote = 0;
        for (; at < raw.size(); ++at) {
            const char c = raw[at];
            if (quote) { if (c == quote) quote = 0; }
            else if (c == '\'' || c == '"') quote = c;
            else if (c == '>') break;
        }
        need(at < raw.size() && !quote, "Native UISkin incomplete raw start/close tag");
        if (closing) {
            need(!stack.empty() && stack.back().node->name == name,
                 "Native UISkin raw close differs from SAME retained node");
            finish();
        } else {
            const content::XmlNode* node = nullptr;
            if (stack.empty()) {
                need(!root_seen && root.name == name, "Native UISkin raw root differs from SAME document");
                root_seen = true; node = &root;
            } else {
                auto& parent = stack.back();
                need(parent.child < parent.node->children.size(), "Native UISkin raw extra child");
                node = &parent.node->children[parent.child++];
                need(node->name == name, "Native UISkin raw child name/order differs from SAME DOM");
            }
            result.values.emplace(node,Value{});
            stack.push_back({node});
            if (at > p && raw[at-1]=='/') finish();
        }
        p = at+1;
    }
    need(root_seen && stack.empty(), "Native UISkin incomplete retained raw document");
    return result;
}
inline std::vector<std::string> getter_values(const content::XmlNode& occurrence,
        const LexicalDocument& lexical, std::string_view slot, std::string_view selector) {
    const content::XmlNode* first_slot = nullptr;
    for (const auto& node : occurrence.children) if (node.name == slot) { first_slot = &node; break; }
    std::vector<std::string> result;
    if (!first_slot) return result;
    for (const auto& leaf : first_slot->children) {
        if (leaf.name != selector) continue;
        std::string scratch; //595698..595703: once per matched terminal leaf.
        const auto read = [&](const content::XmlNode& node) {
            const auto found = lexical.values.find(&node);
            need(found != lexical.values.end(), "Native UISkin getter node has no SAME raw provenance");
            const auto& bytes = found->second.bytes;
            if (std::none_of(bytes.begin(),bytes.end(),[](unsigned char c){return c&0x80;})) scratch = bytes;
            result.push_back(scratch); //8352A0 failure retains prior File scratch.
        };
        // On this SAME parsed Source route, XmlLite appends only element
        // children. The proved native path also appends only kind1 elements;
        // comments, PI and CDATA produce no node. This is depth1 node count,
        // not a test of XmlLite's concatenated leaf.text field.
        if (leaf.children.empty()) read(leaf);
        else for (const auto& file : leaf.children) if (file.name == "File") read(file);
    }
    return result;
}
}

namespace civ5::ui::detail {
// Input phase of the existing private token's future consumer. There is no
// selected-skin load, namespace capability, enabled bit or resource publication.
class BnwNativeUiSkinSelectedSourceOwner final {
  public:
    ~BnwNativeUiSkinSelectedSourceOwner() = default;
    BnwNativeUiSkinSelectedSourceOwner(const BnwNativeUiSkinSelectedSourceOwner&) = delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Parent = ::civ5::ui::BnwNativePackageDiscoveryOwner;
    struct DescriptorSeed { std::size_t ordinal; std::string descriptor, bytes; const content::XmlNode* root;
        std::shared_ptr<const BnwNativeInitialDescriptorSource> actual_initial_source; };
    struct Descriptor { DescriptorSeed source; native_ui_skin_text_detail::LexicalDocument lexical; };
    std::weak_ptr<const Parent> parent_;
    std::weak_ptr<const BnwNativeUiSkinSelectedSourceOwner> self_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    std::shared_ptr<const void> epoch_;
    std::vector<std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence>> tokens_;
    std::vector<Descriptor> descriptors_;
    mutable BnwNativeCallbackOwnerState state_;
    mutable bool failed_ = false;
    BnwNativeUiSkinSelectedSourceOwner(std::weak_ptr<const Parent> parent,
        std::shared_ptr<const BnwNativeEarlyInputLease> lease, std::shared_ptr<const void> epoch,
        std::vector<std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence>> tokens,
        std::vector<DescriptorSeed> sources)
        : parent_(std::move(parent)), lease_(std::move(lease)), epoch_(std::move(epoch)), tokens_(std::move(tokens)) {
        for (auto& source : sources) {
            auto lexical = source.actual_initial_source?source.actual_initial_source->borrow_actual_native_values(*source.root):
                native_ui_skin_text_detail::retain_runs(*source.root,source.bytes);
            descriptors_.push_back({std::move(source),std::move(lexical)});
        }
    }
    static std::shared_ptr<const BnwNativeUiSkinSelectedSourceOwner> create(
        std::weak_ptr<const Parent> parent, std::shared_ptr<const BnwNativeEarlyInputLease> lease,
        std::shared_ptr<const void> epoch,
        std::vector<std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence>> tokens,
        std::vector<DescriptorSeed> sources) {
        native_ui_skin_text_detail::need(!parent.expired() && lease && epoch,
            "Native UISkin raw producer needs genuine retained Package/lease/epoch");
        auto out = std::shared_ptr<BnwNativeUiSkinSelectedSourceOwner>(new BnwNativeUiSkinSelectedSourceOwner(
            std::move(parent),std::move(lease),std::move(epoch),std::move(tokens),std::move(sources)));
        out->self_ = out; return out;
    }
    BnwNativeUiSkinSelectedSourceOwner(std::weak_ptr<const Parent> parent,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation,std::shared_ptr<const void> epoch,
        std::vector<std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence>> tokens,std::vector<DescriptorSeed> sources)
        :parent_(std::move(parent)),installation_input_(std::move(installation)),epoch_(std::move(epoch)),tokens_(std::move(tokens)){
        for(auto& source:sources){auto lexical=source.actual_initial_source?source.actual_initial_source->borrow_actual_native_values(*source.root):
                native_ui_skin_text_detail::retain_runs(*source.root,source.bytes);
            descriptors_.push_back({std::move(source),std::move(lexical)});}
    }
    static std::shared_ptr<const BnwNativeUiSkinSelectedSourceOwner> create(
        std::weak_ptr<const Parent> parent,std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation,
        std::shared_ptr<const void> epoch,std::vector<std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence>> tokens,
        std::vector<DescriptorSeed> sources){
        native_ui_skin_text_detail::need(!parent.expired()&&installation&&epoch,"Installed UISkin raw producer needs genuine Parent/input/epoch");
        auto out=std::shared_ptr<BnwNativeUiSkinSelectedSourceOwner>(new BnwNativeUiSkinSelectedSourceOwner(
            std::move(parent),std::move(installation),std::move(epoch),std::move(tokens),std::move(sources)));
        out->self_=out;return out;
    }
    std::vector<BnwNativeUiSkinTextDiagnostic> current_diagnostic() const {
        using namespace native_ui_skin_text_detail;
        BnwNativeCallbackOwnerGuard<BnwNativeUiSkinSelectedSourceOwner> fence(state_,self_);
        need(fence.valid(), "Native UISkin raw consumer foreign/reentered/retired");
        // Foreign callers return before any non-atomic owner state access.
        need(!failed_, "Native UISkin raw consumer failure is sticky");
        struct Failure {
            bool& failed; bool complete = false;
            ~Failure() { if (!complete) failed = true; }
        } failure{failed_};
        const auto parent = parent_.lock(); // Exactly ONE transient parent pin.
        const auto check = [&] { need(fence.valid_parent(parent), "Native UISkin raw consumer lost outside Package"); };
        check(); parent->require_native_ui_skin_retained_inputs(); check();
        std::vector<BnwNativeUiSkinTextDiagnostic> out;
        for (const auto& token : tokens_) {
            need(token && token->installation_input_.get()==installation_input_.get() &&
                 !(lease_&&installation_input_) && token->lease_.get()==lease_.get() && token->package_epoch_.get()==epoch_.get() &&
                 !token->parent_.owner_before(parent_) && !parent_.owner_before(token->parent_),
                 "Native UISkin raw consumer foreign Package/lease/epoch token");
            // PRE authenticated every installed token against the SAME retained DOM.
            // Existing fence/parent pins keep source and DOM alive through allocations
            // (including new_handler). Checks below and POST precede return of local out.
            // Imported tokens retain the live borrower and its nested-pin boundary.
            need(!installation_input_||token->node_, "Installed UISkin raw consumer lost retained node");
            const auto& node = installation_input_ ? *token->node_ : token->borrowed_node();
            check();
            const auto found = std::find_if(descriptors_.begin(),descriptors_.end(),
                [&](const auto& d){return d.source.ordinal==token->descriptor_;});
            need(found!=descriptors_.end() && found->lexical.values.contains(&node),
                 "Native UISkin raw consumer node has no SAME retained source witness");
            for (std::size_t slot=0;slot<2;++slot)
                for (const auto selector : {"Directory","Paks","File","Style","Color"})
                    out.push_back({found->source.descriptor,token->occurrence_,slot,selector,
                        getter_values(node,found->lexical,slot==0?"GameplaySkin":"Skin",selector)});
            check();
        }
        parent->require_native_ui_skin_retained_inputs(); check();
        failure.complete = true;
        return out;
    }
};
}
