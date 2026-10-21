#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <GL/glcorearb.h>
#include <memory>
#include <cstdint>
#include <string>

namespace civ5::graphics {
static_assert(sizeof(void *) == 8, "Civ V graphics require a 64-bit engine");

// Only the entry points consumed by this backend, loaded for its real context.
struct GlApi {
    PFNGLGETSTRINGPROC GetString;
    PFNGLGETSTRINGIPROC GetStringi;
    PFNGLGETINTEGERVPROC GetIntegerv;
    PFNGLGETERRORPROC GetError;
    PFNGLGETGRAPHICSRESETSTATUSPROC GetGraphicsResetStatus;
    PFNGLCREATETEXTURESPROC CreateTextures;
    PFNGLDELETETEXTURESPROC DeleteTextures;
    PFNGLISTEXTUREPROC IsTexture;
    PFNGLTEXTURESTORAGE2DPROC TextureStorage2D;
    PFNGLTEXTURESTORAGE3DPROC TextureStorage3D;
    PFNGLTEXTURESUBIMAGE2DPROC TextureSubImage2D;
    PFNGLTEXTURESUBIMAGE3DPROC TextureSubImage3D;
    PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC CompressedTextureSubImage2D;
    PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC CompressedTextureSubImage3D;
    PFNGLGETTEXTURESUBIMAGEPROC GetTextureSubImage;
    PFNGLGETTEXTUREIMAGEPROC GetTextureImage;
    PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC GetCompressedTextureImage;
    PFNGLGETCOMPRESSEDTEXIMAGEPROC GetCompressedTexImage;
    PFNGLGETTEXLEVELPARAMETERIVPROC GetTexLevelParameteriv;
    PFNGLBINDTEXTUREPROC BindTexture;
    PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC GetCompressedTextureSubImage;
    PFNGLGETTEXTURELEVELPARAMETERIVPROC GetTextureLevelParameteriv;
    PFNGLGETTEXTUREPARAMETERIVPROC GetTextureParameteriv;
    PFNGLTEXTUREPARAMETERIPROC TextureParameteri;
    PFNGLTEXTUREPARAMETERIVPROC TextureParameteriv;
    PFNGLPIXELSTOREIPROC PixelStorei;
    PFNGLBINDBUFFERPROC BindBuffer;
    PFNGLBINDTEXTUREUNITPROC BindTextureUnit;
};

struct GlContextState;
using GlProc = void(APIENTRY *)();
class GlTexture;
class BnwOwnedCameraRendererWindowLease;
namespace detail {class BnwRendererContextPin;}
// Identity/lifetime only: no liveness/reset/current-context readiness claim.
struct GlContextNativeIdentity {
    std::uintptr_t window=0,dc=0,context=0;
    bool owner_thread=false;
};
class GlContext {
  public:
    // Own window only. Hidden by default; never attaches to the original game.
    // Requires reset notifications; check() rejects a lost graphics context.
    explicit GlContext(int width = 64, int height = 64, bool visible = false);
    ~GlContext();
    GlContext(const GlContext &) = delete;
    GlContext &operator=(const GlContext &) = delete;
    const GlApi &api() const;
    GlProc proc_address(const char *name) const;
    void check(const char *operation) const;
    std::string description() const;
    bool has_extension(const char *name) const;
    // Host application owns message routing; this is this context's own HWND.
    void *native_window_handle() const;
    // Reads immutable owned handles/thread identity; NEVER binds/checks WGL.
    GlContextNativeIdentity native_identity() const noexcept;
    void present();

  private:
    friend class GlTexture;
    friend class BnwOwnedCameraRendererWindowLease;
    friend class detail::BnwRendererContextPin;
    std::shared_ptr<GlContextState> state_;
};
} // namespace civ5::graphics
