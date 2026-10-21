#include "civ5/terrain/river_network.hpp"
#include <set>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
constexpr std::array<std::array<int, 3>, 6> steps{
    {{0, 1, -1}, {-1, 0, 1}, {1, -1, 0}, {0, -1, 1}, {1, 0, -1}, {-1, 1, 0}}};
constexpr std::array<int, 12> reversed{9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2};
constexpr std::array<int, 12> forward_turn{4, 5, 3, 1, 2, 0, 11, 9, 10, 8, 6, 7};
constexpr std::array<int, 12> backward_turn{5, 3, 4, 2, 0, 1, 10, 11, 9, 7, 8, 6};
void validate(RiverEdge e) {
    require(e.direction >= 0 && e.direction < 12, "Invalid river edge direction");
    require(wrap_add(wrap_add(e.x, e.y), e.z) == 0, "Invalid river cube coordinates");
}
LogicalHex transform(RiverEdge e) {
    const auto m = unshifted_global_map_matrix({});
    const auto component = [&](int c) {
        return wrap_add(wrap_add(wrap_add(wrap_mul(e.z, m[8 + c]), wrap_mul(e.y, m[4 + c])), wrap_mul(e.x, m[c])),
                        m[12 + c]);
    };
    return {component(0), component(1), component(2)};
}
std::size_t slot(RiverEdge e) {
    const auto p = transform(e);
    const auto x = wrap_add(p.x, p.y >> 1);
    require(x >= 0 && x < 200 && p.y >= 0 && p.y < 200, "River edge exceeds native direction owner");
    return std::size_t(p.y) * 200 + x;
}
} // namespace
RiverNetwork::RiverNetwork(int width, int height) : width_(width), height_(height), directions_(40000) {}
RiverEdge RiverNetwork::opposite(RiverEdge e) const {
    validate(e);
    const auto s = steps[e.direction % 6];
    return {wrap_add(e.x, s[0]), wrap_add(e.y, s[1]), wrap_add(e.z, s[2]), reversed[e.direction]};
}
RiverEdge RiverNetwork::canonical(RiverEdge e) const {
    validate(e);
    return e.direction % 6 >= 3 ? opposite(e) : e;
}
RiverEdge RiverNetwork::normalized(RiverEdge e) const {
    validate(e);
    const auto p = transform(e);
    // Native signed IDIV after adding one period, not arbitrary floor modulo.
    const auto y = wrap_add(height_, p.y) % height_;
    const auto x = wrap_add(wrap_add(p.y >> 1, p.x), width_) % width_;
    const auto a = wrap_add(x, wrap_neg(y >> 1));
    const auto q = transform({a, y, wrap_neg(wrap_add(a, y)), e.direction});
    return {q.x, q.y, wrap_neg(wrap_add(q.x, q.y)), e.direction};
}
bool RiverNetwork::contains(RiverEdge e) const {
    e = normalized(canonical(e));
    return (directions_[slot(e)] & (1u << e.direction)) != 0;
}
std::optional<RiverEdge> RiverNetwork::next(RiverEdge e) const {
    validate(e);
    auto a = e.direction < 6 ? opposite(e) : e;
    auto b = e.direction >= 6 ? opposite(e) : e;
    a.direction = forward_turn[a.direction];
    b.direction = forward_turn[b.direction];
    if (contains(a))
        return a;
    if (contains(b))
        return b;
    return {};
}
std::optional<RiverEdge> RiverNetwork::previous(RiverEdge e) const {
    validate(e);
    auto a = e.direction >= 6 ? opposite(e) : e;
    auto b = e.direction < 6 ? opposite(e) : e;
    a.direction = backward_turn[a.direction];
    b.direction = backward_turn[b.direction];
    if (contains(a))
        return a;
    if (contains(b))
        return b;
    return {};
}
RiverNetwork RiverNetwork::build(int width, int height, std::span<const RiverEdge> edges) {
    // The original direction owner has 200x200 entries. The former 128 limit
    // described only the early synthetic fixtures, not this storage capacity.
    require(width >= 1 && width <= 200 && height >= 1 && height <= 200, "Invalid river source dimensions");
    require(edges.size() <= 1048576, "Excessive river edge input");
    RiverNetwork network(width, height);
    for (auto e : edges) {
        validate(e);
        // 0x657EB0 stores the supplied direction without reversing or wrapping.
        network.directions_[slot(e)] |= 1u << e.direction;
    }
    std::set<RiverEdge> visited;
    const auto key = [&](RiverEdge e) { return network.normalized(network.canonical(e)); };
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
            for (int direction : {0, 1, 2, 6, 7, 8}) {
                const auto a = wrap_add(x, wrap_neg(y >> 1));
                const auto p = transform({a, y, wrap_neg(wrap_add(a, y)), direction});
                RiverEdge e{p.x, p.y, wrap_neg(wrap_add(p.x, p.y)), direction};
                if (!network.contains(e) || visited.contains(key(e)))
                    continue;
                std::set<RiverEdge> backward_seen;
                for (;;) {
                    require(backward_seen.insert(key(e)).second, "Native river backtrace has a directed cycle");
                    const auto prior = network.previous(e);
                    if (!prior || visited.contains(key(*prior)))
                        break;
                    e = *prior;
                }
                RiverChain chain;
                for (;;) {
                    e = network.canonical(e);
                    if (!visited.insert(key(e)).second)
                        break;
                    chain.edges.push_back(e);
                    const auto next = network.next(e);
                    if (!next)
                        break;
                    e = *next;
                }
                require(!chain.edges.empty(), "Empty native river traversal");
                chain.terminal = !network.next(chain.edges.back()).has_value();
                network.chains_.push_back(std::move(chain));
            }
    return network;
}
} // namespace civ5::terrain
