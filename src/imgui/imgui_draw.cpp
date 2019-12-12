// dear imgui, v1.68 WIP
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas
// [SECTION] ImFontAtlas glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include "string"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <stdio.h>      // vsnprintf, sscanf, printf
#if !defined(alloca)
#if defined(__GLIBC__) || defined(__sun) || defined(__CYGWIN__) || defined(__APPLE__)
#include <alloca.h>     // alloca (glibc uses <alloca.h>. Note that Cygwin may have _WIN32 defined, so the order matters here)
#elif defined(_WIN32)
#include <malloc.h>     // alloca
#if !defined(alloca)
#define alloca _alloca  // for clang with MS Codegen
#endif
#else
#include <stdlib.h>     // alloca
#endif
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
#endif
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"                  // warning : possible misuse of comma operator here             //
#endif
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"      // warning : macro name is a reserved identifier                //
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImGuiStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wcast-qual"              // warning : cast from 'const xxxx *' to 'xxx *' drops const qualifier //
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wcast-qual"                // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STBRP_ASSERT(x)     IM_ASSERT(x)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)   ((void)(u), ImGui::MemAlloc(x))
#define STBTT_free(x,u)     ((void)(u), ImGui::MemFree(x))
#define STBTT_assert(x)     IM_ASSERT(x)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloorStd(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImGuiStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.0f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.0f,  1.0f,   0.0f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGui::StyleColorsClassic(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// Those light colors are better suited with a thicker font than the default one + FrameBorder
void ImGui::StyleColorsLight(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.90f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    Font = NULL;
    FontSize = 0.0f;
    CurveTessellationTol = 0.0f;
    ClipRectFullscreen = ImVec4(-8192.0f, -8192.0f, +8192.0f, +8192.0f);

    // Const data
    for (int i = 0; i < IM_ARRAYSIZE(CircleVtx12); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(CircleVtx12);
        CircleVtx12[i] = ImVec2(ImCos(a), ImSin(a));
    }
}

void ImDrawList::Clear()
{
    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _ChannelsCurrent = 0;
    _ChannelsCount = 1;
    // NB: Do not clear channels so our allocations are re-used after the first frame.
}

void ImDrawList::ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _ChannelsCurrent = 0;
    _ChannelsCount = 1;
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == 0) memset(&_Channels[0], 0, sizeof(_Channels[0]));  // channel 0 is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i].CmdBuffer.clear();
        _Channels[i].IdxBuffer.clear();
    }
    _Channels.clear();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(NULL));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

// Using macros because C++ is a terrible language, we want guaranteed inline, no code in header, and no overhead in Debug builds
#define GetCurrentClipRect()    (_ClipRectStack.Size ? _ClipRectStack.Data[_ClipRectStack.Size-1]  : _Data->ClipRectFullscreen)
#define GetCurrentTextureId()   (_TextureIdStack.Size ? _TextureIdStack.Data[_TextureIdStack.Size-1] : NULL)

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = GetCurrentClipRect();
    draw_cmd.TextureId = GetCurrentTextureId();

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!current_cmd || current_cmd->ElemCount != 0 || current_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        current_cmd = &CmdBuffer.back();
    }
    current_cmd->UserCallback = callback;
    current_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::UpdateClipRect()
{
    // If current command is used with different settings we need to add a new command
    const ImVec4 curr_clip_rect = GetCurrentClipRect();
    ImDrawCmd* curr_cmd = CmdBuffer.Size > 0 ? &CmdBuffer.Data[CmdBuffer.Size-1] : NULL;
    if (!curr_cmd || (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &curr_clip_rect, sizeof(ImVec4)) != 0) || curr_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if (curr_cmd->ElemCount == 0 && prev_cmd && memcmp(&prev_cmd->ClipRect, &curr_clip_rect, sizeof(ImVec4)) == 0 && prev_cmd->TextureId == GetCurrentTextureId() && prev_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
    else
        curr_cmd->ClipRect = curr_clip_rect;
}

void ImDrawList::UpdateTextureID()
{
    // If current command is used with different settings we need to add a new command
    const ImTextureID curr_texture_id = GetCurrentTextureId();
    ImDrawCmd* curr_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!curr_cmd || (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != curr_texture_id) || curr_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if (curr_cmd->ElemCount == 0 && prev_cmd && prev_cmd->TextureId == curr_texture_id && memcmp(&prev_cmd->ClipRect, &GetCurrentClipRect(), sizeof(ImVec4)) == 0 && prev_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
    else
        curr_cmd->TextureId = curr_texture_id;
}

#undef GetCurrentClipRect
#undef GetCurrentTextureId

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect && _ClipRectStack.Size)
    {
        ImVec4 current = _ClipRectStack.Data[_ClipRectStack.Size-1];
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    IM_ASSERT(_ClipRectStack.Size > 0);
    _ClipRectStack.pop_back();
    UpdateClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
    _TextureIdStack.push_back(texture_id);
    UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
    IM_ASSERT(_TextureIdStack.Size > 0);
    _TextureIdStack.pop_back();
    UpdateTextureID();
}

void ImDrawList::ChannelsSplit(int channels_count)
{
    IM_ASSERT(_ChannelsCurrent == 0 && _ChannelsCount == 1);
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
        _Channels.resize(channels_count);
    _ChannelsCount = channels_count;

    // _Channels[] (24/32 bytes each) hold storage that we'll swap with this->_CmdBuffer/_IdxBuffer
    // The content of _Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy _CmdBuffer/_IdxBuffer into _Channels[0] and then _Channels[1] into _CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i].CmdBuffer.resize(0);
            _Channels[i].IdxBuffer.resize(0);
        }
        if (_Channels[i].CmdBuffer.Size == 0)
        {
            ImDrawCmd draw_cmd;
            draw_cmd.ClipRect = _ClipRectStack.back();
            draw_cmd.TextureId = _TextureIdStack.back();
            _Channels[i].CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawList::ChannelsMerge()
{
    // Note that we never use or rely on channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_ChannelsCount <= 1)
        return;

    ChannelsSetCurrent(0);
    if (CmdBuffer.Size && CmdBuffer.back().ElemCount == 0)
        CmdBuffer.pop_back();

    int new_cmd_buffer_count = 0, new_idx_buffer_count = 0;
    for (int i = 1; i < _ChannelsCount; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch.CmdBuffer.Size && ch.CmdBuffer.back().ElemCount == 0)
            ch.CmdBuffer.pop_back();
        new_cmd_buffer_count += ch.CmdBuffer.Size;
        new_idx_buffer_count += ch.IdxBuffer.Size;
    }
    CmdBuffer.resize(CmdBuffer.Size + new_cmd_buffer_count);
    IdxBuffer.resize(IdxBuffer.Size + new_idx_buffer_count);

    ImDrawCmd* cmd_write = CmdBuffer.Data + CmdBuffer.Size - new_cmd_buffer_count;
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _ChannelsCount; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch.CmdBuffer.Size) { memcpy(cmd_write, ch.CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch.IdxBuffer.Size) { memcpy(_IdxWritePtr, ch.IdxBuffer.Data, sz * sizeof(ImDrawIdx)); _IdxWritePtr += sz; }
    }
    UpdateClipRect(); // We call this instead of AddDrawCmd(), so that empty channels won't produce an extra draw call.
    _ChannelsCount = 1;
}

void ImDrawList::ChannelsSetCurrent(int idx)
{
    IM_ASSERT(idx < _ChannelsCount);
    if (_ChannelsCurrent == idx) return;
    memcpy(&_Channels.Data[_ChannelsCurrent].CmdBuffer, &CmdBuffer, sizeof(CmdBuffer)); // copy 12 bytes, four times
    memcpy(&_Channels.Data[_ChannelsCurrent].IdxBuffer, &IdxBuffer, sizeof(IdxBuffer));
    _ChannelsCurrent = idx;
    memcpy(&CmdBuffer, &_Channels.Data[_ChannelsCurrent].CmdBuffer, sizeof(CmdBuffer));
    memcpy(&IdxBuffer, &_Channels.Data[_ChannelsCurrent].IdxBuffer, sizeof(IdxBuffer));
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size;
}

// NB: this can be called with negative count for removing primitives (as long as the result does not underflow)
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    ImDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size-1];
    draw_cmd.ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superflous function calls to optimize debug/non-inlined builds.
// Those macros expects l-values.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)                         { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = 1.0f / ImSqrt(d2); VX *= inv_len; VY *= inv_len; } }
#define IM_NORMALIZE2F_OVER_EPSILON_CLAMP(VX,VY,EPS,INVLENMAX)  { float d2 = VX*VX + VY*VY; if (d2 > EPS)  { float inv_len = 1.0f / ImSqrt(d2); if (inv_len > INVLENMAX) inv_len = INVLENMAX; VX *= inv_len; VY *= inv_len; } }

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, bool closed, float thickness)
{
    if (points_count < 2)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    int count = points_count;
    if (!closed)
        count = points_count-1;

    const bool thick_line = thickness > 1.0f;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        const int idx_count = thick_line ? count*18 : count*12;
        const int vtx_count = thick_line ? points_count*4 : points_count*3;
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * (thick_line ? 5 : 3) * sizeof(ImVec2)); //-V630
        ImVec2* temp_points = temp_normals + points_count;

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count-1] = temp_normals[points_count-2];

        if (!thick_line)
        {
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
                temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * AA_SIZE;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * AA_SIZE;
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+3;

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_NORMALIZE2F_OVER_EPSILON_CLAMP(dm_x, dm_y, 0.000001f, 100.0f)
                dm_x *= AA_SIZE;
                dm_y *= AA_SIZE;

                // Add temporary vertexes
                ImVec2* out_vtx = &temp_points[i2*2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2+0); _IdxWritePtr[1] = (ImDrawIdx)(idx1+0); _IdxWritePtr[2] = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1+2); _IdxWritePtr[4] = (ImDrawIdx)(idx2+2); _IdxWritePtr[5] = (ImDrawIdx)(idx2+0);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2+1); _IdxWritePtr[7] = (ImDrawIdx)(idx1+1); _IdxWritePtr[8] = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1+0); _IdxWritePtr[10]= (ImDrawIdx)(idx2+0); _IdxWritePtr[11]= (ImDrawIdx)(idx2+1);
                _IdxWritePtr += 12;

                idx1 = idx2;
            }

            // Add vertexes
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = points[i];          _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
                _VtxWritePtr[1].pos = temp_points[i*2+0]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;
                _VtxWritePtr[2].pos = temp_points[i*2+1]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col_trans;
                _VtxWritePtr += 3;
            }
        }
        else
        {
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+0] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+1] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+2] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+3] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+4;

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_NORMALIZE2F_OVER_EPSILON_CLAMP(dm_x, dm_y, 0.000001f, 100.0f);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertexes
                ImVec2* out_vtx = &temp_points[i2*4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2+1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1+1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1+2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2+2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2+1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1+1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1+0); _IdxWritePtr[10] = (ImDrawIdx)(idx2+0); _IdxWritePtr[11] = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2+2); _IdxWritePtr[13] = (ImDrawIdx)(idx1+2); _IdxWritePtr[14] = (ImDrawIdx)(idx1+3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1+3); _IdxWritePtr[16] = (ImDrawIdx)(idx2+3); _IdxWritePtr[17] = (ImDrawIdx)(idx2+2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertexes
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i*4+0]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i*4+1]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i*4+2]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i*4+3]; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Stroke
        const int idx_count = count*6;
        const int vtx_count = count*4;      // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx+1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2)); //-V630
        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_NORMALIZE2F_OVER_EPSILON_CLAMP(dm_x, dm_y, 0.000001f, 100.0f);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+i-1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast(const ImVec2& centre, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius == 0.0f || a_min_of_12 > a_max_of_12)
    {
        _Path.push_back(centre);
        return;
    }
    _Path.reserve(_Path.Size + (a_max_of_12 - a_min_of_12 + 1));
    for (int a = a_min_of_12; a <= a_max_of_12; a++)
    {
        const ImVec2& c = _Data->CircleVtx12[a % IM_ARRAYSIZE(_Data->CircleVtx12)];
        _Path.push_back(ImVec2(centre.x + c.x * radius, centre.y + c.y * radius));
    }
}

void ImDrawList::PathArcTo(const ImVec2& centre, float radius, float a_min, float a_max, int num_segments)
{
    if (radius == 0.0f)
    {
        _Path.push_back(centre);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(centre.x + ImCos(a) * radius, centre.y + ImSin(a) * radius));
    }
}

static void PathBezierToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2+d3) * (d2+d3) < tess_tol * (dx*dx + dy*dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1+x2)*0.5f,       y12 = (y1+y2)*0.5f;
        float x23 = (x2+x3)*0.5f,       y23 = (y2+y3)*0.5f;
        float x34 = (x3+x4)*0.5f,       y34 = (y3+y4)*0.5f;
        float x123 = (x12+x23)*0.5f,    y123 = (y12+y23)*0.5f;
        float x234 = (x23+x34)*0.5f,    y234 = (y23+y34)*0.5f;
        float x1234 = (x123+x234)*0.5f, y1234 = (y123+y234)*0.5f;

        PathBezierToCasteljau(path, x1,y1,        x12,y12,    x123,y123,  x1234,y1234, tess_tol, level+1);
        PathBezierToCasteljau(path, x1234,y1234,  x234,y234,  x34,y34,    x4,y4,       tess_tol, level+1);
    }
}

void ImDrawList::PathBezierCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        // Auto-tessellated
        PathBezierToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0);
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
        {
            float t = t_step * i_step;
            float u = 1.0f - t;
            float w1 = u*u*u;
            float w2 = 3*u*u*t;
            float w3 = 3*u*t*t;
            float w4 = t*t*t;
            _Path.push_back(ImVec2(w1*p1.x + w2*p2.x + w3*p3.x + w4*p4.x, w1*p1.y + w2*p2.y + w3*p3.y + w4*p4.y));
        }
    }
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, int rounding_corners)
{
    rounding = ImMin(rounding, ImFabs(b.x - a.x) * ( ((rounding_corners & ImDrawCornerFlags_Top)  == ImDrawCornerFlags_Top)  || ((rounding_corners & ImDrawCornerFlags_Bot)   == ImDrawCornerFlags_Bot)   ? 0.5f : 1.0f ) - 1.0f);
    rounding = ImMin(rounding, ImFabs(b.y - a.y) * ( ((rounding_corners & ImDrawCornerFlags_Left) == ImDrawCornerFlags_Left) || ((rounding_corners & ImDrawCornerFlags_Right) == ImDrawCornerFlags_Right) ? 0.5f : 1.0f ) - 1.0f);

    if (rounding <= 0.0f || rounding_corners == 0)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    }
    else
    {
        const float rounding_tl = (rounding_corners & ImDrawCornerFlags_TopLeft) ? rounding : 0.0f;
        const float rounding_tr = (rounding_corners & ImDrawCornerFlags_TopRight) ? rounding : 0.0f;
        const float rounding_br = (rounding_corners & ImDrawCornerFlags_BotRight) ? rounding : 0.0f;
        const float rounding_bl = (rounding_corners & ImDrawCornerFlags_BotLeft) ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(a + ImVec2(0.5f,0.5f));
    PathLineTo(b + ImVec2(0.5f,0.5f));
    PathStroke(col, false, thickness);
}

// a: upper-left, b: lower-right. we don't render 1 px sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners_flags, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(a + ImVec2(0.5f,0.5f), b - ImVec2(0.50f,0.50f), rounding, rounding_corners_flags);
    else
        PathRect(a + ImVec2(0.5f,0.5f), b - ImVec2(0.49f,0.49f), rounding, rounding_corners_flags); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, true, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners_flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding > 0.0f)
    {
        PathRect(a, b, rounding, rounding_corners_flags);
        PathFillConvex(col);
    }
    else
    {
        PrimReserve(6, 4);
        PrimRect(a, b, col);
    }
}

void ImDrawList::AddRectFilledMultiColor(const ImVec2& a, const ImVec2& c, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+3));
    PrimWriteVtx(a, uv, col_upr_left);
    PrimWriteVtx(ImVec2(c.x, a.y), uv, col_upr_right);
    PrimWriteVtx(c, uv, col_bot_right);
    PrimWriteVtx(ImVec2(a.x, c.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathLineTo(d);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathLineTo(d);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius-0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

void ImDrawList::AddBezierCurve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(pos0);
    PathBezierCurveTo(cp0, cp1, pos1, num_segments);
    PathStroke(col, false, thickness);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_begin == text_end)
        return;

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _TextureIdStack.back());  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _ClipRectStack.back();
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(NULL, 0.0f, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(a, b, uv_a, uv_b, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col, float rounding, int rounding_corners)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (rounding <= 0.0f || (rounding_corners & ImDrawCornerFlags_All) == 0)
    {
        AddImage(user_texture_id, a, b, uv_a, uv_b, col);
        return;
    }

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(a, b, rounding, rounding_corners);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, a, b, uv_a, uv_b, true);

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_i];
            cmd->ClipRect = ImVec4(cmd->ClipRect.x * fb_scale.x, cmd->ClipRect.y * fb_scale.y, cmd->ClipRect.z * fb_scale.x, cmd->ClipRect.w * fb_scale.y);
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = ImLerp((int)(col0 >> IM_COL32_R_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_R_SHIFT) & 0xFF, t);
        int g = ImLerp((int)(col0 >> IM_COL32_G_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_G_SHIFT) & 0xFF, t);
        int b = ImLerp((int)(col0 >> IM_COL32_B_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_B_SHIFT) & 0xFF, t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    }
    else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    FontData = NULL;
    FontDataSize = 0;
    FontDataOwnedByAtlas = true;
    FontNo = 0;
    SizePixels = 0.0f;
    OversampleH = 3; // FIXME: 2 may be a better default?
    OversampleV = 1;
    PixelSnapH = false;
    GlyphExtraSpacing = ImVec2(0.0f, 0.0f);
    GlyphOffset = ImVec2(0.0f, 0.0f);
    GlyphRanges = NULL;
    GlyphMinAdvanceX = 0.0f;
    GlyphMaxAdvanceX = FLT_MAX;
    MergeMode = false;
    RasterizerFlags = 0x00;
    RasterizerMultiply = 1.0f;
    memset(Name, 0, sizeof(Name));
    DstFont = NULL;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere in ImGui to render filled shapes.
const int FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF = 108;
const int FONT_ATLAS_DEFAULT_TEX_DATA_H      = 27;
const unsigned int FONT_ATLAS_DEFAULT_TEX_DATA_ID = 0x80000000;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X"
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X"
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X"
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X"
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  "
    "------------        -    X    -           X           -X.....................X-           ------------------"
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                             "
    "                                                      -  X..X           X..X  -                             "
    "                                                      -   X.X           X.X   -                             "
    "                                                      -    XX           XX    -                             "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2( 0,3), ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2( 7,16), ImVec2( 1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2( 9,23), ImVec2( 4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2( 5, 0) }, // ImGuiMouseCursor_Hand
};

ImFontAtlas::ImFontAtlas()
{
    Locked = false;
    Flags = ImFontAtlasFlags_None;
    TexID = (ImTextureID)NULL;
    TexDesiredWidth = 0;
    TexGlyphPadding = 1;

    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = 0;
    TexUvScale = ImVec2(0.0f, 0.0f);
    TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    for (int n = 0; n < IM_ARRAYSIZE(CustomRectIds); n++)
        CustomRectIds[n] = -1;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < ConfigData.Size; i++)
        if (ConfigData[i].FontData && ConfigData[i].FontDataOwnedByAtlas)
        {
            ImGui::MemFree(ConfigData[i].FontData);
            ConfigData[i].FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (int i = 0; i < Fonts.Size; i++)
        if (Fonts[i]->ConfigData >= ConfigData.Data && Fonts[i]->ConfigData < ConfigData.Data + ConfigData.Size)
        {
            Fonts[i]->ConfigData = NULL;
            Fonts[i]->ConfigDataCount = 0;
        }
    ConfigData.clear();
    CustomRects.clear();
    for (int n = 0; n < IM_ARRAYSIZE(CustomRectIds); n++)
        CustomRectIds[n] = -1;
}

void    ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    if (TexPixelsAlpha8)
        ImGui::MemFree(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        ImGui::MemFree(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::ClearFonts()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < Fonts.Size; i++)
        IM_DELETE(Fonts[i]);
    Fonts.clear();
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
    {
        if (ConfigData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels = NULL;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        if (pixels)
        {
            TexPixelsRGBA32 = (unsigned int*)ImGui::MemAlloc((size_t)TexWidth * (size_t)TexHeight * 4);
            const unsigned char* src = pixels;
            unsigned int* dst = TexPixelsRGBA32;
            for (int n = TexWidth * TexHeight; n > 0; n--)
                *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
        }
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
        Fonts.push_back(IM_NEW(ImFont));
    else
        IM_ASSERT(!Fonts.empty() && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
    if (new_font_cfg.DstFont == NULL)
        new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = ImGui::MemAlloc(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    // Invalidate texture
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see misc/fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(const unsigned char *input);
static unsigned int stb_decompress(unsigned char *output, const unsigned char *input, unsigned int length);
static const char*  GetDefaultCompressedFontDataTTFBase85();
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85*(Decode85Byte(src[1]) + 85*(Decode85Byte(src[2]) + 85*(Decode85Byte(src[3]) + 85*Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }

    const char* font_size_env = std::getenv("FONT_SIZE");
    float font_size;

    if(!font_size_env == NULL)
		font_size = std::stof(font_size_env);

    if (font_cfg.SizePixels <= 0.0f){
        if(int(font_size) == 0){
            font_cfg.SizePixels = 24.0f * 1.0f;
        } else {
            font_cfg.SizePixels = font_size * 1.0f;
        }
    }
    
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf, %dpx", (int)font_cfg.SizePixels);

    const char* ttf_compressed_base85 = GetDefaultCompressedFontDataTTFBase85();
    const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
    ImFont* font = AddFontFromMemoryCompressedBase85TTF(ttf_compressed_base85, font_cfg.SizePixels, &font_cfg, glyph_ranges);
    font->DisplayOffset.y = 1.0f;
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        IM_ASSERT(0); // Could not load file.
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* ttf_data, int ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontData = ttf_data;
    font_cfg.FontDataSize = ttf_size;
    font_cfg.SizePixels = size_pixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char *)ImGui::MemAlloc(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)strlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = ImGui::MemAlloc((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    ImGui::MemFree(compressed_ttf);
    return font;
}

int ImFontAtlas::AddCustomRectRegular(unsigned int id, int width, int height)
{
    IM_ASSERT(id >= 0x10000);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    CustomRect r;
    r.ID = id;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

int ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar id, int width, int height, float advance_x, const ImVec2& offset)
{
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    CustomRect r;
    r.ID = id;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.GlyphAdvanceX = advance_x;
    r.GlyphOffset = offset;
    r.Font = font;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

void ImFontAtlas::CalcCustomRectUV(const CustomRect* rect, ImVec2* out_uv_min, ImVec2* out_uv_max)
{
    IM_ASSERT(TexWidth > 0 && TexHeight > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    IM_ASSERT(rect->IsPacked());                // Make sure the rectangle has been packed
    *out_uv_min = ImVec2((float)rect->X * TexUvScale.x, (float)rect->Y * TexUvScale.y);
    *out_uv_max = ImVec2((float)(rect->X + rect->Width) * TexUvScale.x, (float)(rect->Y + rect->Height) * TexUvScale.y);
}

bool ImFontAtlas::GetMouseCursorTexData(ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    IM_ASSERT(CustomRectIds[0] != -1);
    ImFontAtlas::CustomRect& r = CustomRects[CustomRectIds[0]];
    IM_ASSERT(r.ID == FONT_ATLAS_DEFAULT_TEX_DATA_ID);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r.X, (float)r.Y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos) * TexUvScale;
    out_uv_border[1] = (pos + size) * TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
    out_uv_fill[0] = (pos) * TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    return ImFontAtlasBuildWithStbTruetype(this);
}

void    ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

void    ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride)
        for (int i = 0; i < w; i++)
            data[i] = table[data[i]];
}

// Temporary data for one source font (multiple source fonts can be merged into one destination ImFont)
// (C++03 doesn't allow instancing ImVector<> with function-local types so we declare the type here.)
struct ImFontBuildSrcData
{
    stbtt_fontinfo      FontInfo;
    stbtt_pack_range    PackRange;          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    stbrp_rect*         Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    stbtt_packedchar*   PackedChars;        // Output glyphs
    const ImWchar*      SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBoolVector        GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<int>       GlyphsList;         // Glyph codepoints list (flattened version of GlyphsMap)
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstData
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBoolVector        GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

static void UnpackBoolVectorToFlatIndexList(const ImBoolVector* in, ImVector<int>* out)
{
    IM_ASSERT(sizeof(in->Storage.Data[0]) == sizeof(int));
    const int* it_begin = in->Storage.begin();
    const int* it_end = in->Storage.end();
    for (const int* it = it_begin; it < it_end; it++)
        if (int entries_32 = *it)
            for (int bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & (1u << bit_n))
                    out->push_back((int)((it - it_begin) << 5) + bit_n);
}

bool    ImFontAtlasBuildWithStbTruetype(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);

    ImFontAtlasBuildRegisterDefaultCustomRects(atlas);

    // Clear atlas
    atlas->TexID = (ImTextureID)NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    ImVector<ImFontBuildSrcData> src_tmp_array;
    ImVector<ImFontBuildDstData> dst_tmp_array;
    src_tmp_array.resize(atlas->ConfigData.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    memset(src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    memset(dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->ConfigData.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        // Find index from cfg.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (cfg.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        IM_ASSERT(src_tmp.DstIndex != -1); // cfg.DstFont not pointing within atlas->Fonts[] array?
        if (src_tmp.DstIndex == -1)
            return false;

        // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
        if (!stbtt_InitFont(&src_tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset))
            return false;

        // Measure highest codepoints
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = cfg.GlyphRanges ? cfg.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        src_tmp.GlyphsSet.Resize(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.SrcCount > 1 && dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Resize(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (int codepoint = src_range[0]; codepoint <= src_range[1]; codepoint++)
            {
                if (cfg.MergeMode && dst_tmp.GlyphsSet.GetBit(codepoint))   // Don't overwrite existing glyphs. We could make this an option (e.g. MergeOverwrite)
                    continue;
                if (!stbtt_FindGlyphIndex(&src_tmp.FontInfo, codepoint))    // It is actually in the font?
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint, true);
                if (dst_tmp.SrcCount > 1)
                    dst_tmp.GlyphsSet.SetBit(codepoint, true);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);
        UnpackBoolVectorToFlatIndexList(&src_tmp.GlyphsSet, &src_tmp.GlyphsList);
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    ImVector<stbtt_packedchar> buf_packedchars;
    buf_rects.resize(total_glyphs_count);
    buf_packedchars.resize(total_glyphs_count);
    memset(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());
    memset(buf_packedchars.Data, 0, (size_t)buf_packedchars.size_in_bytes());

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    int total_surface = 0;
    int buf_rects_out_n = 0;
    int buf_packedchars_out_n = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        src_tmp.PackedChars = &buf_packedchars[buf_packedchars_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;
        buf_packedchars_out_n += src_tmp.GlyphsCount;

        // Convert our ranges in the format stb_truetype wants
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        src_tmp.PackRange.font_size = cfg.SizePixels;
        src_tmp.PackRange.first_unicode_codepoint_in_range = 0;
        src_tmp.PackRange.array_of_unicode_codepoints = src_tmp.GlyphsList.Data;
        src_tmp.PackRange.num_chars = src_tmp.GlyphsList.Size;
        src_tmp.PackRange.chardata_for_range = src_tmp.PackedChars;
        src_tmp.PackRange.h_oversample = (unsigned char)cfg.OversampleH;
        src_tmp.PackRange.v_oversample = (unsigned char)cfg.OversampleV;

        // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
        const float scale = (cfg.SizePixels > 0) ? stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels) : stbtt_ScaleForMappingEmToPixels(&src_tmp.FontInfo, -cfg.SizePixels);
        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            int x0, y0, x1, y1;
            const int glyph_index_in_font = stbtt_FindGlyphIndex(&src_tmp.FontInfo, src_tmp.GlyphsList[glyph_i]);
            IM_ASSERT(glyph_index_in_font != 0);
            stbtt_GetGlyphBitmapBoxSubpixel(&src_tmp.FontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1);
            src_tmp.Rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096*0.7f) ? 4096 : (surface_sqrt >= 2048*0.7f) ? 2048 : (surface_sqrt >= 1024*0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin(&spc, NULL, atlas->TexWidth, TEX_HEIGHT_MAX, 0, atlas->TexGlyphPadding, NULL);
    ImFontAtlasBuildPackCustomRects(atlas, spc.pack_info);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects((stbrp_context*)spc.pack_info, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)ImGui::MemAlloc(atlas->TexWidth * atlas->TexHeight);
    memset(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);
    spc.pixels = atlas->TexPixelsAlpha8;
    spc.height = atlas->TexHeight;

    // 8. Render/rasterize font characters into the texture
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbtt_PackFontRangesRenderIntoRects(&spc, &src_tmp.FontInfo, &src_tmp.PackRange, 1, src_tmp.Rects);

        // Apply multiply operator
        if (cfg.RasterizerMultiply != 1.0f)
        {
            unsigned char multiply_table[256];
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
            stbrp_rect* r = &src_tmp.Rects[0];
            for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++, r++)
                if (r->was_packed)
                    ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w, r->h, atlas->TexWidth * 1);
        }
        src_tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    buf_rects.clear();

    // 9. Setup ImFont and glyphs for runtime
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFont* dst_font = cfg.DstFont; // We can have multiple input fonts writing into a same destination font (when using MergeMode=true)

        const float font_scale = stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&src_tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        const float ascent = ImFloor(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
        const float descent = ImFloor(unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1));
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + (float)(int)(dst_font->Ascent + 0.5f);

        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            const int codepoint = src_tmp.GlyphsList[glyph_i];
            const stbtt_packedchar& pc = src_tmp.PackedChars[glyph_i];

            const float char_advance_x_org = pc.xadvance;
            const float char_advance_x_mod = ImClamp(char_advance_x_org, cfg.GlyphMinAdvanceX, cfg.GlyphMaxAdvanceX);
            float char_off_x = font_off_x;
            if (char_advance_x_org != char_advance_x_mod)
                char_off_x += cfg.PixelSnapH ? (float)(int)((char_advance_x_mod - char_advance_x_org) * 0.5f) : (char_advance_x_mod - char_advance_x_org) * 0.5f;

            // Register glyph
            stbtt_aligned_quad q;
            float dummy_x = 0.0f, dummy_y = 0.0f;
            stbtt_GetPackedQuad(src_tmp.PackedChars, atlas->TexWidth, atlas->TexHeight, glyph_i, &dummy_x, &dummy_y, &q, 0);
            dst_font->AddGlyph((ImWchar)codepoint, q.x0 + char_off_x, q.y0 + font_off_y, q.x1 + char_off_x, q.y1 + font_off_y, q.s0, q.t0, q.s1, q.t1, char_advance_x_mod);
        }
    }

    // Cleanup temporary (ImVector doesn't honor destructor)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
        src_tmp_array[src_i].~ImFontBuildSrcData();

    ImFontAtlasBuildFinish(atlas);
    return true;
}

void ImFontAtlasBuildRegisterDefaultCustomRects(ImFontAtlas* atlas)
{
    if (atlas->CustomRectIds[0] >= 0)
        return;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
        atlas->CustomRectIds[0] = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_ID, FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF*2+1, FONT_ATLAS_DEFAULT_TEX_DATA_H);
    else
        atlas->CustomRectIds[0] = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_ID, 2, 2);
}

void ImFontAtlasBuildSetupFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* font_config, float ascent, float descent)
{
    if (!font_config->MergeMode)
    {
        font->ClearOutputData();
        font->FontSize = font_config->SizePixels;
        font->ConfigData = font_config;
        font->ContainerAtlas = atlas;
        font->Ascent = ascent;
        font->Descent = descent;
    }
    font->ConfigDataCount++;
}

void ImFontAtlasBuildPackCustomRects(ImFontAtlas* atlas, void* stbrp_context_opaque)
{
    stbrp_context* pack_context = (stbrp_context*)stbrp_context_opaque;
    IM_ASSERT(pack_context != NULL);

    ImVector<ImFontAtlas::CustomRect>& user_rects = atlas->CustomRects;
    IM_ASSERT(user_rects.Size >= 1); // We expect at least the default custom rects to be registered, else something went wrong.

    ImVector<stbrp_rect> pack_rects;
    pack_rects.resize(user_rects.Size);
    memset(pack_rects.Data, 0, (size_t)pack_rects.size_in_bytes());
    for (int i = 0; i < user_rects.Size; i++)
    {
        pack_rects[i].w = user_rects[i].Width;
        pack_rects[i].h = user_rects[i].Height;
    }
    stbrp_pack_rects(pack_context, &pack_rects[0], pack_rects.Size);
    for (int i = 0; i < pack_rects.Size; i++)
        if (pack_rects[i].was_packed)
        {
            user_rects[i].X = pack_rects[i].x;
            user_rects[i].Y = pack_rects[i].y;
            IM_ASSERT(pack_rects[i].w == user_rects[i].Width && pack_rects[i].h == user_rects[i].Height);
            atlas->TexHeight = ImMax(atlas->TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
}

static void ImFontAtlasBuildRenderDefaultTexData(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->CustomRectIds[0] >= 0);
    IM_ASSERT(atlas->TexPixelsAlpha8 != NULL);
    ImFontAtlas::CustomRect& r = atlas->CustomRects[atlas->CustomRectIds[0]];
    IM_ASSERT(r.ID == FONT_ATLAS_DEFAULT_TEX_DATA_ID);
    IM_ASSERT(r.IsPacked());

    const int w = atlas->TexWidth;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
    {
        // Render/copy pixels
        IM_ASSERT(r.Width == FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1 && r.Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        for (int y = 0, n = 0; y < FONT_ATLAS_DEFAULT_TEX_DATA_H; y++)
            for (int x = 0; x < FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF; x++, n++)
            {
                const int offset0 = (int)(r.X + x) + (int)(r.Y + y) * w;
                const int offset1 = offset0 + FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
                atlas->TexPixelsAlpha8[offset0] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == '.' ? 0xFF : 0x00;
                atlas->TexPixelsAlpha8[offset1] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == 'X' ? 0xFF : 0x00;
            }
    }
    else
    {
        IM_ASSERT(r.Width == 2 && r.Height == 2);
        const int offset = (int)(r.X) + (int)(r.Y) * w;
        atlas->TexPixelsAlpha8[offset] = atlas->TexPixelsAlpha8[offset + 1] = atlas->TexPixelsAlpha8[offset + w] = atlas->TexPixelsAlpha8[offset + w + 1] = 0xFF;
    }
    atlas->TexUvWhitePixel = ImVec2((r.X + 0.5f) * atlas->TexUvScale.x, (r.Y + 0.5f) * atlas->TexUvScale.y);
}

void ImFontAtlasBuildFinish(ImFontAtlas* atlas)
{
    // Render into our custom data block
    ImFontAtlasBuildRenderDefaultTexData(atlas);

    // Register custom rectangle glyphs
    for (int i = 0; i < atlas->CustomRects.Size; i++)
    {
        const ImFontAtlas::CustomRect& r = atlas->CustomRects[i];
        if (r.Font == NULL || r.ID > 0x10000)
            continue;

        IM_ASSERT(r.Font->ContainerAtlas == atlas);
        ImVec2 uv0, uv1;
        atlas->CalcCustomRectUV(&r, &uv0, &uv1);
        r.Font->AddGlyph((ImWchar)r.ID, r.GlyphOffset.x, r.GlyphOffset.y, r.GlyphOffset.x + r.Width, r.GlyphOffset.y + r.Height, uv0.x, uv0.y, uv1.x, uv1.y, r.GlyphAdvanceX);
    }

    // Build all fonts lookup tables
    for (int i = 0; i < atlas->Fonts.Size; i++)
        if (atlas->Fonts[i]->DirtyLookupTables)
            atlas->Fonts[i]->BuildLookupTable();
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD79D, // Korean characters
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas glyph ranges helpers
//-------------------------------------------------------------------------

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // 1946 common ideograms code points for Japanese
    // Sourced from http://theinstructionlimit.com/common-kanji-character-ranges-for-xna-spritefont-rendering
    // FIXME: Source a list of the revised 2136 Joyo Kanji list from 2010 and rebuild this.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,6,2,2,1,8,5,7,11,1,2,10,10,8,2,4,20,2,11,8,2,1,2,1,6,2,1,7,5,3,7,1,1,13,7,9,1,4,6,1,2,1,10,1,1,9,2,2,4,5,6,14,1,1,9,3,18,
        5,4,2,2,10,7,1,1,1,3,2,4,3,23,2,10,12,2,14,2,4,13,1,6,10,3,1,7,13,6,4,13,5,2,3,17,2,2,5,7,6,4,1,7,14,16,6,13,9,15,1,1,7,16,4,7,1,19,9,2,7,15,
        2,6,5,13,25,4,14,13,11,25,1,1,1,2,1,2,2,3,10,11,3,3,1,1,4,4,2,1,4,9,1,4,3,5,5,2,7,12,11,15,7,16,4,5,16,2,1,1,6,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,
        2,1,12,3,3,9,5,8,1,11,1,2,3,18,20,4,1,3,6,1,7,3,5,5,7,2,2,12,3,1,4,2,3,2,3,11,8,7,4,17,1,9,25,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,6,16,1,2,1,1,3,12,
        20,2,5,20,8,7,6,2,1,1,1,1,6,2,1,2,10,1,1,6,1,3,1,2,1,4,1,12,4,1,3,1,1,1,1,1,10,4,7,5,13,1,15,1,1,30,11,9,1,15,38,14,1,32,17,20,1,9,31,2,21,9,
        4,49,22,2,1,13,1,11,45,35,43,55,12,19,83,1,3,2,3,13,2,1,7,3,18,3,13,8,1,8,18,5,3,7,25,24,9,24,40,3,17,24,2,1,6,2,3,16,15,6,7,3,12,1,9,7,3,3,
        3,15,21,5,16,4,5,12,11,11,3,6,3,2,31,3,2,1,1,23,6,6,1,4,2,6,5,2,1,1,3,3,22,2,6,2,3,17,3,2,4,5,1,9,5,1,1,6,15,12,3,17,2,14,2,8,1,23,16,4,2,23,
        8,15,23,20,12,25,19,47,11,21,65,46,4,3,1,5,6,1,2,5,26,2,1,1,3,11,1,1,1,2,1,2,3,1,1,10,2,3,1,1,1,3,6,3,2,2,6,6,9,2,2,2,6,2,5,10,2,4,1,2,1,2,2,
        3,1,1,3,1,2,9,23,9,2,1,1,1,1,5,3,2,1,10,9,6,1,10,2,31,25,3,7,5,40,1,15,6,17,7,27,180,1,3,2,2,1,1,1,6,3,10,7,1,3,6,17,8,6,2,2,1,3,5,5,8,16,14,
        15,1,1,4,1,2,1,1,1,3,2,7,5,6,2,5,10,1,4,2,9,1,1,11,6,1,44,1,3,7,9,5,1,3,1,1,10,7,1,10,4,2,7,21,15,7,2,5,1,8,3,4,1,3,1,6,1,4,2,1,4,10,8,1,4,5,
        1,5,10,2,7,1,10,1,1,3,4,11,10,29,4,7,3,5,2,3,33,5,2,19,3,1,4,2,6,31,11,1,3,3,3,1,8,10,9,12,11,12,8,3,14,8,6,11,1,4,41,3,1,2,7,13,1,5,6,2,6,12,
        12,22,5,9,4,8,9,9,34,6,24,1,1,20,9,9,3,4,1,7,2,2,2,6,2,28,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,8,8,3,2,1,5,1,2,2,3,1,11,11,7,3,6,10,8,6,16,16,
        22,7,12,6,21,5,4,6,6,3,6,1,3,2,1,2,8,29,1,10,1,6,13,6,6,19,31,1,13,4,4,22,17,26,33,10,4,15,12,25,6,67,10,2,3,1,6,10,2,6,2,9,1,9,4,4,1,2,16,2,
        5,9,2,3,8,1,8,3,9,4,8,6,4,8,11,3,2,1,1,3,26,1,7,5,1,11,1,5,3,5,2,13,6,39,5,1,5,2,11,6,10,5,1,15,5,3,6,19,21,22,2,4,1,6,1,8,1,4,8,2,4,2,2,9,2,
        1,1,1,4,3,6,3,12,7,1,14,2,4,10,2,13,1,17,7,3,2,1,3,2,13,7,14,12,3,1,29,2,8,9,15,14,9,14,1,3,1,6,5,9,11,3,38,43,20,7,7,8,5,15,12,19,15,81,8,7,
        1,5,73,13,37,28,8,8,1,15,18,20,165,28,1,6,11,8,4,14,7,15,1,3,3,6,4,1,7,14,1,1,11,30,1,5,1,4,14,1,4,2,7,52,2,6,29,3,1,9,1,21,3,5,1,26,3,11,14,
        11,1,17,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,7,7,5,17,3,3,3,1,23,10,4,4,6,3,1,16,17,22,3,10,21,16,16,6,4,10,2,1,1,2,8,8,6,5,3,3,3,39,25,
        15,1,1,16,6,7,25,15,6,6,12,1,22,13,1,4,9,5,12,2,9,1,12,28,8,3,5,10,22,60,1,2,40,4,61,63,4,1,13,12,1,4,31,12,1,14,89,5,16,6,29,14,2,5,49,18,18,
        5,29,33,47,1,17,1,19,12,2,9,7,39,12,3,7,12,39,3,1,46,4,12,3,8,9,5,31,15,18,3,2,2,66,19,13,17,5,3,46,124,13,57,34,2,5,4,5,8,1,1,1,4,3,1,17,5,
        3,5,3,1,8,5,6,3,27,3,26,7,12,7,2,17,3,7,18,78,16,4,36,1,2,1,6,2,1,39,17,7,4,13,4,4,4,1,10,4,2,4,6,3,10,1,19,1,26,2,4,33,2,73,47,7,3,8,2,4,15,
        18,1,29,2,41,14,1,21,16,41,7,39,25,13,44,2,2,10,1,13,7,1,7,3,5,20,4,8,2,49,1,10,6,1,6,7,10,7,11,16,3,12,20,4,10,3,1,2,11,2,28,9,2,4,7,2,15,1,
        27,1,28,17,4,5,10,7,3,24,10,11,6,26,3,2,7,2,2,49,16,10,16,15,4,5,27,61,30,14,38,22,2,7,5,1,3,12,23,24,17,17,3,3,2,4,1,6,2,7,5,1,1,5,1,1,9,4,
        1,3,6,1,8,2,8,4,14,3,5,11,4,1,3,32,1,19,4,1,13,11,5,2,1,8,6,8,1,6,5,13,3,23,11,5,3,16,3,9,10,1,24,3,198,52,4,2,2,5,14,5,4,22,5,20,4,11,6,41,
        1,5,2,2,11,5,2,28,35,8,22,3,18,3,10,7,5,3,4,1,5,3,8,9,3,6,2,16,22,4,5,5,3,3,18,23,2,6,23,5,27,8,1,33,2,12,43,16,5,2,3,6,1,20,4,2,9,7,1,11,2,
        10,3,14,31,9,3,25,18,20,2,5,5,26,14,1,11,17,12,40,19,9,6,31,83,2,7,9,19,78,12,14,21,76,12,113,79,34,4,1,1,61,18,85,10,2,2,13,31,11,50,6,33,159,
        179,6,6,7,4,4,2,4,2,5,8,7,20,32,22,1,3,10,6,7,28,5,10,9,2,77,19,13,2,5,1,4,4,7,4,13,3,9,31,17,3,26,2,6,6,5,4,1,7,11,3,4,2,1,6,2,20,4,1,9,2,6,
        3,7,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,5,13,8,4,11,23,1,10,6,2,1,3,21,2,2,4,24,31,4,10,10,2,5,192,15,4,16,7,9,51,1,2,1,1,5,1,1,2,1,3,5,3,1,3,4,1,
        3,1,3,3,9,8,1,2,2,2,4,4,18,12,92,2,10,4,3,14,5,25,16,42,4,14,4,2,21,5,126,30,31,2,1,5,13,3,22,5,6,6,20,12,1,14,12,87,3,19,1,8,2,9,9,3,3,23,2,
        3,7,6,3,1,2,3,9,1,3,1,6,3,2,1,3,11,3,1,6,10,3,2,3,1,2,1,5,1,1,11,3,6,4,1,7,2,1,2,5,5,34,4,14,18,4,19,7,5,8,2,6,79,1,5,2,14,8,2,9,2,1,36,28,16,
        4,1,1,1,2,12,6,42,39,16,23,7,15,15,3,2,12,7,21,64,6,9,28,8,12,3,3,41,59,24,51,55,57,294,9,9,2,6,2,15,1,2,13,38,90,9,9,9,3,11,7,1,1,1,5,6,3,2,
        1,2,2,3,8,1,4,4,1,5,7,1,4,3,20,4,9,1,1,1,5,5,17,1,5,2,6,2,4,1,4,5,7,3,18,11,11,32,7,5,4,7,11,127,8,4,3,3,1,10,1,1,6,21,14,1,16,1,7,1,3,6,9,65,
        51,4,3,13,3,10,1,1,12,9,21,110,3,19,24,1,1,10,62,4,1,29,42,78,28,20,18,82,6,3,15,6,84,58,253,15,155,264,15,21,9,14,7,58,40,39,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00)*2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        if (c < 0x10000)
            AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (ImWchar c = ranges[0]; c <= ranges[1]; c++)
            AddChar(c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    for (int n = 0; n < 0x10000; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < 0x10000 && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    FallbackChar = (ImWchar)'?';
    DisplayOffset = ImVec2(0.0f, 0.0f);
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    ConfigData = NULL;
    ConfigDataCount = 0;
    DirtyLookupTables = false;
    Scale = 1.0f;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void    ImFont::ClearOutputData()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    DirtyLookupTables = true;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexAdvanceX.clear();
    IndexLookup.clear();
    DirtyLookupTables = false;
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        IndexLookup[codepoint] = (ImWchar)i;
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((ImWchar)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times
            Glyphs.resize(Glyphs.Size + 1);
        ImFontGlyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((ImWchar)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX *= 4;
        IndexAdvanceX[(int)tab_glyph.Codepoint] = (float)tab_glyph.AdvanceX;
        IndexLookup[(int)tab_glyph.Codepoint] = (ImWchar)(Glyphs.Size-1);
    }

    FallbackGlyph = FindGlyphNoFallback(FallbackChar);
    FallbackAdvanceX = FallbackGlyph ? FallbackGlyph->AdvanceX : 0.0f;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexAdvanceX[i] < 0.0f)
            IndexAdvanceX[i] = FallbackAdvanceX;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexAdvanceX.Size == IndexLookup.Size);
    if (new_size <= IndexLookup.Size)
        return;
    IndexAdvanceX.resize(new_size, -1.0f);
    IndexLookup.resize(new_size, (ImWchar)-1);
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
void ImFont::AddGlyph(ImWchar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    Glyphs.resize(Glyphs.Size + 1);
    ImFontGlyph& glyph = Glyphs.back();
    glyph.Codepoint = (ImWchar)codepoint;
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x + ConfigData->GlyphExtraSpacing.x;  // Bake spacing into AdvanceX

    if (ConfigData->PixelSnapH)
        glyph.AdvanceX = (float)(int)(glyph.AdvanceX + 0.5f);

    // Compute rough surface usage metrics (+1 to account for average padding, +0.99 to round)
    DirtyLookupTables = true;
    MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * ContainerAtlas->TexWidth + 1.99f) * (int)((glyph.V1 - glyph.V0) * ContainerAtlas->TexHeight + 1.99f);
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    int index_size = IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (ImWchar)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (ImWchar)-1;
    IndexAdvanceX[dst] = (src < index_size) ? IndexAdvanceX.Data[src] : 1.0f;
}

const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const
{
    if (c >= IndexLookup.Size)
        return FallbackGlyph;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return FallbackGlyph;
    return &Glyphs.Data[i];
}

const ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c) const
{
    if (c >= IndexLookup.Size)
        return NULL;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return NULL;
    return &Glyphs.Data[i];
}

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width >= wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImFont::RenderChar(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, ImWchar c) const
{
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') // Match behavior of RenderText(), those 4 codepoints are hard-coded.
        return;
    if (const ImFontGlyph* glyph = FindGlyph(c))
    {
        float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
        pos.x = (float)(int)pos.x + DisplayOffset.x;
        pos.y = (float)(int)pos.y + DisplayOffset.y;
        draw_list->PrimReserve(6, 4);
        draw_list->PrimRectUV(ImVec2(pos.x + glyph->X0 * scale, pos.y + glyph->Y0 * scale), ImVec2(pos.x + glyph->X1 * scale, pos.y + glyph->Y1 * scale), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
    }
}

void ImFont::RenderText(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // ImGui functions generally already provides a valid text_end, so this is merely to handle direct calls.

    // Align to be pixel perfect
    pos.x = (float)(int)pos.x + DisplayOffset.x;
    pos.y = (float)(int)pos.y + DisplayOffset.y;
    float x = pos.x;
    float y = pos.y;
    if (y > clip_rect.w)
        return;

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y && !word_wrap_enabled)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            s = (const char*)memchr(s, '\n', text_end - s);
            s = s ? s + 1 : text_end;
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)memchr(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);

    ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                x = pos.x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        float char_width = 0.0f;
        if (const ImFontGlyph* glyph = FindGlyph((ImWchar)c))
        {
            char_width = glyph->AdvanceX * scale;

            // Arbitrarily assume that both space and tabs are empty glyphs as an optimization
            if (c != ' ' && c != '\t')
            {
                // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
                float x1 = x + glyph->X0 * scale;
                float x2 = x + glyph->X1 * scale;
                float y1 = y + glyph->Y0 * scale;
                float y2 = y + glyph->Y1 * scale;
                if (x1 <= clip_rect.z && x2 >= clip_rect.x)
                {
                    // Render a character
                    float u1 = glyph->U0;
                    float v1 = glyph->V0;
                    float u2 = glyph->U1;
                    float v2 = glyph->V1;

                    // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                    if (cpu_fine_clip)
                    {
                        if (x1 < clip_rect.x)
                        {
                            u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                            x1 = clip_rect.x;
                        }
                        if (y1 < clip_rect.y)
                        {
                            v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                            y1 = clip_rect.y;
                        }
                        if (x2 > clip_rect.z)
                        {
                            u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                            x2 = clip_rect.z;
                        }
                        if (y2 > clip_rect.w)
                        {
                            v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                            y2 = clip_rect.w;
                        }
                        if (y1 >= y2)
                        {
                            x += char_width;
                            continue;
                        }
                    }

                    // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                    {
                        idx_write[0] = (ImDrawIdx)(vtx_current_idx); idx_write[1] = (ImDrawIdx)(vtx_current_idx+1); idx_write[2] = (ImDrawIdx)(vtx_current_idx+2);
                        idx_write[3] = (ImDrawIdx)(vtx_current_idx); idx_write[4] = (ImDrawIdx)(vtx_current_idx+2); idx_write[5] = (ImDrawIdx)(vtx_current_idx+3);
                        vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                        vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                        vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                        vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                        vtx_write += 4;
                        vtx_current_idx += 4;
                        idx_write += 6;
                    }
                }
            }
        }

        x += char_width;
    }

    // Give back unused vertices
    draw_list->VtxBuffer.resize((int)(vtx_write - draw_list->VtxBuffer.Data));
    draw_list->IdxBuffer.resize((int)(idx_write - draw_list->IdxBuffer.Data));
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size-1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = (unsigned int)draw_list->VtxBuffer.Size;
}

//-----------------------------------------------------------------------------
// [SECTION] Internal Render Helpers
// (progressively moved from imgui.cpp to here when they are redesigned to stop accessing ImGui global state)
//-----------------------------------------------------------------------------
// - RenderMouseCursor()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
// - RenderPixelEllipsis()
//-----------------------------------------------------------------------------

void ImGui::RenderMouseCursor(ImDrawList* draw_list, ImVec2 pos, float scale, ImGuiMouseCursor mouse_cursor)
{
    if (mouse_cursor == ImGuiMouseCursor_None)
        return;
    IM_ASSERT(mouse_cursor > ImGuiMouseCursor_None && mouse_cursor < ImGuiMouseCursor_COUNT);

    const ImU32 col_shadow = IM_COL32(0, 0, 0, 48);
    const ImU32 col_border = IM_COL32(0, 0, 0, 255);          // Black
    const ImU32 col_fill   = IM_COL32(255, 255, 255, 255);    // White

    ImFontAtlas* font_atlas = draw_list->_Data->Font->ContainerAtlas;
    ImVec2 offset, size, uv[4];
    if (font_atlas->GetMouseCursorTexData(mouse_cursor, &offset, &size, &uv[0], &uv[2]))
    {
        pos -= offset;
        const ImTextureID tex_id = font_atlas->TexID;
        draw_list->PushTextureID(tex_id);
        draw_list->AddImage(tex_id, pos + ImVec2(1,0)*scale, pos + ImVec2(1,0)*scale + size*scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos + ImVec2(2,0)*scale, pos + ImVec2(2,0)*scale + size*scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos,                     pos + size*scale,                     uv[2], uv[3], col_border);
        draw_list->AddImage(tex_id, pos,                     pos + size*scale,                     uv[0], uv[1], col_fill);
        draw_list->PopTextureID();
    }
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    }
    else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    }
    else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b, 3); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e, 3); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        }
        else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        }
        else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b, 3); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e, 3); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

// FIXME: Rendering an ellipsis "..." is a surprisingly tricky problem for us... we cannot rely on font glyph having it,
// and regular dot are typically too wide. If we render a dot/shape ourselves it comes with the risk that it wouldn't match
// the boldness or positioning of what the font uses...
void ImGui::RenderPixelEllipsis(ImDrawList* draw_list, ImVec2 pos, int count, ImU32 col)
{
    ImFont* font = draw_list->_Data->Font;
    const float font_scale = draw_list->_Data->FontSize / font->FontSize;
    pos.y += (float)(int)(font->DisplayOffset.y + font->Ascent * font_scale + 0.5f - 1.0f);
    for (int dot_n = 0; dot_n < count; dot_n++)
        draw_list->AddRectFilled(ImVec2(pos.x + dot_n * 2.0f, pos.y), ImVec2(pos.x + dot_n * 2.0f + 1.0f, pos.y + 1.0f), col);
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
    unsigned int olen;
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using misc/fonts/binary_to_compressed_c.cpp (with compression + base85 string encoding).
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
//-----------------------------------------------------------------------------
static const char proggy_clean_ttf_compressed_data_base85[178490+1] =
    "7])#######0DM@_'/###I),##aq0hLYqH##iDxF>p8[%>+2BPSk_511'[n42#_I<#+v]]=c+RW%g_pu8r-_F6ttaY#gF%##/MF0Fw=+0.9I`Y#L;$##vW7'IeA[`,`j]Y#jt*##r9Z-G"
    ",9qo`.1aY#pD(##khs<B]'T:v^B1_A<v,R<'%HkE%CgR]bSVw0bYYY#Ue>UCJ:X?bVlYY#1ch--f'TqL;5I&4xSHwK@Ubw'E3n0FP6P7BL[d--f7)##mprhFSSf/(@]_Y#`9&##xwS=B"
    "8:lsH^Faw'1=o92N4^=B]4F8gK_]Y#8.&##'rGiFrpEchrG]Y#Be)^15+@fGH`AU?Om`Y#Jda9MmIZY#4`-F%N83Mp5E+?5G>cX7x<XG2mg<v#_X`8%+w4;-_nXlACMYV$^CfJLf^uJL"
    "ku*87?.YW#fw^Guw3Zm2dNAX-6pk-$k,';Q+5Bq#TSv*3$<^7I'4FW-lFJX#3ZF.#Ie[%#GcwN'nLE]FffwD*q$Qn&N.i>7cku?9pgqGht9JC#NCI8%WJ7&4^:GI)?]d8/K)'J3Xcko7"
    "6+%<$F>[s$I;VE3pX61M$'Uuc?;9f3xD8f3#`3Ou*3c31t-c31<jk#?Sum+Vie$OFsJp%#VT0@#r3^BF,8dY#50_n+c't]+2G#i7:$S0:5.Uf):Kl2'tAQDc/+RkLT69t@&Ik[kd;1g)"
    "xhX;.=nHC#0p/6&*vc5&$FY^$.:2j94bH_#PfMx5'b*D+H+E,+mL^gL9Ss$#5n](MgSp%#h/=U_XLI%b4dRc;lvS$0O^`$#,Qh@#?4`T.-NRc;;%Vp.@')$#caGb%_E]$#qP#<-?jn6/"
    "e[lf(5#Q9MZM#<-d_Qt.;lB%t890X:nq;mLaOPHM$oD0_GUm'/8oYca+[<pJvEZEN.@7*N:llgLh=XH&hO^s&eS(C&.V.C&&-5F%j2*^#,Wio]jm<p&;u0H2+NG$#hk[],@3Y/(_/)?#"
    "Mk<9%F_r?#TdC0(C3Yj'R5-pII;,##Qu0W-f0Sw#;BWl%oqnx,?<:,),_hoIS2ic)X=;;$us@&,u`UV(bsTU%r67],mkHG$0>>##KYC;$.AcY#pF-vej<@120Y7B3;tj>7SJs6pCQ6/("
    "0Pa@5m[C3p@qAg100^C(K%Q;p(i7jaE0KpLKH0o/Z9bi0P->n07djfL<@#-M@IT_4jc51M,HF:.8>l-$ok&>.*c1p.;F'>.KH0I$^d]iCk_ZiC;6Kl)9:0F%$YQiC6t/l)L4xO)5*x,T"
    "/=9jCj6jvL_n7T%G<j.#FHtxuD30q#fqm(#8pG#-Ot?C5o7.x#:ef%#5wW4B5C`t0+=V,DY1UF.$Nx1(.x2Ipq#*aal:=3pqE<E2-j.4($)LH(;LYx2,]39/XkC+*I5^+4Yq[P/iB7f3"
    "%Ue+4BYB+*8v19/.m@d)oYRD*n`WI)OgHd)?JAj;(bl]>tp.O(ZpJA,](^l'jdP^Gtp*%,?miK2AE#8/,/BN2[ZEZ6(>rs$gq7t'5.(j(o,?R&L0m)4r?U41as^^@U3`,D&OvG*hJMk'"
    "2G7m0*rg(49$oDE*DYY#aBCfqLV#v#MDW]+(g:D3EEld/6RRc;W18v$v2v2v]9bw',e]@#p<a=lO^C_&B9niL?5+/M)w]%#r`Ys-T(bIMhPg5qhvK%#CtZJVquZ#$_N4$-EbEDaUPvlJ"
    "csTB#r*nl/>t;%,^gLC(hNl#B(8s/LpGv&4Si]LV2`W_#toWc3.:9o0A+@W6d7(+*3[7--[5ZFa(G`x#)bT3'M)@8%VRi5K)9G@#j`-PfDTZe2m0wY#s7+87Y&+22kbW#AuZC3pEaYh#"
    "4<6/(Y,&hh^xPBk,SDs@[_BeMjTiiLWpQL2O->n0T87^$VD)B#oNeF4_R(f)-<Tv-KO<u7?)m,*k_+D#`ULd2q7<PS,%*g3[x>,49NnV)bM7L(Rk.[u9K^F4ZA'J3gp?4'dDDO']3@DE"
    "6%>L<E;v-$il?W.gMLv-3A7=(5RhO;mVO/Mf8L'#;ORwuW9dr#qqB'#DVs)#49187kjDT'KB$x.fORc;=3F$#D$v2v+BBX(/E,F%f^HgL*7NY7U>on15#Q9M<%Vp.K#&$#>t7L,;b06&"
    "qf<A5lhW5&&DMv-B]8?7Rj)a#3Di%#D+S1Mj+:kLI*[D4f<fI4Ev]62.lg972:IUAU?`tID?[?,c>5L#?SbAH8$H@HH9,/(:i>8pFa3@5=fLkLp`6/(pKX8hLQbs08L<HpM.a/o87@`("
    "`X(E>w@rB#/MVS%^xMT/u;Tv-h?7f32]39/xH:a#S,m]#Ynn8%G.^C4Q7cGMDu;9/`Cng)^^D.3H=UhLn2*R4+87<.ekm%FPMU2:9,OQ1_cpe*q;+C5i91S0uR$N)vqi?#f2Tf)*4])a"
    "YN.,*MZjH*/U(1(=+(J):[dc*V(x.2ajn$?'E.X/p,]K)[_j=.<&,d*>ueR9Eq_Z/+5X5Kc36m'mhT,3of`5*aCcw5@Hbu-4d=f_4sVV6/<52'm%I6#Gtb?#?Nu2v`oUB+@s12'X<$0q"
    "q%*=(t[T**+k$B4]O23'84b9%X@087&otghH*wGGffFY9<qkA#8$.Y%FomF8Ht*^6(i;#7eR'*#-^@)vtYik#aM+S2@;iQ&PEaxFM?B6&oGr$#?3/]%-)VBk_141)l/_:%.W8f3X5Du$"
    "s8?xA;OusLbh<HG`7l>^)dJ*lk3XD;<)Q$MlsPoL_4;TNl<%u/N*gM'F:V;$Lv7F/:1lZcs0KCMJD^:/XkC+*Ynn8%ck+D#9gc3=s<usL]+vI#>KS$M`5]YObbg3=x3f3=&Jr'#+S3p#"
    "$Qj)#`kv<6j[Rc;DIBU/r@Ac36dCY$2G#i7/hXU%k^h,).()D+XHDO1IhUx5<pkQ/`:6##4l':kHR>D(j$*0f+r+@5RR+@'kN`3(Ilk%)6&_e*`.1>3DrEo6B@w8%nZU&$pYd8/$w`a4"
    "s`[D*jKR+48ZVO'qn4R*I>`:%#3g;.RE&?5Aojj12F8m&Y&i,)`ja&+r)63'Bl/T&:VaS&12<8&@Xns$]%KQ&od*#-9<TV.>Z,W.%5,$,_%n92j>1:%LCM?#?IKv-R6&S8wU5c*H+7s$"
    "R$N%&a.OI)'T#W.:q3#-^[t20l&.H)TqW5&N&`;$Y1TF*M?Ao823B;.E),##sgBB#.EZ;#xa8Y_UDD^QlXH/_7Cv2),*.%%k_Rc;*`Y8.(nLC$?AsS])e1($&PF&#eI&W7[3j*.M$.L,"
    "`o1I$s[v>#1?pA=b.W*@9JVa<HN&<QGw>N0PBK6&Y6[S%PdHT#,YCv#Jo3,kctE4fS^VJ(P4_t@*d0Cu]&Ym2m/uT8J>'o0k:=F3)`OF3>HF:.Aa@NUZwSh#2OSxZ?A+/[6T7K1`HFI#"
    "&ji4#?qVV7mvSa%P<CG)Pp,R&U^c##UcYDc7,DLf)Q#'3VOT&$B7)[Im3dg)NCI8%OeKZ-@GIQ0,@L02F34+5-lJm&/K$a3*m4(5NRWB67FlBA5gQxb;2WV66h+/(6_U;$bOQb871;te"
    "dQSX-w9DJ#$tiW#0kx?04aE(s.U:nN&W<PAA-rvuqAS$#8Uu2vt2H2(H;Ut']+oP'3H<=&I%vW-<3Mk++Hl'&c;#?#MvR/MOY>W-hh-L,`dJ>#bKSa%jK*-$<dRB#,>fOf'Vt;9G%.m0"
    "HO>s.V%M*%*dMD3ldpx=Yq.=-t7T'+YkuD4.+dq&1u'f)?Nug1:s+02E5v=cxuP6ah76X7;/,##+;>##[=UG)'2>>#=$G/(v=+87M(-4(?xYXN<QM-M]jA;KYMWL2<aCD3m1i`*U8#mA"
    "Ii08.?nl1gTIRP&^O18.B'Ig3O=gu,lXVA#=>auPVvp]#+g8c*A-,n&jP^j(dP;s[]lUh-KRId+p*Uf)PcUW$X1N2'-Ksb$Lw=#f5wN<h&Z>*j`hSh(,<v2ANW8g2coZh*E'*N9telS/"
    "l/8gU#>=oD(dk7DJCg14J>#G4CZwL2=n@],=qIx,KNQXVV8CXV@(fhF-,mhFJ4BS/S,Zo/L),##T,>>#][F.#M,GX7B<[s&5rr6&A12w&G%d>#A*pm&QY7X'=YG##(.pJp0uED#C>AUu"
    "$VX#3r5=P(^k2Q/$F.J3HgfX-=1&2:`'H%5Iua$>7I/uC]o'x,c5s#,(Uj?uoYCqLZv'kL/K58fo4lo.Gop2)S]QP&+:ZjB;0ED+fUb5&K;;$#(xD8pP>>S.Zws'igwJj0k*eX-aT;E4"
    "$[qA$GR+T%'cCE42Pr_,F1Zs*Oxs4'$[S@#FuT+$o6ju7(;*=.2sfY6eG`:/l;dA#*:1)5G4iW-50/hC.gU)34fD)5mx.606`TV0&#)U833w501DD]bBM(L2^Wg6#pU]e]Q,871C&l;-"
    "QY#<-7Y#<-2Y#<-.V>W-8sY$0M]S5'Ra2:0Em-31MGS-#YIY-3PP#1MtjJ60gd_h(XRNp%p`kgLB_H=7^KLW5c(^1(%rV^cfw^Gup`4Hp'BoN%v->n07]R_#q0=Q/p?Z)4tB:a#6M:n:"
    ")Rlr)s5/JhpBgq/Q5cL:m,@.<rWw0)Nd,40te@-+Q3B,5Di$f*+9&+46he;'EbnW$6ZSC4`=`jCs3QG*?ioL($%8E+2ies.1?$+<W7V-4B/5##?,>>#m[F.#>L7%#a(xR&UB'n)K0>j'"
    "Wk3T%L]NP&NH5n&ni*872[)22Zn8B3=mtghB]B.351/kb[@Iw#,uJL(S(/T%@MHx6&+C>,MShv-&eQ^8590uua_5h<59YZ*<+/),$rg=P^'@,@&T@UCL),##;?AX#^S8Z0n%iY7H<w8'"
    "TG3HMkBUiBqB-3B.Kf&,#_lO'SnQ)*TLZ$&99K5pdKW6pQjK,;p/n(#([Qd3W:uD4rN7g)YCn8%#DXI)BHJs7t(Qm8P&ko7hN;Q;=cY>#=?5s$:4Yg,i-4g*^=09&uxOK3FY*8&>:L$-"
    "v=.=;[GM0*sofs0r].v$NU(gCjF^'+E/5##/@Pf%6v^]FcEAJ1[=Yp'sc2wAqn7]#N42nBc:.2_6:Zm(Q'pB-u0k6)6Yq/Djk$-DfPOO%ZHWW##d86p^0A1A7PWL2fBo8%XiWI)ib>&m"
    "P<--3%8Y']K7#*4vcMD3RIR8%?wgE)ZYG>#'t%E+sAqq%H'_1;ZJeI2wan0FD/R1>4TP[7?RNU.YZ+9%fF>?,ZC(J)PJi-2iV]D4WiIj:>&K3OAYDQ2hP$X-:qbp%U6Xc2?ur=cI:NM'"
    "X>H6#TYPjBkq?w^5(7w^p9Kq%P2?_%3l:$#jxEaM0);UuQwXL2$8o]4ZH7g)wQI^4W),H3wYK=ujs@V7x[uS7?UcW1wB(&GNCV+#LaB#v?rap#f(+&#0pm(#M'8Y7[ugh3f*l^]TjA,3"
    ";1ds%VPOO2a#F/2Kv]q)X:l%ldQXp.D9;$#6=Qn*LSc'/>A%C#HaS(%upu2v[7Dq%e/ET7);^;-DY5<-@YnA-jq$A-l,/3%/jIP^Chc&#A6*jLP;Xp'Sw>w-'AM=.iI:[,Da1o(gw@T%"
    "e$:9&c$l>5hQ&##7$)22H.N7/>*(@#Kk=%$*<p;.P`Wh+opN[&VjY/1_fBI*Dmtgh)jU/o]XEj0a2Dn0t?Y@57S]o@qGg;-_Zbc%&ckA#^eN_&ZIWQ/I[5l1XHSs$>mt;-ORw201A^+4"
    "=S`(ae`b9D2nYF=itee4tQ(41((0L;,*itBV[&Y.i6PV/].TE-1%+&-FY#Gi4GQY-RBxu6b>2l0bNMb=wpC</.UFx-e2/U),//P'a+Ro/_m&;QeCf21NJ1G+lIZY#OR6uul<OPJ`*EM0"
    "t`Zv.NlfE-xov^7;vH212veE-o2xU7,B:l0R6Xp.jc7I,`CppTGG<3pD'O'cU#>#o&&mD2c)))3wWAw^FLD8.-G]fLT`=u-h?7f3twC.3EV.T%lsxF4gi_V$oNa?#fNE:.0xB/)^j)E4"
    "$r?(+93PLD[7(0OJ1aUKY3EJ:Q83E*OtkR0;W,8/U$okC>;]90L,wV/iQwfOt@ZI#kg0wupIM0vp,;G#xf''#5,>>#;?P)3Mr,U7FK?D*5bN`<<)-F<0x$lL:Ur+v^N6t.bA^as)pK@0"
    "3?/##$tTB#$L9:#>doW7@c:Z#<In8%0DYY#/`_V$D1`$#BR-@5Wc:D5*g6/(BQNBkao`i2Mb;H*YUQp.I_M*.$nl`*q0*L#-q@1FJ<?4#Mj9'#eT*T#D$ffL)BuV7;f$s$;:R8%6^@D$"
    "u;$DW7i9+iNotghZUY@38kLt:TZ@IMr4VB#RO$QMM**wC=bA>#72=T/v9DciFVck1'/vY#1f(Z#-2>>#kHM>Pt=`-MqAFD#2t>`-xAX<UShwP/TbK]$pa#JL`b[[uZqs.LZSg<UYH^<U"
    "C?_&v#nXm#f.4&#p8YX78G/,3>Y6%-?U'$--Gtm'jT$[HfE#R&xYffLW<iL7Ni;/(jUU;p=M5D(1lWL2?W8f3p)g8.?;Rv$cVUp._-f<$UpY3DP35c,LESM0Y+]Q0n*T>.Je`p17EsN("
    "SfH<.[7[@-x7&P'x[=2(w1sO'<0jm/rpck07:C/)lG&c*l1m++kU<t.s0BOTB]4**2@rw,dDnd)NTCh.Z_?K#nad65Ix]E#v.lr6PwYa'b3^4_7Cv2)Ktb_-nI7q.4qE$#]mnX/8.#i]"
    "68YR(ImoP'qF%^5J.;t-)_HgLM=2=-FF[m/XN#v,nhRc;>>^;->G#W-Ei@['TEe--aQo$#rY>W-m^=_/Y9`v%9Vhk)rwE.^(jYb%u0;B(XFC,M59MhL8V>B(E&1j((=tV7B^5<-3>8F-"
    "$>^;-#>^;-#G#W-g1Lt:v^8I$ND$`6[#&U7p84vI3p&+4Fa7&4O_*VAaa@uI*j,[-]X>.V'r+oJ-?i&$BQ+v,N)qoI2AKg<wMOvI?9jv6TRvlL;N.0)I>Q;-C3B>,K@#,2/$9M7aH6:T"
    "CU,@5VdKpLQr8H#?XNb%X?2@5ZL*N0Mtw2AlQSm2e=N?QG<e/=vtFR*iRS[[<w9a#*%C(47Wp>e>&]iLnu=c4oFL8.gmFT.`<K:%In@u-:S1KM5HKM#udnH++Sdb+edLS0[9hk1ofT*+"
    "UM2A7FZCQ&4'8A,=4[i=st2$@R:rH4ri)N'm52k'S#i,)1d2^4,5*f2vk+k::3Z/(6;?P9OStgEJC&w#1jLv#60DARCocG@FsN%?;3ln/$1,;dS.G_+G/5##-,>>#']F.#DR@%#+X95&"
    "lXAW/-F7G;&sTe$crZS%YB':%H2<E*bSeD*<8mT%/iLZ#7wDW%ku*87^-Rs?,x,@5>3lZcxZ6F*>_ln0eY?<.@vNl(a:,i(NFIw#9Co$$N3d(XF3Iw?W*M$?Z*UK(j`6pIU;%L(P.qQ#"
    "`?ShuE1]+rV-=#76%*^6f5>##S^oX#EtWrLa<d&#=bB]7b.qQ(OEw.#`cxi'$C;$^B5^I*YR[/_>-4H+?8Kq'VtVZ#$qVL)r/2[..r<k:BmKi:2$pmC0^W6puUAC#+]iKAwU8B3x<]#A"
    "ZM#'3E5WO';]d8/K@XHt%(YI)].B+*4YB+*&.3W-u2Yx@YGnA.hsHd)b-ns71'J?\?DP';0[S,l(4N*Q1+&Bw,9m0$-^>ok(S71b*+f([,oPv1;v9kp%8PQp%rkID,j9MF*ERTl#c<3:."
    "N1`jCL_v_+;PlY#^*,##`#M*M#NCsLak7S^JGc?#Bsn.MIF%V%d+ZY-M*R;-D@kK(?K8#,CZ:i:SZNv3j#E8pM3=/($8'hhOYh.Ag[b[$pY+o$D,m]#s-bs8$E0W$$@7J*XOml/QjI0;"
    "t-I)48lu;-n@dB,?22M:N2h2)nUZ;'-^Q[IfgZN'S[7t'&qCF=R*-5*fGA_+[g5A4LpEciRb//(%p6A4g@2N9UKPJ(`.RH+3oVO(?8Kq'aA/B+cXsE,1P_0*GHf;-vWma$h<ae*);u&3"
    "(i=+3-t]5/C0Tv-wL9p7_PGjB@TnW$d*Qm'U*Y70HC2?#,&WkD<I5Z.JApY6Zl)3Cu8Qi;UtWb5n$q4fcC>@.;2RS@NrDg;F*,##d,>>#^[F.#kN']7hfF;-jWaZ#GO7<$d<GG23Fsl&"
    "ep;T7;<e[%`*M,ME5=T'A*k30+7D/:x?r/:1V$-+Mh=n0t=Ve$kUvgh?6kx2dNAX-N.OF3O$Z[-Er)8I.DfU#_ItqC*d.#DS;fi_dG5v%2.l22va_v%2Vv9%G'pE%=.)?#EnaT%EBq+M"
    "1.,GM=(YT%$m:Z#8YlY#MkF8M`WKm(*fMkF5)e)Mtg_#AKM@xMZB=/Mc:r97JD9>#,sAkFBJtdu/@Jv$w0lp#kLb&#)Z5>-TJL0*s>SN2L<>n&`v'^#T+Ne+4>>R(7Yo^f_]EQ7vCA_A"
    "16]t@;mFHpdlWL2`ie^$:)TF4)<oO(J(;68C,x;6pIl?6;e0t8$cEQJi,Ta5g@9Q0*9MY.S/.;%FOKa#/Q;J<.Toe#(MsRee35*=a9'xIcaZN'RRrW'cXJe%)V$##YPFDEjYiu5#PE5&"
    "$JwW$0FI/_9UVj)9Hr?%EL*p%ZU[>#3+eS%4uLZ#r3DqV/cFQ769R*12KOBklLN3XR)kJMMc0i)T'Uuc:SF^J3L+Iuu2iZuvP6Rrx=8F-d;8F-L6hw/om0t$ua[>#C-xN3?-]0:?:VZ#"
    ",;iv#&3=Z#-<9B37#Dm/@9@UuxCWL2[bO[4]H7g)JH/i)jHfL#v'otC;[KqCmJCqLs;i#vQan#M`e&%#htGi(_8*hL;x&l'<IET%]N>/(3c*p%2sGp%'-1ucj<Cd/aX(9(+uE4f&hFQ/"
    "ecWF3'K+P(H'*E#%MI(>f87G,tUaA-`kGT#g#Ee<etdG*<,KG3r8g0Mc,:kLwF?)M<qg%#fXKl'gf^l8t#lX$?kL^+S#ed)6uUZ#rn^%%<P2>7_OOTuSZvgh]S#'3Pcn$$MTC0ccsVt6"
    "I&sR'N^0#$OlPZ&$>m++a$`I2`mW^OouOC50]$E5)SS)=usK[uvUFeus&us863p49TMH^,TEx81A]/m9kNOVecp<]$_Yd6#K$u'_30;_$u0pa#2G#i7:_*n//SlY##K^S%k`]3/+$FH("
    "m4DwKcf*F3#n1Seq9VK#BJ+Iub90C-'70C-L5%:0f)S=^o3W$#M0A_8c42$#cB^@#6)-F%jau2vxN3pLUZ@U7-F7G;$/D;$2%6U7*$d=7c@U<-6jA:'DLRc;p%8h(g>aT73c><-9f8,M"
    "d$VT75R7G;-)MQ&l@?U&(KWp'BWw;-=_7G;aOju.Z?j$#VI:_ArqM$#h[$<&#Cp9;$p3m',C,+#LH0g)%fW*+6/>7(87QU7WQXkMKwpg-3JZv&X0E$#+kf'/rl^'/iq7@#j'kB-*l:p."
    "F1>^$aAl'&#7TQ&tUKF*^<=X$?6Nn8Q($##[%VK*9P=a,D5WeFRK?D*dDRD*0lBJ)lvX]#gSw-)ZHBU%*I3D<3)^e$NfED#A/A1A]A04pS144(3c3@5QOm?I<XrdifDZh21q7t@Y]afL"
    "=k)/:#aSq)2V2s@>xdMMHqV[,)OT&$lYh*%-qxw#*13:.dk>^=N<pb*mxSB+i*DgCFbns$>LrG;=sU>G@wAQ&Wm,o)w`n_-wc6[7geWaSEfo=l$V-s$2pFhWJ##*#R8q'#B8eo#H)V$#"
    "x;.3'gRRc;UuC;$)p$M)g-=@7W$x+2'<tZ#=4tK(xQ1$%Qcq3(7<V$%FCR<$;fCZ#(xS0%]/[ohLDxn$wxrh;:GTZ-RqUN(6P,b[f9,F3nLuW0=CHuB/N%P1-uE$fFBOe#3]PqC&Ww1K"
    "dO_Y#2d`Fr$[ooIUuou,Sbi,+PYAfh51DN9xcEA+I)lN)ZL/F,G47L*iZx,5PqEH();u&3wo$6)nUNd;83Z;%$)nm/SfA7/I*ng)@m$B40TL:%C2$1F72-1FPY,P1F)Mv-b9oHN<k(UW"
    "B4p-$biw20Z,>>#q01/#Je[%#pLPE*#b]>#[Ji,)*Fu2vCkI7&J$DB+6-+1:kv3L#XFET%VW-GM*Z,@5v^.4(0^j,MQ/(:@*^`#5:%1N(H@<u-#/NF3J29f3#)/sLrFvTJhNu31*>]T&"
    "^U7&6hRKKFuNwp#>pUv#>)gY#5I>G4jMdc%e,Zu>w?srH[[-5/UbMg*v-@A,N+Em/NWJP/I*)b*:FTq%mjHGM=u%7pvmfZ$Bvatn#k`i2_Qes&YLFp@?g1T/=.Wu7Vdbb+8P%fG3i=?>"
    "uiHDIT:2;K1MO'4FE)UW@[@rV&V]5's<?U98+&Y1)ue&6^',f%Hr$##-L[YGa_LS./d,E,90iZ%de+>'l(K2'`'+tAtoD/LVLZ5/4pXn8]qN%b@?MK(X5NW%kC)Z-Zp:,)NH#3'&0Up9"
    "$Oj>72[)22'%?@n@wNBk(l8B3rnH,3WMZ5/)IW:.-#qs7*/5d3[1$B?VLSnhds1^-e&u6/K_GKL0G1W7^AX315k)j2d:^^+e5UYGDbih38NfC+bW:'-B/k@m=M_8%$R2#3epOf_U),##"
    "@?AX#lZq/#fwUS_$J/kX%]D$#r=*tA5hX^#.ex$G*ZQI2l>*hLu6bd&a1@k0'-MY7G6]0:'6^g:L)v01IE1t%Y%Zn(d:EDuQ`-#5m:=3p&.l(1#$FH(6oC1As0/(#K(.x6Ld)i%7jE.3"
    "Jk>l$s5]5`9#+&%a%NT/ps?Un7,Ir%EE3h)xwI@-1S6D>fN.Y662(x,eE3;7`Dc+H^`vU.FEf;'$,]-)t#UU%U#+$6#ffK+`KqH)WfTf)aof^+t45%?leD0C[a]Nk%p###n7GDEIPou,"
    "vicY#&P<T%$$r?%0]l##N.i>73*>m/S^V#377E:.:INJ1.i@[W[:EDE0tOu#R_Y'8c2aFra@^'8Dnx+2qqVn8rd?S)]JHV%gdhD3?6]0:^#::%'_p*#YT<8pQT6/(sAuQs]+=j1IL+F3"
    "Qx_E%:Vo/1ivUU65:_nJ%krD+&K_U%7uLF==o^A?:hjhusPn@-@IG)4x0ngufa2&7ekUH-=NYO-K0LE7R'Db7uXO2(>e01(s)27:U]C0_S*`p'K9_[.Zlk;-P<Im0Z%ffL7TS#MP[Nj0"
    "E2WR'qpcq9sUTT7@)DW-J,3**L->j'gNJ[#iYnD*hciS&lejP&c/;K(=R@[#[WiBm'7R=7aPcu@a97D(xu2F*i./(#-2Ys01cHL#*$.l1q<J79IS4K1K>(g_s?9:/xS@a*,#j0(*Q-V%"
    "V^*N1kAm#[x%VL27TWn37e:=LsngSWVKS6:3`VB#hd1r.w%)$#69=T'_Uv>-ojX,%PQT6aMRRc;gqN%b+O#<-H_e6/&DV$#J.<mL^]7.Melmt.ad(VdT5w<(k?He6x_Ktq-?,F%pDNhL"
    "XIp/L(C;s%)@)W%eY5<-BapO02XRc;4CTh(qb%#>@XbA#8:G&#cL5p/9#NS],p7F%HA73'ruTK-mMUw-%m9k9MbrS]=SUO)@H/9&9X7G;DUV=-v(6H.g-,JL/WAs%.CvV%bs%Z$96GqD"
    "((.H4D.0Y%hbEX$ZJja*4)R?$t;ZR&5'F?#5d&q'Z*jZ#Q(`_,NtNE,3arG)'Au?,U,v/(IT'T.vt:F*Us-i$o#>#o(FUh(K:MkL_-JD<uE'e=-nD^%:6*D#:8&c3g6rI3_=q8.Gc*w$"
    ",VEj13(A['HIT&$LptNF=KvR&F<SYG?tAQ&(kF11><NL)Q+0q%I7LJ#Y)u87k4xh(x6G;)m:G:0vIfvKAtq<V)<x]Q#DqG7EiBtVP9?\?K;2]J;iKt$#/f1$#faMs#PYI%#r*4=-8s]U'"
    "@=hGMZ0E$#d5[w'g,;W/7jA:'['Ft$0J>##WH,j'OLRw#bo/F*Z2;K(Fm$h(Itf2'3fl##cIA<7m@#12cKQlL)?IO(%D&.$6wlZc:?B3km(:,NjqN//R9$`4vPeZpj<$/CS/KA,W5DC&"
    "qo5f*kTA6D8&Pc3Z>@E3ClioKl[2C?&GVS.:Z<xkSg*m;N/l@&ea/%#*O;j(BM###+EKs$Lp3&'RMwd)Y-B2'D:2$#?njT%2Y1Z#F:S;7G#*_SPr+@5[1v>3Jx)T./ARL2GkG1%^kY)4"
    "o4]m/,:A[u.t``+<n37/ljQoJXX2KMoPY`<uKmh<5S7?A5A/joBFr9:W$EF&sFPfC1,%<&-O./_=Xv#McK88%9<u2v:=.3'@(k,)WDr0(72Ap]?Ag6(DK0o/:WRw%G=.@#:uY<-]%Sj1"
    "GFI[#7pB[#+p>T%(fe)M/b5T#@h&9(],m(1bc45(V>Sm2U]rB#]+:W-g_-lL$S#<.8;Dc`be2d4F](Zuhp,x63NbW#TS;78<TC62[YqTAjOKnC#MAQ/FU.H#h=u6#D,GX7Opx>#qr$s$"
    "ppNS7/(wo%GcUS%^BGg(Rb//(Phns-/^FpLSdQL2+d$i$9:DJ##r.ku-NS*Be#cpLHD_kL4#`%MY1@##NmT@#JatrH<[e[#)85##=-uf(Xfp9;&h(E#9:7D(rq)f$nEp/cP0oH3L(GRK"
    "i&4h#T'YwLxX+rL<Wa6NQR(>$W4?mLNW)?%5x:?#o`TwLFCm/(nJ5j(<g,<-%A_h$0?RL2,EY[7-Lp+MNV3XV@X6(B5knBSnV>uu(iGDERR)20*DMv#1Vc##5:nS%6F<T%1G^^Gkc*87"
    "xsbC#6TRB4YcD<%<DeC#&7]ZKc+VI#DIh?jiY5lLYM>gL.4O8vB7>##13bZ#n>n&#:VR[#A1f;7n0dTi#TWL2<0fX-f*?ruhEpqLi=nj#ECqf#bjbk1Bh.v#Q>m1K):BS#R<S%MHQTN*"
    "q3.'V`;BP]*?x+#Q/$$v*9(R#j$M$#:[9]7THXA#Eur]=J*Qq&L*v2vT,_p&sT,/2Ul)D%)ogi<u$=^#b'%U7&9cu>Um]W7P^vt/?>2$#f-f<7dS%@5Prw2AfERn0pYTBkawXL2?Ll.3"
    "ok,L%4M0+*3,m]#PAlw&,23T%Sl2Q/Jg2c)0u-t-.*Ja+ddD]5#rd_+F:,O:uwYD4:]st.?;Ls7_.8%>@YZ`E9bAm&1e%a+PxRf2).,onoWNq.@B8'5l^Yj'1->9%txkgL4?*mL])Mq#"
    ";n:$#36-^740[s&R8e,M(UY,M#7,,M`/;hLaDL]=de+>'l?2d*8>[1*`wSM)Ex_0*beai*N0GS&QU@%#E+3:%:]EQ7Pg,]@1[_%%%43W-pqU*>w*C+*mo(?=HH(a4q5;]$iLNV?a,UR/"
    "jliS&'$sg,no+M2?]GN+5P7R9F^IN8'#2vu`Wl2;pJ%?.'-+8R9.8q%7]X,#(p/kL,3cT#w0u<3.M&I2(D9T7$w=%Gx<*Y-ajh+Meh6n&Ft6;-[b/;%AV)Q&1@o20+)VBkSTWL2$:T5)"
    ")j:e(W4Vv-JTp?-;xpT0e::J)7aO$6N(mB?VwPi(Wg>4:-J:Soq(XW/R7hB%GnPd3*DK[%S[83';`X,#odHiLP_6m#;n:$#4TZ^7=;6A#DkOS7oBfU+)/$A#^&=5&s(m;-0.r5/:x;$#"
    "6G;mL(%Y&+W$,&#lOV`+E[78%Hjiv>Ij(v>t[L$^LxJQ7Jmtghmu15pBS9`$kUuGGe:[b4^^D.3s2u6/?;Rv$S@h8.D7BT%,E4c4/u@o'(=wT.7MVB#A-J62kUk#AO$1RJN.4c#.oQS%"
    "Rwr[k0]qKG1SF6&KTnW_IqnW_A9ZT#vme%#_-iS_K%I%b#LwC+3EC`=JR:97cwVQ/n1T+GJ(-5/)?X$#+bVQA=sC?>joa]+@7?U7D>@w>;&?@-Evc/EEf%B7W7q%),CO;p?RO9`sA:B3"
    "6H0dc?#(.8/#Qg)f4:W-KOjG*^uKj1w/-i;:Kt87L5/g<#HoI#?_G^;4MMK*ux&R1&s[Z.*.,**D<b:.1+o7nu2vA-;hWB+I)f4(Y*#k'G*D/F1%GM(1Rq/)QVOY.g*BloK:^]+3.f6#"
    "HqXb^#sah<7^o._q4T506,<Q/?h[%#[%Y?,QZvF#PL)#G%wC($_tU`+ZHbi*JiK>&)E%)$ZT<8pCfO3b^6S@3`4._c[mWL24]Zs0k_Y)4fQ,<89&Pj)J%1N(IIRb-FCOT/n+UF*cU`w,"
    "wV/:bl&Qh+=mo0)XL6_%tQsq.k%pdua05##HBiG#gpp:#Ud0'#7T.,)$%Ug1nnr$#8BS%%7BY?#V8L]=8ch(.s@7$$so+m'`mLT&8s^@#YbFk'iw]4Alr+<>JrF5pU;U;p`W<p7-Mu)4"
    "`GUv-.'?'v`5E.3#IR+4DbOT%BI9_0@]4Q'k)baE'c)]J2s=H*&(A@-4>BB+#K-ZAepu$@JJa;&iG'Z#mOGMF`tV[&-*t:._H96&;uefGbpM2%%#?M^%c'58)3?'=/AdZ&WfjP&JHZ+#"
    "1Ii._evg1)0e#F#bQg2'BD4&%28G>#)GY##f1ei0;j(E#j(jhhP7nGpj5,W-f#'q'tO]L(EfTN2kxmW(mn&O0-8oT0NsYP0b*aZ#oFso$)N;J<Dnm?tUtjv#fR_Y#=;A(sQl/;ZM]ou,"
    ")fe6#cAwn7RLRc;g@B+G>&Cg*=hn@#8h7X-;Jfa-E?1D-U_D#GQiOv,Wom9%S%Rx#/?###-hBIp(*RS#Prw2Av6]#A(]V#3m1_gkNK*78%Hl)49EsI30Xma$$f'#,j1fM1j:+j1a)+*,"
    "wd;3(<xoV./&m]#N`oQ0lboQ0(m-4'GIdk2j%X@k8THN'RlPg2aF(;Q0DQ/)#PPh;=ZNa*9Aun(>9Z+#8E.L,_Av2vR#_5'G'RV-Pb>A+2t`D+=@kI+#-tA+^gDF*Lxno%E%Aj'PA(W-"
    "x.S*[cjE+1(T)BMg`+G%Hw<<-B?(`&2;]f2fK;F=%Z>l9@Z6,aID]ENjrMA5wXhb*q>N'+()nu>U=Xq`5wMs6ovF6#$O']7Diqr$`,?v$%Z0'%%;<J2bdlR&gOK.M5wer%Z2=`+/g$I2"
    "bF`w#hY*&+D,3**HHBu$2eNtA=N4N3#d86pJ''9(XT;'d;f)a,91Fj10TF:.9*a$)Bf8p&),Qh2gX1]5&WK?PZ9(O10:VK#aBv-3`+^C4it$c*iWY*,EC)d4c)ZcVK+;+8ZwiW[mD6A4"
    ":v8-#*k<87_$gi)0xMa<^wi#GNsHH)9a1?>Q&.1(1^c##ILf+%_csShM%Z/A](tt.$3Tv-&wRq7p.qog9@9g2i=inL4Aqr.tqe[#P[(KC&wfuCw+vw-rB1_A)v,&#R@9U#&g0'#rLes0"
    "(&W9)&@ZU7E]qk)k_Rc;gGGj),v8q)xdM%*=&r(*Q[.P)HC1T73<CV%_Iu>%PY>N%:YPr'VOkPAYhV20XMCC,K>T>,lk]A#0FGx>#sNI)L*Ss$^I/w>n(+87nvj>72%.=7Notgh1^-9("
    "S=^;-i&`*3pr?n09f=PAg]*)3jA[L(:as[?6xc,*Q?WR8ohe,;o/xF4ae_F*=5nu>No<8&SXuj'PT^U%L6b6)W9BU%nkBF,1>&L(F)%x*xw8_uG67&,aCVv#/pk',D9;:/a3`:QUx)n&"
    "[,VK(2x3T%6CWxkdHAJ1>x3]%FBZ+#2G#i7?*]0:P^tt$r*UR'7v7)$2bAs@+4@']twEH(QwXL2o&@UMGktO%/49^-@kUl:OO_g,4#]l'q),/&]l0l%E(G'#B*FxuGIBU#-26i4SKJW7"
    "Pgm+#hURc;VCW_=mElx>V74x>=]d;-)F/Z$`d7#S2E=R%mbiV$sf<Y/IiUv-#`8R-i`=.3FmuY-hMfh2&it]uv7[L2h^q]4;If>8_U)?@/;J#8[Lv>@7%61<<lgk;9uw;7Uq(JhwTwp&"
    "v`6A4wJ5R(,RUJ)9GwL*)k<m'88`H*`$TJ+]j^6&XiO,Mg*QT%B)1W-ZoX0PNlWO'<f)T/pjB-%^%3;?b3a<-wBZK)en#n*Jar02V98mBsR*Dnc3]>#3[/6&Arh=@Q]qq[U*^N'04XJ-"
    "v2XJ-jh5)/eP(Y7fDsQN@q4%GwEW:.I7.>'nWrD+.[(v>8k?.M5)3$#E&HsALsrs@^0A1AawXL24Q$k0IH:a#S,m]#N_Y)4_J2T%08^31u8Cq%eW`I3M-4D>)OdL2F`JQ*7M7R9>*D28"
    "(&2?#%k/&=0//^7[UWp%n<[D]Ia/W7**,##(->>#%kg:#a=VS]E20Y&2G#i7:*]0:I;;v>K[ns$:i>8p69R*1PlWL2#RM8.dN1K(IGxfLx[bA#(DBv-EPa+3Rf-1DmG@Q0e,MV?3f1Z#"
    "'M*`WY*$+*)4?v$nJ(F4*NOf_84ac2)fe6#argW02XRc;YB(D-IWj/M(L-3'*Z@H3#c*h1Q.*5(_PAn1BDk&+Wml<%[OJR&:]EQ7?`9D51Vxjh+mE+1eJYx2an-b4X0d^=J3?v$Ewas-"
    "T@4a=*[Y#>uVrn':IZ_+j3RG*Y>Al1_2JL(>GR[-*7U/)qRV_,PvpC69qm.)Ud+<'d,)4'R#Rd)cPP^,QSsO'`KqD*d&VK(MkNp%^_8u.[m*-*k0JX'TWr2%6wqlAQkPfCGQRwu<V=mL"
    "ZuS6(H`RI2_;5=QYEK:%k_Rc;w,=$#=Q,?#@1[S%t1/*-/?K&=#w1c#M5Uf#Ufwl'DAd##6mt;$;Uj5&t9t*koZ7B3laQ?pw6S@3[)'9(agW#AHDh.An/41)9,B+4cT/i)(Uml.tB:a#"
    ":F=,%tP:%Q;DxJcuE5C+'O``+/F[t-ZAZ980hjxFI73CO-w'>-BohV$X9fX'Y(RN:)Y?U4UKSX#]3k9#R>cX7YWP(-]G<I)tDap71X6C#I^$-)R'06&=&U8%/9L3i(@Zs0a8kV$2#ET%"
    "NH/i)^^D.3J>eC#A.QH58Ba3(]9c',*LXq%6gif<QQ*)sT_n>#U;C`.2,>>#$U$A/?Kif(_5%9=t5;da/TRc;%=bw'JURc;-%Vp.@')$#Q-Gb%creR&8mSU'Bc:;$N;wH)Rgg6&c.oZR"
    "a/6/(+vEa(eZ^krT^mH3SLgCQ_Pmv$vb'`15TZ'&-G85'7ESA,:5(kC(-jsPIpJJ=g6N(?tbCxprZqH%7Zjq/t@e6#('@d^2XjtA?=gvHR0,F%KiY#MN8xiLGZ`v%>e9B,lbRc;ig7JM"
    "Wt)$#1X[L,T<7L,Z:=x[4?rHM`@On-RW8njdX?p&GKsS&xD7F%f^HgLD7,,MI0<q-+CKtC=UQL,T'u2vw,m<-IUIq.IQV$#n;vb%E0je-3)Wm/Y-F$#s%c^.hJ>&#%6:Y$^QB6&xGYA#"
    "p2rG)tZE20C%($//wU3(c]'j%1t&@#5r3w$0(%x,^OY(+SHTQ&R.i>7`fBI*1h(E#-0as0=5Qk(ge].2av(CGU,S4(;=^;-3P5W-wBh1XYtf3'Ik/[-4AXQ'sXF_$vn'u?[n)QJ$F.J3"
    "7i(?#?QIL(Wa10([F'q@eMDO'Ju?p%_Dik'A2F&%]#Zn&9uLv#lXZQ/kv2i2wh^G3+fJ21$`V$9J/T>/I(/D,I$'v7uBIf=vthtq**,##8,>>#24k9#N^fW7G`(v#[@TF*Zj,V%SReS%"
    "Y'Ot$=@<5&(+sZgmZ.A2Nn4$gi/41)^L?Z,?:$gL&F4jLq^@U/3'5<K,T=R0B@:k+k(OJ2EX*R/9aw'X'C%?RQZx;8Ptue<<f,42O3O#7uu$FIJBiG#2FoG6`,GX7qWJ@#]d+x#YsZ+#"
    ":LDd*WNZ7&[plj'Vr2N4^V-E#mOtZc_/hd(WgSm2f@Iw#*r#m8b,#H3Zhs:8&qp583`::.K=c.)'F^u.HI1F*73FU.6qd?\?=W@E*t/h6&ipTU%h`XpKq8]r/IZ-7&M%79%[Jg(m%2e+>"
    "b=T)=tIQjL%&1*#[Q[#MW%k$#71=8%C.I8%Rj=F^:Er?%uGc'&ri.0(5+OX$#pC_&$,TiL&6X$#t@FS79`cxF(2###MG`?>KRes$6/_s$:]EQ7qbwP/jdfB5_.l(1vF>,M7Srb=5hs`Y"
    "5Xw]6Sxg:Q08Bf3Yhko@E4l97QCEjC22,l;ox@qL$]/<75Om1^ZHRS.a?Jd+.$vO'e3EAFi3aq.SGC[Gn@5?,uw9U%$R-iN?L,@5LhBT%4t5Fk$,^C(CwXL2xR'f)@]WF3[jTv-=/9f3"
    "p?Z)4AC7lL]tmLM4@,gL)>&x$]h^e*d3k'@-lPD4-9g49d[1E-vg=5&#roDGxE.3<Vc8n0m-UWRAL)$/DcRs8sbcf`CcZA5p7]m/9W]W7j6wK#A#6V#2QJ>#0]/E#+DP##;@r$#].i>7"
    "a97D(WdW#A2<T(/H-$g`']0VBaZ`e0$PRZ7i>x8'J^q+M1`hBF)fa1+-,_?-UI$##)dwe)5XCG-FY1;>D&?Im<a=D(_dPTu/.Qm0T8c)OAPl]#T8R3k31q%O7:=>5mgf=-Q=<[6<_[x,"
    "vpWD+nDnP1u),d*<+$*No`97/M.PwpvB5;/e`>n%qnPuYrl*/CbCpu,FG5_+k-D<-Rsp6&+tL-MZOF:kGKITc%L@9c](tt.CLD8.]CI8%jiVD3$qIa$e(tO'F3,g+fKI0)Tg>n&GXkG)"
    "_#'E+fN[K)JPU7'GUX,)7(vu)dET2'04Z(++a+/M6-VHM^Or$-C>ou,M;#Akv*af1S9g6#aU]e]p7b$%?SI&,@aIq.BO=cV#Mx<(2i@v$*(,n0#&8l'n.F**/xai0-sUi:;e%e*ZpiL1"
    "t2[0VGea9%''xY#'-1uc2^'CuL8#7p[+D9.;mS?3fiP-3f.fO(gBj>3k)d,9iNK/)MAR,2Dc6<.0-$E3Y<Ss$$`^F*/Gmaleu3$%0LZ)49wZ)4u%Xd?iP9a+`-CG3*)l^(X8n0#nF0]("
    "pV4v.[m3D+3hKQ0=L$:&-1GD#N[B_%S.ep%6DP]%RQu/(9`]12W'At'wZGI<D@Q)>Z8l]8d>xU7ivSa%GTYR&EbV$#Cu=S#MI(QHrTx5/$6cu>t^%2^*m:D3ma4',Dk5*=T12S(8mKn/"
    "'hB?-Hrwo%b/%gL'J0ucjEo?3TS6/(A;Q+M#?mZc=TjB).Nih1$Ps+3x&uu-lSr,)u*K)=[,_$-B2GL)CVOe#+u[C#mcG5(c/5##K(CJ#k;w0#^d0'#bsJt-87YQ'XOdl/Y?@q7Fvf<7"
    "&otghk_1:crwT#3k9'=-;8Zs'I_jq..o:W-`]?S/+P7R9exqq.0xUs-b/V*34+#3'%F&j0b-$H30;(H4v,)BQbhvu#4p%crxB&8Ik-Id$;^5[7T?<T't9?x%aJ_v'JOvY#?ASRAtCjqI"
    "Q3pi'jD]a-0UPj+rD[h(,eKr%A:no%5o)W-QskT`#7:ph*TP8.vR8h2Z5jPJE4>)4i6:^=baMh)5AIcVB0/&,=^vCb:5pr-XP9GN2WB30Q(A60ctw#QbV$(#cFs%v_::n#J5i$##7Q2("
    ";)Tu&sCMa<FHs;QBbjp%QUno%J[,n&Tm14'5TvtLq?F6f^_9eaHrCS%W#vC3S9JC#Q&%6/Du29/;wZ)4)a0i)H$cc22KR[%Qf,4Ft?U[ekriv7qh>&DxGFqLs%[lLCbf#M/7K%#a#FD*"
    "]8KT+?g'1_8QK/)8H0j(dCsV7G01V%8]BF*F<0A=Mk<9%R';T71akv#JU78%tTIH)g=0RA[.2[#iX;[#fxgK-$1pK%5mtghf/_?3cvuC3$hCDdwInv$>:ib8;7.OX?P4hY.XfS%1/dp%"
    "-tr0#xdMRNb8xfEA4,c6EWkE[alMqL.#L(GU:t(N3n[V$qD_#.s2F$#IW7p&Hw8_#1x6s$+SUV$3Y1Z#7rCZ#Ka:v#7//872?1Ip7XQ/=i7:D51Z$9.7S]o@,@A8%Zk_a45Z[[?>sUi#"
    "x;D995);8%v,`Fr6<poI,p7a3(WH(#YF>F#0.lr67Cv2)ph@C6aD#W-Xj9I$e]C%#`vv2vFke$'9ucca432X-RlC_/B`w<(.J%B#j>_<.-4'#,nVL;e2[6iLCp4?M%0,A#S,uv-eLX,M"
    "@I6j%_5>F%LGcY#Jw:^#(?Q'A7(MU'@a6LE5k0H2@vfjK5-I--SP[q/dpMT/gfND#dI4T9HP4K;r>?9/]BFY-buuW:TLZJ:Q2i#$P[/,2'96k4N;4e-tcP'&WT<8p2=^m/;.Th(k%EH("
    "t&^,2,a9D5Notghh8OHp-V[Tc+J2s@+CiT.Z9bi0OnWY$v7C[.m$s:8-R(a4VjQs%e<Zs.49Gx#?@cA#`]tG<4iUkLSa6L2u')O'o)0I$Yc/12<40I$f9Z@+M=)P9]vc3']))O'@#:]0"
    "8:ugL9HW^88BQW8m&[Z.aSX/M6bIq.hQhc1nmcU)b]x;MKYJw8n,eZ.USZs.b1Q:v?5aFr2$2s6,)7A4+i?8%.B&##jW0/(qSG>#*]*X.edDb3R*3e3S%+GMsU[lJS&^V$k(e6#BU]e]"
    "+I5+#2G#i78ORc;,Auu#2JG##,PuY#;:I8%roM9&27^1()lpwa0=>n08I3Q/>DYV-aSVGR`q6Y2Vs+(#7=7wudp`s#US@%#LS/`7xH[s&YcN-M)EWp'k]8327mg[#GCBh(@ndk(7>Gj)"
    "Q_km)2Tn8'AS;<'%WB?-e1x`=VD,1:G9x0:@LPJ(4^'[#?KXZ#;g5p%5=P&4C>U;pb1=/(U_-9(6oC1AU)_4AL#+IQAJr1N)=_F*NIHg)IOPs.biqBFp]>^=]19'+l@BOEj#JP'`NMD?"
    "O)Se(o_c$,gh[s$_XvY#FKNFSpqoKO[7LqCp1Vg$IIQO2IX&D,`<r:Qx^r%F^@/,)-9`v%UMd5JdM7-)]86)c2G/3hT9JC#Qsd)*lj>G,=':a#0[Br(6sHV%3,t)3YIV8%[Hxw#'4--3"
    "JY;m9m'mGMi1HwLS_9(MfpkV72t?W&c#3T72s]U'eFCG)k<itMpWGHp0=>n0?aks'aXi8`DW?ipp@5C(wF)20NCVxkfcn?B?F.5/`ejT`oXi?#%:)%,_J($-iAu,MLTei.(n=&iDC&sQ"
    "Rr];Qn-wC)34#0j0B=AFSYaE*(nnE+:CoR9Q*BY(N/5##WZ_R#RtM^Mdp?X7cDo5+NYII2[vUO'V%f;QQiIfL6/h:)MS/875-&f.#rYSuUO>s-_'nt7_'*qKc`cl9hx)qKGVc',c%S.2"
    "DoQ>#H0Ms.`>iY(mGJ^GRqXfLk_8nWO6`v%Jj(I2UZ>j'3@QDc&U[Tc88JC#5k<e$']_87rrRfLV.@%-V9i/)V1R@-V<i/)YNCG3L^5o/oOTc3W&#d3BRnD4+c>0u,@wu#st3W-5+NK>"
    "8=+D#$Q-K#ehw?0DQt?0iVw+M;rvN^1iU;$WCS31;B-x2E16g)96&tui3-e3Bie@#eE)4#@acZ_JxH%b/TRc;?:90%DK<x'f)66&$7/b<v*xP'T.:w-T6]nLo?m<MSM#<-TGf>-CtIu-"
    "R6]nLL(O(l<uv##P)X.)M?XX$r>V,)P-x%#_^U0(s#Kw#@^@iLaS24'nmn0#e8>D([S-E#h;,9(T?Y@5q%CM9`*ACQk;KYG3=g;.XwQ-bvC>c40vZ,)/>81(.J]l'9Y]v5>v,g)TeXF+"
    "c&h:%wRGw,gNTQ&%$'R/4Gv'&(c/l'7wbf(hv<`jBP:8%+?<YP-]4O'U)1B#:2[V%arW**nEV@,O'TM'p`r?#4$*D#+PafL0r$s$BuN^,HkGA#a+-##w)Gb%o>6W.7(v?,2kYn*JT^U%"
    "O-f[#s*aw#[#p*%9K.XC[<9B3>Hl>7Z8fk(8REv@meEC5x5wGGZNHH(a:kIu00rTGuWiX-l;1I$8@-@5qlSL2Smde)_kY)4RBxP/m<E:.$F.J3vT'Z$J49l1vMiSe4_1E#-cmY#cI/&$"
    "<:@W$@W8F4:47W$c6I>#R5-_#k88EhJ1kS)tRx?$e:_J3k<2l+eRQk2tt6A6V2T:vOrA(s?`BP]LJHY>*`$s$.B&##)9@W&:BRF4s)nF#MC7QqGRR3#5Q#N#RCP##bt8m/#w4W-Hgt&["
    "j:x,&,I%/_ZDLa,&I5+#.00I$R`1Z#@`G*RYSuY#,S(v#74e8%%8NfLgkQ]6UwHO(c*/N(<to*%b.><kv./O`8Zki0Z#5J*$`=oTk>uYCT_U=2TcJbX<H$)*cc(Xn-&55/=7KV6M'g6#"
    "wMB@M.p0R'2G#i7`LRc;P3F$#CF-L2QVNN15(;D5mu7U/:UBP'vk#T0lZK#Gu$W[#N3at%u/mc,9FfC#moMQ&iS$X$-mv2A9ov%)YJ_.AA/q;-Nxx(%T.a5MnWL#:wpcG*(.Fm/hFq8."
    "T$D.3C_xf$Hc*w$?]d8/5a4K&a6bx-&vMF3H,8)*Ytq(4:^4F+TE7K)/hFQ#tI+2(#/8h(Y(<50TdqH24NK<A/+)%,0&s20;m>R&8A?p%mxhaIPs0v-`ri]7#lg#,xW'q%NIjQ&L^Tr."
    "Vcx[79Dm-$#qW[,>$07/R`P:v0J###<+I/12`/;658s20X9w0#Yjj1#7WB[#f#P407*2c#DmdF#co/r07a/f3xe8]XxL_c;[Pi05EV18.P&2@GV7l.#%8eXHCfG>#'9)h(Vd&WAZ]Y$I"
    "M['c3#C.J3oise)BmhZ[aA)3$D@ufC=sK.;NZVX.:re@-:ScC5hedS.s_&U/L,2N9Rq[f16kXuc[8SV-8=f6#AE#i7XRRc;Dbe@#B<s`+WZT+,IXv>#9lR/(r6>G2w/xu#%n2D+iH-.)"
    "a653'3vqV$X(]fLQdQ@3a0Yh(gmuC3&otgh1P<Hpm0t-$G00dcZ9bi0d4Hf*qq`<-(dd.%/oJd)WIo$$2+jvJDwdC+2$?a=oD.G,G,o-)wuQT2X9^@#djPT)-g*m&Q@<Q&r3kY7PXm_%"
    "/_gsL4gZM;:oIW-J,>>#eN_/#5*q]7]_;?#fXvu#k3;k'Z*jZ#'tIa+Q3R<hb[L<hHsPA#1?;l'LQtE<+U&,2S3kP&Y#>ZGuu*87msj>7=B#t@1YP4(#5>9fc'(jhbqnkL_MlqcgRWL2"
    "L#G:.NCI8%nhcD4ew8add`sY/kmqc)9uLj#m'Xonx>u6&28MF,J_4=.bX6g)E6bU.Q5.8@;EMT/ZCl9:lKCk3b=i.*VRvZ&N>s&+qCZY#8jGY>uK>)*jB*20/L4-/j[Rc;+=a=l0kYs-"
    "1;RHM#$q*^1L7G;Y6)iLYxqh7[=D$#%O[s&Dd3<--;^;-l+m<-+c><-,f><-ndW7%%',F%Sonn&xLGV7.wK&,I%29.8Aun(<2jo][A9p&eFsV7+@8H;58jV7WNa#5J3,c,,+4=(V@6T."
    "Bg0J)Q@4n0B/9KMWiXJM>*g1$I0i>74+VBkc#QS#YZ)22&gJ?uStw2A*^Cx$Fao0#pL+k0b6$`4m%'f)]W^:%3OX/1Zk_a4?D,c4$=_C4cRRB&L&r9/bLr2)c4&>86IhF*HYR@'d2>v0"
    "dxB0M>0KrmWPH%7oL3X.RDbv%h[FgLNO=j1Xf%@'/'qG/roOjLH:?>#Ki.T%7ZF.#.%3X_g7Z=laBF$#7&VK*rcf>,Lp'Z->NhG)+SUV$o^<#-8F.%#xKhJ)<L*T%iZNn8o&,]#qBj`%"
    "g[KG-o=SL(G<cJ(:%2v#cF=gLp&(T9j9iI3Tm8B3=WO=7NjFHpA=Y@5,AS.216'N(IMYh#c#sHM=HF;.*]Et.U',T%P:k=.hCo$$T_C+*050J3o#$E3N.OF3>M%%Kx0fXCQYUZ-OSRF3"
    "3VKmKA]2T/N3MRN[ur->#@T/;jGYV7Nda%Kuk'4;-BZ)#pon4;fx`<%CV9Q(4xhV$CEG##G<Y/(:lYY#:@QDci:BS#XIm?3FsxpcD#0J3o-*bRMda*K4c>g`$%#mA5ATw9xOW+Vcq35J"
    "F'`l8/Whd/,5*u$gdQgL)XsY7Ae7G;'rN%b.Gn'&b/4N9`_.]67ns>7`4a5Cuanl/DC*Q9+(_-#.g$I2w>$I2'&R79rA&W&rsg:/L[VV$>uv?,rb[x#'-1uccCf#ei:Da/M<U;ppDM6f"
    ":Rot@cc45(QwXL2%]rB#$VI`,hN8f$Kun$$05Hf*`,0KC,Imv$$`^F*QXp^$2MHs-jc[8@F0f#g'TZR=R(Wp/OSQs.4H3m1Bk-0)%Hjv//RJ&,sZE-*/IN1-/9S1CCP0*-G7?r%`vRk("
    "A+,)4i*0q%bl.1(r4p^+n(7W.p/m017q<4)Xkxj'u@Tb*La3A/iJiL1F69h*5X5_'1M,C+nD;k'0Jp'+Q%ZY#=kPA+m$XcVEj?D*>At5(qR1<-[-wY#]V%h(i<@,VpExM7.otgha:01f"
    "eZot-;iAg%^IIFOa.13,W<W`+RQ/>M.1I20-gt>-V5(?-JPZF%c5>##1?AX#3RB:#rJ6(#V=WX_H3]=l0WRc;SJ2F%KXRc;i77Q/ocI3'u2GA-.ZsG<kV>1M^Ba$#*q3L#Q'$`4j/UTA"
    "KZ:$$)WwkKu4[S%H'TP945bB+X*T:'X$[>#C@@w#?V%X1`okE.[c;Q/<,V#AuZC3p2';Au%taY$(^9D5wq#@5>_ln0.bo0#Y8Q<-0G8,&8)TF49)iI;p`Gb>=+F+30;/T%_.,Q'm?`[,"
    ")mxQ(L5il0xIG$,Jqn9.ji3**3QFY-m,vR&lH7<7t<dE=Ca0<7t6Q*=3IUr,jBN`++/>JMx:^'5l*Rd)Nk6c*;BuX$jTNv#T)6s.NcY]6NKg],knA[6OZ5#-J*)Q*B*Hn0>v>_/P*Z31"
    "W9K%#9JFI#$gPj#Rm:$#PGs_7PJ=T'S>n,MYo#/_$)i;-k*Tv$>s,F%+88T0,nLC$<F&q(71o60Pa/++&Ks1+alE0V7k?ML>1D$#iqb@#Y;j5Ksf*87q*dt@lA35(*+8/(`mj;c,Bm5D"
    ";o0B#P]rB#:T2K1iL0+*3,m]#YhRs$E3)?BtikeDJKMj9RufB+RpY7&CU1j(f-Qm'UmpR9ALVE4%gMS&]CX+3Ae2R81WS>#'o+j'c'&@#PR#H3vrd2'1]<50Lo(Z#iV6;%3>)Q&X%R/3"
    "lB=]bMj]V$4&%8@-MV;$F3TQ&Bf^^%[#G^&a:#w.Ocp>#A8VBo6')0(1+K6&CXws$8MlY#Cqj5&m%+87;^9D54Olw$2X6C#e(?6gX@[4AG$6mi,#Oh#pi0#3/'_:%8sus.FUmk_a[0B7"
    "@YK=8Q87gNnjVaXA7?gf++C'#Q2h'#fahR#TZvc%iE%<&Qc5v#H6SYG?>/87Mf;/(Li#'3&FZ)4F>mA##ED+iiDCHj4*>>#Zw6S#<T/a#9.`$#kCgP8lI0j(eJ+W6pgdl8r,@*'MYZW8"
    "r:w2vrpBp&bOVb5iXRc;l[p6*TP:I$&IpC#c7i%#85Id+VMwgLV3.<*e`(W-pPWq)'hoJN_q(T77Aun(+C(hLUZYN'li/iLm&n^AwIBT7G_bdtq,42&g72E<oPM^4-'ZT8WHQ?$nT2/)"
    "Hh4/(:1]kL4Fg-)/2HpKBZt?#d'N@#hb/@6`J*=%8B:v$H=Y#%FH._8d-CBulKGQ7@c9D5*s<3p=Cd;6/<@WGs#fO(M.A1Af?]#AaZZ=psDE12)o'.3$lJ7&O-*kL'LZs0skY)4Z<Fr$"
    ")/B+*Tu29/PK-<g_q^2Bqd,g)>3+Q8tj.fjGr^/)s'-q&;n68'xhTj(4xkM(RFa]b9ATD4u#`T$YWhK2?^=F+Mc[*3jV;4'^B9X8M_l,2=l0f#80Ti#X6>##BU;##I@UJ#9V<8p.H50f"
    "dQSX-rqlI#%(Zr#Y=]Y#6De%XqDXl_MDW]+<5Xp'*v;9)7;B$,&pe**[/`$#nRSCMZ]h[kODIh5B,m]#0l99//,-J*8XZ9:s^Mh)jof:-U7C+*5)C88oh3K*:1hF*vxPf_Y@e@-XBr/)"
    "9dRm:8sr%,:nr%,ZeHA/L,>>#M.BkLKDW$#[]D>#be+T%V'u2vQsT5'8cuxFeIdZ#w;Fi&:]5V'te8e$I?/#G4b+/(d[O,MF#29/@>s*G_7HZ#7du>>->&0LG-#n&w%)$#h6X?#T0Rs&"
    "2N5>>mEjT`6%]#Ag`u23)1R/AfJ5-vKp#BM8-o23QDW*>O[E]--rKb*$C=6/$PNo@K[8wLpHGqCEcI?UGJ:wL>2FBGa@SfLf8wx+cQ+E,jF;8%87cGMW3o8%a@IS[KTSZ#l<@],9:YN+"
    "b+E>0Nt+@5xB<D5QIM.;hq2w$=Cn8%m_G)4w@P5(tD^@#K8#2(.6thuCPg*+$DpB+N68I*kX<'67e(*4t(]P'UG%R97S=2(cA;4'0OwY6d?r;%e4L*Rb..]k&U&W7Htb_-gRRc;hqN%b"
    "qi68D9a[eM?Z`v%.1C#$ga2p/76jGMChYhL8+#i]XH=OM9'MHM6%#i]:X3N0'k>x%5)57(`e_-M^EW$#X5S5'A-KK1/L7G;a-N$#'c%?#0fp?95*;HM@W*&+i6]s$xav--^rnm0B-A0#"
    "Xxp##lOg#,g?)L)'GaT)#+sJ::E?4(34xMf'_wpc'3tD2J;uaas9v_$<s_kr$>rB#A&$0)J29f3$LTZ$*mT49VeO1:wt*H;iVp.*Z+L)+m0[g)L,$o/]maR8brh_,r*D@,U9@^5p(dN)"
    ",/iB#oOwT.Tcc$,8asT'3E^r.'itM(<0n(+6d)4'<JZ9%h/KlS9i@*&QSL;$Ot?0#]ClKY%QWL24k_a4t/v]1n#m+>._Xw0BtX%v7%cQ#haR%#^>`6&&-BT[UVJB+L1wL*`qN%b2Igm&"
    "`1?c*WEFt$F49Q&V.i>7w,c8.Kk8B3];.T%G#4I)SwC.3*S+F3d@h8.SH3-*1tE:.^14-))cc?,*XH8AV3jh)PtHK#se98.$(/w#Au'fu,Rgq%*9JE+wRTsL`:MhLVsg$dU',,)v(s80"
    "6+uq&xG%-'w)Bm&`Y0j(8.7%)NFZ()JIi4):`###];)4'lNa`%6,X0PkZ)22*0)3hCUIHpov]b4k-cW%E##:.2f<<%PJVY;1V;Z@nnq?/.+/.*4*s9@(A2L3OiG#u+_=m#u?3`<p.IcV"
    "(2vx=9>L8%4V###H9N*RTg_u>HRff(TG[`*+:G/(d,:u$VvUg(T;ppc$AL`.'SWL2GsJm%2r.[#rg`a4`^D.30cWT/O50J3]t./1?qeX-a]v<.i=b2(46cX$9B.`$w^NQ/'lti(x'Ba#"
    "cMmn&gxDW%//^6E-.XM'KpWKuaBN$##+$Q'nhRc;==Kh(5+oN'$9loIGQ(O'EJ,v#>RA2'OeI<$-*)tq#vkl(7Y7B3$05_cm6)>30=>n0),B+4f_kGM7T0N(Z,X=C;*N`+LLAf;+X>'="
    ">Y,R&gIdg)b0c+#q?5F#YxA(K9>X2C2T@0:%c7p&kKs;-1Y#<-0Y#<-/Y#<-.Y#<-.%Vp.3iM$#dDQ9`[lKC+H+F?#rfP;H0W1v'O<p$/xi8+i1>B_%Ke=n0'D:b*@V[N0iREi1GsTW7"
    "am3fM4J5iC`f)T/`YQs.UqV98]P@i1S]2q8p<up803nh2-tHL2jk?t6hfVY$DLes$<(rv#1%wo%+G(v#CeW9%Iw86&2vLZ>ITJr0P$$@5A>9F*]F79c&JET/%a5J*;Q1a3TZs;AJschu"
    "'hV#AB>aq/noepfIR,W-o+:kFEF>ipN=nl&Vfsx+mM<ig8dMr7B1M)+797Q/4NE=&qn*i]@72da2^Rc;d-8+%9d,F%ou@k'ObIb%uk>E'a@p*NTY>W-BU#=(nmHS73O7G;KaO>-x;UJ)"
    "Hatu-/PQJ$g5G>#2pkY-.Ylf(I,rW-'G]5'iS71(2`>v,wwP?,+7'>(cS6;%2(R8%on?$#D702#mWO=7FiQ?ce6=M7(Jp0#Or+@5=fLkLkM6/(p0&3A6k%hhwwt6pmn]ZNfc-W-2hY[R"
    "4*[g)E#[eOcfxF46Rj-$c]c)47i(?#cH3T@8J:8.k4/j.+'VQ*euiFV$0]:7g`'/)_Da'+OWCau4,VV6om5W/D_kK;@76o'@EOOS>`PYu@mdK)imN4S>,#v#%)###/R[YGi9el/j>F]F"
    "CbDT77<e[%'c%T%.(G31%4Y31:SnK)#:(k1o*W%#:wb?QO]?#Q[GC_,KE^J1KE0j0*IV8.<=mJ(%7FOT%x*m&=5_IPt?j>770be$wI-Eb$lMHpNJ/o*7M/-2JLEt.aHZd3OM>c4QA^+4"
    "s=ip.qYVO'P7X[p+;<'op>&q#;DfL(w02[,PtP[5i%Q/*u3NZuH_WU-+vBt5_sLm&/t-0)WdY3'6,<'4N+]Q06rX.)_d-38q?#sIve#?$Y?70u[HpoI_+I+4FXgr6K`xD>(4W$#ED2K*"
    "_+'.Mg<]C67,I>[?hvw5KLJd+)=0N'eT#r%Lv?H)89X>7R';Z%TUe2LRfoY,R0/w#h&rA$`t6s&(-9`5f<mq&==bu1.4>n*UT<8pmWO=7slopc]ilM9X+:B#45>D(LYWb$s&AJ3dXP4("
    "DSgs$LY<n08SB+*R]]c&[G$'Z%((f)rL4gLRNUk$^*hs/RdN78q=CW-2:QK)K,^M)vlik'qC/t%HYQ`,PGjK(ZlwM1H<R4(dpS^6sa%e*4#@E3LEhCCv9Ec`G<#3'5g:6&E[nW$FbS30"
    "v];S&B?&],OeNT%Cdg@%a8)99`D$_#jU[u>UY/MgvGu(3vIe6#5E#i7F6]0:XaGBGlJfa-.plV-?sr1+S[eF3@0]0:H^^f1-e;0(ivqh('-1uctIbA#)ZQp.V8<h2/w8njls$d)la(n/"
    "$[qA$?gfF4/thX%*oQ>#mW>w-c8HR&nx>7(iO#]mNrQN:mM[h(,k*#-gT'3*k5Sb4ZLJU/M*n,*r]5U&)J+.)n^^Q&W1Q:v,>###Q,3s6p0MS.F8P>%WleIqms,,)44Puu._:B+EE^a<"
    "?DmF<@^-xQVAU.vBfm9/+9kZ#-NRIMqbE7S-Y5.#I/uZ-Q_eA0x`@=LM9e9DSij+Mi1ro.lQqV.OoK21@h+MaT_vY#@`'kL'>S8%I^K8%e]HX.bOS(#d>b_2O_R%#v>$(#>fR&5Htb_-"
    "gg;e$i74w;c2>d3_:9u?nL(&lO'-ERIBE39A6Z;%8J+q]1L7G;&#jIq2^Rc;:%mcabNI&eRC_kL4GQ`,UT#7&4Fb<%Mf4v,:pJ*+YM)P0'V@v$G$.s$Xv^_,G/mp%SGa&+F[*9%T.i>7"
    "2[)221Y7B3Z-Bp._j)22_DLe$L5h;-J/9;-7o/K1Enn8%0wW:.BIr8.l-tTBh>6N'9b,l1'+CW&'u<q(1ltu/1^Q4,O&C;0ut(@,sG^<7Q-hI;vFSj0%ZVk'vc%L(Z(dS+2q<hurJs.Q"
    "-i8KM7wm(+PAwa3[33^67*=v6=+=%*dn,f;Fd]2)-Dd.UHW[wu7uup%8;ln(gRRc;UqN%bFBD$#]NGb%Q>U3M.[c,MA&lJ*,V:v#ZB,/(];IH)ZaLk'QBT6&QN#n&Z,[`*ZHKU%al@h("
    "AvO1:?lQS%?qI0:WpCK(::'6&)>eS%P;sm$fwD8p6KW6pf%rP/%J3,2-V[TcTJ@#>>RWt(-m1Q/RIr8.hMD]-%VJT%s*ct$dod$v4fuN+ro1C,Y%rF?KpZd(,^v&QK#n3P>[rKOLYUe#"
    "`)?l=?pV]+Pb(Jh*$:A42ro51;QmN'7$fX7J9_[.9)t.3<1J-+x<p6&`F2L3&aHj:+hKb,a6s0**QFsQhk.M7X-6Ohf%(0o=%#wjilFHpNXvghW`/q$v>qh:?e*^,sh1`(W*wdbb9fS%"
    "^9^k*q8:u-hY2P:=.js%KkTeV@S:hj3:qB+%)>>#d,>>#kRi;-;$5L'AOcIq3ec3')-mF#Acf#I(;-U8Y-H0)3.GuupFIN'U2u?-6P-U8&5-=114hv%l%r<.xd''#CRPsft*7r[4*u2v"
    "v_nN'F#f>/8,dF<uwYQ89-3t_We$b[FR1s6%dJ214LP`t6aav$V`TDN$kdEO-3>cMk*hHNu<1hMR5.U7]iIfLot40M(5a$9-r9^#]=:$^I*2c#*bJN-Ib)367wbf(LpEciV0GG)@3<J:"
    "4AR[-QicV-%iv:Q(f-p0ju(q8HR`?#(q.e*Ym,n&^Hk=$S:$7(;dM_$ZAY58LJ$C#>TT0A?S-TcU;U;phxv5/O<x9.s=$Y@q9HZ$NIHg)dRn>'=%?ci6%kE6.9[)4>k^B-&)xv.PdHO;"
    "[_Z5/90_6&F1OW%r&?A4cC>@.:+@8%3dvs$xKc5*E#N+<0U-$vQ;q'#CA*5$wd''#VcPW-kBw,XVXH>#eE#N'BQo>,=Md5/3?AX#l<)mL5S)u7Th?_#jHhv%?7-(+X73u-'$#h<;vk>-"
    "h6uY-m`$QUPIQ>#2#50M]4*nNHKQ>#v#Y?-P6-D-obr+8p1h#$'u4d3ML($$bA*c#`4Rt1?a/f3aD.;6--#AbQDUP&Q3g6#B>pc$gHu2vmOej'-;ZT%%(?v$4siS][6]nLRsn0^=_7G;"
    "/rj4q?=a$#/%&?#=q8q%@Iwo%BhAQ&7u_v#ebXgL_5U6&e>9cH%7R]6T=1F*6a9D5R0GD#.S:ocRjJg21/Ys0mh[5/Yc*E*OsxG;QwCTJWn.w9<533MXAhB#0S-?KTU2$99gU9KAvV]+"
    ",eDciw5-,)j<el/l,h6#U[Fl7<wRc;bwmER:B+$Mx/QU95R7G;<9A?&xj_v%V`LN0fY@L(4Z+Y-W=e8%CGCv-OUET%-a0Z-E`C$-N`gN(DG>,.5XmG*YuCi:B%DZ#(MMj:if*87$Ek>7"
    "m,>0.=wM#@EQE/28,2T.;.Th(r'JY-u:pkV%wji0w)fF43V;E%;<Tv--A>W?t$^:/A)WF3hK]P/2307/#F/[#UHgh4)*D%.xZPI5-$;%.Juo&$@nv[,0WL:%6-,;)D'eg)U]SLV19fq/"
    "Q<7XV6Koq/sXmQ+BZxbi5Z2/<SSMB#2NGb%P)>>#?\?AX#4b8a#ou$;8.E5=nms,,)3.Guu($7?,Q&Zb<@S2G<:V6vPV>:2#/7Kk$s(P$,avX?-@5L987itA#TheA0#d@=LP,CT.C)&#,"
    "NT.>/K:8H<39Jo8?uR5'VgJVne.c)Z_AT_/gTbt$:bQ)*)VHd3[g5A4g2BZ5j[F.#Je[%#<QXH*DGZ>#Zv0:%T3Y8&c)b=&]+Oi(:0Mv>nKJ_FWs?-)T<'6&5ll##BR-@5jGF[?<O;s@"
    ">HI+>5m9eXB)0i)DGM.;cc4K1nVN'JSH>?'0hWQ/-=IA6/7RhC';`eG@3mG#ZDx:dDTEI3C>ou,]hJloMs'[T8e6Q1F>%n&jtPJMA>FJM@8=JMY8FV7;SUO)gRRc;-wJf$A11H2ZqN%b"
    "OadN'7^h`=Ve=a=F%_u&c6%<&<_Bh(]3>*=[sr#?1^Mw$^O)s'h:EDuOVKf$qjo.A#kFHpv%F0hXm]+%Hm+1#qsUO'm?`[,Vg'u$,-19./]NT/Bh^>>3N0N(Y#jXIC#a2K?Fub*A5d8/"
    "]34=A[Ll**B0n_+d_xM1,ug+*Wc29//&bk:O5+q0/v6m0baoA,1@?c*2%xf;JxCgCp-dc*3%Lb*#;f21]voA,WUJ_uQTJC2;n*d*,uQ:vESA(s>]BP]:R_l8*`$s$V:P<#)9@W&4_YL2"
    "OQ2c#PF1E>V>cZ-`RX_/d-612ucbJ2kgqfDOT*W%1BG?-9ZuG-h$cZ-l_Z_/[E0S<?J$`/qvA_/]&t5(cpTd22.wA/k)MJ<Pl(:8N=.OXh?0u1+j$qLwQRU7<xH8%ZjI>/jGQ*3s0L1;"
    "=pcG*X$sVn=b=A4$skI<`xrv;FRRc;T=a=lA,Sc;B2LJ1<VxT]<NE=&'Yd^7lXRc;oo1I$+*9I$(:&j:?Wkf:t06e<AgD(=cW_:/,sj`-@qOD#7u220q`Ek'*u]?$Zapf+x66&%Y<KC#"
    "a8QP&Xlv2A(xD8pukm;-=@#h1%L@9cjawpc:B35(RZrt@>kwAGj1x*3WtTg%nT?\?]1YR_#J)=s?_bT/)tN$h%t[39/+0Zj027Ti)?\?s@,9qf[#_WqT%XTeO(`cuu-/^Le;?g7P'@<9R/"
    "DuAc3>OTU%@$W`+^[Vv#L(MTh9U3Yl:k`%,`ri]7kHZjB[bZ),/F)`+>8ho8:qmD*/kbT%C2;Zu(&NA+5M8p&Ow])C=[?T.P_%/1MF,Mg=b=A4,:gg%TJe`*2G#i7<*S0:l$gM1;u<_="
    "H.p<6F-S0:0_lI)0IFm/xVO2,AD%w>s(+87g2p>7NXj+&r%DUc=L?t@-x]b4d#<R'MFRW--s;fOhvQl1^d]jMmKfW$OnxQ/U7RR9/f@^3*)xv(]h>r^8FYI):,3j;BAxu,Atl1gVNE5/"
    "RR'0<'X&L_#soXuhAiT/'-2c#l*9jiYP([-P[w,=3#-o/kjF?-)TcZ-=_X_/_hA<R?tRqNh.uW%U*e^5ul&$$US*c#eG790GJ=jLDv<BMPPtl&Qe()k?`cd$)]gv%YI)$#@03**:0`]0"
    "OPjBN[%Po7037IMoO*7SssGtp<>vlpnVH:R[3J;Nu3ItLX?Eb$/C)9':b=h>w7C<-=tQH)=8NT%F6aFrl^<J_%dU`3:@fQ#k*/%#((u2v]V-:&cN5+3Gj08.b,D8.r[,)48L>'4#@U#?"
    "tH&p(ueSM+FKt:.4Fi[,_[l]%WT<8p29fk(/Y+F3:3^khN&DQ7hDg*%P5>D(,;RTuu`ln0$2M^6']R_#YwM'%g1NT/a'@lLEPp.*6pGx#7/7T%0XLD3J29f3GwVI-.XYe<j_ML3(=01;"
    "RX-O(SU'U@Zga<-?`oP1[9xp*W`R60oRbi((]dY#>P9C8E<J42Wb?o1MH(#..8E+3jk@L*S&ud;EOf/;1qoU%*jH40Z_$3<LwJF,fiDn#Af''#v,>>#Iv[N2RL-##aI78%l%if:U%=B,"
    "FW*Gi-]C_/Y85QL?r+o7$PEs%.J`M'AlHvPLTAos;-Fs-BJOJM$Ki:9uVO&#0mx/MLSh5N@KQ>#H>ij:_</FIfM77W91es-R[$lL'%hM'YG`M'7@_#5Nj9'#M]Ep#4f1$#Uqn%#?1%C+"
    "pnRc;+`^JCiP`da6jRc;`qN%bd2Hb%O5so&<Yx%#K,i%,?o,D+<tg#(D;j>LMZ]Z#?//87C1Z1%$1lZcx7'hh-r.G9S18o8TNkM(sq)N&Vn6g)[*?T0ZIoQ0c7QHNdc?_uZUMkub0Qh."
    "a]?W.3*OxFcl?<.].>^c66>##rR^U#IJ8=#aKb&#Hsm8^6w*$#MnY6Wvb`$#<8?>#gnd^,llpmfpt9-M)pZ3'>V`6&1@A5)])=r7Vi*3h*fueh%oSL2+H9o85@d;%jYP`%_N$`4n.B+*"
    "j_^F*qGUv-EO]:/E5WF3fo#GiV*W13ffh7/hqmC+Ww)P2M)(<7*_o[uuj#&O]Ndh2MJGd29If>8AmZE3V<QA>*;J#8FA2C&J5T4(luSe&07@_+D]6*<_>ru5&*8M^VNE5/jpMa>ZXHuu"
    "*:#**9bN`<uH5'$dLsQ0R[+##*0O?#&*.IMndjn8aJ_`/9*Os-&;?:R?tRqNj7lw$MJ(v#2(En<txkv,:_$-)kjfs-(BmY8P6dV@WqZ[K<H,<.%l;i-4^S0WHB(&vW>Bq#@6i$#A>w<]"
    "AI`._Q*v&(=E$K1;$S0:cSh:%&qjN&u3l3+[Srq%OQwv,Pom9%9c($#ABar7-2N;72#Dm/wA:B3h9R*1+9361,2VT/N:B_4-/'J3Qg?a3?;Rv$hLB+*k_+D#0R7R9.8TUJ.-]P'EN&n*"
    "CE_g17:J#8iZl2$aeZ7$:M3m*7+es-S1OcMjmjk#Nog<hF@o/1<T%8'vv7##Mn.##=BpS%2xd;-&<Cd%6wV1adeS@>;a+O2oF_X:>jCEO3b+/(#DeS%S#DM'Oe@)4vJ*B#&)7G;(=$_#"
    "hx6?$2aK8%ACc)'ev=nL[G40M)6%E35jQb<&=0T%U([Y#-KYu>6=HDEY$&/1sV?Z.=J&T&H+Z##U&mN'*v$i1MY$s.h=i$%Gg6-)G9^M'TEbT%*OR=7BR-@5D$DQ7t2lZc267F*-2VUc"
    "Auj$>)KK/)18m8.@M9F*maT6/>;gF45KC=>(@O2C_a7oAah4u%ggpQ&aG.l'2)C(,^03D+Pq1oACWO]u/+N<'$G8H)cK06&sS%Z5rgRa+82`U%f%P21na1H;l&-;#Ec/*#PwQu%tCRE5"
    "O$=69Gt$29^<dJ2+u@b<7OMn85fR1*^?@)*TZ8s&V&'#,2Qea3d)E%?H',j':i>8p<b3>7eY.@5mJ1W$oEKH(WI5D(2[P4(IwXL2W@h8.R0bu*+nbt-SUx)=sc5g)(QEU-`dD($_YKi^"
    "n>2B,)`b2($:/c([mdi;2)B^+n2;0([%?3^Q./7&LAiEn>Z#]##DUtoZhOf2+@cOovZi%,IR.[#Q4k`*(Kn&4>s`]+C<.Jh7BA5/I@$##fEVuPdr&32-^cc2/AK>,mu(N93pN31B'2Mg"
    "VNE5/]ew>[rPp%#)9@W&2vk>-7WmF#f_i#QUY5.#Rw5`-pvU6=a#9hM8pmW.V%-j1q6qA#,1Q%.*&CKMIFK88K'VPgIVIRW4_]K4ML;g2@&*RWYa/SW-2NjNEY<A-=hpC-Y,)iNE@FPA"
    "OUV,3=4s3=*PmD4'.P7_#22'#,Y(?#[04x<%N?_#>rfv%@r+M(d3GS/0?7K13L/2'wL6s$<WTP&[+TY,sEr2)I5Ku&gURA#GWX>-UqZY#AaX0_Wanv)dR7F%f`.r%Y,kw,j/;8&Wh[%#"
    "J=N:%>om9%(fe)M66N;7^#7/(Fa26p=<eah.FvSu'x]b4^^D.3c>29/,Z`,)^*(5'jM_:%'/:f-xvp6&/7))+Yw5s0.FI1<?R'nJksHUp$],a*q'4U./2]6vX*AF%XMY##$%###A$=xu"
    "*kS(M,K;-*1%GS7UA81(EH+Z#q_r4)is]@#L'uf(VxfY,V.:#?oq82'QiOA%0hos@j$KQ7/_31MKSA9c$G*'iQPwh2Gn.i)/1Ne$twRp.9,B+4x?qw.(Yki%:vfo4d-SQ2e7+B@TQd0L"
    "cS'ipWU_'+x06K)3I),D%j;l&Cgf+H]l@K:pK9[uB,4c#w//p>fQ9j(Wdc<%iZgNb`wn8.q4im&.0C-;kfJGN[Or29cqK#$e_X4'#Ij;-$'%;9'Ad0)3.GuusX*0(B3Ta<o)g;-qDG`%"
    "uK91>c1M3(sclY#3U88%_+.q$R`x0>N0>1>.*RLY)i$Q'>N6D-']g4MI-Q`-W6G1>^k?>#dC[u>m4GDEIf%8@ue52(g$N?#?@(T75H;mA6oUv#fORc;xn]&$SJMs%>LJB-)ae[-=f2g$"
    "`t3?#?7rv#x.*p%1bCQrXWO;p9d;/(xC:B34Hg.AI;8phO&DUcwJWgtZ:;')Zr)8I.DfU#jxN.)`s#n/$O@DE*VG'[9G1E'_A5/*DAou,ouTp@.'-;#5&*)#)Icf1LO0-&HnX_-2G#i7"
    "?bf%#j/a%.xs`D+EpWA+7':'#[gRc=^4pi'7C'f+(^Ji+PRmr']@#12QLoC##]]R&NMQA#TP?9%9HT2#nQLDuU,6BdAtN;paI2[g`%#wjv/QL/9wXL2jnV69<D^;.N9a^%0`gb*Z_7<-"
    "'u%Z45=S_##F/[#<dr`'xkOR0AVf;6WKOd*Y27U%pcJ^+k/;0(GpkH;)kV)+-8W4'%kJp%XxaF3Du5#@]LF(=O$E1D_LYC+cTPN'P2$5J1%GM(1Rq/)]9/<-%cFW%_lRZ.8]]n7HR`$#"
    "*$J>#.Vbs)Iv#=1U9@W&#8IQ0'-2c#xHo=fx316$M:_Q0+i?8%eLDJ1L$0Q/b3Rg)Y[GA+&E^s-<,+p:K?$j:JB@v$FnY&#]@`3=YoIq/M<2c#Ntf4=L]jNXq>nA#+1Q%.3uHLM?pT,N"
    "CkpC-SOOKM2;]V%a?rE4AD4p/O%-j1*Q_PBE8]8/S<qC-9](Y8u.+Q'^?#5=p1Klfw@24=ou;]b-(35&b,fJ1>tl[.-rQ:ve(hK-0NB_-*kf?pwElN1L(HN1A1SucdO?_/O]d'&K9,>>"
    "3k93(0_N`<`h0-MdKZY#QGf--u@a3$vL7%#@`]Q_2,=%tCs<I)$:A'=&@ZC+R,v'=GqN%b7Xgm&KYu(+IvW+NQe<t$G]2Q&YI%ecph[Ph%(</(9h7%b555D(IwXL2Qd-Y8'W::%Zk_a4"
    "mn&O0'7bC=w)00M*t2?/8nVp)D'uL<8%%AkWfuC5L,<q26&53)H=TiBQ[*Z#vPHC#*)uj)qF0j(-5;o(>oUK*s@kM(xQYJ(:klj'masZ#Qgsl&IJ(O):941:W>w-)@[*p%gR=gL'Wj>7"
    "[0&CuL&HHp60,t@>_7UumW<.3-k/g1*72Z3E#jc)J%1N(#AlJMbSA+4)w@+4wBxT%;^e^1tLs^J$nu;LY;C::pJs`uG/((CPT`au,qu,#=e[%#5?Cn#L)+&#'Z-T7)wh_$bGcj$-B5F%"
    "StRc;fI&W7vcGb%BEb$#k%#r)Gjfq)Ym0i:8+d,*JZ_fL3xZ&67S,<-L(qH33EqGV$C)1)5k:,)_k<9%5-LGVWas]$Nw%<$91D?#81@s$b?0u$'vWR'Cq*t$W(f+M'wN;poZeGG1+Z7c"
    "xwt6pqT&3($^J,c5_e;-LG58.XvmH3BBxG3ej&ktlr)c<20`f<83dvK;gkm'#nVN(?p4Q'aQUG)krIF<[7#T^m7Gk%La)phBegg%hg;A+0q/a<>PM(=?jZJVj_`[eXS)>p@pp8.(Qjs-"
    "flqJ:vsJ;g8j<bNhN-e%S=jl&gK]x@r[_@IX+gnNWPcOOS8<C=Wgw0>T?51>/1MnUO=-##&1o0>08kLYuK91>NUMK=MCeKY6>j8&7WiFGEnS(#Zl)%v<e$o#Z@O&#IT.,)LO0-&6;ln("
    "2G#i7:.ST%t-o%#$HMk'kZ][#=MrQ&,Oqn''g>r'e4Jp&FoiU%EGg6(XFCW-Ml/87fPGDuBd8,2iRM0A1=c4A?j%hh'SWL2Uexj0kax9.]BF:.J#G:.Mqp)3_0gE3f1%[-DMORNfP*7N"
    "`@]O91&[3UrgRa+IGphC;[KqC^$MZnQIZY#.*oW_-m/;Zbg9D3/xe6#vU]e]Zc013RpV8/8Y4K1I`jF3X/m@-,`F*=uDhx=eJN.3+K0t-C.@uQ%CT$$/?###%7OHp5WXDcu>A$^u&LH("
    "T_-9(='rs*(9baNbICu$aH+T.mOo$$E@^7h'=-c*@eis6L$O.)RB9U%w03Q/M-Ja+=>iL1m32hLvK:N(DcBb*209<-SA>-&'UPj'BYaF37cx(4]a##DQ#-U/7Yj?8&&nY67lw/10$###"
    "ds,U7=OJM'vHj3&X),##>KSX#$4k9#+g;P_L(I%bm@f$n<mR.M1L6;-gTwu#A86'=Sa#r%L0.-)?&U8%/v`Of$5$ktx;LH(kE2e$$>_AOIYEigE1pC'-PTb*O1hgLA8n&//n3s$;Y;tU"
    "fWZ`*NJYxk-d=kFvGK%#2&3-29sp[#6XQE$63u2vMg#M(HLG&+67Lga>Zd`*X^L0(-;rv#.25##4OGDu6N2hPu%bOf=L?t@I]gG.V$nO(@f/('LN:a#`d+f*^Aje+/M%<.p9rn'-;bE*"
    "f<ng#:*nPbH,#1)i<sW'*BlO0>G24#_r2%v=LUn#^n(A0Q3=&#<=WX_S4Dhbw$g+Mo`TP'@>n&>`xxM-sNmH-kv=87<)eM1U]=K&oTE5&dhN1)9Zn<&QEc@7UrQ]#L^]>#Rp$,MLd7iL"
    "#aAS[w&(118iK;0PR4x>KIR8%4T>>#R-I>3-WLDuKx`OfXGg.A+=]4Au)GaMbNvghEoj#AHaln0a*2N(bD_7L/0ot-c0D`$3vYKlrZ'u$jbg%5Fxqs(nCF/2nLbJ2N0sh)jleL()5)3="
    "[A^[5R'<(/l+*9/K2gbiDuBb$qr(B#01C;A]Jt(Ni_Q:vA;aFrxH1s6#-BJ1noHi$Sv5Q/C-_U%1inf(1m6N0V6VG#tubm#*ECpL'rZ##IZ###*0O?#%3G?-rv'w-RInqLF;_0MAO=]b"
    "vE/2'3.GuuM%1B,k.9r0q@lG-+ebv-Fdn#MCV<FPl'='M;S]1(@XP&#Bc)d%h_e[#Fme%#Hs2_7UWYK+$w<V7qjb01tqN%b8nYN'3+96&sLc;-b?<Z#w[0+Q>HB40M4[<.>rqc)aK3L#"
    "ZxLZ#9.RS%g(]^$vpek(JK8)c69R*1(1>.3<N<I3;G;0A<Qwq$?0'()3*J5/mZU&$(oRs$$`^F*je,?,G`;[G6f@&,%Q(C&h(D(,&l[w-BolY#?Blj'JDG3)Mam`*I`AU%&4Qh<nEsC?"
    "],Hf_'IrW-opaH*B$N52SRv##dIa)#cKvV#E_rY4YC91(l4W(=#^mR&WLcC+[&)K(gek,MSlP/EgnF5p^edx2`E?h%'+Y=?vkDB#O.P:vKb7wJR7LkLWed###guY<hE$C#4Sfv%2;,##"
    "(sH8%a.],2>KSX#7IN)#MXI%#Q5P;73S.<-b_R1*`l`4'0u@?#2ZF?#reTF*gUr<)A2h8%'-1uc;g+o$o8U;p+wjGc;)Ym2uPNj(xb2Q/U6ZD-&;tY/8[egbiAl[#P[(KCPs3qCLXww-"
    "S_Z`E(JE5MVUZu>_`FDE$7g+M7>Eb$hQc6#v060^o7>']&x$lLHggW$E8Gb.6d#9B*]i'#:?cQ#6ZQ7&C.Ve$rI9^#,DB$.&*I?M3B#OM]?^Z.Oq5r#sG#w.Cd_7#D]*@9qbgI)dgQ&#"
    "WGR^$rYF.#?4i$#wYM#>GqN%b8p@0:F)@L(gLsV7)7fT%)Z.1(q5iH2-TKw#I2D;?Qj1k'5&k9'XT<8p2[)22NotghEPtZcSx(t-0Fwe*=oxj0Ff*F3wZPrduh^G3,C6_#q9VK#iXlw/"
    "x+cjLf/k=G5<*xtw9UW.Sg1#59i]6NqP>&#7]/(#d22a<7.Rs$5Via<_FhW-8/^U7-F7G;('ic)K3.)*vA,%0b6FM*a<cx>&smi$3(KwK3f0)khf1hP1wB['iDKiPd@9u?7/1^#H)_K-"
    ":sqxnn,kxF@4%pJ:?:T%L;XHu29%K)',Guuq.>>#^[F.#_ORZ7Zk=/(8hxi'w^l^$I*'U%Jn/m&1Muu#ea3L#ZuUZ#$(2/:J9&>GfNUA5)M/T%QuED#V=Z?3prmZcJ041)LgfX-ik_a4"
    "_-h_4]tHtuxS;nB8xtB#S?='nwj^C#ItQx?QxQp.F+^]+Yg4eH[j,/(B/8>,M;Ox%Pswb'h/^M'p/QR&Cn]2'R<Mk'SZW78R_x4p^5,W-wD61GZQm#%@=().4oH1M*qId)oZW58S&ra4"
    "+DUKuph6@/T9llr<Op.,qS;$DWK@i3@8%K)_`/j%]d5'eHgro*SeIW-q34'el8]-Dbs^#];>LW$b*Km/O`aI)&$mF#nrvI%#gZu>XMFDEwfAv$k%ip.H*Li(ObQTK_D)R#/2xJMVu<]b"
    "NY?.%RoQQ,+Xp_-ZPrxTQT0i:cDX2($:B_-%u).H07%##$l^WMBF&L_$,P:vC;gb**PI)+&4n;-`e0h2Lp=6#S3=&#g#kK(-]`$#_H^e$g'u2vGETh(JT6m']ZWp'b_v?#Tw5hLmW.Zn"
    "cTk$#w'D[(]SYJ(p$J0:5.tw$iH@%%qi2jKmhZv#vUPB%x'pE%[=Yv,ZPuD<@DCW$vgn=-^jH_'BX*p%%72N9GVc=7D98)c6;eahM%Q;p.kxghGge(1N*MT.0=>n0C*9S%H=3#$:$nO("
    "J'Jw#irxF4/]NT/$cG[,pFJw>dn*#-SZDcuW>cj'HAG3)j^g`3$)&h4]bCDEcqAZAfG+IuArGH/fC*t.f(rm9ragU%`L[7n9-^p7S,um#WYCo(,Y(?#cmm(#2EaP]=J2G<O),##VHrG#"
    "W[F.#>SJQ_YfY=l+PM$#@4D97,.vm8@I$m8MqN%bH4vms-]1%,VlGn(A_:%G0lDQh<kFDum2lZc#$FH(%gKpL[k)9/E3fi&mX5jBIGPH3mexC#5Xk/4MWfQ%G4mX&)sw-).U`[,KJ>du"
    "[`qO&C'FD?G:?_%0wtWUJr8S@kBWxkvYq%4kwj%=%`9b*?sT&=xWGp3;h70:(_;`+K(Jw#CuE9%R.i>7PJRuLO6h.A)Q#'3wh=&[*I1N(B@M&O),#D#ghCR'?bZd-@kUl:S`T*>%M/1("
    "wVY*tNFnY#'[:r@kSoDt,wpS%upJ&?3Yjfiu?2=0/?-f?7l0^#5lLH'mdf;-,0Z`%qA>W-^TJT(ps^Z$Q$94=;0,6/xFNj1_S`ODHvxc3MB)4=r0=4=YQ@gLvkv##?:qk$chJa+Hja(j"
    "<f]g1XDOS.p>DD*##vY#hP=jL`p@)*Z:9)*b3Hh%jC<U2.6TsN'2:_.]lFkLYngM'WDpP'5r)E4deEon]9A)4,Fd;%8$`[%<rjOod#vs.A?2c#s<ea5%5voL5f=p#*JvD-Gp1x&-2bI)"
    "<HmF#A-DM%+)[u>-L[YGOSf0>$R]p.`Pe-^L1a3=wg3qic8Bb.Gq<]kx(5<%hEg.LT;@#%*JMQ,;9^OMF(<I.tbJ.qDN^5/A=ok7T4LKMNr#tRE?FPA]f_8.^Fgb*,VI)+7BI+<@_#qr"
    ">cc+>52Md2GRBd<A4m-N$O>_-CdWuP1b+/($P<5&2uat.v(_G3oM=oCtCOOUOH40M$-RA4)(bs-L:dl/Gf3Se:`BM9[Qg6#NvE.^7U7G;M=a=l$8D$#g-=mL-^(3_p:Y#M<d7iLAm@W&"
    "L[#w-&O>4:QLM0MF_0T&B3Bn9+(LT7Ri?]#Bl^F4$k-m:br@.2wX2DF5skAu'T-@5(U&I;'CGdjX;R*1$`fVk^lUs-dKH/C>XYD4v6+T.2r-x6DZ^@opB]s$NZ#O*d?J8%cwdN)9x6I4"
    "u,NY,cgeE$'%:h1H[IW$k3f9(+1`@,&@mc*9.k:.6Z?T%++#H2m&-;#M2Ue7=UpP']fBL3i.%Q/,=a=l4dRc;:%mcaciEX(]kp`=`#6W8Z:YZ8CZ+F5YQv2v8+MQ&C9gN:Yr&'+YjaJ2"
    "E[d&4fV*&+#H?%-gUF%$2b,D#5F'(#2hw=-Eb2V7a=kx>q-[p.hRC+*n;cT%_.i>7Z8fk(pGPh#rCa>fn99I$?mtgh`x#wjP;R*1^0A1A7PWL2&ao0#^(@>-^<A_%lcMD340_:%`GUv-"
    "=HCn&?r5q7qdH?$p*H.),8(e*0XB;.u`Em1]4j30X,*a*f23b*hT2oA^OUp7Tmj)+(),##>6VG#m[F.#)It[7^B@W&8voq'$+7A,:@o@#]u;%lK[jx#Wh&1$g5_-2Y7UW-]cFZ,SAP1:"
    "k<ad$xW8B3<a9D5iRM0APBJpL4k81A()Ym2-hh6*J0x9.gH`hLQbv>#<(iL0C9>j'.(^XWW*j:ixv>r%L4FqCR:FgLGL=jL@oGLieqW`3Fhf6#UE#i7eRRc;@(Ae2&bom2a<2d2hi)B#"
    "Qd5B#5Fg)=wagF,k8K>,&th0(+T:,4`3oi3lW(B>9=;<-iQT:%H@V$#Q-p>7/ZrkLkk-Wc]%nrekKCNhKD9Ccv''9(KDR;=>-2oAQfm;%;@%SjWx$<$LblM(dA:'%YvEV7;mf/0IOiv#"
    "-P,03xC?0+#d$n&%)>>#=,>>#Bb8a#X]>lL1udUpYrVv>Ept?0d;A(s`w$2^Fu1#?ddJAcYq,O`BWRH-R#db<t<X7q6B%r#H)V$#4BrQ&$5;$#X6^e$`6u2vZL;3'KdqfL[N>Z.KKXs6"
    ";GcY#1PuY#7U,n&RaYN'5X@0:C@ns$(v:v#5MYY#0brD5Pn8B3.'dC#AK&Scrg.4(OGw-3AQAoc2P(kLdM,;H'K#<.9EsI31eZC#J)_ZK>SBQWV*?w]se(-CTFgER=HlpA<(EfU&gHiL"
    "VivC+dIKiL@)MZ)dFBiL-dx>M5`GW-[P<X()lsS&/AdZ&*X1_&AHu2v>'f7&#pL?#X_vu#8LW5&,ODmAOfcv#`74GM$4+Q'NZ85/#BMD-5DlR(j9:V(:s%)$<,T:'b_x=':5=A%,aZHd"
    "Rq+Cug5h?3>M?4:?q:pfxLU^30D>8.-0as0e1Is-6O<iMetPD%FpjOBln;HE@3pRc$6ha*d@+O0PK+oCI<9uCOBJc*U_x^FD,.m0]`-<-X,sU%:[RE+7KmF#WW7n;s6QwLqDE,#2.tm/"
    "b8::%(m#p%2?)p&/l@4=v$as-Z]1`FYFn#6&*8M^1xD4=Zf(3(sf(v#Bj^K'R5h;-%9wR:/q'B#CoBKM8kG%PoL4VNPK40MYaID*7VZL-WpbZ-Z]r1+g*&c5f6*20d'YlA1axmK.fN]F"
    "33FF+=@kI+AEWn8Xdtm)La2M2XsW/2r>2W%%7u2vIBb4'cXfA4S-:d/Z9TR3-C`G):bA2'wYu##$Qq>72+At@Notgho8C0AUUM0A2x%t@%vXL27]R_#7],2t&YFI)p98W-5aPKbYcxn$"
    "x4sUIE.bj1sv97/N.65jM=Vg*C$Lm/=o^A?&D_fuo_o>0#Udeu$b2&7Y5l@3O?p7/AEJE+W]0i)1Q#K)pUf9CPiQ.832_nB/:jl&4:G>#W1d##]a/f3rn7g:*]_&5+i?8%JA[g1i^w&,"
    "T&Zb<DfMG<:vKi:27n#6Atl1gbpB1>$Mr%#5&t5(ZI#g)8Z7>/@DdF<`rhwG[t0H2L6VG#_[F.#fjxW7#Q@90u9Bm&#`nd+AP0#,Q[Vk)#mT2#$^EQ7Jmtgh*(#wj>[Ps-VQ`#?=(jgX"
    "(6ko(-V0DN'UQG*xGqp.3EVJ<M70rI&4X^,$=[x,lx7tBC.+f2EiB&?4kx:(EFU-#7uclLi(:R^x`OP/e'Rj)(4+50tN*20mZMG)66JnAdLiw#XqN%b2Igm&j3SE+afEE*;8`v#IpCL."
    "Lrw2A^t[&RonC5%(-+iCj0/h=ouo4][n=J2;8/P'%Fgl:NR$--Z-QoJw0_f)`0g91S^7#6RB[x%1Yo]$M&4JMx^Q:v:i###CW2s60MNY5.Eh6/L]WS&u_w,;a]rS&&6trn/o$@0)@f'&"
    "k=Wm/&Sn60T)nF#TXS^GvDpQ_giw%+bcTgL/(Ml'OR@W-&D*1+#wA0'Sf*RW,A;9.Wkes$TZO/)^3s;-(4S>-vWZL-p3LOGaZkA#X2G?-i9xu-H1BkL:jpC-_n)e$O9@W&R1GC++(3j("
    "pGGGWODs3=c$;x7i(ugM_txfNiN6E&F3IIM#blG%$:hA#&uY$KNnc&#2W;uLJ'ls#AB%%#cW$+&;Bu2v9+2q%FF^d)(3,4#Tue+M6_YgL+/EG)KA.w>j`*87b%7/(7.i#Ai0l(1kQEI3"
    "CLoJ:;:=Q',^/_OtM/b',.pW5f5nn&[kx/(+NO^,3`P:v#f5gLqd6lL18w>#pl*##@[D>#CP065J[^)=/W:E45nIf_u`d@>ocxb.no,360ACgM[*m1%wCv`4i:^Xf?Xu.^C^G#>C.$I4"
    "EQ+?-LNZp/Q(35&xIlS/K;d9%c:$RE:`_M#bB%%#G^0%#CH`k'TnMZ#Oli?>T9gm&82M$#gZ>M7>3lZcIA0%En`'2WTEZ)4G3<2,+HMP'`]cf*d7XT%X9r-#+R+Q#L$ga#=*V$#GBto%"
    "?4=-V):BS#V`bx2'TNI)Q:iA&P[eY#]>;4'Hlr?#=&jd&38[i1I=hb*m9UT&c'mRA<6g<7__7p&)7`.__85H+=T8]$I[Rc;41R8%7%r;$5u6W$uwqY#/AAF-Xxq_%:/sZc0>O22e?-@5"
    "Z9bi08T[0)k/5J*%nZ[?>sUi#b<D99fU5_A7S;/18@>DNH+kf%bl'pN,DSX-peYI#%(Zr#]fw?094:)*]*LYYTlSY,dTxvP2EAvPQ?Ft$W@dZ#->K#kM2<l%'t&hh0241)UD.&4c_C+*"
    "KG>c48ZVO'u[=)4iGAd#6Or[,iniR/.ZE.)Ore;%L^g7/OeAu.IU7N3Gpn@#`VD4#M]:k^&e4m&Xl1mS(1_0>?2S8/i)Ei%4BUP0EQk-$+fo`*YGQ8/Y>6s.bH7VQ*;fiBMMmi'jNai0"
    "514T%+I5+#HVlkKve//L<ji+Vjia.);7b1#?vU0AZW)L>o/wKYc-ei9h;lA#06wb)>#/W9W#K6aWs_u>8Y22'O2@D*'ufi'P3+t$spnrQ6Bk>7_dLMhK2+j&eIYH=Z(jr?[n=J2]1%KD"
    "`LAQ/Hddp.a.20*@jAgLoixS80B8M^2_%v#P:-5/,Y@s$5k=/(1(eS%)x@L$w4@GVhA&UD5-p>7VZmX0HTcTc7OA8%qAshLZ$eY#WcZMt`O]g#dQ^hM:HBLMGqjt-Hg0#=eF$C#`*hv%"
    "L`WS&T=fj0k6a29I@GT8s&(,)8u^[0x[Jd<;Sh[0kIV39,3.m0#I+59`d%T&N#D2'FiK21;t*<-Z3eY8b]'[6vjd^#^SeF#]Rd3))@aM-of;^%kqNH*wiP>#eWwlLW%?/(YVP2(.xbTB"
    "T#ZjD0FqB#Ubgv%iWbt$j`P29Br]QC/gPT%84hv%%4E$#Dvgo.EWK_H:T>-j>,#v#1Q@W&KV^>#NKc>#-;G##^#7/(VWEHp1oH>##6X(j+x'I#AK+IurZ<X%kGC8%/&moIEj?D*#Qu(3"
    "_CcGMU65+'tT(lK-w*1#JIuj+Y_jp%-_<-$QT<8p'7[58lG=Au.G_0>q)fF4m'YUIVf;r7xnuS/AFp0%`]<lCMc0k;Mf9k;@,f],<3f],pJ2O's/t-$d'nk?lU=5MfxO_5K,>C5a:c**"
    "$*k-$qT[Y#5V;2'lZWYG@6GJ(oX'<&'8rm&,)Uw#89u2vRmks%'_(($.c_;$wA6K(K9u/(GQUg(JCi;$M&7L(4(%W$hd[KuR:Y@5dTqx4)g6/(Hkw_hOrF12]`&i2*0qB#mjic)K]+wR"
    "tQVH*j,$E3v5L60=f]g;;Lsq.FiUYG,9hk;<j(O3Gpd@-@$&>GSJZlJ%is&,^Wg6#o)9(/6URc;/CZ0PU)p2)WS*hLK$=H&VMwgLvhCd4)#%@[Zx'<-HG`s-?0<I->LGS7D;SY,dEOt$"
    "&<cB&g]0F&[N<-$W5pi'@sffL/*RG*BAQ=$-<9B3SW8w^O3u2AUP1dcX[tihD@Q6ai=%T.W.>n0xSloI'_u)44EOv6Wetae(&)u$WH]@#UW*D-J)oh(C$LP:xx-W$2U=<Om>ZY8r@)V8"
    ":a[@%']&p&48bi2Vgnv,k7<)#``qpLR;sf#Ka($#O4,a7U+Mg*`gmS7=_7G;-:@m/6#jIq3aRc;7eZk%S:GI)`+X1LqnQZ#anNM'^<-nA8wNw#TAOM,T9K>,.@V=-Xg_V$jJ2$#3MYS."
    "JF>3+X_*$N[U(%Mp$43.^6JpL%MZ=pQNWL2KX#-04aQv$@-D.3%/'K/RR(f)a<<AFmQ9T7$oRs$(-.xmQh7W$GVBF*e%,5(3MZ59d.VG$mSMU%Ik9/%MQ?M($lD^6dYZw-3JDqB]CI;-"
    "KSY##;]bI#krE$%H#Ws/xpw##W'9I$2f,g)C^*<-B$gG27U7G;9J>##5J###YZxuGH.i*%GS/kXaUbah@&7T%%V9C4>'[)4k'Jw#cxD*3SWqf(<**d*;AR<?&v,5;h6V*>Q5>##AZ(?#"
    ")Zq/#jpB'#fMr[_h?VB%3/r3_9TFq&XRa3_ZK;B(rEmjLMm`v%DXfb,.00I$2gVS]h6Sf$+V>jL)UD<-Ek7G;SvmgL:+8S'k(`_,M<o%#lu=T'oGXS7L,gq$#nZ*7lP$R&4F2`+wMZA4"
    "7@pU/l2.1(-S:v#X^B@#%m*87P[)228>Y@5g50I$Jmo0#.vU#AXRm1M&T@a*Rtr58v`Wj1p:gF4n1'^$(Pr_,Tg'u$^)U:%$2JvRV6Ss$ZJMT%tBxF4@p)0<^FtS&5u^+*UC<x-UFK5'"
    ";niD+4g%&,6%#m'j2NF*,@V@,dZ/>Gk^a[,GslS/.'Ja+51PM(/1l2(1PfH)7G'b*x_>(+1;:xPhVvR&`T'6&;/5##_)`?#1Zq/#N?O&#*Q##,W^[Z)K7ae+5b,<-DxCT.4dRc;?V>W-"
    ".>Ib%>1dG*iGjT7:a&u&VK9I$cZ/%#iqv8'G[x'-YrTwKuk(d*&<]&,7FVt-vpa]+E&'3Kn;4L#BlE2KrCg5U&p@N8ZXI&cp$]#Ag>bR%^uDj0NP,G4.m@d)1$=u-cGeT%JbDE4G$Ts$"
    "Vlja*&H+J*%%VW-8^M0(4VvP9cOeH$n;2U%t_Gn$KB-M(PDTQ(BLhF*+5w'=9l;P<if_>$))>>#1q4Y##w9f#Eok.#D&V*M5Wf0##o%P#=Tc##pA)=-H5)=-nX=Z-I@oX(&b82Lhs_]P"
    "8V6Z?MAv1KS?mA#bex>-U;)=-LA)=-`vQx-,-IqLRFq4NBLoqLWmq@-NA)=-SLx>-74OJ-'R0,.<A]VMM<:8Mm]-(#Zfv$vsN+m#OAO&#rJuX7eY=X&C-a<-_7]_#p4[A#wbYV-M:g3K"
    "Cg<3L#8+87rsc/1x8#7pDVvghIwXL27,Yo$>O6a*n3X6/ik_a41--fE2)b)<_&Co/w_'02WH]->i]68/VTnU70XpY.;_VYZWj:m&p.0b*&XZV#7e?o1ap%F4+p,_9.V-/*B'MF*q0OA-"
    "][UkL,9tq'svg6&$`(kLg12h+HdEciv.9;-a$u1BO,Fxu;+^s&s;hq/p6X[7rAZE3#2,,2f3b`FqluxPgJou,W39;2'a(C#i27+3Zk=/2fB/62qYR92F2D@2E,BE+3*d##><ppcimtgh"
    "2RWkhvhc,>5p''#]q-b4hTxjk8`B+*KG>c4JO&^&7/;h)wX<9/?;Rv$-?]'/I5$]-T68G$39]o83UFe+JE9K4XjU>.XkZ42Xp_>./@f49*4xd+B?LG)N;BAFLJS>,S/C;.C_'P:LI.I4"
    "9hBl:+[8*%.NO>-c-wDE.Juu#tD<%t9q[YG'^u(3jX?f_?w_vu]4l%#5Lu2v%;o%,SAw,M<^.iL20.ENHf=.#R^Fl78=Sc;NDpF4NXtL)KD>(5RO5&5V7g%5MqbY1eTtu1KD.r%cWRP/"
    "qi@&4pt>12*'w-3Y^LO15OTJ1L]xS%&7)S&+Aka*)&es%1/=b*,cH3#VIGQ7?`9D5,AS.2FJEQ7F-IWSxX#K3qm,,2rZ=D(p0&3A0LD0hYAr`*/=<W-'gx0##2.f;SvZ;%(8XZ%^4[u'"
    "RHXQ/LhU=6-[AC6m3)H*hZ$)4W$R0)UT/A,PCbOo$0]:7g`'/)<[AxbbfjS9TSWa*+CsW-lM-f;.h0^#)FRXJdG6$$r><%t@ZpoI:_$29>c]1ggbm$vaqjg(lNA@-Ae7G;TqN%b@2Sc;"
    "e0sAHPmNr7*CPb%F'Ee-?']1MPoIq9SP[A:gHZ6:b:MU'5Iv`+5=Nh3I0s80PwgQ0wQ8c$;NF/:KGZE53(P&$$B40Mg1to&Om@<0b9nD0Z,O20S=5b%Q6jr7/8Dd*>BIv7bM16p.K46p"
    "Vu5A2h$Kv7;gBCc>')@5MOtZcf(;Hp]cvXNd>;@KGf:m8*,sAH2g%d)sZv)4lWmF#-#;B#4#ko75-Z98vK220t5Qn&-7vc*`xUW-cpl>R6l&E#wtLm&/t-0)WaY3'75WB4M+]Q06rX.)"
    "x.U4S@>YV$uMW@t0U[YGckho.;1KV6m1g4]eUZ$vTYx$#5m(wMMu'kLBwe8Bpp3T@+>-&#sv0K*t9kw7:5R>[NNSU7&h#W8v>tv0Kd#%G@6W;QY]2mJoIA<(Spp;8JW>;-1+t78`tZs&"
    "[7pA$KwM`+7i5v,I>JE-VIQ0-[:d?06rc3LY4-5LZ*FG%;ceGt2lWj$m.&3A6k%hhOqD'MwJwhgoIfTemi#<-jdWr$;U]q.$rh8.HA:pp1m_`3g-[29N+s206`^F*1A^+4nGUv-aI:8."
    "9*uf(j]n-);rOI)4pMO']RQ>#de3u7eX&auB4i*O6r,(+JaxO9VheS%VK0q%[#?xba]N89j,L6&+lP_+XdCK(&fc$,6s0_,4w^M'vN=c%j1/n&SpPR&da/*%f.SfLZ^FT%mtm+8)aZp."
    "wcxX#?[q/#*]&*#vBV,#fKeH^ld9%#,1u2vhcLU)/C,+#x02O+l<rb*BgtY7K9_[.Cko%#r,_e$,SId++4j>A?BbJ@Kq'^@'p+pJ9uJN4>P`S%(.2-#pnRc;sD]5#?E3<:SO4u$ksYuJ"
    "[Dq#ATIAWAD#dv&@<xD$:f%`,t@AsAU$d&$OasML)%tQ3>@$[98#m6L7T-#8C=:Bu/cKv@D1Q]6@4Jv7Wh*^c,(-nf]Z`=-0G5s-]TGh:Tb45p7p-`=;skA#F#9;-D:dU*EYSm_f]$r+"
    "$pWe)BFlJa]tek&8o,c*@ub,k,7B9&HYQ`,iBnc*gC[p.m)uI)aVQ5hn.fd)h19t+QdNq*1O8X-H=tDui?pUdptOV%BWW@t9q[YG#2ju5h:M]=eaqr6J`VY7N$8G;^d9%#8G<C&*&>T'"
    "F;h1_uPs#)8v0W-S5RdFG^O`<I.TgLWdu2_]8KT+qBv/MC>FJM6%#i]Dou8.$5f%,bOr;-<0XV$P+A&,`R&a+cRl%l4R<W-9hOh<R7=qCMw7G;(Eu(?;.LP)%EC`?6p^k,BFgY,AJwCa"
    "L5QlJ2.up);^M0<tMXX?:*glL;q>D#1h/)?4qF?>mEuY5jWD)3V.N`+WakM0M<)B#%FuJ.03Bw:72Au8pB6'=jGM?#Zu0l%kcvM7uIGQ7,D5C5uY)6/I)kGc*V2J8SJb2(/x-<8V1_KM"
    "t35GM,).(-ax1W-rt-B?&]R_#GQ0s$t*&88TMtoToHSX-a.t+3*65O0`EOt$.%l.).PuY#+cXH#_Y4Q1hf$s$ZH+Y-[N#R&1n1k'Fnsp%+NO#-A.b`u(O/d+.rM^6-Dv&#AVs)#*s7k7"
    "q=Yp'$NlA-,D#W-J$0I$/fv##d,w2vc3xK(+F%/_K89Z@/I5+#XQ9I$+=A%#aTF>#5T;99m5RN)2T`p9Q/;T%h3HoJ6$AB/GdIe)-=A*5kiR&Q7HCY[NtXd5A?fi5A>M99fA]Q8P`dG2"
    "b[+Z[E8^5Ur1XA$gqrq1VFV%,3_7h,EHR2Leclt7o0'02j.>8p,A't/`fU#Aw.bKhhudq&_=DW-DT5mhTGFd*fEP)uwlbv6a&[)4'(Jw#t*;8.*=-db6(D*8^k$_us](k's160bs8$7&"
    "D8(U8)VM50?WZh1Vv470*^]/3r#3e3748]=>AW`#hd$GrL+75&(N#,2.vh6#YU]e]QoQL*1E3gLoJ/,lEd=`+vsWK*ibSJ1`Nk<[h]211,C>70J;`Z'3Uln0j6-B#xU`@#TFwf$jr1,%"
    "1I=@$AoXq&K-I2Lx;ck0--cQ+nP;$#Eh<9%ZKbG2>F_S[xx:/UvwUf:q.Y5pEZ*@53&9WJqf5HpnG;0ASfGH(`GfGpr&v>Im;'>GcT[0)e.2A&v/@X/e:YP()J:8.&T&U'utE#HoHSX-"
    "`+k+3*9Gk0_?Ft$-rOi(-JlY#i7<B7K-)I#,gHn9^64m^?^3H*:PZX-Nk2E5Y,:'#hd$Grj]8;-N3)58WiA<3swRc;]Bm=YOts$#xbL/)(r(W7[tb_-SgJ,3X*SY-fnPn*f%i,+KiHL*"
    "8gG,*3*fp&PW]e]S_#l$Y0l34bXu%l]OOKM1B]x,cLd--mH?d=Q'8G;)t#<8gp6N)&tGs8?K=D50Ivi((jwv7Dg,T8(E]q*bmA]&P+iU8b$es&^=#B$'fF]$nS$x-,;9pAquZ#$K6pT%"
    "(%BPTsK'3'fP'##WY:mL=O#@5vMfx8:sYcaa+<j0=`ZBbbqn;cYZ)22#j(>-#**k+Ma:X8-Z8(8O;jo]?mL7/hEhCCL#g+8IMj(t>guu-tb#q7Gj0W/cV9G;1*l6#BvE.^V38G;F=a=l"
    "11f%#xY%Y.V@VN9_=>F%?:$QUo^NT+CkQa$pPC&#u'3I-B2HF%?3W8&gj?F%Hn5F%aC35M>huM;[Cb?[p(*.;R._0<KDvv0m.lj+il6,<1W1nLD/QS7O<DD3O]ag;vajt&r$)C$3W)q/"
    "d+-[9P,xT2_S#U;,.TK2'W#W[b<qMUX06L%26o&<WrD&d^V3Z%4-BB6)08D([Q<Hpbqn;cRfh<-F#9;-#6dccpne'&WuXs?H*#d39@/6/[W'B#6_id*S;v0:)duoJ6o@V.FEhCCvcEJk"
    "0Dj+&J@V8pgww+%68BP8@P4K1a:F&#-Q?(#uCfa^hbGI)XUj3_4($6(9a&u&sNS%#'B1K*BZst&rwE.^?>Zb%Ko3m72(Sc;Jv&f3Rk0M)L,'J3;S5c4UcvA#'9<.38E&b4M)$`FrY`^4"
    "@?UA4G7oa33_;s-.JXK2KJ7:/aSMD/$^R5/QqeD*<-BP8&7k-XP`<3pqgwGGXYO=71nvC#%$F0hSwKH(NA8WbRS_.A<:7D(7=ab&Hm&kL*/`+,)'sFl(#;B#65KP8eB$,a>71@%&K2S&"
    "U;/[8rLUp7YUuG7u-Pn0KY2K*ANbLu&[F.#)2PuuRw>r#R;r$#ij9'#5J$f7IPp2)WWmt/NkRc;NqN%b)hE`+q>Q31F6R7AH;8N0]A)GVMZ]Z#^HuN1ngJ^,s`k,QX&>[#:.x?$;]=q&"
    ">3#s.b5P^,CPO/MXMjo&?x6kMB*12Uk5[-)iAT*%b+/JLjgQ1L$)+87Ps'*)@F@WG*C[s0b(=d27h(E#S5U;px_;Hp2_%hh'5/C&N@',&E#Dt.mGvA4%*LQ//,-J*YBo8%#>3a<ET;ii"
    "R)2U%G0;i2Q`U[6Z?^Z&W)3&+MLVZuumEZ%,uAxb0WFJaCeI[#J[SQ0mS8G=i8;0$dfDk'c[<]bA-9t-n79p7xa@J3N3)58[-ds&uqle)90xX7Sb+(-qn*i]J+Vp.+i0#-WT#=(fn(gL"
    "+K?l7B*Sc;c^_`3h]Fv,Eau2vgSgqL=YiX.*E^[#%O0Z7k,IN)'[0?7uF96*Rn)sJ#-[s&93;.&mH*p%uIHK37tTS8PM5((0d<t&kg>n0mvTOLP-I2LT5cb+]9YG2JTk=Q@pC%lQbtLU"
    "ep+D%.GAdFUf<3puF]=psOi/o@Da,FsiGHp>^TBk_ONFp`;1'M=Qs^%V%]BG`tE0MI-PT)`HmT0K+lo7Qpp/)IFH>#wL)X-2%W9gL1uo71g6#69OWX-LW.ha)rVu7^f0^#i,]aH3&$X$"
    ":v8>,T[m;A3?X$#(U[s&nfAU7A8YR((-(<-0Qt$#XYK/)SYb`+$=W5,7[r%,4iF2(;9S],>dvg(@TKs$CQC>$sV`o&(ukKL=Q(n&48###:o:v#=v0<-#^Yl&pa,-tnJ5Hp04-nfCBcd&"
    "IvA0Mq>,l&Zdt/:q^:&5jOD<-/Gm*(TxsJ2wgRu6)<nS@?d6(#osqFrWeNM'*Z>G2aT+QCjh'S-CUN/1(?X$#s3[s&c#3T7>YY8.9*.%%,=Hv$L+ST%Fb:O1aWDL2q)@L2:w+T%&#mb+"
    "M%8X-Xo,wP6hPL2;Dvg1(u^>,J7?j'E%@nA*-5$l9iI0ULM6jTo++87:HKv@d;,<->Y/7.%=bQQb4OZ%6OhtqVBg>-np1Q&L:#O;w$deu4,VV6Mr_W-c;1[7awOw$d,7A4*^)SeA/mX7"
    "^lKL3GURc;X2q%#&?:I$9Dm6q0-r)N,XPgLSH/WApU`b5%Kh=[9O?@5`=%A6[WC5:r]`#5]f$%-g;AGVH2i%lV-^>$B<fx,`rPG4E$l$6*`h`5Oh0$5Ku[A4c_e92SC>b%(/Oa*PcZX."
    "3lrq%Ah4Y$:]EQ7G^)*)N>>p79Is;7'_wpcC1(gLjW$12E:?3pFT_v@B:Oj$>WjOBlU0U':4158D5t)u:X]v7a)^G3E7/Z-3O)KWZIH##.[ZC#711/#Lqn%#hQk&#EOpV7sO6*5PJrGF"
    "pH9=8n:9?-.3I@%WN9I$SqRc;,fPW-JFe'&ik`v-9<lJ.Lpp;.>D[Z$EcQs.`2>@#mV,@#4HX>-DN9r.E'',2WIQp.%pRp&w2h>$@Ug6*1XXQ)7<1Z)I`T3,q=pM'-f.h$[A%tD.PFKj"
    "1i>0hiabk%@$kGccqDgaLAPx)d'0U.nW'B#+XL*'rPsj`Gw#l$X:M<-If6^(PF?##),>>#711/#s>$(#YN,i7T]Xp'h6s?-3a&u&#Iti]D]>W-/EP3iJ)av%Y9_[.rpsZ#Od(VdMw,F%"
    "-<TT+e0SnLwO;7ql#1%#ZBRL2j/]I3$aWI3.%KQ0?b3-$2sse3+Z?p/mE.L,Dnb@$I1(r&=ckq&:m8NL3HWML*fF0L7IFG2DX$T[)>rJUhS'##2aKv@:u8i:+R&<7Jmtgh97X$%U&7Eh"
    "X2Ph'cQYBGT1i;-)U&d+'E?aP]K;Y9fNTcjxUDX-UBB'-'hC4f;m83Bxl]A$,>###AjPPJX:lr-,gYc2:ZCZ.N^0%#46Xp'E79?-B=je+WN9I$SqRc;@fPW-_Fe'&?wI/_sXf/<P*8G;"
    "6qq02H=XL)8'rk1k@BG3*&$$#0l3x-jsj+3po?-(A9vh2=P;-2(oB#,uoCk+W;I9.grJm&gE]X$_4#r&laqmADE=w#dIgo7TYTj(d,EQ7(I&T.Jmtgh*RG.+hV#<-T-q1(A=3u7G?QEP"
    "#&;B#Vm4%3_L*Q/Qiqw6ieKTI*_aW-jrO9K8YWY5K-)I#8`/P:k4sG<8:%(8?kkA#q7,)%rXZ$@3u5*;>c'W/%59fhXcB#vVf4%#.7u2v_l_^+;q__$K,Sc;?'v6**&g32)TS#M@&]iL"
    "OM9Q(BfnA,ZPq<1Q8$&#P.n>#Ed4?-#V>W-P#0L,tv^60N;*a0b.oQ0ox$@-.L$w'7nX?Q9p0xPLii9/(2f/13D=q&/^Q?$+;+.)tS@)*93qj*]Xw`*M9#Z,cVa5&lvKR&lS6NTJbFa<"
    "=X1O)]QNFpx_;Hph:7D(#At&<Bws9'T'>n0AS>c4N9Q)4#t,<%cP?j:&u'B#%bG^Gev`1)IFH>#fMxS'.<#L4^j0$-Xj%eu+R7b%1wCL(ZDvN'+B9W$D95p%]5sU0Sau406Sob#nqPh%"
    "d7Lv,h(KB+OlUpBb$@uue88t#Y;r$#$,3)#ksci72XL/)o6d/M)'T1_->fv%^>on1swRc;qD[#%:sPF%9pPF%Y*u2v&/c%#g=m]%x-eA62w/j6EMv]6GlQj).L$w'l>^Y,eVb22o;IoJ"
    "q(1?#e:tpJLm'fDQ#On0g5h>,S$koASbt1U+4QA%I]^&?KG<3pWtt,2''LH(^=PFp4/,D(r=T.Akx6h:xg&E>$rVu73foB-P)LX'Ad=Z@sUh=:wZ;8_@oQ>'20LjDbCr2:ZIK]$q5w_s"
    "8n[YGSc8>,'K#,26I.]k;#&88WMP8/JFn;-KY#<-OLRm/DO=cV=)Sc;mdpD&k&wgLJR(T7<wh_$^.cp7`uw,XKQqnJ=/['4wYl>-pN_k1oxL(,Bpl29-e2&-qpKs.Z/QG.s2M8.5l@h,"
    "l;9e$_ui.L_U'Y?S.6k(d,EQ7.GnShL4;Ib==?0hbV.nfV(0wa:;]C(c<8K8o@.m0vCbA#s1?dD8xN^,Tg'u$/N2b&Z2+%5kS4p'NB;i2uJ_R/xbX6DS.S[#h);k'j*;m&ps>xbVW>&6"
    "Ckew#`w'auQbnF%&O.U'ft7f$O8Z20>>N)#UP#c_?ljj;l$8r8sKpV.A9Fs-d6V2:Vo@p]R-8G;eS]3%6AFm^S*8G;4>j'QoP$K+2J-x#5Eau&hF96*&>SA4;-$@#Mpp;8'3c_,tp80#"
    "0L_[%rXk9)[7UOLQ6eML)Xpi'5<bb+k:Ep+iS@q3L]:W-6[GKEudvGG:1b7%(%^,2h7'hhjQtZcfw^GuM(7EhZ;cW-v>=EGB0d.)$%=w7wV&L,9l0h<r/mf(;I%Q8x51^#Z>vM*:g`v8"
    "[QX2(2QaB%Mbai0KtxCWx<WV7HXfb,GURc;3WQ.%7v6EPPvFp8dG442Y<X<[nuih13w?h2+Y%w'vSge*`F.#QUeOR0RX1Z>mGpK2st3^#%qdLLPHAs77[lS/Sn,w>v'%2hm,(Q/425D("
    "$ROTu8FXwnJ%M*%o,,=-V3O(&u1w5hJ0*39'(`%ePU<))`D7P'9JN?8wqVmh$r8JL[UI<-9ANr&9/^a>)<s/#l8q'#Jc/*#rw1G#0S-58;w*W-?J2F%)P%AecgA%#oYew'HDHF%=/$Z-"
    "XZ,=()=ZF*aC#+NK.pGML.^,M3Ca0_/;?Z7+a*1<`g9I$4',&#mtj,+dA+42CCx3M:EY[7'-wf;+93g;]v(:)''2:)(-;:)#%uu51k^p.L*^%#V5ND#J2q%#f03i<uh;f=/N8f=#NB>6"
    "s77J=Qwmj+RbLnNHfd2r%-1T9U+[p7i'ic)v[4X[cqUe3uRt:(P2ks:?j^##wrVA=24ja<WMoH-Zj`1LsIQ5LV'J6LuCb^4BOp@#93qj*(->t;g)rk)LJ<3pRult&R3DTuj,9;(C>U;p"
    "P:`3(fGKH(t-oSu8/_Q8qRHh,B;V-OTok4%4_h6Av2cA#Im=m8J#C#$ZA1<-AcrU%&'[XdlfPs.Q:+5Af)cA#U4.X-/escu$)+K9wiQ_uH;6V%WTK6&_[-gu4,VV6f24X-?^YX->obN2"
    "rKp:dwtlo$+Z9%kt;6T%QHl2'EqjT%m,qD3$hDDEB/8>,5]%>cPh:;-M)5D<.fcS7*[lb#wObA#vFF&#l@2m1`/^-#MXrZ#A(v2C4,lb+69e;%.(+:%LtVD%FK%X$sZ)22Notghh7'hh"
    ":mPW-t6Y=?l[SCOlUc/CGP29/]E/[#@]WF3x1f]4GBkX*QB0R/,bktAg4,`4-G369u*)&5CV]i1?aN=&9PWo&j:fM1edUG)^Nw[%RRs6&*;GB84W[$8Nq`L3$&xv.2fG'?6QD*4&[KY."
    "ImWR'5nMx/e3e##'aQ8^eK&%#2Cu2v6r8iLm&>5&UTc/(Fen<$E_fi'8f(Z#-L^*#Ifn2.:M4mL?#/D(q0Q]6*vYV$;`SL28m]F4aoK.*U[7s$4oR:/.wDw?W*M$?0*q110>;o/;RlHN"
    "-PuJ]6Zofbunu7#LmT#v<X<p#@Nc##A'4$#](NAFnaGN%:<u2v:=.3'5p>T%dbTDNX/`20+o:+it9JC#2^FL#DHofJemCDEijdfC59?Se19?#-8%fofC8RJ*TX2K*U$]%#8i%t'SYNa*"
    ";?Cg(.A.s$@?Kt:bwD8p?aQa0ivlZcqN@HpCh[%M`qJw#,0fX-QS>c4wa)*4<rk,2D*px=f6Gn0ZTt]u-7sp11L'C#r$;D)LM`B#,6hp95fGt9<xs;8M3imCVEX#7ZWat%fw&PoqFLP/"
    "T<g6#+u)e$[YeHdXV%n&l]8U7>8YR(gc><-6c><-u6]T%=N_6&RuUw,JdN9[cDl[#'qLG)xsRt-VcPP9?%*t.dF`s-PN#3'Fdj&+vD>##+$?g1Notghp`].A$^J,clI([EDI[m0UH:a#"
    "R7m4%[#G:.Gqw3Ext0+**U=U%8P39/G.1E>k]IY8Ak>*=^/2V&R2Y>-Pshd2L*IV0LrB6;em5#-_(+=8FTZ*?;$%Y/:j(Z-pd_d<,HjoCJ0:*nSpW.h+@B>5;?_c)9bLv#)*),#=6jd%"
    "Hg5926xK6fdQSX--,VB#$Y7H#mZS:v<o###m4GDEEt2AF9j_a3;ho=c2&t(E@<cf(RZv(%[?A&&66]e%iu.]AKQ]c;uh^G3((UB#f0c31t-c31_s8u%ZbJloBppl&(As80&3F$#V$9I$"
    "SI`D+F'BpIJ*aAFA35n&OJsP&%,,##<(@s$MTc/(SNBf+JG<3p[pk8.s0/(#sN1T%6f^F*:bM8.HlE<%[so6/FtANb?nf.*,<'H47$C_#Gr-;8w$NhLGBN$#/GmV#j1ba%``NY5fm'B-"
    "#?$H2hAL$'#m&m&/)eeWCR-*XJCiL-^wjR4bviFW7i9+ip[jfL;q8+ir#@rha@9s.tP1T.H7Gm'#S+V&%5#0&%AC8.h7]b6hq>g)^^D.3tt4D#1^%P'u:be)8$pJ1O^n,*QR6c*[#lU."
    "P2.@'gjad*Omj-*'>lT'6hhhLX'R$,E*C(*#J+.)pPCa<A3x9..Rm)4AkD-*`MY)4&r6lL-Mno)mTR;7UFv##UMQ$vF-Ro#Jsn%#5Oto%[/09+1hR/_=$o,+t=T@Qd4SX%u:eS%/JSCQ"
    "3+V$#D/5##cx*,3<$J0:qq=x>RSiv>bR087lhe)MiFRm0M%Q;p`I[#A0=>n0))'J3_[C+*?jTv-d(4I);8Lv-sHZ)44?T*4LM.I#k>-V%8)$)OFa_1pDnJ>#5a5X.0im'&kg5X.8<hc)"
    "Jdg7n,4OA4Ef0DE[sJe$f_vZ^;%W#vB`Ep#H@%%#t4s#$1](?#Y5VK(V^5n&poe-)YK>N'e3bq%YCLJ#/<FE$(xJ]X@0F0A]3^kh`WO=7?Z4eaA=Y@5vI=F3Va$E38p'.$%(@lL)&ra4"
    "J2&n7s].iN=/]Q7$8FfOX1WiT#hmnC4P%?074v,#Q,_'#Koap#iAQc%RoYi9]4d;-/I7G;7@3T%]Ze@%23VG%?[*t$77nS%)J(v#pECZ%R<&&&&rr4#Bmv2Ad,%#%]uED#*SM0AJAjRc"
    "Z9bi0xJ(Y$[V2<%RlnM14L?d*-q.K1$Vj4AkQjpEGFA=TO<f_Jqv>_Jb.f6#I[Fl7$+/%#D9;s%Utqv$0JYY#l8.:MOPP,M55sV$s(@8%S2%W$3<-pfN<<^MV+ki0vlus.0Fn>#]'vB#"
    "@grE.$F+onJn,tL1u3J-'3k+.f^2BM_'$xuGXgq#VsC$#^^@c7_RV@#lRjxFK>l/_$)PW*Qh@F%0N>F%qx$?779cnCTfb;-9b-Z$IUiX-wR<X(v^ib$^.F.)R7?>#GB,##TqET%H+;;$"
    "'Db**L7m##TTq,)LO7W$',rfU_ULm(5>O225v:d*K$8G$&-wGGd`9D57S]o@(JC0Mb_fo@/6kt64kC+*MCn8%x9#;/Vo8gLWDkG4x?.#64:Cc3%0E,FMC-.um1A.2?K8I*I>@A4ER1o:"
    "8kMBB]9i6:_&x@75$qQ0K/)Q98$cf(ZiPxkgRi'&0+ai0KZa)5SvS<_gCn#>Q[C;$uIZ>#>ePA+SxwI2CAZx#$]2v-0.gw>m=OA>:knA,[dBJ+DlP;'1oC?#5d1l-)`;L#b/#sob1=/("
    "Us+Cu`Px&ipheChvY=h23JE(=CCs<%[;Uv-^fse)J%1N(.)8N0J)9f3twC.3sEl7eH+#l0cRoQ0QcJEbX;[)*S#vY>WIV+4#,9N2&]Kb*:(u&45HaE*)-xhC$*oxFprEk:#P;v#*+-=h"
    "YVu>##%###7=7wu>,M*M?B3$#]RYa7Yn_v%Ja-GM/^EX&#^T?-]]:a<03T&=w2F$#,qx>#qr$s$,URP/:hR=-iv[U7L2)(=2Xf1_=$o,+rBtE,+Ne].]&?G,aSn`*J7=9%0Q;,#Shqjt"
    ",2[KuFaxgui;X4(5>P0AL0v2A-TXm2Y+4[$E[)P(@A0+*+m>juAFDC5%6UV(TFDPt$Es]+UY;^#=`,>#ukd>#sK^(Nm?<J1%(>uu5A4I#(Cpi#g<Mt-+BnqL4a+L#F#Al#_OY##l/m<-"
    "F)m<-j@oY-F(/x'9^v.Lfm_]P4P6Z?qS$2KM-mA#G/m<-1V^C-S/m<-H/m<-9;Mt-OS/(MD4:SM@LoqLGH:@-J/m<-O4S>-')iH-K/m<-AV^C-j/m<-Rfe6/q?gM#'28>G($,)OpNQ#v"
    "cg`4#UC4gL:-$YuImIAGY[niLEbmQWo;R>Q:]RX(X.KcM]Vj(vgf,9M^O;uMHqOrLHq?TML?h4NKlrpLu?;/#B^%&%6fSGEe3.44a9NF.S[)F.7Dc-QODeX(K4=e?l=SX(M5IX(Q),##"
    "qZ^U#bn=6#dq0hLNUd>#uRTJ&,er8.S^g_,Zxj;jT+,##u7VG#GvW9#PSuN^@E10(hPT2#9d;/(-UU#3k&E=Ix]tV/vB:a#TK3*$u^]9.jX9J)$;<O'343Z#+*nd)&h:t/4je@#7tq7#"
    "spU902%wo%]pa1#ES:[97>_#3HQl8/X5Du$U/.)*oPi#73-P]#=3x7(po@q/tDB&c`E+,2$oi12F(v1^%0V:vX&DfLeD.Jq6CNP&5=]w'*h^w'p=sw'8Anw'_<.`sp^3>GL5c2(N>f--"
    "ShY(N'E>Vm>mBj(4:]w'gmsL^[k$2^O[g;.Fq]w'VLh--nB%.-%jx--[O]w'Iwtw'l0pw'BZ_w'ZSrw'm3pw'j$^w'7@tw'?vvx'7?qw',Z%5AX_eP&>7,dE<53_Abc<]XUK(XLk<AYG"
    "]`dw'2bA8%H?diBXS+#HLtG&#u]6JrK,F>Hs7llTC+>;R*7`VIjFjBYo?ED<IlL8%LV^Y#1a['85Cq.CCKS;$:sC,NJEcx'n7sw'2f>%tMc:;H@XvPTq1[(WwK=l(FKIfh=apl&_NMcD"
    "^P(W7Htb_-/$4m'.qnP'>I)B4cw[40U))O'6%&W.^fDp/pZt&#V)iv>Odc>>HuC8(&9^[#=ijJ2a;:wGti*875QJ=7J-lN@DlZC#@Ba7cDTkXcUqQ4(IwXL2&ao0#*%^:/vBo8%BcWF3"
    "NA&d)s;xF4:OjJ:5)m29kDsI3PQT@#6FeK))GG$,@$h=%fR/I*-&1I*T<Ai3c]4a4]3MN8'#2vuke8E#>EZt&@CZ#@.5/^7bLiZ#w9ri)oWu14I7p<ndPSV-5]3>5%vZv.7i3l0T8LD5"
    "P2Q5&O:=#5'bTv-6mAB+62CG+]*cf(NM<,#b]vb91*2d*M>58.7Y7B3S>YAcGcokLZs*'M09Ds-YB;hL/qXI)W8(69`a*^,fSRl$)ZVO'-%$Xhfu.&4'P'T/:Fbu$o,Md)Lv3Z6.o()+"
    "drpT.6[8A62'uq.pJF1)Un2#&rCP$,<jgZ-@mtY-cp5V%;xLZ#([`[,;m,w-Y:kS&mB^Y.hkT1(h5L88;^ID*,wH8%Lr7M1S?pW.tCH>#j05##S^oX#g[F.#Nqn%#K92/(R3;B(8<=0_"
    "rFl#M_FW_=2CIQ/:&S0:fY2O'q3#X79'S0:c@N(=[]3-M:-;hL-ubE*3;E5&D;:[#GP?9%ZvYm/mj26p#<R*1FXjfLFn=n09TgAZ$Ri8.NqUN(7(ccM$G4jLV-&t[%5/^7^LhD.ccFPA"
    ".&tY7d[`s-ac;p)@Wl1DH5`Z#HR-PA:*2S0h'ffLF2(&vILTq#<H.%#[3rS_L-b.+3<`-MQZka*[NHS76g>g)^2pE[X*]0:j/p%#reU-M<9MhL=H]t-@Exu#Fi.@>K'-#?7tf/.Z:[fL"
    "_w`Rug7)Z;#/w8B5h6g)v?/NMA,1U+5P7R9n-`]5Br%4C?iv#o/f*L)0fCw,Be=D#F;ov&Mltdu8hCE%H2Puu41#D1]p=6#QEX&#2kVN(-3ig%Kw_v%&C,+#.C`v%b.2V.FWK&=)`s8&"
    "[B2K(G[]R&5iv3#USC8p*=*hY<8F1gSlE+1vIqP8n#(B#`q8V8sXrk^'nk]u/Uv)6nPrj),Z^:+*vW;P-6pfLYg6:vK:9q#S5i$#nORZ7Bb?W&<3@;-E8W;6TLRc;KgJ?&PpK>$HS./("
    "`nsi*RV[w>:Suu#3UI8=N]Y8ftZ%hh+P/j-xQY6WJ`C<-95;q%hwsWU_P9]?xU**IpYL9]G]gf3wm9^#h4jd%?I011rkp9CQI8Yclm6I34?dQ#'),##3?AX#uZq/#(Uk6^F@OH,O)v2v"
    "?Oiq%b+ZY-l:0j(BYQx##Lp*#j:EDu?hC5/'_wpcdNI0M`F=u-iJL<-KR$3Y*ESi)?Hbo5r%(j(0ABo:jLr(,p>^=-V&J'5/k-X9R`'_+aQ6**9f_G=8j%l'Re1O'wv7xI>LEuMKX?/("
    "d2KM0*ie6#Q_x*^OD0U'.2D&0MuF1:i?nM9=ocW-aA`$'hY)X.o)k<-E695#8lmx#=]`6&Q<M3XLUEHpY'p;7sd^%M^A26p<W'^c[7JC#WN^5/SwC.3Kl@+<8]VH*JjE.3(@n5/nCj/:"
    "@O915CFn3)hcU;.fY`P0-8->@ID;69Mv$W.PEmm'#BHV%l$r<->0vW,6%I)>8Z`k'/bYO0:KOI=ve_T&(1/W$AxF/(Cc0j%b5aFr[$si0#q*20VlFQ'nsm+#Cn:#?gc[w>`<D(+5v1$#"
    "aSC5pC-T%Mp,j..d_ge*.LJ#PKw^K-;vBW$g3l,;Z+?#@?v@5'O;B*+*N28&pO,(+$^_h1Yxmw-)_sg*HYTb*0l;Z@E%A^3?Aj^#'rW8&8vk;$9:gX%Eh5H3+Ppw%Xq]N'=l')vpU.U("
    "LHW@t&7[YGg?EM0/xe6#ZGxn$q^u2vgF]$#9Xu2vtK,jLr<Hk(lbRc;fAC/1R)GF+9peW&Ma#l0D)dp%]Ld3)ec*87GYO=7H8&nf11&3Av42T.P->n0oh@T%p/>t-Amxe*n4/K1vBo8%"
    ")o').T%iI*'hNac%%RT8wdCm&-k-0)d6EU.`wNX$*;4mXb+T_-<C*p%^,20162bV-TqXt.+)NB#deA44>j+Y#O[F.#P'+&#x4WB#w%)$#Nn]e$),Hr'A==&#]c@`=[7XM(LEWa*.qk?#"
    "%4#F#0$DAuwlKXf<x$]MepRf$[$vM(u;Tv-u@%Q/lD;)3w<@+49wQd2^cmb<fRAe<jB3F.$a<806H[h(GEn'5VtjV$3J^b&,[Sh4-CQkK-QJ>#5N2-3L3#[T^#A(s/m*8[LtwZT7k'B-"
    "&w*$#Z09I$$*U&=`$E$#`vD8&Lx.w#_D5?,%Q###N`<;2b1=/(hw,D(?e5x26E/b4i1Ds-XiOjLTo$&OiH'J3c%B(4/u@o'(=wT./[0-<b<rPB<vD]A14nk(<*ThXs=+&+GmcZ&Cgf+H"
    "<K8NK6YNg:FBiG#4'-;#2,3)#U'%^$?X]%#w5Lg*?E[;-2e1($iw%%#9WF]%Vt%##.Pj)#2<G##C+w7(`$c[$?NlG2nYa%?vS3I).PZ68uZFeY+Q9o%510EMUs=n0Hq.[#?E^b%4-g5/"
    "x?7f3N3@QK[_mZ$oYRD*#C7f3[7PpI#EE?,i;hr.+pnp0)dX)+K:=s%k8;k'$EoY6fWVf<i^9c5qhZ`(?L6(+=dZ&4V4fC#oSb(5aGne2gR8u.__P3BD@I8%^5%h(49tq.Q$DL;o=0G3"
    "?>3P0(?Q3'R$`N(h@A(#dIa)#H]Ep#Dg1$#tg,X$@.rjt5-sw$._n/_ObK-&:`@3'_%SfLQ.2K*=<wC#wgw>#x)=v#-?3X&/En<&-Sl##)09C%c(8/(^Q;ocdW+5pOAlO/7kH)41:rkL"
    "7wuM(RY)bjvY08I*.;hOsN#3#.*]49ZI0_,I;hx=M=x8'm'?jL6US#MC8xiL[WYv.Bl<#-u.lV/uKr2)Vk:,)srbx%+i.3:SU?t-L:OT%;DLw#,,GhL@cai0v[0B#ULD8.K9,n/C]R_#"
    "OM>c4X;B(%,*`iBwH'^-_s&],ZRsT/wd:[8dvle5od5]#Ph^P1<rJP'[XH23^/nn&N=jm&caZ_>$FJ3YDu@L4vgm3BdO0M1+;?>#9Vwdd`0poI(1OiKNC=T',_(($>KiA#&PF&#/sel^"
    "jias.-4pi'>(1H2_YAh(21ZL2eLRc;RP:nBp*;-GcGG=Q0s)f2Q@l'.pmGG2hV(D+]S6Z$aTbq(g?hS-6bi5%l8G?u'otgh1P<HpOce3(I-*dcDMm5/x-,Q'>K/495-<r71:K*#^)-o)"
    "_g`I3kWg(-Pl=.)ax`11HJ]Q0XNA_>H$M80vp7^554X1D9.9`=1dJf3t(8/((qrD+#T-r%w>6O*tOe3:GOWk),t>j1gbv$,i^Q**`+Np/F_9oe8wIf_R6CD*T_Mg*x_^+*c>3hLfkOa*"
    "PeTL(3u&m&47>j'(20S>A^R&clS9%bJi6q`0>).;Y(h)3ecWF3hTAX-L(OF3+ISh_oJwP1.x+i(/)e*3JD9>#%-^h1^ac=$@)5Z#6JtqC/7r(:V1Wm/%>7Z7u00'-4UcY#eJXe%sv7I*"
    "C2[B#b)5#-Zm(@.m;^e$;`Oh:Lg_x$6q'*)GH6/(smf)M`uh#AIwXL2r.]90)_n5/>'%gLKt=c49QKk9LekA#epQq&Fp.-);tdd30X0t8$cEQJi,Ta5crw8/?7koIs6fp.1$+?6.5qnf"
    "%wW)#.]:vunn`s#NM7%#xoKV_]XI%b)D;$#`a(B#[4.F%;Q#A#-b#:)A2nN'[QQS70p;Q/JRI@#rkH5/wCuE$[#KP/wQU-)mRdg)hl]Y,hA3I)En[[#KvLN'b#p;-XGNfLJZQ)iox7I$"
    "5>9F*6&_e*86RL2YoDpLO:FPAqkXZ.5d7)$*@xw#J29f33>XQ'fL0+*@6:a0qHR^5-X>;/BT99]X':)Iha,3'RK(K(R'o[#7HxW9m@1,3B0'r.mNlFF])dju#'KEu`#sk';><8&:Gd8/"
    "OTar7Y@qB#jvF6#Fm)_7$*oiL4Um/*L97)*3$>#->P,v,=^kjL*YeQ05lR?Qs%W$$4m:H24E,B=t/DK&svI*+ju>Z,?]l7(F[WP&2cY##^>58.(l>8pZ8Us@Sa%u@]jUm/14u2AZ9bi0"
    "3Wgs$5o0N(nK_v>KY1*47.5I)#0Tv-80#68cM(0(WWB:%c?S?,N]PF+sEV`+f;]Z,>N8b4nl=;-5g_:%](4cMlA$=m*Y/G35ji&7v?uD#Vih&32hs%,Xw`kL_[2A-<Rp4fP$cc)o4e6#"
    ",E#i7DLRc;dDvV%SM7l'ogo[#8W]Q(n%fL(,2h[#.QEX&j:EDuI>9B3M/Y&cpff.A03&_]-EQJ(PMqJ:u2s20XL3]-mq)U.QtQO(i>ff313W3(h]?g#crsv2oXuJC@bu`45Caf:K`^9M"
    "(r$##?la9MwQaxFe0xfL;JO[74s]U'UWG3'?pn@%;7@W$tO]X%og*]%UVnh(B`@/(>LGS7x8=;$RpTU%B[N9%2f[w#/PRs$?9YJ(vDTS%-<9B32[)221YP4(d:N;7%7/s@3M63(95xRc"
    "oeXt([T2hL[mQL2?vsI30g#9.N.OF3kOeF4<o^MN9n'+@jojHY<8KqC*<VxCSxGO1/4E5&hPsu,(M%6#mp+$MeUhQ&Xv%g)1&38&LgQ,MCW%iL5Uv;-efSA(Qlx#,Q[Mv#Kvm`*;:w58"
    ";+i?[)J;Bu-+O22ND*'iHuXL2bH6h%A-]x6TOr_,j?n5/vBo8%ne^F*[o^T.%0ISJ83Yw,2mZR&31C/)?&]L(Dcfs9sP*,<'9_[uvUFeuTSqO0dwaP&]OVS(mGfC8RArN2B]&<nU?no."
    "8AD^-IPu>#ZGi0(>[64(I(A9(e_c/(la$)*0a4oArwRx#d3<r4Bh70:1'M#?X/Hu$Zo3**6cqV$IXa5&+t]s-1x):89b3n0ZHFRc-'kGc*bhLC_T,d3o02i%(o_$)3Pd#*RN/@@t=SL("
    "'Zhd2/J7.2[UUF*1#U.*e+]6:*i2^6MR?r#1xO($;sO;$*m<j.UKSX#@#^85rn*[7G_+/(.rFS7D58>,VGZ'=SZcN'>PHX$]d3X&%shwG@0A0:[*Vc*ptKd2m7#7p=mtghsCd?3L=DY3"
    "xFlLC?H>B$<^nO('?,T8mo;Q/Iqr8$Uu-%-uSsWLX@;UMhng,3KqE5&R``<Bu@t$D2?l.O7t+x#sfYj(WMT%k$Oi&4.+F[7BOV%,o,vJ(I_el/B-^Q&YUHc$M:c2(Lp@q/sY<A+(D(#?"
    "ZZ^t$oo*87wi9+iqxo0#N)_5pQj%hh]o<:e&*eC#)IW:.4AXQ'9<Nv6T[39/PgWjL;>Uv-%)2LNfwDf42E'(,`79T&8tje)oFc?,9_-aNpDEI3Hxq;$o4<w&3r9/)DD?@-+@x(FAL278"
    "L$2X19tZ1<ceqI=Oo_8K7w7U&YiZj:i3kT%h.jm/HEBnL78KwuUEvr#`_''#Aq*)*Ne#*'t9vo$4aPF%KURc;40;R*3ZGF%pC'<&CF/+,%8;$#=hu2v3BGT'F$,<%h.I8%_o5K)<wUG)"
    "q$]A,uCmY#?0t=Qw1j9)Qfu=QZN_,)uvtt$36@0)%()%,X_f;%R@[s$WnalM%m=8pVLL]?IhVs@1S>3pM/C29D).m0<x[5/7]R_#`d+f*Jiw8.<&tI3BcBa*;FLT%VwcMTT.bS&Y-.h("
    "TagU%hfSC+Utu/C*gKA,M,e*3p-=4:[e=mufF_/)YI6p.pVK^2SV(Z#-Ogb&VTg6&6,Guu:JrG#W[F.#+nT]76-f:7TPiC=_WFu+6Z@W&`YK],Ew^-)kj:,)nD@h(i^uf(n^FA#[3Xx#"
    "R);,)KQTq%iTl##><ppcQ-j<h6-xgh]A-'d/A9t@%oSL2c1NT/=#Q,;Eg0wg(a[D*<gRB#MQdH5'n05:7lch2fTo+47@_7D$9+SI2fvb+KvPm/('QwAo_$Ku%A>Tq%E'%#ci)'=MPZ'="
    "GqN%b2Igm&<?nh(LHTq%)[_0I/vrJ<vj=5p?c3C=+xpN(]=]:/vBo8%0<hbajZ]v5W5qE8a/5Yua6>##3?AX##[q/#DR@%#Nxf`7rrd+VA&sx+Q.6##Y'S[#.&5J,]KS$G<`qr$Ax-s$"
    "x2?T%2pai(^[Rs$*:m$,o>g;.T0Rs&/Wew%E23qKGhDV7IOW*,LZJI*4Bm'$d:EDu5M?t@Fov2AWE'9.`S9t@9jmW_XXo7&SeF+3i]%>.$(nt-nv#E3;PNuH674GHNZGik-pQ7&H#C5&"
    "?*N`+j_M?#^.***uifLV8pUwLUL;G+-][s.SK#A#VL0[6(6R51^Gl@M0FCt#^g[%#dW]W7o98P)xNO-2ji_f1W4Ar%gfOKM1cr2%s5'hhZN,BN,v^C4>U^:/bU(f)hFq8.q=i8.9'l]#"
    ">kNW-RhW<U)$V=%AoKF$4FRP/Gt36K0j9?-0WL:%F.t>->.P,M+A%k(+dnd)3&)P0`K^RLlQjJVGInf*8A(U%dj`FrG$p`3pg&/1_KFa,](FG)[=*>-OAOoA6sQi1$GIK1lA<$?`$U#?"
    "s(+87:3]`<2?)s@KQ;s.S#KHM_)#V/Y0Bq`%ntX$^^D.3JFJW-pbv>#Rl%,=jdK(6G&JG=(eU=%CVk*$ob4M(&M0`1avHCb&>&#.ru)S&r:gB+_h[D,I=`<7>qk>7kt[Y#x6v^]=T.r#"
    "mXt&#YA/Q_JxH%bcFqb*6>Mo(e(FK2W`+/(P-[0#T?)4#@a*,4Ta`g(C8FX&)mfaHh]d)M4&wGGx3?+iH,RL2U<3mAsY`-*fM[L(e06l1rCn;%QjPs-euuLMP`cD%B0:a#Uh1a4Ln-O("
    "_wKc3wralAOgPn&fD%L(rC[s.JCfH`P;%1(OFMZ#m<:hLs#XZ#9x-<KU,]-)#-@(>w^S4]h3YK2K3]k9v(eW:,8vQKHIs13)0rb='O/M3wG4&>3fjJ*c;5Yl*a88.e$>PAKhfU+KFm7("
    "+#5of2Q]._I-;a,/U^*#YX*rA0?r'l*Z.]$%._K2u[OE=3ovG&*6j*.F+;%,t2Uc+6rl/(Ikj9%s(+87R/m>7Paan0r7:ph]n@t@bZx2A63fDcI;,f*#$n5/[8Lv-`_ag;i/EUgE0*/:"
    "is_S0f9v2(nOr.3AwM-*Z)kd*ka?Z$6fi3TY^Kq%Z,ZR&72s20;=/^7XF)Z@02/^7$*?3'0=.e4[P8S[%)XY5mU:&5,uw%-=_UA5]P-F%$1-(#RvwFi[^$&+?HSc;r)ChLd[ak'YA+42"
    "Sl;^#*s^5/R;9Y&J?Nl%&$ZN'JnBn)c,Sw%>RWT%l@P?,o5>##=YP4(sS-@5Q1=/(OAldjtZ%hh/(?m0a_^0/.Ep;.7A25M>QrhL*Z<78JPxDtWIe++e7*fDVdm)3Te>r%`:a9/4g$f2"
    ")CHuBYrlB=TI=QBaqq?6tEsZ^=q'&vW>Bq#L/4&#k8YX7`u5n(:Dh7/[1%01o6PS&]L)s'8wZA0aVFgLfGj>7T;9mLc2PTu2xgK-W#3,>J]XjL,h-t$GZ5<.vrse),s,f*=G0i1k-tx-"
    "(PfL('LIx,vU0b*DPbh1IUrt7dC%W$s$#S/Kubn*A$JgL-0+%vQNFgL'Ykb+=*<J:AKMO'OI/n/5$XV$kZ7*+_l:0*2G#i7C$J0:tVECXb*QI)Ng>N'j@q*#>M<+06KW6pJmtghe57i."
    "U6'hh[aQ_BZ?o8%oVp;-e`IQ-im1P-AJ)@*#)Tb4qtWt.4ocW-#=6`47+2-)c@L=%?SD[*LIL]?w1.?A`iac<4r^>mv>ZE#N-8C#g(FC>6:hB#lZq/#VffQ_NgL3X0L>6'x__O9DNcG2"
    "`ZB)3G8GV78ptZ#>3Rc*S`ej:nOlv,ttUf:XXie$LB35(nsZ&%@?]?^sRIHpZ9bi0=G>c4JW,#>7I1a4n7xkTcS7W?^`WI)PFsl&9DDV.hxK[6Q]t(.s?2/)5qDOBqf:V.2Q^oBxs0%6"
    "K>?w-.BY40dbV0CRQBq%PhkU8(LF9%vRXi(AfgQA])5*?x]TE+`(BF*+il?,GrkZg.sA(snW8YlqSF]F^Wco/KF/+,I?/SUbxw,&7640_ok;F%%',F%jSn517X7G;c-K?$Qpd(-Lesl&"
    "o5JnAIMRw>O`n;%U]$A#lZ1T&>K#L>Z>$9%iw]4A=L5s-B5cf*KWOdM;x'kLEU`_<F*M3t^GUv-YMOk2lSl]#/)D_&,*f/Lo-&n/oCHC+E,TG=eo3/2VhRX-GlN/2LjAa+W7Wc<YIdk2"
    "j8f'5:w7a+,gUj*kD/$,[+iC,^KR,*MWlN'd`N**?:rv#ipjE*pr$s.<KhU%T8.h(Q[`vu?$x%#RCKq#DjUv#i$###.]:vuKW<p#O)+&#W+Ku]C.j&=<.wI17h70:'Krda>.S^#7U=U%"
    "kc`N'Ho2S(.Hv2vu;tk'$O_d<tk9B>`.lV-PPvO,l*VG@CRft@6mtghTj&9(Ex86px^b[$4UPUplf&J3<)]L(5WLD3,ZM^>+h*H;KG>c4xuk9.->]90&X%E+RWw,*lBx'?I:st.e9Hc*"
    "%sV4'D4/l#Z.QX7AYI-%D_Gd3:sK%@UHWe)aOVZ#8t0?-)A&p&,),##(6ps4do7-#7O']7wXWqIqr$s$h6h&$0F#12&9$W7)mlt4:gM%.Y]h$-knxv$w8Hv$Ah8/+PQTq%kKCNhC/1uc"
    "4OGDuUns+0$*=%b=SM0A>[lS.QoWL2=i)M-`Bcw$Q>P`%&iq,NdnY:A;(#H3HN`hLEUWD#JJmN)rJD7:6:^1(++Cb*go'W-%>9322*TwJpZ=-)[uZT%Ke4K;&'Vv>`Xu_+7ll>#a<,I<"
    "8#Wk*+jCgL'fi8&U;tCjLUJJ(D`Tm(=#B6(Q/`k'8mNt&/'f0:pc*87[k,R1@jYChHSWL2gL=p@IO$9.nB7g)6o0N($CeV%YGt2E:^$&K`hu(+8j(h13T(mC$VqV$fHrY.PPYxkYd/20"
    "#&<W/<QW-*MdV($BE^?-lB,<%771^4Ok53'?j=?#tD-(#'6`c#ZT<8p/.1uc+(LB#Q1/`h:-S%MJq_H-qfp$&Dv19/x)Xf-qZUq^6h9a#5l7F%>o^:D$c>.<.)hm(>*<L)_V0:bJ68=-"
    "+)8L(AE-R0k)T1C2S6S'O+.9%M2ciC7$8$$P'@dD-d_h1=_8w^cAVr?Z->g1wl9pAMj6-);YC;$S8*hL?f&L(]FOT#AB,7&#R*tQs<>b#c:EDukq+o$u0j.APhm@=-.H0)]H7g)IGEF3"
    "^&bK64J(XWgU+u2%>k**QY9.=L4FqCq^L?%CqL;?fd+^%g2aFrI_t7e$aU`3*4IU77s]U'B?s?YX'Rm8of_V$Y5BCO=fCZ#P,1NW+hj>7%otghS$F0hZM#'33ovn(D:XF3'K+P(pOiJ:"
    "6x@Z5Xq.[#Q=oT0B<K8/oqoU/PrXfC<a8[6`rb9'g/YJ4#30VH9^3j;B9aA,RQgm&]2K[6pbuT4^>#jLmQI60XC3Sn$76;-[6f),+P[N'6Q:)=A59Y&s*xL*+G#?,PqYN'/_<8%/rHv:"
    "QV%CS_B[F4>U^:/GRGsfRMCe;Tr0B#Os*H*o/JD#XZVY?JZ;</4@Iu-^-E?#76%a+PKgm&v:7Y11#.=-5@KC#S4i#7>u(bRb9<vu[p`s#be0'#6s>^,v(S#.vhb^._9`S.[a[<&u?5/("
    "qUi2:I_I0:B^9i:PEY/1?.YW#-Ce.A$8'hh,^Ps-%Xaf:4nLE4;EsI3Ym?d)C.IW-X:x<(P.C+*w&O98'biB>1GAm0MQ<d3IOX+&BwFt.AUJLXd<IpK.mMLXw^k4:ax?<-b7mi6r01/#"
    "@s2p^sw;$#638`$2G#i7>Bo0:,IPm'[/.@'8/%@'8]OQ'7jI[%=skdXd'0uc0eEs-s&PfAC+7m0vA0s$sRIK:@2T;.]n0T%oo']5l,uEEW.<l2#nYg(PV`50VEuJ(*g@H)ab6t-j_]E="
    "(DpBTlPL,5/iS(NawGm'G+LR1T[Nc+BxB+$'),##25YY#K:C/#b8cm<eo%t%>at0_JwZ#Mp0j0_9w<F%*8>']tipEEcB*j1TB$30U$kDaFH:K(QM9F*cbV$#sRL[,noeh(,,/1()/f-)"
    "QK+]#R3gm&K(c4i7.H#@dw<A%1-;hL9Lem/jp5uckBRL2JA58.CiVD3JuNT%KIE:.w1NT/9a[D*j./-)NZJA,NI,>%^>R-)9HGQ3sAm'&#/8P'bP;J2&^I-)MEjA+U&?joocf)-v5%C-"
    "qJ-M)wWQ%t.M-/1Hnf6#:vE.^5R7G;kc[m-'2n'&3GS5'xi`>-`J4E3nG;k'l:m%='+p60U34x#GCtW%em>V%-NgJ1YhxM1Mml8/o?-R&W.i>7(W-E#&/sZcsmKZ6t0$12TZOM@)Wwm0"
    "o`[D*RP,G4d(is-Vf/b*jXHU%)$3H))A4I)05A$5WhDH*dmg:%YKKm&v7xL(o(L[6KIuQ'pYiO'i$Hw5ivAfh8ptC?(q2(.IH(6&KLUG)a[>l;Vs9<7#IbT%o=8N0f&O7HTGcl1@(8Yc"
    "Q-()*w(bs&7v16&mcAU72jA:'6ugr]=_7G;SqN%b*E5$#nt?W&,$%%%HXRc;n%fgL`H<mL5C>,M$@GGMJf>;-*I5+#0+4N9el*I)b#Vg(A7M;$EN5r%E+vu#AtWX$bS<e)e&Zn&#lT+*"
    "A%Dk)JmUg(hD&U7JP5Z#-Hn<&i:EDula4RuPk>8pa.Qq7wIE/2=h_JsO:$hM]K&s$@#G:.W4:W-pYoF4>'[)49]rS%);Dc`HUXkW+3T@#U^54*p&bvSE<[fL8CL2<+N4Y/^i?qAh&gvC"
    "Z:oS8'^Rp]i-c`*SLwqBm+sK(kS&U7`D7I$G[blr)L>gLJ9;b7JYRc;=4pi'W;N)'5*S?#l`N=*i]IS[8LmnfS8oA(*#=E(#vxT[,(6nfR2wk$>0j>#t/DK&kLBmfE*gQ&rc.L(XE0u$"
    "/JG##Wtjp%I:;;$Fd?d)_Pr0(hj#n&*:Hc*jLJ3KGF.l)P3FM*F[3T%<A$t$-<9B3Rx]4Aab-F%WU[X-8rQ1#Mr+@5kc%adSn%s$Sr(W-gK4E,+ZcL4>'[)4SQV`'IP<+PD[_quXf)h)"
    "e.it-Shko@;5*_4J2jO11E5;7.F0M1IaA>#1a`FrtIGDEEsk%=O,Fxu'g2n';s-a=KlC@#=HX6/eJ8^+%&ri:(1DA/1529/ECnS%L+MZ#%9FZ#jNT;7%9Y,;dlWjh>KITc,tN,cncrU3"
    "Y7o]4ZH7g)Yf=Z,Ug'u$C]R_#U,MT%*Qq;.36=n0FnYIqMW]Q/tI++3/K'CO+bSJ3H<FB,)k$m:5Y&C#C`^02fbh6;w1#G45)721B&V'@QBt9%/h:l0M_=K;.jXlp^GaW.;9w5(M'2hL"
    "5*pP''luV7;lK3_]<e)5GkFC-hSF;-wq.>-UqR:..M#v,vkiM25eE^,t]1>$pXv,*liKS.i:([,Q/0#,+_@g:Cg&c$&)WjAuUC6pPF*'iSnSL2Wp?n0*H5s-pHaf:d8Vdcgo=c4'?]x]"
    "381K*OG;d<t%qf3+RJE+mFIY@9*1uAdeWt.qY,ju&inm0)m]Y,9wqK2h'&X',(bk:^a9T(qaln0Pl$:(fT.O(=d#F4Y?@cis2sr-M%rm,gWkg)R^Fl7(^QEnf*Nd*+Nc&#_XZT.Rvp]="
    "wKPJ('P#K)]Ovu#[t<e+=v:P-m'Hh'/8tGhsIg.A,D5C5SB]%Ma0Et.iAj8/,v;ObIG>^,O_YfCmGQ[7D3-G,KZ'O3a5?C?WlhZ-#ssRiDS?W.t^m*4I,e3'b=24B7_Bp(D<>O-@W:L."
    "gNr3(FHRd2R6VG#h3k9#3iwZ7jDQ^=c2g],^@c**-Kn+#B*_d2$@2)+$gMk'XggwQJE=1:-p/E><Kek(<c[FYwvR`(Fbs4(3LYx22)?x6HZU&$h=S_#hTU&$v*]sAT'JX'=RIw#$`^F*"
    "NhFN1pQpY.L<Ig)$fCH4=jF#-6$,5J?8h;.[tSu.3^.G=E>JP'A1G/CcGrD-6N-r%WV3e)rFXi(IdW<-%OVgCv-kY-qGn)*h4*T/LS8P'OdcN'Y>vr%CvEjLQ($&#DDwo#pk9'#gKif("
    "5r7T9V'(%.*PV$#jAd:8?w@Z5HXRc;Os=;$XL<5&)wBu$FvvD*9MeQ0fnPjBMc6w-(n^Z$=kmU7<%YJCV*2KC$,+87fsUTuv,Em:q@wihAHTF9.Gk2(&XG$<Cp'B#QRMR8Z+jw$,0fX-"
    "-Q/f3OM>c41M8hE&Ng4&/#_@#J/.u6.J_a#D@Y+D.%+;/cS%p/eGd40R+PW6`h0)GlK65-]CZ<0D]Lb$UqIw#?OnmBdxQo/#Qe-)$nbp/w5hd13?AX#_[F.#O:p:^%'jWhv/v2vN'vjL"
    "#c]l'oU)I-WKs4B72im&nQf;Qk4,c*2N4MC+B1R)XB'>$M%u*$V=On-//MU^duiR#ZmF&M(B26pix,`%A#Dt.`X@t-fdFp897=W6h<Ss$RtKoe`aj(4CSt&+Kq3:8mU[SB*A7<$omOxq"
    "-JY>#SNq2EE_eW$1n6.2[v4b+e@@L4T4fl9ge[?TO34t_KcGfhLo%8@8;`v%KY?2_TEr#)jk#jL^j:W/J1c3bm*=A#,:TD=&h/V.n0F_&<o->-Pb:1./LdAO$dV6phGh5MR(D%iEJg.A"
    "mXCs$`G/9/MD2#$DXAf$]>dRax%F.3oX+VFAXKT(2TYS/%qre3iGE228)iv#F@)Zqc5)#5x5vV;P=7dD;F[s$+fKb*^@W0;&P%:(%C&nLj^-lL@v_q#Y'U'#SbTP&1CJs-ajh+M1.@R&"
    "T>CJ1)Bi@.h'G8&vpa]+9a(<-O$J0:)fwF<#0kMC=/mA#vbYV-<)hR/`Tt]#S=Ub$V'4,%6?%12P;6N3$ium0pkCTuG4a%;ZOO&#QO$A%2Li/C^5$.O@fh0*]CZ>#NP;J<-k?P1-=wt-"
    "N*j&5<-wg),;BO;%+56,u&@51m_^)4rd53'UqIX'kD`O'W3kT%)`(2)I:<CO-49r.rH[=-(G;*%1elT#eh1$#aiLY7Dh_v%oOM]=[u7p&)r/m&R&Z'=Yie-)mpC,)Y_WM*WARh(F&ZPh"
    "J+e;-.hlG%P,``3J'1T%A8o]4NCI8%M1'C4*^B.*-;aO10tWQ/l3QkK=7k*6VtlD6g`si'=[3Z#-S:U.P+[T8nPCi)e8D<'V&T:v<C=S@qwm+`5;<PA#)P:vg&_w'o4aw'<x=PAtZqr$"
    "80;-#:B$)*rHD*kIY.,)]cu>70d9%#Y-9I$)$9I$(w8I$,QF`%xwGm8=3:,)epS^FLHlN'Hv<I)+<n<&xE.0:8H.nA8wNw#T0Ft$,0i>7ka4Ru1]*u<op'Bu-DJvaEv+Tc#$<K:GXLa4"
    "9EsI30D>8.N.OF3w:O<.J%th`>+VmWRR[qJe,AuA$YU@k4V:du0A=qAOuv58lFJX#5m'f#-;G##';G##2sFu#[/U'#UEX&#2dZ(#Il(K2og8x,(kJ%#x,1^#+@fA#/2C<7sUFE=tD-(#"
    "1l,tLH#w2vLmce)Nc?&#Mf]'+-_3&+k`kE.6n5R*&=mb*1EWv5DBvj'>p=]%IjWI3ti]cWW*dbWmuF*XR5S)XO`IT%^]EQ7AJ;T##q*-1=-xgh5>P0ALrw2A(e2.A>')@5f?pje6F+,%"
    "NqUN(9;d8.sIR8%jldvn)reC#7e;B%eap;%5F$`4^2a]+#qGv.F0I(+%NpW7Dh%-'4J&p&:Is$,ow^6A`J/e$)VR;%rP$V%Oa-^>o'AK#%V$<:KiCZ#[pOv,GZC]0fW;N41fcEG5h4v@"
    "/h3;H`6>/4JZWJ07I+hDjhMs-f:h*8Hg`#5IMPJ2bU1F*DEYA#umXx#0O+A#pS=c4*mElf]8'v#GqN%b:tYN'SU0]7qns^#/m.&0'p*m/Wk@u-GMNr7Jacs.TBl?.bG#o$@E)B4J>k+4"
    "Z>at2UAA..71Cp7+Q5=7n/6,k<DNW#$EjIuXON0A8Ift@Nt+@5.$<F*K-xgh:N@9cwKs<-8;4M2IX2D+huO((#srO'YT&8'2VLd49[Z?,.8'].HE[e<3sXs@s(Pd4Y9Q;@NG]p0<<G##"
    "xG]t#9n=6#S?O&#4jd(#^>,/2iXRc;lDsV6c3W$#0Z@&4^/E&54bi[,Q*l2_],X<*oE1f/`ML_&2jd2;Xg-X/V4w>#GZ27'9xA2'CvEm1Y0f&2RGe+#MZAI*ER#j'wq7T%Ya+xP]JHu$"
    "-&'f3`;[k,^-fd)JoGj'T+lr-al401SOMl*uBj>72ut2;3d,,29)rpctr8ja._nTG_ihj*xlv20LCZ.c0=>n00NKL=D8^I*93fq7L6$Z$Gkas-)<,98;?JX%/MDi:m&`l0#1lx>K,2N;"
    "v1f_'DrCv#fMvn&8-nd-oSo8J(X-*Ng#&^7@QTB))rs2;%m)D-v)OD+?HuX$F(P-r/bEs--6k-;nKt87;@%%#>^<J<i*6Qfs+Q<$$^sT7A;0U'CcUV$$TlG2I?-/qQob;-r?]`aU)g;$"
    ")EF;$2x7g:MCrZ#1xZS%J*e>5PF3,2]8G0ARwTBkv1Zm2f*2N(]ZfI*Gjt,>.ZjT'2'ukoOO@h)s7Aw>2eN'6%Vwkoj'w9.e>[oukl^32=5>##R6VG#Kf,3#?XI%#KY8`70LOi*$9X.'"
    "7<'YAh$Eq.nlPD/SYQc$LjA(M>>8W$F+,B$Y^9K-)^Ng62G#i7N<o0:bwA`=<Ebm-:R/-%sNatQ@fuY#U3qKY#Qq>7gZ>M7*2=/(+=]4As#4;(^)_4Ah`4HprP(Z-,Hs;%9V$uooL4I)"
    "H/h;-r2Rd&6GL=upZWS'?T9?-Jq3$6;xb6''3<D+V9a%,-9ps4M;oJ1t,@oum+Un1El1Z#']ds.W]ds./q&a48#j7gA5MJ(lTai0cG&9'l,3j(Hw7@#HWlf(5Q1L.O=l1:ln]W787]0:"
    "m6HT7<a4u%kH.nK)%gakivxW7uVEq00D_'/[?;O+)rN4#ej/W7pLCpA?`(pfl`bY#Gmk.#w%Mt.<44I-;C3l,XKKf+B-+,MT'u%X+h+6&2Khx4BVC8pf-f<7XGg.AM<U;pjtY(1=:R*1"
    "TPe2hf.^=p264r7ePC#$$Pn8.A*htmV6YO0Z_4U0`,]?,K'Gw7vxhBuERh118X'm&RMXV-n[VZ#.<a&QuEh7';Z0A=C3fU7K%`0*XMLR'#TpU'%o*&'v_e`&vBo0:qX$?#1Pb#,P`ir%"
    "F(iZ#Aea5&_9BV%I&+22fBk>7Notgh)oB*1bc^%3D[=h2cCo$$:o0N(X;*F3CFTvR:v3A%&F0f)>S45;cEEj(ju.w>&uhd4qLN>A.YHg#$[>N#k/v>:&0oA,7g/J:8D_e*'5>##76VG#"
    "9f,3#O3=&#(Wbd7x;d(-'#op&VjN@,xK@W&<n7J<]%r0_ukFg<V4Dk)+Kk$#@4D978v9w#Pk&H*+CY*e;+cn*==c87,QXv#E4F=%t.I8%9C/a'->m<.F__^'3=WuQW-tT%e9_S-bQH5."
    ":a]*NhNFDu*GoJ0:HqTG=R73hV8Le$kV`BGx;`3(<c<WPMQp9,<21<-#D.$*qR?q7n9Rd+F5or#%N7%#(WH(#Xj*T/rtRc;HW'%#i#m'&;#m'&&?Db=o_dY#TaG$0mo*m/2G#i7Y;c1:"
    "5mbV7jET'-u<f0:rsi.M@L(+*XLor%7T(2:[Z)<&=5H?&v'>R*Nt)`+c[q-M^Hr-M2fdwP9'ic)R^)b=qRC(+G_kM,R3k5&7RS1$[T<8pK7eq$E1=/(&$7q$-sWGu>d86pa@vgh'#5HG"
    "Gkn;-M:sY$BQ,9.e0w9.gD<J2sxAR1^n>m'V#Bb*f8qY$8fY**(),/&ao1q8&vjT#S?<-*3A5YukU%G*K)>>#-,>>#4run]rGsbNsaH_#Z/1>$2aK8%:E?n0&6U<.FKSX#T4,I&c:i3("
    "#DeS%`h(/)dZ2b3'DZD4,EAGaEiK21FM(lK*6:'#E4rr$54uJ(mQqV.FtBmgVNE5/1vt?0jAEPA&'(,)3.Guuq'BH)@kN`<$BE+N765xuwE8_MWX=CPN`Q>#?@-(+[R/r.fh]A,sX=Ji"
    "r>R&#sB2A.3_+/(tZ6'#qqv##aCQENj]XjLhXw]MZ'uCPJYH>#tJiG)DdOv,VE9#6#JOP^rKN>?%Q3B#V&_:%t)Js%^#(w-X@b(N,[V,#bWt&#4dZ(#>oN4)G[J@#=B,$4>q_84aap:/"
    "&EnR]e`X`<qn*^#_V>;-E%;NV7n@Z#ncWe3(rlnp9&W%$T1..NC1PF0s<D8._tLA#^8Tq[3&(h:xt$<A@qN;p`JG8.6GFJ#?$uM09`YGux%F0hov=aHM7fS:xcflBi^KN(4NVO'tU1$@"
    "DDKCOT96H3F3H4Kpq6_>@$+Y-@*=u-r;hF*DSP8.H&lln)O`;6Ri/c/?WvN'LcMB#Pk)R42d2^#/G4#->6f],D(;T.>VR:vMm_]$0-.r#L6D,#,]:Z#v$###D)6x#?Zd`*DbN`<VnxH<"
    "I%aG<<2HF<H5pr-VF<u7q+Up7%pCq'u3_0224fT]e1sL/?/Xl(GoH=-1Df_4/G)G+]wN2(/0fofIb+Z$q^Rm8iB.`$>E%J3ltj9]ltR8]`n*?7,aDQ7W*f.8tYx4pE^*gLBtN;pHhdOK"
    "PQHa56cE<%R_Ia5bB?&8n,EVnt9s8.cMtB>q5f6+H/G],msh]$PwLp//H;x75qK#$fDwY%lw.B6EA1;.(wL($M,*c#@TdF#i[)B6KOo(#de_c2RwCT'_5SR1?;^;-OcuS.9=a=l0nP<-"
    "V+pGMWA=/MP5+/M;s[.M@[<N0_/xL/`H@P/r9ju7YqWC53M;G+H6eA#hqN%baIHC+Zx`u7Kih&5J0Nf3#(CM+v04M7T#5W-b3vAHlS7u%oOJX.k/5J*blOE.lip4/VQV^>3B6t.'jq;."
    "d/GmL7%co7$_txYU<x<-xI81.#P#w2QtRc;b/>)%$GtAHJft/MVYb/MD]9,NT,fiL%qOs8b[&a+P.8E+-<jA#xU$6%D)kh)P&/J3HS%&4?o7g[whS&leWG?$w^Rm8poN.3`&#?2`@1K."
    "8oh/%21.68%o=5p=nJpL)'pIc9RwM9M'.m0[qrAHIoEb30:%2&U=8`&F>`:%e1WV(3>v9.C5i9AuNT<.5r^792/6=-.rF%&?v6;%*#6p%Sp,p%jgQq&'Bou,Atl1gvZ(&+rn/JLR>bxu"
    "O;J$#X0hJ+CJt+N?vRiLwK8Z7DskM(0-O2(/$4m'.qnP'-hR5'*?_s-5]WP&d4I&#_[P+#e<.E+VFN`%jN%E+X6fW$SbfR&AV-d;A)6k(I?%12iRM0A]&8I$8c5x2t-Tj*nRs,;G<Y)4"
    "VnB)=7VlS/x#0W-1K+v0nXaD#S1&g<&^O+$1B0C6`n,i(J+v?6>mm]u,ZN:.G:+@$ER/B7OJsM+Tp@o'X$gN'G*D/F0x=M(5rVV$TDA^4L;<A+&Zu(3e'GPAa?Um([e&4_(Mhp+p$pW7"
    "#OS%#x0cC#Z.=X(^?eG*%+)(2&K9/1VHI21[an21vvwl/gg6F%f^HgLjQD-0iW@5/Ie5F%k>3'#a)uJrFBE/Mlvk>-RgVGj?33/MJDOJMV0K`t=9a+Nf75dMK.iIrt4xfLtCBE'U0SnL"
    "g_Dla6]&n&PlqT]lQFPA^_9W/>&+5(S4vY#bAQ7&%,>>#b5-r%hMD^4+JpF4`BQ5lX'9^#fcX`<6W:&Gc3u_-+;w]4oU%&4LvjC#k$t9]#$OM0Qm#6#cBk]#=D@/(EvEp[6.RS%6Bk>7"
    "I?%12eoH$.E*O;pX1lZc/Oq0#q#F0hq70M@iZ0N(W&N-M+:s&3]G9t@q>9o0@Vw;%j7o]49a[D*>IR8%YweP/]^s=6N3d(XXmcL1m8x*Ng0>n0E*UK(j`6pIU;%L(H<tO)W`3E@c4N$9"
    "f_]@Tl,jM.@=F]-@iuC5u#=4)l`(T.(g''#].dX&`SuY#aw7),D`7j'#W.j'lP%j'`]r6&@Bf58W;@Q03/i6#i'G[$+Ev2vvPr+*t9kw7BS1C&w/N,MTr#.'oBGX(mH3:7]oIq/GFwR]"
    "Sj_akf016TR1[8]W0-##cBk]#=>7/(0<#QU^r<3pa#Q;pQh?H#j:MHp832WS>Y`q%k.2=-$;[B&a50i)k6hi+:3+qK8&Jfn#7fk*D1.W-kW]El+'/$&Ou(##5&t5(kiRv54jQb<@HXW-"
    "g<Ob5&WT[#@D24#+Pc##J&T*#Ts,U7=od-+Lg>b<=&Li:R@DH*XC3SnX>#@0[C%@0-l#AO,EF_/Ot&a<INUa<Q^O4MPSe.%]nooIwqNiKXcB#v'qcX&4mSU'bDg;-:U<j0B40L(l]8U7"
    "C/>7(.qnP'@fo/1:Ovm8qJC#vx#lk0`qEP0os+#'io8'+[jPn&/Z1'#D7oS%:Pv##8^MDuUKkJ%)9J4(TS6/($8'hh1C&wa.g3x2Vj&kLs*p+M<piPKWnC*49,B+4_T/i)XRmF>[&Md,"
    "j?n5/D#`T/ww_97.R^U%]%NW1OEl`6(Rpp8cqQ)46o:W-+Z_M'Q_dO2U*pY#^m-<8Hm<H)0/*xIcaZN'RRrW';/5##HBiG#4qp:#r>$(#+-@40rtRc;2l%T%`RRc;npw##4Iu2vuQ1Q("
    "Ua2:0.Aw'=,>YY#xrG@-@2)^#fEDB+P376KqCfd<w*F^#T%;8.<HbjLTPuD<-OZ$?AuTT7B87w>(+k3NJ2v2A,&+22X3u2AJ>mhMYZNFppSM0A9RF1gqRWL2:l`h$bPnZp/lh8.W/2&7"
    "6k9a#h2#LudM&J30<rG)*)TF4=wZ)4-V39/d[F`Wpx:CI)W+H*pI;_,A4GgL?e/&?WYlq:BWT4/3,/p&$2bK(i42/1NTFMF`tV[&-*t:._H96&;uefG,ka]6xf*h<t;Puu>w3xuA+06$"
    "S7,i%Qd.-#9o^`$EdlW.&Msc<*B&e*gThT.Tf''#Vqbg$=W>m/<@=31M<2c#f?,2vO+:kL$Wv`MeI0L;Ya)1)3.Guu#hU^+_S;c<e*)d*Y-`T%#nmV6^=5;-I<Ig3O,r0(MWOY-@/2(="
    "5OMw,:Hrk+o^FA#l?L+#xPo2)Mu?VH'DMH*[*=]#ri-XHO4W;7BOjmL8S9t@diHUc&)8/(9U-Tc:(`h#q`bx2RCT&$I-Kw#fbo20N:B_4#a5J*Z;1,Ha)@TM*nv>/Pe-f2)w4qAU]Y(5"
    "@sV]+T0@ci7k/na7:S(#>19m0raRnAX)p<Q,//C4$Z6A4wt4'4Z#Uw#P5mp%+_nrQC'nRc(xD8pA/c;-F[;I.%vXL2o0,+&rZv)4C>-T.@4[s$JOf.%xM1@G']ND#0*C(,Zk6G*il,pD"
    "SHwf2LMBB+]Jn)*#:,2+f^9h$.[6G*qhkdVHMHUW%+8x89YGZ,NR.[#3e8NG:HLd2TGBU#,a''#*8.Z7uJn+#s>'W7HCL2LK+5d3u>Za36Q@n*JQdL2x9TN0H.Xp%8.L<hhdt=NgvN;p"
    "krbXhKLjG*^E]s-NDx`*EGhs-wh3BGh-nY6>pa;%7nIO13?PY-hMfh2?-bD+eTZRGGFrD+7N^6a3QOqilgos7AGq=@AWKs79nq1;;+rVG0Ukxu^&Lh$aUR[#8c&*#o[d;-K`Oo$0k,&4"
    "q)?b<ed(J<rgums)k&i#VQ2uL4O<Z#g####<OV>#;2a1^eF?>#]a/f36<CE+OSu=l7nEn<uk0dkF7Po7D;5b+8ZB8%OZ%>/IT.H#lPR#MVeeb822@k=O9@W&-YIQ0p5/m0&/Lg2aF(;Q"
    "EklA#]vvp/r8-V%1inf(fc;608$cf(an/PoPEkYAGx[^7(f9g2Ja7iL_5eh2>r2p/a[L7+Ng7-#$VM'50lHX7x%^32t6]0:0Q?H2bD6^#UJ;V%#)it'bp^5/,nA0A9b0>9dZTj(_U[Tc"
    "c%Gt?MU]D?v$q=/JfmQKg*aD+nm[P+/?rhL-g8x,*P@@-4aTJ-iPcA#X=xq/2G#i7Fa+k/jcnl/t->>#Cl;m/T?)4#/IY##OZ>T%1CJ..W+U7+1%Kd)6G;0AaQbs0x.2s@;0Ys-_Kb#8"
    "t-7m2CiNK(nHM9@1%.<&s7>H*H5>##TV=s?AO4fE98*h1VsC)+2^3&+H+qf)g[F+3^5FT'3H(&l#/O2(XSMB#+l=D<@pUaFOvSgMt@k(X_?m'Xav_0(cCmxnm,6/(Mm]#A=sv*2D9)@5"
    "v^.4(*;RTuIUO[?JlcA#*mbZ?W>NNV<EWuP/=(Kai(+&+M0J=-C73DB/AY`8*h%?5<.8q/M<2c#@V0r0.u?>#5m`FrrCGDEcA^uGs&>F+CjNA+06c1:nReR,ev0V,'mvcal`G[#m(`Y'"
    "gOnB=3s82'%t$s$oL]CQgXk,MG_(f)Zmss-YX7[#IkN9%@CeS%;FW4#J1J9a1nj>769R*1&CQ&i7OT;p=-xghdllh2SJ<j1^?k=.Fl*F3)vsI3XD4O%`?vI3]q.[#/]Fe3N?^:B9rqd#"
    "W&vN'FV&2B8<%8'g/YJ4E-8t#xk.tu.]pY.MN*?6$:>]40QQ&#[l)%v^c#r#MsC$#boUY7XQm+#jP`/LeXsGVSVJ^+X*KB-Y'nY#?KA0:Uj$)*vD>##(0i>7mD35(CF1(BpwcG*`;O,M"
    "_<[&4=%^K%r75p[[*HH51<N-<:d1<7,9&/3aHh&X=gvH)%0PwI`t#>HVO_&?5?*$%&)>>#R,>>#-b8a#R8^kLYkd##VtV<#peNb$=Chv%ZH#N'7CCg(.Nt;-)XOv-kPR#MCZ,oLTXZ##"
    "5TT9X['88%7O6)*GkP&#`NOc%E.IP/8YR]4+mh6#j[Fl78URc;?d0I$IhLj2Th.H+F;h1_qF/N;Q^vt/;,W@#d,#v,gj@:74YF@54V:v#)>?>#DXqA5S#:;.HA3#58GgF4mxD_&aUG3L"
    "JH$F#.@5/(t4H>#0<#=$lvW.Nc,S$n[6Ux;vYMs@uZC3p;$0t-hVC+E$wcG*+Ncp.,G9t@lhr<f2C]0sLg=d;^Hu`44H(t-*?-M;%)m29`CE9/o7T'+2:.=-nVwd)]gjH*>#2O'e^Ja%"
    "g7^'+jcE**j]3e)FJmXQnD,AR%%t2)<OmG*rNwX-T?wq7j.P2>wxMkicC>#)4a,0)1.vP5]-MbG7u+s6(8Sd2u$###*?9<$n(+<6+jQb<9Js0E#D@K;BW$#Q`SZm/W'Yx#9jgS%UJ1x&"
    "Xuw0ETsZV)tq,U7HYrC=HNUa<$uC(?vOEJi&:1`$R@)##fEVuP(qcaO<QC3$-GD'#Sh4sLU@GxuK(oo:@B=;.ST;ig[1.O(1CP>#1KZ0^:;5##W@sY6_4tV.0+v`#biD<#%X5r[6-u2v"
    "*EwZ,EE^a<D];G<^R>c*MJa<-$u/r-%]dhE-#@A4FdA7.dQP.<5w8E#Yh;eNxOvwLnq/c$0LRa+&j*^#Y:^fX%*fqNMK6t.+9kZ#h*]fXjeDu71kQ>6>#ZVe)eJN-fT&?MUSdl7PUD$#"
    "+*S>#1wI>#>K(-/9KSX#/IC$^A01R3%8x+Msn9)2ExdK1Lw8a<PXoH</DYdN8UjMMI_xvMOiN4#J%`E%:Lhv%4Be<C+kd<CGw7CAEw7CA46@^#_6U*@xhS*@6*eVB13,O-fZ,O-vS(L."
    "+NkV$0E*Z$-A$jEj3;k?h*_^#M`*##`R=$p$Y(9'^I)$#EWJB+?03**$g>X'LaKXCU,GW-_U6w^GwlF#r$k-Q>k9w^uoccWs-s8.lPjs-7I[58Ae:Dd13,O-b<,O-23(X%<L)9'C7-(+"
    "^_J7/W73u-wO8B,FW*Gig3;k?ui9^#LKDt_Zdo._u%`q2KhN`<=oMq2bC8r2LT?t_Qsmt_wt[f1KmVE-9SimMAu5pLC+Q5Mj-.U7[2]jLu(^fLGnF?-K@&j-Kh?w^bd]w^bluw^j((9'"
    "A/<4'L]`i0F5HP/(/^G<f81;e*fUA>c@d;-:f4w24*u2vx)%_+P2Qb<F]vF<:/<aG>vj8$IWKXC/s:BZROQuu%@8E3S)sE-<f.nM_HVPMEq=wMlu-o+tq,U7Tl0s-8BmD-1))mM@)M#v"
    "kRv]Mx#au7oq'^#Oc.kqW[.a+8T2c#DjdF#3?kt_[=i;-.P0o%`;A,MZAAn7f)C:%Fx>>J;&XbNncV6M`eR$8lhtA#K6rM<o&I;@e+@w^.#:p7jbtA#EwqM<(R6a$rN.##KN%H#pZq/#"
    "ee:S_]XI%bS^$n&'o(W72NE=&3t;kB@vv^7Kuo,3M@G:&(%=?#$Iis-@2r'#2,u**h$FY$'-1ucrc;T%s%_e*I1ZW#&a=D(pRsGhhQRTc0=>n0WB:a#+87<.+$=Qj6qF^GTvfG3(VDS("
    "3dBs$$C&T2/-Av?M`.@-40X78*xwd+QhRi3a2.Z.JrUD57@n<:rc-r%ZYbP(?rmp8A63'+#$je3`BF6)xETL;I9L+,#2<P:5--A#@/pY#pwq+;q.1h`-&55/5_5-&okRc;IrE/%=p,F%"
    "&8giLinIV%k3U)*3'dW.1#,MChV/B+VT+:(bNC/D;0S3kJDMd%-mtgh2t0qiO;&'#P0ki0R8Dl-,t/_6L9Z)4ic3T%.cCE4W&Rh;+._K2)@8SB)'7K1,caQ9D.s#77>>g3Ee^E#&g<?6"
    "x[@H+F[V@6lQg@#[N/'+m.3X.GwAE+w[6_%4)>>#Q,>>#.b8a#m4/h;:2h3(qSG>#ExY##0lFJ(3^ZT%G7lV6SVW]+,YSp0;I8a'ZgJ7LWoF?,0rBJ)C7eA#W;P>#Kb>7*ER#r)w*g<%"
    "PDcY#6Bk>7dI5s-OL*Q:4v*BcU;U;pkQEI3Ks+%&i6&&&i9`a4s8S:7E(/e*JNTZ5-?*9Ac4HJCD9Kp%P_dO2VDaO8hG>Yu]7-r<;+s20Q5-$vI?no#[G.%#`:=S&7:u/(Kvm)*f8jJ)"
    "(wUG)]3bp%v'8)$6,X0P=6'5p0D[78-[h8IC0Tv-6l*N1-T,^60x0RJ)6f&,?\?fg#k=w9.61kBuBLD]-/_Ft$s.(v#:;7S@;a8M^wJu(3_o1A=T>YC5[)a&-:_HS7MURP/DtNp'O9Z2("
    ".D%x-e>6,,EI&C#xK?I)=:KH6HkTn&e86)c%(KQ7t],B7]%N;7&q/BcU4&3Asg=%b>[w]c5'3E4B'N/C[^D<%U*j8.q8c`</-)H*NwmfC8X1E4c>29/2<B4&hI-?#,#v?,7,oH)W5hR/"
    "RU](6IZZS/BGeb<uB&.*h:V_+[&Id)2)x)*[&R/VmYIL(m.'b*3%#S/xKi&5iE;I32?\?Y'eaJM31VXQ&)RsXc]o(c+csng)3@WP&RHJuGMT4^,o-E$#@t3xuIXgq#g3h'##xUS_YOI%b"
    "RtW$#]&MuAi2uT8RlZ5/Px*B#H'pv7>+[c=GDEb=81s&-fZ4,2=)-]#J@hJ3wYu##6xZKu&h(E#h4MHpS.&3A*hA0AT`Njh%vXL2X8d8/3)>,M6<rR',d0W$3H7l1&+hn$cCI8%#DXI)"
    "EW$bu#6:V%O.,Q'Uj/CIiwZC+Gh8Q&PmLv#.m'Z-Jo6s$Ybnp%f[g+*%s.)e0FUj(0x=M(@%Ld21nB^7a:rW-Rmp30hBoX-Fsu01'$Jk(?bQ,*kmiN(F%^5'%*Ov@5K_:.lisa*76Oh#"
    "Q7*9'/$`q#Vl9'#9J?W_j6=>f('g1_<[Kj;4jA:'Hf%T%1r2$#8OL*#9T@nAs*&=$FRx$$c$D($ZknZ7n]r/LG353'%rBK2^:&$$:B@)4(E'A,fA`-2EVN$BD<-70<%os$_@Iv7pQ;Q1"
    "hSGHp,.1%ij'JT.cT[0)%jhk&_KW78m1i*4@k#;/?D,c4xm?d)N5>C/$j.T%XY8v-[gSE%`Gf'5Ys@G=ld9c51Z[l1-$9*%@1]M14('q/C&l1^O7=c<5Hr</.3=`+G**+=,U%^5Db9MD"
    ".%nE50w$e<S[f_u&-vC5gx>G4d),##VHrG#W[F.#Y-iS_s[Z=l9sRc;+=a=lafSh(?*Z**A-t9%NZX&#rpk]#&Mg>,ET^Y$+iCs-(Z(^#Uuv]#:%&V%L2Mn&fAQDc*Y>L#SJg>&sp&9("
    "O$jje0=>n0<IR8%iIjq2hVS+48'c4Wj@<D#=5K$7)%=5&g)@51.x+i(;V3F3:6C+3qsWQpivuc)Oj2=BsWbq8sqwS%fK`Y5]XqTAjOKnC*&8U84FQl]odZY,+le6#.pnh%TbRc;_qN%b"
    "=.E$#AO9:%iWKY$(M5;-=0o],f&?G,M_7W$Mkc/(fD$EN9/a#M?tU%MqE:D5iLvgh^(vG3F)1MWbTfFOcmQ_Y5Xw]6_)d`&0?v3'':`@,f(fC4fp-CbTN^lJldIF>W#hJ$3Z4Z#dr-/;"
    "-`SU9%#*;g8T8_%HrB6;xI`)+jJja*c8]-;=w0@0Q9@W&?Qeb4.'d_bcJc/CdL&&vI^Ep#^Rk&#*fJH+x+2$#<#PSAb34&#$3:^#J2-A#hh#C++K]a+Mq9*#dE+]#s5H/(HL<5&:l,?,"
    "vMQA#;IdC+QGG;-Cn>N'Rl/87U_EQ76^)22Yv.E#DL1jjHg/g1n]]C(&U4ph)bXm2qnU58>/Vo_f`8f33sxF4Zt@X-,qf;-TS8W6wS,[-JcR21AIp5'/^g30hn3p%GS-8I.DfU#5Q7(X"
    "'%WHE*VG'[>NYY#C[1wg3eYT#5A$(#BCk[7Iv)D-X%N^#*b5v.:Ou1+&C9&$RN;22R,+&/@dE%?c5#X7w*$$?X<^6&Q5cAc/Q1dc[^`3bV9mQaj;U;p+)R;`iOJ[[ww'f)m<T*^[<Ss$"
    "&nRgWSh)?#:KTs4Jf*QDW=uU0O$K>#%,?C6_hpL('C060Zl3g%L_NQ/'lti(x'Ba#cMmn&OW^U%oJ$@A>;Ur&S0i*4Zn`[&Vl98.1%GM(1Rq/)o2p&ZDL,s6Uuou,0AR]4.wVS.q[4Y@"
    "2F[b4R^Fl7nQ]%#3t&a+oa]@-Wj&g2Wj&g2TEeM12`G,M-dx>MfC?-NC75dM&gt/Mro9KMoYb/MLc'KMJ]9,NMfYs-b6]nL>Uli]$%AT%>De2DGn7G;?b791k5V<1l;9J1@L&C#2<FN9"
    "cJQK+0qWA+a/RD#s4Rirn:MS]v1C?/FUEB#wQ@M9+^:(M^QC6TvBK:]>Wj+VC=b_#-a,,2L#&ZV`x-I.^E6##maB^#CXxr%H?on](fe)MX`U#AGmqMMJ5v2ANjFHpnt[#ASs@0h5W]GM"
    "de<2'Hm+1#EGtd=82@a3V;-EEpA<$SH[JH%:n7V'Tq-pD7M65E'rsIAXa_B#A1V-42C=o9Sxu8..1Z&Tw[0^#UsO4)]j`Fr1-J`j=6lHtR6eCB#*N@$Mxgo.ShN`<jvCJ<]-,I<%L$a$"
    "r=mTU;j@iL?L5L8=NE$6`s:]X)2F<'A&mk:Hm>l1R^Fl7UnRc;;>xH)/IrW7Rg;:0]vU*3F?X$##r-g):H<=&oX2V71_6iL&^lX7rHo0:N'fA6G2?X7i]SU7%lpr603Y1:Gu,X6vS215"
    "lJvK1h'4C#Hn)p'.K2./8F(p7cVbBGLiwGGv]tM7aGaBG'o@m/>KITc=L?t@E`83%Fao0#(FFV.XFL8.$j#Gcg;E.3@-IH3uWEmL/*s(Em<r[,rUpF*jJIh(q`Q/(Ih3n/nGM0(LO)jn"
    "Hos4+`eq^+]3'6&tgsQ/k`;4'mPc'&?;s&vJplAQVBB5#[k2e$HZ,@-*wL($lx*c#s$:FGmBjf'CO*t-DChb*pZ`*<XPP^Zphlr-GB2&.+QniLxoR(#XX;f'ffLlSK1uT8C?IgWgj%<&"
    "#[ZAmV#:B#bqN%bYD/h(::qk1VtNS]lI75/X)lN)c9>C#IiU-N7PWL.9lMR)`u2W8F^x4p[<8<-lYfo7CW-K)u;[;@5EhGb/$3oA>Nc>[>q;D+Glhp7Y'FB.'w&/1ThN`<$B)-26rQ:v"
    "0J###x?0g(2`/;6F,vT.Pl8+*]'60d_JntLKG;pLv[Z##v?hweLL?(%x3E$#Dvgo.R*3e3IoKS.g`LrmGlED*<wv.:R>bxu#H?6'@4ccW@=Q_'H5gM'tAL&=N7HP/aZi,*5*O:.B^`(X"
    "h$@/:w7+87Ps'*)'?%12Q1=/(0#VTu(UM0AH,RL2Pmx9.`RLb*5jKT%=P=j15`<^45WLD3J29f3?;Rv$SHpqLa7A5'TWn,*VS[40X0/K:=u6-)+UM`+mVV8&-u0&4Ws+/&B3%B5Ibrq'"
    "eMU*<PTKf)4-&A,/c=gLvG%a+38hS%)dSS.9vs:QcIH$vuYx$#W^v2vfHOh(:KX0_IGC7'9BF0_^$hv%Ae7G;OmGG24ThV%9QaV&[aYqI&64U.'<mc.SC14LbA;_#Y3c)3RdEA+OBIe2"
    "6C(4(.^Uh1.N%L(ZA<**hYKB+0P2$'`0&_NBN$12&xe5/Te=Hp=A*'M(,^bKY*7m0,Ka3(&esM(6ae;&1lK[%#*2L1?js8&3,++-'MN:8`cYR(n>EI3r;MHmP[Nv#&`_R]6b->m_Hm&4"
    "NT.H#Uo7-#ivK'#.(962qO&ak`&VBo`Ha^%<9u/(>L:T7Af/QASn+T%8e3V72Zh11'Aa(=X=V4)pW$8)D%Z7(j3(;(&BoE++Y7]Gs8Uu$_s7-*$<V0(`/2O'xS:u$]2rc)Y&/)XUl/87"
    "W6ZRK#`a/2jT2hL&*W6pfF]C#.;8phku8Ru.qK%%Frw2AA>$98:#)9/Zk_a4bVuvebmG[$o>[&4Y=,C+Ph4r/^-rN2XOks86F^[JkLVZ.(d^a%:xiY$?]M2:1CTY>I73COY'=D8Qne,;"
    ">bnwuKe#r#N;F&#72<)#:7bY7iUN^#fC^3+1n'7+Rd6p0a#Km(@@Fg5*A5/(BRnUAhcYLM.u0)XmYx)XSUi$#^iX*Xm_C+X_H$k0NjFHpnG;0A(;v23-+3W-kMP7(;w9a#*%C(4C>G42"
    "V(,VJxA^v-hLB+*T_+T%v)TF47hfDtAg97/M;&1(R?*)+.>c`+HOG2(dBGV/[4DG+_dUK(WKbX$8a$k'oXxX%TZ>7&N5GT8C,:x?l]K3gT1DU.'JL+4KCqp&6H2f+Q%P.)g]tY#98U7/"
    "A=vwL7RG.)>ZZ`*[QwCj(NWV-6fNY5Nv]&6lD?4MR=wV%$X,T%>)x;6rut#/>EDO+<2l5(/gt;1%%/=)@I/#.s@TN0K(2K(i`eC'eFlCR'gDQ7(xD8px:^r-].Wa#HBeq7#Z_@.-EF:."
    "aY.7&6oUs-q?`[,c-ikL4:%5)vu.&4NDn;%WEYF)hE%'54.`w,Ai#_+u:PD+HOG2(d<,;/[4DG+`gUK([#?7&,Vp6&w.In&gOuJ&Ib2?@'vr]7WhuYYD%Ts7A>h=@C,*$>B.]/1=%QYu"
    "fa^g2<.VYuERio%k:,YY'a5DaOD<A+3O?M9RaHN'`uJ^+*hvc*o+oL(@=v>#ln?=-ueP;-L3_*#p;1#$XQ5N':%.s$/Yl##BJVS%+])22?d&7p-1lZc#b?F*Jv]b4QIuD4,D#c4j`*i("
    "R`?<.kAut$E^wb4W#U7/A3mVdf`_<17>OT)Ff@N:/j-,<[,)R)LlNj1qt]A$(^XwPC@PJ1W$<aW(G34SP*hZ#)j9_8qGA`tuea_=@sV]+iM`.qaZRS.i#/DEEE@wuM/8$#-4u2vL&JlL"
    "#nVL)?K=',]^4T%bfvA#H#h@#)0B@#(f;Q9(Q'(,)hF=$D(7s$L,%1(j7CU'(3se-lA8n$70p>70)eoMDls+0#wt$i?&0.AG_8GMsm=n04?kT`^J`Kcrn.W-`jF0l$0_:%@4P)aL^D.3"
    "X5Du$Vl/+*A)6<.VcP>#5D+m'>J,;7Aam3D:<)S&gpV+*%8=Q'=%9u(g1h2)+Yr%e'9'e$WKUo0Q%M;$^s<>/:tO[IOfQ>#-qI[%M(?$&CY].CSk,3B@IHM9%/@A#?bme2x/5##R[=Q#"
    "vSb&#w8q'#reFA-7URc;<8`#$tW(0(aiDw$Qpq-2+?60+td<&+Zp=;-mvTH2jQnr-BU*t*^h8;%dH_6'C8UW$3@QDc0eR%MGx6M7(AU%MZ(h.AGpgA5NpD'NUcJf3u*`I3lIu;%[@]$p"
    "d]d8/*o2Q/3nVa4ePd)E=ldP/7<et9Jlc3)(.H_+l)P=$Va7P'[tp1('JlL)p,ID*^XxZ7usmF<L4B5'5=xY8-[$%5F:5jBAbW5&x:uY#NLv?&:]:v#<(Bb,=w<a4GHmD*#i?>#5Y###"
    "_`FDEf)q%4x&oiLhi7p&;K^&=<toM'4`:Z#2N`c#;QDG#3Sl##wi9+i78^L7$6.kh(WpXc=3e.ArP(Z-X8t#&T?gC#+-Xp<KOh9M;<%PBpNuv]xiS=uG2hP&Bppl&vXBP8,+rT773I@%"
    "BOJM'QpP3'VE,j'8ij5&MXrZ#CZ0T%=<lj'n/5##*5o3NP>uc-k1h)5eVai2bG[]43MQ-H;31T%9G&>.0(3r84/IZ8.odu5q-sk<(:3hu#vlY1MgEiCAAA>#Mu7S@nW8Yla]p(<d_sY7"
    "U,871b;6x#T:F*,ihX@7=gK?7r7,(5F[Vg(>>AR0nAf)*PW*&+T@%-DeH5=7UV-E#>l8ph;mFHpFApje+&A9B*M(3C:5v,*8v19/W#gc$091#$]q.[#5^1#$5-:T%kP=j1NII`,i@4L2"
    ".Ih/)nFM_,Bb%.4&n2/).ai028h3Y-f&3222*VF=_Qg7/v8Z3''20'+me:a4wn=31Hp5O06C*<'d,)4'R#Rd)cPP^,QSsO'`KqD*d&VK(MkNp%^_8u.P-`/)gN4X$6>'B%2PrP051lT@"
    "=+Vv#KB:T%bIHfh,YJV6v@uS7@+3.+f<4&#f_Gv,/*0Z-@,:v-mEc8&hZg2'@74O'ln.WcUSC8pa97D(.?Wa%m@0B#jLO,MlU5J*3(qWhn/9o8mCED#WT@V8+c,d3gjS],=/we2[WqO1"
    "/OTY.E'm-&UY,'?:'ZK1/4Q/CldAr.2VE(=aDpTV.(o(#H]Ep#jLb&#IJr6&TTu&#W4NU.k)$4*,GY##g-1uc5BY20IwXL2&N8f3R<gP)-*<d*GDeT%)45N'MWPJ(fwa)+OPj^4YirT/"
    "Pr<31;t2f<vRF314.PM()?<k)'7Q(+,5.v$M7pMBIuEJ2X-2b=.[w=-nJ8nN5c9nNUh8Yln6io.+?d^=Zoa]+eo'J#(5ak'+U/?P^s[2Ngf9a#hQ@w$=*U&$_R(f),DTsMdP1b*-xLw,"
    "EY@Q00.oS/jl4(5n.vw&8h:-Mq0EEGLQjD+Y;.L(u[,c*[((b$C0fg:qdFB,eM<**sW8%#8=7wuJ@Bq#Zle%#1vv(#`kpl&eECH&5*x/_sM^#7/WaX&ei:p.+5%o/bliQarL()5E/#$,"
    "F>im&(5o51j_%tfr_(0&#lso&lvGr%@&il0%*-a3NML8.'0CH%V=Y@5,AS.21(Jt@x*@H#htd?3rHD0h5VKc$*_ln0wrFjL]CUv-F*U1;v/a<%4XZg)NIHg)./`PMeO.3:_T3T@FkR@#"
    "W@fG=KSt:0fqbG<awHo1nct;74KNO8@HMo&.r=M(`d'7/0P>C+_/d7&:*CR1+qr>PLO8*%^MO_5pLZN)0$L60Q(nY#&$Aa+]tv<02;]l'f9MYP:GHg$YMGDE]@PV-+mh6#XU]e]:]qk)"
    "Qax*^-F7G;8]GW-d;m'&H'@W&R8e,M&IG,M%C>,M$=5,M]up._,vB^6%_Z#><:9*#Sk7[#wbkM(G&7L(kwv9.)[W?#7Pi6&XRE5&^&6v$UC_s-g>cZGw?@A,>haP&3-q*#YT<8pBR-@5"
    "QQ&7pCQ6/(I:8phS;v23BR2JG?[5<-VUd=&Z+=j1*t]79W?*(Rq4WO'`YSF4dB>j)%qK[4'?Zk0rO&_JiSv018H^a+BArW-40Lr8C>v0#.$OH4Fo[GE&=JA6X4R:v+7v;?hm`s#DjUv#"
    "b$###]l)%vg+&t#=*V$#MXI%#HnQ:&Y3+9%Pmcn&W$A%#>1vY#$($G*B=5a*;4no%/@5/(PMgF*2<;_$9N+1:2PP>#4BQDcmWO=7<+V&MdPRDcKd(8p:>9F*`_wpcj'_=%a62N(@pGx#"
    "@vv;%J6CD39%k-$)iB6.C)#I#K;beFNtg8^`2_24shn,=/YBa#GEh[u)e.#D)*ohu7V@;$?K@o[&afS.t#BJ17VR]4#OKm:2&h=-AiET%*-GU.#nQD4OYa#5p[fX%u3':%4ni&5fg;b="
    ">%P%?wb@P^XRd`.O]4a*6qLG)QOt5&1jQV<w*$$?s(+87BJl>7:=7AcfCH,3Hr+naN>Ic9_N-hcWGcFp^meA%SvBp.Ynn8%f=hWJ^BKc/3nVa4qV6<.]CI8%u1NT/^.B+*XPL7/NfH',"
    "U.V0*@:R>#8vR=-^epL(/x1i)`TAo8*)_uePuq;$+5W4'[oH2'9imj#8Bh9nu2vA-mffU7b)XY&N,+&%]0&]&4C@,;hj,>HdASJ(4]=I)MS0I=HCYW6NLDM0P]:YlVZZ]+gdFa,SW8o8"
    "`+B>,eEx%#=?ID*lZNI)Q_wH+7H=h:ux*87Y3/f$(Hv2A-1lZcZ9YWk#U-Tc^RTfL$-sm&34$?I3ci=.kS6MTP=#f216[i;fqB(4k`E=IJ`:87F`/t9UgU>.xHS9(OKgZ7$6t-#VMQ$v"
    "]%Hr#^q:]0W4,a7Y%6N)g[p.M<#f.M$',2_p:Y#MOP',NDJt+N?vRiLZN>Z.;j(<-3XFp/Vvn(+S'me).E0$-g2X]+'^@i(K1S7&23M_$w<[I/n:#7pZfe)M^X2-g@Q=,gH;fW=CZbA#"
    "a]UT.ecWF3q8DP-hq9S-UjU9&KA';Q0WVW/GX49/kl##-,>t<aQwYL<K;Y>-i8Y>-L<Zi;<GX.)-4Kr.X*&G>DW8G#1aoQ0lboQ0cBgs-DQ7d#SXq/#pD-(#vuhR/ZNH*3c]Zc2^F]r%"
    "AD>R(4]Sw@oY+5pnbk'%cmB'M46Xb3*^B.*9oR6M<_Tx$?]d8/l>Gs-Dt)`>SLt87q;;9Ccb^q%a>I-)Xhud38^hh14<5=KtQVD+uvbt$uW`D+tsbt$IN)UWAe^Cj$oiV$TtZD*qWmQ'"
    "1xFXVx`7O0IT.H#Yqp:#G#K0M:+$S/CaeZp$aCJ1kb/;%vNatQQ,HX$*F]5/jQtZcPSYI8]tP,*B3PK.-.Jw#3pXi+'cR_#(Q6+<6_i>P?;Rv$;BDf<]r]E-j%oh(qU5c*j[e;8ToD11"
    "%F64KO<ot-bcIP'Kw=Z4F=?Q'7cDE,cr9F$rL,29S0?3'l5>j3i)ZT%*J(v#w/5##6,>>##11/#dWt&#Bq)7'gRRc;8EeW&9?9s$pW1_&5%<d&&d#w-KwG.)wDCU%Nu_0*vNkl&?\?q*#"
    "kZF3bg5-N0>.><k^9_C(bYjkLR;9B3nd[0)?DXI)Q^0#$Tu29/PHF:.9EsI37l]D*kvLl-<oo^fb(N3AYnR4(=V&p&i5a=.K*_6:XLjT/6P#6'LXkn0d(fQD8NfC+bW:'-B/k@m29h`*"
    "3]uY#P?o$$']m$.n[xS#I`Rh%-:Jd+^?#n&PE`0(xMlvG`D***elV[$=Cn1*C%m##Rx]4A*u*LYQbgNh#wI+gK9JC#D3Tv-U^nI-dX8e%NG#G4S,m]#R,/*'<]sS&@4ZxYp0$^6ktU<&"
    "Y$vQ'2vtV$6:*T%.P[iZV+$+*2xq;$X)t8p$Sx%+K#f4]Q8XxuPspl'X1?>-GF/+,glls-dpdgL'csS&OPB=-7KXm'qn*i]364Q'0[)BM*L>gL[o1Y7i[Rc;Q'E$#iq3u-hNgm&HwI%#"
    "q(bA+$ouC+dt%w#,TZ3'<Ll^$uV[h((I@A,Jr:s$--4L#dkn[#f74,M/uKm(I?%12?2^7ps+61Mb)j2/wol0f&bb<8.%DT.?Q35((<X=>K)g*%k+Rs$BkY)4ei?<.;wHH3DiaF3O50J3"
    "$&M:%igj5AtH]@#khOb+O.n0)bohgMt3(HMoXAjKvImm%?Www%d^:]168/jo9xp?\?OT1G+OuUFE#=vB46V[(#.]:vujeDW#wf0'#m3Vb7Z2AW&MNQX/hURc;8e3$#:N4/M0-52_rI(?M"
    "C2oiLjxf/3jfq58Q7OG2EOV%,[0E;$)l3[$(7:0290Q(-RFbn0/*u=$7pUS.)^V8&G2bt&p:EDu=3A5hRk8B3`4AC#Z^`WkLITb*A+EW-2A[w')9'<-I44R-3BsQ.$3Tv-?N5W-hv8kt"
    "0S*Lurw_3t2%398,h=F+k>Ab4Htr@,xEW&F=jaOB>tG7&vqln&T(tT%.c*X$+lI@#ticX8H*g^5m3XT%vq'H2)_h`7YHr<//.SW.PM%`aH0R(+W),##IT.H#a3k9#t=7Z7mxbx%qp+X$"
    "fWea+5Wox,IfM$#&hKM'SG.l';gc>>CHjacJ'Z@5l^.4(H]G3kb?2@5*d45(3sx&3.r'<91D&S:aY'-4oPq;.hBMT/?'3W#I0`50h0%^5U]10GmKGR0iUWc<:MVZ7TuMk0GNAhC0oP-3"
    "@M,?,k7QmBK7,3'St4k'lxLkLnJme#l)Ki#/B%%#;gG9%Y/mN'Z7dERv(087N;>dBqwVm/^141)5Hrc)Ru29/vBo8%C.@x6u[=)4iJS)$6Or[,iqiR/gWI=)p#2n#Yd'^#tpIuu.&'5]"
    "-l68%3W@W&34e8%*A.,Vp7v3#-?XDcN3LMhQo7^.jH3Q/Jc1p.ZReL452n-$0*BsN/MZmL^[8;/$_(?#=;)mL*dK%#]jxW7:;o^fsq-],-(l&#)U,KLI+k9)0BdtL&gCUusC+F[*vU0A"
    "t9JC#J`5J*7_n.'ntI8%%rSZ$'_D.3Eu5uST4Tm',>]h(%QDk'>cYs-Ek+JE<1_^#*?u0;g;DIP0xu8.nreh(eB0hP+$<f#dg&i#ah1$#Uqn%#6Z9E+`iN]F5<bb+h4jo&uLVf:4Z`4%"
    "'Bx>##OvC+skU_OZx=^O:]EQ7o[gNt,[FpL4$?0h_W?#fUR/&eDRjO@a:1^#kV8?(L/5##d&V?#,jn>18_;V7*nh$%YjPn&b<(TMc<MDht9JC#>a,X-)n+>fc<aD?JuPEnOA%j'8,.g("
    ".6%O=xv4X7*@rNVUi=?,2Nth:^V:i:o^&h:c(:##DiGZA_)6/(-=QTc;'p8.*IMH*H@_GM,`0R&]U]I5b3#7jlg'c%LMDnS.trP+_`pH'dpcs-w<Z2(H$?FaRqEx#<H*=&G[Fm#`IlY#"
    "aRn-+cn&:'<g#&OZ&<0A2Ukm($]wPJ=h_B#^Jj.E<G6c*MYjfLne=u-@'=u-^j+]0Z0-KQ^e$##EXI%#DloQ_S=I%b4aRc;fpu2ven5^,nG0],;qH#)<@Q()Km1g(<67Z,9er@,/:im&"
    "j'nRc&otgh&&?3(oVRx2l&Y98L<Ma4:>`:%9^o'0sIoB>.0ch#*N=45E@^v7w8^Y#LIE2'32G`j^7pu,q:e6#8Vaf%2$Tg*BXfb,9J%(#@ogd3o%4a*u)U6&a?goS]_kUkjxTBkG[V*/"
    "=S,9(Co1[Tc'+vo9@PcM;#`Y-HUlS_Ntb)#Ifa5$4e''#Nlls-0;hg*Fs)<-&u>I:b[)eNJL0hpvk?>#02t[t$]GDEKf`r?KT+V7#<1d*7mcG*.dv5/n$pi')1DN9]k.@#tO>;-oI#m8"
    "U/(?-@:Ne+H?kh:V%aG<&w]]=-G:v@#s'*)d,EQ76;eahlCM+gbTIHp?mSL2P]rB#^GW(vZj`a4mc[ft985R(Z_Y)4v**2-FX7I4f2TW729LY$_U$9.fHHU&jYYm<',@nMh*OO:VFMh3"
    "R;xH#13`o0m9#ip;G4*vGTk&#bK;8$6w^^#>*MT.e,92#_8<x$K#M6&)klF#TP/r0ElNBol85xuE@Bq#jFX&#iLt4'?7L+*w0H'$xHXM*PeJM'4Nkv#S6q*#POIs-'/]-M-A6/(_4rXc"
    "Wfkx2@J@&GN.f4;`T/[#@#G:.;d6Y87Bol/QjI0;t-I)48lu;-jrK*+O>7v$i$Md;j*+r$XfA%Uh^Wr0P'x<$ea@1(u#-bF?coR&2:@iL_ebjLc/Gp#ONY##Y&>uu$cc8.Q8FDEVKI21"
    "8Fh#$xd''#-(uA#F`I/j[/oU778G>#Werr$=w2a*/]Am&ke.<-@[gC-Q)HZ%GV$##)M:;$PPYxkZ)su,K51A=TTVs/qhwl/jrD>#$i<s%2:x:7Fe;V7SX<e+RM<5&Uhl01j%An0k=oi+"
    "Qd>/('Wtd$YT<8prp@&mgA6/(r6&3A>BdIu-UM0AfYK$&QqUN(ml;C='*.b4$LR+4&[D.3rI*(=L;2N9MW>M1oG;l0JXQZ,bR-_+J7Is$u+;D))Gp,&gi`4'/8WDbA),1)Bw]M'cbUk="
    "oxg#&T_(x5Y,5@If>%hq5c.Q079]lLCO7$vA1Xj.[EX&#N6gc$5WZN':$r<Le&Nh,Dg3H*8^W9)1rC,)m@2@(KG>/(3Ew8'p_-wPTqN%bCW7-)Z(-K)?07Z$iui2KMhaP&[Ph-mHxJQ7"
    ":`%N0N#b.1gJu23ng<@M3K6h2im/C%Pm3jLsC>IT&9E(?bTw%RJWRP/aoFDELf<(fgtbf(A1SucTH$&+iv'B-Ke.@J4='5'/9.w%LA)v>m.9'+5tI0:>(iZ#su*87`*Km/Q1=/(Ue%D3"
    "+Fb_$t$^:/vBo8%0Xu;.Q4qb*CFGT%/?(B#a8<s%nYYm<V)sx(cWEZ6;p<dZA@a;$p4qV$%PWI$aN:b=O,ueG)%6K&+-92#snGK&5^-;%Ad^S%N3ihtMlUo%P5`+9O<JT&PRno%j[&'&"
    "Fw_FrO^U>5tPRlfI-M#$fx:i%KVaj16elF#+3/KWm,hZ$uBH.#W1%Z7A.9*#IL`Z#/+?m8(,a[$+sqV$)HQr<4I&m&6`:Z#wmwu#jAT2#nv&7pCMI14(^pW$KBF:.w&EC$l&0@#,Lu`J"
    "9Q:7X5<*xtimN4SBudW-V=P0P5R$iLu7.r8ACT6(NqN%b/Rcj';FFA#f53-M5YI7&MH@$,H^iF*(q@>#&d0[#<;vp%:]EQ773`j2+tVC(-QNFpW:ce)S,m]#<3%iLBF,hY/,`uAS$/s$"
    "vO+hY%E,QqvAADEbsp%4#/V;$sw8*#QnA.&_N3p%3VHR'?^Rw%%xL/:)4D/:J0KtUOXTfLc@mShsl2T%WZAX-Jl*F3b1P7KTQuAN1@Ue#Dm)MBGk]Y#857S@:lwE@+?<A+'ah6#;E#i7"
    "2[Rc;f'E$#rW2E-+O#<-B]V6MFI1T7;k?v$%u#Z$$l^>$h_SW7G*Jo8VA4GMdTI+43B7[%5/`H2+NBw#]eKM($G+?#AWI@YbhKR'#TpU'N=Zo+3ws@#5%bp%7=^m&vn7[%tA;=7q>9B3"
    "a3vmoqfDHpU;U;pTMwih5LZg9o#.m0G^5Q8$OU&$k$vM(k_Y)4<7h/)PO[8%EL*U.j0&[#U&-^Gdh97$W]Ee)eW<U1l0CT2XZotugd=l#n7vM;$7'h*o_U:vQIg>%TMY##K####ZFFgL"
    "]gnVoiRDJ1W5Ba5l<+9%,n/F*X,I)*:0'_,S7Rs$eps-*$Snv$D=^k1]L*T%Ddox,DeqN(=@n8%hd#V%;fCZ#4g(P-G#=MB3SW/2OQ3oA3bBN(A[1F*(QI+307C.3u/41);AA(4WIT&$"
    "Vf=a*iZ#6/wr;O'(N3jLiPN>/@B0A&to`4'kVpF*m8v0+xG?40&I`,Dr?;`+c2&_+h'M,D=fi50hbr,+MBjs7ZQX(t[I6A6bGnaP/;$F>,Fva=%9q[u@p*b+3+;V0N0`o:f63A9)p6I*"
    "MA>]k&Ux(3+mh6#B@[q$--'wMHYb/MZ0E$#t5-F%6CMkL9T]=-5a&u&'AD$#sHniLdZVT71NE=&`OV$#lI8R9H>1k)$*&a+OWmg+NX8R3-O1N(%wLc3U[sX.an320,l)/:r#dc2?$@4("
    "39UH2'C@g:S&KJbJn,@56&_e*XdGH(b=4SG50@m0:+,@'pSWL2)s41#HY<n0a^C=%Cv5*EXku8/MD2#$nDDT%*^mQ<V)XI3L>(?-;r(3CmH].4naHH3hCeoB9'gU1N2`50pq#-3#xao9"
    "%cwu0E]<W%SO>jBN+FJ2HieN:%(Ri37kXx-g@1b$UZcj'Ag5O0R),##R,>>#W[F.#Y%iY7p.S5'lfxm8SN0N(DBq+Mdx6R&5>7/(I9Yj'FhET%pBu#%e<O(M5B26pcU/T%hsnO(S=T&$"
    "max9.GCn8%Sk+L1fJTlBYRE91iYrFMvYw6D9W>N#[9juE)Xbs-t)(69m,E%$V$###Ss,U7D9_c)D-Ka<%L`39Y9eM1Qj4snCS1s$)K,eZC*`Fr%q-eZB(*20rXZm8naa?#'wn##UB#r%"
    "A.VZ#8KAC%wtM%Mc5PTuG.H4;dfxF4A_xZT1I5Q'QS/B+rKOt$/lGM9Vw0s$u<kk:.Uv&+t5b78T8S4S45+;J)RF13qbvL-YeLH-$*.*.DG>M9XTnxcWB.m0G]%a+.Dofii$]<Ud>-na"
    "tre+M$]qV%OMc##SN'@#eBPG2.chV$YI$mf?#>/_1c'9'56V$%)wq?%>uJmAUTx4pCg<m0$*[.%x3p-$80JfJ[mCDES7KAu+F_U:[ONJi-9*UDx/hbIlVHbIR:ow'2pI0Y<5Z$T2`KnN"
    "dL@$TXrGDE3I$299t*V7Exmh*QUoM)1I7a+Lvk>-R?^#.kpn`*+@2d*7?f0:8PYY#n+/#%hVcTc-V[TcXWFs-l?1GFh?`C?aftK3Ow6u6M8%MpQA:Z-V3Hh2%r,)4>9kU.?KEh)0n/dN"
    "]?QQ'iNXt$QMA_HLfm$6k>wn&Q+IT%.L@PjBOX7<,g_$7<5>##(8>/jqT42'RSW]+C3DW&t^A%#j=hJ)@m01_tGW^(]uj-M;geeMuq$E5?/>7(8tkU%3nMb7WpU/Lq$*<$e81Y$<>L[#"
    "wABu&$s/m&,DpE[@#sx+x;,#'PjUg(aclV$No/87Km8B3&i0B#U;U;pP^A,3^/B,3hg45p].BZR;c:4)L,tI3xCXI)BkY)4U_1+*W[Rs$g[NT/eCd++APtf[d`'b-Ij>Auk=Rw-x1av+"
    "W(GrnbDqT9r]Tv@K:.T(W7R#@?Q7](,*>>#6Rch%WnKfL,0BJ1Y?@ci*G8R*w7t80+]nN'Te+#5)[lb#gRRc;E^HgLcb-##lm:H2#6tv#^>2BQfN<E5.cnR&Fc(^Fx-.Z#?@`Z#5%no%"
    "$sLZ#40i>76,e<7V;T.A+uWGuT-j=>vccs.k_Y)4fwxZ$)XTK.,(JfJBRsdu>?Yw$2qoAZ(9]I*k?n/*PTJ?#4D;U%.P]X%sL^l89_p#*Hgu2vBhRR&QTrb*6:.g:/@5/(P$kT%&]2eH"
    "n?wGMf3v2Av/41)5WLD3EU@lLh=Z)4QLeX$fPsI3ir&V.+%UN(^[P117@`-4tWn1^LrWS&XwIJ14:,6$tis[?8(R-Qh<m>5hG2>5T3B]&Ig10(,B[@%=/I8%G8^>#MIYQsY+eS%xB#F#"
    "a@=gL7&kXchj<3tONK0A?d*BGNfXm2B41Xq9YdI3;t^u54EGK=EK+Iu9ke[u?@=,M^btxunn`s#H)V$#&B422]V5D<5_fi'2kKb&E8>/(`kUg&=4/_J3id(%?n<X$(,a[$2K'#l?-Y/("
    ":`:Z#O*BQ&'?o>I$BuC3R`6s@kbW'/:_@rh-tKqM$ll;Jc`u[0,k^/BWC't9n4#$8jaKhPFao=#7=7wuuk)'MHj^j(xlQ]48B,P9+)-$&aOB/)rRbw(TiSY,VcBA,>PBU'v3#F#p4<P*"
    "^s5/(S'+5pGURs-=6cO:KQbQ';]d8/2J0f)b=JT%N)rS,S9>A-TX6o16P=TJjKAj0fFf_#'35[*c7$G4tJ7V%VEr*iP&.F45rvu#WRL-Q%_R-Qm/w%+$T[-EHW[wu[S6U&j+KIMpk'`$"
    "bP5V7IFwH+F-%<&X.DK&cQE?#XL<5&fMBmfj#(_Z(csk'OBRZ,ejGV%V&](+GN:,)KdTs-=3Yk:VtNn0)l]#ALIOBk3*jJkB6X_&J3x;-L(ju$+O;8.`sAG;'$?Z.Uc9pAdA:&ej-e5/"
    "Ko.C#Da$BeLV(i)CoJMB2W%Gi=N7/3XtpUdTL_Hta63$#-?L;]7x_._C:fs-O$S0:Ra0q%IJu;-]]:a<N;a&+NKe`*/D%w#2'=h:2Z(;?\?UL6pimtgh[YP4(DNs&3^o-qpk7ZH.0V<+3"
    "bOAT%#5/$N>&?S1nmtsu1cRl'B_C/)/L1#EM;iZ#3qG*djgsU.Q@&(#N#h#vTEvr#lRk&#r['p&+FK*#0W(^#-6>x#emOh:bK#n&L6kx#b2T2&u`9D5a]1W-vQ#RN>D9CcGIp[>Ni><."
    "hCo$$$i9mAo<Mj9m]WI)?#hp5m%(j()#K0<#Hn2%[D#++q>^=-Rg%b4+_6t9s/-nChBZ3B?%,T:x[T>.b3mNBPa#R&hqIm<h&^o1C8SY,E:>Gjrc-<-l8q'#V)S7#:#W<#4'u2v=I%R&"
    "w72A=Je;ofMbsx#KY;:%rit=&p@O2(vE>/(qoAU)>],C+(;FR0<1kP&7l7[#CH/s7<PP=7j#E8p69R*12RWkh1n@t@+$@e*x7ds-t+&-<[Rsv6TOr_,X4gb*S3(U%TVu9.,@?d3DDxr6"
    "(6a&$Y=n[-^32N(PPCK(_ggq%BW_g(at:Z>em>A4h]*F,@xQ.>,(HDE]-X*6uH[9KKb]i3.E@(>)_Si3.K[C>93CG)A1Suc[^$&+,GNY5T,eV%(.c/(WOd5/Lr.r%=(+4'ekET%R35&#"
    "Z?Z;@Hf+5p(s]C1LX]khUUM0ACDWL2$hOG/N_Y)4GL7g:W9P)4,QL6agKYF3-V39/pbv>#X%=31r-Qa7rJWLLDYZs._@.[-(um(5FgIH)D3(a#L&R8%S=]Km'K?&-][&H+RICf30pxtd"
    ">,Nn#V5=r7C[?T.ef5s$hE%##M=dl/;IToeLC##,]hXlAJdnwulNt4'?E[;-6cuS.,e1($OwfG2ajS%#OY_u&2G#i7Jh70:1['t5rdp`=-Ytx+P;aS(4kVP2G4M#G>[s5&-AVj*B)QT."
    "I$U*1GFe9g`NJw#*_@i&56Ka4sq4D#=Yxq&/hWv-seg<@8m%E3D5^j28$h1;J#4_.7jva,vSk[t>BCg(4o,C+5)DT/B'Iw%c(U`5@)4D>.mdD6#)gABGKBo'6]cYu>N'j(oOV_,Ij/I*"
    "Kewu#`X#`s*-Xxk@HCG)7jWuPqIhT7]_26'gRRc;+]5<-04ip.<=a=lT[i^odNUS]c^M50sHPX7ORV$#xKw8'&Io^=$m4S1E.Pg1Sh70:I,%r%IQn1(5mcx#;KO`%a;eS%(9J9&qoi?>"
    ";1^&$<@el)WT<8pNu6E;/KPB$bR7%b#d86p%GM<-#lau$l%r;.jNom%>_nt-6YTN(71:p@_7GjBKoQDa9Zb6/+Po6:N/u#-F=TZ5tgun0Jh5h2?:'l:JLdG*nr`O'DV`wA[k$],2B+%,"
    "X1vY#_xlR$6]c>#I'%<$84b/2Hquq&)^,8/&2o+#(<M,#:Z_R#TCP##oAg=-AHwC#2rF/(]6<v#^`HC#)K@D$6PY##<eA85iRM0AvfXm23>0N($'lA#K3ZK#OpteuDv];/=bnwuP2W'M"
    "dKW$#Q3rS_1-F`<p.op&X%3(=t*+@'BTHN'@Ts&+gGGj)H392'H=Y#%UuS*@00ic)D(=t$BSpu&JZ10(3s($#BBBkFh>XDc93S-HT'j.A7Be.Aa4P*1iW[Tc<T/b4x%8&4HH/i)_TIg)"
    "kwIw#1RLb*5Gol')5D=.d16YgQM3e)J%G_.q&>t0&&wANlb.:-e<GE,.GP:v8,A(s/H9Yltku.:mf8r-c,0I$ec>w^EIW'#*g_m(_[P+#44B2#M/j:0Hv'V.+e<^,HRRw#+&i50XK+t$"
    "C;C;.>ZRk(kj4&#KW0_,luRH)1v%9&[9'u$k@xfLlMN2M?[,@5MK%12@rJ,M^2QJ(`j)22<OVL&^ZNP(]3u2Ac7JC#$F.J3Nn/-*D>m8/qklZgt:<T%jxeP/9a[D*cHki0p[0f)d8WB+"
    "B64:.9i6C%FS+'.pX2@&8:(N(:q))+_HUd)h_^mA&>*+3jHE%,7(f%-@c3B-J0':l#in#-UFjG+8&kI3j#/f3-Tr7#WYd$vxjFQ#4G-(#bkgq/D/S`=2>m&#a2.)*q+.H+S*A5)Qj7x."
    "'B))/7'.)._,U#$h$>I<+IQ$?g:S_QFgu2A2re20,0,D(UQ<HpoS<=gY?o8%jA><-Z%2e$FLD8.k<xkCR.[)E].VL3;Xo@-Zi;9/DPx90EA5H*AAa41=4`@,-;K@/x_L[,q6AL*Z=aN'"
    "Th]:/C`8L(:3ja4vD`k'g8#(+f&QR&[#@M(QBtT%8>>>#c6%##^4Gfhoo.;6/%=9%IjO;-tv8[#xVHrL+mv2v=CVq%48./(@eoi'>In8%wDCv[P=D*%b'_4A-64Wct,oO()0xF4C2eRC"
    "h7jGF-6ToBOmiT/:QAfh/r(QLB*qB#B-cs8A3(A=#1+Q^Q?E5/aRj?K.l^`NDe2:KN/:a<&)G)NJ@6)MV4T%##OBw#.hG'=O9<x$&/HR'=,0U'C-d+#'qLG)vuat(-tcj']x#G*I4Z##"
    "RaKq%DhNt$.K-a00>O22e?-@5V5Dp9&xLpf#<R*11H[08TA(a4@4gnhAhuH3e)f-+&d^02dOl@5#ZgZPZ?Ck10:VK#OeQ<08iGG4.n9T93Ie:73ebBklEHB#-s<%tgUi'&;8PV-RoK/)"
    "HX@l)9r;U%f&Dg(;cc##6q=s1h8OHpJ041)eh2Q/oM&G=t'h)3Jg8t#').M1vWx9.jWG3'aW^`3;P[u0Lk8e4b1H_%]-[:7[HRD4iJkxF<TZ`*Run+M4U#&#rt52'4&fa'-hCC$bh'69"
    "9J:4)Gc%T%?PC<-*x,nfeF:[,*f0+*_8@L(Rf[I26?/Q(PNFX$R'=x#%&tx+t5:u$c,V,)dTXX$Y'b5&Nw86&[Y3hYS=Y@5PSlo.JmtghZ)AambfU#A;KNP(]]lS.=csSh2ud.%)-1#$"
    "^7o]4Z(i5/.oq8.Xs3jLiFY.Wq*Q(+k_>c*X7-C%Au:Z#@)`$@<M$,$DhE9%LGJ=6IJ2Z-Xx6j)iKx+HD?'XoRqhm5le'p29Q>#6x@P4_[cUP&U%>T'7>Gr'sLVk#1uhv#:tYEn'kYN'"
    ")8rZ#@+96&=:@s$a@cB]Ls`E:rJr.3bH]=p.Y[[-_TIg)JH/i)kUfC#Zv]_J2G9j_mLnd^c`LK%/7dr#&hc+#UZNE#]wd##'0DFax^s%=BHCD.]>OG2W+qGa*wA&=L0e&/36V$%&M6hG"
    "-%[>#0)65&Xlv2APYK%%2%]#AR2Zm2>q5u8/Ew)KL.Efh+Y:TpGwdA/jT.H#^^A(Me*n+#xWZ&%wwp6N?%l6Nx:5kO8aaQs&i`i0G;5;-`E.@$iB.`$[sqV$065>>hpdT-mfFK0HV;/("
    "+5ITu;Cs^?UQZ)4W^7gS6>(>YQ*;&G;<4?/;BiG#Vw;'M;H;pLQG?##DB'b%p+%2^6U/2'j]#;H,NGB,MXtJ2i%oca===Z$B;<F&%3n8'9xA2't%cHVDG4;-D_cS7f(blf+*KA=]qN%b"
    "W%qb*1hX@#[Z&K:$T.5'YV^^+;Lx7&3AZt$*2+87(xD8pHb&,Mh6;BuvO7jaum.T%J]-9(lCWL2Wm?d)BiaF3N29f3O<x9.jJ7&4$2f]4i+@CF[uPm8C$Bt.L.c^.U;KpA*QxK)*Ggj2"
    "q&9N2u4mt7t'Ed$9QeM;eTp99]Or=u+6sP/g,NO14fO<%GGso$^pk+5`E>A-u+#S.[rl=luiP;@-e)D#24/PJohGfhd#q%4/5-##4YNT%xBFr$3Y;#G>hAm&Gkj5&1m[`$6%@s$hk.M7"
    "avo>7<Z.Bu>rI4f2b6H3_8f/&<C0^GL^<NF$PuAN5i,VQ@)B>#AUV0G2jR$BJI&/1=n^6&C4o@#h<T@#pGQ7&Kqk0(Jtj5&OmrS@xors@KROTuX141)l7*T.N,tI3woNM%*UVFX++uM("
    ",ramJnDWT/g)QA#f3cb*:PlY#b>_QTK2ps78VvKGW=rKGp,/[gB;;]7b.H]F.%no%w@+,2m1)E<;4IW$OG-G$8af._>-4H+P>s*G6n);Q?<J;$fJo/07(%s$8j(?>_jv5.QQg+9DD2s@"
    ">;+_$t]ig1)`OF3=Ab.ho>[T:]@06M2:Q:v1G/?-,D6Yl[s4n'f]NE'j[Rc;?ot$/E?D$#ZHGb%Q>U3M>GD-2uIME<Y4V<$m5&b[v3t9%qx0vuNb2D%B3VG%8v+?#`Oc>5WQ'^#t0&#G"
    "8t2;Q:t=/(AC%w#:du>>da,UVS0hRu)`o`cZ)&[<<K2s@1Zvh-g21lDEGgc$7`mN(:?(B#OKpBB[2^V-3N8F[u4b%p7wbf(E<i.hF/cv5O9F&#5c8(#gRRc;5(FG)VA5N),p&U)ecQv$"
    "h+_],Xl(s'dZ#c,.,Gf,dAv(-#*F?#OUs:%Xlv2A9Dms-`9cU=YuU)-%1:+**)TF4P45d&J^VN(ZUfC#-A^p&kuSB+KAAqJ'TBW7k[pc3wG=`>O=1,Tb:vY#Vem`*##,40L;L[6mE0U%"
    "Car-#v2*jLCTR&Mk(t$#]Kb&#@C<<-m,oU7#S&rI_@:p$]B7[%v&w4*v0re7tH&r.KAW@(b-,J*GGg6(w5`,)rT[>#]]+D+nkUW-te8q`&R-@5NY8g%36iAuqd86p$=4qK@k,1%F&[J("
    "s*^:/vBo8%nL)<6=Q=2(QWw,*q'4&-]`0pA]l;KO&jrg(Vg>R&9w+,2fWJR/ODU[6h*bT%q]Ti0Vq_#$RipY#G/5##p/VP-m$0,0T?O&#l>m]=<w^@1:$S0:&W?R&Z:]A#Zdc-2>wI0:"
    "%low>qdNY-K-5,,OwOoU'[xf.HuXL2KvPp%'31#$b0V`$o90f+Z/LB#93%O(5]=I)o#7d)ge@1*.5.3:CA7(@#F`UJ#/(j(O7i;$k]VfLGcvpgdEs6//Jg4;Z_o:d<TZ`*;=t7e)Wsr-"
    "0r4Q1FTm+#Ig.S[JrKS.'7%9.0Hg@#ExYV7OA]nALJ.@>k1,Z,.-[s&7,o`$n0CAu7(E^?OGo3%F/,D(N)(W-B^.K<Kcp.*2r-x6`&[)44p1i:#RXs6YwMT8><s'>ws/88hw'o0#T]Ub"
    "1H:u.jVi114JDVAdQQT6Cu^5']hfj','&E+;`Bt%Xq]3'4]b,#jEqhLJtW_ls?,)*f&f%FWYAT&r$s.;XDqFj9Ht',?c.D=/AdZ&hfjP&2G#i7Jm=1:A1XI/BQ@5/6j$9.M*S0:].F,)"
    "]R*uf&0(l0W(4x>^h9#?J$f[#n%+87AUC8psFbA#f1mea9a*-1=:R*1+xo#A?#ht@]_OF3lr4dMavHg)%BYKlh]8f3C0Tv-]]WI)gwC4BL0D3;f6m2(hmUS0J4Uv5oitU&VtET%-_q)N"
    "2JBfhh(e'.0:*)5O:=dEYZHj*Sw9Q/_m?P1`CkS&0D[M)A1Sucp0gc)0xFS7jq?g)3J@@#u'K`=lBI0Y`Tn@%FZC;$(Br%.mN'u$V1sQ&lo*87A[H,M$pr6posab$Dc5x2A$Ts$efxF4"
    "1[v)4g#we<6?sU0h/`g%`v=OqVNZkKgIa112egk*vsIO(4^bQ'U4_'+@0XdXZ1?_5S(2x5I4CD$6PGYu)]KT#sh4M(E*4d;tsB^#nG%*+QeI?-*[*X%C['##5&t5(SO)m/`w8a<V'PI<"
    "I48H<.*3e3X),##CT.H#@vF6#K5W_7$nw8'Kwt$.':%<&$(1=)Gw@@#:P[N'^]Jt%e9sm/1kw8'DQrB+djNA+SUVM0sRtx>2b<q.?DM?#Y@7T%3iU;$iq7M7=8^L7Y>XFpJmtghX[h.A"
    "@ulX3Th%hh4H]s0Haln0k_Y)4-#Ui)gq]8q13Wb$WkC+*I5^+4q/@T%uRHc4Ww(['vout7b*&d>34,i((qWq.5o4Q'aQUG)krIF<rZGp%P_dO2,'iD$EAc;$DEDM8hG>Yu_JN`<KN?_#"
    "Vx^^#Z.>>#+3NM,Oo;m/J%Vv##?F;$PTH1%9rM#vHC9U#&<F&#BUhX%vgD)+dd<&+SfvA#1N'@#@Em+#]VPj)Wm5,,j9:V(F=28([p5;%3dX?#.V-d;PQE/2s4[m/`KjGcpSM0A)s#s%"
    "*Pp5UiHf^&T+Vv-E6B?-B:?q0h=:J)iSsN;msMf&wx$p8`S?f;M0V[8hK7x?RadN'v?-*3_]Jh2NLv?&HRd#@,baD4?0*k_ef5s$$'&##GoTS.U>rlgdnaw')'Qx'gnD5/Z9iNt$RR<#"
    "FZPm#xoM.qY9,o/*n1c#IjdF#xl,wJ+fR$v:6JVM5*hqBCHXs624k9#m&U'#d*?700lr$#+qx>#L?mlKTpY^#=iA>('94wP)?=wP18[$$Eg/T&Z3G8&YCW:%1uI9%)o*EMC&O'%5@c4A"
    "&:B1gM5&3AZ9bi0o.'J3pKsn8&iC9L;$U&$)NO1)BUbT%ZfBR'tRi>7ibn%-sB1G3`IQA6vFSP'vO%J+Z$tT%[uo>,g&$$$Fhl61HT6R&j.Sm*>e#e5SNZ&+/1U*b*@8T/dAgi#LRZY#"
    "GYA(s+4hj%]fho.^s.K33T''%t3(4+[PF;7_=<#$PCI--IW5v,#`m$(^a:((uY_-21+JF<5Iur)>8KJ-Xo_0_:_r/*D]c3);r90,m/d7&g5?7&9pJ,NI=[PhHv5[p;@dIuDFL&iuYm*%"
    "'qZ9r.W<@7/,tm.uj//=vZW2/.nGfhWthkkR)>>#=,>>#B2(n#,25##)mTs$=@DP]5wo7nO;Tx$ik6$#Edc6#c`No7w'_+*06+v#>DY%n3#PZM-?Q`ML8CppJ_gDt;KIL(NTT3*lsJwu"
    "WP^q#Of''#7U]v&R(TK2,elF#2fXd=?Ns;7Ua3M^[cUP&qj/H+c5%u%QnA.&Ok;?#3K7`$t),##IM(v#u-U&43-;_$H[ws$6EDG#2lqr$hSSDcB/mHtv7_.AfuU0A%vXL2,owT.sS8$K"
    "ulfv$'kNi0DAou,ENe+i[&,/1JqGS7Ru.R9eD0W7:dL&GV3Nv6<^,T%>3HrL99K/3sfS5',4P<%X_O0(jx`F&lPsGhi7'hh6>,>gCSPf*V2q5/NS>c47(vM9C[V7:,N9s1Wm6l1^^Jw#"
    "@]WF3a:kS&jxlB,o;K<7aSdW.01@9.c#&-)exf^+HeXS9St;l2WHPA-5-I_+DB0;.GI'S0rR<@*72Ui)exYB,<qS@#-^@d#?\?u/(oq(`+?J$0D:j;&7JDb`>wVZ<.+7nT.upWn8mIk^o"
    "hg8wuJ@Bq#uA%%#*O']7.sd+V;*&v#oEEC,J8K>,d/M0(8`<T%BZ9@#,Sv&3I<l/(t6]0:J+83'2.Bx>RUG6s7u8Ru2rXh(,<v2Ata^5/QEO&i%GEa?6)d<.IvsI3N#G:.Lfb[?tX_s-"
    "#hV)=Q0EWJc_1-j<jv)*([M)+fgR<e`MEQ&XN0I5&fRa?t)L^#,d[Cs@g+?5GaR%#2It[7'.T-VE);_=VA((,I29#,[I*T%9fR%#hKu2vF0F4'G9:p&S)i`-d.Fx>Nbj5&J_e1*goDaM"
    "T8o^PJrAO9fHD3;nCo-)Q)rT%pFCWJ5utn/$2p/3.;vF5Gm;8,0+D%,mlM8&hRB+*r@VAelDj5;Ttws-fbMc:)mX&#Dh<6)gsOP&wVU`3?Rf6#]_x*^Gn7G;,T@h%PG<k'Z1Ua+fn:mA"
    "^:Zv#K,SO(afC>#hk(7+:$8GV&;w^#P=Sq/PL(J)?5$j:`MUWH@+%P)ZVU#A$iiJ:0ab&#OGh'F2(W1)]CI8%JjE.3SwC.391<v#x1f]44))T.vBo8%ei>g-SoLV.RqSM'&cl$,rG;k'"
    "Q#w)*LQt:.PUE5&,]/:)xA`O'jaW:.JQ&Z5bGfO#$q#=7oZ>3'p336/ZQp[uA[7'5)M=,M0kI9.#w%iLhVGxuGIBU#oB%%#;j/V-Eh-l$99@0)_kIS/,sFS7Yvl.L^m5A4oO(>&lB7K)"
    "D?XU.1urM0Xv/'+uf,Q84Y#+u]<$p-Th<Imb?+T.h?7f3;72I0k%Cs-4cWF3CW0Rj[+e'&2XvcMr7#Gi)@J:R3Qi/:VHPA-wT9O'U-^q/,#gB+v^7&,vK>K(5%k317HCQ&qR1[,*R'<5"
    "_`@W.0%Rc4[Mr;.d%([%T#Jl1KZ(k'[$&W$E6)o0.x(t-^lRfL0OZY#((krLcA,[7%^7p&m`8U7Av4#%JXAm&91Is$Yl+$,?$+B,S`Zx#V6Bm&<i/QAl^A<7Ni;/('ER`(w2PO(6@-@5"
    "Z9bi0_)UM'Qk0^>nK#B='b*G4*^B.*5Z5<.VcP>#:<^b*F1T6*Sf3W%,0gb*mJ[H)]gu/(*2sL1ES<O'Lq<9%_oWC>=O-;9K7.t23t6:MANk,>1-2]7Vr#R(H)xiLu%O$#&2>>#&<F&#"
    "aDQ]#EORc;CU[<$<YU;$-e`<%24;W-Vts5&VVAV0snVI.I?%12&jpM(LMKn07Cn8%e1NT/9a[D*QXAv6%o^wLh>P#%qMFs7w%&*=:32H)eT96&XHBq%6ceM1F<8e*bTKU%I^T]=&GM^?"
    "tmP^>grMv-3f[@-E[(j(Go9'%O[`BS1pv:?MWU>5f<EM0C_f6#-E#i74f]e%+K<H&dNoo%=eNT%=8Z01YcR.2acGD<36<P]vPHC##BY>#_C+x'#FNu-><1V%_#Mk'$',/CsVrtek6QGk"
    "1o)E#U;U;pN_A9cC92@5LaKs@2(:(;DVkaZF>0i)lk2Q/?O'>.h?Ql.3]R_#P$wa$Lc5J*'.=bF&MuC5V:_#6:,)V.:Ow4CbJ8D4tu[S@$7hF*T#])$Xq[4CE;-E3(UVj9Bp7@-pPQiT"
    "mt7f#''kJ2B6S&,A-/],/Cj$#eL&&vn]pq#>ZI%#i<v?]:LR/_#Q-E+Y?/nLMud/(@v,k0RKJA,U1ql8bnNT#sKQh,2$HN'fjuW.qC+^,5/0:%YE5n&ED4&%>ZYCh.AfTiUF_Bom6O;p"
    "f/Rah1EY?plD45((E.kL3b8V%?70Co^IR8%$R^P2aMx>6*Ngt-2mP408IcX?kXG_#iuY,*jCxS@M^`E4G65Q/h+pX1c>l++Ge3W-TiD'oJ*W`+6_rJ2LQ7g9*#Wm/MG.%#=Zd^7(3mC-"
    "$&###og7-#akYg)MD2_=D.)?#P4&H<`A?j:-YD#4Fb^h(3wsD<h9/_Fd>([GHOrZ#4?P,%6u8RuerXh(R.sQ99u%t@KD9CcESM0AXVAiAF(-*PU,^O9NbD^4xZ'B#nm6&/S#T+4MkL'-"
    "ZP'ipZo8b*>SFI)WY/]cZAEQ&0%&e5eM-9BEZbS%>0j%,kL)$#bO8T%RId;#cd0'#2MA),kSTT7GM1w#>QNd*7%@l)vb'j(wH,j'[>w-)J.?U7G5WeFtwV[#b).%)eR9f)`4lSA-UO#$"
    "nsH,MOxHn&1WTs$>CUT7KZ9i:Z8&n$iZF4(NU_.AH2]C#=NK0AoUpXcq<c4%=UPF#-l/0%:hnm_5UC+*h`7V)#WYp84KP*?#wskLWr2`-<bn=-r.&T&lo4P]No49^Mq6f=6e$YY2tKT%"
    ")65##[id(#TO^q#TG.%#86-^7pO3T%3URA,?s(9/9hwu#`LW-$]146'Ps+++=DB6(wHg'-U_jT%K*O9%DbJM';DU@#0twu#Xlv2A<a9D5OfSh(W8gV-GQw^f]s6T%n>9F*&041)?DXI)"
    "NW^:%$Ah8.:%1N(H@<u-#/NF3;[pU/29]+4*`Q4IxRVA5Ooxe2M1os(u%V$-=J.*3oB<Q/6S(97hRKKF%d+SAnP1W$_OkE$DU?DEIV*T#</-j))`ujL2'V5%sl),)=BSc;MiW_=5f`W'"
    "w>b#-2>AL(]_VV$U,U;.cVV(=tLfh(#p$Z$M.hY&aC([,5?*T@r=eKY?-acab+L/2YZ4$gb)Cp.t;Tv-R[;$^baRqT>,MP-Y-Sf$-MB+*V`(<6&`W0(UT$SJFJr*Fg.$H3Hq%J3h)_Q&"
    "/@YE=b4Vv#A0Pm#-DUHM`(6F-HOaf:c3]>#1[JQ&Vh'b,D](W$33%r#fh1$#h'8Z#'xGm8<Hu2vAk*O'?V%n&'0n<&'XY'$fc*87Z#E8pU'$]1=?qXcl_bx2$S+F3TH>wT_gfX-55UsG"
    "%McV$E]Oqu)XCqL^_mZ?9>$Daq/98%_Qb&-k+)a<4w?8[<'jY#oM(1$U006$7=es$/M@D$A3*.b)drA.].uSh>EEgLno%DaGhZ9H$4)VQESR4NHIxr#J5i$#$Go;%D<5n&P-bT%)t.##"
    "M5Rh(k(c*.XP;U%'tuB$,q:_$Q%gaI)MDpLQ=D%i[0&Cu+s]I3NXvghjn`)4r%-f*wSxGMm+p`?TQuANt#tf/*o+v#*KQQVPC7j(LqBQ/^u62'UbMg*,`Lv#4VDQ&HUIW$H4)c#o_u.:"
    "HX3T%JNGDuxwt6pS7a6E==DY3SFoB.:.OF31^(<-xvAW4%Grv9(B4I#ZLoLWPhs:#Q5-$v4P/(MR=vw-44l22<*8##LZ8<Q5tW<$$.)W-+N1o(Dp/W74dLv>-cur-?Pl/*L97)*t;mJ$"
    "+k1^=7+CB#.;8phhtKR1RIOR-C,3=%?<Tv-iVp.*(l+T%^LD8.dN1K('+eguCBmD4^vW5K6?=2(Pg57&KvYU.Kl^B+ZItqCc_+p8Itt'S8>+I#NQqR9d%vY#5g*b[MK(&vS)kT#rGX&#"
    "Mfr?-e/$%lX%=x>tfuA#ma6m'1%6U7BG,R(hPSD$AgdF#9X4_-hc@d).MQ68=?)s@.V%Q/Ii2phs%5D(vQT;H6c3hYCS(rATl`#5$'lA#9>OW.IOa(,H3xe4dp'U%-,e&4#3Ev%v-IK#"
    "E3_+?.2oc=o&$D6SJ/2BMl5*5U_wY##kD..ZFFgLa+=Woke%,2l@;Z7'S&f2Tn941O0Js$t@GZ,b3N6/=*p2'=]Y##6xKR'0OLK2$EvV%n3-O(L^bY-nZJW$^MmV%WX%[#:x/v(A#7/("
    "F8j^%jgIO(AZNP(6@-@5hl9qidq'O0DiaF3Z_j=.?P7EFvqk#-5`Fx>cc1e.*eQA8,*pRF-M+H;WeHK#<4k9#J#<_7,Cg2'J@-G*WWkX$FR-G*Mdlk0Wd6d)MacO0F2$<.L1s11vo)s%"
    "h+Ad)0pr8&_?0u$9Ddt$u<o5&6Bk>7b7T;-H7F(M/@su$L#,1#uK0Q/AK&Sco@pr6IpA,MqG`hL)qg,&>S>c4nZPn/`GBE+20e$?S[0K%inOV/:kv(+i?#R/wSV>.`;sANabs5/@>w]-"
    "xU)rA)c=J*Zg8]k<r#/:L1q>@Q?X$#FZ]s&oa]@-Y/pG3RZc'&WDm29-(E$#@o;nBNaTV6jA>mB5Q'_8M``P8Y>mL:4$AK:,R?L2@>m0Dp4WB#n-WW7-i,%59DU_,KDgY,f0mN'Rl/87"
    "0eSdFmbQ]6r^k=AEW=Uc3Z^3(r42ucS5U;pQ_5fGImbD<jN.U/ecWF3;_#V/3k#;/%qX<-6S-Q%]@SU.gB<Y-`c4a$`<Js$YaU603rW^FpY6k(>.[f*(+:F*petX?JPt2(b(Bf)J,?<-"
    "lgXZ)$/^Y6.ld##WYd$v/iFQ#JwJ*#&X;S0ceO&-^Jp<&]VtK3r?o@#%5b?%*7BC#L9:Y.Ur(W/_=#>%LX*&+AXK+,j29#,v.Lp72[Ds@DSiu7HRZ4(/$9M7iRM0A@m%hhW)&hhD.;Hp"
    "TA_l);ldp8VK0n)(?RZ?05N?8Sh)?#Va)n'm+<0;7SpK)fKWK==DvO0:h(</vu;O'PnYNBLd'jCP>K?&l@3x-;8>>#NJXB%4>$&#&&3'#*<=],b>`0('8TY,iGrc)5Zn@%#Q;k'KaG7&"
    "9:7<$ZdnLCku*87LXLm(G0k>7_OOTu(DGT(,&+22X+D9.lZ[[-*FfG;[9$3sET.J38+xC#wc<N1cRA^JSYC?-@b%U.&fSM12@u^G(GWX8wE2s#T$8>,_t]d+rwp0<163B8@11xL/[-9%"
    "xVg%#Cq)7'%8;$#QmrS&5L>A+m3/5'&A+.)n#UY$>kL^+6Q2-)>qf2'EXws$eBr/#-@dXMn8J+/xMn%)aS/kXEd<m0KU=c/Jl*F31S>c4MP6W-@Z.kXm?<<-OU9nJa<^M)v;O31qsA88"
    "MHHYBL0-k_J%mH*1#U.*]8+qoU@DU3@,>>#,]F.#pDFd77B[s&6voq'9TkQ'Qax*^.I7G;5c,<-2c,<--]GW-BEWk+G*[s&P,R,M3`;A-j@l_+q+7Z$+/2o(U;Dk'/K4?#hhCw,rvJw#"
    "h^nW-XXMv#Ce[8%W&m3'9rl##T.i>7:7cahZi%n$*@c4AW?cI3Rmt23vbd;-ZlJcMcb2xJVP,n%>a[D*n(K$,fgjn8R%$&/*BT2)>.^r.Zs>o/buRI2[-_Y.pVfOokxE@OT3'C8'hVI="
    "P#Wj;0=.UrbtHfh'?s:Q%a?iLI]vj'b=9iLQfx>M?)OFN>#FFN69MhLNJPX7XLRc;_s#N'IY0'%>nNt$@4Mv#X&]iLE*MhLf:a(=.]be),vv7&EcUV$V-kp%B$%Z$#8:-ML86/(JDQ&i"
    ";mFHpxp+W-5dPb5UaQL2wgS/)[w1gL1xT(%BNHT%(5_<-87P,M%GL*+H`&p&ffO;/)lA^JRh$v,q/IL2^D#;1arR6:w+dV1D#(V#N&(rL^fp%#k)&t#.GX&#;4xr$M[^d&Nv?,MjqtjL"
    "l[Oq&Z$+=$QNA&,e[Y;--`hV$+hO>-2Sg#,HUjP&O:iZ#lg9^#Q(087P8&waFs$^=gL;s@9G=w^nbm1MaEj$>ttZ<_4VnXHvZ9T760x9.]m=YJrV$S/PQwaNg;s*4rJ]f3>p:(&]D#++"
    "kCl`#6Jl+PP*'v7[Gj3;PV*,<#r]<.r^ua,U<Z(s-75O-@Hl0.8iR+MWn`]7OPW8&q?.OV?0fZ#BOI@#$Q(>$P&wl$gDuu#%C]],=n?MLvfRl'NVS^+M-Op%X645&hk.M7l(x0*Wmate"
    "L-S4(&3j.AZ9bi0OZd;.IEpN-(Q7h%#>+1`@M*l0[M:TRt+>(5v+LD5M]n%'bdn0)wsAqJa#WvIwA[L2eJ=c4tKW78jcUS8BG/sRxN=X_>)Os#Q`R%#uFo;%G@@<$u'vC+AGqg*M?ID*"
    "[GH7&EmZ)*Iees$#N6U90WDu2'9C%i(l8B3YZ)22*TRL2b9WEmUGku&6%Q0hqr0_O+Rj,+Z49T&H`EY-*Leg#8WJ:v_#YD5#fcc<FI4q8;($Z$gsGT%<R$#,rM$?.G:.w#$nioA6+Rs$"
    ";U;%,qn.A,G`uu#ctW*,[&)K(1i3Q/t?Y@5>.><k`7@s@5=s<fSGf0)>Dce''Hh:%laem0[l&c3nDK?P?MQs.o#'Cd2f3+3X=1F*1#U.*eRt3;VRov$aMs<90#no'pT3K*l%J5'=fGr'"
    "KNlC-N';T78oLZ#>MqW$5wYN'9h1O'?`lY#2p:Z#9);?>ku*87f<]Phk_Jj0x;U;pk3&3AbkuM(,`l>5%G+F38+xC#<rE^Jsv.^G$q>.q%roarQ%99T;hX%vt)kT#_*V$#Tt52'O`(W-"
    "u[MF%1WPF%,<,F%tQ6U&%d#p%X54m'P2`K()Y`4((TlG2Lcl;$T)2k'2pW]%J$'q%0MDs-@72mLiL:D5Y,EQ7'w0hLxXI4(Q-hJkf)vGknpqY?ER'N(6UtWU#h^^JhxZa.,46[$-5b%p"
    "U*,##Q,>>#t[F.#r'e[^fM=;-ALR$TJXRc;u?1F%eO@$T+;2F%'-,F%'>piL$T#X$6+uq&@XJB-);P>#rU4:&#SE5&WMBmffUb,MES*S&;]IN'>x_Z#I<gQ&J,]fLOnDv#Zr[fLRBh0M"
    "IL<3pkQA$M-19t@fFY?^lVVrL.wxj-&ao0#-$+JGn$1N(clCI3L[xC#A_^sC-hSe#D.*xtHK7gLh5*2M,uAfh<kr.<oE7W&wZs%=Aha5&c>Da<+7Xe%;<eS%jM1g)aKRW%@j@D$$iUP/"
    "E*tp%<uUv#B$4j9lTx4pxl]BG]7h?3HSWL2Y/VfL%O<v%+3]%<O9D^Jm)qxF/xh&6/6ka%4v7S@q@GDEP+AS@]<5i*v8D_A@0EvP*R*<$*2im&eB0$-)Ywj'j'&+.BE`=/@=*A/-Q'R/"
    "Kla%X=N4L##3)W%L;.k'<#,=7_'CRNR#7/(r#_GuMeX-3Ee?t@%oSL2Bo#V'qjSf$%i=+3tB:a#h=S_#[ULp7;/x;6a>$3',*xR(j<LX%IFe8%a@G_+]).-)]q<T%)Aqd#u<kY#:i0ND"
    "AVH>#inXk0$khl:_Y5YuZq1?u'm:l-vu.beIu;'MXI.^7d.tJ/`2Q;%KdUpLJNYGMJ5%*NIYc2_<qRg*eWJ3M802W-RX#H0LMIk'm%+87NK7Fek(5m090#IePa-gUF:BfhGZZ`Eb96D6"
    "le(a1Th]:/X:kS&gqeoIBB0nalwbf(l1e4o&Ux(3XkIiTcIH$v?]qvKlj>l7*^GW-oo1I$N>:I$M;:I$L8:I$L;CI$JHRW0SRmN)`jZGM/1cJ2*sjf<w:DMMp=+m'+?[s&&&uN9mX(a+"
    "3+Lo0Q4SA4Q_$<&GvQi1Wv[D*Tadr%XE9x*)'_m/wq#@5'9C%iraPl$sZ)22rSPKf3jC#ft04Q/8T[0)5HT7/x=/K1]q.[#ne^F*H1NT/Fo+r)U'H<.m)RD*Mw6u6`v3j1iExT%s:fu-"
    "Md<?6$(BY.Ug<?6l$180Rw,F-MLEc+1kh<9KZSA,L4oM1#-m;//@wV$wM$r%MgYn&TnI<$e@NkD*Z*M2XJD?-BL060JBW7'w0QJCp:JB$Y?Q(7l.I/4?_<9%k`q8.j'&4,BoCW-TOuI)"
    "7]Bb*24[Y#[fd1KH>:Yl9%0;6i#h6#/@Ha$r^u2v+9V:'prSU7=J:4)2?0j(16kM(228N0kIhw6%1LL2I`Nf<p%)MMT=Lt6:Q@W604gl:rv`I3gb;v#-9O$#oqq-MY]ch:`RWk:oLKp7"
    "q/@m0MEOUK+vDQ7VD:IpV1[khUELg<CdOUc0nj8.*+4[$q'<<-cm$>%Y'nq71$mS/hp5T%O;Rv$iukXJPbVL3I%Q>@1S[D,?38E+VIK5'T8EJ2v7b+3<I7X-rw1x5b<f;$w>UU%Xa*JM"
    "]4%97=cxG=s(+v8m[CKCn&>o/Oo'C.k7`q8CORt-x$H`4<N?3'Gg7](ddL,)F5rH2K)>>#@6VG#wmQg#wI;4#&m(W-WT@.,cr`7#;GCs$Iv%r#Dg1$#Y,h%#.]Cv#vk.?#e3`g&=IrZ#"
    "9#lZ#4p<A%XT<8p<N8%biRM0A*_^GuW_+qeE#jc)F$nO(w.5jL4%vCjwalo;EKtqC#F:(s69(gCNx49/oB1Pf/P<b@8Jm/*7FaP&lCpl8G9pm&)Hn<&?47W$oo*87mSF7qQfv0fw[+qe"
    "aTnkLv0QD-;pI>/W^7gSg>XNEqoD,s^anxFOcC6;dZ@DE-3Y>6J%DQ'g`Lrm9w'9.]oUY7Cw8&=Q8ed)R*tX$co.l'5u6s$MNhG)FU[W$i-x+2`<KW-@,VkDBU,@5VJ*EMr:Ss$'G7)@"
    "t;-E3:h-9ASk_$?Bv<B+u/-n&48<8&ae7-mUd#IA]^UZ?aMZ>-KR2v/1F/2'c[*SnUQ?A+0=xM0qAS$#<bu2v4(bIM9.#i]DAun(W#Up7s<a=lW-X$#kX1$%jJX9[X&>[#bsGn&>LV$#"
    "gl/^+^+VK*Xx5##j9:V(W-s^FRZYR&b;rK(KO%W$Al1Z#0,4oAZ]Ms@OU(E#NTk8.,AS.2%;;s@Vqx<(Us't-:NP#>OeiQj/LC+*hQ@A,0;<t(fSPb+iDj#KBHAx.Mp<D#u&A0<#Q#u@"
    "[84F=uXg=AnC7wRu-BZ@l5g%#Q2=o&T+R<_$mP>-`jvS7<nLC$d>)C&NFD6j#8Om11L7G;F@rv#An&q%F'O=$RQKQ&$_;D+>^*2(Q]Tf)t>20(CLn8%mo22Koo*87N)9mL:L,@5hctLc"
    "st.t-iTV>P872#$^7o]4#T(g;*Ff[JS7'3:1v%('vXuR>4=.^>7[uJW[wMvuQ%0T%]kr+;QgdN'N;aA+T&Ih(N8.l'RMWA+lnp**(lGc*J%2v#T-o<$>7.s$/QqD<V^n]cFxOlHfMN/2"
    "Lec-M[#RL2c1NT/YFq8.`*v#$--/J3Q@2*%U>:A=L^ED+L/DC&d80.*L:q7CCFJv6d%IP3ClioKl[2C?&GVS.:Z<xkQ`wO:d1-E3^eHK#p3k9#wBk[7$*2k'6f1?#N=V;$NFwS%mUFi("
    "(`gB+O$T2'QU[8%HL[s$Y@pF*,4Ss-dK4mLmf'E#%SZL-DiBs$0]%H*aA3nW@G84X>/W3X*uDf4.3)G#E,'G3?6N-*O9Vp/&)9ZL@_8ER`F]I2'ndC-8%*(,OUS),hUiilI3.po5kFJ("
    "hVl:mQXji'b7BW&4S[j'qO,m8I,^._AH0E,.U^*#SGO3Vt=a0(iD=/:Sphg(eekgL$G6##ZaokLp)oJpIFgjhoDb(1[F&3AxH&t8>$Ql$`*x9.b'S49[r#TJYX4b#w(j:H$Dhr$Yo$l#"
    "AI+[KNiBSi2?,[7S-@W&3QWX&5946'*nQpIaKrKP4<aD[^`*Pf'5>>#S*t5&bBgN'@@%w#,@F$KduEtL5mtgh*hueh,2?+ivg%n0eeY)4J%1N(ZI0F%[ZrY?4@B-OoV5eu:LS&Aon`W#"
    "<QWJ%%b+Q/7sQW]Lh[1*%h3U&Cr_0*a#S%,ePw,M0G[R&J3i^+``D_=DT-)*K,n`*wD=gLjEj>7(efRVm^26p$&X)iJ.(i2?/_T;'uXI)SwC.3n#qb%07v+DG:qJ):x)c+3>eX-H?moJ"
    "6#+6$tis[?XVWH/-RF9%K+(xtoiF>#=kAuK*)-&#Qu$$,(GM$#gd>A#sVW1g]<X$#]/&P'03X2(jb%T%J-=$#g^5A#8QZ+#hQ=]#eOIA#.Qkx,kPwq&osE>-F,D_=Jmd`*>%v>#,x7]G"
    "gkqZ^$Q5jM_w8H#@'g5M1ODpL13>k-.bo0#O-*kL$etQ'DO=j1n%tI3@]WF3x1f]4%G+F3]>6>>-)9+*R>1q8f&PZ0fv$L(+0C0cJchY#_B8R/rYi&Te4H>#MiEpuVg04`+rg$dVZZ]+"
    ")Ds80N/'q'h=c_+p6bM'2i=:&j-;NV9tIZ#cGd3'*e7##/iKF*KkwS%J'9XUwC8RuA]mrg8b)wa[+<qVmAWL2$n?l1c`9K-$&qK-HZ[h%es$30[Jpj2MnI>-FZ`o&A$O'+Q&oe#]Y;A7"
    "cQhc4c(PJ#Z?bc231gp#DUwZT7%j'#DAeS#haR%#hK4h)3Xlf(_%`w,`iJb*D'r7[@uSjp?=[<$n?'j'Z*jZ#1g1H2Ra4ZGT7qc+7<V$%UM.w>C`9u&H-S%#J$8w#`s(K(nt0HMW,Z@5"
    "dl^t@%elFM-R9n$hO(HG;w/s@^8a8/j;P@MoISF4mH:T%rg'B#1ZtC?tZS<&B1f#KHTJg:mrU$@;#qq&l-oY#e>_QTbeQ<_>Uf(#Lujp#PYI%#[Kk$#GoH=-iv[U7k:5(+2HeW&2G#i7"
    "?$J0:@'/g:ffNE*AL*p%l:;a/%QXW#3IABuMf6W-F%@Gc+O1N(wrTY&0KF:.G?ma*L<Z*3MnLC,4;>Yu4WG[ft.jb3;`&@DC%hN_964s$oZ%##.%no%*;R[$[E#,2YKg6#2/Ue7c7;$#"
    "=N=T'Pn-qAV^N/2bDg;-WX<j0+-Dd2>FMW+;rWA+6:#30T0]0:)FKI2mnxF,)5k22Fd_*#U]J71f2-C-N6+t$0G2q%'-1uc@G%bY(MDpLDqaRuilFHp=-xghIwXL2%]rB#cT[0)^^D.3"
    "?TO784o:T/.W8f32[dv$$`^F*&&WT%j+^:/.m@d)Z9]),-0-v$I?*h)G1*c+_Y@B$F$Ha*?,AS:u<-6'Yr/_.(<GA#`.@m)E1f/(anxH<*Qr&=A5fP'oX^F*R?`mN@vKq1#[?nAmq6p:"
    ".%no%Atl1g,soo.%.k.Lw_tW%NTZX/Gc%T%s1YU&mgb9/1cgJ/O^R5/V5.B#g+%A#(*j)+Gd<&+@49`=GeS2'vCx5'ANu2vPspl'fJNHMKpC$HO*Sc;#P;20>;;?#m'%t-Kj98%'-1uc"
    "sZT;pG$Tv&$5HHp[uUTctZ%hhW1j.A%oSL2T+4[$Wtt)#v5^S%<DXI)hk,K)SwC.3X5Du$El-X$xNW@#=-k'%V_sV$>lxi'._8,5v0J>#O8';.1@(R'-xJ=%C<tU.jnrX''oh^+bV3**"
    "dJ6v$%Rg)IpbYZ,*9vj'Pexj'ZJA+48rZY#Vi`.L/'poI+ju(3d.Vv71qw##=9AW&1lf51*u291#]Xu/)76;-eIx_#2dB=-BhU7'-QZ@-jG1%#%&3v8x@JA.Y2ZN'C%Vv#hXW7%G<C)u"
    "t'q`$)6'hhFZgRu-i%hhqRbs0qUco7Uc2<%jm?l14$No.,Pl_+.&+'.u?[q7pX3[$II%w#]-@'4>kO:8ATf,;wsWT#.m9'#ZlS`7Y8L<7veka=$Ej1+j,?##$Oj:.p*]0::C9`=SPn+#"
    "QQl3'(0?E=?DP#-fmj22nGCJ+(,<<%*u291CIB/1=L#E3QGY##,6=)ca[,B76KW6pUJ/ucM#Z20#VU;piQtZc@:[5<uQ[5/H[Sv-#SmGE]&Zg)UF%v7$>&%+EIZg)s;>d*r8mf(q-_Gt"
    "xi-;%9Yi20_DUSI&ZE+4JxE(=FSAt%fw&Po23A5/u'j'&1o^`NYb>]O24'Q/p3HQ'Mr[f1i1r;-JE4v-=3%#MgPA]E5::'#b.]v$P(S/1C[G&#+d2T%duGDEG5hx=PJmP9%d?R&38MT/"
    "#b1$%RdEA+n[Ea,f&jd,Pb7L*O?YJ(vSl##1uQS%3lCZ#5<5RC[%=3pRx]4Am2lZclk=%bYZ)22e-*b$,aQv$)>NB'/xRadY.-e4&^E.3FH7l1*NeB+ZA-63GfF]'B6h#.(Ck(I`N+X$"
    "21q58)oc)4*GlY#)q=Xp'Xji)kVN[.PR]vKnsFJCGZ<52p62hLKm'&v^l#V#G4=i%wt-K3F5ml&Hl&m&ZuU;$@V_Z-C^NS.Z2df(,8ms.8K;,#cN1#$L)l/*Uq93*MO)s'lUOgL;`#t$"
    "KlqMMA9X/OV&O;p#f9C4jf_;QSkK,NIDQ0`GB,VRk2*.3RTdMEq^.P'G99U%)Kuq.6tTm&$:VK#Olk>/p#.12mtB7DOcW6aEg2w.+%W?#^`3PfOC)Y%U3@ciXm;>,2(Pm1(n1,)H$jj9"
    "Xt8K2oqH=-Nfk843HiZ%Q^4&#Hb06&6j0tQ6EXZ#pKS6aQTSDcb1=/(FSVTuf:_Gu[mN12ZnSL2(8o]4;14]-90+N&@e+X$qs8V.cg(K(#6p;.QqSh*Yh:E4[XC[,[=N.;pT5wf[*jV$"
    "]=(8D+lpfLVV/wuo`gU#+6>f%N#<W/U4=J2$%r%,dKpF,tD-(#w5>##QaNa*%mwD*WCZ##F1XT%3dc##)`;L#iU3M7*S6]MdjJ+MeS=#f75TjOE,K.*DT@mJlEvW.6Z(l:2qwp.W1`vT"
    "@*'<;t3t-*:kjxFZvI(=)DDH*wdn/*K_=X(cF(q'gP(]#)(-g)CGw)GXo_0_=$o,+BVc3)[^+]#&fu5(UM.L(R-BU%/'2C$H<EDu%XLm(,CO;p[xTn0WVAvfLtw2Ad7JC#9=5L%n^D.3"
    "Bjlx@nHTucR%fZGQT`=.2RF^J3L+Iu/&iD$EAc;$pXD;$nv%gLs%Y%v91uQ#gZI%#3[a<-N8;(=%haBSfPv'=4Q*K6K'S0:5?x>#@r$s$pVIH)Q<`0(V-Xt$9j7)$,SBWMIqU#A9h7%b"
    "[%,FpJ5QW-6%sBSgx:<-lAbU%i]9GNsu]_Jx:d)/dBGfhQ8jER,+ZvIis/%#_<fK)%8;$#bQ^e$.<^e$-9^e$t<^Q&(Agl8D*31)WM.h(T;K*[:-ZN'B<Id)LLI[#wI#9'G<'U%?_*9%"
    "Pl/87:F>8.6eFQ7/U?,Mk(D4fE]+qe2P(kL14158ENkM(FA)W-SlHdO^]i`JI]DDEE<HZ>BPQpK49MhL,phj(0f-DE&OV8&.R;s%69JX%7G,##kRQO(Pp5R&Qm,bR@(e7'TWqG)YKg6&"
    "K(&n&;F<p%74es$Fi32h.(S>%Rh%hh%8B1g&mqSuXNs>NWe#98^6#B?F#LH8h$BgMqiJ_J[2i*%A#u58]<qc+1u4Q1=u,##X'4A#=c&)*sT)d*0Qn`*(=XW8U`9(,U^Fh:b+'^+nYRd)"
    "2P5##gw:JF*sDQ7iRM0AC>T.A6(mD/[$ht@HXG+FN6P)4:P(u&+fWj$C%1N(_wN117@`-4&psBHchg'HOe&r'^'W;$:;P4Sxxad;udtQ^k?CA+-re6#Q_x*^o&I2jYWYv.P4E5(&[+1:"
    "ZW^q%R55$,jq-++_Ghu$[a7L(snojKiheAc'ojEked'%1:RF1gi7<h2hT[0)$_#q.oDsI3SFgEu`uj@(+)s0()RLZBpHv)@u_S$p8dF^%wMgwuktis#]Lb&#@Z>>,MJp;AGn7G;<UmC+"
    "f^5J2M5WeF:w8]#=w_v%Z#%kKh$Eq.q$u,M%FLV&NaBV.+&RZ$rou&#ma@a+e>XR.3m+B,'91j(^gVJ),iu(+j=AP'j,cx#ij,V%*+eZ#ec*87MROJ*?w$12/g6/(/f86p(s&HG7hSh("
    "0Efk(pGPh#[kq)1<p-4(aN?+1e)^Enr#xU71#Z,*GI`Z@4kM8.CZNv6[)HI%kDdR&n4xh(Y?(h(24,$%alP^?0vU8%?+DZ#2L;N;]10H<FdAIM=AD+6cTvEl$V-s$2pFhWJ##*#<bnwu"
    ")lMW#rGX&#a8_I*%C*U.xgIa+t,1Y$txjA+5K5W.F<:;,R.0#,CdTv-Tt7W$hZgM'pDv7&]<k5&KEZwEZwlG*.V7f$*,[j*?v130g:?0hP=eah>q/L+,n]N-jUBvERJJbes)Mj'[B06&"
    "eQbt$](=X$WQlO:tiVK(,BYiMLIBfh0dtC?vNwf==+c&[VXH+-CT-bu=P&b*$ZQT%+r$##Q8FDE[0AJ19R'q%06;c#[YHC#9bjT%x>k;$w0^*#i:EDu+:o#%Zg&wa:Nb&MLUA+4k_Y)4"
    "+pn^$dHfL#v'otC;[KqC?a7*v'po6(qwLS.^kJ30fgJV./92c#E]p6(*Y%$v+BCn#Of''#wo7R&50Ha34jQb<?(d-NAkpC-(B=KMs4g+MYb>]O;2Ds-@`'kLM4S8%r>Zh>gc5f*-:9<-"
    ")%QPM'hnU7A4*p%t#;e?>GFw50EDmMaups0CS1s$p_(##:NHD*HB]VnCgcw'Cetw'M)vWUNLT>5<;4p/bQ,a+(^Lm/^B]%#*$J>#*+<M-t%@A-lnRQ-E;i&MOL.U7gEoj0Xw8a<+&g,N"
    "0$#xuQ=>o$[65)MR<a0$,ZbF+>ESc;nITp&AE4;-gTwu#b$B`=ZY5?#[r-F<D>@w>a@(TA1bb#$_./n&37:97+;IAGUHm*5sh)dc>h=1UMS5s.wREp.OM>c4d:?NFKZ+d;x5_B#S1&g<"
    "$KoI#V&=Bg<%:'+Ip@o'X$gN'4E?1Uu`txusfM*exG?_#hlE//Y.f+M`a@(OOFaKr,hW?6MWM($aY*c#5OiV_BO]Z$Fhjk#Z,S-#*JY##Jh+##I_%Z#o[Eo7@'6>%6:'d)e1tc)C;?:8"
    "3<5RC6TW;Txq2:8SB2c#XceF#`?kU2cv%gL7DWul<u9(#%FmU778G>#fC]v$^BD#543^c$;8$T./?AX#n+Vl-E$Xt1H/H1'w:<o9^g2c#mreF#MMj_A)+4U2(C(,Mf^`xNK*691]$E0*"
    "/3oY#^*fY#+K1@-v0q-3un'f#:.V$##)>>#IT.H#8w;'M%[MXMX1CkLW]+5$`v^^#c)iH-GHtZ%ocAb*;H^<-&F4v-;GmtLkn+#+/kO<-G2_W.'$J>#5qD'%FiK21u.eg3%(^fL?&m&#"
    "**S>#;wI>#9CdtMVPe#N_&:LM,;mf(4e[;Qej@iLHm-7$&aR&#*0E7'WAP>#Qq-(+d?C11U%R=-.[kM'&aR&#Sb#a&X/bs%/PiM'WDpP'[G2;_nnG,*$gq%4Zubi(Z5'T7=/Yv#%+PM("
    "H>Bj(Q<ux#A5mp%3U1B=M?4Au^ax.%TJv<-U't*%VIEAJRfCV.^%w@bjajU.=ed,*GjqR#jG1B#ALHUWwVaJ19m1M1<EXv6d?X%=I`d'&sr^*+uHO<--VDe.9QXH*?]&0%?wl.LKo08."
    "-N^7LZ@qs-9/kF*>i6A#Z=/R&+#ni$+ts;-'7H=M;w-x2CaHi$w>@K1uF>c40AF.)0-$E3&O6&8dDS.0uDEL%ZDh*%6s1K1WHvK1*GIL2SGS(4`Fx$veZ/aYmJ^YGJj-h1OxTS.Yk&a<"
    "Q_+I<6BAs7X:L#$8N]c&X/bs%0VrM'WDpP'/)pQ0Roq:m%b@5/:4T_A@TS=#=a2T`/2am7>b'&$YbPA+jXGA+YfX?/2KSX#k9YwL`r'l8Hg?_#2On##m,*/2_h8;.TH%Z$o-SPp>`Qt1"
    "wD-'-C2Cv-&X0gcocXg0;BiG#2m&i#>W2Z8vDHv$bgdpM]JXgMI])eN6YO8&J1Y)4via^#K;eF#OSwtL5+A']RGH/MmD*-=)0=q.FG>']K,KkL^w/N-m5AB.XDOS.GR*T.qM><.LgDE-"
    "=ppt7T[:_A,u_l]1'%B#=kV_Ag7ET/Oo4p/*?hQ(Q+:kLBU45$[O+2hu=/h1AmDA+ipTa<L7&H<#HsbN[w'kL#9)n#3?7^#&(x3'EaCl0?92c#(N2_Ad+nt1Y),##neHK#v[F.#d$=[7"
    "dxDa,IL$mB],h)31]C,VhY^v##7^&$3@WP&OHl.LF*#n&LwDD%UZfYGfc*87Y>XFp=mtghT$LH(JBJpLGvu2A<v]b4bj:]$'`j=.>G#,DITO%oL8(gCLBuANJ/PG-O':53f3k9#Dv4X7"
    "GE^&=cf?C#5FonLe1W;$,n:_$x_u.:c9P'A9VC8pM-CMhxS[Aup)tt.:9^OM=wKJ1`B]fC=QuANaavdup+>>#?^oX#$J?_#f7f084/3@$&]A,3D#jC$SDE($^gCW/tfuxuBrap#BFC@-"
    "iS(M'Zbq&5FgiC$bVE($XL@+%J;G##Sdbv-cHBsLhSE$#bWt&#t_So'#0G]#sn.T%`3-A#ghRP+OqWA+o-71(V?RU7[@eX$xJ1'#v&(^#X)dA#TUnH-LdP;-2G#i7En=1:+7+U%?AdZ&"
    "LW>##/W+<Q5,BT[H$B6&]aw[%je6?#x-,b%qAnS%J^-?#Ps[c7`22/LibZ;$EDS2L5wKN*P:HO,o+0m&?JM?#m[)s$3Y1Z#5.RS%:Gp0#Pb3>7FxD8p,ZZp.'^nTGQt^Gu.koLks#4;("
    "7IaBGbW(9(N(Q;p@:ko2B>6N3TlB:[,.#H3<Cb6J&NW&F-87<.uiWI)*)TF48?k=8grBxc,caI3AdaI3FwKR&[vPn&QF>JC+$EDE$3ji9`]rgBfT?L<[RZh<k$ut.$+'u.I),##WbpU#"
    "R+*)#A>G3^.V(Z#ZC087v7&nf[uRO(&3lA#$x_?#d_[$0fQ`Fr20poIw(_uGUP'#vr3#9'46[[%ctq-MgXtm'*C,+#Sc>C+w1VS.@#6P9^lr=.OV>;-Sbs$#h`OF3Oloi+w@qkL7ZfT."
    "cFIW$'Z>##gZ>M7nb9D5q:+F3-sOLMeWE?uXg;/(%,d2f.00dcIRuxIxCrB#i<X'60C(f)0,NF3h<Ss$>gfX-rjCv>;Ch8I,@k=.MA#G4_+xC#lx[u.p`YU.0E'8B%QDLDmsfIi&:B>5"
    ";Z2D-IMnM1`Ww4E5$VC4R8Xo[/ZK?-`n79(R6%a$E[3m)^5J)$<<@K)X[4K(G?x)?m:$Y-89A>#<o###n7GDE`cq4J>qUvuKFIN'>?R;-.qRp&Ap+`M/3tm/*u'^#5#sS])RPF#8;8N0"
    "xxiIqHb&%#>x@@#N9CG)3LPj'i/smf]`Ab*CgI[%=1rZ#f5qU%ciHPJ(ulG*fO>1%f.>.3K62Cc7//(#TFEJ1&]rB#8>`:%Bbp=%_V(.$lL3]-FIt9)5Awx47l/#KM0f7;[SdO#TCEx."
    "(53w.T7MBBgsgR9fw>4UxK<$/mJbi%gn9)*kmdrZJs$)*)fwW$Jqfm&kQ)4#FjPUc%L@9cbcDZ'pPunI79OGmSr(E%ol'f#MQk&##)>>#S^oX#EtWrLQc'KM@UiX-VXu--6ZT[M'qjn%"
    ",uI<;,B2F7H1^?R5.m<-nR.U.Q,>>#UG^01N####<U[wuNw>r#a3n#5UKb&#uS1P9QqN%bE?dj'AbbA#W;P>#68?a2LG5U.x$M>#Sef'-CrUk)>XcJa2ih[k=]hJkZ&N<q^#G:.SPOrh"
    "aGS&%4ZVO'CO=j1MWs.QQqs>7[F*j0H<4)>t2q(6F8=D>^a2&-k%Yp1*pt7D*st7Dhe1x.hNKg>h_l[.hTTg>2%I-d$_Y-dgr@-d3?l(&:V-5//+02'?OIW$`ggq%F7%[#BRNT%$[4Q/"
    "a97D(L?Y@5w<,T%hIR8%q$L_4@M9F*.a$j16@;D+N8_<1ZY@=1,earrd14AJRZnQ#`?Shus+sRACgfBJ;&6]M&E_kL.=$$$C.$##gxFDE-l68%Yt`i0-Vr;$C@rZ#&@D/:5%RS%,;iv#"
    "4AT:@?atBcQ4A%i^141)<OmG*>3lX$KRT?-$EFc;]8xduHA@Q/?U]9q^iIYDEMQp.Q(2s$co^>evs:SeJn=jF@3S>-49',.ndHiL+)='MMIp%#]uCb*fORc;tp/I$a,Tm(4$o/_dMw)'"
    ":]p)E.I;s%BL,F%R`qh7KRRc;d/QV%d$N$#ne'/)DSmX$`s?D*::i;$P9bp%'9FZ#R3nu3mWO=7n=Q]6t@S.2-Y7B352IEhvNTK/mb9D55l$hM2J`h1TT*qi6W1#$RIR8%rO-?5-$?Sn"
    "&a3i27.Alu/74n0GpH*3J't?#K2:k)pn+D#L.j>7xmPnC/5Bf3k+dx6(Ia0=GkeA6KrPE5Yq9S-F-wT-a$Uo-SqxgOFQ4v-l:/vL6sE^M0Jka*Ub3n8DObb+1`W)G59O$#_GS5'K8t<-"
    "1t@p&eY5<-+>^;-^e%T%j7%A#H=Y#%,)7s$3$,?#`S$Da6Kt]k*1=I%OX2^F_=dv#YP[/1Wl6A#`CJK+rEd(+WYvc%ouTT79TGB=>S@I27s#x#UtKh(Btmk(F^u2vEB#e)GwZ#)_oio&"
    "3A5##^#7/(I@[BG1I7HpkN<I3R$nO(&CO;pEkE+1WmxGGiVKg2]^7H#)OEs-n6sVABJmv$BkY)4M2`h$#sl]#kUfC#=(6,*AruG>@`JvAu_XM(LmpW-I=:(ie@]8ReYT8MIm[[6<'*<#"
    "g$4Q/oe0'#=mGW7?Y-<-8.ST%Wa[<&M4pi'kmalA0</Q('^t^,gJa/29W:p/fh4W%;P;U%$*sP86aA<7<w4$cp`;Hp*J;s@E/3na)_ln0qI$+sdTVj0MmT#vV<ZV#oN7%#(u3[7;6`v%"
    "*gX$#RkVo(N&ic)T&u'_L%Dk)kiBY&hOsV7D-B01$$D#?WCFx>Guww>e`rx$mmHkh#6'hhqRIHpckZs0B%NT/7M3T%j*'J3>5oL(4?9r.^^D.31[v)4sc6<.VcP>#fx<1)$`eS/40<PK"
    "0WT71mOw[beP6s.I2wd)4?jd*e=K+*S'`]5bji<LuF2ulMX_`N&YK1(md:3$C9DE4[g5A4ZNwCjJ1BA+fEq.CXcB#viOjr&c0+p/Sdmd3&<AW&F`<b=C)n+#f)Q&4#>uO2)o7b<%d;b3"
    "t*0$$EOFJ2LR5/(pn1@,*0N*e^i>8p(:e)%ojfBFnGft@Tm*-1YZ)227//(#j*'u$vE4C&9%1N(/Yq;-V6mh/x?7f3p?Z)4uH%gL-[Q(%a'bJ.@tt=.=AQ^,Vh$--@%8w-_dp@uU&Cr."
    "YKre3sOP_+*$kY-tQbP&v,+`+K/^G=)O96&?N015,oU+*%72P8:mYS/_N+TB,4:j(5APC52OJM'ir-SndU/,)R5JB(EORc;s<a=l>>,H26olp%Ej<<-6<e[%2G#i7LV,<-M:2=-'>^;-"
    "lm%C5KUns$M$-e)=7X,)8LqG)P3T6&u=+87n3D9r#'LH(;mFHp/>O22kd%k8](bn0=weP/Df^F*O50J3dL4gL0lb+bo%pF4b:X,3TMK2)im.F62xj%XkcR.RL>2[:G/[1NGoHO:M/5##"
    "7?AX#^p=6#2,3)#i0;#%q)K$TJ7_E[SQ7U7U#sq0ct&@#Jf0pANeZ>6x-g&47:VM2VZK2'5O><%W7H;'U<EDu2p%<Gm[+5p;Fd-.A:;T:/.UB#*5@x6S[39/C)H)E%v3^,U8b8;;sSe*"
    "5f<40#TRp02atD+/J6l95mmd30s.l'M.o[#-^%S-1jES7<w/_c7+EG2G?DW&$x;$#nj9I$1h70u-lKI)@p91_'A:9+4C,+#M6&?5[QK%#29PqfhZ1%$Vb1r&wDWu/2Q;g2dk=Y'egj?#"
    "b`#q.HS(C,w,]>#aG@U&>s^@#`nA;%N<nkg_q,T2f%V#ANjFHp,CZTc.In.A9od8.&]rB#]V$Q/@]WF3[jTv-c(^n':;gF4g#ZT%:OwT*x-W@,)6pr.$w4[,MFt311HF@,^GI70A>[8/"
    "Sdb>-MT@H3Mf^iBNWGN'i@G_+Mt6-)tOb.)[QCh(_?Yr/Ru8L(&IZd3ld>n&Zd5A4L=g1gN$RS.sRjN$@YU%#3&H6sqWYN'PiGK)dLZr'CvXq&=2Kq'u3FM*:'G/(a5d[$6d86pvv_A2"
    "owXRuP^b[$lk2Q/N29f3NZu5/)M3]-wLffL_LLK*m?d1guQJX'xDDS0.5J.2?G=`b^@OY?(D.]6$BoK)#7'B-B::N(s;?n&;M932ulBN<i+wI+.'ec6USEO'cx'kL,>(&v:7(R#mB%%#"
    "U/VQ&NW[L(7*<u?Qwtq&<?nh(qDhH2B87w>q(+87q)D=%LdGH(n?scY/ik*_Qjg$-B3i.h;TTP&Vl8>,bTFa,Mjm+#oHYlAEDou,SGf%%nK^f+Zu3x>/`*w>S]Ok2cEZU=8sdC#-<Tv-"
    ">;gF4LZ>x#x1f]4evKA==6-E391JqC03JqCX/i:.ZY_(alja:.F1S_#g=$C#6;RJ[<*./[sJ;G3^+8n9dE?ipOChlDDqhX.C0MigdU/,)VwId+3.`$#3xa;2tBP##I&4]%JA)?>[D.1("
    "U10Auu%%Q/jQtZc](tt.t4<T%/wZ)4<A0+*KG>c4b9NT8-HDW/(gs:-O&L_,sO>$,.NG[ZDb&g08(ugLHcb)#90,)M$1g%#:Kif(ET1T%T0u2vsp'9&&aKs$%,S0Lu3gHVd7Y$,@wj%="
    "&mv7&wAc'&_9uJ(lmb;-+A]^$Xf%D3(UM0Abbbx2hT[0)/@?e2Ijgq.]]LTRq;nh2nr')5Sw%^/3Ipm*:prX-k4%XUwNtWU.*7r#RSk&#um*D+s_HK)p=-p.j#[+#1=cM(n^^u$M=`S("
    "rv.WT]v5/(B4e[$sjFHp:+rkLLuEr1r:Rv$irxF4ae_F*)IW:.4AXQ'QDIL(wi#f*):Q(+[j>R&6t`d*B)$#-n*vm'8tje)oFc?,F(1T7U22:1T%F8&?rgB%;KT;8tRMM58nQ1<ch-f="
    ";2Q:vee8S@^4GfhgKr:Q#)P:vc.B*[i)vw'vsLfL2sq%4MF,Mg(N_(5Md)e&iXRc;h3B4_N[B-&>I2`5>#)C&hjZgLO$d[5$S@k0h2ew'7*QN'XB2>5*')x$AbTp&e_r$#C`>(+L6qHm"
    "Vfms-4H?Q@`;R2h#<R*1GXjfLhe3[$1]-W-d_v#@sVaU-QXRF%SFql8w]CgC#o$],Ug'u$tWZS'Z[Lx?og%l1Tq8L4iRr@,nSD50t[1@,-l4:C[DTE+;TSE++pk-*Gh:O+7F`%,E=Yp:"
    "9$JC+MH(uAR7dZ&vdw=-*:-g)X0(H)RTb:.aETm&SlQw-l)+d*>N/hL>[b)#IT:]$=ELlo%5YY#iq8S@1BXxkPY8>,%$AW*;Lu2v6olp%P^3w,ejA='/w(hL'%6Z,SoMQ&=ZFL:30N;7"
    "ta+<-CsfR.B%NT/g7is-epITApwxc33f*T%/Q721v`Z<B>F$J2x1dW-ekst.O@gt%B'>Q#,b3sLi2A#8&iep%=QdW8<4@>#QcZu>0K9Yl>9s+;J75a=&6A&,wLe),PB02'87:7'U_]d<"
    "^8V_=2p%;3@$J0:09EH*mp<#-RrRw>3nN`<5kuMVfT&9(&>P0AM%7a4x>4k2EnZ29F],g)*^HT%)h#R90pu599i8n9X.tb3q`qC6SB;W/xG4.D(=:7'#hK1;*5]>8o[>3BjEkW8aJIn&"
    "Q;lB,%]nS/1ck**C/5##5?AX#<4k9#_d0'#+u=`=6fGI7Kh70:K5,=Qwj440'Ck?'%#B2'_%.5)`-&p(Lck]%CT56/U6'hhZF.kL9ddi90nh?^BW5x#vu.&4@PMQ9Ej*H;-bw7RM:Bc3"
    "(IUh;,nwY6=k%Vu&aH7&)iP$,*'qQ&u)kxF?9-n0SY7<./7E`5?2miCHXM:.=I,e<Z<2*4V%NX.:),##>KSX#d`Z&12s2p^Q8Mk'e?,j':Gc9&C;Kq'A*B&=afH['+?rK(=6%P'G+3:%"
    "+N%)$K%IX$2g;/(NHGTu@0l(1J041)]H7g)0M0+*N(Wi-q;0q'Xn_:D_hbF0>$]A,0F[t-bSdT85qjxF.njs3n/Ea*n$dT.K?AX#*TL.6Kp+X7E0Z+#U,[-)8>_w#$j)%$Hd(4'Eksp%"
    "l]8gLh:O;pU;U;pis=u-QP:bEqp;Q/L&tI3HP'&t0E0--kp)5Kg_i-Gkf[77v$#)uF[5SD>ZZ`*[QwCjEP0E3<4i$#R(p`7F@V97,lpT7QG0U'v112L=9DJV6BG8/>u@b<Srhw#qow0L"
    "a=_Y#oY(9/`Y[m/f006$,JR@[mcC]#T[:T76^(?>Xg#F<$',>>dE'u$.2+87Bmv2AalJGM^cAT(P4_t@oec-M1I9t@H^ut$q?`[,fO:L31nUN(WIo$$:-1T%JZVO'o1WM.7uL@,lGS$,"
    "CG2X.wvnp04q@+Hns:q8'(Ed$`/XE4iKG/(D@aQ9J&D:m2]A<7<c3AJ<#p*=9cduuOxfb*P,B-M?Bb'M-F/'-UMu>7H=Y#%4Z`4%UTS7LohHZ#-ndG*BkjX*&SeD*V'G&#%Z#V'gw)h'"
    "OB#G,(5pi'-`YZ,RnD8(TpY3':r($#<)%<7XF.M7*@I,36x25p``R'M[Aa^%IF3g)B$?t'1qcZ(<cgN(ohpmA*aIT0ssEQ1iN$e)Z*EK=iSjM2:V,<Au]HMTuu2v6:#RLoieBp&8f>>#"
    "4,x_s;m9Ylro^uGuh5m']ZWp'lG5j'n&(?#ZMHC#%[6`2pk+Ia+jkO2'oIC#@KN@2ui5,2ke8:/++?K/mcV%.gqN%bQrGC+4;E=.([?$?6*JI*C7E:%A.2$#USC8pJ@pV-vUB*e9;aRu"
    "f:_Gu&?58.jTU;pcdTL)*)TF4/]NT/;N%d)k@C8.'3gW-w*.db+p6W%tUfC#,NVO'w?5a=')<o&R_S:/<Gj0(u$$-3S=k31fgL>$V5@H)C[@s4VDKF4vWp&,qDe$.QaU>.DZOh#k4(r0"
    "]CBv.+_Ni)`=g$5O_R:vb8+##=&./L?`%'7,LG5va####Ae`Y#.T49+DE^a<+)$C/hT&C/dI)C/Q,APf@x0I-$Jdl/#)P:v'G8R*#vs*7O]d'&?O-L,M&4L,b)NB#O$da'f=FcMrX+rL"
    "NYL[MrIH-N_1eXN;--+^C(rr$D_P&#p3ou-*g@e*Q?)t.BBdFr/8r-?SK(.?K2%.?wW%fh.WhJ)9?m`-rCk0,,U<C/,YHC/IYCC/NRqR[k-]L(U-W5'vmEVn&l8C/O&qW-aS^'/rr'W-"
    "8X6d)`X@W-Vl6U2`ii-?vB&OPE#TfLmJ8q/W3:[KNM?p.)sF5/qr'C/*:$5fI_+/($M*p%=>fd),m[i$$rQ:vxj`GM@q%uMd>FJM4dic)njjD+(JY-3t[J_/7r8S@4%V_/q-+S<v`&+*"
    "9w8a<J<#UVld?W.C0MigCT>AYT^rBuEu$L_v]8xtA#bI))elF#[Ac?-o)HXMHDlO/7WB[#R+#c*vkHeMTn>[NUv[lfDMOV-1CP>#rnao7EsniLbnkP&W,dP&K&ZP&@3YK.7a/f3%i/B,"
    "FW*Gi-YV#S&mYT%MQk>-)n1c#gVChlv.?_/4h(,M9V@K@Fg?_#nv^M'BQxY,471g(O*<dM%Fk'M2Rhw-C,4q7;ve5'NV6X1^?L,)FAS8/A788f;^xvMR3G0.oPahLlbID*iPQ9/ORfH<"
    "w2niL=30COk+#xu(UACMa$?J_7*<)N<WIE3&Q$b<x?feEW*h)3qd1?#^SB:#A>G3^,]Cv#7//876bMBkS9JC#4C4D#bB@EG13_83q*,##tWVO#wH`QNuU)v#B<Eq/<nsl2_CQ=*^]b;."
    "L2>>#4%'6/Ej?D*nsx+2uShpRh^f%#PZ$?#?r$s$G*mOKwqenS_(MmS(fe)MBtm]/^aYGu=_;Hp11w<'ZHsEcL]XjLaq$)0RlVT/-pUB#b/S.MHcQo/8,P/<p3N;.Rv9v-HAQF%MQ9Q,"
    "#+BP8Nodumj+Fa3SS'##Edc6#Z)Rn7:wP:&BAD2'`B/(%hP]Cs?cx%O]ldTO#hVE4opmS/HVo^PLH?D*##vY#T.cjL,tK2'g>D2',fZm%smnlT/km#f+;Jn7j3kq2<d;t2fg)%BGA%U/"
    "H<:q1<8NE-M@OJ-2mb0M_gL[-^[?N)h'`lOB*L@6wt>,<RTsZpc>bGM`dL@-O'M@-DPaU/Yv*##<shiKD?h,2361A4uj42'44PuuFWBu-5c$<-q@uv-/YSGNAUjo]elGC/#i1uPN21iu"
    "[[260>_%Z#K?J*^W>P_/&rQ:vF.r%0S[tUdj&G>dQwC%004s%0hX/on4HXVn*AhT9sdpC-%K1w-)ALxNY4G.^h)P?-hqO?-;MW6.D#-EN#:+5&Yd)eNC_+gPa@*0vS&P?-%&?69<03vm"
    "I'u%XVp[v$T'[n8'v.t%w5kD+6t+X7uCMp.X?AX#Gr'`/K$k5/];nn7$?tMM5N):SO(^0%S+wo%55Z$0gDCq8(SuH3b;PvP(vAk$VZf._.r_5'1TL@-ix?:8_xK^#=6;i2_wD,3n_)Q_"
    "[,OJ-<mb0M_5EU/9j&##sjgiKspt8.ioC%bk-xP/8oi)^Pn35&P4v&O$6dmup4:<.r82_+EXP]uq+-%.Bn6qL+#(q[SDs]uNpr3''JHd37;-`aB:)mTCm=%0r5x#)dBW`b6#8#QH&>%0"
    "u/AB()tpC-YL1w-]sfkN,$Rv]g4&n0iq8S@r)YrmK9U]X]]5m8@F$C#lNhv%^k78%DpB8.er#'OHW5T%]1R,*k&c?-.mT984V<L>@D_s-8-`hLv_mY#G*fY#(j2f%*#$$%9xOo7VOjq2"
    ")*;t2@D_s-<^NbNk$Ud%mP+,a1H'$%w'$C+ceZ'Af[YM_^[?N)5O,l$WZf._V8kf-Pv`$B;IM-QZ+wp%%iGr2f,T>P*liaO/,s7@A1Sucx[C5//]d'&)5SJLW#LW8]_*)^c2@X/?E2eP"
    "$6dmuZ4:<.h82_+DRG]uwfRr;#L]%=*d,9.Epr3''JHd3%CCrZjZ*mT16=%0kvw#)PARV[6#8#Q5B=%0mp@B(j?D,MmB0kP_1[juL91w-u2^'O<PAbMp9TvNr@Ciu*CL<.CO'Y-]/*&0"
    "JFj&0VJfBH#/kc:dPH9KVXHuu+(BH)GkN`<.xxM`&&U98?u9[0W8kJMSC>L%1,$N`P&1w-p^id*W)wvPYUu-&?t)9'^:aq2)*;t2Lkes-=^NbN]:t(&h$&N`MI9dPW5n]4n:q58P_K-Q"
    "gxKKM2+elLfVu-&oBDr2`ja&O*liaO91-JC9>$Dax[C5/N0$N`E5hH8GAbM_G._I;^VkxuZ?uv-,/GE9T/xP'bsXZ-qO?D/,HQr?pQ0n/.D4e3V#>uu]L1w-WZAkN4Q<w]m:Vn<S=GM_"
    "6#8#Q>^=%0u2AB(WT@,ML#-lP_1[juU91w-(3^'OD+5cMp9TvNr@Ciu2CL<.KO'Y-FYfxuDL`MUe%?%02/Q%02V*hs]2$)M,9e:96,h3(qP5##x0w##^'L-2I8:vPtLXR-EsbZ-v.Q_/"
    "wnIa/w$as-QDxiL-'hM'aG`M'[Gl`%qjwlT,9N'%EgJgM26V2U*thX%EQ'?-J)wY-?cRk2.Et6/ceZ'Ac;f$v@@AX#+<3h#&Q?(#/u$<$dmRK8x#U^#uUMN-Y=TK%XK=D/7<^p.9oi)^"
    "w1HC/ln`jP7]^@%^do._Tbag.j8_W.V#>uu'oB98%eb$01CBH/pYHSf6#8#Q<0/hW.F2$^GFh`$(ndi_CCD2UEs=%0=hBH/I_G&#9i)T.5,$$$#DPl&`$lf(KQ#b<m[[0sxMts7Kj'B#"
    "3&XJMv]h*<YAWEGAN1u8ka^/)L,Y$0q:6p.Ze%<-h^gLQ<ls7%J8G>#eA1@-HxZq8gWBj(1-2(Aw?ntJ_gt)9gZT/)slb0M_gL[-T`JW&t=GD8m6O0Pcf9KM(Iv.<s9Eu1IRxu1%.Ou1"
    "l+:/#eZDs#ve''#e(MT.<0cmu8?\?a$o+S:d$3:N(.D4e3?.]:d@4)mT@GRp^&i5#^U8P_/4%QPTCCD2U%h<%0hMlp+8@?,MwWK_Mp9TvNZ8ejueBL<.:,oP0h=Z.F2_Srm*sNM_&rN30"
    "H.Wd21Q$b<Y;$IOLiGd$9IXacnRmX7RH%IO&&U98pL'N(55Z$0Cxcm8t_w,;u>su1]9S<-1TL@-ix?:8rCb2(sP/%Bc(i;QhX`U%hU;iMl:9dPW5n]4r_2N9S/[>6omME-SV%2GU/Y3b"
    "A_Xac]VL@-I#.x-2+r(<nAr_ZnZsj&9f&*0]fE/%p,vaP$6dmuJ_3?%m`[a0(tXZ-US>D/gSs77*d,9.lkk60'JHd3jSuIUB:)mTvW<%0v?q#26xVk`tUm-.c:imNJ.d#^Kxt_$+3af`"
    "k^32U^.rs-Z1]*OE1>cM@.lp#AnpC-NnV''=QpfNu1EuPO[*_9&DC%0Fmk;-)Bem%.m)u8V-/s$?Afd)p,@i$U)n`/U4r_/Vo#.MZeFg$vVEB#_nn[-n`LC6Je]r%T8NR<MTc#>Q,G_/"
    "1SYDNjo'F*:w8a<NwB#>u/(`/*fQ2(QZs-Mo9'5#.oq;$);l8&MraLjEG5%&XPsR<2&vj9YO=_/X`^DNV<ZJ(qhv##rSNj$1R[&OAk#`-]]EY/'gPQ9SX,:V02=.)/9mF#_R_P%#G:&O"
    "Ak#`-V#2D-p,#xuQq5r#Yc$6Ux)Z@.dRr_,v4c8&&e7]%Y>WR<fI`o(SCQ:vLSZu>igi#5$=TJL*`$s$RP'##<Bl2$(G+cra[W;&x*7q$=e[tLPvi]P9aD&.oEp]=2f_%I`sRI-Dine-"
    "?([w0'qrM1E9NC8)Be@^hd6G@I5D5Su6tjPOE#K1RadD-V^Yw$q,?\?$C1fI-W)Zt$BKIu1VKCw1]+S1vp+Pu#5E/A$N*Y:vrmq;$L'>uuJ[?qNk5dmu.$&Y-2)sR*X+T(Ns'M]l+](mT"
    "WH-S*0itR*(u'YY%q&8o,fC2Ufp$S*L;,LMXX[&Me?CiuZ;a=-1;]:.dC[u>^7)680DL'Y9j&q[4*u2vK$w$-D^DQ1'2h98d`m=1uhw=1K+CgLG1cw$:-UW72AP>#SD]eM_rVY$Q4IQ1"
    "M,u?-u[lo$=.c&Ur0cw$.29Z$S?)G@DlBgLw1cw$Vg<s%tI@:TNK_t%FHB5p+DBBP%3$:8q_]v1Dh((vQ8wN8)sXEXn*wr$`x'##+vv4AIlXM_vJ%S*SvnnN?21iu,r`=-4)Au-H[2JO"
    "5B4rN@8:iu>r`=-+)Au-GOvIOkv#`MY'G[$bt1T*l02]O#vw%+XTW%kS'j58J64b+<8Yj^hA?e2-(R68b]?A0s6`DN9v77Tgb+w$X]cm'rcuY#kRk_$KF@vP>c+w$;CMW/VAX1^A2=UM"
    "$rQ:vx$RW/Vx&T:cH%H@q6%aN9)+_$S0D#$GU`58hS4oN?]`-mBKIu1)aL5S4U@`Emw^`$(&hi^$S3j$#.b20<+/*^B-%H3wbxK>p,vaP'?IY%3b<oR-op/N=M<iKXA@e4VhvY#qr9r]"
    "<AQ_/k)R]ukWVN:cdb$0$?bm5l51;eZG@e4N2L<.dmXf303af`ZG@e4M)1w-_$u+OE1>cM?+lp#sQGe4N2L<.?U)Q:r)I]uZG@e4KmO?-RKVN:XMX]Ywt;W-C;,W'YjM*4i1.=-*L_<."
    "Xr9>#`Wb8.4)GQ/YAh9.bCvr##/m8.PvVVZdMcs-3H]J:_-7m0CG9[%pW2c:./72LZaJ6'44Puu+WH41p<2m0S>W7<-ex7<%Nt&d?YHT%J)o>$*'uO%)r_KMAoNw?*Q#UVIRqTVi@Uau"
    "(x5XMG(8F/-,>>#J5a@?<akA#s>?L-Sx3r$Y&JmKO4:@0tBtsU_=,oN2gRD*_vX?-a'rd-bcLk=:(gkN_w0auRg=6#k.k]u'^6Y?_`'#v=u5iPO8:iu:p=?-7>L8%d3wu##H[Cs9FHDE"
    "Ej?D*1[;J:B@/s$p?d(+>w&q%-F?U7AMGn(g:D%,^;-7&O]Q9%uqi%#FZjW%4`:Z#mu*87@P2l(^j]7p;v.E#6k%hhG%k^cGDcf*L*RT%7M9F*9Dn$$X$nO(4?9r.Q0mcCD;8D4j:k(4"
    "n/b000l^mA7QxRRr5`&To<Ws6#WCP0eJWY5wXZ;.BF$s-$4YuYx*DJ:jr5s$+ANfL'mk&#u7VG#GvW9#PSuN^S)D0(xYT2#9d;/(%@_#35f_[,tr1u$RFd;6?O&s$nF=M(=1B$5L@aj3"
    "WbiZ#>=4g1s_i$#2sGPMMX8w#KTKq%596)cF^eDk^141)T+Ho$bG&J3?DXI)G]9;*$29F*R2QS/tjpf1q;LU%9w<Y-5Lgd<I]Le4Pc[@#[,/5#;X2V7H*p2'Oe35&1-)^XP)?W-PS5O="
    "$nVd-7h6>)Oue;%L^g7/Ob/Y.95>##AJ7@#H$n(#EXI%#1+F[7cpFu-4JCQ]D<$0%&BA6.jw(s-T3B>,c^t#*A?#6#?hl^X]Bwo%/@Y>#K<eE.a[,B7N8eMh43u23N/WAu*ouaj#Yt,5"
    "#XLkHY0lj1$.,Q'8<Nv68e?']__5k.CpKv-9d)daC4^;.j.G97E0nH)nr8'+5l,29?hgg2]R]U/6Cec;D9jD+FBA&,$_(u7eat9%e1=i(['m2([qJu.%aAU'vrH.%w,?\?$=q>>#KjW<:"
    "sk45pNQF9^SD:[-#'=GcVx.31@[D>#Hs03->w8a<$m'1&,[-2_O,8'Z#62].;@Cn#supC-6Cr9&V^%iLEMdY#K,_S%M3']0QZ?[0ml62qj2=U2&x$lL^KEY5F_0SecbC&I;@CP8w7WdF"
    "N@-Yu@ki3'P4Q)%:F<D/)(CuYImu]Y$^?ENi]XjLa4@]M>v'kL+F5gLRS>SN2o#wLO(6jBQ^&g24s7#Qj%UC/J[4-YSUZY#,d[Cs@OuAm)b9CGU%$G*mpLG).W,U[aDuw#srNlf_>0v#"
    "HqN%b;'vj'm0`($8L<5&lF9mfmipv#BEtR/K0]0:GtGa*FxrM04ab;$:CeS%wtPm8xH5'$$@PTu`OWkhOWh*%9EjIuFxav*>1OT.)w@+4%vYT%<A$8B[$ipC$X5'$im@DE'/JA+=-4F%"
    "xUGxOLYXgLW.#X78HNl]eJ'v#W1PV-ufcY#=ws9%FN:,)EG(0_ZOF:kM+2,2e>q'iu.G&M*X_:%Qr+N`[W*/:Zhos_#v.h(>98A,`kf_#)xA0V`:r.&5Z;%.'7?,<)J))X@cX'#9n=/("
    "FnI@#tn^`NGcnaj,[3%N2o3Q/W>GVDW$?-6EY'(*$)t**X(kZ-B0EVZPs<`^G$(9.EWu>#9$n(#`d0'#1>@S/OWG%liHX&#pcK?#jW]R/qWSwuM(35(w)#,2blG1AJuu0fdWdwj5+&hh"
    "@e8ocX')j8Y8Q:raCI8%tc:2'N/w-2w(Hr%u$)D+vi.H)d0HYYJvvH2wwcR&b].h(,,ao&6=U/)wcNTDNUPi?4l'B#rHw.CH(#djQXEu-*#2=-6Wv`%*pl[0p@gv%QKkXAjjSwuE40q#"
    "$BCT.nuS<#S+q(%]CL-G3xh3CmX$25VE)NR'O.>)^U.LGCjn>$oKil-)bk<Uq#Rs?'_3/Mdtx],lFDs-^xaKCOCv?&vZ0l:1K/wo,>']?rjGQqUL-##SRrr$<t2a*2d8dM[awEN_[Zi9"
    "Ap'B#m?LHMj?3n8,xK#vI,>>#.*gkN[n0au6?_P%L`''(,i`8.BD4;-4s7#QWDTC/Hj,?D0`b&#bTK=8gI-),ti(v#_;.H)Z'I0#8//87C69S#P+&`hQ1RT96h+BG#78N0Ynn8%hNf@#"
    ".KkoK5LkA->X?g)#+Q,*6b'&10jXD=[/:Z-vVdV%@6DhLr$0nLAFbc8UqHwB[0:,)92/b-i/*xBVD.bC=9Z;%?O'?ZGIhhX6A#jL.v+i(imo[#Gj2$Z64xZ8%fKo0n;ZW.g8:A#_n7%#"
    "wQ;-*gjcR&bos^fH>xB#hEGEh-UM0A&8Hg8hSRF6-hPi8WuvKD'EC:%9@)D+bfWa*blxO(-[WX-)Vp^+$^)4'c8nd)/ECV?@Ub]YNw8v$[p@-#tq,U7FH-)*N;MI(96@DNHbP&#'%cZ-"
    "$]ZUp7H0g'w$`xLP?kJ'i9SBno1r@-%Xsr.9r8S@8)ZrmdJ7s[nhCJ:6Jv0)44Puu&-RZ,WH^a<1)KU'Jl(kCAs9[0l=qKMI;CJD,jAQ((7pfLh4^B#jE^M';n)a*Mn)d,S2ItLw&=V7"
    ";AP>#busm$P'FQ8H)R'n+5b@?:bFZI;jk:.6wlF#XtN:%YY1j(:sGZI%x9+*^1eG*P.pxuOpS7.W*gkN[n0au_mp@%h<ss*Hka8.J7dl/3m.#QtRsf%&CckNOGv:?r[OJiulKq;Wud'&"
    "b/GJ_S(ioLSR6Yuha.50T;8r0<$V1(%5Qj)E4u'+CaOoRxHmC+)o8n/&otghEqXah9fJ_+5I3Q/'V*:.CJT8B(fj6/dXs7]6+:GD?xK^#Ri39#xg6'#[Ql>#.O'%MTWH3^pU,m8N=fBJ"
    "nPWL2>Gbv/)LsK`NY4v9?^)U%hPd20#49/LE`$s$Hc9[95Hg8._5C<8d9xW-*UK)-Ucm9/:?cQ#4tnLMVAfU0U<$##?bV>#UVvGO6m^X#6QNrNBZOk%;Ohv%@,$C/2TV^42-0<-a8qdN"
    "WB6IN'+I:9)pN#v,?AX#+OS&A$db&#$fCB8QsK^#-<GK.]Eb8.h=#294s7#Q'`UC/r.^[]Kfk[tfw&Po^g'5`nHKC-U6UW.Fm7x]7H;]$FF`G34s7#Ql+UC/c+asExRO&#5[#1%QGLP/"
    "9`[]4f[d$v_hrN'MiZ2_s><B(O[.F%[RVj)Re#W.k+)X-kU3C&KE<T'GR)fD-&(@0cEu2vO;J$#v(Vp.x+2$#-^;U2b?u2v3sF%#EOPQW+eC)N':#gLL%m/*V;X@$JP*`#8vLW3)m'c+"
    "nlI%$xW2O+'IDC+Sx@T%$[F[&RS3F=P(?;-S25##:vH_#f`Lg*nfAU7wiEL<wTvQf+V;=$&?ET7aQtG2pn-<Qlm0-2CN39[gPOV-@stY--/3da.S`]#scs:#g-8$4%tQ1254(v/trr%."
    "?MTY&G;8)$_BbY$Oc7T%UdK0AT&]KhpmC1A32>0AhRbs0?O=ZALX&-1#D[#Ar2BUJSF>+%.O,i;c<5H3$SHx68F@1G:F`t-Bo[j:-5*T'^UM*>/g-aPL'F9%^cI1(^EFt$K<0U%3[K&,"
    "qWE$eTgtA#)Q5lOa<fe6skY'4^nb9//9,#-E_TQ0Na]4(I/7,OvC+ek>=LP8`S+uBS1Ef8bK-C#4C4D#/_Gt[2prs9SrUa'eLrv#cS2MKNO*T.)oWL2qEiq'=cM:8RjKV.$uU<-f7_s-"
    "mD&G&H,Y:v9C*2E$]O&#]Ena$^oq;$jv###>_%Z#xDw%^B5i)&Rkou1R7Q=ux6]u1MI79+OqP]u/rhw-&-UpNQ/..%@wjp+^E^a<$x)W-+iRV20uf@X;lmD4YjsU2C#:]PY8:iu&oC@-"
    "%<sa-E[JV2RL=ucF*hS88KgV@Iwe$]vHIP/P<(ipqGR,ErIYgNf*o4ALe`MU5_0X2PK5U2v,/,DQ_h?Bv*vR[us-W.Lvl&Qmx&x]K^w]%9=:eP,6dmuiqhw-7hsaNWE&VQ;,s]P)B5u1"
    "t^Op.kGTq.CL`2M1GdlPf+Rjud)%x-6;#CO^Y)mPY8:iufmC@-I5WE-M'%x-O9o(Qe&9$PvPAbMVe'BDmht&#39Biu93@=.d'`@50.ZV2Zj6X22DwqL?'bs#X]C`$UPhML#@h)>)Y##^"
    "`Ew]%p,vaP,6dmuIqhw-DfrdNEM<iK;,s]PYO*SaP-a1^QAFs^)rn$v63YX8AI?$$o$###>&QjuR/@=.j.o#2uM1;ep$:#Q`[WV2WPo@BkjPV2VpKrHx9>#-1dexuVb/EHaPK,.)H2/Q"
    "ll7s]NCkkbV.SV2sg*VQB[O;7U'Q&#5wu'>uv%3Me@-N^)?S&,Ot4j$Iv8kL^7&r%jpmW.9#,N^q_oL(9J6X$1L%sIQhi;-E^+Z-K'T$TZ),##R,>>#(]F.#[eua7@K`v%=%*.+AJVH*"
    "rwE.^5R7G;>:2da0WRc;;7;X-ufHb%J9m**OgKV.*fHp.3x4Q'M:R&#0.2H*gF,?,K,x8'HvdA>S2(E<GG,Z#.T4Z#iw]4Ae6,Cu1J7M7Jmtgh?7#7pQ1%Tc7DY`(Z8fk(#0qd%Id19/"
    "kL3]-$.C+*B$s8.au$5&dKvI3;O'>.t9,R&PO)J#u7cY':mfQ(#o_q:,Q7l1b+xb36*J:7>Xa-i7fpd*YTtW-k.ws8nnV>7+bE;7Ytut?dJE,#e.ED#kELn#AB%%#n&U'#WO>+#GaX.#"
    "T]77&F$*AFsX7+>wm0W$4Wc]F,8+87`j)22?5-L-%.6xLasQL2Hq.[#'ME(=L?\?Z$,5ntHT%lJ*]1K+*2M#G4i1NT/At>V/[%'f)l7^F*3nVa4VV5W-C:Lk+%;7<.DV]C45`E<%+2pb4"
    "'VmD4x']t1YHo.*Bhp:/(Oo8%PSGf_:)8t/9@/`,Ul6F%Gs5]u`(Dk'/lL5/9<7W$v<l,MFMt'O&[NU.UZ#R&glx87e>7_#kjCO'R'k5&tnp+*d5Dk'w'v_+Rhws$cG7h(f&Jm00U_hL"
    "@q06&5J,W-YQ6`ANK+R'V^lj'Vauj'4DUhLh&Ww$PH06&[c'x,;8d40[&?R&:>sS&<:RB,mkFQ'Aw]2'Z:1w,=._v,H*#3'T,q58mpS@#lmc.%P5[d);I3T%O0KM')9&##vY]l]-l68%"
    "=wfi'M,@D*^7pu,nBIP/(N#,28YR]49$g&-p4v##=9&iL&(0X%InhP0GRlS/UwP8/GF#<.+$hj]X$hv-kCwn71S,<-ROM=-aT+?-L=2=-<<2=-K=2=-A`H^60N]a++f*e+F=rk+3OC^+"
    "MMsA+_M(G+Yp2L+Ws<X(Fr<X(E1,+*+NShCRKCg(.7hj(2EoV6;4wo%pBLv%@1dY#Ee[%#>jo5/Qv9B#T$A8%nukv,m$9VI@[>E*Yr*p%b@i[#&h6S'KA7s$/ZsgCGqJQ&sL*p%ogNZ#"
    ",uM_YF_jn23Y9r2L>&<&`Iad2ZXl3L`1_32-g%&0piCEFndvt/6.WB#01#/LZ0$n0.>Q^Y4cuw/wAD%0Z*9R3*j[,&<mX/MZHRmKRjP^#8o6`.BC3B#-L,Z.t)+B,lfKo.8%-^Y,>^_."
    "og,c.7Y9UD?1VS.=.,JL[v=nE12*D#D=.1Mi+]p&M4%L*;dob4a4'o0qhNS72_44:Kx>;-&]ii0S5*Jq%V`i0auC3(u9qK2N;%U/L]NS]V?-F%e1o)0B4v/M;u]+MM-C;(mwKK2;k*',"
    "Y0xo%BOX/24es[#()su5'7]t)Du<5AbXmw)9p_(*BiU@#K^a]+N;ku)obhoIVjQd)]tE5&Ue@@#7$Hn&?R]x&j$;u&565+'ArxY,?Q;)'EYHPJ3^st&<@V?#l.+.')#gdDhOX,Mx5C-2"
    "?/PZ#qVru5PQ`uPJQSZ#8L^1pZT]a%$B4?#dSik%nf`#5@_aT%D1VO+G3+A#4_]%#_7.]#H&=1L(2'1q7M@]*6mw4]P@3M*>Q/o8_=ZW*XEHCXZ#*E*^$F5&$0qH2OfUw#K9fT[%>Zw#"
    ">aap'm.$@#VU)%('F;n8Il%k7m+b%XSk[t'r./_4NHuj'hh=:&RETD&g$cV-*/qN&]FH?#hVi:mCpMH&v;F?#OL<5&GRGxm0o6s$$Ux1_B'dv%YjU'-48P(5fL2>#ai(%6X<$D#t0J.4"
    "W,<i2JjV'4[ovC#bSY?4%XUFbdN_%$p&>743J%D#eY324Zj#D#Gu-##71f/Mb,EO';fB+4::BY%V+=`#e9*v#39v2v<B%c*au20.XpT3.ED$B#'kgC#b',C#ICUS7aaP8/>Gc<@h=MMM"
    "S[GS7fGu1:6EpB1IJB/1eRx0:ICEJ:*_pW7Y5a61R[;##@Z$b[$7B:/ekWN05P:5:'uE`aagh0Mu]*b7BrR3:?:Qo3qSqC#Un&,;g`YLM/go87owI0:lxIm0Ro`#$5wMC+ecB;0+@0X%"
    "3(#JLk^.3:;+vcaE@4jLN=6>#<on.MAcrV]-p5F%%I%9':Er?%g.8U/5UFK1NL2>#qS]6Nu4eV%&r)U./xC)+;4O.+6VY3+OewD*V^v%+9>L:+&j[o[-]oq)$k)&+2nM)+d%3>lN/97+"
    "/tx+Mtc.2+diAc*.R>T%,NR@cgUfXmk#m0+bSmi97Nb++E-x@#nf?v$2fu>#=-'?#xq'8IhTr4%opw>#k%lU%Ax-dDB90]#w;CD3C5`-2Wj6kKOEu/(v3hA(oLVfU>>P7(',B2'R]C0_"
    "pu2&vH[77(bqX:(2w;H(DX%@#U^>YYUsF[#01L3(xSffLbQ3X&e%5v,BSiMieJ5/$q*p=uo2A9$%[^S@<Wq3$(@`>#K3Xx#N2[D*dcse).5R.2WSoRARg4xP`Ym:%[FZRfFLWx#:>=D<"
    "$RpM^kB.],/ZPx#9Z-s*e2u#+Mk%cE(FSx#4H8e*ZBURo5CCo*t_5v*FF)fD8TXf*g8p@#O*XP&R.)?#C<0?#2(CSIvPOQ%x8+?#33[S%>XN9%I;@8%r$U7'K*GJ(v[kM(+BoV6VVgY("
    "r9?c(%.Qq7)gGc(vtn(WD`:S(H2M;$6jK[#P*&](E@Hi7#Y[8%.09R/S^UK(7F@[#IASr#ktJ]X#q1c#iQR>#'I*DWU%+GMnOblApo`fLcx7g%84`Z#Mi8pGr$9ZA,A.JG>Z15/ph@:A"
    "'*RrL-JPfGAM.veGQ<=(D+*s@HPZh,RVUj(rZ4N3(aWq2E'V_&R9.m0rg+s@'@Ib@^mTC85@?XC$V(kLX)C0MYY,veOhYBG_[Zs0YJJAGG=IwB/9nH38n'QMH3YBGRKD_&0bJs@97E0M"
    "dOXgMR+pu0CGG@3<CEEk@g3^Gx-B.3cI/UiYiY0YMCY_&sKtdm-d74XvaK*n?r:B$a+6*O$T-=.i/2gAFCM,3wA'wL*cr5M$v'kL+)+qe7m/gLe7gZ.fDT-3_3s>Mftw'M=)Sm25Qh5M"
    ":[U#APV#<-4@cDNW]_#Ap=C%.f/QJ(BVffL*s$&46E'.$;p,g)O>YR*W0._#L1AA,]id`3ueq-$@89JMW1L3,F4t9)JBmRe];W7[E%V5/dtK^u1Z*?MwCioImq1cD8+5GMM,fiL0u<BM"
    "R<AG;e-D'#*>cuu`/sW#kSu)3jpB'#9I[o/KbRc;E?X$#Z_<mL8Z+>-9X7G;JID=-PtSv$)3,F%Y:T-)c#3T7+*.%%,IdZ$PW]e]]6]nL5xxh]Q7]A,iO*Q/tI1ci6jRc;:%mcat_>F%"
    "Om<B(SMEJia9[A-mQ,@.1ZRc;5DSj0ardU&8;ln(&FnP/Hm^Z-dD<E*,-GX$P,U'#q$J@#Wc>;-HJM^#1jL>$):$C#9LWP&_ULm(I?%12(A2s@'<^;-ws02>WN$12@R7U.$TRL2U;G^C"
    "o]ve%#;6J*X16f*dYZs-CJ;(=qYS#G`xSF*-5^+4nGUv-iL0+*b3hBs-:?G*(n`)+2YAL(/PXe)mW]@#%5te)nIB+*E-L's)5fA,+Mp^+^Q#n&)-2k'10+>-mpTR)p=te)Y'JwuIiW5v"
    "=T'^#pnKB#RQF.#I^''#5,>>#J7C/#w>pgLeUiS%4our-jaWV$/E%<&nN.##xE,F#AG/87M3=/(>P#'3pxpkLJBNPgRi,V#b<ISu_+,##pStA#QbAr/t#O;$%RGb#Yxh@-c]h@-lF&V/"
    ",'d7$:11f<UN;#vqEAX#XM?=0nmwY#xNc'$X(:rD<76g)Rm3jLt^sv<;XTbu@>C1MAW?##5$E29ER[?T9qk;78L<'MeS`8.5Lvr-r2.2$.#c?-io<vL]f?##C_V>#vKGb#3BM2MPnL%M"
    "`K3]-e0csL'9fjL^W5P21eDV7%M#292oLZ#ToK)N1/;2NK&*i2T^Ma-p(&hcWQ:QgGxLP#dR3Se1H/hcT_d##p0H2(aiY`<HKbA#K):J#2oUZ#&J3T%gRq>#kWO/U',0o1Lv^C#sRDHp"
    "nA&HG_[p+M`>x9.wR#hgYlR-m?H):`@7,s6[HXc23i78%xSjlAINJZ#cj/e$]vRh$s9#b#'O%K:`[0)NkU_?3$5h;-$x,D-)lD..U2&.M9?+jL*+l@&QOqq/eEJ;Qu+%,VnL529o24sQ"
    "j06^#p57U06^,B7_OOTu.YJpLu;MhLa_KwL]Tsv<.rPw9Tx_INq'['6Y=7Z7d9u/(w/kv#.P]X%ea&i%Bk<t$OcZG2NT%C#w]T2#rAwo%%)7W$x'8s@Z,f#/YIu23t6GhYVAK,N'T1N("
    "q;U^M2Gl(N>bSF-[Lo;%ACYB$#%DJ:9Co#.0$AX-60x9.,+KQg$,dUmHHaY#3HX;fxEP^$5/@S@_e`^%O;eS%4F#n&M;h+$o*^E#w_0T7DONXA&:%K:lEcB&E]XG2@UWT%k=)mAw*`J:"
    "f_r.LE?XA#d?wq$bbW#AB)_PhRHem00m-ahagEI$@2u;-bNwE.$Bs[ko3<Eb)nb%4RgvC#xewI-FM/`8h.(<-QcCY7qp&?#*3R<&[js>#s-^E#45rZ[moLG%q;FsQLXKk.j'ZPh#xt$/"
    "_OOTu$*EI$T7JC#uW=gN:QAX-L/#]kjf#w<eAKYGu2;cMsJVqMbMwLg0DAF-SYPn%F/s80EETq%qsEx%[tTmAgu6aaDtJQ&62xlf6%7W$xFsl&IW4B6$g8+iD_[BMMl%TcL#L9i)1=j0"
    "C%G)4nYHpuHT]l<vl_JM#ENfLDtJfLL5&Z$J^.;6%V1T7Qq`qKHKJZ#<Ze@%cM0?#^jem-C3xjkYRWT%JP`^67DAb-];IwB1:.JG,ZNv%=N23;mSY]u+%)B6qS#oLW40?N4KR##OcCY7"
    "^72?#O5[S%ShQc$4jo>#.Ggm$ed-[BNb7K:PfGa5/sVE-g@@M.Bkxgh@c3E5q')W-bjhx$EG6XC3c4Sevqp<MZG?uuqEAX#^@0H4hCf9&2t$<&L&Y;-^wi#GQsF<$r5UH2MZjY#qKu^$"
    "3n@K:'_C_$fGT2#:%=.8#ko>e6e0a#l>l;fEcBX#og6cMprk)59YfF-c.A7M7_vX7fW-:&Bk*?#?7`Z#&Sa5&i/s58*)Sj2wgvGG^;#WoVeH1M200d<EcD*eH^_%4k8HEMh50?Ne8c[$"
    "GEd'AvwsJ1DJf5#&d@lf&2>>#Q_q?B@I,F#X>U.8$$8N2B]-<.4t@X-O,8'.m?A1NW/]egXrZVew-sE-m=X04,NW)#-Mc##@m5X$`]>29q24sQ%tJKNwEj>78L<'MY$RL2T_Yw5,@`Pg"
    "s>xd<b8-;u?;G##*Mi6&6#l$#F.#a7XgfS[87n8%.Z.HV@9+w#t/DK&2$&49V4.)*9'RO(MSYJ(Kc->'9=sD'pegmfCF[<$qFM>#Qr8f)fLw<&B44O'[xU8**C5ND#@j>7,^LtL)h.ah"
    "^vU0Aaef.A_R168<:^;.lSx/0k02N(Jc&]$L)>4/APRUgJ98I$Bo)oL`)0I$pig;-pv6M0@Eeou&1d%4v<;0u`9s#%fKw##c[eZ7MOR?#;%=k'1KSm(-XcuL?rC*#'*@8%,1P/(ppjK:"
    "/wC_$EBjrQ>G/87mt^AuM5&3A98md.%oSL2UN:tL#Ag;-nG^(/<@(i<i/s0P15w(uCr4x8T'U#$/kt5:MC?##p,`oBAkPA',NnW&XL<5&vRq>#@E*<-GKCK(iwsr2j@#29t>X$##GM>#"
    "4:TM#$v6qVUpksQ4)q<$WuB)N@<arQ4/*2hm$&T.hdW#A;A)*/L2&3A71[e$mSKI*6>eC#3K9N(J%1N(Q;.Qg<Owr-KkAtLAY,2$.r?r84(U#$#]+N`T[Ae%CT?Z.lC5T'V/^M';Lm29"
    "d0Cj$>0P/(@<p6&cR$*N$ZpS%ToK)NtRjj$hF_W#/#5D(k:U;p_J>h25%eX-'<oO(VT9Z-K+,B#P[kgLZ-sFrJ7GB#1JUOVO0G[Khh8T%Uj.s63@NP&k2_#.pM?I%lj*?#E[R<$%]Am&"
    "1&Zp%Gh<h<E?XA#UM529n[x4p(3W,MVoN;pX.;W-P[nHHR]g6EcgQbuZbpW$at1#,KM+UDT:$3ViQ3$#``:<7DnA6&PmoS[?kSm&g8hh$[[%K:ipWL#m6f>#4%#B$Y1x+M8YU#AepL2h"
    "TV6D(Uppp%$xAOFImV]u7?q1&^P4Iuo.5)%S;)Ab7LgPqawwv7D?rc%]kI?7D?8Z#'^FwP0XnZ#]>WR'G/^M'D`5##2NSm(W'si#f3iNV1[%Z#*2rZ[MEY3'd0rZ[5.e8%v'ZPhaLes@"
    "C>U;pcP9^HSQ'(M/A'nf:u)s@+M:p7UvsY-u4d8.iRRUgXb8j0,KUOVuE5Se)+]lJWjo;-2s[B03f1$#?tY9%oL/bIQ74I%&3R<&2G#i7`vU*3dM<5&+i,29%vtM$Z*e>#nZ;BO8<pNM"
    "]j[IMlwSD&uDFsQ&bHE$isbG2/S(Z#-N^-ZHl+LYe-(?8,sH['r'QtUd$vM(K4MPgjeNYGRi,V#Tu_8.)D4#,>bsiLe?5K1(m)_7avM/L[u.jK5QqQW@rc29x`,t2)3n7[UJ-(#kC,29"
    "'&(N$4VR6WJKcB&g]0F&?DN*GB<a;Q<I<T%i$p6(WwdY#@.;bR`HcPhx*@H#nt[#ALI[#Afw^Gu*eXm2tS4jL3mmhM4esFrsXHpuq$MYG%KNL##sbUm)O<aY@Qim$'?8>,&D%<$6uQS%"
    "#KK8%&p($#%5=h8slwAcD)T+4V/FjLFGsFr$ErFr7LUOV(P>L%jj.s65FNP&I%6N)/p>9%scLv#tf1(&G7pe$H&o+MqnDQ7^#7/(AGG79e*m<.%G+F3Hl*F3KkAtLZ2%_daVR%+$1,90"
    "<iBS1&ZXsQSXQJ:mYpM#K78(O0b31>+o(nU/sUD3%rur-#8r328'>SnR(rWfhH3$#,S/C#5NWp'#Z439h85F%$6]N2QR2#GGna`aW;Gs$?PE+0sY#j#tZ(C$<.1`.xE;N3qV<f/+`sjh"
    "c&uaaWixfLM7V-M)#6X$=VoF#>QP8.$ivC#JXvddj]tr-xY^W$]'w##S'T#Q*FRZ#_7UvPx-.Z#nN.##re6?#ZEQ)Nq6G>#WMh>#N_kfL]%Ev@BJl>7c6&nf+9:phAlML;[8^;.r<,.9"
    "q;a9Dl;ISu$q[@uf1&F-I10o%p'b4LohHZ#KHL/LcChY#naI##N(6IQ^7+gLl1HwL0gaBQq+@cV=iiQM(7k<9U3?Z$$G.r8'w4DkfS*w-uJ_%4jkHiL-V?uuqEAX#Kg1$#Q7r,N?Hl$M"
    "OeK2'?^BM2A6oZ#Ch/q%RwlJ$2hIK:<>/872F+F3UG#Q8LS^;.3M>8.A>Ln<+O)d<E[C]$+4*<-kQVF61Yu##U7VKMqn?Z#5_YL%m9u>%Lb)?#qVTM#MSD.0#KOsQB9jrQEF*rMSaw>8"
    "U&Y5ppSM0A.*Q*IdsRO(hcoF-k:U29$JpEY/^:x7e-VC$DL$]/mb-Z#<?WS.5vpV&]8F*&$mLZ[_9OG2>[&6&HQ8sQb6G>#ATPq;)i.njZb_tC7f$<-K==R-SAg;-mJlD%jNwr-uw0#,"
    ":@x5:EC,s65_fi'9b:v#>FT2#RH+22K9JC#hK/@#&4@w#$uWx#:;q'#=r0J#v,rZ?@k2N9He,87Xp[i9i%6D<#1fu>3<?PACGo+DSRH]Fd^x7ItiQiK.u+DN>*[uPN55PS_@e+VoK>]X"
    ")Wn7[9cGi^hV)F3JZrhLY6W.N$paMg1KvD-Nf[fOt$%-M8,WT/q>K;-N,+t-;tXjLmTg_-qA7g%(3^>-^*XT/E[n-$;[(G%oJAT*Z?)T/B]+1,k=cYH&uuYHc1kY(:CP&#%CR/C3rw6#"
    "Qih7#bC[8#rtN9#,OB:#<*6;#LZ)<#]5s<#mff=#(DY>#7rL?#GL@@#W'4A#hW'B#x2qB#2ddC#B>WD#RoJE#cI>F#_qp=8-SCa4@vv;%GHI,MNtm<-f+:HNjrc=-s%q]S#`Hb7jP6T/"
    "CFv/M9=-W->')w%>%jY$L@;T/9e/J1.x>,<LGL#$#s;r((@jvPK^?5N<hnR8rv=R*cE7_8LLQX1iKvS/[_JW8,+eA0aHd&#e1Rc;`'3Ec]6SX30+%##m9k=.55T;--Y#<-((n<-vtvxP"
    "<ltHa:Uw9.%GIUR*N9R/fdb<Q['#t(YB<aN#Ec;RDohS8Sp(T/&e[^dCIpk$UFHENMjA#0f?v3+42Qal4.an9oGFaAt$70u=GTV-%Pvl)s9Q0)F?*H4K#g<Q7Ui39%_8Da-MuY#4(B>("
    ":G>/(Z.:7'E#B2'19e*NBHA'd]@u9'tj?*NTs&sQ6Bk>7KJs6p0#YY/53o237<K0Ab89g2tO-xI(I1N(Qs=)4BDm5/F$CtLCM[OMn+S#%(gvC#dR3Se%&fCS;iMg*HicY#,PIW$D:2Z#"
    ";Upq%D%+k'>l/,Mwg^?3]7h?3rZTfLr0<.3M%gI3A]QW-3S.JX^ifn$'3)^='H>G,RDmI5.3px=88k(E,JYY#cQx+2'0&##1vW8[rH6L,r9w0#O,^q)J0'?dxmS]=A)s]+lS2GsvN&QK"
    "-i_v#;Ke]G?XIab@gQd)SeDQ/V4YDkk1OW60=9/DxUO&urnPQ8]XbgL=^''F^_kgLn`?n&b*qdMas9h1rfQNq,ET*<%Z@0q:ja9[MDWfi>?s9)b:/R3KHb1#mNq:#(]Q3#AB#6#+eK*#"
    "=VtjLf4&8#wUrpL'kL:#b)TnL5m*<#X0&_Sdw/5#1W7iLg$6wL7je;#Lm>6#[KEmLphf8#ISa,#Eb1wg1888#oa2xLF/H50WW&###c$##AQk-$rN*##'Vm-$Cp+##t7m-$[^o-$,*+##"
    "Y;l-$d'M600KDM0bS`i0L(.m/a^%/1qf@J1iuhP/<q[f1O#x+2t$@02/-=G235Xc2`Gpg2O>t(3JG9D3:$fG3N*xd3%OT`3nYp%4IuhP/Wd5A4klP]4t<&a4`l$B5%(2>5sWF^5::iu5"
    "mBF#68gj/1UD.;6]LIV61Ter6CPP8.3_*87EE(<7vhES79QP8.Erao7gi%t7[.BP8J7^l8wL]p8#?#29^5T;-eQP8.eI>M9oEUm9RPYi9]6T;-4RP8.T[u.:fAWN:Ce:J:mnUf:jOp/;"
    "^vq+;A@T;-='l?-2UT401i)##9#$##NOj-$:R%##3Ro-$?g+##dHA3/Sc$##kDq-$Cd+##*9###lAq-$8B+##0F&##h-k-$@i(##]XV.0QJ$##.8)##P?:@-^Gau.-2)##$Xk-$2'+##"
    "'o%##n>(##l,Zd.ah*##Br.>-o5v)/53###:3l-$x[m-$/p'##WC*##@P)##]<p-$6CXMC3PNjC_iOfC'6T;-bWjfLG.AqLjY(7)amqThx_0#vc$.:1uMk]>Iu62LtEX]Y-QW-?4x$1#"
    "&<&REG-x$M9jw;#%xXhLdgjmLi9m.#<v8##+@(iL_H<mL1:SqLY+kuL$Ti].#4+##61QD-,SGw.P[&##V_o-$.,q-$VQ%BGPw%>GhVjfLNqOrL@*T.cWO`1ba<&snoqarL?(crL7q1;C"
    "^N(&P6`g/`_2FJGq'trL/hWPEberQNNAw9)nZvQN&Jv$#1Ko$M8)F<#K<_9Ma-*F.SMiqL)E`x.L7&##=Mn-$dvo-$6Dq-$3B+##GR;a-f/.F%/.n-$YWo-$,&q-$--+##hqo-$Eu(##"
    "%Qm-$K-o-$tPp-$K.r-$G&,##R*l-$Jw%##YK###Lhk-$pCm-$Bhn-$m;p-$?`q-$=^+##FRk-$f%m-$8In-$ZZo-$-)q-$3'+##*b'##)Z$##bEl-$(om-$SEo-$(pp-$'q*##6lj-$"
    "RAl-$6H+##W#r-$Ev+##Rfn-$)j'##kd###R_k-$j1m-$<Un-$LvhdNkQ^%OSZ#AO>5T;-l5T;-M[lS.Te>]OGVjfL[F+]BeH&sn2^DuLeeO]'Qs(VV>hjudk>$jqtKLS*ZcQ>6#NYQs"
    "I*Y9VcQA1#Yhe4#%_emLHq*'MI7->#^VSjLoS#oLMBRe-=t1F%vZ*##@`j-$K,l-$qFm-$LB)vQ7Y+vQM-D9.ZWn7R7aI;Ru@Rm/*`3SRqrj4SMVjfLI=fAgb;0mt0&,0-+ZrS]u5^#_"
    "XILAl@]ovLWM`T3(2H;@NLjfM$tpYZAcxvL$KZgdw=SDt>CaB0b'4kOdg8-vG0(6#Fw>:#H[tfLJ%1kLuxYoLGjqsLrgDxLGkw&MH1$>#k%KjLg$6wL6Y1%M2W[;#R<hgLL1CkL_IBvL"
    "*'i:#1t7mL2@]qLg=88#f%E-d]_G9r#h;;#xl`9Mv@epLXc<uL)af#M(qU:#XT$hL[6wlL3FfqLcwSv.?8*##b5n-$Cs+##DB'k.%6)##C3t//2V&##RNm-$/.n-$Aen-$SEo-$f&p-$"
    "x]p-$4>q-$Fuq-$XUr-$>a+##]ij-$(o&8#,Wu5#66]$#1m16#[b<(#E,c%#0SA,#.F2>#jK')##P^<#PX48#Y1i&#;%T,#EZA$#mTdE[_X[w0dm,5#hs_9#xBJ_&8;:-vH)4;#I7b9M"
    "YLF-ddEv^f)V/7#>1PiLDoT=#DS.sLh4j7#9/.*#N3rjLw.moL;5@i.Jt(##QxR#/P(###x:p-$UO)##aFl-$R:)##;9%##Rlk-$Bm+##8pj-$Sc8GjX+gGjo5kCjH5T;-X5T;-i5T;-"
    "#6T;-36T;-PWjfLY$MeTxQX]YUfBwKr,P&MXdB(M=Dk<#1mvfL4KihLFcbjLZ:n]-+[.F%'lm-$Q?o-$l?*##US;a-jh-F%crl-$sLm-$-(n-$=Xn-$M3o-$^do-$n>p-$(pp-$8Jq-$"
    "GP+)k`ku8.LSg@kRv@Dk6M#<-B5T;-T5T;-e5T;-').m/lZ,]kIdGxkW(.m/Aoc=l]w(Yl*).m/T+Dul93`:mtYlS.]=%Vm7LE/1<F@rm-N[7nmUwRn4uhP/Ba<onciW4o^YL9o:tsOo"
    "S&9loA/T1p5ME/1L9pLpIA5ipTIP.q8VjfLCG*tGCp:*Mc,E*M9-E*M#83eLDol-1V7l`kG2`*M^*vBa<teqHBPB4/>?s9)JI+6#nV`Ee2uTk+1Q,3#`uHqLg:/8##_#tLLIB0/^1C>#"
    "wck-$1LE>#OVB>#'0A>#q^@>#nQ@>#<gA>#TU?>#emj-$i0E>#wbC>#S>:@-M/H50<S>>##aF>#6*o-$ZZo-$E%D>#Qxl-$rXmQ'h]oi'LpOJ(B?u`3/#lf(c+1,).5LG)/=hc)LF-)*"
    "s6T;-K[lS.7QHD*C5T;-o5T;-C6T;-l6T;-D[lS.2d)&+(YlS.%kDA+fYlS.dt`]+>[lS.>2A>,Y(.m/V9]Y,DK=;-wYlS.%UXV--[lS.+`tr-/PP8.(g98.MgMW.-pTS.-6T;-bME/1"
    "X$qo.r,65/B5QP/FVjfL_EWdEgc45T(%.kXfmn;##Rl9#/B%;#*wC:#,[#gLK+:kLI:6>#2j2_AU;#$M19.;#R7cjLnMpnLY5kf-'Z1F%9Mq-$7QF>#Oak-$pCm-$Dnn-$m;p-$vaE>#"
    "7kj-$-BoJ2+N#<-[6T;-EXjfLiG5++M[fh;HlqlKs<=AYHdC5g.AgkLsQob44$^E[RO_E[e.v$MEDk<#KSJiL9*:*#T1>jL3^e;#CsDhLSv56#A_]w'WrM'#?tx(#_T2wg8k3$#B`p#M"
    "+-r:#?H?gL&hL:#^9q^f/hO4#=W;:#rg.lL%Q;a-&w0F%2kTs7@+0580bT88hN(E3m2KP8';gl8DD,29PMGM9UVci9qcvJre=IKsEw7nL]^Nn::gX?eDiV6h;iF_H(+U^uVYi'Qn&t-X"
    "KEonLc:lt]#.r_?E_Q=J'K[%%ihPEnBKG4#sRJ+#BY?wTq76-vP;M5#?nk7#oc?rLkRS8#drwgLG*k4#=5plLs8.:0?m>>#&#D>#UAm-$8TF>#uEk-$?k@>#KeF?-b@qS-Ki_d-Mncw'"
    "lw3TAS/EPAXS)qA48alAYC&2B$6T;-Q6T;-3XjfLAUK<91wRH>i,C2q2&S-#(A)W@ao-qLYD@$A&`Ft9Xv/_Ie^+poLZ#4*DFP8/]v;;R0n]E[7+V#Mv'8+/S,?>#=*l-$vmcJDriu8."
    "27UcDYqtfD_<_,EUG6DE0Y]GE:87Q/3RQ`EoYm%FH%P*Fqd2AFklM]FUuixFW@5&G5D'BGP0JYGLYlS.`:fuGEVjfL?kuf,<PJls[drlEbA&WR-l&d6#NYQsIq[1#_QK)MEu^=#+plpL"
    ":qOrLo^#`-K>3F%)'F>#Ddq-$rvE8Ju6n8JO.?PJXVjfLk.PgPN+*-?t=+&u%bpsLv3EBS9ecE73X5,EgoX5TqR)eHj7hMU)$?tLvRM$B=E.&GlR),W.j%9]+3d9MbmZwB<T*#Hh.hiU"
    "=_NYdmG$Nqd&*=C-<dtL`8Y-vt]m-drG$1cWYNLYcd_CA1U_Ft>s5.m)]=(fF_7cR1H?]KRoPY:ecuf$(J`SA+ZrS]6l2#1U*5dNEEdM5dK/WnH``fm;;8vLkHQg-W.H^9:qZw'pmA1#"
    "9-30#%]t-#e]/jLOLXR-uX6E0n0>>#AXB>#bIk-$xZE>#l%A>#iXk-$6EF>#dY@>#.n+G-^43U-trO,/o$G>#LNm-$-(n-$s@%*X7_&AXUgA]XUVjfLU-m=;O_CxLc#Df@QkUxLjCGu`"
    "FYn.I/Oq'u2K2)';DkY)C42p+@l#W*B%Hs*FX1p/E*:kFwm*0#iTT)#:7:kON3#5#N;>nL1T[;#a-dE[I>,F.'.:9#I6i.#Lf=_8evd-69T<;#L[o^o9@+qLXc<uL05P$M`PQ)Mnei*/"
    "?O>>#CFl-$&im-$`vA>#lKwA-g.OJ-FrCl-0Tbw'6QF>#dpBo-ca.F%'lm-$0BNJ`fN#<-8[lS.)/Cc`>5T;-sPP8.d6_(aWn-,aPq8gLHE,(1CW]eAskNw9?S1$M/-r:##g@hL_H<mL"
    ":qOrLj6QwLr'>9#iWPfL=-lrL<(i%MClT=#>PIwKq'm-#LsLk4NR*3#u?JpL&NJ#M6Y1%MFen&MVpT(M=Dk<#pscfL03DhL@>+jL>4%&MN?b'M5jw;#iFvnL(YVpL8e=rLBLI&MRW0(M"
    "<>[./@.G>#Bqq-$TIr-$9WF>#sPj-$X]>>#02k-$.[E>#/CC>#&%C>#+0@>##k?>#a_l-$1?F>#+hm-$<aF>#0tm-$6:tPg?A)Mg%[lS.aJDig77T;-NwhP/2T`.hq[%Jh-1pMh3g@fh"
    "9ZlS.#p[+i1*.m/nxwFi%+=ciA6T;-i6T;-8XjfLg:nn$52LX4&8)s@MCNJM)T.5^TxxcjYtYR$J'I21s;4&>HlqlKxjpuZ`0N;nf29+(]u2v6/4tiCfr02U81r%cHF?-dBi)&#*;0xL"
    "T$61/S%G>#]%l-$Y6DDkXN#<-2[lS.Fc5]k<5T;-qVjfLQd>1S6]#,a)-@k=8)h$#xJ2wp`Z#qLtc&V-/jj'/?5F>#K>k-$-n;AlAN#<-e6T;-:[lS.H(2Yl25T;-iVjfL<QBoQ4JBJ`"
    "Bd4R*I+3&#,4[nLDWUsLw/sxL*'i:#<;3gLK+:kLuxYoL<2il-:r1F%Erq-$CvF>#]fk-$m:m-$Bhn-$sawYm;LIrm'5T;-V5T;-@WjfL$/cubgWJ8ouC`D)ZcQ>6.+XMCWH_]P+gel^"
    "XF:&lk;=m%NKaJ2(J`SABd4R*nBa$MdW?T-9omt.H3@>#K'n-$UKo-$)sp-$&wE>#A8kf-qU-F%&im-$tv/8o@q8gLWU(?+X;&H:80i`FbMooS5lu(bBd4R*Jl?%#)]CmL6b%Y-;]1F%"
    "Doq-$H/G>#s:l-$f_.5pWN#<-2XjfLKLB9$?PGH1tDOA>GcUPKq*]`XC?GSfplVGs36D9-fq'm9skNw9Ns[vL7Gl$M2W[;#Q@0hLUh?lL(YVpLPJntL#</#MxKu9#GVwfL-,/b-R</F%"
    "VNo-$.,q-$Bu.jq-b:fqd5T;-&6T;-KWjfLMfI?X5S^f`Iv?MhfgF5pscOq$P^A,3(J`SAeC9-mHnA-m$%m;#hA`EeU7imL@'crLTc<uLmHmwL:$JY-vH3F%3HF>#'Zuk-ir,F%Q>l-$"
    "&im-$LD`fr0t:T.T2R(s.5T;-[5T;-56T;-e6T;-:[lS.[;nCs05T;-Y5T;-:WjfL)uEh^[Iu`k'YZO&S#>)4(J`SAMCNJMxjpuZNA*jhtYS=#HkhP0s;4&>DGYSJpkM,Wh@YI:w`K>?"
    "j@HJVcN]SoMS7+M9;;c)ccW48/4tiC]v;;R2P#,aF2Lk+3sBkF*#3)#ZPhpLi76L-YXjY.*FD>#)*,k-w*3F%>mbY#sVk-$wXm-$ZZo-$02q-$<gbY#qMk-$exl-$6Cn-$^do-$B#cY#"
    "]em-$H5cY#'mk-$m:m-$M3o-$.,q-$/?bY#h)k-$VMl-$4=n-$_go-$3;q-$0BbY#fvj-$RAl-$+#q-$;Sq-$'/Bv$?.hv$mf-s$MNIv$dN#<-;[lS.mqH8%<VjfLGo'QAVt`m/s;4&>"
    "j@HJVC?GSfMsiEI,^$(#hr_pLrqDi-$i2F%=jbY#-gk-$rIm-$K-o-$'mp-$.<bY#u8k-$98Gp&DN#<-vZlS.`QA2'$5T;-U5T;-16T;-c6T;-B[lS./[]M'WVjfL9(6$Gp.^wQ]CJ3K"
    "&Z70qWMcCFg(G4g9iHwBmo4kOkPs,#7He--J,Y$M4NR;#;hjEI]?CsLoU]8#4x:kFem-tLY)?6#[B<mLI6'5#0<VhLCQLsL+-r:#kA^nLKB95#.*2hLEsQ+#6WrhLL97I-(Z,)/OL`Y#"
    "<Hk-$*/_Y#Jpk-$h<aY#+fm-$g)p-$<gbY#o+m-$[n`Y#Hak-$RBo-$?pbY#4xm-$Fuq-$O2cY#UAYY#n-aY#]&l-$e*aY#3i?>#(CEJ1niWSH2Brs'_Yvo%L,)=(LMKlm86<F%e:xU<"
    "Kwm,vn5;0$29Lw-_7XcMn2sFrM>o--vH4aX7xef(T(Sc6HJ>f_,@/>>$nel/iH*>GI^',2w>>G2xm*20u+d'&SjYc2w,BJ1SV'vHH_1p/0qG59`DOs%N2/L,fU,8772%a+kulD4*5Le$"
    "NuAD*(['a+ipip/d$IP/q[s9)+-Ke$OKqE@C<f%F;FRS%='+/1<`l-$/;pu,iePV--KrV.kPQ;7Cc[A,7xs-$iPDV?7,fW.mSLM'BNCG)`FfS8G4_f`3W,g)(n8QK/@sl&Z;8_8GsbxF"
    "B+dER3JhERgBw:QCktA,Q:+L5JQ_r?go(E4*j))*Cf]&#:-+_S1mo?9fM:c`36Sa=Oq1GD4b?']@0g'&t:L`E#SKAGS%uoSH6O/)S]Y/VDFZf:>b:^#e2a9Dmrj?B/E^.$.:6qrLjbD4"
    "@>w-$%u5qr:(GcMX0nlLsr1'#,_R5'[b=#-14Q21;jcG*DuE#-u1B;-?QQEMHuBKMW8,&#GR1p/^f6#6aIx-$HRl3+Ut+_JPML.$vjvE7^,YD4e6Jj(>>>_/KV?/$,JPV-itX.$VGR]4"
    "TCbWqtakEIQ@,W.0-.L,O^Yc2::,87hq'm9XE_.$i5i*MX1CkL@V5s-@*,)MRE3mLJWKcN)%f(#g'wT-CG5s-'XajL(DT;-,+EJNE]XjLc'6s-sRXgLPAVhLc<PgNVusUMVndi2ZIf5#"
    "SN=.#@]+6#XQD4#o8v922qe5'<_H9iT(f?KqD<MB8-A_/1DTfClQ*>GvcgY?C^J:)OxfKYO)###(Duu#>N$)*$]X&#n@nw'1v$`&S.no%(]F&#L/P2(S=]A,T3Dp/wu^e$[U9]X4<=Q'"
    "ju`c)8Na21G04eH1cFcMfYfCR%As7#jE24#F]%-#`jTK-2>eA-3i>%.R82mLaJ/C8Q5:mTT;$)3bXF59@n:'#CU,87iA]=uGe+a+M?4j('(g#-_:pu,hnj)+ed:oe;'<s%>fW>-pGfM1"
    "j47#6e@%K)lki'&KCTq)-5TY,C4Nq2QfrDP=pIiL+ll&#)?<jL&x1'#QrY<-_G(a%=wP'AS^BJ1<B(a+xR2*4v9<c`@U')*TcjKl&cW-?1evcMT%1kL=^tJMQjuA)VPMv[,RGgLv't78"
    "v1)F7Lu'58<4`D+d=-g2+.3j(r$o--3Y)R<h$S-H%x%/1C>dG*<cb_&Xr/;6*vD#-p#^D4b0k_8ls4D<3JqY?OWwE.;=#+#Y_$N1KKvE7;k]f1s$1RND,xE7m,[F%gTCG)7j.W.j/4L#"
    "SMm92+<df(a1&:)k4S]4-`9emQ/5-vK^Yc2WU=#-)<As%/fxgDjmBcVm(f(NH(HL#BP:w^$af--gV>_/*x-eQRBFM0'Ub?KffkKP#1WV$)F;s%Xw,W.CakKlMnLk+_'Uq)ea?D*1v*w7"
    "f,p%#`,:w-uRXgLY+:kLIQrhL7?+jLO2M'#>U4G/xj9'#C*uoLfL>&#>VPLMuC3mLE#T%#U'3JM)>`'#idF?-:YlS.8'U'#=2^V-Mt(F7)#h9;8ou^]DB2_Aq;KP/;vrV.0O.g2+SI)+"
    "kFiJ)d%]3k*&9>,^B1_AZX=X(R&cD4F@pV.`7tV.)wiM1:>uP'FF`G3Shf;7n;4L#MU5R*qGfM1]=x-$2@3j(#S<a+?=Z3F#b02'x8v-$gCD'Sr&J_8JXGcMasb89(;YD42*oP'wxxV%"
    "Z>`X:bJ)kbv.J?pfvI21Met-$1m@kXUVWEe(@;s%<65H*qR=A+=BgE[$g>&mPotjL^:#gLSp/,M;WR2.?3*jL$6$##'DHL-,OD&.U?<jL4[b)#$Ln*.GV5lL)RGgLWLr'#>+<M-_dF?-"
    "+5T;-+G5s-,WajLJqugLxI<mLbB3mLFJ=jLm:MhLQ6N$#'M@+%OP-F%,L&_SY*,_J3Nw%+2?PlJPC$AF/O'JL5V22'jm;_8;*h34b*kEI1A9>,$;qG3'3qE@3LUuYJe1p/P-_.$5r>7v"
    "?i4:vqo2%voL-,vf(O,Ofp@`EBcI%XOHBxbYnF?Rf^/jMa7s7@on(VQ*g<o[b]WCstj'kDOXfrLg<-JCrWsLT;%2f_sk1uu9(hWJ?)o.MtI*MKb>ZSfTV6XUZ%7f$g%:fu.lPjuQEsM-"
    "MrjfLwOUau<wcjuU>:nuPx<cMFZ)>M)KxRM&SE3MO#u(2l0vjuus5ruZWD8vfRs?Qe(w'v4@vtL/-kkuHg#ruX(o2Og4w##>AF*vcZu##KEuwL]X5.#Gi`Q2I*;P8&Zl(3vFFS75G,AF"
    "u?//.'L?IM5=f(#)_B#v<UG+#5:M.M)r('#7c-2M`3E$#?`GlLX)doL)p('#DQDa/&xp19P?T`3$[ghM$-MuYL`bFi2Trx+8>Ro@[/e8Th%55(&+AJ13Vcj1c_8S@^;c'&n9vLgt=F&#"
    "GQCPgI'@@'E6QfLfixh$X3CI$U&###f@[u>osqFr/t9S@A4OfLYdeFNV_-##tM]t#:CP##9T_n#gSV=-b/Vp.4.&)3a:]q)pqap%1]1v#`wp:#v^Y+#D.)F.o?:q8H?v)#C&>uu[^e#M"
    "E*4%$9iUxLHPEk>.5F^#fZe#M4KG&#<4u(.Sun+MZgkl&$D:vu3.RS%E4,qr)'Ke$'8f+MUw2$#2x(t-hRXgL/'MHMX3N$#3fG<-9FwM011`$#/s+>#qk'hL4E%IM^Q&%#8x(t-vw9hL"
    "fp8%#2mx=#$.LhLbjJ%#4Ag;-<YGs-):_hLk&g%#<x_5/U[ewuk#*JMh8,&#@x(t-2R-iLjD>&#<SGs-6_?iLrcP&#:af=#:kQiLEVkJMocl&#Fx(t-?wdiLqo('#@Ag;-O4@m/Em9'#"
    "?Z]=#S:@m/rT0#vG)U'#EAg;-PYGs-NKNjL/]r'#BNJ=#RWajL%V.(#IAg;-UYGs-WdsjL.iI(#KAg;-_FwM0OfZ(#GHA=#`&BkLY$wLM-1x(#Wx(t-e2TkL5O4)#JB8=#i>gkL_BNMM"
    "2OO)#[x(t-nJ#lL4[b)#[x_5/Dm)%v3X%NM7n'*#`x(t-wcGlL9$:*#[SGs-%pYlL?<L*#R6&=#lYGs-*&mlLDBh*#`SGs-.2)mL@N$+#h4@m/ZS/&vdK5+#bAg;-sYGs-7JMmLKmQ+#"
    "fSGs-;V`mLs`5oLBJb<#hxf&vCv'PMJ5*,#px(t-Do.nLLA<,#jAg;--GwM0n>M,#_$a<#K+JnLPYa,#oSGs-O7]nLRfs,#oAg;-3GwM0sc.-#ctV<#VIxnLV(B-#tSGs-ZU4oLX4T-#"
    "tAg;-9GwM0x1f-#gnM<#bhOoL]L#.##TGs-ftboL_X5.##Bg;-?;@m/=@i(v(]O.#'TGs-o61pLdwc.#'Bg;->ZGs-uHLpLBqOrLED4<#JeI)v.1:/#p[2<#&[hpLBEUSMlQV/#5#)t-"
    ",n-qLn^i/#0Bg;-L)`5/Y94*vda5TMrv70#:#)t-60RqLt,J0#5Bg;-NZGs-<BnqLT8nTMxDo0#?#)t-AN*rL$Q+1#9Bg;-X5@m/>N<1#(Pv;#VZGs-JgNrL/pX1#?TGs-NsarLVc<uL"
    ";pe;#%?h+vD#'2#ABg;-^ZGs-W50sL6DC2#CBg;-gGwM0HAT2#/Dd;#_GKsL4]h2#HTGs-cS^sL:u$3#2>Z;#gZGs-if#tLmtGWM:+I3#S#)t-nr5tLBI[3#68Q;#r(HtL>Cn3#OBg;-"
    "oZGs-w4ZtLGU34#Y5@m/MVa-vVRD4#UTGs-)M)uLsg`XML0b4#<,?;#.Y;uLv#&YMI0'5#`#)t-4lVuLw#axL5K.;#`1T.v_9J5#^TGs-;(suL'NfYMPZg5#lYwm/iI#/vcWx5#bTGs-"
    "D@AvLTs56#bBg;-/[Gs-ILSvL^/Q6#l5@m/vnY/vi,c6#hTGs-QexvL3A([M]M)7#p#)t-Vq4wLel;7#Ljp:#Z'GwLafM7#nTGs-_3YwL:lh[Mdxi7#&Zwm/6U`0vsu$8#rTGs-hK(xL"
    "h:88#rBg;-B[Gs-mW:xLw_S8#T^^:#qdLxLDRn]Mn_o8#)$)t-wvhxLGe3^Mqq49#/H@6/NBo1vWnM^Mt-P9#-$)t-+97#Mv9c9#(Cg;-T*`5/YaF2v]</_M$R1:#2$)t-6We#M&_C:#"
    ".UGs-:dw#M.'V:#bKB:#>p3$MWpp_M+'r:#8$)t-C&F$M-3.;#:6@m/pGL3v70?;#4Cg;-`[Gs-L>k$M8Q[;#8UGs-PJ'%M`D?)M+e(:#'m-4v=Z)<#:Cg;-g[Gs-YcK%M?&F<#<Cg;-"
    "pHwM0A#W<#n9':#b%q%Mk7saM>Dt<#H$)t-g1-&MD]0=#r3t9#q[Gs-l=?&MIcK=#FUGs-pIQ&MK+_=#u-k9#tUd&MG%q=#HCg;-x[Gs-#cv&MP76>#P$`5/%/5##SxefL_^/%#V(4GM"
    "Tq`;$3/r5/1*[0#d`qpLrsR1&MP8>,)bfc2OZ*/:ubV]=;a)2BrcZiKI^arQYij]+?oe]4_FhrQVpcuc$_e%kl$()N@t$#P[CacV.mNS[m1+5fC/&jBEjn%t.Vc.r>$SA,Ihof;h`aY?"
    "p:#s@>`do]ON]lg-<WlpUeE]u=e`D+Ns9v-e_bJ27P-p8M('sIDA>>ZrNoo]1qZ/)HaBv-eR4j1rt<^#'5Fm'9:?g)JN4^,0SV2:-OnMCpjT^#4c'3(DnVd*XGHW.#_Qd3l3FdE4%o;R"
    "I&=7vY2At#@8aX,uuoXu2-bKlMlQS%=__A-L&-)3QgEJ:wnrx==mDMBtov.LKj&8R[u/#,79B&5s=Z]c-6DJi0JiJMk)>-m2M-/$_v@1$0vk2$KU:;$UYHG$k=7h,TFpu,DZ=#-bv:1N"
    "gt&*-C:>3-TQb6-2Pa9-np7=-BN4>-w-LS-<@-5.v]6*RV7V1Nf/;hLt@'cNx.O?SOV#oL?X58N#A/cr(,5^tl7mx3t]x9vS6d7v8Ug6v3b#7vW(>uuP7T;-^&Y%4NmN9#hY&*#Hb0'#"
    "1/iZuptxXu#s.>-w.fu-bDpkLGfg9vnCg;-a,(V%Q?%VZJ62]OJ+-MJ9NGif1t/Pe*L3Sd&4RrcwhUubrLu=bl(#AafY&D`jach#t#?l7>*%##fD77v.oI@swJD%j/S@;,nP7YF[S4lI"
    "Wv%##gwB1Spu0F%d]ukkFvDkke..gkm;AdkH(Ttk3Sjskn;qqkfg0qkEh[okDCHT/s'oOjhU?_)Ci(q&YsM4#LBZb:I[S'JEx6I$:C(C&IwJVe9(t-$LI3F%#P^%OVjg_MY6W5/GJ:7#"
    "1%6]Ma(^GM7f^;-4B)=-cqmJUqg[QO[F[]Mm$Jp%w^XoI[Ex7IF*=A=xqrx=&4SY>*L4;?BmHPA>pDMBB2&/CTrAJCtRblfO0X`a`;2;dTEt%b]ppxba2QYciPmuci]MVdn.foeaB&;H"
    "pB.AFKGWiBZt)>GEMo+DSeDYG^Q=SI+Ss+M7Fl%O;_L]O?w->P/lScM3.5DNcLn+Vg0,2pn]kuudqJPov8[crh8oxt^LNSnpj_fqbjr%tW(RVmjEcipjD4>u^F38nn^CJq6uh;$GN*,i"
    "'?%;Z)K@VZPKOG>o?W.=_Lqk;NY4R:>gM99.tgv7t*+^6d7DD5SD^+4/Ll31ffHW.Jtvc*h>Mk'N'tp%=47W$-AP>#c/Q?.E?TV[_:+N$RvZ6:Y42X-:?$=(?pm=(w7%N-t?cP9P8[V["
    "tr-A-X<K%P/I<mLbE2eP2e8jM5*5gNhZ6/(gL0;67/H59:>1,fd$,2_(8cC?e&%BPYs`D-Qr^_&v6X_&Hj/KM4klgL=#(Z-W%&9._1dID9_,Q#D^MZ[(20LO:xYQ1caDd&,u-T8A5k31"
    "?/ZQ&0hrS&2jDW[;sE%]6qtD#/@q^#'>[u[:tx2'`.079A(u59?L?s<V5*t8/tqY?1%P&#ZhP[.#Y:Z[rTp$.5u:#MD`@]O7cxdaX^gB(gixs0qW[2C?0R][>/Mn33GC29q,Ub[9>nD0"
    "e4oD0sR#M1U8s^[QB:99Oh4jL%31I=6GbB#dN'3(<nmqr/Nu0.Nnb)M?N)=-@f@U.TFldrFNi.MvmXW%XU,2B+]F&#fe:qVsc.Z-)&Vx0R(q)#U57[-SWdC?Y'q?-S8^7]i/dn0:SplA"
    "KOZcaOJ6g)5ir=-xw*cN]`FB.K0G0.sH#3N/peSD$^wZBMBkW[)pbh#+3)N$K7e59sKN5/]NeQD3+f5B%UKp8%5S?-n/mx=[wTs8eFsV7n>'%%n'6Z2%HU[-RFPBoSbDb.BabC?A&K&G"
    "M4g88Y#&gLt6gZ.J]_h=c>hhMUrMZ[+jh5#MW744kn*79K&w596ZCHO0Z,<-$CY)3;=aC?f[d$v`Bj@-%uN(#43:5M?&i;*5)<tUviCHOLG'_[xqh1CfdfpC-PHY-t7Z][WH_A.@Q[]["
    ",eG?.Con&lXo<Z-A8voL?41lLBRqHNKfZe1^bMW%d-m]+5.Fj$@Jw<(-nsmLY]g88s$f:.eTNp.>B#b[PR'?-Bun%#d9+XCWEm'&MDuDOr1M9.,^-6:`4fV[^ZCh5C?>e+ZNN-Q+Jx,R"
    "OaGnaSd_V[hVtJ-^DtJ-`jUtLt84)TIhX0MwTU?-CV%0NS42X-,wO1>,c,UVGc?mX4a=nE;<fKu,_NmLbM>gLH`?6:8B.^#F(X>-C?3W%@fPEnC%5K1:0Q?.lPg=-D)cC-',uB.(#>F-"
    "h:XC?94;^?^_3b?*#ZQ1g7'A.sS_D?n$'@'ED(W-]9-A'v=;&5VE2(&@LZP8rx.a9).7r7:.nw'tHpw'M#n31/wC2:9*`M:WrHh,Qc;c%p0pIOd:X;.)2'(#s`vZ[(dQ][1/HDO'GvhM"
    "sHqB#3C+tAg'QB.e8cP9sMx2vB$5(]IqJ1GlqXe5'JHDO)<`KO.PRD6;8voLjGjC?dCLsL`X2W[,F>F#2GGQ1XF;##;m-B#D3?g7Z6W%@E_f$#vZ=1>0PV>#k_w?0u1dID;abC?wETB-"
    "t8Fg<ea`PDVr5v86Er&#g<*p.5*3D#6,Nv6N*I8.lRiY#EsvG&Iq4Z[mgj2MsKN2Mu^/jM9IG,MJ<s8.-Z3)*<CGS7'?m=06Vf:C8f+;Cw-:90ABG_&9A]:CeJ_d+6Fn-$?sJ<0tD`9C"
    "EYOk+U_QT'lv:9C=)8R*#7d:0G$fj)S_x7C5)Ok+%@F_&P:S7C2vNk+pCm-$ZlA909>V6C(WNk+6p16CrWu9)SDqv7pF):8D.=JUl)K5'_v[`$Q0),#oj;k#kLW?>c43?>Zrd>>NM->>"
    "_(w>>RY?>>/)xQ#C#c@#80uQ#kP5h%rm0[[2sm][-3Y,/]l.T%1QB%@iNYb[.%[A@/JM>#tO3LPfd;E,:+Y`=?YDO,N^XAL-:L'#^jF%#kd?n0(m+1#?kNe$g1:/#;8WX(9Lcb%_X8F."
    "T(&e*+Of5#@YF.#0SP8.o#@(slG?/r]lql&(r9#v)0cu#glk&=;TF&#BaCpJ[4TfCq#/d<,UpBAS+R`<na/F%;Rn-$HPlSJ#Do3+G=UfCA9dGEF5Pe$NDYSJS1M5K8_6.-<:0i#SF?YP"
    "G%x@X55G&>na/F%W1,4+`U-P]s5Sd<w)S>#8G5L#g4OY5o6LwBKBeS7A(JiKA.AYc=Qe@t$m>X1.KGM0St&;?#>gr-<h_xOR48DWop*v#@ww;6d,Lf#X+um#_3ED#$Up2#1bId#ETR[#"
    "o+L+vecqn#vkZg#_WI)$=i/*#`;hf##@i(v7WR[#K[T:#EAtV02(V$#67>##_RFgLf7'vL0=`T.:R@%#x4xU.-Sl##,B`T.a+3)#4B`T.G9F&#oB`T.Ahn@#8@`T.H6OA#P@`T.V0wK#"
    "krG<-:X`=-@tG<-;L`T.dfmC#kg`=-:(H<-bQ7I-cQ7I-W-nA/+mk.#:ABsL3h5.#HB+.#SreU.0E-(#S`:5/1c/*#vS3rL,(d.#sg$qL=RxqL/<TnLx:^-#Tlls-jB4OMB(8qL3F;/#"
    "pm-qLN3urL5x.qLCwXrLE-lrL%)8qLM8*,#v`=R*?kNe$VR?M96CH>H%#sfDgdJe6ZpPq2W=rKGnZK'JK&xf;gf&aFD:.F%>=l?Bl_X>-cqk>-G3j9;+7auG&W*aF$h-XCL%G-Zluw/?"
    "lDun40'jM1vXE5BFo@2C<OOd4e(B-#.f9DM24JqLVA;/#]Pi+8IcE>HDUA5Bd+&NCjci+MZLoqLR4oiL4;b?S>JIF%7C8X1&e3X:We(g;61S5BrbU,X6:SqL,RD/##-3)#L2^V-uC<L#"
    "YP+XC-d*DNv7J7MOJr'#AaoF-YbOZ-T?*1#wH<1#7S,<-6[$#PQ0Js$<s_s-(5>##,AP##0Mc##4Yu##8f1$#<rC$#@(V$#D4i$#v-LhL]'&i)P5@D*TMw%+XfW]+](9>,a@pu,eXPV-"
    "iq18.m3io.qKIP/ud*20#'bi0'?BJ1UM%XLbpYc232;D37Jr%4;cR]4?%4>5`asMpE8E)#IDW)#MPj)#Q]&*#Ui8*#YuJ*#^+^*#b7p*#fC,+#jO>+#n[P+#rhc+#vtu+#$+2,#(7D,#"
    ",CV,#0Oi,#4[%-#8h7-#235##9e`4#?-85#ib_7#n%.8#rJ:7#5pF6#:d46#aSE1#4^+6#4ss1#tPC7#<A:R#uOD4#-G31#xX#3#=N<1#=lj1#kv%5#x?G##3H9:#:F^2#`5s<#H_,3#"
    "phh7#I)a<#m&/5#d;&=#hL;4#E7I8#D?S5#>$S-#n*]-#T1f-#b=R8#kIe8#kUw8#t7o-#sb39#OE]5#&xV<#60k9##+b9#xX)<#EaW1#LH4.#QaX.#Sgb.#3nk.#Zst.#_/:/#c5C/#"
    "gG_/#1Oh/#mSq/#rl?0#X%Y6#D#R0#x([0#<`2<##<1R#0Su>#4`1?#7c($#w+GY#<xU?#?%M$#%2>>#D:%@#75G>#JL@@#NXR@#Ree@#Vqw@#Z'4A#^*+&#mcoX#c?XA#gKkA#kW'B#"
    "od9B#spKB#w&_B#%3qB#(6h'#L@9U#-K?C#1WQC#5ddC#9pvC#=&3D#A2ED#E>WD#IJjD#Dx,;#HX&E#Qc8E#UoJE#Y%^E#^1pE#b=,F#fI>F#jUPF#nbcF#j&O1<1]Cbt%HUV?0q08@"
    "43ho@8KHPA`e,Fl-wcF#FB4I#JNFI#NZXI#RgkI#Vs'J#Z):J#^,1/#HKvV#cA_J#gMqJ#kY-K#*&?V#$NB:#3[T:#:q,V#srQK#w(eK#%5wK#(8n0#^Ul6<IC2RqS7$iPEE*;QI^arQ"
    "MvASRQ8#5SUPYlSYi:MT^+r.UbCRfUf[3GVjtj(Wn6K`WrN,AXvgcxX$*DYY(B%;Zk7QlSg:2MTmsZrZo*$8[%b^%XR`h.UZ)]uPM&ToRHq<`aBbW%b6A9P]:Yp1^>rPi^B42J_FLi+`"
    "JeIc`N'+DaR?b%bVWB]bZp#>c_2ZuccJ;Vdgcr7ek%Soeo=4PfsUk1gwnKig%1-Jh)Id+i-bDci1$&Dj5<]%k9T=]k=mt=lA/UulEG6VmI`m7nMxMonQ:/PoURf1pYkFip^-(JqbE_+r"
    "f^?crjvvCsn8W%trP8]tvio=u$,Puu(8P>#,P1v#0ihV$4+I8%8C*p%<[aP&@tA2'D6#j'HNYJ(Lg:,)P)rc)TARD*XY3&+]rj]+a4K>,eL,v,iecV-m'D8.q?%p.uW[P/#q<20'3ti0"
    "+KTJ1/d5,23&mc27>MD3;V.&4?oe]4C1F>5GI'v5Kb^V6O$?87S<vo7WTVP8[m729`/oi9dGOJ:h`0,;lxgc;p:HD<tR)&=xk`]=&.A>>*Fxu>._XV?2w98@69qo@:QQPA>j22BB,jiB"
    "FDJJCJ]+,DNubcDR7CDEVO$&FZhZ]F_*<>GcBsuGgZSVHks48Io5loIsMLPJwf-2K%)eiK)AEJL-Y&,M1r]cM54>DN9Lu%O=eU]OA'7>PE?nuPIWNVQMp/8RQ2goRUJGPSYc(2T^%`iT"
    "b=@JUfUw+VjnWcVn09DWrHp%XvaP]X$$2>Y(<iuY,TIVZ0m*8[4/bo[8GBP]<`#2^@xYi^D:;J_HRr+`LkRc`P-4DaTEk%bX^K]b]v,>ca8ducePDVdii%8em+]oeqC=Pfu[t1g#uTig"
    "'76Jh+Om+i/hMci3*/Dj7Bf%k;ZF]k?s'>lC5_ulGM?VmKfv7nO(WonS@8PoWXo1p[qOip`31JqdKh+rhdHcrl&*Dsp>a%ttVA]txox=u&2Yuu*>Y>#.V:v#2oqV$61R8%:I3p%>bjP&"
    "B$K2'F<,j'JTcJ(NmC,)R/%d)VG[D*Z`<&+_xs]+c:T>,gR5v,kklV-o-M8.sE.p.w^eP/%wE20)9'j0-Q^J11j>,25,vc29DVD3=]7&4Aun]4E7O>5IO0v5MhgV6Q*H87UB)p7YZ`P8"
    "^s@29b5xi9fMXJ:jf9,;n(qc;r@QD<vX2&=$ri]=(4J>>,L+v>0ebV?4'C8@8?$p@<WZPA@p;2BD2siBHJSJCLc4,DP%lcDT=LDEXU-&F]nd]Fa0E>GeH&vGia]VHm#>8Iq;uoIuSUPJ"
    "#m62K'/niK+GNJL/`/,M3xfcM7:GDN;R(&O?k_]OC-@>PGEwuPK^WVQOv88RS8poRWPPPS[i12T`+iiTdCIJUh[*,VltacVp6BDWk[e+VvZ>AX$tuxX(6VYY,N7;Z0gnrZ4)OS[8A05]"
    "veMiT>f,2^B(di^F@DJ_JX%,`Nq[c`R3=DaVKt%bOf4l`7Sa.dgVMVdko.8eo1foesIFPfwb'2g%%_ig)=?Jh-Uv+i1nVci508Dj9Ho%k=aO]kA#1>lE;hulISHVmMl)8nQ.aonUFAPo"
    "Y_x1p^wXipb9:JqfQq+rjjQcrn,3DsrDj%tv]J]t$v+>u(8cuu,Dc>#0]Cv#4u$W$87[8%<O<p%@hsP&D*T2'HB5j'LZlJ(PsL,)T5.d)XMeD*]fE&+a('^+e@^>,iX>v,mquV-q3V8."
    "uK7p.#enP/''O20+?0j0/WgJ13pG,272)d2;J`D3?c@&4C%x]4G=X>5KU9v5OnpV6S0Q87WH2p7[aiP8`#J29d;+j9hSbJ:llB,;p.$d;tFZD<x_;&=&xr]=*:S>>.R4v>2kkV?6-L8@"
    ":E-p@>^dPABvD2BF8&jBJP]JCNi=,DR+ucDVCUDEZ[6&F_tm]Fc6N>GgN/vGkgfVHo)G8IsA(pIwY_PJ%s?2K)5wiK-MWJL1f8,M5(pcM9@PDN=X1&OAqh]OE3I>PIK*vPMdaVQQ&B8R"
    "U>#pRYVYPS^o:2Tb1riTfIRJUjb3,Vn$kcVr<KDWvT,&X$nc]X(0D>Y,H%vY0a[VZ4#=8[8;to[<STP]@l52^D.mi^HFMJ_L_.,`Pwec`T9FDaXQ'&b]j^]ba,?>ceDvuci]VVdmu78e"
    "q7ooeuOOPf#i02g'+hig+CHJh/[),i3t`ci76ADj;Nx%k?gX]kC):>lGAqulKYQVmOr28nS4jonWLJPo[e+2p`'cipd?CJqhW$,rlpZcrp2<DstJs%txcS]t&&5>u*>luu.Jl>#2cLv#"
    "6%.W$:=e8%$oG29,Z-D%KGRR2Kqn-NJ&N?%A6vsB%K$D%9JRm/?>kdGkvb'%8Je20B)a7D'EtNEnTJF%WXapB-JraHtveUC>s80ORBIQ8xGA>Bi>pKF8gsjNEn?m8p2W*@4D*nMM^kpB"
    "0,DEH^nCUD6M3nMAqZ298`Kv$4W5W-8Pxe6<r/kNEwd29ORh;%6a>W-EH'4=3R&+%vs9-O?o%-D8h`8&`R9C-CS76/I;xgF:khmMeu.-D;t%T&QPn]-+Q1`&MXsTBdAYGD-X2w$bx^kE"
    "]/1dD+=d;%*n1l-vmJF%nBoNMwc.dD+=d;%j=ji-vmJF%DOnq1$E'kEf%Sb#LDig:7l(T&C/dW-?TA.QFXGLO3>uEEsp%UC#BKKF-W:q1l;OVCIBXn/:'usB#qb'%;W5W-?MsL5G_PLO"
    "TpoG;u:ViF3mWRMJ^]G;pS_G$7E#W-<l=F7ww>F7Nj_73IkcLOOmO)<]vIv$G]Ej0tX,-GBCffG&'AUCgZwS&?h?$$ANx*$fs9C%,viY$@49oMR5lD<)`I'I-)tRM`uUE<,mX7D/o;2F"
    "0%OGH8/%A'w#lOMJ_#ZG0k@p&Stwe-8)V%'w#lOMJ_#ZGx;o`=RWIv$BW5W-vDXv@w.mvG;qD<%KGRR2C[*.N0IBx$YA[A-H/i5/?3G<B;/krLUld>>'wI:C3MTSMQ]m>>?#k;%Fa>W-"
    "s-X0cDLTSMUcZ>>*HX7D&tWe$3>a+Pr1jiOX]m>>HM(W-,<KF%0)w.O_T.9I2,hY?RM#sI4CHv$SVkn-.<'+%@aZ>82``8&W]RQ/s?vLF^bg^=aRVW-9,V%'IK],3#PRF%g^?gLS[cUC"
    ")ZQ@&q2JdM::@E5ZICeGgvb'%FQp;-AP1d-+3'+%i)28CuAFVC'j2&&LsPW-0hw$0G9'+%@e6WCCLXcH*g^oDx`lgCYcp1O1/l'I?AM9..6O_I=E+6'-ZkVCKSrT.m&4RDw,Ix-Gs/lN"
    "YaV&.;-%RMrgT^F5C(`&K</jF<?x5'-KFVCvq6XLGQAp&l03nN803nN:63nNj53nN9*nQN,*r2B=oJL4Mf1D5$$=GH%&DEHeo;^F)jTSDm)U$&cq^b%CMJgLuwj^-Ta'LPHs3#Hcj?6N"
    "4wCHMVM,-MYXCnD?Br6MicGW-_]t057$&9&6=ofG`w'@0J7_>HeTp9;Em3#HL=fRMpHRSDe#[6NW0$&8p+R@'lg@_8Dw,gD=F:%'lpn?93OvV%c1-wpi$_b%cR]$Kn;+O43hrS&kDj_&"
    "SSf_&53SMFxQ3.3]fYs-B_E.N5X'&$*U_qLq)F%&:GViFU68/G`8fnM;98ZM/k1HMph><-9*8;NQbFSM,L>gLxZ,<-^KUpM[<1TM.RGgLYD4nM7>J'.<6@nMA5#HMCMuDNDPl)NC>,HM"
    "BAGdMCMuDNESl)NDPl)N<Y*30u:ViF8uhjELu]mC&>mZGBH7]'Tvxb.^gISD-5<&&W5FVCug@qC/[u'&MD+7DN2Z/:0'AUCVN<iEcZfe#&p968qgaNEQF/C/heMmLD0(8MC3%*NeVU@-"
    "1oP<-.`aNMCH940VrxoD02?LF=Z]n*X;A^HE,k>HAjZ:C$,Tv$`es*%QaA+.cf^9&[`'kEq`,hFuV9I60PT0Fh%cSMaUPHM%m-'=;%=GHCWR7MD8)WH]>gh2Oi9`$5pWi2Wmc^?aP8J3"
    "c>#d3[/>)4]8YD4^Au`4_J:&5`SUA59<'9T_V+(5q4m<-jY#<-kY#<-lY#<-m`>W-prQF%WuQF%%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5%QYA5.2VB5&hUOD"
    "#BCc$K,#dM)u]/NTv]/N1u]/N1u]/N1u]/N1u]/N1u]/N1u]/N1u]/N1u]/N1u]/N1u]/NH1u89vPg-Ha%@.H=q[hM2%g/N2%g/N2%g/N2%g/N2%g/N2%g/N2%g/N2%g/N2%g/N2%g/N"
    "2%g/N2%g/N2%g/Np;CT9'DsS8VY#<-iIP)N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N7+p/N8*TN20f8p8iIP)N81#0N81#0N81#0N81#0N81#0N"
    "81#0N81#0N81#0N81#0N81#0N81#0N81#0N81#0N90^N21oS59iIP)N97,0N97,0N@d:-4hweBZ4F+RMd.U-N77,0N77,0N77,0N77,0NK:(t82F2@K723@K@wRhM(>,k2`x0@K853@K"
    "853@Kdf#Ludf#Ludf#Lu:GjwK0T<;&Y.?LFplqaHiT&3B4Om;%@_lt(iiRTC5-ST&_#N=B::c_&Gg1`&$0X,3grQF%4ZO,323Xf-C(2LP,:RhM4iZL24ZO,323Xf-C(2LP,:RhM4iZL2"
    "B>(W-)^+9'VgZL2B>(W-)^+9'VgZL2B>(W-)^+9'VgZL2CGCs-<7RhM&lmh2b4hwK%^iwK%^iwKcAGs-=Iw-N%jv-N%jv-NZiv-NZiv-NZiv-N[o).N[o).N[o).N[o).N[o).N[o).N"
    ";bhp$/Q<.3[tD,31LI,31LI,31LI,31LI,3&*F,3&*F,3&*F,3(9kG3'GOn-kuQF%'3bG3'3bG3'3bG3'3bG32UeG3upaG3upaG3upaG3upaG3upaG3upaG3upaG3w)0d3j(Gc-luQF%"
    "v#'d3v#'d3v#'d3v#'d3v#'d3KQ+d3(<'d3(<'d3(<'d3(<'d3(<'d3(<'d3(<'d3ZGw20T0eNB,ZkVC&%F(I*vXVCh:)hFxjeUCi#/>B&DJZGm9?[A'+OGH[VLwIJh(6;Abr8&GA05N"
    "9QnaN*o;+Ox0a+OCut$/$x^kEW,OL5Q8Ce3CXq2B8Ann2>ZWI3`;G)4[l5<-qw_&Os6seNj<&fNaB/fNbH8fNcNAfNcK/JNaB/fNo3Ei28x/p8emPRE2kseNT%WeNJ%WeNJ%WeNL4&+O"
    "9d/N-Zb/N-P%*M-Mh^C-P%*M-WNdD-WNdD-WNdD-XNdD-XNdD-XNdD-UscT%cr+xBcr+xBcr+xBcr+xBcr+xBcr+xBdu+xBdu+xBdu+xBdu+xBdu+xBOl1<-d5jE-d5jE-d5jE-d5jE-"
    "Bd0K-3^L@-3^L@-3^L@-3^L@-Cd0K-9'+J-9'+J-9'+J-]NdD-]NdD-]NdD-]NdD-]NdD-^NdD-I-eT-h5jE-h5jE-h5jE-h5jE-h5jE-h5jE-h5jE-h5jE-i5jE-i5jE-i5jE-i5jE-"
    "i5jE-i5jE-i5jE-/^Fo$6$'fNQRS+O.d]F-.9[Y%-I3INnaQL2Qn.<-hlxb-xUQRE-I3INgbQL2i?3g2wr[m-14H_f7jrW/_JJ+4U(Vs-N7NU8MoAg26qnQ85Z:E4d&thF'#OT9gk_5%"
    "HnJr88',d3o62m$7'tINF62m$ItSr8H',d3Ovdi$@H+UKbfF(58o-W-)jLaYafF(59xHs-o&mW8=2es&#(_%%8*k.NgCj.NgCj.NgCj.N%M+89Uuxc3B^/aN(7P[$L`rHOsZ0l:^##d3"
    "t`p;-2%]+%8*k.NEM+89Uuxc3tQSs-On[hMK?+,%P.4bPsZ0l:b##d3xlp;-6=+,%P.4bPsZ0l:b##d3xlp;-6=+,%P.4bPsZ0l:b##d3#v5W-I8jwK58jwK<kRhM.%g/N0VO,%TRK$R"
    "sZ0l:g),d36Ym+;f##d3&#q;-:UO,%Aag/N[WO,%TRK$RsZ0l:f##d3&#q;-:UO,%TRK$RsZ0l:g),d37.iH--sL*%&#q;-:UO,%TRK$RsZ0l:f##d3&#q;-:UO,%TRK$RsZ0l:f##d3"
    "&#q;-:UO,%TRK$RsZ0l:f##d3&#q;-:UO,%TRK$Rn'8r8[6N$RsZ0l:f##d3&#q;-:UO,%TRK$RsZ0l:f##d3&#q;-:UO,%TRK$RsZ0l:f##d3&#q;-:UO,%TRK$RsZ0l:f##d3',6W-"
    "MDjwK&#q;-:UO,%TW,w7dhF59a>p;-+Ur2%)r*$[rZ0l:h##d3()q;-<bb,%Ve,[RsZ0l:i),d3Zw/8%()q;-<bb,%U^5w7eqbP9^H/[RsZ0l:i),d3eT4t:h##d3()q;-<bb,%X#2t8"
    "oCuD4VeAr8RVjJ2q[Sv$>13:8KO4K1bWu89i./>Bj3xa$Tf0SD0P8QCE?5j9lmV=Bx':oDIA^f3SRZ+H,AvLFt.4O+eeimLu./PD*1q>$i3,C&]8u9BlK='%KQrXB8dhc;_#Y_HC;KC%"
    "WgitBvsFf#2YNA=S+'^=nZV=BQ7_aHO9jt:Yg_ODUYVmLd-6(Hx&IQMJmrpLXID,DJOhZ$#PRFHwM3nMBD@5J6IC#$$x,Z$]AcG-P>cG-SAcG-e(uoM#8GW8aC1g#&#3^FK:u#$t2h(%"
    "^S:d-90#+%>+krLhOqoMa9'#GN+&mB9h?['+UV1F[TT?&>m&?H9uR+H)TQaM/L>gLj[,<-xDHaM1n(-M2,H9%kVV'o72PA>But$':;.%'Tma8&7s.RMAk1HM8U=4N(1&rL):A7M(:A7M"
    "lk<B-QncdM:NHaM:E@*NV$?9NH'8>-g#/JM;k1HM#i><-'Gg;-/l(T.18,-G,[NNM+5)WHsWUR9?`'gLWhv-Nan).NYt2.NZ$<.N[*E.N]0N.N^6W.N_<a.N`Bj.Nhs]/Ni#g/Nj)p/N"
    "k/#0Nl5,0Nl2pjMo>Ch$^caN;O.,F%%w*g2%w*g2%w*g2%w*g2kE*g2kE*g2kE*g2kE*g2lK3g2CMGQ;na/g2X?Ch$=iU_A?`'gLeOGQ;WwZw'ts9RU@nC*N,@Ch$Ug_kF?`'gLuZ.S;"
    "WwZw'ts9RUl'SR9?`'gLV_ZhM7v2.N<@Ch$gV#d3'uFkk?`'gL6[)'%Gx'gLt&eT-SgkT;hj4R*qCf-X@nC*NDq6i$^caN;W.,F%(<'d3qCf-XHH7+NDq6i$+BOwp7G'gLuZ.S;OwZw'"
    "P-]wRn-SR9#B-Ra_GW_A_GW_A$bmJaZ*E.N_+E.NDhDu73,>)44hE)4,OE)4JUr;-WoJ0%/kt&-[0N.Np7@q$Y>I6:].,F%`B]D4`B]D4`B]D4`B]D4`B]D4%@aD4hZ]D4%@aD4ps]D4"
    "%@aD4x5^D4u'n;-*.iH-,7.e-NYnEnWf(W-SE,I+8=P)NavDY$fLhwK+piwK+piwK+piwK+piwK+piwK+piwK+piwKr6akFsFDs-j?bu7&G:&5G^B&5r/>&5G^B&5r/>&5r/>&5r/>&5"
    "c.'bY#kA0N60ei$VmhkFG^B&5ImgA57R6p-quQF%o:fHXm*SR9VKoW:awZw'o:fHX@nC*N-Dj.NBn?i$nehwKE%(gL,KoW:awZw'o:fHXl'SR9E%(gL$KoW:awZw'o:fHX@nC*NBn?i$"
    "fLhwKE%(gL>gf(%E%(gL>gf(%E%(gLWOQf:wa/g2_m?i$3fW-vE%(gLNrL*%TW%gL$q%W:XwZw'_/6nU@nC*N2=Lh$^Y8FI=c'gL$q%W:oa/g2V<Lh$UA8FI=c'gL$q%W:XwZw'V<]<S"
    "@nC*N*cXg$^Y8FI5J'gL$q%W:PwZw'V<]<Sl'SR95J'gLrp%W:PwZw'FVS0NQ[_68XUO0NPRCq7.WO0NPRCq7&evTK@nC*NW&)e$ZGeQ:0.,F%dXh&?J%)e$Mt/:DsV&gLR2o3%sV&gL"
    "$q%W:8wZw'>d$UK@nC*Nh&)e$NHg&?K%)e$NHg&?;pAc$ZGeQ:v-,F%Q^tJ:vvZw'lYAnC@nC*N2x4w74qbP9x@fP9/Xn;-/iDU1/YNWHsWUR9#Pm6B@el_&ht8`&p69`&q99`&r<9`&"
    "s?9`&tB9`&t?0`&gl&`Agl&`Agl&`Agl&`Agl&`Agl&`Agl&`Agl&`Ao.'`Ao.'`Ao.'`Ao.'`Ao.'`Ao.'`Ao.'`Ao.'`A?*c:2r<9`&owW_&33SMF2;NU8wvb59d>:3Mw:gN2ef?F%"
    "6G*KEfgjmLTY?^=P1%lLdFq0MZwDINXhv-NZ$WeNCelHM&Cj.Np;AF4`Du`4aM:&5^2>)4[v]G3Y_?443w>44x+UF.2t>44H77[BOnsn<7L%.Q1Rw-NDxDINT/WINZwDINb.WINq?M.M"
    "DxDINj(NINc7seN3,&rLF*NINB/WINdO<3M<(NIN0/WINE+aeN$odINVqvh2KVC)4p%P[$p2JuBw3OFHp0;9Cm'^e$r[gNEQm2eHmnmnETIuHZq3n0#J,P:v&2>>#@FHa3Q[ZG$IaoiB"
    "&_S.#wFFVC6ZhoEhpf.#?T@%#W'dH=jFJh;(^H&#Scj=8PsI-##)>>#jBUl1BF.%#sD-(#W=#+#`mHQM<XPgLYL<$#X9F&#)K6(#YC,+#RWL@-=`oF-0#b71p&U'#@Pj)#t6D,#un)f3"
    "4xL$#]EX&#-WH(#^O>+#6*]-#9A26M:`YgLnjW$#_Kb&#>8E)#Y1g*#xBV,#wBXw6_wX;9rX3W8?8I&#gC_O;`pOM=5#I&#Jc_4;i=1XMnk<$#8.`$#aQk&#>2<)#Nc/*#i[P+#j5wQM"
    ")R*$#:4i$#cWt&#1jd(#dbY+#nMa3Nv9e##Pqn%#x8q'#JoA*#(Ur,#YQ%J23Sl##gd0'#LuJ*#*[%-#V-vh1I'+&#/&*)#jtu+#rMERM)k*$#@@%%#kpB'#7,3)#l$),#DTF.#uc:eG"
    "%dPC`hUN<B3B9AObFk2Cqm=f#FrEp.lEvsB)/Wk$22JuBbD1XMtmiXM$<JYM*a+ZM0/cZM&IBUF%0lf#)S>j0-ggKFpS;>B2uMfh=tGgDviTlf$VIs-%v_kLx(Dp.V,T7H0?Ak$>:2*H"
    "@A>S`1GY+HT&^cH_u0'#%/5##t'ChLM:-##,f(T.1l:$#7xY<-%Gg;-&Gg;-4i&gLOL?>#%)###R?3L#'+?v$/FZ;%-XrS&$lK#$T]Fb$N>uu#@*gi'&R(4+,6^e$hd?D*./GL#a3LfL"
    "TAd;%k8D,#47/X#flDD%Hr'R%Kutm%x&Mc%,UNP&u@dZ&i0jE-,`Ys-pY^[M:Be*P^7LkLULdY#kgrt-]LOgLiX41#ZvM..DHdIO`;3,#3]?D%/HFjF/Y,WIlv+8I^cp:)j_@#Pi68:."
    "*VUV$'aH&,34^APZ90_J't8SRJ9N]FQO32'@_a%bE5[xKSgw7[2)Xo[xd*-M/</#M4Pd[&.cZ;)08vlKS;/2_du%^,`;/2_3bkt-s(F$M+r&qFkw-a/:30$LRWpP#N/4&#'####,Sl##"
    "+@A5#M3MG#o#u2$gxE=$t#OX$M)vW-h>Hx',%n$(X$='#%CV,#EN=.#fY$0#0fa1#PqG3#q&/5#;2l6#[=R8#&I9:#FTv;#hixX#1lC?#Qw*A#r,hB#<8ND#]C5F#'OrG#GZXI#hf?K#"
    "1ia1#k?Y>#RVR@#SgYs-U]4WMesf@#YgYs-Ms%VMg)#A#`gYs-GB2UMi55A#dT#<-%VwA-AWwA-B^<^-Y'?F7<.KoM,>fRMMHguG`^mA#(^<^-_6?F7CX5pMS)`oI.I@8J/R[SJ0[woJ"
    "JuoA#1WwA-2WwA-3WwA-eWwA-fWwA-gWwA-hWwA-iWwA-jWwA-kWwA-lWwA-oWwA-pWwA-qWwA-rWwA-sWwA-tWwA-+ph51NH.%#*Q?(#J]&*#h@iN,&=w0#@ib.#Ha`UMf#p@#agYs-"
    "XSxVMB>b+Nw:TrM+60>G?RlA#]FoY->k-.-EhY5Ne&n5Ng2*6Ni><6NEmX;NG#l;NI/(<NK;:<NOS_<NQ`q<NSl-=N-c&m-HOU']=O-AO>5;d*@2CF%;+PcM?D%a+?RlA#f_iiLi]iiL"
    "]DoY->n6.-d*7.-.I?F7HwcpMXV<MKF838]GANS]IS/5^J]JP^P=CJ`QF_f`SX?GaH;`s-H*V$#lj9'#6vv(#V+^*#w6D,#AB+.#bMh/#,YN1#pnrUM[%p@#cgYs-YY+WMA5FfMFwiYM"
    "FwiYM'ZGVH].ZSJWT/kX`RrlK?)NS]A;/5^hvmA#Cg`=-Ig`=-m;7r7&tlA#>RB,.TA]VMh/,A#V^>W-vF5QU?5FfM,?KYGI6fKY>RlA#j_iiLeZ<^-F07.-0O?F7J026Ni><6Ni)u;N"
    "I/(<NJ51<NK;:<NRf$=NSl-=NTr6=N2;W7#4l:$#Sww%#u5$C#L'hK-Lx8gLd82X-7M.x'1YVF%%k95&WuSY,x5^f1BLgr6ccp(<-$$5AnTUe6O7hl8AG@&,Y'wddX'wddXrnW_W:rlK"
    "J]JP^O-mA#;6m<-]fW?PV_Hi$;(5GM($KkMP><6N;;:<NfrV7#Y:4gLsL5P9)M2W%;Q8L#,=jxFN:RP&qY?MK93r7[Hxa`*JwE`a[3iWJnv3AF&p?X([wbf(1[dWJCe=rLn2(2#3-2W$"
    "]3uf_[IZY#>1hWJqrcrHXR75&5pJc$vig;-DQ'8/6:r$#Gw.nL7$TfL4KihLN:>GMf/,GMbhYL%:r4R*5s`c)V1CN11^BJ199_c):[jl&kj1_A6)VoIh.u:AElK`E_6auGfj=SI&Vf(N"
    "TY1MT0'ho@>J-9.RVMiK9q.Y%tBh%F[%i.U`=IfUdU*GVhna(Wl0B`WpH#AXtaYxXx#;YY&<r:Z*TRrZ.m3S[380P]8Sg1^<lGi^@.)J_$QoCj6K4]k>&LulFVd7nN1&PoVb=ip_<U+r"
    "gmmCsoG/]twxFuu)G(v#1x?8%9RWP&C95/(KjLG)SDe`*[u&#,dO>;-l*VS.tZnl/&60/1.gGG26A``3>rwx4FL9;6N'QS7VWil8_2+/:gcBG;o=Z`<wnrx=*ROV?<,]fCMxt(E<sA>#"
    "?__d-PIg;IX,^+`H_@c`LwwCaP9X%bTQ9]bXjp=c],QucaD2Vde]i7e`X9T%aEFlfteBig#+$Jh'CZ+i+[;ci0'8`j8WOxk@2h:mHc)SnP=AloXnX.qaHqFri#3`sqSJxt#)5##+SC;$"
    "3.[S%<h82'EEPJ(Mvhc)UP*&+^+B>,f[YV-n6ro.vg320(BKJ10scc28M%&4@(=>5HXTV6P3mo7Xd.29a>FJ:io^c;qIv%=#%8>>,_kr?>8x+DO.:DE6q69'hf[`*<wqu96RG9#+SL;$"
    "ptc+#TKaH$8Y.ZP<uit'vg0:2]p`MB/&WAPu0F;2M8m<-#Yqw-2#XrL0TiN$e$7=.HiC?#bCChL6(@##?(02#cJg2#f9P>#_>L/#+PUV$i][_$P%###";

static const char* GetDefaultCompressedFontDataTTFBase85()
{
    return proggy_clean_ttf_compressed_data_base85;
}
