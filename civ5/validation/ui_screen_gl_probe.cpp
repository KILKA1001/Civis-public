#include "ui_screen_gl.hpp"
#include "ui_glyphgen_styles.hpp"
#include "civ5/graphics/bnw_ui_source_observation.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/ui/glyphgen.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

std::shared_ptr<assets::LooseResourceIndex> loose(const fs::path &root,
                                                   const fs::path &directory) {
    auto result = std::make_shared<assets::LooseResourceIndex>(root);
    result->scan({directory, {}, false});
    return result;
}

assets::DdsData selected_dds(const assets::ResourceMountTable &resources,
                             std::string_view key) {
    const auto selected = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!asset) throw std::runtime_error("Missing active DDS: " + std::string(key));
    return assets::DdsData::parse(asset->read_bytes());
}

struct BoundFont {
    ui::GlyphGenGeometry geometry;
    graphics::GlTexture texture;
};

BoundFont font(const graphics::GlContext &context,
               const assets::ResourceMountTable &resources, int size) {
    const auto path = "Assets/UI/Fonts/Tw Cent MT/TwCenMT" + std::to_string(size) + ".ggxml";
    const auto source = ui::read_glyphgen(resources, path);
    const auto name = "TwCenMT" + std::to_string(size) + "Base";
    const auto texture = std::find_if(source.textures.begin(), source.textures.end(),
                                      [&](const auto &item) { return item.name() == name; });
    if (texture == source.textures.end()) throw std::runtime_error("GlyphGen Base layer is missing");
    const auto packed_key = texture->source();
    const auto index_key = packed_key.substr(0, packed_key.size() - 4) + "-index.dds";
    const auto atlas = ui::reconstruct_glyphgen_alpha(source, selected_dds(resources, index_key),
                                                      selected_dds(resources, packed_key));
    return {ui::glyphgen_geometry(source),
            graphics::GlTexture::alpha8(context, atlas.width, atlas.height, atlas.alpha)};
}

BoundFont icon_font(const graphics::GlContext &context,
                    const assets::ResourceMountTable &resources) {
    const auto source = ui::read_glyphgen(resources,
        "Assets/DLC/Expansion2/UI/Fonts/Font Icons/FontIcons.ggxml");
    const auto texture = std::find_if(source.textures.begin(), source.textures.end(),
        [](const auto &item) { return item.source() == "FontIcons.dds"; });
    if (texture == source.textures.end())
        throw std::runtime_error("Active BNW FontIcons layer is missing");
    const auto key = texture->source();
    const auto atlas = ui::reconstruct_glyphgen_rgba(source,
        selected_dds(resources, key.substr(0, key.size() - 4) + "-index.dds"),
        selected_dds(resources, key));
    const assets::Rgba8Level level{atlas.width, atlas.height, atlas.rgba};
    return {ui::glyphgen_geometry(source),
            graphics::GlTexture::rgba8(context, std::span<const assets::Rgba8Level>(&level, 1))};
}

std::unordered_map<std::string, std::uint32_t> icon_mapping(
    const assets::ResourceMountTable &resources) {
    const auto selected = resources.resolve(assets::AssetKey(
        "Assets/Gameplay/XML/GameInfo/CIV5IconFontMapping.xml"));
    const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!asset) throw std::runtime_error("Active icon mapping XML is missing");
    const auto bytes = asset->read_bytes();
    const auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
    const auto *table = document.child("IconFontMapping");
    if (!table) throw std::runtime_error("Active icon mapping XML has no table");
    std::unordered_map<std::string, std::uint32_t> result;
    for (const auto &row : table->children) {
        const auto *name = row.child("IconName"), *code = row.child("IconMapping");
        if (!name || !code) throw std::runtime_error("Incomplete icon mapping row");
        result.emplace(name->text, static_cast<std::uint32_t>(std::stoul(code->text)));
    }
    if (!result.contains("ICON_PLUS") || !result.contains("ICON_MINUS"))
        throw std::runtime_error("Active icon mapping lacks tenet markers");
    return result;
}

