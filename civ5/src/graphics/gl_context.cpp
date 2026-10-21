#include "gl_context_internal.hpp"
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool extension(const GlApi &gl, const char *name) {
    GLint count = 0;
    gl.GetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i)
        if (std::strcmp(reinterpret_cast<const char *>(gl.GetStringi(GL_EXTENSIONS, i)), name) == 0)
            return true;
    return false;
}
} // namespace
GlContextState::~GlContextState() {
    // WGL and owned resources are render-thread affine, including destruction.
    if (std::this_thread::get_id() != owner)
        std::terminate();
    if (context) {
        if (wglGetCurrentContext() == context)
            wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(context);
    }
    if (dc)
        ReleaseDC(window, dc);
    if (window)
        DestroyWindow(window);
    if (!window_class.empty())
        UnregisterClassW(window_class.c_str(), module);
}
void GlContextState::current() const {
    require(std::this_thread::get_id() == owner,
            "Civ V OpenGL objects belong to their creating render thread");
    require(context != nullptr, "No live Civ V OpenGL context");
    if (wglGetCurrentContext() != context || wglGetCurrentDC() != dc)
        require(wglMakeCurrent(dc, context) != FALSE, "Cannot make Civ V context current");
}
GlProc GlContextState::address(const char *name) const {
    auto value = wglGetProcAddress(name);
    const auto bits = reinterpret_cast<std::uintptr_t>(value);
    if (bits == 0 || bits == 1 || bits == 2 || bits == 3 || bits == ~std::uintptr_t{0})
        value = GetProcAddress(GetModuleHandleW(L"opengl32.dll"), name);
    if (!value)
        throw std::runtime_error(std::string("Required OpenGL entry point missing: ") + name);
    return reinterpret_cast<GlProc>(value);
}
void GlContextState::check(const char *operation) const {
    const auto reset = gl.GetGraphicsResetStatus();
    if (reset != GL_NO_ERROR) {
        std::ostringstream message;
        message << operation << ": graphics reset 0x" << std::hex << reset;
        throw std::runtime_error(message.str());
    }
    const auto code = gl.GetError();
    if (code != GL_NO_ERROR) {
        std::ostringstream message;
        message << operation << ": OpenGL error 0x" << std::hex << code;
        throw std::runtime_error(message.str());
    }
}
GlContext::GlContext(int width, int height, bool visible) : state_(std::make_shared<GlContextState>()) {
    require(width > 0 && height > 0 && width <= 16384 && height <= 16384, "Invalid window dimensions");
    auto &s = *state_;
    s.module = GetModuleHandleW(nullptr);
    s.window_class = L"Civ5CppGL-" + std::to_wstring(reinterpret_cast<std::uintptr_t>(&s));
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = s.module;
    wc.lpszClassName = s.window_class.c_str();
    require(RegisterClassW(&wc) != 0, "Cannot register own Civ V OpenGL window");
    s.window = CreateWindowExW(0, s.window_class.c_str(), L"Civ5 C++ OpenGL", WS_POPUP, 0, 0, width, height,
                               nullptr, nullptr, s.module, nullptr);
    require(s.window != nullptr, "Cannot create own Civ V OpenGL window");
    s.context_gui_client_creation_=s.window; // Actual created client HWND, never a viewport DTO.
    s.dc = GetDC(s.window);
    require(s.dc != nullptr, "Cannot get own window DC");
    PIXELFORMATDESCRIPTOR format{};
    format.nSize = sizeof(format);
    format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 32;
    format.cAlphaBits = 8;
    format.cDepthBits = 24;
    format.cStencilBits = 8;
    const int pixel_format = ChoosePixelFormat(s.dc, &format);
    require(pixel_format != 0 && SetPixelFormat(s.dc, pixel_format, &format) != FALSE,
            "Cannot set own OpenGL pixel format");
    s.context = wglCreateContext(s.dc);
    require(s.context != nullptr, "Cannot bootstrap WGL");
    s.current();
    using CreateContext = HGLRC(WINAPI *)(HDC, HGLRC, const int *);
    const auto create = reinterpret_cast<CreateContext>(s.address("wglCreateContextAttribsARB"));
    // WGL_CONTEXT_MAJOR/MINOR_VERSION_ARB, PROFILE_MASK_ARB,
    // CORE_PROFILE_BIT_ARB, RESET_NOTIFICATION_STRATEGY_ARB and
    // LOSE_CONTEXT_ON_RESET_ARB. The measured material-cache profile requires
    // this explicit context contract. A reset must become an error, rather than
    // allow subsequent readbacks to be trusted.
    const int attributes[] = {0x2091, 4, 0x2092, 6, 0x9126, 1, 0x8256, 0x8252, 0};
    const auto core = create(s.dc, nullptr, attributes);
    require(core != nullptr, "OpenGL 4.6 Core is required; no silent compatibility fallback");
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(s.context);
    s.context = core;
    s.current();
#define LOAD(name) s.gl.name = reinterpret_cast<decltype(s.gl.name)>(s.address("gl" #name))
    LOAD(GetString);
    LOAD(GetStringi);
    LOAD(GetIntegerv);
    LOAD(GetError);
    LOAD(GetGraphicsResetStatus);
    LOAD(CreateTextures);
    LOAD(DeleteTextures);
    LOAD(IsTexture);
    LOAD(TextureStorage2D);
    LOAD(TextureStorage3D);
    LOAD(TextureSubImage2D);
    LOAD(TextureSubImage3D);
    LOAD(CompressedTextureSubImage2D);
    LOAD(CompressedTextureSubImage3D);
    LOAD(GetTextureSubImage);
    LOAD(GetCompressedTextureSubImage);
    LOAD(GetTextureLevelParameteriv);
    LOAD(GetTextureParameteriv);
    LOAD(GetTextureImage);
    LOAD(GetCompressedTextureImage);
    LOAD(GetCompressedTexImage);
    LOAD(GetTexLevelParameteriv);
    LOAD(BindTexture);
    LOAD(TextureParameteri);
    LOAD(TextureParameteriv);
    LOAD(PixelStorei);
    LOAD(BindBuffer);
    LOAD(BindTextureUnit);
#undef LOAD
    GLint major = 0, minor = 0, profile = 0;
    s.gl.GetIntegerv(GL_MAJOR_VERSION, &major);
    s.gl.GetIntegerv(GL_MINOR_VERSION, &minor);
    s.gl.GetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    require((major > 4 || (major == 4 && minor >= 6)) && (profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0,
            "Created context is not OpenGL 4.6 Core");
    GLint reset_strategy = 0;
    s.gl.GetIntegerv(GL_RESET_NOTIFICATION_STRATEGY, &reset_strategy);
    require(reset_strategy == GL_LOSE_CONTEXT_ON_RESET, "Graphics reset notifications are required");
    s.s3tc = extension(s.gl, "GL_EXT_texture_compression_s3tc");
    s.check("Create C++ context");
    if (visible)
        ShowWindow(s.window, SW_SHOWNOACTIVATE);
}
GlContext::~GlContext() {
    state_->facade_retired_.store(true);
    if(std::this_thread::get_id()==state_->owner&&state_->renderer_creator_cleanup_)
        state_->renderer_creator_cleanup_();
}
GlContextNativeIdentity GlContext::native_identity() const noexcept {
    return {reinterpret_cast<std::uintptr_t>(state_->window),
            reinterpret_cast<std::uintptr_t>(state_->dc),
            reinterpret_cast<std::uintptr_t>(state_->context),
            std::this_thread::get_id()==state_->owner};
}
void *GlContext::native_window_handle() const {
    state_->current();
    return state_->window;
}
void GlContext::present() {
    state_->current();
    require(SwapBuffers(state_->dc) != FALSE, "Cannot present own Civ V window");
    state_->check("Present C++ frame");
}
const GlApi &GlContext::api() const {
    state_->current();
    return state_->gl;
}
GlProc GlContext::proc_address(const char *name) const {
    state_->current();
    return state_->address(name);
}
void GlContext::check(const char *operation) const {
    state_->current();
    state_->check(operation);
}
bool GlContext::has_extension(const char *name) const { return extension(api(), name); }
std::string GlContext::description() const {
    const auto &gl = api();
    std::ostringstream out;
    out << gl.GetString(GL_VENDOR) << " | " << gl.GetString(GL_RENDERER) << " | " << gl.GetString(GL_VERSION)
        << " | x64";
    return out.str();
}
} // namespace civ5::graphics
