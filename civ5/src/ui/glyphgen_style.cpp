#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <charconv>
#include <algorithm>
#include <regex>
#include <stdexcept>

namespace civ5::ui {
namespace {
assets::MountedResource selected(const assets::ResourceMountTable &resources,std::string_view key) {
    const auto name=std::string(key);
    auto result=resources.resolve(assets::AssetKey(name));
    if(std::holds_alternative<std::monostate>(result.asset))result=resources.resolve(assets::AssetKey(assets::resource_basename(name)));
    if(!std::holds_alternative<assets::AssetHandle>(result.asset))throw std::runtime_error("GlyphGen style source is unavailable or ambiguous: "+std::string(key));
    return result;
}
struct Pair {
    assets::DdsData image,index;
    UiTextureAssetSource source,index_source;
};
Pair pair(const assets::ResourceMountTable &resources,std::string_view key) {
    auto image=selected(resources,key);
    const auto &handle=std::get<assets::AssetHandle>(image.asset);
    const auto name=handle.candidate().original_name;
    if(name.size()<4||assets::normalize_asset_name(name).substr(name.size()-4)!=".dds")throw std::runtime_error("GlyphGen layer is not DDS");
    auto index=selected(resources,name.substr(0,name.size()-4)+"-index.dds");
    const auto &ih=std::get<assets::AssetHandle>(index.asset);
    const auto &a=handle.candidate().provenance,&b=ih.candidate().provenance;
    if(image.mount_id!=index.mount_id||a.kind!=b.kind||a.source_id!=b.source_id||a.archive_relative_path!=b.archive_relative_path||
        assets::normalize_asset_name(ih.candidate().original_name)!=assets::normalize_asset_name(name.substr(0,name.size()-4)+"-index.dds"))
        throw std::runtime_error("GlyphGen style pair has mixed source identities");
    auto bytes=handle.read_bytes(64ull*1024*1024),ib=ih.read_bytes(64ull*1024*1024);
    UiTextureAssetSource source{handle.candidate(),image.mount_id,runtime::identify_bytes(bytes).sha256};
    UiTextureAssetSource isource{ih.candidate(),index.mount_id,runtime::identify_bytes(ib).sha256};
    auto result=Pair{assets::DdsData::parse(std::move(bytes)),assets::DdsData::parse(std::move(ib)),std::move(source),std::move(isource)};
    if(runtime::identify_bytes(handle.read_bytes()).sha256!=result.source.sha256||runtime::identify_bytes(ih.read_bytes()).sha256!=result.index_source.sha256)
        throw std::runtime_error("GlyphGen style pair changed during load");
    return result;
}
std::string text(const assets::ResourceMountTable &resources,std::string_view key) {
    const auto result=selected(resources,key);
    const auto bytes=std::get<assets::AssetHandle>(result.asset).read_bytes(64ull*1024*1024);
    return {bytes.begin(),bytes.end()};
}
std::optional<std::uint32_t> literal(std::string value) {
    std::replace(value.begin(),value.end(),'.',',');
    std::array<std::uint32_t,4> channels{};
    for(std::size_t n=0;n<4;++n) {
        const auto end=value.find(',');const auto token=value.substr(0,end);
        const auto parsed=std::from_chars(token.data(),token.data()+token.size(),channels[n]);
        if(parsed.ec!=std::errc{}||parsed.ptr!=token.data()+token.size()||channels[n]>255)return {};
        if(n==3) {if(end!=std::string::npos)return {};}else {if(end==std::string::npos)return {};value.erase(0,end+1);}
    }
    return (channels[0]<<24)|(channels[1]<<16)|(channels[2]<<8)|channels[3];
}
}
GlyphGenGeometry glyphgen_layer_geometry(const GlyphGenGeometry &layout,const GlyphGenAlphaAtlas &atlas) {
    if(layout.atlas_width!=atlas.width||layout.atlas_height!=atlas.height||layout.spacing<0||layout.spacing>32||
        atlas.alpha.size()!=std::uint64_t{atlas.width}*atlas.height)throw std::runtime_error("GlyphGen style atlas geometry mismatch");
    auto result=layout;
    for(auto &g:result.glyphs) {
        if(g.width==0||g.height==0)continue;
        const auto left=std::max(0,g.atlas_x-layout.spacing),top=std::max(0,g.atlas_y-layout.spacing);
        const auto right=std::min<std::int64_t>(atlas.width,std::int64_t{g.atlas_x}+g.width+layout.spacing);
        const auto bottom=std::min<std::int64_t>(atlas.height,std::int64_t{g.atlas_y}+g.height+layout.spacing);
        auto x0=right,y0=bottom;std::int64_t x1=left,y1=top;
        for(auto y=top;y<bottom;++y)for(auto x=left;x<right;++x)if(atlas.alpha[std::size_t{static_cast<std::uint32_t>(y)}*atlas.width+x]) {
            x0=std::min<std::int64_t>(x0,x);y0=std::min<std::int64_t>(y0,y);x1=std::max<std::int64_t>(x1,x+1);y1=std::max<std::int64_t>(y1,y+1);
        }
        if(x1<=x0||y1<=y0){g.width=0;g.height=0;continue;}
        g.origin_x+=static_cast<std::int32_t>(x0)-g.atlas_x;
        g.origin_y-=static_cast<std::int32_t>(y0)-g.atlas_y;
        g.atlas_x=static_cast<std::int32_t>(x0);g.atlas_y=static_cast<std::int32_t>(y0);
        g.width=static_cast<std::int32_t>(x1-x0);g.height=static_cast<std::int32_t>(y1-y0);
    }
    return result;
}
GlyphGenStyleMetadata read_glyphgen_style_metadata(
    const assets::ResourceMountTable &resources,
    std::string_view font_name, std::string_view style_name) {
    // Resolver callbacks may replace borrowed caller strings/storage.
    const std::string font_key(font_name), style_key(style_name);
    if (font_key != "TwCenMT14" && font_key != "TwCenMT16" &&
        font_key != "TwCenMT18" && font_key != "TwCenMT20" && font_key != "TwCenMT24")
        throw std::runtime_error("Requested authored font is outside the selected source profile");
    const auto mounted = selected(resources, "Assets/UI/Fonts/Tw Cent MT/" + font_key + ".ggxml");
    const auto handle = std::get<assets::AssetHandle>(mounted.asset);
    const auto bytes = handle.read_bytes(64ull * 1024 * 1024);
    UiTextureAssetSource source{handle.candidate(), mounted.mount_id, runtime::identify_bytes(bytes).sha256};
    auto font = GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(), bytes.end())));
    const auto style = std::find_if(font.styles.begin(), font.styles.end(),
        [&](const auto &s) { return s.name == style_key; });
    if (style == font.styles.end() || style->layers.empty() || style->layers.size() > 3)
        throw std::runtime_error("GlyphGen authored style is unavailable or outside the declared three-layer profile");
    const auto index = static_cast<std::size_t>(style - font.styles.begin());
    std::vector<GlyphGenDeclaredStyleLayer> layers;
    for (const auto &name : style->layers) {
        const auto texture = std::find_if(font.textures.begin(), font.textures.end(),
            [&](const auto &entry) { return entry.name() == name; });
        if (texture == font.textures.end())
            throw std::runtime_error("GlyphGen authored style references a missing texture");
        layers.push_back({name, texture->source(), static_cast<std::size_t>(texture - font.textures.begin())});
    }
    if (runtime::identify_bytes(handle.read_bytes(64ull * 1024 * 1024)).sha256 != source.sha256)
        throw std::runtime_error("GlyphGen authored metadata changed during load");
    return {std::move(font), std::move(source), index, std::move(layers)};
}
GlyphGenStyleAsset load_glyphgen_style(const assets::ResourceMountTable &resources,std::string_view font_name,std::string_view style_name) {
    const std::string font_key(font_name), style_key(style_name);
    const auto metadata = read_glyphgen_style_metadata(resources, font_key, style_key);
    // First declared layer provides logical layout; no Base-name alias or
    // native style fallback is inferred. Existing A8/L16 reconstruction guards
    // decide whether the actual packed texture profile is admissible.
    const auto &font = metadata.font;
    std::vector<GlyphGenStyleLayer> layers;
    std::optional<Pair> base;
    const auto layout=glyphgen_geometry(font);
    for(std::size_t n=0;n<metadata.layers.size();++n) {
        const auto &layer = metadata.layers[n];
        auto data=pair(resources,layer.texture_source);
        auto atlas=reconstruct_glyphgen_alpha(font,data.index,data.image);
        auto geometry=n==0?layout:glyphgen_layer_geometry(layout,atlas);
        layers.push_back({layer.name,n,std::move(atlas),std::move(geometry),data.source,data.index_source});
        if(n==0)base.emplace(std::move(data));
    }
    return {font,std::move(base->image),std::move(base->index),std::move(layers)};
}
std::optional<std::uint32_t> resolve_ui_label_color(const assets::ResourceMountTable &resources,std::string_view declaration) {
    if(declaration.empty())return {};
    if(auto direct=literal(std::string(declaration)))return direct;
    const auto split=declaration.find(',');
    const auto named=declaration.substr(0,split);
    const auto document=content::read_xml_metadata(text(resources,"Assets/UI/ColorAtlas.xml"));
    for(const auto &node:document.children)if(node.name=="Color"&&node.attribute("Name")==named) {
        auto color=literal(node.attribute("Color"));if(!color)return {};
        if(split==std::string_view::npos)return color;
        const auto alpha=declaration.substr(split+1);std::uint32_t channel{};
        const auto parsed=std::from_chars(alpha.data(),alpha.data()+alpha.size(),channel);
        if(parsed.ec!=std::errc{}||parsed.ptr!=alpha.data()+alpha.size()||channel>255)return {};
        return (*color&0xffffff00)|channel;
    }
    return {};
}
std::optional<std::uint32_t> city_name_shadow_color(const assets::ResourceMountTable &resources) {
    const auto source=text(resources,"Assets/UI/InGame/CityBannerManager.lua");
    // Pin the independently inspected script, rather than treating matching
    // text in comments or a rewritten control path as executable evidence.
    if(runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(source.data()),source.size())).sha256!=
        "c661ebcd04ead9aef5e30b5f8daef4498a7e51e8e8702b5ec21c778ad85b00d2")return {};
    const std::regex assignment(R"(local\s+textColorShadow\s*=\s*\{\s*x\s*=\s*0\s*,\s*y\s*=\s*0\s*,\s*z\s*=\s*0\s*,\s*w\s*=\s*0\.5\s*\})");
    const std::regex apply(R"(controls\.CityName\s*:\s*SetColor\s*\(\s*textColorShadow\s*,\s*1\s*\))");
    if(!std::regex_search(source,assignment)||!std::regex_search(source,apply))return {};
    return 0x00000080;
}
} // namespace civ5::ui
