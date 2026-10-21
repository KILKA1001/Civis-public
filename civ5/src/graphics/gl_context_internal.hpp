#pragma once
#include "civ5/graphics/gl_context.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <thread>
#include <atomic>
#include <windows.h>

namespace civ5::graphics {
struct GlContextState {
  private:
    friend class GlContext;
    friend class BnwOwnedCameraRendererWindowLease;
    friend class detail::BnwRendererContextPin;
    // One actual Context strong member per sealed Pin object, creator only.
    std::size_t renderer_context_pins_=0;
    // Actual noncopyable facade destructor witness, independent of any shared
    // pointer proxy supplied by the caller. Textures may retain physical state.
    std::atomic<bool> facade_retired_{false};
    HWND context_gui_client_creation_=nullptr;
    // Installed only by the closed Window issuer; no app callback setter.
    void (*renderer_creator_cleanup_)()noexcept=nullptr;
  public:
    std::thread::id owner = std::this_thread::get_id();
    HINSTANCE module = nullptr;
    std::wstring window_class;
    HWND window = nullptr;
    HDC dc = nullptr;
    HGLRC context = nullptr;
    GlApi gl{};
    bool s3tc = false;
    ~GlContextState();
    void current() const;
    GlProc address(const char *name) const;
    void check(const char *operation) const;
};
} // namespace civ5::graphics
