#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::runtime {class BnwSelectedWorldViewContextOwner;class BnwWorldViewNativeContextIssuer;}
namespace civ5::runtime::detail {
// Passive immutable source metadata. Nothing here is an active child list,
// native parent/initializer observation, LayoutOperandLoan or numeric grant.
class BnwWorldViewCanvasXmlMetadata final {
    friend class ::civ5::runtime::BnwSelectedWorldViewContextOwner;
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Seed {std::string context,stem,payload_sha256;};
    struct Document {
        std::string context;
        assets::AssetHandle handle;
        std::optional<std::uint64_t> mount_id;
        std::shared_ptr<const assets::Bytes> bytes;
        FileIdentity payload_identity;
        content::XmlNode layout;
    };
    struct Entry {
        std::size_t document;
        std::vector<std::size_t> child_path;
        bool inside_template;
        // An absent value is DECLARATION ONLY, not missing native child/NULL.
        std::optional<std::size_t> expanded_document;
    };
    const std::vector<Document> documents_;
    const std::vector<Entry> ordered_entries_;
    BnwWorldViewCanvasXmlMetadata(std::vector<Document>,std::vector<Entry>);
    static std::shared_ptr<const BnwWorldViewCanvasXmlMetadata> capture_owned(
        const content::XmlNode& actual_ingame,std::span<const std::uint8_t> actual_ingame_bytes,
        const std::array<Seed,3>& actual_selected_children,
        const assets::ResourceMountTable&,const BnwStockLuaAttemptLease&);
    const Document& document_owned(std::string_view)const;
    const content::XmlNode& declaration_owned(const Entry&)const;
    void require_all_declarations_expanded_owned()const;
public:
    BnwWorldViewCanvasXmlMetadata(const BnwWorldViewCanvasXmlMetadata&)=delete;
    BnwWorldViewCanvasXmlMetadata&operator=(const BnwWorldViewCanvasXmlMetadata&)=delete;
};
}
