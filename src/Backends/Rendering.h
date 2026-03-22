// Released under the MIT licence.
// See LICENCE.txt for details.

#pragma once

#include <stddef.h>

typedef struct RenderBackend_Surface RenderBackend_Surface;
typedef struct RenderBackend_GlyphAtlas RenderBackend_GlyphAtlas;

typedef struct RenderBackend_Rect
{
	long left;
	long top;
	long right;
	long bottom;
} RenderBackend_Rect;
typedef struct RenderBackend_DRect
{
    double left;
    double top;
    double right;
    double bottom;
} RenderBackend_DRect;


RenderBackend_Surface* RenderBackend_Init(const char *window_title, size_t screen_width, size_t screen_height, bool fullscreen);
void RenderBackend_Deinit(void);
void RenderBackend_DrawScreen(void);
RenderBackend_Surface* RenderBackend_CreateSurface(size_t width, size_t height, bool render_target);
void RenderBackend_FreeSurface(RenderBackend_Surface *surface);
bool RenderBackend_IsSurfaceLost(RenderBackend_Surface *surface);
void RenderBackend_RestoreSurface(RenderBackend_Surface *surface);
void RenderBackend_UploadSurface(RenderBackend_Surface *surface, const unsigned char *pixels, size_t width, size_t height);
void RenderBackend_MakeEveryPixelWhiteExceptTheOnesThatHaveAlphaEqualsZero(RenderBackend_Surface *surf);
void RenderBackend_Blit(RenderBackend_Surface *source_surface, const RenderBackend_Rect *rect, RenderBackend_Surface *destination_surface, long x, long y, bool colour_key);
void RenderBackend_BlitInterpolate(RenderBackend_Surface *source_surface,RenderBackend_Surface *white_surface, const RenderBackend_DRect *rect, RenderBackend_Surface *destination_surface, double x, double y, double scale, int cR, int cG, int cB, float pct);
void RenderBackend_BlitEx(RenderBackend_Surface *source_surface, const RenderBackend_DRect *rect, RenderBackend_Surface *destination_surface, double x, double y, bool colour_key, double scale, int cR, int cG, int cB);
void RenderBackend_ColourFill(RenderBackend_Surface *surface, const RenderBackend_Rect *rect, unsigned char red, unsigned char green, unsigned char blue);
RenderBackend_GlyphAtlas* RenderBackend_CreateGlyphAtlas(size_t width, size_t height);
void RenderBackend_DestroyGlyphAtlas(RenderBackend_GlyphAtlas *atlas);
void RenderBackend_UploadGlyph(RenderBackend_GlyphAtlas *atlas, size_t x, size_t y, const unsigned char *pixels, size_t width, size_t height, size_t pitch);
void RenderBackend_PrepareToDrawGlyphs(RenderBackend_GlyphAtlas *atlas, RenderBackend_Surface *destination_surface, unsigned char red, unsigned char green, unsigned char blue);
void RenderBackend_DrawGlyph(long x, long y, size_t glyph_x, size_t glyph_y, size_t glyph_width, size_t glyph_height);
void RenderBackend_HandleRenderTargetLoss(void);
void RenderBackend_HandleWindowResize(size_t width, size_t height);
void RenderBackend_ClearTexture(RenderBackend_Surface *surface);