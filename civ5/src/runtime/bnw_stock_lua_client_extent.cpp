#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::runtime {
BnwStockLuaClientExtentProvider bnw_stock_lua_win32_client_extent(std::uintptr_t window,std::string owner,std::function<std::uint64_t()> revision){
    if(!window||owner.empty()||!revision)throw std::invalid_argument("Stock Lua client extent requires an owned window/surface/revision provider");
    return {[window,owner=std::move(owner),revision=std::move(revision)]()->std::optional<BnwStockLuaClientExtent>{
        auto handle=reinterpret_cast<HWND>(window);DWORD process=0;
        if(!IsWindow(handle)||!GetWindowThreadProcessId(handle,&process)||process!=GetCurrentProcessId())return std::nullopt;
        const auto before=revision();RECT rect{};if(!GetClientRect(handle,&rect))return std::nullopt;const auto after=revision();
        const auto width=static_cast<std::int64_t>(rect.right)-rect.left,height=static_cast<std::int64_t>(rect.bottom)-rect.top;
        if(before!=after||width<=0||height<=0)return std::nullopt;
        return BnwStockLuaClientExtent{owner,after,width,height};
    }};
}
}
