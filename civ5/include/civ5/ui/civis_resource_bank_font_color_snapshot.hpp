#pragma once
#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include "civ5/ui/glyphgen_style.hpp"
#include <atomic>
#include <memory>
#include <thread>
namespace civ5::ui {
struct CivisResourceBankFontColorBudget {
    std::uint64_t source_bytes=128ULL*1024*1024, alpha_bytes=128ULL*1024*1024,
                  retained_dds_bytes=128ULL*1024*1024;
    std::size_t sources=128, styles=64;
};
struct CivisResourceBankFontSource {
    std::string requested_key;
    assets::AssetCandidate candidate;
    runtime::FileIdentity payload_identity;
};
struct CivisResourceBankAuthoredFont {
    std::string font_key;
    GlyphGenFont authored;
};
struct CivisResourceBankAuthoredStyle {
    std::string font_key,style_key;
    GlyphGenStyleAsset asset;
};
// Actual authored CPU data, never a native completed bank, UIManager, Label,
// GDI/HFONT metric, extended-locale glyph or GPU publication certificate.
class CivisResourceBankFontColorSnapshot final {
 public:
    static std::shared_ptr<const CivisResourceBankFontColorSnapshot> capture(
        const std::shared_ptr<assets::CivisResourceBankOwner>&,
        const std::shared_ptr<const CivisResourceBankUiSnapshot>&,
        CivisResourceBankFontColorBudget={});
    ~CivisResourceBankFontColorSnapshot();
    CivisResourceBankFontColorSnapshot(const CivisResourceBankFontColorSnapshot&)=delete;
    CivisResourceBankFontColorSnapshot& operator=(const CivisResourceBankFontColorSnapshot&)=delete;
    std::shared_ptr<const CivisResourceBankFontColorSnapshot> retain_owner()const;
    void retire()const noexcept;
    void require_current()const; // Pure: creator, bank epoch, same UI lineage.
    void require_boundary()const; // Full source boundary; outside client callbacks.
    void require_same_bank(const assets::CivisResourceBankOwner&)const;
    void require_same_ui(const CivisResourceBankUiSnapshot&)const;
    const GlyphGenFont& font(std::string_view)const;
    const GlyphGenStyleAsset& style(std::string_view font,std::string_view style)const;
    const std::vector<CivisResourceBankAuthoredStyle>& styles()const;
    const std::optional<content::XmlNode>& color_atlas()const;
    std::optional<std::uint32_t> color(std::string_view declaration)const;
    // Named palette only: null means successful valid parse with absent name.
    // Missing/ambiguous resource, observer, parse or invalid row errors propagate.
    std::optional<std::uint32_t> strict_named_color(std::string_view token)const;
    const std::vector<CivisResourceBankFontSource>& source_records()const;
 private:
    struct Control;
    const std::shared_ptr<Control> control_;
    const std::shared_ptr<const assets::CivisResourceBankOwner> bank_;
    const std::shared_ptr<const CivisResourceBankUiSnapshot> ui_;
    const assets::CivisResourceBankCapability capability_;
    const std::vector<CivisResourceBankAuthoredFont> fonts_;
    const std::vector<CivisResourceBankAuthoredStyle> styles_;
    const std::optional<content::XmlNode> colors_;
    const std::vector<CivisResourceBankFontSource> sources_;
    std::weak_ptr<const CivisResourceBankFontColorSnapshot> canonical_self_;
    CivisResourceBankFontColorSnapshot(std::shared_ptr<Control>,
        std::shared_ptr<const assets::CivisResourceBankOwner>,
        std::shared_ptr<const CivisResourceBankUiSnapshot>,
        std::vector<CivisResourceBankAuthoredFont>,
        std::vector<CivisResourceBankAuthoredStyle>,std::optional<content::XmlNode>,
        std::vector<CivisResourceBankFontSource>);
};
}
