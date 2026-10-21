#include "civ5/terrain/scene_direction.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
constexpr std::array<std::uint8_t,256> permutation{
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};
// XY components of the original twelve 3D gradient vectors, in table order.
constexpr std::array<std::array<int,2>,12> gradients{{
    {1,1},{-1,1},{1,-1},{-1,-1},{1,0},{-1,0},
    {1,0},{-1,0},{0,1},{0,-1},{0,1},{0,-1}}};
void require(bool value,const char *message) { if(!value) throw std::invalid_argument(message); }
void validate(Point2 point) {
    require(std::isfinite(point.x) && std::isfinite(point.y) &&
        std::abs(point.x)<=65536 && std::abs(point.y)<=65536,"Scene direction coordinates exceed supported range");
}
void validate(X87Precision precision) {
    require(precision==X87Precision::single || precision==X87Precision::double_precision,
        "Unsupported scene direction precision");
}
int lattice(float value) {
    // Original signed 2^23 add/subtract rounds to nearest-even before its
    // conditional subtraction. Keep the operations, including signed zero.
    const float large=std::copysign(8388608.f,value);
    const float rounded=(value+large)-large;
    const float floor=rounded-((rounded-value)>std::copysign(0.f,value)?1.f:0.f);
    require(floor>=0 && floor<262144,"Scene direction would address before the permutation table");
    return static_cast<int>(floor);
}
float contribution(float x,float y,unsigned gradient) {
    const float radius=.5f-(y*y+x*x);
    if(radius<0) return 0;
    const auto &g=gradients[gradient];
    const float squared=radius*radius;
    const float dot=float(g[1])*y+float(g[0])*x;
    return (dot*squared)*squared;
}
} // namespace
float initial_scene_direction_angle(X87Precision precision) {
    validate(precision);
    const double squared=std::bit_cast<double>(std::uint64_t{0x4005851ea7ae1480});
    const float length=static_cast<float>(round_x87(std::sqrt(squared),precision));
    const float y=1.f/length,x=y*1.3f;
    return static_cast<float>(std::atan2(double(y),double(x)));
}
double scene_direction_noise(Point2 point,X87Precision precision) {
    validate(point);validate(precision);
    const float root=static_cast<float>(std::sqrt(3.));
    const float skew=((root-1.f)*.5f)*(point.x+point.y);
    const float unskew=(3.f-root)*std::bit_cast<float>(0x3e2aaaabu);
    const int i=lattice(point.x+skew),j=lattice(point.y+skew);
    const float origin=float(i+j)*unskew;
    const float x=point.x-(float(i)-origin),y=point.y-(float(j)-origin);
    const int ix=x>y?1:0,iy=x>y?0:1;
    const float x1=(x-float(ix))+unskew,y1=(y-float(iy))+unskew;
    const float tail=unskew*2.f-1.f;
    const float x2=tail+x,y2=tail+y;
    const auto gradient=[&](int a,int b) { return permutation[(a+permutation[b%256])%256]%12; };
    const float n0=contribution(x,y,gradient(i,j));
    const float n1=contribution(x1,y1,gradient(i+ix,j+iy));
    const float n2=contribution(x2,y2,gradient(i+1,j+1));
    // Original sums the third contribution first and scales in x87.
    const auto q=[&](double v){return round_x87(v,precision);};
    return q(q(q(double(n2)+n1)+n0)*70.);
}
Point2 SceneDirectionField::direction(Point2 point,X87Precision precision) const {
    validate(point);validate(offset);validate(precision);
    require(std::isfinite(angle) && std::abs(angle)<=65536,"Invalid scene direction angle");
    const float coarse=std::bit_cast<float>(0x3bda740eu),fine=.025f;
    const auto q=[&](double v){return round_x87(v,precision);};
    const auto a=scene_direction_noise({point.x*coarse+offset.x,point.y*coarse+offset.y},precision);
    const float first=static_cast<float>(q(a*std::bit_cast<float>(0x3ec90fdbu)));
    const auto b=scene_direction_noise({point.x*fine+offset.x,point.y*fine+offset.y},precision);
    const double phase=q(q(q(b*std::bit_cast<float>(0x3e490fdbu))+first)+angle);
    return {static_cast<float>(q(std::cos(phase)*64.)),static_cast<float>(q(std::sin(phase)*64.))};
}
} // namespace civ5::terrain
