#include "civ5/ui/ui_quad_clip.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5::ui;
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
// A small alpha glyph with asymmetric rows and transparent edges. Compare
// cropped drawing with rendering the unchanged full glyph under a pixel mask.
// This checks ink continuity at all scroll edges, not just rectangle values.
std::vector<int> render(const UiClippedQuad &quad,const WorldViewRect &mask) {
    constexpr int width=12,height=12;
    constexpr int atlas_width=8,atlas_height=8;
    std::vector<int> pixels(width*height,0);
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
        if(x<mask.x||y<mask.y||x>=mask.x+mask.width||y>=mask.y+mask.height||
           x<quad.destination.x||y<quad.destination.y||
           x>=quad.destination.x+quad.destination.width||y>=quad.destination.y+quad.destination.height)continue;
        const double u=quad.uv[0]+quad.uv[2]*(x+.5-quad.destination.x)/quad.destination.width;
        const double v=quad.uv[1]+quad.uv[3]*(y+.5-quad.destination.y)/quad.destination.height;
        const auto sx=static_cast<int>(std::floor(u*atlas_width));
        const auto sy=static_cast<int>(std::floor(v*atlas_height));
        if(sx>=0&&sx<atlas_width&&sy>=0&&sy<atlas_height)
            pixels[y*width+x]=(sx==1||sy==1)?0:1+sy*atlas_width+sx;
    }
    return pixels;
}
void compare_edges() {
    const UiClippedQuad glyph{{2,2,8,8},{0,0,1,1}};
    const WorldViewRect viewport{0,0,12,12};
    for(const WorldViewRect clip:std::array<WorldViewRect,6>{{{5,0,7,12},{0,5,12,7},{0,0,6,12},
                                 {0,0,12,6},{4,3,3,4},{0,0,12,12}}}) {
        const auto cropped=clip_ui_quad(glyph.destination,glyph.uv,clip);
        check(bool(cropped),"Partially visible glyph was discarded");
        const auto actual=render(*cropped,viewport),expected=render(glyph,clip);
        check(actual==expected,"Glyph edge cropping shifted/rescaled atlas ink or alpha");
        check(std::any_of(actual.begin(),actual.end(),[](int pixel){return pixel!=0;}),"Partial glyph produced no ink");
    }
    // Successive scroll offsets must preserve the row-local texture coordinates.
    // The viewport exposes a changing slice of the same row, not a squashed row.
    for(int offset=0;offset<8;++offset) {
        const UiClippedQuad row{{2,2-offset,8,8},{0,0,1,1}};
        const WorldViewRect scroll_panel{2,2,8,5};
        const auto visible=clip_ui_quad(row.destination,row.uv,scroll_panel);
        check(bool(visible),"Visible scroll edge disappeared");
        check(render(*visible,viewport)==render(row,scroll_panel),"Scroll boundary rebased row layout or texture mapping");
    }
    const UiClippedQuad mirrored{{2,2,8,8},{1,0,-1,1}};
    const WorldViewRect clip{4,3,4,4};
    check(render(*clip_ui_quad(mirrored.destination,mirrored.uv,clip),viewport)==render(mirrored,clip),"Mirrored texture mapping changed during crop");
}
}
int main() {try {
    compare_edges();
    check(!clip_ui_quad({0,0,2,2},{0,0,1,1},{2,0,2,2}),"Touching invisible edge was painted");
    check(!clip_ui_quad({0,0,2,2},{0,0,1,1},{0,0,0,2}),"Empty clip was painted");
    check(!clip_ui_quad({0,0,2,2},{0,0,std::numeric_limits<float>::infinity(),1},{0,0,2,2}),"Nonfinite UV was painted");
    const auto high=std::numeric_limits<std::int32_t>::max();
    check(bool(clip_ui_quad({high-3,high-3,8,8},{0,0,1,1},{high-1,high-1,2,2})),"Rectangle edge arithmetic overflowed");
    std::cout<<"PASS UI partial glyph and scroll-edge pixel crops\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
