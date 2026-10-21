#include "civ5/graphics/gl_bnw_natural_wonder_anchor.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/content/manifest.hpp"
#include <climits>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::graphics {
namespace {
void require(bool valid,const char *reason) {
    if(!valid) throw std::runtime_error(reason);
}
struct Icon {
    std::string texture;
    ui::WorldViewRect crop;
    assets::DdsData dds;
    std::optional<assets::Rgba8Level> pixels;
};
void verify_natural_wonder_xml(const assets::ResourceMountTable &resources) {
    auto selection=resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/GenericWorldAnchor.xml"));
    const auto *asset=std::get_if<assets::AssetHandle>(&selection.asset);
    require(asset,"BNW GenericWorldAnchor XML is absent from active mounts");
    const auto bytes=asset->read_bytes(1024*1024);
    const auto root=content::read_xml_metadata(
        std::string(bytes.begin(),bytes.end()));
    require(root.name=="Context","BNW GenericWorldAnchor XML root changed");
    std::size_t matches=0;
    for(const auto &instance:root.children) {
        if(instance.name!="Instance" ||
           instance.attribute("Name")!="NaturalWonder") continue;
        for(const auto &anchor:instance.children) {
            if(anchor.name!="WorldAnchor" ||
               anchor.attribute("ID")!="Anchor") continue;
            for(const auto &icon:anchor.children)
                if(icon.name=="Image" && icon.attribute("ID")=="Icon" &&
                   icon.attribute("Size")=="64,64" &&
                   icon.attribute("Anchor")=="C,C" &&
                   icon.attribute("Offset")=="0,40") ++matches;
        }
    }
    require(matches==1,"BNW NaturalWonder authored icon geometry changed");
}
Icon feature_icon(std::int64_t feature,const content::RulesDatabase &rules,
                  const content::InfoTypeCatalog &types,
                  const assets::ResourceMountTable &resources) {
    require(feature>=0,"BNW natural wonder anchor has no feature ID");
    std::size_t matches=0;
    std::string type,atlas;
    std::int64_t portrait=-1;
    const auto feature_rows=rules.query_rows({
        "SELECT Type, PortraitIndex, IconAtlas FROM Features WHERE ID=? LIMIT 2",
        {feature}},[&](const content::RulesRowView &row) {
        ++matches;
        type=row.text("Type").value_or("");
        atlas=row.text("IconAtlas").value_or("");
        portrait=row.integer("PortraitIndex");
    },2);
    const auto *entry=types.find(type);
    require(feature_rows.all_succeeded() && matches==1 && entry &&
            entry->table=="Features" && entry->id==feature &&
            !atlas.empty() && portrait>=0,
            "BNW natural wonder feature or icon is unresolved in active rules");
    matches=0;
    std::string filename;
    std::int64_t columns=0,rows=0;
    const auto atlas_rows=rules.query_rows({
        "SELECT Filename, IconsPerRow, IconsPerColumn FROM IconTextureAtlases "
        "WHERE Atlas=? AND IconSize=64 LIMIT 2",{atlas}},
        [&](const content::RulesRowView &row) {
            ++matches;
            filename=row.text("Filename").value_or("");
            columns=row.integer("IconsPerRow");
            rows=row.integer("IconsPerColumn");
        },2);
    require(atlas_rows.all_succeeded() && matches==1 &&
            !filename.empty() && columns>0 && rows>0 &&
            columns<=INT32_MAX/64 && rows<=INT32_MAX/64 &&
            portrait<columns*rows,
            "BNW natural wonder 64px atlas is absent or invalid");
    const std::string source="Assets/UI/Art/Icons/"+filename;
    auto binding=assets::bind_ui_texture(resources,source);
    auto dds=std::move(binding.source);
    require(dds.face_count()==1,"BNW natural wonder DDS must be a 2D image");
    auto pixels=std::move(binding.expanded);
    require((pixels ? pixels->width : dds.width())==static_cast<std::uint64_t>(columns)*64 &&
            (pixels ? pixels->height : dds.height())==static_cast<std::uint64_t>(rows)*64,
            "BNW natural wonder DDS does not match active 64px atlas");
    return {source,{static_cast<std::int32_t>((portrait%columns)*64),
                    static_cast<std::int32_t>((portrait/columns)*64),64,64},
            std::move(dds),std::move(pixels)};
}
} // namespace

