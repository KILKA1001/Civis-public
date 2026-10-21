#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/ui_quad_clip.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool condition,const char *why){if(!condition)throw std::runtime_error(why);}
template<class F>void rejects(F action){try{action();}catch(const std::exception&){return;}throw std::runtime_error("Expected authored style rejection missing");}
void fixture() {
    ui::GlyphGenGeometry layout;layout.atlas_width=8;layout.atlas_height=8;layout.spacing=2;layout.line_height=5;
    layout.glyphs.push_back({65,3,3,1,1,0,4,0,0,1});
    ui::GlyphGenAlphaAtlas mask{8,8,1,1,64,0,assets::Bytes(64,0)};
    mask.alpha[2*8+2]=128;mask.alpha[3*8+3]=255;mask.alpha[4*8+4]=64;
    const auto effect=ui::glyphgen_layer_geometry(layout,mask);
    const auto original=ui::project_glyphgen_line(layout,"AA"),styled=ui::project_glyphgen_line(effect,"AA");
    check(original.advance_width==styled.advance_width&&styled.glyphs[0].width==3&&styled.glyphs[0].height==3,
        "Actual effect ink was cropped to Base or changed logical glyph advance");
    check(styled.glyphs[0].left_from_line_origin==-1&&styled.glyphs[0].top_from_baseline==2&&
          styled.glyphs[1].left_from_line_origin==3,"Effect ink bounds changed the text pen or baseline");
    const auto &g=styled.glyphs[0];
    const auto visible=ui::clip_ui_quad({g.left_from_line_origin,5-g.top_from_baseline,g.width,g.height},
        {float(g.atlas_x)/8,float(g.atlas_y)/8,float(g.width)/8,float(g.height)/8},{0,0,8,8});
    check(visible&&visible->destination.width==2&&visible->uv[0]==3.f/8,
        "Effect ink under partial clipping lost atlas alignment");
    auto runtime=layout;runtime.glyphs.push_back({0x414,0,6,1,1,0,4,0,0,1});
    check(ui::project_glyphgen_line(runtime,"Д").glyphs.size()==1,"Runtime Cyrillic Base glyph was lost");
    rejects([&]{ui::project_glyphgen_line(effect,"Д");});
    auto mismatch=mask;mismatch.height=9;rejects([&]{ui::glyphgen_layer_geometry(layout,mismatch);});
}
void actual(const std::filesystem::path &root) {
    auto loose=std::make_shared<assets::LooseResourceIndex>(root);
    for(const auto *file:{"Assets/UI/Fonts/Tw Cent MT/TwCenMT18.ggxml","Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml",
        "Assets/UI/ColorAtlas.xml","Assets/DLC/Expansion2/UI/InGame/CityBannerManager.lua"})loose->add_file(root/file);
    auto archive=assets::FpkSource::open(root/"Resource/DX9/UITextures.fpk","Resource/DX9/UITextures.fpk");
    auto fpk=std::get<std::shared_ptr<assets::FpkSource>>(archive);
    assets::ResourceMountTable resources(assets::resource_lookup(loose));resources.mount({1,0,0,assets::resource_lookup(fpk),true,true,{},{}});
    const auto shadow=ui::load_glyphgen_style(resources,"TwCenMT18","Shadow");
    const auto stroke=ui::load_glyphgen_style(resources,"TwCenMT20","Stroke");
    check(shadow.layers.size()==2&&shadow.layers[0].name=="TwCenMT18Base"&&shadow.layers[1].name=="TwCenMT18Shadow"&&
        stroke.layers[1].name=="TwCenMT20Stroke","Declared authored style layers changed");
    check(runtime::identify_bytes(shadow.layers[1].atlas.alpha).sha256=="a7b0ca60ff867e23ea87d9f68e567da6ebb3e643bc6bd622ae8536d43779abe7"&&
        runtime::identify_bytes(stroke.layers[1].atlas.alpha).sha256=="ad11b4aaf99ecaa7d9aadb25dc1c6e783ddf633e089ac22e3136af43f59f96fc",
        "Actual authored effect pixels differ from independent FPK reconstruction");
    for(const auto *style:{&shadow,&stroke}) {
        check(style->layers[0].source.sha256!=style->layers[1].source.sha256&&style->layers[1].color_index==1,
            "Authored effect was substituted with Base or foreground color");
        const auto base=ui::project_glyphgen_line(style->layers[0].geometry,"Delhi 1");
        const auto effect=ui::project_glyphgen_line(style->layers[1].geometry,"Delhi 1");
        check(base.advance_width==effect.advance_width,"Source effect layer changes text layout width");
        check(std::any_of(effect.glyphs.begin(),effect.glyphs.end(),[&](const auto &g){
            const auto b=std::find_if(base.glyphs.begin(),base.glyphs.end(),[&](const auto &x){return x.code_point==g.code_point;});
            return b!=base.glyphs.end()&&(g.width>b->width||g.height>b->height);
        }),"Authored effect's surrounding ink was not restored");
    }
    check(ui::resolve_ui_label_color(resources,"255,255,200,255")==0xffffc8ff&&
        ui::resolve_ui_label_color(resources,"Black,255")==0x000000ff&&
        ui::city_name_shadow_color(resources)==0x00000080,"Selected XML/ColorAtlas/Lua layer colors were not preserved");
    check(!ui::resolve_ui_label_color(resources,"Black,256")&&!ui::resolve_ui_label_color(resources,"Unknown"),"Unknown source color was guessed");
    rejects([&]{ui::load_glyphgen_style(resources,"TwCenMT18","Unknown");});
    rejects([&]{ui::load_glyphgen_style(resources,"TwCenMT12","Shadow");});
}
}
int main(int argc,char **argv){try{
    fixture();if(argc>1)actual(argv[1]);
    std::cout<<"PASS authored GlyphGen style ink, layout and selected source masks\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
