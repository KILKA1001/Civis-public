#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
std::int64_t half_floor(std::int64_t n){return n/2-(n<0&&n%2!=0?1:0);}
std::int64_t magnitude(std::int64_t n){return n<0?wide::subtract(0,n):n;}
std::int64_t coordinate(std::int64_t n,std::int64_t size,bool wrap){
    // CvMap::coordRange deliberately retains the negative-exact-multiple edge.
    if(wrap){if(n<0)return size+n%size;if(n>=size)return n%size;}return n;
}
std::int64_t difference(std::int64_t n,std::int64_t size,bool wrap){
    // Original helper adjusts only once; it does not take a modulo.
    if(wrap){if(n>size/2)return wide::subtract(n,size);if(n<-size/2)return wide::add(n,size);}return n;
}
}
void MapGeometry::validate()const{
    if((layout!=saves::GameStateLayout::expansion2_version1&&layout!=saves::GameStateLayout::vp542_corpus)||
       !grid.width||!grid.height||grid.size()>INT64_MAX)
        throw std::runtime_error("Invalid gameplay map geometry");
}
std::optional<std::int64_t> MapGeometry::plot(std::int64_t x,std::int64_t y)const{
    validate();if(x==-2147483647LL||y==-2147483647LL)return {};
    x=coordinate(x,static_cast<std::int64_t>(grid.width),wrap_x);
    y=coordinate(y,static_cast<std::int64_t>(grid.height),wrap_y);
    if(x<0||y<0||static_cast<std::uint64_t>(x)>=grid.width||static_cast<std::uint64_t>(y)>=grid.height)return {};
    return static_cast<std::int64_t>(grid.index(x,y));
}
std::optional<std::int64_t> MapGeometry::plot_by_index(std::int64_t index)const{
    validate();return index>=0&&static_cast<std::uint64_t>(index)<grid.size()?std::optional(index):std::nullopt;
}
std::optional<std::int64_t> MapGeometry::direction(std::int64_t x,std::int64_t y,std::int64_t dir)const{
    validate();
    if(layout==saves::GameStateLayout::vp542_corpus){
        dir=magnitude(dir%6);
        const auto centre=plot(x,y);if(!centre)return {};
        const auto xy=grid.coordinates(*centre);x=static_cast<std::int64_t>(xy[0]);y=static_cast<std::int64_t>(xy[1]);
    }else if(dir==-1)return plot(x,y);
    else if(dir<0||dir>=6)throw std::runtime_error("Invalid BNW plot direction");
    constexpr std::int64_t dx[]={0,1,1,0,-1,-1},dy[]={1,0,-1,-1,0,1};
    const auto hx=wide::add(wide::subtract(x,half_floor(y)),dx[dir]);
    y=wide::add(y,dy[dir]);return plot(wide::add(hx,half_floor(y)),y);
}
std::int64_t MapGeometry::distance(std::int64_t x1,std::int64_t y1,std::int64_t x2,std::int64_t y2)const{
    validate();const auto w=static_cast<std::int64_t>(grid.width),h=static_cast<std::int64_t>(grid.height);
    const auto hx1=wide::subtract(x1,half_floor(y1));
    if(layout==saves::GameStateLayout::vp542_corpus){
        const auto hx2=wide::subtract(x2,half_floor(y2));
        const auto delta=wide::subtract(hx2,hx1),ydelta=wide::subtract(y2,y1);
        const auto dx=magnitude(difference(delta,w,wrap_x));
        const auto dy=magnitude(difference(ydelta,h,wrap_y));
        const auto dz=magnitude(difference(wide::subtract(wide::subtract(0,delta),ydelta),w,wrap_x));
        // Divide without overflowing a representable final result.
        return wide::add(wide::add(dx/2,dy/2),wide::add(dz/2,(dx%2+dy%2+dz%2)/2));
    }
    const auto wx=difference(wide::subtract(x2,x1),w,wrap_x);
    const auto wy=difference(wide::subtract(y2,y1),h,wrap_y);
    const auto hx2=wide::subtract(wide::add(x1,wx),half_floor(wide::add(y1,wy)));
    const auto delta=wide::subtract(hx2,hx1);
    const auto dx=magnitude(difference(delta,w,wrap_x)),dy=magnitude(wy);
    return (delta>=0)==(wy>=0)?wide::add(dx,dy):std::max(dx,dy);
}
} // namespace civ5::runtime