std::optional<ui::WorldViewPoint> project_bnw_natural_wonder_anchor(
    const terrain::WorldLayout &layout,
    const terrain::HeightOwnerParameters &height,
    const terrain::CameraFrameState &camera,
    std::int32_t plot_x,std::int32_t plot_y,
    std::int32_t width,std::int32_t viewport_height) {
    if(width<=0 || viewport_height<=0 || plot_x<0 || plot_y<0 ||
       plot_x>=layout.source_grid.width ||
       plot_y>=layout.source_grid.height || camera.view_count==0 ||
       camera.view_count>camera.views.size()) return std::nullopt;
    const auto &m=layout.source_grid.second_matrix;
    const auto &t=layout.transforms;
    const auto a=terrain::wrap_add(plot_x,terrain::wrap_neg(plot_y>>1));
    const auto c=terrain::wrap_neg(terrain::wrap_add(a,plot_y));
    const auto grid_x=terrain::wrap_add(terrain::wrap_add(
        terrain::wrap_add(terrain::wrap_mul(a,m[0]),m[12]),
        terrain::wrap_mul(c,m[8])),terrain::wrap_mul(plot_y,m[4]));
    const auto grid_y=terrain::wrap_add(terrain::wrap_add(
        terrain::wrap_add(terrain::wrap_mul(plot_y,m[5]),m[13]),
        terrain::wrap_mul(c,m[9])),terrain::wrap_mul(a,m[1]));
    const float x=static_cast<float>(grid_x),y=static_cast<float>(grid_y);
    const float z=-x-y;
    const float world_x=height.scale_x*(x*t[16]+z*t[24]+y*t[20]+t[28])-
                        height.center_x;
    const float world_y=height.scale_y*(x*t[17]+z*t[25]+y*t[21]+t[29])-
                        height.center_y;
    if(!std::isfinite(world_x) || !std::isfinite(world_y))
        return std::nullopt;
    for(std::uint32_t view=0;view<camera.view_count;++view) {
        const auto vp=terrain::matrix_multiply(camera.views[view],camera.projection,
            terrain::X87Precision::double_precision,terrain::MatrixPath::sse);
        const double point[4]{world_x,world_y,0.,1.};
        double clip[4]{};
        for(std::size_t column=0;column<4;++column)
            for(std::size_t row=0;row<4;++row)
                clip[column]+=point[row]*vp[row*4+column];
        const double w=clip[3];
        if(!std::isfinite(w) || w<=0 || clip[0]<-w || clip[0]>w ||
           clip[1]<-w || clip[1]>w || clip[2]<0 || clip[2]>w) continue;
        const double sx=(clip[0]/w+1.)*width*.5;
        const double sy=(1.-clip[1]/w)*viewport_height*.5;
        if(std::isfinite(sx) && std::isfinite(sy) &&
           sx>std::numeric_limits<std::int32_t>::min()+64. &&
           sx<std::numeric_limits<std::int32_t>::max()-64. &&
           sy>std::numeric_limits<std::int32_t>::min()+64. &&
           sy<std::numeric_limits<std::int32_t>::max()-64.)
            return ui::WorldViewPoint{
                static_cast<std::int32_t>(std::lround(sx)),
                static_cast<std::int32_t>(std::lround(sy))};
    }
    return std::nullopt;
}

GlBnwNaturalWonderAnchorBackend::GlBnwNaturalWonderAnchorBackend(
    const runtime::LiveSessionState &state,
    std::shared_ptr<GlContext> context,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const assets::ResourceMountTable &resources)
    :context_(std::move(context)) {
    require(static_cast<bool>(context_),
            "BNW natural wonder GL context is unavailable");
    const auto &map=state.world.map_header;
    require(state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            map.layout==saves::GameStateLayout::expansion2_version1 &&
            map.grid.width>0 && map.grid.height>0 &&
            map.grid.width<=INT32_MAX && map.grid.height<=INT32_MAX,
            "BNW natural wonder backend requires a complete BNW map");
    identity_={map.guid,map.grid.width,map.grid.height};
    context_->check("Prepare BNW natural wonder anchor resources");
    verify_natural_wonder_xml(resources);
    for(std::uint64_t plot=0;plot<map.grid.size();++plot) {
        const auto plan=runtime::plan_bnw_world_anchor_clear(state,plot);
        require(plan.valid,"BNW natural wonder backend found invalid plot");
        if(!plan.removal) continue;
        const auto &source=*plan.removal;
        require(source.old_anchor==0 && source.x>=0 && source.y>=0 &&
                source.x<=INT32_MAX && source.y<=INT32_MAX &&
                source.old_data<=INT32_MAX,
                "BNW anchor type needs another authored UI backend");
        const auto icon=feature_icon(source.old_data,rules,types,resources);
        auto resource=icon.pixels ? std::make_shared<GlTexture>(GlTexture::rgba8(
            *context_,std::span<const assets::Rgba8Level>(&*icon.pixels,1),
            DdsSamplingColor::linear)) : std::make_shared<GlTexture>(
                *context_,icon.dds,DdsSamplingColor::srgb);
        require(resource->belongs_to(*context_),
                "BNW natural wonder texture has no current GL owner");
        require(instances_.emplace(plot,Instance{
            static_cast<std::int32_t>(source.x),
            static_cast<std::int32_t>(source.y),
            static_cast<std::int32_t>(source.old_data),
            icon.texture,icon.crop,std::move(resource)}).second,
            "Duplicate BNW natural wonder anchor plot");
    }
    context_->check("Prepare BNW natural wonder anchor backend");
}