void screenshot(const graphics::GlContext &context, const fs::path &path, int width, int height) {
    const auto read_pixels = reinterpret_cast<PFNGLREADPIXELSPROC>(
        context.proc_address("glReadPixels"));
    const auto read_buffer = reinterpret_cast<PFNGLREADBUFFERPROC>(
        context.proc_address("glReadBuffer"));
    context.api().PixelStorei(GL_PACK_ALIGNMENT, 1);
    read_buffer(GL_BACK);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
    read_pixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    context.check("Read UI screen probe");
    const auto visible = std::count_if(pixels.begin(), pixels.end(),
                                       [](unsigned char value) { return value > 20; });
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        out.write(reinterpret_cast<const char *>(pixels.data() +
                  static_cast<std::size_t>(y) * width * 3), static_cast<std::streamsize>(width * 3));
    if (!out) throw std::runtime_error("Could not write GL UI probe frame");
    if (visible < 10000) throw std::runtime_error(
        "GL UI screen frame is blank: bright bytes=" + std::to_string(visible));
}

// Bounded own-client fixture: actual authored atlases, explicit fixture colors.
// This does not attest initialized original Label, CLASSIN or native pixels.
int authored_softshadow_probe(const graphics::GlContext &context,
    const assets::ResourceMountTable &resources, const fs::path &output,
    int width, int height) {
    const auto need=[](bool value,const std::string &reason) {
        if(!value) throw std::runtime_error(reason);
    };
    auto report_path=output;report_path+=".authored-softshadow.txt";
    need(!fs::exists(output) && !fs::exists(report_path),
        "Preserve existing authored SoftShadow fixture outputs");
    auto small=font(context,resources,16);
    auto regular=font(context,resources,20);
    auto title=font(context,resources,24);
    constexpr std::array<std::pair<std::string_view,std::string_view>,4> requests{{
        {"TwCenMT18","Shadow"},{"TwCenMT20","Stroke"},{"TwCenMT20","Shadow"},
        {"TwCenMT24","SoftShadow"}}};
    std::vector<ui::PreparedUiGlyphStyle> prepared;
    for(const auto &[name,style]:requests) {
        auto authored=ui::load_glyphgen_style(resources,name,style);
        const auto count=name=="TwCenMT24"?3u:2u;
        need(authored.layers.size()==count,"Actual authored layer count changed");
        // Same authored Base plane, without locale/GDI synthetic glyph insertion.
        auto atlas=authored.layers.front().atlas;
        auto geometry=authored.layers.front().geometry;
        prepared.push_back({std::string(name),std::string(style),std::move(authored),
            std::move(atlas),std::move(geometry)});
    }
    auto styles=std::make_shared<ui::UiGlyphGenStyleSet>(context,std::move(prepared));
    const auto *actual=styles->resolve("TwCenMT24","SoftShadow");
    need(actual && actual->layers.size()==3 && actual->layers[2].color_index==2,
        "Actual third alpha binding was not retained");
    // Invalid binding metadata borrows the SAME actual owner; no second cache.
    auto invalid=*actual;
    invalid.layers.back().color_index=3;
    bool invalid_index=false;
    ui::UiScreenGlRenderer painter(context,resources,
        {&small.geometry,&small.texture},{&regular.geometry,&regular.texture},{},{},
        {&title.geometry,&title.texture},
        [styles,&invalid,&invalid_index](std::string_view name,std::string_view style)
            -> const ui::UiGlyphStyleBinding* {
            if(invalid_index && name=="TwCenMT24" && style=="SoftShadow")
                return &invalid;
            return styles->resolve(name,style);
        });
    graphics::GlRenderApi gl(context);
    const auto read_pixels=reinterpret_cast<PFNGLREADPIXELSPROC>(context.proc_address("glReadPixels"));
    const auto read_buffer=reinterpret_cast<PFNGLREADBUFFERPROC>(context.proc_address("glReadBuffer"));
    const auto read=[&] {
        context.api().PixelStorei(GL_PACK_ALIGNMENT,1);
        read_buffer(GL_BACK);
        std::vector<unsigned char> bytes(static_cast<std::size_t>(width)*height*4);
        read_pixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());
        context.check("Read bounded authored SoftShadow fixture");
        return bytes;
    };
    const auto clear=[&] {
        gl.BindFramebuffer(GL_FRAMEBUFFER,0);
        gl.Viewport(0,0,width,height);
        gl.ClearColor(0.02f,0.03f,0.06f,1.f);
        gl.Clear(GL_COLOR_BUFFER_BIT);
        context.check("Clear bounded authored fixture");
    };
    clear();const auto blank=read();
    const std::string text="MWAA";
    const auto paint_for=[&](std::string_view name,std::string_view style,bool anchored) {
        ui::WorldViewPaint paint;
        paint.kind=ui::WorldViewPaintKind::text;
        paint.destination=anchored?ui::WorldViewRect{360,160,0,64}:
            ui::WorldViewRect{120,120,360,64};
        paint.text=text;
        paint.style=std::string(name)+(anchored?":right-center":"");
        paint.font_style=ui::UiFontStylePaint{std::string(name),std::string(style),
            {0xffffc8ffu,0x000000c8u,0xff0000ffu}};
        return paint;
    };
    const auto draw=[&](const ui::WorldViewPaint &paint) {
        clear();ui::ComposedUiFrame frame;frame.paints.push_back(paint);
        auto report=painter.paint(frame,width,height);
        return std::pair{std::move(report),read()};
    };
    const auto complete=[&](const ui::UiGlPaintReport &report,std::size_t glyphs,
        const std::string &phase) {
        need(report.unresolved.empty() && report.glyphs==glyphs && !report.icons &&
            !report.rectangles && !report.textures,phase+" glyph/draw count differs");
    };
    const auto unchanged_outside=[&](const auto &pixels,const ui::WorldViewRect &clip) {
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            if(x>=clip.x && x<clip.x+clip.width && y>=clip.y && y<clip.y+clip.height)
                continue;
            const auto at=(static_cast<std::size_t>(height-1-y)*width+x)*4;
            for(std::size_t c=0;c<4;++c)
                if(pixels[at+c]!=blank[at+c])
                    throw std::runtime_error("Authored glyph changed pixels outside explicit clip");
        }
    };
    std::ostringstream receipt;
    receipt<<"scope=own GL authored fixture; originalLabel/CLASSIN/nativePixelParity=false\n";
    std::vector<unsigned char> captured;
    for(const bool anchored:{false,true}) {
        const std::string phase=anchored?"anchored":"normal";
        auto paint=paint_for("TwCenMT24","SoftShadow",anchored);
        const auto [valid_report,valid_pixels]=draw(paint);
        const auto count=ui::project_glyphgen_line(*actual->layout_geometry,text).glyphs.size()*3;
        complete(valid_report,count,phase+" valid3");
        need(valid_pixels!=blank,phase+" valid third-layer frame is blank");
        receipt<<phase<<" valid3Glyphs="<<valid_report.glyphs<<'\n';
        // Float tint contains only slots0/1; third layer must use packedColor2.
        paint.font_style->float_colors=std::array<ui::UiFloatColor,2>{
            ui::UiFloatColor{1.f,1.f,0.8f,1.f},ui::UiFloatColor{0.f,0.f,0.f,0.8f}};
        const auto [float_report,float_pixels]=draw(paint);
        complete(float_report,count,phase+" float2/packed3");
        need(float_pixels!=blank,phase+" float2/packed3 frame is blank");
        paint.clip=anchored?ui::WorldViewRect{310,146,28,64}:
            ui::WorldViewRect{126,116,28,64};
        const auto [red_report,red]=draw(paint);
        need(red_report.unresolved.empty() && red_report.glyphs>0,
            phase+" clipped red fixture unresolved");
        paint.font_style->colors[2]=0x0000ffffu;
        const auto [blue_report,blue]=draw(paint);
        need(blue_report.unresolved.empty() && blue_report.glyphs==red_report.glyphs,
            phase+" changing third color changed glyph count");
        unchanged_outside(red,*paint.clip);unchanged_outside(blue,*paint.clip);
        std::size_t changed=0,directional=0;
        for(std::size_t at=0;at<red.size();at+=4)
            if(!std::equal(red.begin()+at,red.begin()+at+4,blue.begin()+at)) {
                ++changed;
                if(red[at]>blue[at] && blue[at+2]>red[at+2])++directional;
            }
        need(changed>0 && directional>0,
            phase+" actual third red/blue color did not change framebuffer channels");
        need(red!=float_pixels,phase+" explicit clip did not exclude any original ink");
        receipt<<phase<<" thirdColorChangedPixels="<<changed<<"; directionalRedBluePixels="
            <<directional<<"; outsideClipUnchanged=true; clipExcludesOriginalInk=true\n";
        paint.clip.reset();
        paint.font_style->colors[2].reset();
        const auto [missing_report,missing]=draw(paint);
        need(!missing_report.unresolved.empty() && missing_report.glyphs==0 &&
            !missing_report.icons && missing==blank,phase+" missing third color partially drew");
        paint.font_style->colors[2]=0xff0000ffu;
        invalid_index=true;
        const auto [invalid_report,invalid_pixels]=draw(paint);
        invalid_index=false;
        need(!invalid_report.unresolved.empty() && invalid_report.glyphs==0 &&
            !invalid_report.icons && invalid_pixels==blank,phase+" invalid third index partially drew");
        receipt<<phase<<" missingThird/invalidIndexRefusedBeforeDraw=true\n";
        if(!anchored)captured=valid_pixels;
        for(std::size_t n=0;n<3;++n) {
            auto legacy=paint_for(requests[n].first,requests[n].second,anchored);
            legacy.font_style->colors[2].reset();
            const auto [before_report,before]=draw(legacy);
            const auto *binding=styles->resolve(requests[n].first,requests[n].second);
            const auto legacy_count=ui::project_glyphgen_line(*binding->layout_geometry,text).glyphs.size()*2;
            complete(before_report,legacy_count,phase+" legacy2");
            need(before!=blank,phase+" legacy2 framebuffer blank");
            legacy.font_style->colors[2]=0x00ffffffu;
            const auto [after_report,after]=draw(legacy);
            complete(after_report,legacy_count,phase+" legacy2 unusedColor2");
            need(before==after,phase+" unused third color changed legacy two-layer readback");
            receipt<<phase<<" legacy="<<requests[n].first<<' '<<requests[n].second
                <<" glyphs="<<legacy_count<<" unusedColor2ReadbackUnchanged=true\n";
        }
    }
    std::ofstream image(output,std::ios::binary);
    image<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)for(int x=0;x<width;++x) {
        const auto at=(static_cast<std::size_t>(y)*width+x)*4;
        image.write(reinterpret_cast<const char*>(captured.data()+at),3);
    }
    need(static_cast<bool>(image),"Could not write bounded SoftShadow frame");
    std::ofstream report(report_path);report<<receipt.str();
    need(static_cast<bool>(report),"Could not write bounded SoftShadow receipt");
    std::cout<<receipt.str()<<"PASS own authored SoftShadow GL fixture; no native parity claim\n";
    return EXIT_SUCCESS;
}

