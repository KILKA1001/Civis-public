#include "civ5/graphics/gl_bnw_natural_wonder_anchor.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "ui_screen_gl.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::vector<unsigned char> pixels(const graphics::GlContext &context) {
    auto read=reinterpret_cast<PFNGLREADPIXELSPROC>(
        context.proc_address("glReadPixels"));
    std::vector<unsigned char> result(128*128*4);
    read(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,result.data());
    context.check("Read Gandhi natural wonder frame");
    return result;
}
void clear(const graphics::GlContext &context) {
    graphics::GlRenderApi gl(context);
    gl.BindFramebuffer(GL_FRAMEBUFFER,0);
    gl.Viewport(0,0,128,128);
    gl.ClearColor(0,0,0,1);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    context.check("Clear Gandhi natural wonder frame");
}
std::size_t light(const std::vector<unsigned char> &pixels) {
    std::size_t result=0;
    for(std::size_t i=0;i<pixels.size();i+=4)
        result+=pixels[i]+pixels[i+1]+pixels[i+2];
    return result;
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root,
              const std::filesystem::path &asset_root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    assets::AssetEnvironment environment(asset_root);
    auto archive=environment.open_fpk_archive("Resource/DX9/UITextures.fpk");
    auto *fpk=std::get_if<std::shared_ptr<assets::FpkSource>>(&archive);
    check(fpk && *fpk,"Selected Gandhi UI texture archive unavailable");
    auto loose=environment.loose_source();
    assets::ResourceMountTable resources(assets::resource_lookup(loose),
        assets::resource_lookup_all(loose),assets::resource_keys(loose));
    resources.mount({1,1,0,assets::resource_lookup(*fpk),true,true,
        assets::resource_lookup_all(*fpk),assets::resource_keys(*fpk)});
    auto context=std::make_shared<graphics::GlContext>(128,128,false);
    auto backend=std::make_shared<graphics::GlBnwNaturalWonderAnchorBackend>(
        state,context,host.database(),host.types(),resources);
    auto scene=std::make_shared<graphics::GlBnwWorldAnchorScene>(
        state,context,backend);
    check(backend->retained_count()==1 &&
          scene->supports_gameplay_world_anchor(),
          "Gandhi NaturalWonder lacks a retained GL texture owner");
    const auto offscreen=backend->compose_frame(128,128,
        [](std::int32_t,std::int32_t)
            ->std::optional<ui::WorldViewPoint> { return std::nullopt; });
    check(offscreen.overlay.paints.empty() &&
          scene->supports_gameplay_world_anchor(),
          "Offscreen Gandhi anchor lost retained drawable readiness");
    auto offscreen_binding=runtime::bind_bnw_world_anchor_graphics(state,scene);
    check(offscreen_binding.ready(),
          "Offscreen Gandhi anchor did not bind to game event");
    auto offscreen_draft=state;
    runtime::BnwWorldAnchorJournal offscreen_journal(
        std::move(offscreen_binding.emitter));
    check(runtime::stage_bnw_world_anchor_clear(
              offscreen_draft,635,offscreen_journal).outcome==
              runtime::LiveActionOutcome::committed &&
          offscreen_journal.publish(offscreen_draft).status==
              runtime::BnwWorldAnchorPublicationStatus::complete &&
          backend->retained_count()==0 && scene->retained_anchor_count()==0 &&
          backend->compose_frame(128,128,
              [](std::int32_t,std::int32_t)
                  ->std::optional<ui::WorldViewPoint> {
                  return ui::WorldViewPoint{64,48};
              }).overlay.paints.empty(),
          "Post-commit offscreen anchor removal left a retained drawable");
    backend=std::make_shared<graphics::GlBnwNaturalWonderAnchorBackend>(
        state,context,host.database(),host.types(),resources);
    scene=std::make_shared<graphics::GlBnwWorldAnchorScene>(
        state,context,backend);
    ui::GlyphGenGeometry font;
    font.atlas_width=font.atlas_height=1;
    const std::array<std::uint8_t,1> alpha{255};
    auto font_texture=graphics::GlTexture::alpha8(*context,1,1,alpha);
    ui::UiScreenGlRenderer painter(*context,resources,
        {&font,&font_texture},{&font,&font_texture});
    const auto project=[](std::int32_t x,std::int32_t y)
        ->std::optional<ui::WorldViewPoint> {
        if(x==35 && y==15) return ui::WorldViewPoint{64,48};
        return std::nullopt;
    };
    const auto before=backend->compose_frame(128,128,project);
    check(before.overlay.paints.size()==1 &&
          before.projected_plots==std::vector<std::uint64_t>{635},
          "Selected Gandhi anchor did not enter the UI world frame");
    clear(*context);
    const auto report=painter.paint(before.overlay,128,128);
    backend->confirm_paint(before,report.textures,report.unresolved);
    const auto drawn=pixels(*context);
    check(light(drawn)>0 && scene->supports_gameplay_world_anchor(),
          "Gandhi natural wonder DDS was not drawn into GL framebuffer");
    // NotificationScience is actually BC004; NaturalWonders256 is COLOR DDS
    // and must retain its ordinary DXT5 storage without requiring an index.
    const auto natural_icon=assets::bind_ui_texture(
        resources,before.overlay.paints.front().source_key);
    check(!natural_icon.expanded && !natural_icon.index &&
          assets::ui_texture_storage_profile(natural_icon.source)==
              assets::UiTextureStorageProfile::interface_color,
          "Actual COLOR natural wonder DDS incorrectly requires an index");
    ui::ComposedUiFrame indexed_frame;
    indexed_frame.paints.push_back({ui::WorldViewPaintKind::texture,
        {0,0,128,128},"assets/UI/Art/WorldView/NotificationIconsScience.dds",
        {0,0,128,128},{},{},0xffffffff});
    const auto &paint=indexed_frame.paints.front();
    const auto icon=assets::bind_ui_texture(resources,paint.source_key);
    check(icon.expanded && icon.index_identity && icon.index &&
          assets::has_ui_block_atlas_header(icon.source),
          "Selected authored notification atlas was sampled without its index");
    assets::ResourceMountTable missing_index([&](const assets::AssetKey &key) {
        if(assets::resource_basename(key.original_name()).ends_with("-index.dds"))
            return assets::ResourceMountHit{};
        return assets::ResourceMountHit{resources.resolve(key).asset,true};
    });
    bool rejected=false;
    try { (void)assets::bind_ui_texture(missing_index,paint.source_key); }
    catch(const std::exception &) { rejected=true; }
    check(rejected,"Tagged notification atlas fell back to raw DDS without its index");
    clear(*context);
    const auto indexed_report=painter.paint(indexed_frame,128,128);
    check(indexed_report.textures==1 && indexed_report.unresolved.empty(),
          "Actual indexed notification texture did not paint");
    const auto indexed_pixels=pixels(*context);
    std::size_t opaque=0;
    for(std::int32_t y=0;y<paint.source.height;++y)
        for(std::int32_t x=0;x<paint.source.width;++x) {
            const auto src=(std::size_t(paint.source.y+y)*icon.width()+paint.source.x+x)*4;
            if(icon.expanded->rgba[src+3]!=255) continue;
            const auto dx=paint.destination.x+x,dy=paint.destination.y+y;
            if(dx<0 || dx>=128 || dy<0 || dy>=128) continue;
            const auto dst=(std::size_t(127-dy)*128+dx)*4;
            for(std::size_t channel=0;channel<3;++channel)
                check(indexed_pixels[dst+channel]==icon.expanded->rgba[src+channel],
                      "Notification GL crop differs from reconstructed authored pixels");
            ++opaque;
        }
    check(opaque>100,"Notification pixel check lacks opaque authored coverage");
    auto binding=runtime::bind_bnw_world_anchor_graphics(state,scene);
    check(binding.ready(),"Painted Gandhi anchor did not bind to game event");
    auto draft=state;
    runtime::BnwWorldAnchorJournal journal(std::move(binding.emitter));
    check(runtime::stage_bnw_world_anchor_clear(draft,635,journal).outcome==
          runtime::LiveActionOutcome::committed,
          "Gandhi anchor clear could not stage");
    check(journal.publish(draft).status==
          runtime::BnwWorldAnchorPublicationStatus::complete &&
          backend->retained_count()==0 && scene->retained_anchor_count()==0,
          "Post-commit Gandhi anchor removal did not reach drawable owner");
    const auto after=backend->compose_frame(128,128,project);
    check(after.overlay.paints.empty(),
          "Removed Gandhi anchor remained in the next frame");
    clear(*context);
    const auto empty_report=painter.paint(after.overlay,128,128);
    backend->confirm_paint(after,empty_report.textures,
                           empty_report.unresolved);
    check(light(pixels(*context))==0,
          "Removed Gandhi anchor left pixels in the next GL frame");
    std::cout << "PASS Gandhi NaturalWonder COLOR paint/offscreen/next-frame removal; actual BC004 notification opaque pixels and missing-index rejection\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==4,"Supply selected Gandhi BNW save, game root and shared asset root");
        selected(argv[1],argv[2],argv[3]);
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL Gandhi NaturalWonder GL: " << error.what() << '\n';
        return 1;
    }
}