bool GlBnwNaturalWonderAnchorBackend::has_drawable(
    std::uint64_t plot,std::int32_t anchor,std::int32_t x,std::int32_t y) const {
    const auto found=instances_.find(plot);
    return context_ && anchor==0 && found!=instances_.end() &&
        found->second.x==x && found->second.y==y &&
        found->second.resource &&
        found->second.resource->belongs_to(*context_);
}

void GlBnwNaturalWonderAnchorBackend::remove_drawable(
    std::uint64_t plot,std::int32_t anchor,std::int32_t x,std::int32_t y,
    std::int32_t event_data) {
    require(event_data==-1 && has_drawable(plot,anchor,x,y),
            "BNW natural wonder drawable or GL resource is absent");
    context_->check("Remove BNW natural wonder anchor drawable");
    instances_.erase(plot);
    ++revision_;
}

GlBnwNaturalWonderFrame GlBnwNaturalWonderAnchorBackend::compose_frame(
    const terrain::WorldLayout &layout,
    const terrain::HeightOwnerParameters &height,
    const terrain::CameraFrameState &camera,
    std::int32_t width,std::int32_t viewport_height) const {
    require(width>0 && viewport_height>0 &&
            layout.source_grid.width==static_cast<std::int32_t>(identity_.width) &&
            layout.source_grid.height==static_cast<std::int32_t>(identity_.height),
            "BNW natural wonder camera belongs to another world");
    return compose_frame(width,viewport_height,[&](std::int32_t x,std::int32_t y) {
        return project_bnw_natural_wonder_anchor(
            layout,height,camera,x,y,width,viewport_height);
    });
}

GlBnwNaturalWonderFrame GlBnwNaturalWonderAnchorBackend::compose_frame(
    std::int32_t width,std::int32_t viewport_height,
    const ProjectPlot &project) const {
    require(width>0 && viewport_height>0 && project,
            "BNW natural wonder frame has no live plot projection");
    GlBnwNaturalWonderFrame frame;
    frame.revision=revision_;
    for(const auto &[plot,item]:instances_) {
        const auto screen=project(item.x,item.y);
        if(!screen) continue;
        require(screen->x>=0 && screen->x<width &&
                screen->y>=0 && screen->y<viewport_height,
                "BNW natural wonder projection exceeds viewport");
        // GenericWorldAnchor.xml: NaturalWonder/Icon Size=64,64,
        // Anchor=C,C, Offset=0,40.
        frame.overlay.paints.push_back({ui::WorldViewPaintKind::texture,
            {screen->x-32,screen->y+8,64,64},item.texture,item.crop,
            {},{},0xffffffff});
        frame.projected_plots.push_back(plot);
    }
    return frame;
}

void GlBnwNaturalWonderAnchorBackend::confirm_paint(
    const GlBnwNaturalWonderFrame &frame,std::size_t textures,
    std::span<const std::string> unresolved) {
    require(frame.revision==revision_ &&
            frame.projected_plots.size()==frame.overlay.paints.size() &&
            textures==frame.overlay.paints.size() && unresolved.empty(),
            "BNW natural wonder GL frame was not fully painted");
    context_->check("Paint BNW natural wonder world anchors");
    for(const auto plot:frame.projected_plots)
        require(instances_.contains(plot) &&
                instances_.at(plot).resource &&
                instances_.at(plot).resource->belongs_to(*context_),
                "BNW natural wonder frame contains an unowned anchor");
}

} // namespace civ5::graphics