// Controlled own-client commands, not live units or original Label receivers.
// Reuse the existing selected XML/color parser, GG style owner and GL painter.
int unit_name_font_selection_probe(const graphics::GlContext &context,
    const assets::ResourceMountTable &resources, const fs::path &output,
    int width, int height) {
    const auto need=[](bool value,const std::string &reason) {
        if(!value)throw std::runtime_error(reason);
    };
    auto report_path=output;report_path+=".unit-name-font-selection.json";
    need(!fs::exists(output)&&!fs::exists(report_path),"Preserve existing UnitName selection outputs");
    graphics::BnwUiSourceObservation source_observation;
    assets::ScopedResourceResolveObserver source_observer(resources,
        [&source_observation](const auto &key,auto results,bool all) {
            source_observation.observe(key,results,all);
        });
    const auto registry=ui::UiContextRegistry::from_resources(resources);
    const auto skin=ui::WorldViewSkin::from_resources(resources,registry);
    const auto *name=skin.find("UnitPanel","UnitName");
    const auto *button=skin.find("UnitPanel","UnitNameButton");
    const auto *descriptor=registry.find("UnitPanel");
    need(name&&button&&descriptor&&name->font=="TwCenMT24"&&
        name->font_style=="SoftShadow"&&button->size.x==280,
        "Selected UnitName authored style/button metadata differs");
    for(const auto color:name->font_colors)need(color.has_value(),"Selected UnitName color unresolved");
    std::vector<ui::PreparedUiGlyphStyle> prepared;
    for(const auto font_name:ui::unit_name_source_fonts) {
        auto authored=ui::load_glyphgen_style(resources,font_name,"SoftShadow");
        need(authored.layers.size()==3,"Source SoftShadow candidate must retain three authored layers");
        auto atlas=authored.layers.front().atlas;
        auto geometry=authored.layers.front().geometry;
        prepared.push_back({std::string(font_name),"SoftShadow",std::move(authored),
            std::move(atlas),std::move(geometry)});
    }
    auto styles=std::make_shared<ui::UiGlyphGenStyleSet>(context,std::move(prepared));
    const auto *actual24=styles->resolve("TwCenMT24","SoftShadow");
    const auto *actual20=styles->resolve("TwCenMT20","SoftShadow");
    need(actual24&&actual20,"Actual initial candidate bindings missing");
    const std::string text="WWWWWWWWWWWW";
    const auto w24=ui::project_glyphgen_line(*actual24->layout_geometry,text).advance_width;
    const auto w20=ui::project_glyphgen_line(*actual20->layout_geometry,text).advance_width;
    need(w24>w20&&w20>0&&std::int64_t{w24}+50<width-240,
        "Actual W-run fixture widths must distinguish candidates and fit this framebuffer");
    // Controlled command widths, not a claim that live UnitNameButton resized.
    const std::array<int,3> button_widths{w24+50,w20+50,w20+49};
    bool withhold14=false;
    const auto *actual14=styles->resolve("TwCenMT14","SoftShadow");
    need(actual14,"Actual final candidate binding missing");
    ui::UiScreenGlRenderer painter(context,resources,
        actual14->layers.front().glyphs,actual20->layers.front().glyphs,{},{},
        actual24->layers.front().glyphs,
        [styles,&withhold14](std::string_view font_name,std::string_view style)
            ->const ui::UiGlyphStyleBinding* {
            if(withhold14&&font_name=="TwCenMT14"&&style=="SoftShadow")return nullptr;
            return styles->resolve(font_name,style);
        });
    graphics::GlRenderApi gl(context);
    const auto read_pixels=reinterpret_cast<PFNGLREADPIXELSPROC>(context.proc_address("glReadPixels"));
    const auto read_buffer=reinterpret_cast<PFNGLREADBUFFERPROC>(context.proc_address("glReadBuffer"));
    const auto read=[&] {
        context.api().PixelStorei(GL_PACK_ALIGNMENT,1);read_buffer(GL_BACK);
        std::vector<unsigned char> bytes(static_cast<std::size_t>(width)*height*4);
        read_pixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());
        context.check("Read own UnitName font-selection fixture");return bytes;
    };
    const auto clear=[&] {
        gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.Viewport(0,0,width,height);
        gl.ClearColor(0.02f,0.03f,0.06f,1.f);gl.Clear(GL_COLOR_BUFFER_BIT);
        context.check("Clear own UnitName font-selection fixture");
    };
    clear();const auto blank=read();
    constexpr std::uint64_t fixture_frame=123;
    const auto command=[&](std::size_t n) {
        ui::WorldViewPaint p;p.kind=ui::WorldViewPaintKind::text;
        p.destination={120,120+static_cast<int>(n)*120,button_widths[n],64};
        p.text=text;p.style="TwCenMT24:center";
        p.font_style=ui::UiFontStylePaint{"TwCenMT24","SoftShadow",name->font_colors};
        p.unit_name_font_selection=ui::UnitNameOwnFontSelectionRequest{
            descriptor->source_key+".xml","UnitPanel","UnitName",text,"SoftShadow",
            0,1,fixture_frame,0,button_widths[n]};
        return p;
    };
    const auto draw=[&](const ui::WorldViewPaint &p) {
        clear();ui::ComposedUiFrame frame;frame.snapshot_frame=fixture_frame;
        frame.paints.push_back(p);auto report=painter.paint(frame,width,height);
        need(frame.paints.front().style=="TwCenMT24:center"&&
            frame.paints.front().font_style->font=="TwCenMT24"&&
            frame.paints.front().unit_name_font_selection->button_width==p.destination.width,
            "Painter mutated the published initial font/selection command");
        return std::pair{std::move(report),read()};
    };
    std::ostringstream commands;
    std::size_t total_glyphs=0;
    for(std::size_t n=0;n<3;++n) {
        auto p=command(n);const auto [report,pixels]=draw(p);
        need(report.unresolved.empty()&&report.unit_name_font_refusals.empty()&&
            report.unit_name_own_font_selections.size()==1&&!report.icons&&
            !report.rectangles&&!report.textures&&pixels!=blank,"Positive own title draw incomplete");
        const auto &selected=report.unit_name_own_font_selections.front();
        const std::array<std::size_t,2> expected_indices{0,n==0?0u:1u};
        const std::array<std::int32_t,2> expected_widths{w24,n==0?w24:w20};
        need(selected.selected_index==n&&selected.measured_indices==expected_indices&&
            selected.measured_widths==expected_widths&&report.glyphs==selected.draw_line.glyphs.size()*3,
            "Actual painter did not use ordered candidate measurement/draw geometry");
        total_glyphs+=report.glyphs;
        p.clip=ui::WorldViewRect{p.destination.x+p.destination.width/2,p.destination.y,
            p.destination.width-p.destination.width/2,p.destination.height};
        const auto [clipped_report,clipped]=draw(p);
        need(clipped_report.unresolved.empty()&&clipped_report.glyphs==report.glyphs&&
            clipped!=blank&&clipped!=pixels,"Explicit clip did not retain/exclude actual glyph ink");
        const auto &clip=*p.clip;
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            if(x>=clip.x&&x<clip.x+clip.width&&y>=clip.y&&y<clip.y+clip.height)continue;
            const auto at=(static_cast<std::size_t>(height-1-y)*width+x)*4;
            need(std::equal(clipped.begin()+at,clipped.begin()+at+4,blank.begin()+at),
                "Own title changed a framebuffer pixel outside actual clip");
        }
        if(n)commands<<',';
        commands<<"{\"expected_index\":"<<n<<",\"selected_index\":"<<selected.selected_index
            <<",\"button_width\":"<<button_widths[n]<<",\"measured_indices\":["
            <<selected.measured_indices[0]<<','<<selected.measured_indices[1]
            <<"],\"measured_widths\":["<<selected.measured_widths[0]<<','<<selected.measured_widths[1]
            <<"],\"draw_advance_width\":"<<selected.draw_line.advance_width
            <<",\"draw_glyphs\":"<<report.glyphs<<",\"clip\":["<<clip.x<<','<<clip.y<<','
            <<clip.width<<','<<clip.height<<"],\"outside_clip_unchanged\":true,\"clip_excludes_ink\":true}";
    }
    // Separate source-authored280 branch; controlled equality cases above
    // do not authenticate a native/current UnitNameButton width change.
    auto authored_button=command(2);authored_button.destination.y=480;
    authored_button.destination.width=button->size.x;
    authored_button.unit_name_font_selection->button_width=button->size.x;
    const auto [authored_report,authored_pixels]=draw(authored_button);
    need(authored_report.unresolved.empty()&&authored_report.unit_name_own_font_selections.size()==1&&
        authored_report.unit_name_own_font_selections.front().selected_index==2&&
        authored_report.unit_name_own_font_selections.front().measured_widths==
            std::array<std::int32_t,2>{w24,w20}&&
        authored_report.glyphs==text.size()*3&&authored_pixels!=blank,
        "Actual authored280 source button branch did not draw14 from actual W-run widths");
    total_glyphs+=authored_report.glyphs;
    const auto &authored_selection=authored_report.unit_name_own_font_selections.front();
    withhold14=true;
    const auto [negative,negative_pixels]=draw(authored_button);withhold14=false;
    need(negative.glyphs==0&&!negative.icons&&!negative.rectangles&&!negative.textures&&
        !negative.unresolved.empty()&&negative.unit_name_own_font_selections.empty()&&
        negative.unit_name_font_refusals.size()==1&&
        negative.unit_name_font_refusals.front().issue==ui::UnitNameOwnFontSelectionIssue::missing_candidate&&
        negative.unit_name_font_refusals.front().candidate_index==2&&negative_pixels==blank,
        "Missing required14 must refuse before every actual glyph draw");
    clear();ui::ComposedUiFrame composed;composed.snapshot_frame=fixture_frame;
    for(std::size_t n=0;n<3;++n)composed.paints.push_back(command(n));
    composed.paints.push_back(authored_button);
    const auto combined=painter.paint(composed,width,height);const auto captured=read();
    need(combined.unresolved.empty()&&combined.glyphs==total_glyphs&&
        combined.unit_name_own_font_selections.size()==4,"Combined own title fixture incomplete");
    source_observation.stop_recording();
    const auto source_json=source_observation.snapshot_json(source_observer.failed());
    // Exact fields emitted by this existing observer, not an external JSON parser.
    need(source_json.find("\"observer_failed\":false")!=std::string::npos&&
        source_json.find("\"immutable_recheck_passed\":true")!=std::string::npos,
        "Selected source read/recheck failed; refuse successful fixture publication");
    std::ofstream image(output,std::ios::binary);image<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)for(int x=0;x<width;++x) {
        const auto at=(static_cast<std::size_t>(y)*width+x)*4;
        image.write(reinterpret_cast<const char*>(captured.data()+at),3);
    }
    need(static_cast<bool>(image),"Could not write own title-selection frame");
    std::ofstream receipt(report_path);
    receipt<<"{\"schema\":\"civis-own-unit-name-font-selection-gl-v1\","
        <<"\"scope\":\"controlled-own-C++-commands-source-fonts\","
        <<"\"nativeGetSizeVal\":false,\"nativeLabelReady\":false,\"liveUnitReady\":false,"
        <<"\"currentLuaReceiverReady\":false,\"nativePixelParity\":false,"
        <<"\"fixture_snapshot_frame\":"<<fixture_frame<<",\"framebuffer\":["<<width<<','<<height<<"],"
        <<"\"selected_source_button_width\":"<<button->size.x<<",\"controlled_button_widths\":true,"
        <<"\"fonts\":[\"TwCenMT24\",\"TwCenMT20\",\"TwCenMT14\"],\"style\":\"SoftShadow\","
        <<"\"text\":\""<<text<<"\",\"commands\":["<<commands.str()<<"],"
        <<"\"actual_source_button_command\":{\"button_width\":"<<button->size.x
        <<",\"selected_index\":"<<authored_selection.selected_index
        <<",\"measured_indices\":["<<authored_selection.measured_indices[0]<<','<<authored_selection.measured_indices[1]
        <<"],\"measured_widths\":["<<authored_selection.measured_widths[0]<<','<<authored_selection.measured_widths[1]
        <<"],\"draw_glyphs\":"<<authored_report.glyphs<<"},"
        <<"\"missing_required_candidate\":{\"index\":2,\"button_width\":"<<button->size.x<<",\"draw_glyphs\":"<<negative.glyphs
        <<",\"typed_refusal\":true,\"blank_readback\":true},\"sources\":"<<source_json<<"}\n";
    need(static_cast<bool>(receipt),"Could not write own title-selection receipt");
    std::cout<<"PASS own UnitName GL font selection24/20/14 and missing required candidate; native Label/metrics=false\n";
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 3 && !(argc == 4 &&
            (std::string_view(argv[3]) == "--g0-warrior-portrait" ||
             std::string_view(argv[3]) == "--authored-softshadow" ||
             std::string_view(argv[3]) == "--unit-name-font-selection")))
            throw std::runtime_error(
                "Usage: ui_screen_gl_probe ASSETS OUT.ppm [--g0-warrior-portrait|--authored-softshadow|--unit-name-font-selection]");
        const bool portrait = argc == 4 && std::string_view(argv[3]) == "--g0-warrior-portrait";
        const bool authored_softshadow = argc == 4 && std::string_view(argv[3]) == "--authored-softshadow";
        const bool unit_name_selection = argc == 4 && std::string_view(argv[3]) == "--unit-name-font-selection";
        const fs::path root(argv[1]), output(argv[2]);
        auto base = loose(root, root / "Assets/UI");
        auto gameinfo = loose(root, root / "Assets/Gameplay/XML/GameInfo");
        auto expansion = loose(root, root / "Assets/DLC/Expansion/UI");
        auto bnw = loose(root, root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({0, 0, 0, assets::resource_lookup(gameinfo), true, true, {}, {}});
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
        const assets::AssetEnvironment environment(root);
        for (const auto &name : {"Resource/DX9/UITextures.fpk",
                                 "Resource/DX9/Expansion1UITextures.fpk",
                                 "Resource/DX9/Expansion2UITextures.fpk"}) {
            const auto opened = environment.open_fpk_archive(name);
            const auto *archive = std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
            if (!archive) throw std::runtime_error(std::string("Cannot open UI archive: ") + name);
            resources.mount({static_cast<std::uint64_t>(resources.mounts().size() + 3), 0, 0,
                             assets::resource_lookup(*archive), true, true, {}, {}});
        }
        constexpr int width = 1280, height = 960;
        graphics::GlContext context(width, height, false);
        graphics::GlRenderApi gl(context);
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(0, 0, width, height);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        gl.ClearColor(0.02f, 0.03f, 0.06f, 1.f);
        gl.Clear(GL_COLOR_BUFFER_BIT);
        if (authored_softshadow)
            return authored_softshadow_probe(context, resources, output, width, height);
        if (unit_name_selection)
            return unit_name_font_selection_probe(context, resources, output, width, height);
        auto small = font(context, resources, 16);
        auto regular = font(context, resources, 20);
        auto icons = icon_font(context, resources);
        const auto mapping = icon_mapping(resources);
        const auto resolver = [&mapping](std::string_view key) -> std::optional<std::uint32_t> {
            const auto it = mapping.find(std::string(key));
            return it == mapping.end() ? std::nullopt : std::optional<std::uint32_t>(it->second);
        };
        ui::UiScreenGlRenderer painter(context, resources,
            {&small.geometry, &small.texture}, {&regular.geometry, &regular.texture},
            {&icons.geometry, &icons.texture}, resolver);
        ui::ComposedUiFrame frame;
        frame.paints.push_back({ui::WorldViewPaintKind::rectangle,
            {240, 150, 800, 640}, {}, {}, {}, "Grid9DetailFive140"});
        frame.paints.push_back({ui::WorldViewPaintKind::texture,
            {512, 136, 256, 64}, "assets/UI/Art/Icons/Top512IconTrim.dds", {}, {}, {}});
        frame.paints.push_back({ui::WorldViewPaintKind::text,
            {280, 220, 720, 40}, {}, {}, "N/A", "TwCenMT20"});
        frame.paints.push_back({ui::WorldViewPaintKind::rectangle,
            {510, 710, 260, 44}, {}, {}, {}, "BaseButton"});
        frame.paints.push_back({ui::WorldViewPaintKind::text,
            {535, 717, 220, 28}, {}, {}, "Close", "TwCenMT16"});
        frame.paints.push_back({ui::WorldViewPaintKind::text,
            {280, 270, 720, 40}, {}, {}, "[ICON_PLUS] [ICON_MINUS]", "TwCenMT20"});
        if (portrait)
            frame.paints.push_back({ui::WorldViewPaintKind::texture,
                {32, 32, 256, 256}, "indexed:UnitPortraitsAncientMedieval.dds",
                {768, 0, 256, 256}, {}, "UnitPortrait"});
        const auto painted = painter.paint(frame, width, height);
        if (painted.rectangles < 2 || painted.textures < (portrait ? 2u : 1u) ||
            painted.glyphs < 5 ||
            painted.icons != 2 ||
            !painted.unresolved.empty()) {
            std::cerr << "Painted " << painted.rectangles << " grids, "
                      << painted.textures << " DDS, " << painted.glyphs << " glyphs, "
                      << painted.icons << " icons\n";
            for (const auto &item : painted.unresolved) std::cerr << item << '\n';
            throw std::runtime_error("Authored GL UI paint is incomplete");
        }
        screenshot(context, output, width, height);
        std::cout << "PASS GL UI screen painter: " << painted.rectangles << " grids, "
                  << painted.textures << " DDS, " << painted.glyphs << " glyphs, "
                  << painted.icons << " icons\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
