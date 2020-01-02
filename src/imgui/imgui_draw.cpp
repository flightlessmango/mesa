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
    "7])########0tO6'/###W),##1xL$#Q6>##U[n42f)']<P5)=-'5N;6aNV=B9l7_2T->>#tG:;$Z2XGHQ=WO#/HAi^K@tc3DJk7DQO2$2iO]._b(35&*a(*HMY`=-J3S>-x^Nj0#-0%J"
    "f9-eB0[qr$7[.nT7-d<BOZl)2u(@cV%JdD4AJr9.)A,Gi(Xx%Yw>eW.;h^`IIo]7C2N(<-JRUV$g3JuB=]G%`-a8]XL6YY#EK[^I5a<a&%&5YYU2jq/+>00FPj=m.,*m<-dI0h4l%S+H"
    "&Qf$p-gHo[AYqr$EeFVCC&4$.T&BDbLNr'#dHuY#EC(7#.M(Z#u7`'8Qb3G`fNE3k?;%)3W?q?BWOap%0=Im/s[(##sD4;-DcrQM`>B`?847;-*lBvd(+JM'W?]c`7nN&#VHv0,xNa-$"
    "Wo8gL.Ag;-'YP8.B)B>#(ok&#B=vu#+(:B##Su>#aH`t-U^UANSn_aNu'^fLD4pfLHqDw$2+RS%6C35&:[jl&>tJM'B6,/(FNcf(JgCG)N)%)*RA[`*VY<A+pT9e?k8,&#a3=&#e?O&#"
    "iKb&#mWt&#qd0'#upB'##'U'#'3h'#g/r1.]8^kLA*nM37Jr%4;cR]4?%4>5C=ku5GUKV6Kn,87O0do7SHDP8Wa%29[#]i9`;=J:dSt+;hlTc;l.6D<pFm%=t_M]=xw.>>&:fu>*RFV?"
    ".k'8@2-_o@6E?PAwKbKc'O%1#.M<1#<4B2#TwP3#d9v3#iQD4#kWM4#spr4#uv%5#%3A5#'9J5#-Kf5#2Qo5#7j=6#<,c6#A2l6#1gY+#L$S-#EE=I#A;L/#0hb.#>L`,#VKFI#cF31#"
    "sq@-#Q<+I#D0f-#,Z.H#6b7H#rvI-#4ABsLs=X4#YsH0#HdkI#aQHC#x[FU%5bt4J/9v1B%;NfLikdV$u$DJ:iu#)<ki',;'x(M^JgX(aQl2;?+CSY>-9p92L7?>#BJqJ#nhlF#O.e0#"
    "-)[0#5SE1#=x&2#=lj1#w3/vLUrZYQWc1MTOw&Sn@4tu5vj^]4A+=>5-Z;;6@l@A4KsBSIKXwv?igWV$TA@D*ZKbHW53F8%db:8.*mCD3uCRm8tT%H#7U5j#`Ano#Frc/$:c$4$P]eh,"
    "lv@=-FZF>-V5:?-#YEA-L.aL-54Lv-#nIt->ZXu-[lHw-fV+'.#Zo4v/]b?/Z9F&#AK(xLt>E?#jjBB#0v)D#2YkR-.n7^.[;eo#`b0W0*U&U-96f=-YDOJMe$c(P:wjBO1,J6M3LV(v"
    "F*Q7vP]q3vKJU3vC213v;pb2v6dO2vt9;0vTfteu(diOu8)eKu+YdnlsZVClLB2ClF*dBl)os@lYLk5lwX;ck6)PekwM]dk^H)ckWjX]k5moE8ml$hcR?%@#q=8a%uvV]+QpNJ:rHSP&"
    ".su(3kCkZ2,CS<$$Mwm0Md*P(3IL,3+b<:.IZD.3m]WI)x3vr-=n/K1cs%f3sJ))3x?7f3JOQh,aO5rJRm9DEK'&JCt)/E+_I.[-^deg)MQ[^5NaW&5IW5o/q,8(svA#)$*eJ+uUo7T%"
    "(2.o/bD+H=e:`(,-%9%>1nVJ<X$++5W`=CHD05##P.>>#1u9'#-Mc##b+<'#A@7,=*5G>##kn>#'k&&=4UA2'r,b;$.%[8%947W$br9&$vpFk0jn0N(NkY)4w?t&_YftAu$T5Fug$itL"
    "7S6##2tvC#=M_PM@m<3;NDWC?vfP>#vZs%=2`Cv#3%M$#/s1K*Ia8U)sq[b%BXj%=Ct:B#WOs]7WCpF-d0ju0[aNL#hK)s?$gxo.EwtNFL/5##n5YY#VMwI/tbnZ7PSofL)<#gLY@R>#"
    ";);Z#97VZ#9%78%Hl7[##(ZQ8>IET%8ckZg%eT>$D*&&,47?c*$DXI)ws*1#3Qr0(5qZ0)k02N()vsI3N#G:.E7g;.B]fZpue-lL;c1p.Ae]ZKKjp0#%^2*@&2ei0^iVR#BA+*r[eT:g"
    "r;4a=W#YEM9H?##'vgK-=G/e%G-^f1Pps+;nMDq%ABGn&+DY>#98c?#:=[S%;>n8@;1[s$47e8%;/.O4W10*#?XwW$r%&(#hSR12EKRl1Dpl&#4q/Z-;97p/3?k2(Dq1DN80r_,]YVO'"
    ".q1DN@p=j1p:gF4$UD.3FH7l1um7W?dl@d)4(XD#XbN)<Q=Jo*0^Dk'`i^CO7McTGHh:j(?w*q4$Hv/(S]b7K8Xdc*(NU(C_uSP-B:=9/Ipp49HRPQ-iN:H<+@^17OK=B,Ah-'5'bn@d"
    "704kkCD$kkf-(,)vAYc2gU*;?RZ7R*T(7W$gf,]3@F[S%AP3T@8c_;$/skZ#2;G>#'et293-K]=+&###R5OK3Zn?g)@JG##r7k.3VjIp0mCE=.Go'J)1DS5'I$XX$85>##VA`0(vuK8."
    "L,B.*[*'u$BKc8/s<9-mU#+jLYIbn-rUA['YcQiTQ:+;/_*+HpI'ie/KD56Beu`50x,H3'v:#_+ta]W.NR6w/^cH<.s:,_+[vC0($/gbinq2u7HN0f3'-(<-E4H;JIH=>-N?h:HTm[a7"
    "HODsJb..A6PI]],P_i],5dQY5S8q'#](N%$m?O&#piLY7u.dY#h_&*#tUBp7g,&F.]'m+#'.ZQ81S:v#)B=;$'u,m8`ovs.rQDq/?28L(2#]d)aW+T8)CGH38%1N(#w8s7]bj5Aj>%&4"
    "(V'v5_b>g)/vQJ(,2S[():np.)Q93C=*c,.rV2l0cBIg)1D&#mPA)e=oAS[,wDt$6t.9+CmsJ23YDc%,'Asm9(#+PAl#?8.$),##LuvC#KLV,#Il$V8Ob7p&k3qA#EObd8RB0j(X[=_/"
    "oRi*%BXj%=(ok&#aYbv%E-i#$:Xew#;v'W-lsKaP'mN71h)i?#gG(7#M?8W7ejXZ-;tlanOC0*#%w]]=8$YJ(VjUq)-a<A+>>.Z$wiic)pK/N(Yt-thWdEj1Cm;hLnjuH3)CJV(t,?n&"
    "2Ne-3L]=Z8[+U&?x#uBCA*I0)hc6m0sDV-,+C,%$:E9(6qBZFI*b>Z.)>>##@`f&#P0WmLa'U]=JB,<-%uSE#e4Al'F.)$#`KKY$M1x*3jA[L(s3si0jfE.3(P(H-=:fT%mtu$?G)?T8"
    "<(jL3bXBp&$lbM(w-6L.,Y*'utQVh3s#EB+wnrV$A-R,*W^(&MN.XG#Q2.w#]4i$#=0J?##SrB#P+5gLi]_:%Zk_a4,g;Q/F]+&+Q'vTJp_UN<(u`=0B(v#BIMP%LQcvdECrD:Be*>Z0"
    "O;Li<*WTG=cIE/(g*SP&lZ0[[=lj]7Ff1Z#+GcY#-WUV?EL?)=SaFn'q+>V7?,>>#T2QeOmO$##%3xu#pFc^?o^xB/]RNP&0A58.'eZ0)r&;d2)wZ)47'8MZ#+-Po$AH'[.%U-HjATf:"
    "*a>SIuW5^,Zp0pplt)3&E7M8.D])>1MDhLgA3q6*?bs&5?#YM0V[(16/%eX-.wK^uCn[xC@`x&#]#'2#0-ex-(d=78Tbb&#TH9HO6D6a$.H*7N[Ulp/bM@;6]*X:2ltVmL2cJR0QW'B#"
    "a+7Y>'8Wc8XJF&#r5N506:WA5&t^I*j6IP/h`Rp0h=2v#i_&*#6@o#.+]hE5(wS]=2L82'jK.##D_m(+Qg?D*Na>j'_=o#.[P^88C@-$$4.,Q':**785W?C#FS180^aRB5(;#B5Q6rhL"
    "xa)l($x/C+L&@hEnMQR&W2f>64$BrQT^:#.TRqR1jko*=qPH.UZqZX-*K*A+Fd8e*`K&kkJ]/kX]0+m/R@-5/4Svu#+E7`$8_8m&;[(]$e5K<qX:no%Hv2;6iiqB#HgfX-kax9.5Ta-Z"
    "Uh^KI2Sj:6JX65AUqtv-(p/kLMoHlL$EH&#r[/(#*;cY#.K`,#+wJ]=*&###uT[Y#QW+oL#(Bw#2?ou,+8fL(]g>n&MBbA#u$Dnspq.[#9OnA,/;b9D1I@x6>J))3>U^:/0pvC#CV180"
    "[ZIB5uqxD#IAOb)=N$TS_Y_Z-T]G<.h5&12H0iVI:_uE2miZ597N_QCX5OJ-@2`]0oiLY7eY(v#BLMmL6^vY#Nt[(S%=BK1Vax],.J=.)m_HP/jg&(So^0#$#5ZH.iF3]-ndAW-UK5[T"
    "5W(<-eW;^5cD^Q(%)XE*QDZ]uY_,@-,J>C+p;4+.9ZY]u/'xf$ex4V&.u.),[s_>.&Ysc+DBALCp90v7a4n0#uF)20`;`Y50,c9&_?]f1OE:q%VYA*[X8,##*>cY#/&O&%8_8m&uVmX$"
    "W:jl&8`($#lgn0#wYIvRvDg;.]L3]-:[TD3q1-v=,4iLgwV_oamaG7eku;5FngG7eWYJtCu(4GMguQlLIt[.M>M.U./VC;$@CXr.+Ex>#gZh?BNPuY#>xr62YiW]+_MhZ-%6lk0p<6g)"
    "d1)W-U.^TraTL,3x?7f3ICuM(8J)]b5BGV/a)=$6*6^M#>Y'/)pJY]u&]KNCXqtD>0U;6;VQ-/2Z*Cp9&v:*nfww5/jvK'#C7;kL+)j;$]D1pL$@h]=0S:v#PLl>$biuw0fJCZ-ZlW]+"
    "P>e;%R3'Q/%NCD34tFA#0(rP8Oq*H;=J&v5u'7]6/xrB#OY*4.^7Ws8vcEm1x42W/X)9<7rB,N'&)Se,vf%4$uKj<7`rNhN>po0)>$=F5HW(2;_:9xX:8e7&N.)&9[P0K-Nv%v2<t9'#"
    "1qVV7@2G>#%3]>#p<PB$pNR##N6+=$j9<?#HxF)4#1eF`6Vt]u6]X&u'V$`CB^V&#1cEB-Q7w2M,'#)#U=<K*2chV$lmWV$0M1v#8A9^%-Pc>#.wGpLL`;v#H_+T%E]^l8aaJA,2_cL2"
    "?s8b4]`MP02vm3'[ra]+DwlH3*W8f3mOo$$sa*wp7DXI)Xc=Z,qm<:.%Hk58]Q3A'm;a;=%GGQ'Ngg&=vO?M<_GD4'OJ2.R_YL?-HGJ60<RS8/2+ST%mPUZ-h]`@@5K,8/MqH0FMgJd3"
    "5J'b*i]PW#UE.6:;^)X8Hh9A7j$F#7,Oc)#(j&kLr,u%$m-4&#scCY7^AP##0cLv#3&hWhAG9HMI#b?#RHYA#;Du#-[NW-*`a5n&L6OA#+mc2r#4?&%Zt;8.TekD#vcMD3rQFg$_vYD#"
    "+87<.#/CX7B0^n/[$Q7ePf/F*hp6N82'9<7WU_b*XGE**o_U#$1u&L;F_=H5iKWkB8/U?-j2;nL=#5L)RR*?6.]N;]hCKfLQ62e$>CZ;#-Mc##bCA?$%&###+KCV?-1+v-2'qL;$%:B#"
    "m2Ahu;')*%hATf:2lQR<6mn51#NE^#Rm9LNKYvY#,dQq('29$$;X)>PJA+bOZ_^YQeV2T%$wp4Jr:Ionou^V$6<CN$xFwm0W]*E*$iEonf$HrQZ6b3bUI8874]o.C9i5##i^-_fWqH]-"
    ">1eX-Y##Yu$$Zju4`fmCw*)UC[tu=PuYm>#b)Bu%cp>V/BImruh'JU#a[g[MYwWtL]Mo(#TMZ$$k3=&#tIIZ7^n>B#2i-s$nZ,)/*DY>#L2#qrUQ+?#stYQ88+Rs$/CXkDISY>-hvS;."
    "T/1Z-5r9f)S90q%cI#v,5V.n&r(@j:c$**GMGG:.e#>g-Z7(f)7aS#60&Xk'`On@-^4&q/)=mV#D@;n(,HHe<B`_r*+we>66;ug3Qp]3<<lAa#PrVr?7kBJLekLS.>jTv70oC?#6xH8%"
    "5^(v#W:=,MP^Q##6jCv>0WAs%Q<Bp'Sfp],iv8Z-S;hV.(MbI)U<B6&;rLv#^('#,?MF847QBv-t-YD#WOFb3:5Rv$;GEF3Dus9)YqFb3K=?g)Sf%&4^WvduKunUC/aO@tgskA#-&rdG"
    "o'^>-[S#eQ(@Z<WPQGA4p;M^u_]l^I*;Liu'&g0bqx8;-U-5f%5j<87U088%R4LS.3V;;$(MC;$,^XV$&hY+#w$ZQ8)5G>#1Z]Q(+a;Q/$9uk0@Ir8.Jm4:.*bM8.$%`s*22F8YS2rD8"
    "49(2$x=KU#=k=6#9cq;-oUnf4Sww%#oiLY7ho_V$i_&*#.fh;$0W`,#u.Zs-bT`l8_%e<C%2HP/K-&&,'>=.)IOI@#Tk=W%`'([.#]o-)G&9f31EQJ(fj`#5hZvA5QmpV.vo2o&j39>."
    "+kRNP=YVAP3vxZ$G*L$.@lx(>Rt1e3Q';F=Y_*m/(h,H6Jii$:t%'tq5]&*#t2wiLiDjJ'wBb?nL#>-;f%$[-kmqc)i&l]#>M.Xfp?\?O(vcMD3CugXul>67&l@=M(6gF+`h/=-*h$kx-"
    "s]1R)23T[uoKHw-M1L#6$lwE5vqJfL@:>G2Ssxc3Q6s5'4qr+;rmb?K;l1/:Z*xa+gj>n&;%`?#O&+g&ECuM(cS<a3^?ti08VC-.[CL+*PAZ]uH8>>(o)tF.E/#[P;;$T.2qKe<FGn:2"
    "=/#u-+[N]FG$rJ)_2MY5o-Q50PtvS&tmX,;:PO&#,OcH&<&4l;(WFkM&2jc)LT/i)816g)++<20j]eBs1]Rf(^V<&u>Qo.CpDwA#5^qE@Rfd'&#Sdl/wl#RE`(aq'+`Wj$[%lgLpj*V0"
    "2Xof1sq1##Dx=wL<Q4%t[]LxLQcEB-_>2U%=aRp0(>YY#-^UV?vBsM-,Ls'%?Cw(<K^Sq)2;TY,:0p30NBWb$H'9Q/>uv9.Vn@X-ad`#55QZIqlGQR&l:x1(M(mc)x>wA5KZ5o/gq-F'"
    "X`x.C[mmJauQwq$ET#[T#G-5/%GrKGW85##5,:W$3Csl&nGcY#R&r]$YweW-@,5L#Q?V-M?AIxi`V]'Ms0jFV[0ibik,42Uh,wA#14k>Rco^W79,ko76:q#$(>cY#a>tZ%$EeA#I*+W-"
    "=$+qi-p9BY2;@wY`[KNC344tB@Qi-#8%mlLAdbjLaLj$#+D#jL0$1&=1>HX$H,,##(1G0.LU`mLd^j$#:#?s.gfqB#rv9H%fu.&4`N<s/j39>.8#;/(j8-7&iftAu;Y^C64P$u-OL+Sn"
    "sM_lLOWOjLh=*mL#u<$#m-:t%,55##g?%##'/,##AJl;$3Csl&hW]m(onkq&T]eh(_&`,)P;*F3*&E,)bE1K(&S>f&usSgp;AGKUp#2oeKRN+V,x`oI.E)uLjFT_-9DJwBN0X'6]8@Y%"
    "9pwO;Z`V'6#YJL(6otM(T+SHuuB&j-2]GF72KwE7KM*RW)U#?.6V(v##x#xL@Q$##xgwY#4L82'd?:'f#vPj(0[ChLU@P]#<jp,.$7gfL-_kxDFaZf><tvrL;Pnj#dGgq##,G>#f=T58"
    "QDGF@u%18.cIG,M-C158Z.x+M4'22NRlv##,d7#/+Q4b+g69e?\?fH#d#L05l?+d%.S[_pL`0p@uOd#`-EZdX/djJ%#x2O`+dY1$-;)jV,.Z-a<s3s20kmqc)x83^5KW,o/-_mO.]@C+*"
    "Y.]v$l@=M(gGXi(fRY_+d//C++JDnqV'K>.Lhca3;LHw-BV7m0pPi1+BYFI)39T%#X2rt%ma_c)dGr80>IjT.)&###>Ws'%h9_c)d&d3'C%v##F[Et$,SdW-,<*Bm$JrB#ppmA5OG=>."
    "gY40QFx7/$m3&w@(8]>8mcj5$OQj(RAtif5:mcj#3t9'#rVH(#/DY)')b,E,'8G>#(Yq(<u8$?$m>2V93N7a+wKe--HQ8@S[Vr]7f$x+2N=lM(F<tXASD,P9e(tXA0TX:.?x-*5:7(p7"
    "KhR'5JN#o/ocII2@Om?.WEpH*X(Sv$l@=M(g2SZRI(Yp7Wn5&QRKl]-UGL#6hZ0>GWa:#.jCrO08X[X-,n/>GIfm'&>-f;-<wmT-H,h)2T'+&#&I:x$+Jc>#A)ofLo^)Z#';u)N9X-##"
    "2i0f)iGVk'*&F-=.C(f)co=Z,hh:8.dxrB#'YeD<lsKd2T3j)?ur-S/+c)Z:uOojPac$w-F*dgDL3;N*E_g,3N3.cTE6/m/<Ora5PL%##V#9pppKBh;L2_dOx2g0'M9'/10bME4w@;9."
    "4(XD#du;d;.#;Z>*6qB#UD.&4vcMD3-pN@tj/hq%p%M@0cFUF*TJd]uKo-h(iTTQ&93D#?#ZX-*i09>.qJPq(cU'c3k8oc=)PFv8<idU.k44j1Q$@#6-38C>`+KfLVk-##_5YY##t9'#"
    "5'jV7C/,##wfY5&2#Hp.B<1K(exU@e0Bx9.94=`#@HOd[+&xAu6T$`C$),##.>Hs-?hZ.M>uDv#3Csl&gWDa-TZqvRl5p%#iw0(4sbV/)G@xr%Jk3B6[Qr*1fCog(`C;8(E8q5Srw:Z."
    ",Tlr/bdtAu;F-[-BV7m03@pEu;SB(6Hmif<Dg*1<C+Iw^qxCw%FcS?20[M1)*q^#$/=W3K,2o3ZFTf8J,KjbinLKB#+;###bBf'&C^c>[g?R##5>'^%2V:v#vWR>#$wSA=2c_;$Pb`mL"
    "&?W$#t#41#:H(l00=N/)E/W;.uY4USC&l:At_W$QPr+]$#E:oL8n4L##&Mc#%?KU#iw4E9&J@&,*4Fm/OJuX7hx1Z#+)6X$R2,##<##v#+JuY#`-N3(gk/2'i]@L(*eQeM%FD,)ET$6/"
    "%+,;dP3TDRV$EOGW.F3c]o9^u$9F]uQweH#$paT#Moj1#iEqhL-C^nLxuv##b[FW%/d=p7;)Ns%gp-[B7[Z*N:Ix9.jbt3Hhp<Cge_$I?M8<JLO>^@k'1$b[R//'HWbQLMkV?X7R>cY#"
    ".DG>#Y9^p9rWxB/^VumL[:_l%nlic)&0fX-ZxF)4blO1^mE`Xu$Q-`WJ,RFuX?Wa#b_A>#iEFM'faAG`+BGY>S)8)=[Q4<-YUjd>fm3WSSS0i)-BAfhx^Q._0#eetrb98[oq'60[W'B#"
    "#cprEwf/J=[T.e-'aom(IQ<R8>TV&#m,p7.PU`mLQ6iH-5De.%1uQ._)HAG`q'uC$3F8(3Br;]bYf,m/?Q$)*(5v>#B,rE@dNr(E&&###bNr0(efqB#BVw;%R/35A$j=tdU3qB#o;`W#"
    "Ho<g4#&P:vWD=*vv6>##om+T+nr^s&(5G>#f]2a+<>I3-31eX-k2]HuvV5UMsS3'#.U#N#^JhhL`5cP]P@ml/4I&r.L/mju.J=]<K:d*._/5##K5YY#94e0#^EX&#&%^(#1cC;$%F[G;"
    "QKXm'qo-W$)I(E3Wu,##IKOu-6(1f)Sees$TJe`*P?+<->GW/%kf%&4TMrB#FS+Q'g)ug$%B_v1FXsZup%,V/T-)*4lbMY-^U.p/(*6K`;W>L1D@.M9@VOq0Chf%-jolMCG@KE*F9__e"
    "k<?^#sYU9V4<pc;G&w%+dGr80-DlY##h<;$RZ=oL-;dH.)*px=U2u8.n)?n&UA^T&+E)K(tlDC=g+?v$E@=T%oc``3*6qB#[P.m;ge4fUu7gS/U3,N':Jm/$t7%(It$>+$51Bu$kcZ)k"
    ";5Rr%M?o;-mBml$)^2Msqvwo;*'+B,dZU>?CNk;-<PH*2moJF**)TF4@%eX-MR^fLjWGx63jpf1.j[b*h7X2(<>oU/eqUA1[f+.)dRA_#,lda*OfG&<Jl]iBKZB(6is2l2;/5##CKWU-"
    "I^87.1pb%P;)U]=fstQs8gRo:pc7*+e3/%#kE]30l4tA#UeL).1><o9H?tM(02dd.n]Q_ux(=a%=dSK&@/9pACq@l'x9T*u&cnC>^+UD>=/JSuB4a</G)W-&`VxiL$'1kLaPX`#krn%#"
    "0IpS)UUG^#s%+P;pR6C#;>Y^,n/)O';VP##d>=k2'7(f)jAqB#m&l]#O+.t-?=?:At+7m0LO8e%B+eJ74dt>ns9ZG*`E?E1Zf+.)j9Ut%V_rO'RPaE*vVWb*ex@JLFY%e>oj&I*KBUt%"
    "=kI5%D^o],h'b5&.Z>&#:,Kc$r&pc;^9LdkOF-##w####9NF7%Vo:Z#`IU?^)>Ng1_;[L(%O[X-:?uk0Xe?#@eBOQ'-T/i)0q3L#Tmx%)e@p,2Jnk'+SxT1P*'FJLEg8O(%15o/6ZcUm"
    "Q.g-#R5-$vj%<`#krn%#h>cX7]o-W$j#]GMJkWb$4L@>#iDcN(hE%##PWx],gq/E+@P3<%3LCs?9,*Z6iFYg3gV`G)b'mo79->)4m?*?5Ra0`/AePvcv#&k(vPe51ZEg8SDOsd*X$IKu"
    "?x2l2c3Cd)XTW-*iHh+kKSuh)&A851O:Od)`]D?[S/7+#GKJW7P]C;$#70*#k1BP8;eB#$0&wJ16$(,)mgWq.sV`,)E`/GP-9x9.C=S_#a#$c%<+[1evv%k(?[jE*oh[0$)Jfh1B=.M9"
    "utnP0(G`Um4M]*7*N9XoIm6+#3rC$#[*SlLUmt%=8&QpLrF/W$.Q4?#,CvQ8*2;GYa9a;$4[S2'KKgQ&I(KmA8w%t%uiic)AtlX?[W^`P28Ylu:,r2$6h?aevP6tBC_l-#C9]uG3cP>#"
    "]/uN(pSQ]4F:<;$t.:kF(PXGM'21&=0S:v#P7e^&rqr$#]%xh(`'&%#%kj;-Obj?'[gx9.%IgBJ?aZ.quY*Q1n(?p/v6?V#B>GJLLr7M;)r2[&:;D^#^0A,KvYtn-J>wTM4)EwK0O=G2"
    "A(n>#kKj;-u2Xr$OTi6Nw4OcMjGbV$^_jl&QvCT%I;*F3<.[>#(D0N(u%BI&2_:Y#kT;j%fM5bu2u`Cj+?>2'HC]*@dH&d3]qH/2WdO5;d[4t?>gf6K3j-sZvDpEuWa7e#7=>R<;r)R<"
    "_$CJL+E/SUMOmY#F$Bp.('Ov#c3sJ4'%5.)f>Mk'JR[s$N&ic)bP#68xw?w^oLJZ1%$xENaunL(#U9*PSJgj)r@0#,C$2XC1c,gLbKj$#W2J;@X04m'N3=W-jVfqDiKPJ(5gX&OEhAVQ"
    "tprj(rB]'.Zkr^9ocXvI$6v%.s&niL.^WmLfjJ%#sNUp&J7.<A7JF6'jK.##L$ED+NGA<?U/9EYbP`G)&i15%PMrB#bhR@#S:`80^8K<7<I'bn,9^W7Bnau->l)D+G1;a+C>%f2s$P76"
    "UWB49oe?_usMT+4S=HJ3<7[Y#VdBM''B'_S4W8>,h`Rp0+GG##'9,##v90*#(wS]=2L82'%m#N(p3F$1(s%1(TBK6&B3pi'a+C)NwUF:.WTD.34RlQWP?ti0;.W#1Rm<vLR5mdT6TPb*"
    "[]AB+8+2f#<i(+=M'&LPg-F;H$-DE*,&#7.7`ofL(?UkL.jZLMC@p%#d1E'#Z(]fL%QmY#o^r(##i^l8Ol/f)X6gi'dXtM(gcqB#&4Ar887Sf3Djv<7C6gn/b@$_+i70/,iXVZ#xVP:r"
    "$@pK1/WX^,$,>>#d9%##N>[9Mm#[`*bc<X(Etl+#SXlB#wbgl8)?C#$;4l^.E,#:.')'J3TAA8o,QL:%DXoI#Rg5;%<G;^uOE-@Do@9C-4=9C-R=9C-pw]S)efHGlvwlr-D@*]-=Iq'H"
    "io@d)1O:FGs<i8.nmo;-a/^(%u+'J3[EL4ff;1og[Er/)@3/]OPvib*O$mk.0T1I;[qvWC.tBF,vF-6BDh_Q0pq^c3O2wrB0iM?Iv%;'#&VI#17+Rs$xde>#wWj%=)Zp8.)3,#>IFPa3"
    "aAeL(*J]L('?uk0,T/i)44`[,an7T%K]R_#m_Y)4HeZfC(ttD*j,(43.5.qR6[OaXL&%i-mF#jB#G>K3No)hXAw)#0P5YY#44e0#g-xb%7-&:)&Aip.g_?X-.);LN>l*F3>,x<C:gm2'"
    "/&w#U&2Yw66JAJLi^'Q0eYfY,6ZGL1g%*G`]w3g)jk%qrq_%T.-AP>#VUcm;6]8T]lis8V7$M$M`6,A#`d,tL=H-##/5YY#UFSd;,K4m'@Gpd3BIr8.<8vY$aP]#%#WleX,)(bX056X0"
    "-nB^up%%Su_LhaMGLihL*IgnLc<3$#`bk8&3YC;$MKoRL9]fF-S[`*%eE3$#ei[h(#_Z0)RnF%$2cE<%ed*5]<;)&g7H+BJZcCp#[3Jqu,NGTkI'^E#`p5X$ewAM'YorE@%Y)20<u)?#"
    "HExp7ft[9(Tem##o.o^$BHCD3B[TfLMP[TO73/hue?Q*mZVIlAOBV=PE0sxF?*0YSFVOjLOiv##W5hX-u[-C/hX450%VgU#_[e@#?r;t_8,*D3Oo7]k4*,##D)i?#3H(7#hv4X7*C&[#"
    "h(058=MxL1A$O',9$xX-e`qB#wO7tf_Q/[#i@T>,eL(-'VntD#Qt+T%;2)i0@bL/)<6<-*<Iui(S'Y70ICirA3mvL4tF-B?WWc*9YGpe*)%&U/+l_<--_a$uUOgP'R.=4:`;E5ACJPk2"
    "ppw*5%T^G.vY&]t.9?/DvaQ#$$l+$%'4)nsOYjZ8nJQJ(kUu+DK%,qt(P]5#6:mQ0h`Rp0M]lY#BwbFClO470/#fh(_)hB#7h=k03,m]#`?b,*.h8k3UI'O.mOo$$7)7T%hv)t2svaa4"
    "3t469IbeG*T.bS&&K6o/0@7=-;&@$$/26M;girk:%A'dut'Z03Z^Ew,QksI1bR7t-)`>LqY>TE+@xxe%U;_c;uNBlos2>G2CC/s$)G:;$.sH8@*S:v#up@>#*NI`$+M(v#0h;A+uf.1("
    "GOes$G<XA#t/i3X8)TF4$UHdPG$c?.kmqc)fH:a#8:Wu-976(+d^7_+a(*A-Cqi`+Z<=t$`OwX-CUgv+bLSU/m.d',@=ui(<_`w,NEg%#Qd7fhjN)>Pcb]o@;SO&#P,cs1,a_V?$,>>#"
    "%?G>#Kqq(EBW]I*&?gw#xwMhh>5+?r$D0o[lcFsTFQLsL)dk8$aL7%#ju_Y7c;,##9PP>#,a-8@btxE@fV1v#;2@8%$10*#<L[<$?CA%#wQ6,*RmniL^Z'r..nw0#V<A49Im[J3>HF:."
    "o]d8/)$fF4FPo=%rM[L(oYRD*uc>`I0A4K1qF%E54N&S'Y$tP&u'*m/-k'G;*b;w8J`9^>[@A7,79Y?*Nlc>#/F)##N).*-m5^%#v6uw$+Ml>##QYs-aV<dM3a;v#-$#t--U?.MO^Oe)"
    "blpHQpgqV?ODg;.ptgCQX5q#$6Ik.)`9`W#c0(P0qaRB5*HkmBEZsH*-tL7nqhmWCEOB(6)AjN4@5YY#At9'#b+rY7a5G>#pZqv%]UtGM;mlA%.Yu>#3iU;$>>`Z#stYQ8cq&Q/9?$)*"
    "cCXI)bI4I$+&ta*INLD3D>07EK5h'[j-4Abx=iee7-TV-Qdq0#AH0m/$b;DEO*pp9:V@oLR0fS%x7g_#DLw+2U/>G`-l68%[mYv.#90W-n(Ok+$<0W-fjjSpl/QJ(BbLhLtc0>GOiR-$"
    "5/MG#6E@[uRAlwL-VZY#K`KrQK0I9i,r[(#Eu3[7IFA8%<1D;$nb&*#5.@8%72e8@-Y:;$6/I8@AoF^#W#coL5BHZ$/p_V$#Y^l8:r$##k3k:8(dr%4K77<.@e;D+>^MK(0%eX-goj>>"
    "cGv;%g-.1C2n3-Xd2Rl1q:gF4OIugL;4_F*ttBk$Sf8crJo+C+q-8m,731:%(bw9.KuN/2MwkQ#++Z(+-O(d3H^pr.fdt9%4PFO'8OYKLO50&$Q<Pp/u^M&5*WD11/v*E*ZV)q8Q.$[/"
    "dh9U)hDr</`1]7&SKnK)[H7cM=ims.RA[Q0s=$##9MXe#*4l:$mBd1/fj:3;m=Gu'bY]>FB3r/)]L3]-j,(QptptSN%f@qL4CVm20bVs#QSa)#^d0'#3ODC;*+BqD.VM'#+]hV$5JT#&"
    "2(@s$2)%s$%70*#$$g]=2S:v#%qWV$--m+#EqW9%)s.[[pJL@-);E://r68%r3hxRT%g+Mw+:kLoPFjL=bk;.6/I8@.ElXC3_tV7,E]jLl;$##]b#AO/Lx8%3q?OOZ4Dj9p7t]7ONwg)"
    "kE9D38*V80[:LJ)_;XjL[>JX-1mrg(27E:.];/Lt6$BK1gc``3;=bx-]]WI)oikO2_o598O7]>.WP@&4CIR>#Pgvu-AJ.2N;<;S0bNs13(3.(>Iu@T%,K*+30Rr/FV5aB43:/^?F0YX$"
    "-n;f#iJ3x-wZ8:/Vu2=4^5.G<,S.,QI&[+<,P%,QF:Gv.,_vWL^R[m'kej5MVD(cAJ5YY#`nq%F<oL20]@PV-h<1K<nM5&#.V_V$MpX)dd6*?#+*^]=6hxi'r+>V7>@-x%c&o$$m$o/*"
    "Wu,##F?4Y-T4Gl;kmjR8)Ed&4Fi^,-we;Zum;Sa+,B^r.O.Yb*5loB,W:1+$X7#d3/t=Z*f)KA,OGW4'FGC=oPg]a+(5n0#_W_r?(q/p%bQsr$8=RS%D?%<$x?]spkG**4(^*E*A@=`#"
    "jC+Jd*MFfd%nu2E,wXM60t/gL6M-##Vr>r#MHP)M_7:W7E,>>#=,^>#)>cY#wDcY#xdY+#m$G59H6%Z$$KW&48_Y)46a?/(0s7P#Tr;wX4*P:vw-AC@pf(B#@/D9.;hfi';S=OWes0B#"
    "*Gc>#W`s]+R;G##khw[#ESa)#JXI%#8Ja)##pr>#m4058&RP)3V;G>#/MP>#HMG?#,fh;$[D1pL?@R>#/N`,#:C39%:vO?#5T>^=@IET%Aomd+PRkU2U`Lk+T4es$tO#s/63tD<:_Wp%"
    "E7dZ%1Vs]7cA^*+tmP]4Fg*Q1j-3d*TBsH*lJVk'R3'q%<7%@#%*>k0,R(f)Y@'u?l@)h)LYaF3?-;o0G+IV+n00&4CIR>#SMfQ0]sZ*=vnHo1du]51C&Mv-VEHn0F,Mv-dPkE4qeK60"
    "4i_+4rkgQ0:9dxbmG,f*Yr7q9/q_]4<412Dc*[o1d`EW/dBEP2cJ[v.-Q6RCG'Eq:GZ^3MxFbq0$),##/%lm#Mfe[>xen&,nr^s&8<kR/#9G>#uX^l8<G?v6:_G)4xXKl8V+aWB%/5##"
    "+FgM#eT5+#Q3=&#+D#jLXIdY#_^Q&4(&###$%TE#p*Lf)DcY##FR*X$CpUCeOaOD<O*]qJb=@]6bTT+4<#BF*W#]h(:#?0)YbBg<=O-<8kwxW7JTBt.2Ioa/ifXjLW?Q:v+O:nNBIQ]O"
    "]W7DEH2?5//iO?gaDEPAH#?lJ-K1[[4F?5.j3<&FjduT8;:w8%+Ntr$j[+gL>+Z`Om@$##0Suu#ZJ4g24/F4'?L82'`2Mk'?xU?#ngn0#,Yuv,sg'P3%I4GVUjfM0$#nx4I@n>#n4058"
    ")dEp@?g/&$($m+#=ORw#0-8>,)GXI)ZNgQ&K6OA#nlp)5$rXI)55%&4xPt]Oj6_iBnhoQ0VSmlPfM@h(68Ve$j_eM0A10Y.;S:@6VwDD+`&K&,n[61M^OJ#M;@v2$fke%#GCMkL*^vY#"
    "oDT>o`60v$,DSfL^c$G*`d5n&EIIW$G$Lu]ApoA,8gfX-/-6fE)P_s-O/aT/g)1P^mG5<.Ms*.)w=-m/P)G_5LP+=Is&Fd*jU$1M@ZSo#T4A9$<5>##FE.,DZd2[$GhDsuv)VOBlTLfL"
    "JF-##Z1.[#_qg[1I^fW7M>Y>#'8>##SMe?Tbwo,;a6Hk(gbtM(_&`,).uv9.oajiE=t1I3J=UmEkh;a4IQ,o/R[*m/G'WSRpaKA4hjSM3qu<</Od@fh[u+>PwD88.t5P_&UVc##4,>>#"
    "06S^Hf;?T/mRIM'*pb@kZ5,ugl)-&''Dq8.XQB<1S$Q,g_Lu(#HV3T#[_P/$VYu##Sc)^#%,>>#<Vj.FP#L#$nH0K'MH/i)k;oO(pso?\?JhWa5K'(]XV*420(p%31E=_J#/@S9$`@%%#"
    "P,66)Ol#m8b?[A,fEbT%IZlf(C?+<-(88^$kc``3_:GI)RJ))3)?E:g5vnqJmtZ?,xog/-WP<'4<#BF*V.(+*1Lxw#x2A:Tb$Hm'bT180M@m`uPBXg;kd?W.w4;'6l,:q%W:;'6fr4c4"
    "DJ8d)04.Q/KW(5Q,YIMd[Q1EO(o^mDvYhuMx6K*#>T?D*tE*)<Shvu#H#[`*Grw4AFT8G*8.M?#mGcY#3+iZ#7,Y?#=:[S%7ARs?.Yl>#@=g2'4Csl&,fWP&3^Nx%57'L5]owN'+/###"
    "'G0b*uX'f)u2%Z>R%:3(i9<?#XQ]s7gQl'&3NF:.o#b1);e=`#52&L;L2.]XHX3E5(JI60kQ&w9Y[X&u)2q^#iR&K1m@E1ph$<-4tKB1p'FU?^E3gQa.P7Rax:]o@Nj0[^.cqZ^k@@8%"
    "AtE9%@DQP&?*KQ&'48r`ZUjp%Y]NdM>5`HMY<ws.Bb2D+(GF.);aBn/*8H,3;Ca8.ANCD39Nn)P^Qrl8Dv?F#0%Y16gMGD#&@/q/esk4^_R%N:li71(mctj.Q5c$6MmvI%Ic@(#';G##"
    "g44R-XAIn6B4i$#VM&`7[(XP&Ob%[#p@9586=no%BYw8@G9L@.mRX,#<?T]=LUIw#uwYQ8EH#n&9.,n&0Qt?#+hj%=iDfJ0P@`;$*+BO1'KdXp>i7I20GNk'G9pM'6?T<-wS?i$4i^5U"
    "eJI#%m@6u69j0J1rU$I0Kfs#RJP_du%LSQ0:#=,W,Ii0(B513MPG<qME@b1BQ']W-&7T</v?[Y#iJ_i'4GIA4fh18.69=^6,Vu>#?1:GD^O=gLaNJs$(*,##.P(v#)KkZgx87gL6Hj$#"
    "@Aei*3>JZR4RXD#`Q_L;;$Xn0bl@d)Lov[-L3rI3sh;.>(Db]+h+h#r[wd42`t<W-@Jqh=I[o(W0hBuu7u@.%+=rr-_?o&#&o'eZ_i?8%f?%##KKfi0U/Nk'Du(Z#Px.R,(Gl;-S-:C-"
    "#tV_-)1%:;Zw[&6B=3T%/g9CS^u$s$6vNx%FdUg(/H)%At]Q3'dVI-)6.JkrxQrs-=oc/ZZ4DMM5=pZ7$fSm&;rUZ#n73jNV):6&];@gLWSU<0EW^v-8lXi(VS':De9lPTj42GDsuDs7"
    "1W*(A-YG<%=+06&r5=sQ(W8L(cf3I)nueh(^2M0(LfGp&K/A'.h+'p7/Nm0)btHP/>jTv7=ohV$PQHh%Rp98%hq2t_tx*GMON-^=F**aEe7`o_#4L+*(ROW8a>),)Nmcj'<<,Q/2;2T/"
    ".cg/)tk%T.*bM8.M3or'0(m]/P.k$UOQC1U,UvS/gV46(.&=e)e,D0(i_7w#WGiA5Lk(+r2`lM'c<tx-9h(G#*]2L(76*5(Q[`$#D3=&#n9>'$Z(V$#L6E8;UYWC?7P?>#.c1?#1#US%"
    "37nS%6p=?#&*T&=2Ep8.)w8&=JONs-K@cKVv_;;$lsGQ:3JZZ?w]###jPr0(Gx/wJrI4**27E:.6KbV$s_-lL_54b+6ax9.>?C6Gkh:rdd7oo<NOG%$R$3xt.l,3sIp9^uoMO*vfbA&v"
    "+3wiL2CvA#(S[Y#oWt&#*JY##E$###Qb%lL4f[i$Ng:6/tl_V$UZ+oLSR,=$M3GA#)B]A,,?PA#'IEY->l,?,2)(lLq^Z[$/LPfLX.w1MD^XjLT7OG%I`g/3^pY3'-'uJE$[L::X@H>#"
    "1OEW-=AE0?-qrlfljd.%nWbd8HEj0;1BVC?B1*?#6YP###oDU`@Zu>[L:$##Mkw;Hhu^>$oGLs-0IrKM-Ifs$d3G=2U-4K*?YG>#dVI-),6$:9KR8KUHL%f$qu.Q:.T7KN`#1q%SI%f$"
    "_GF)=$P-h-3,g6E3=%BOI.<w,N)>>#_5YY#cK17#@ZUI5Kd''#^0%J&6mn51Eg0aE]J5j`Zm5x7QaPY<@CRs-*e^v7vnjd=hA$i$9%=gWoM;T&1gvnS7`Jc$e,6W8_]kdM14ab$d,JjD"
    "'.%##4[fi'cc*s6`5HJ(*;3>5I=e>#.JcY#sIpl8Q1]p&s(Rs$QETW-<*7*lv@Nk'TBK6&FKPJ(h]qB#bed4%NCuM(k;oO(L#G:.MjxM&p.%i-^FL+*QGd]uQq.3UcpTIuwWXY1(^)B#"
    "Of4;?)Y4duvX7m0iEk8.PkIJ1NlC)4M7W20#2?5/S_$##2QUKE+0,[7vjUV?ED/]$@4I8%6G*T@7:.<$,aC;$1H@<&aw2w5;SG>#(U%t-@Xm(+w+oh(JU<5&QG@L(5&il0I8T[fa]27#"
    "/l1x/`-*g2f:330Fj3e3i[O@$&Lg;qat96A#%e_#DlU?-ZCM?#Y6'Y$iO$##p+%?I-]R=8>MgvHhAE;$[qas8C?#W./1M9.5S&mAY,l8.($gx=,0oPA5Y7a+3^5C/PMG##'/pj1[15f2"
    "3@bc2NA,G4Gn&+O%7EjTG5q-%ru.Q:f(=<)X]G*Re4m&F?p$<ABX):1+9%U9%74^6]Yp+4o(5c-.hrJWFP)Y$qvp;82c_;$5>?9%9x-W$b=#=C?#0>C2OI1Mv/P(5SV,G4Cjje3Wo#T&"
    "VxF(#>hD]FdZ?crULRP&=&Wd&em;9DL*tM**$4Q/eC4D#?S(XJ1+YaIUb7<:;9*<:3ZCtJ011^Jm####0cCvuCfR)$Dxw%#&K6(#D*QX$,GP##IM.$$O5.1(.]Cv#388d/,')B-+S1v#"
    ";3G>#s@O($4`3/%Dipl8[ak;.:=q(Eo9(S2w4.5Jmqf]?w`,##g2P#-bsGn&F)sx+U&Qj'C$]I*UYf)*>[f5/AUr8._1XiKV?d0))>WD#vu.&4uW/7/&B;T%1f>f)i=+</i9TY.U>'r."
    "Ofgh(j39>.eS.P'hGXi(U<o^)p5A_+lh)m/qsl,69gD>10PFW%Yj:J;*owt$&S4k'Ed8e*P=LX#Xu[X-hl####)>>#P5YY#`.Tk$vbZY#ox6V8&EiT/rj=#-dFd>#4OD](Q5G>#<Yo(4"
    ";@h>,sQ#X8&<.(:3^?g1N_=A-/]u>#p]C;$m&$Y8c2Z;%e)Ik0`4H>#N#:;.NQo],$3Ss%E32ao(JHs-uHB;8IUvV%Ms)Z-Fm@#/XG$W.8XlgjLqp(4AeH?,(W#aETQ;h`]W7DEc1?5/"
    "_*%##pI^l8X5At%-55##,`[3'#^*H=T/[+#64%[#X>gl8II9j(m4OFG*[Q4(kp=G2?%<;$*VL;$1dx>#xt>681]b&#N&Y?GmP$(#c==.)D.`$#8sgZ-#M-kC>p5K)`5qB#bM?p.dxrB#"
    "EZko)3jqC0/SL+4rqsAPmaG7e#?Ek=T),##I5YY#`R5+#(&iY71LGQWan)?#w<M39[6Qv$-EdjsU<l31b>:Z-6cbI)0g0H3nrZj'xgFp.Xc=Z,IVf3OkNXLDvR(C&T/E+=;En8@GuEb3"
    "BgdC#Yv9%%q%81(*;kE*X.YgV)vpf)gI>C+P'rn12,jH,njx6/2toc;F'J`49'up%>DXQ1uLT90dK4C5]OJ:/]:>B8[2dAuTf9P4Xt&%#I+65/?x8^6;0nr-aK+'#9ICP8*`$s$V',+3"
    "4-xA6efHW.kloA?jwK#$bo8N%8>#$5[1Q>#M^t:.::U+*Ww39%X]3&+quU'#<PEE-K(da-;/3_A,+_,=nqSZ$h6NaEjQZTgX0N?#1UM39YWVs%;IH`EQU#W.j49g/kL8Q0k%ns._(?p&"
    "5xCp770lA#$]-&/ufk,#%Gu3FmVXO2RZnQ8GL*p%1-m+#n3_dM_p-6S'SgY.XGKf37hCO1,:]#Qh+@q7r>UJDK8G^>d0]Y#'HoW:h2+6'BCR8%l&eT-Tq1d-@C(Xqs_Aq/q4nW.'FFZQ"
    "dwgB+ktbW%iFOW/M+=>51l.>>nmX?#:LNT%8d`,#2Mc##l+*jNs(YW%[I=-2Lt2D+P4`#7?R5R0'bq,3uqZ5'_lJ#$1sGNj-Mv&#<Ja)#*Za@.*;>>#vR8m&0cC;$I<Su%/.*p%5&G?#"
    "+wJ]=6P5##(-`Z%&]fu%O2DB#xR0/3)fj9/u4R[-@_v(+G@>$%:4VS.Cp5aExLP)4earRDfGE.3e&&J3XE/[#7rs9)Y-VpT5SDx-,kBloS],G4umJ'+T_@`<BLcM()l06*#TxU*Ddu40"
    "n)1j(tJ';K?qGR&Ijqg(#uKJ).8og1@f0R1WVl.3er?T%NvcSFoA(a4bCq(%wtX)##)>>#h1.[#*4e0#<6>14Kd''#j,>>#`P)T)P+NBFlBpqBG/5#-gj/v-Psb>-a4B>,N.sqB'D`'#"
    "K.@V8i.I21fo*m9o:o98:rNY5S^wh21uH##QSB@%cc_&FI3w_#?P($-rAMk'?iu>#da,;%E:_E[O[H>HsQnA#UKMA.1u)13?aX29Kt5al@tHQ835?Z$jv5Y8br+3)aj$125wq*4DvjI3"
    ",JS1cES%mSK<SqLFb(]7'di:%/950:ut-<A?rxb-%Yd3F>&'J3GQR12FBu$Q4s^B+@v9kF&l$##$DR&Q,xR(#4txRn%+X/1$TBJ1SmZ3'B+`;$T4.m%1I/W--?b:U2DZX$&W7L(I[=PT"
    "<@JflnP#D#xpG(#H55T'1f(?#ri_V$4sNx%&wD'%Pa3]%eBD>PaX_Q&AWUK(_>Mk'GR<5&_ljHZC&rl0<(^JDm(Jp8.U6gLRKL&=C=%w#+=mQ8%7Ds%?1@m'<S'<0fV@h(^jYN''KR@B"
    "F,rl03Y@W]U27+#[&U'#r[/(#--^]=.DlY#R=lcYnR5o$Z&jD3=`7q/G[Qc*A/?k0=)3K(Z_1+*XpOs-@7qb*O(NQ/UD.&4/rU%6I/3T%q%NS,sp)F*dKbj-->Ms/Y/B<7)f;X76:ov7"
    "=`jE*KpIw2?NuO0+_pt8-*#114#=Y5$%Mga_r1i:3FWH*l/E*[]2p;-BEdE&Jk%##gPru5JWWf:gT#i*ONnQ8@4I8%Zj=oL9f^^FW#UsJeR$)%47][#uZQ0)ji6<.H3/&,%,]L(W<tT%"
    "?OR[#1jLl0QV*?-i7TP8U]3?-Kd''#oKpS%8aRp0#lS60h^XaEC;U^#6#>^,iNuW%F7H>Hl=-A;RS,lLkeSZ7c6D-kQZ/k9G3Z;%gH/W-Pat0NReJ60)rA02,_:O1YFLIZZgb1:M=klt"
    "XxclL7Oc[7*drN*SmSp7'g8UDNuhW7dxYc4DK[L2tn`OF]u->-r@%o;rO&FeQrxFex[U0Mdb-L23q_k1#.G31Vl#T&IU+T%c,i:d<9%8[-l68%o1FS7Se*?#-]hV$45'^%-V:;$1phV?"
    "YIW-?k9]nL^n-##v,>>##mGT%+/###I-BQ&JVD(=kmTc-/jo2)@O9]$:%xC#vp%$U<O1P8X^O&J,s^DI`lr-#&5>##Nwtm#FYK:#^?O&#wC@Z7k`_V$lc1^#=F%@#n@B58Y,e,E(AcY#"
    "p<WE'31Wp.l>,<.saZ*7XEp30#*Yk0@<_=%a3FA#s3si00R:a4m(qb$sk'+*+.,T%C=3:.bhR@#=Bs*6Vvpn/OVLg0P_*)+uPe51rjA^FF/@;Y#=G]#-2Bo)=kP76]AVI;BH&8:s6e'&"
    "4LM8$-K[IqxeYC5#ZQN09OtA#gcZY#80?>8u.MT/^((f)kE<UK@5_0'N5G>#jhHX-%*=UKWpUX%<=d>Hn?o@bmm$&4LBp>6JTF78NVZ;%4hb&FJ;Z0)K-1,)+x>c*?Bn*N>uOd;Ux6La"
    "&xZL-b5>m;3F+Q'V_,/?K?7.M,ldC+07?c*vI'+*6o1T&Cd4-vewAM'XuZW8cWa$#Qx%8@*`$s$3Nhx4t<CX%'2>>#4LgQ&sxNS7&;cY#91CGD80hQ//(wo%O>7L(i</<-I)0o1[R5+#"
    "S3=&#v6uw$rkui9Tw0'#v0vQ8F</#G-)u&#t9LJ)]D_w-g[sg;H:0oiFBG_hV?9C)c2:[eO/EK1&M&D*4m'$-sjhj(JD?mk9xw(3D8Qs8>BCUi/Yqp7XAE`#'upl8S996&DjqK(b>Mk'"
    "QQ53'ctc6s[Zfo8VU>W.ei<877OU8@@Ei`Fgq3aXxk,K3-2%6/2<T&=8)Is?/D]5'Wtf7&i%aG<6:BeX&9F]F`as+;Xe[Y#OwfZ$sCEh>Vk--b]QpZ$u&UH;^O&+,h&%Y%HBrA#xr8c0"
    ".=,)#3fLZ#irH2Nw^h>&8lS/d;1vZ,ODa;9#[@9.B:o)<LukA#r;.2.5j?4Mv%g+Mrh<7%0#+-M'-w^*&O&*42?712)HhqB(o^B+er9T4IoKS.xt529j:X]+x`q%4G=w>#>FDmLif?>#"
    "-<M,#/w&&=3L&m&Y`o6*@40*#7Sa/18d]Q(@AvS/^T9U%gMYZeeZrGDMvjj1Bc``3]lf`*.PaK1WjMP0.d$Y75I[=-dw4l9m@Z@-+=L+*k)WB+RKx],ndkA#[%x2H<P1U8(7*/1Oms+;"
    "pAq@#eFVa/4%%s$7,[8@^nP#$R*Xf-oJqoqiLF6'o`_;$+KXV$-16m8*]-##Da&F4wo:Z-05JL(U6'q%'<0/1EwlH36K?q.A,m]#CmPV`QJ(<--p/^&A,wi1)QT$PtUr`JEYw],P?@QJ"
    "&DSriiv-buQI$##f(058FYbV-'ER8@Mhq@.pL$X7SNo],2/bS7&>uu#_B=@n6xJu%Ld-8@=;P(HujT1;fI=@7pHD/#ws/X:Tsb>-=.)XJ$C0X:CLR29Cgt`SUfGlLv%k4#QNQ8&B&Jk;"
    "Yhr*%@MXSq>4dmRl1QD-WB?D-G`e+PkI6>+j4`D4uPiX7mPdA#q40588+Rs$)?o>#iq?[^3Q*H=kM*R'YYV50Ashl0x_%E4-RKfLQVr]PI*,@MJEih.[PHW.A5*f%O.BmJhR;h`]W7DE"
    "&2?5/I@$##]FOcM3dSZ7.EVL.'qJ]=&xRt-_)$#>^8F',k&d3'?7.[#f=?e4l^w##5>F*$(sC$#^3=&#,7#)#+q&&=;%`v#`krW$l0a:K:^b',[vrx+FBo],+GXI)vW-22,]Es%rDQJ("
    "uAOZ6CH:a#'i'U%r7q8.uv-ou@-26(KIZ&YroJ7/v<tZ'Lxu>#*4g)$4XBZDIHfM(d93'gNY$8@So>w-IlY$#gBVuPbGx(mwkQ>#DF$G*v14.)b,)O'wmJj(XOtWqafW&#O;^;-wWJn."
    "`Wt&#_n@a$C_=h>Uec^$)>P>#0v:Z#uwYQ8Fn$HD4e+T8R8CsHoO7a+*eRh,J?%?$YAUPMQTxv6$Rt$#,Qo],VXI%#@#qZ-am#7&D5%&4_T/i)J*)pJmL<$6]vZ)4k&l]#iZn/PSgtA#"
    "#olT9GCm<7D9gn/n'Prj?g_11q*TW(T@`v#`6/SB:q@6<97`RBFAYQ<0hRf**@Es-BbsVA-9Hv$+V3j$r0WmL]1C&=uH<+(Mo`=-M#/c*@7x(3B,6W84UrDN8Ag0%h?/<-xOb2%Q4]_#"
    ">_B-)?dLl0gW<q92H5[7pGY##>I3T%Kj@Q-n1O$%nB<IMBtA5'2ctI)bk5nAki7p&CeT&=*:#REs&p(<Mq@'?0_-a$8bVqDMlYIM9uj22fuGG4dm)+=0[I@%uaA_)DRDp7ZvKwBTT<[%"
    "M),##m1.[#(t9'#m7.Z7s4@8%hN[Y#,V(Z#+9A>#vaY+#stYQ8u0`J:`^k6'qo-W$-Ttr$/7?m8Ykk#-5SvS/2Slo.K6mH3f%.f;h8G3DCXOD#cF-;s`DDr9XN-$$gpM.CxU=t9TL5b@"
    "tb%/1QXa'5+R&,2Ua@M9nD6A#m'oB0.J1v#1N`,#CgXveWxF]%mXu.:r]ju/K.RW.DRHc*_TT6&-a,,2:YvX$X)pGD=.dg)=J&v5(k%)]Y@EZ'oxBr7@(5Dk]C['&QUgS]52<r7ne5an"
    "f2:dOnd$p-V-Y`-rRba*l+$##188i1Y####U&=[7.M[S%ZhEW82#89&:Les$P2;-cL:$##qVAQ82o/w8<lS`*%eQ(>JbSqDd@d;-`DX[87K$MNZ%](#ErvN:<_:*4Kd''#M,>>#6+tg("
    "N>YY#-^UV?GM:<-(;Y_$#eibltA^<$UWlA#C4n[--xJq/>p(B#/bjY-;,6w-If?T.`1.[#wW;m$`4-RE%.%RE7dxQE$tSc;_9tr$4.7s$#A8'$'.gfLV`;v#r;R7*ZJ`B#[p:80M^nL2"
    "$J*X.1KYA#+n]r.)(up@KNJ;i4XEs-P4<g*6&q29OQo`=SQxYPa@CZ-k($dNb)em8ldq`fD?W@I8I$29wZToi[-a;$%0]Y#8chg*I^3HMidf60/.NEOF)i)NuiBM''$+m/<5#5V.lqjC"
    "*#[LcfNXrHMWEh>J$m+#Ss+$G)JtM(_Ztg$GH%#vm,CkbPk>_A6N.B4LF_/#TS;4#Edav%1oqr$7,%W?8a82^aHZY#r+TV-$C+kbw>'%#e7.Z7xCrv#9UWT%5%M?#92c?#9u6W$:a.d#"
    "0:jl&tl_V$)%6m8K`###$]KF*bd(,)$3nENok0b*ujrI3RDL8%ocfA4<O9]$e)?40`tUB0`RV'VsA:ti&v`tL^F0HuG(]duf)3TBJw(0#0^.RN'Bpc;K@9MB&&h;-P:2=-L[-&/$bs%="
    "h14wRcg2m.8q+j',QUS%W3;O+q/_oI+0+Q'ojFG.DCv5(16ha+au7L(VmwiLi?FE*2P;g),3u,Oo)[)4i7:[ji@Ms/:E1N'h-=<UeMo_4^X.f*i+`9.:M&##wCW1:NlH#6CB:QCLQ<R8"
    "Z<]589.3_7X:je()_^7/c-H(#Un7lLKX>o$h@p#$-Wn@%q_n9.;SG>#coU<-SksY$(.on87bxS8KH#XAHQ/0:hxQw^%7k0:?Hd;`%u-ro,D@Z7gAP##:7e8%=/MZ#1E>=-UmSn$TH(',"
    "#%U+*;u_amo:8L()4YgLmg?>#jGCM'$q*m/$m6A4&^j/<Cn$q7&5uu%7u-W$$$tr$.-m+#I3':%h####/esY-;pdi$CQ1l0%<oO([@%gD1KS^dAQX%ZktMWY#mx#[fR5e#LRMHuC2,9)"
    "H/5##MXY54LR5+#dEX&#D*;*#8Ng&=AXjp%sW6$gRhY+#$7vQ8'&###4>?9%v'D/:;W9$-b;ou,R#1Z-$DnA-;(Ei-tr@OXJH/i)R6u_0NkY)4gp:Q#g6V-PK2Lq^Qkt^C=&&2Yi_dtJ"
    "^tO9+@Ac58@'E'#X]Q(#UPQ_#d*^sTTex3'%qHh1i;oO(ROt.L353p[0#6tLR15%Mn@6##CMG13It9'#;L7%#nOs'#(hs%=PKA#G$?6$$&*Mc#1C/2'6BHm/fcWI)[NgQ&[4&70Omu/("
    "@Uw9.&B/lB:R;=.ZeAk#%bRB5#p-;(%m_sH0J:a*u@sN0T5-$vuk]&$Yx9hL/.^%#'O']7XPlo$#+9Q&>$#N'kAlY#8c;q%6Y:v#(6TS%^MNr&K+h+*^qr$#uaDD+87q+*.+?c*c$tE,"
    "v[0b*m?bU._kY)4=nx/.(IUb*k$=Q/G<Yj^poP%UjA/k(jg?ae$AEJL#)>>#FCsM-8lIf$qRZY#hV-R9)88dsM7.Z72?<a*'oiW-Guw[>v[T:@8_9N(3owN'Pho20?q82'lcRh(B;uLW"
    "A2oiLFO6'$>OtA#<L;_Av4O_A>Z+##BHe#%/)j''VWO1:ZU'9p@G&b-+<IwBJbET.T5YY#<x-q$YGSwBW>JwBwQx:H6P<]XUH^#.l[=gLU6Mg*Gbk30,c$s$M>Rh(vUk-ZASPw9?Vlw9"
    "?*qjCrd%##7B3STi?9C-IbTn$[p(N0->G##-VCv#WsRfLVW6M*GNsjtgwv>@Uk2IN?pZ##E$###s'P[$RNnA%Zkc+#v'dQ82c_;$%3xu#)%$m83GG>#iX4gL#OHL-r_DI.WT^`3D(H/2"
    "*Fe;-W$tnLsa8#%J:)H4Jv`sfcpS3Mw7J(''ml=-CKWU-e;`iNuG5gLXp(+$>AWi-.c8`7E9nQ8K=]$^-G^MY4>#29]%(m94/Ok=90Gk=Rk@a5-WSW%7oU;$rpY@.-'9&=IXe,=Og&ZM"
    "1fAN-7(7m%+Y$##e8vx4.:2i^jln51hA?I%oE'qR+QsM-x63#MjcAouo_:Y7gx$s$.iqr$)SMmCHIhl80IqA#$b=c/>QSW7PSuY#bw>^#4cDdOc&1'#,Yu##VNO=$.]JL(2'2K(6otM("
    "/&JX#5?:;$wurtEJS>+rxLrd*tI*n/RDlX7%_5n&-Wi]./>Y>#fwnm%uvY=(pEGJ(p%Oe)iiqB#8<v20m_G)4NaZ&4o_Y)4R#^iZ3j-sZ*]U2$Y20[2'A=xbP;8/(>'CC#7m9hGRu`=-"
    "iHWe$00;W//>P>#6Z>W-*EvX?XVBNEo=AL(F3XA@#A`'#/lR)ZxET_-ws/X:?DXI)%sES7'55##&6,###U,'$c,(W-fFAgOn=kA+3i6m%H/T?nrK'1;vFlD4Y[ReQfvv`*%6=NB_iCv["
    "r4?h-u*Jk=+wBM'O1E*6$G`'#FjO`<qw=bRhUibR6@j/)>^hr?FVnkX#EIkXfc*+<L@NZ6W0k?#`i8eZHF.D?o2O&FfmM<'+I&N-?G$9%EJrn/75v(34-xA6H^=S(G-##>a@Y#@rT7h@"
    "wjSU`Dj9W-kc<L<1c;'#T,GX7#4J[#m40584cL;$$L,'$wYlh'Q####cGnL(;#?s.klqB#K[ms62k^#$Fl*F3)B-&4uY*Q1::@p/S`*(l/'(:$aMpr*tS;K1[$o@u/gtAu&o7-#JdP&Q"
    "tY5&#*;m^,&/,##uwM.;C4>T8.cCv#NtU]$_SqZ-5F#gLue6l1Enhd->o]cPu*hb*F%KK1,TLj'+pm#UKHs<7jDJnN&9EnNO').&*xAh;Q'sbNNiPrd#)>>#@.J=-o39c.EqUY7Ts+@n"
    "q93*%WSUS%AD.'F&k;W%AFg+Mv^A5Bpci`Fki#aND:nQ8^GXk%?t.RAwhx>Igl)?;LFl98Q[<;$-Y:;$?V69%/L?l(3eQh<a2nh2DM::8g*fA6AP:%SI:NjT;./b-FE3ABf3Bu-rvFD<"
    "6xTB%^`Sn$0]#d<0A'gLaFp-#6[tG%=;E)#>ii4#DZEZ%'>uu#/vZS@%$Y<-fUl0&cF[PB%wrc<;N1U.[Q1,3^E9S)iVfgFx=6##IlK`#3t9'#W3=&#(+g(#)J1v#3>B#&gSBXoUe)?#"
    "qh]/1:DUK*W2SY,6FZG*oc6x-A0;hLi4JH)27E:.H:w_Faa?T.SH:a#@mQv$t@0+*sII[#Vi=RDdG]v1YV]h(?WIh2F<4>-kZBS%VCEL1J(0GV<dSk(AP+`uI>8_#26>##.5YY#V4e0#"
    "(dZ(#Gu3[7?(A8%9%)v#jb&*#.;cY#;GT#&2cc##^]OgLDMwQ8.DlY#OsmG2#%TE#4k`)5=eln0Mb?,*#<ms%`FjkWl;ms%]f)T/%UGccC5^I*;E=r7,8dG*x<uT%^<]@6a^=D%&J.XH"
    "4Ms%,`S8H3,xh<-`k)X0+H#*t,1v?,%M42(0NiQ7SweW$S8`k'=TOR2i)U+,lR5jBbgl(-mxi0(KY;6&HBFS(xuDx$x_$##WDq2D(`.(#*mU`3.5+-25'@>%+Jc>#tsb>-1sES77B:=+"
    "t>]Y,,PbI)g82O'Hbn[#0xGo$b,rE@YR)?@bQW2MS9UuAcG0$-M)AL(H%)Z#?XY&#6-,c<E:'Z-Ru'P%--CM'vN^Wo@I3w:/6U2:s)$@TnN@W%HjToIaYBX8:^4#-o_,4%f-u>ecH<mL"
    "j&)Q9TPekXwmt7I+?&##`/;W/cSc<--.#H&&3Dk'i`[h(-aHv$_m.n$#n2:@&<8H<(5wA#o*wW_f'+KN4SK4&EC0*#:$`5J.AEt_Yr_>Pu(]dD-:NpBZZOo2s>cX7k7`V$p?u#%1lLv#"
    "M0`H,4%co7(^bm'fV%l'KsL1.heR3:%R>)4RU/30R,=.3cI=@$CO@(FGJo`=4(Mp&3flRA_EhB#TD6C#'S<p/D/-Q<FD4jL?cx;8&N,<.Kd''##wcT%EY4T/&Ib;Rw6P%7-Z7>/Hu4d3"
    "xtQLM0#Og(_%#'-R$0Z$PJY>#rGal8f-27W[L.t-2;jr?r%7@^E,%m0)/]^.U1.[#nM)Y$N7H;I?A$aAF*=`A:s1B#BlqW$6k=Rah5<Dn($+.AKU?j:4;,%?W_Q-dr-)-M7gFc%aroFG"
    "fcmHdrk)aNFaYgL*uH/80@1B#6Q?(#V.pS/RR%Q'KbZp-i?MqD*cGHMalx6%mlHqDJ)>>#l1.[#F$Fr$.7[Y#u[Al:#PPH3AuQ>#xg/&=A<0<-#rGt&_(;?#B-0q%Q;2'obE:K(PEO,m"
    "(<kr$=%mr-GAB1MZft.Fh]Js7nCYa*:)v=-TC^$.+]gI=LYOf*UZd`*6O2D+Uee@#,UeX-g&HR&nGhx=athP0Nop`NQW[F%X-xA66B2gtUqDv#CILGDbJ'dMU:ho%Bcgl8Er_m'VpY3'"
    "A)Bs%G/d5/Y5YY#^O(.Pgpd##a'[4$>;1_AqK$'#9=*9%X8/m-m?'Tai*iOBT]tm'h`Rp0Lsm`*>#`)<Z8op&qo-W$Y0M4B].q/)'M3]-UOl>5Z_Y)4[o%Jq%pS@T'5fKo?uP)D$<kuG"
    "RMHXJ6@4%$j-4&#;QaV&b4xfLWDAW$/B]Y#rU=_$T*V($5_oi'u7w8%2jTS%oqX,M+L5&#FNOY-0]F_$&#ta*e+GG-?;pn$e.qkLx>OiE?tkD#0pw.S?,Wld(sc@>Xu]WRQlXYCi@eEQ"
    "_B`@>:f(pJ7s#d*XxclLE,k4#d[?ZIxRbA#7^Bb*99.e2`4H>#qE_c33IPn0jrQW.e-s.lFEWN9%5ot'A;8F-fF90%PZe%%Cnm`EegV8&+VRt-b3R`EpBb'I?`8F-aU0_%*u$##w3r+;"
    ":#pG3AD1:'MT<R8Mvj6(XZ_>-?;.T.k*LC4F-`W&4wk9.pQ?(#.YqP'u.n6`w)pYDioKdOA,6`)Lg,Q/,7#)#.Pu>#ICXsHY+X-ZnRfi1h]$<.4wUs?J0(KEhh?lL%O&mD@#2_]QHOU."
    "?Rd(+EF7WA)Z`A.&>uu#kqp,2l2h'#=iwZ7^#;^#m+058v<8K'-[Bx7_a`s%.Y(Z#O)k>5^(-##O^eL2ax@M1@1YI)KR[s$YfEA+@-<r7ut]G3M*I=&DvOZ6m('J3SB:a#EUAj0)eUN("
    "KtMJ:M[CW-7K'w-<?0o/(CQk.oBdkK3U8EFj8-7&MpNI-iC5C+=kV78%c9j(3+4nM.iSH)Ak8%u*X7m0$So=%bg=v*0hf[#7p-90FhXs%xTit^<M<>5[?LV6W,ZKl^.[s$4^`,#/Jl>#"
    "IlN2EDs2L)i3FG2e#p&,04$K)6+m=&I)>>#P5YY#Bl0gLaFp-#TGJ_'ZEX#7vo8?$(5,##1NS_'&5[#@mMN>,Z0p3VpK&'&MJv`,Pb$+Q@r?>#[sBM'3H+m/12V`3C([>#Z`7M+P]8D$"
    ",)###vxdQ/oMPW.Gk2D+5)q8.55SX%AEg8.X$nO([/TV'0c37/oSlr/DMfa*VkC<?P[gv-4=T_#d?]*Hub-*(v4b(NH5:a#uXt&#3^oO(gujo7cS1T.&wfx=D0M](KLd(+h,)O'<]Y##"
    "vD2<%Z6M3b<HZk028Ds/89l2'/&w#U<Y)0),kGD#LXqiW/U]b/+rZS%+g68@9*Bp.S1?5/><<$^9/h,H1H>[72Y[S%HLMmLw,R##6GB#&>q&6&6S<5&8Q[W&[=S+)vq.@#^%oL(#;=.)"
    "G:%@#t7O_$$qNEEak%9&)TUV?\?ke;-<V]F-5T]F-8]xb-Y%rsD@&^qD^H+W-IR4H,YXaqLh?9V)CiJq8_Ek?#<Lns$);YY#j`9$&)w,:.64%[#Xm_A=4X0j(x9XAFQSjEe`^9Z75vra-"
    "xcN&BYp6HF>cQ_#'upl89PG>#Wv10(>JI+6eD%Y%J?M&#)CwGM2_WiK=:3Ec>`8o:Nr$v9(;AL(N7xe%Q`h<-U+d9'PAcY#/GG>#awi5a71cS7Bd-LWD0[Q8:)Gs'BZ->8L:?uuBdNw9"
    "XZfs7$3*?6Kd''#to,X&0CW:@)TQuSD1,]RQN'2BjxaL>)G5HMBMD2%dwK]@u23gS:I+_.,D@Z7nT(k*jXggU64wR>Zjv/;fZk?#pDWXC=XUZ%t6Y?esn=&;AFYrD8.R<-Vp%3%#4XR/"
    "75v(3FDm89eM*o2@_'?EYBVwCR_l,2SZBM'YDpc;#^U`3@l?>#xml+#u*mQ80S:v#02-9%8hxi'qo-W$)B=;$++-m8dQ-[-%0fX-9O[X-)G#c*4Vk?>+Ng;.Sr1iCd1qN([nH*Xw'RK)"
    ";a/iC_xIP^]l?T.l'h-#RJ1MbJVGs31G8MBtqU>#BFrr$q@<3$'8P>#k-Jq$cYG>#wa&&=+DlY#9]md+Wjc'&#$wf;%l9^#i%k0,RI6##bc'<&)Yqr$L;@L(ecqB#6cE<%]HM`N_:c99"
    "@oC)*I(7$IcakxB$MB`N(i#%.`Ix+M-@)&.PmThNlDZY#O<90)w9$`4jkf$NI;8GV384'#&6`c#BmtgLtGj$#t^cO-Y^JZ0)Buu>-AP>#_:SfL<[7.MVO?>#:6YY#MK44Mh;n+#o[li9"
    "JkZgL(k)P(k'Uj(2$Yk0>]WF3I)tI3$3Tv-ePJsn`J)p/v4e^5Yo73:BU&w#Aj0Z-bUsX.IU,H3%V/;/k&&t#cn.m*MG4s/GAHVH*MnhC,7V=/$%Jl#s9b$MP@W$#d'ojM?9$c*lk%t-"
    "Tu[fLer#j'eWtK)(&###+Duu#t%Y7*B1?T'AL)j,k]-f;cqgZHac)T/i6(Z-qws*,4tv_+hABa+hnsx-SF9'vUL?>#q^FM'#k-eZFviu5#BQa<fQx68rED8A0.lA#Pk*J-v.$L-BR`X-"
    "-Pj:)qE=m/mqZX-4qOk0hQmi(WrW`:9qc'$ZdSZ00iOm#G[@%#r3bw71ODh)/x6W$,M(v#.a_;$DX1a3=imd+*)?>#%R<D<&&###.P:;$daTg-)b<@0(Ys]7Q(Wd%I9@D*H3+:.66lk0"
    "))'J3SWLD3I)tI3-<Tv-C0Tv-.#[Y81>at.2.)`+x#wZ.Q4YM(`Z[o[Hghm&spSn/.R#R&H`###WVd_#F1xe*nTV[.0R:a4ElL5/SDIpMTO)uLcc@)*iENfLQ]Oo2E5YY#=t9'#-Mc##"
    "Wcd&#0-u9qRXmY#+qgS@,jt&#547W$*s?W%_kEX$O/O1)ZgKZ-vgJP^U.9]FSZx&&$/gbiaP;4#J(-5/QHT>6Qwi02b;Vs/$4J[#,;G>#54LK%E_c^$loZWB3=CW-Lb1F*06@d)s9Z)4"
    "KJjD#TMrB#jBFA#$Y0f)9,B+4hC]hXlMl]#JA1R'u.NF3oF^M/3R5R0?b%?5?H'-.W1(f)7i+vmf&xg)enax-MH:N9Vt%d4VRsX4+P`pHh79K2?mb+1E)Mv-d[BBl2Ltt.<d&ktA(.@@"
    "0LBJLc?&/1H_&@#i(058TQeLCZjb&>4UA2'-xTk$h[R0)mZeX-sc`,)P5]k;n3-O(W^Tv-6YTN(.:%)(@-Q&>PFQ21KVD/Ua7BfCP#0*+kCN.LOWp6&&OE;HBcSd+^6wlLxI3$#W%j>$"
    "')###+Ex>#vUdYoN7$##x;<+NXltA#6^JeGB4^V6Gwd2O^Z4nE$b#d)6klcE:wK'dH1(58?dlaXv_g/3nc@L(aTc2;)17xGg.lA#,=5H&E57m0TC-X]10QD-8>'5;k`KM>QYK>H;aT>H"
    "loPM-3D//.:?Qw7^S+:&=hap%E[>3'Jm_g(0cje+9Cue)GfG>#(opb*v+4I)bZc_-/n.X-N)>>#Z]L=-e%:IS@7rdMNmZY'M)S&#UY+H*h_Ms-*m9e*n>f9JB-K88b>392?c.i*b_#o^"
    "/S?>#aBmY,gpsc;W?Va'l,e`*fYRQ/vuD%$0(XO+vlX/2buVS3L#####-S>#KS/b*2phT.^^A@#F6Eu.DlfLgO]Bb.XW;#v])Z3#g2Ix0g($HN'X65N2M6##:Fk]#>'U%189/W7OiU;$"
    "(,,##@]]&#Z#+iCE&D$^lR=F3w_]9MsfLU`3[4nahPWS<C&L=QE7h>$gt=`oi>kJ-B>pV-kkin<U5F:V945b#lwpk.@jxW7=Y2I.r6[c;$L$B#Qr7/?vGf.?eU<K3^c*,#6CIw#Vf1$#"
    ":88d/$4Og*%=)gLnxAV-,wxj-_6SqpPp<V7LC#hL`C6##'C`b.TurjV't7v&l6*$@r(tI3i=tA[7&+/[N+/?;$/C20$MD>#@O1).sudmMGmRv$,Pl>#/V1v#;Dwo%-qC?%%Lb?Km5-Q/"
    "<L[<$8o&B-M/5R*7<eu1?tr*.EL;Q(]JD&=#qJe$?Z:l0d>vw$PHF:.9EsI3>iJGMV?BsMY+M4fg,7>;xSPwT(Qf@tC;9/:LLpM1hbtM(^3N+`uCpO]n]1bI(Eou,kI529IS#v#5FNP&"
    "h`Rp0:x_v#'mD*eRMlY#0rC$#W@]v7lh?lL6_9,NQ2aBU$TpN#p';pMqqJfLbe*$#-dR)$Z(V$#`9F&#=Ja)#&[%-#?rQ.#)N_V%IKe8@Q[Iw#R/)k'+;>##?Y:@#6H#^=M[[w#'.ZQ8"
    "7S:v#1/L<$4L82'`W2i<^#xh2K<AA,EY,##*ZQ=Abo/U9B]k/<X(@=7:=tT/q*Y/2HT=>-8x9f)&(H_+Ahfi'Gx@@>k@hXQ]aEZ61L9T&_A</6WV6<.rJWJ6=c4&+Pp0v-SQo],w5Q7&"
    "d:gF*0$4w[`@o0#XhBU)2BvR&L6^6&[uXr-QYDp/Ng'v-?BAT&L1#/Ce<)S'J<DH6LZnbinq2u7GBbI3:NYg*Ab'9.(6g:H.9i_,P_i],GeBoe`f#5SbQsr$F;3:M[a)*4/Ur.QGB'kN"
    "na)'5O=ml/8P0f)ip>V/kXIPQ@jZCY,+$3E^E?9/F5YY#)0up8^7fp&g####Ks3g&',:8.gUs`uuR_+i-@-b[1v.U&l1(58RXe>#*8,##.KCV?B1.s$=C<T%4QM,#w&/gLVkFT%1jpo%"
    "<K0E<n2'?-9rOM(6scg)6>8L(i*?,GlR?x69B7g)HLuZ%eT/i)<=6g)'wZ)45hN-*TV)B#9H*C+[pc)M]rU]=fuhB#sQQ$8SW1#.<W7m0DtBtBJ<B-lLkf$MObI$v/MpkLlIF/MF)E?#"
    "C^8P<Z-3?#2cC;$3jhV?Q/KT&9]WP&q76L-uFkm$0fF,M]XY&#Blg<M]Q4I)186L-n4'WML^R#1=J&v5'^Bv-+EsI3KuhH3HAMD3IOFM(ETs0#/SY_+H9&`j/d`mLJ.;5SpA5U(o0AkC"
    "wv](+;*s0CJ]RqC(OF$K^/5##[Cqf#J0U'#>F.%#J,qT%PCvl/@iBTTs[0^+6ax9.Tm1T/5d_s-/omd5Smj`5<1k:8r2(j0-%nK#1s-Yu$Us`u;S8Hg0gw#MAF$l$Ig55&rt)R<n9F9%"
    "s>rX/UxWD#u?lD#(e+H(MHlD#7%x[-3+gm0_9hP>Hrq;eaBFX7@'BR/?QOB20Oe68C=/R/`GEvEG(1-@42Qk0v94M04E&O(dv$BPB.6V3Vk^HdnPOV-i7TP8C`@-d_wFQ#xFwm0jn0N("
    "LUaJ1J:)iu(I.m/nrducScFe-$I9D3vfF/21k`s%8N$0%_wPg1pjxw#.&Es%T>$_u95VT#`Dl>#&1v7$3%-iglMjI#Mn>3#xp(G#g>%@#?%ffLn1$X0;.=`#veJE#_YCqL+-g%#F$8nL"
    "%M>gLj[)Z#*_uc+'U;8.)$o_Yn-&fuPUpA8la`Fi2_mbimS#oL3T-N:e1OA+IKru,6lju5r-)q4_*@g)(Mwm0UH:a#ae_F*stC.3xE6^4@V*w$C]R_#R(1+*Q(7]6L77<.hHK:%BJ^P:"
    "M;>F+JCce)m.4J2>tWU.,eh80(%Ki1`4D;$dHMd2dPh;.bb<w-]SUZ-shmY#&2W4'dDVk'&=-g)JGdG*(fS5'?T97/t7+i(H`^F*VUE(&PkwjkekAM'xIqc;NpxjkvuKd498Uv-*iqP'"
    "LUZ((KK:a#@)s+PGkNp%ST(K(:>`KiU1[K)M/B<7[9GRCk2YO0xXV+5mgShCBRFR0WxA-=XJ,##1q2`#EE'VmK/(&=&;ju5X^00.GAYY#t<X)<)+xP'I#jm$FF$<%;W,)<cZCo*KJXp'"
    "(7'U/e#5B,tXmR(un&60c<*H*tmFX$77@^5uRNX.U8-W.oB:V%I[mC+Y(m>#Zi-51@0/a+&L;A5XjB;.:-bu-[Vo]5QEOY-_SGB,%%ZhLGB?>#Vl24Dl#'%#>6S?##SrB#s`%E37jt-$"
    "(LM8.mDsI3_+xC#0[0@Jbx-`Y9vMbQO,0fa3`(SmT_idu5-7LCE[AuuMW4oLJ[]w#'bft'mA5&#+[E.DjriW%B?@W$%P*n0s'Y>-UBIv$G)C<.,j?S@4O7>%StVV$67RS%B3`v##Jwm0"
    "(KD,):K_#$;<<D#iWB1FoB1r:4?Yb5N#BSR;'TY>w&DuY_G[e$0_M<%6h_V$>Frj2*obA#1qM[$'H@D$v=xs'_Yvo%M5DX(vTuJ]o1IF%*WCxu14.[#KL17#-Yu##UE31#J&###F,HM'"
    "hw&mB:=,&ueGMfLYZ7fMea7DENBZ]+tCF&#N)[ihN4RMLg$C2:`uUk+TxD_&e$s-$Xa=.$$%/F%qFm-$*c;&5Ne1p/KnL50P^A,3J'I21?wdw'<wdw'N@t9)k)/m0B4i-6K^G_&-(n-$"
    "ZAu`4-?rV.iOJ88RWaJ2XPq]5Rpxc3S#>)4p.VA5_oQ>6DFP8/l/fM13ls-$PC-j12APk4iN_w0k88_J>@Lk+:Nk-$n-lk4Zs0F%ej6F%]ifiB_idw'@i*.?*/EkF:JK21kg*:)V]?kO"
    "L;w9)3DO_&@xSkkI:oV7^*U;.BY9R3;,D_&(nj-$4<k-$D<5Ra5X/F%pEs-$$Zk]>`J><-1kBK-=M#<-g7T;-`5T;-06T;-?5T;-A6T;-@Z(*%Zr-F%L/l-$*j^w^uk.F%)sp-$&jp-$"
    "'Pc_8NO3F%Z[r-$fIQF.oheG*;p,g)<Sw],U5u`4B4pV.Q]<#-u*K88`>p;-ef#<-v-R^OA*@q7Jn1p/ZO^F%1V,RsMJ-F%>jcG*et:_Jt.X&H5u#<-g-n6%?KjDNk_;u7eNYF%WLX>-"
    "4C%^Ou`5oLgno4QYLI*NSJlDNlJT,MahDu7jf(G%_qt9)'kj-$-4U_&VY#WoS$U,M]7QA%qW+?I`Bj.Ne9$<-C6s'%->wEn(.=:2)H'd3(MV8RSOQ-N,9$<-`rA@RomKDMjnS/NC>T,M"
    "<jK,N6IuPSb2gw'O[vV%rqm;-bWCx$EC%<-R>5lO^TiH-^9x/MtrPoLgcm)Mqa;xL_IBvL$M%2N:EK887g%RNL`D_&[hPG%-a>&5Okt-$?x2F%n6$j:;:B.$&CF_&XH_]PTpo;-CGY)%"
    "fjm2`M1$-NF.94MA:xiLtkiU8/9`D+l9_M:*[7p&b:G,MG/M1NUuj;9l-.<]J4$##RV`S%XZPrZ8IZ`EdRqf:HXcrZ'L0g(5/0)EN`dfUHAvM'5RXMK_`1T%latfUu=%H2qJ+m]Z6QGr"
    "0/@a3I@nYlRnCgChQX;m.Y+<6idMsZoF@6&%UGT@uJo5]2@mm8LZxZ#h2mmJxEn)jDExg:XLbaW@22<mn<W6/Cs.nA4JJNp3M?$GeMG[#vO,bNZ#p$#+h1U@I6C$cVc`$>EOsH`:@HU7"
    "N#:URcg3bsmC<C=bs(Cbgbtt?fi86&Lh5?*]qe1*w3I`*$X_$+vc8m*L839+.VY3+twaA+0U7P+tQ`W+TR^^+S%#j+94w<,Hr0V,AKmG,9S&Q,Zjr[,Ovep,181$-_:j1-Hwl;-G>JE-"
    "5r)p-m?T^-1Ruj-u+:v-k]e1._jI9.dILC.#g^j.Y11_.%gg//o>Mw.IP@5/Mcb>/`jEI//PHS/&UW]/jJ[10Octu/Iw?)0qCH40u,[D0@?[h0Cc0V0`],c0=OAm0XS%x0H+?01E_?@1"
    ":WEe1C%qR1])^#2XWCk18i7&2[5@12/RH<2kZ+J2944#3koWi2?9at2]72)34Se53]7;D3jm=N3GWZw3lmFj3'(F:4CUV-4Q^f64DJYZ4CIMH4K^oQ4LoCw4fT=f4S,Yn4XUR=5&;w-5"
    "rJJU5)%2D5E,lN5+)<]5TN)p5MM&#6Z-)-6e)&66lLY?61^OJ6PInp6`/h_6x*9j6pqLw65QO+78o6P7<]kF7HbWm74E%a7i#Z488.5$8XA+/8fL:88#EbB8J$Bj8BR([8Rs[e8fq-p8"
    "9i)&9qL+393+WA9m#2h9#v#[921N*:30Bn9?uVx9aauG:IHp3:c9[9:(Cl?:k/`d:$::S:org_:Iuqw:9f2&;,J`1;H)b>;UrKG;^#2L;Ks-X;`VYg;gQWm;Q:@8N;Ce3#QlRfLm?Q.#"
    "4Nc##rAP##OvK'#KS3rL>DmB&u,+,M.Y5;-bNrdbR8RtLYa`m$PoQS%EU^R*MM:;$AVm_&cZZ`*Ecm_&XO/2'^Un_&+QC_&5(o_&,TC_&BOo_&)@=G2/wo_&ZINP&`]q_&[LNP&kxh_&"
    "$Vdl/vCi_&n2<A+Hhj_&?+5R*,bk_&4mC_&T/m_&`FRS%^Rn_&]%;;$we/ed)SFgL8q2?#7B`T.6@@[#?B`T./fLZ#OB`T.XVd_#WB`T.D0bx#-rG<-+fG<-GrG<-,fG<-f@`T.8,4*$"
    "*A`T.Dn<x#1tG<-/fG<-KB`T.+wL($bB`T.h+j@$[@`T./*iC$6sG<-F_:EN=Fa$#kIk(N99aP(AHuJ(MhW?-;3IL29Beh2EqRxIxarTC%hitBhxtLFu*cM:C10@-b-%F-$WA>BdH`9C"
    "'ditBBjerL:vAeGl#oUCTe0@-T1D5B`EQ@-8q5)F2AhoD:H*vH3i7FH>P%l=*T1O=2oLnB1uwiC=n5)F5U%F-&p>hF)s8oDw?vsBQZ=lEnrCkLeE+)>W^:(6djeM1V#f%'#;1eG38vLF"
    "+YD'PSO,-MXmSE-%O;9CuicdG/iCrCgHaE=kW_J;f<ZWB1FuC&E3FGH,C%12p7-02mH7UCa_6B$.E$6T^JvDPdn0T&wj9'#ZqB'#vP?(#N/B;-Urls-aJ#lLfYw+#cst.#L*]-#P)1/#"
    "'6[qL;KG&#wX@U.[5C/#]RFv-^S3rL]1CkLxf>oLN(8qL.fipL(FD/#<MM=-?5)=-<>DX-J,Ok4T-x?0J)@X(V3Mk+%$Kq;6tF_&6Cn-$^Q+F7JERfCS0K5BETfu-1G<RMXoUH-BZ5<-"
    "w%kB-VqX?-Wr@u-.#krLUit(R/g9kND3CkLlf=rLN4oiLAf=rL`3oiL.P#<-e&@A-5u&4MGJr'#$Z5<-9bDE-KOl0.MJMmLrRxqLv$](#V8q,+8[=)#&=M,#nUG+#<^(@-B=]$Q-%s=-"
    "`eQT-StL1..T3rLqC@;T@bIr7dg@5B0'n]G+tB2C3l4)FAdw]G$#w9)o3=-m8&/kXdYH?-Q&@A-wp-A-Y+KZ.<tI-#I##xRnR,.#?NYS.#-3)#mSoo7c`X/2pU<e?+ZCk=m]2R3/6KQ]"
    ")qOUf(x5x7jP'99MuuD%mb60#w;-e%7('i#d]O.#&25##C8B:[52np712A5#KA*1#?X#3#=N<1#=lj1#kv%5#w9>##^?G##:F^2#`5s<#:cjL#]_^:#_4m3#o@/=#eG8=#t8J5#`+78#"
    "4uI-#m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#"
    "%/e0#TmD<#;Uu>#4`1?#8lC?#,px=#0%V?#1/PY#3.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#7QJ=#;0=A#a9OA#eEbA#iQtA#m^0B#qjBB#tm9'#&)_B#%3qB#)?-C#-K?C#1WQC#"
    "5ddC#:x,;#>rvC#=&3D#A2ED#E>WD#IJjD#MV&E#Qc8E#lIf5#ppJE#Y%^E#^1pE#b=,F#jdf=#Z0?;#VK>F#jUPF#nbcF#rnuF#v$2G#$1DG#(=VG#,IiG#0U%H#4b7H#8nIH#@@:R#"
    ">*fH#A-]-#GBZ;#1=+I#HH=I#e;':#6OB:#3[T:#3C0:#nL4.#Z_,3#+[XI#RgkI#Vs'J#Z):J#GVx5#8pF6#Joq7#8j=6#l5n0#nSE1#@%.8#LPC7#T6LJ##rs1#VqG3#WrM<#[@T2#"
    "hji4#GLg2#t4(R#E9P>#m.jD%KGRR2q^);&6JrEH381SDkoH&Fu@ViFv.1VC(3f:C$]eFHpg`9C]&77*25emMDbCp7)`I'Iu)IQMEkLp7-2vLF]Ta2B5$89&Y_nFHR/0I6:fsjNEn?m8"
    "o)<e?4D*nMM^kpB0,DEHeGcn<m?+0N5C4nMAqZ29(/Kv$4W5W-@CQ@9<r/kNEwd29?xg;%6a>W-M;Ve?3R&+%up9-OgkBkNQasj9&5H1Fs86h--^U%'MXsTBdAYGD'7d;%ALRe-vmJF%"
    "nBoNM8c.dD+=d;%+s4c-vmJF%vs0hNG:NJ:Cc#4EZGmL>FXGLOTpoG;'+OGH*mWRMM)PH;JQAp&sa=KM#)+aE(xg>$tpia-pH3I$tjbkMw$1oM[;@IOF^BBF6FHv$O/R]-xm&+%XZHHF"
    "brlKFgtkkM4J0^F,7Hv$=j[m-#q&+%m)VOD%Lv<B:>-lE&cS>'LAvW-q,k0GA:9SMS`HB=ECDp&M8mW-q,k0GBCToMwxuOMAg>vG5'&X%O9`9CrdM=B.Hpb%$9ClMc?LSMRP6#>^sKG$"
    "F/7m/]@poD`?b'%DW5W-^dDeZFXpoMQVd>>(0Nv$J8.Q/Y4^oDtFoVH/q[5'<$mO-RAvW-nkiAZq()WH=W=U&e%VeG%^4rC#8A^&P)H<-2)H<-R/dW-x<X0cIk,TMYFfs?+ZKW-:#dC&"
    ":3Ux7eu@p&ToeQ/x':oDM.wt7xT(:8>#8kEV+Gv>*G%'I6c,pM%;DPMr5V8I76Jt%X^IUC-mgoDI(dPNW%&W?kO$sI'Ip9;XdZL27II29*HLc$LXvlEr7999twp?H*p_aH.HXnD3qMfL"
    "JF-##$)>>#K3w>P;@e3#cOj-$&t9'#vdlb#O'>uut8X$Mws4U%,q+/1fdfpC.w4/1#KTV[>b*N$X(+N$=ut`-c`pCOG)=s<H-^V[MJ->-C6n'&8ap`?D?839m7T)096AT&j]+Z2TZaJ2"
    "bg/eaR4wiCes`@BNGZa<<$2R3W9Vk+/Tl'&[.n'&dSng+:pq`?pgfi0T<pV[1eXSC_#vW[IVoiLaF7p.lYJk4jAXC?>]f88?`f88/tqY?cWUT1&ul<.&1>?-K%mMBx'v3+]3S?#HVxiL"
    "`EBB-0_3uL-;5Z2)[S4=Gs2f+3QUD?=5Ok+1KtC?Nx%a+UG27E6K)N$Jk2Q8XO`Y#_0>?83C(gLq.h88mTg5#EmM^[I@'_[%TX0G)woM03?c/1-5)E/1u99_b`qpL,p#-.KU=5Ma1tv8"
    "e<`;%LBi'#+WIg%xLJE-xZa5Bc>l[MV.X]+c,Oq22`$V[Q<j2/uI,<.53AI661/t[#aLZ[L$ZQ1670kL1-Y>-krcm:'$VC?JR#<.$]#H&&HSD=2hV8&[sEN1*op?-`xGqVwUhHQ?'&RE"
    "K-WQ89*A%B,h_gLTrtW[Oeh*>5o.?#@B#W[1f&WI9dC01d'@5;1Y`X-Nh5R*GIGR*9@/t[s19E>EE2n&BN1X1KNlj0q@HP8=`x88f%9I$)w/I$eY3)*E+?']Ujmx[sqdsA?fx881(8/M"
    "E@BN0v0,O4FP%a+(^s63$wn5BBMXV.$xDQ8%kUL.()Jm#wHsW-/54K<tDq5#HAGp8tI:Z[ko?Q#ttdsAQP9a'%B/[$k4G,#Erf88BTtKG*aV3'/f1$#=;B%BhD8j02g7b+'#vW[YSdT/"
    ":BtW[h26Q#7Q0N(1=(,M62v01c.5F%7R/t[*lu8.pVMj$;vGg)5UL,M:JD11g:5F%;_/t[.lu8.x%f,&BV%*+3BYj0J[abJ[0,_JZ-,_J]3,_J?M@&,VC9F%_9,_JA`w],VC9F%a?,_J"
    "CrW>-VC9F%cE,_JE.9v-VC9F%eK,_JQ'vW.]&QY[c7vW[?F(tq*ZIE-pr@['=&Im#3C/F%W7=Q0H)rG/PFbm0P)lN1Gqq?-A]1s7-9?<.Umu>-Mg18UPaFY--<R)0Uc;N0hA6d/jPnc)"
    "i/+jLpr,t73-@P+hxq?-S=Af.+6/S[IdMj(wgfi0=Y#.MtS%)*[;w0#ENh>.8AnB#O5`P-Lmm*MmfC12x@HP8EktW[m89I*lrJ#2lYnV[M0'qL>-)41BG$)*bf-@->u68.==*p.t2Gx9"
    "h:_5B+ktpLK&-Q#jh^#2NYWI)'BOK2lS*)**SEjLLYDm0J])q0em-=.*$GGOB.)g2bhb>-nQ=v-%=ufL]hev6ONjj<7C4g7-#fm#,7%a+W+$$$qi-<8uG@g%>^3uLVPq%/HsD^[fL>9'"
    ",JJE-DCU694gd_#N3nB.&0*9B6bQ][:+xg+5.JpAUC3p.hBB5Bg]IW[b7)Q9PW(;.n.fP9[Jn*%cg-R<%2;nN>dc<.Or)pA<$qV[1uipL8mToL#gipLje'KMU#`5/^>1a+Jx3/OB]mS1"
    "8SkW[$HX>-uWa5/=JHp.=m@P1LM-7*xN*9.%.fu-V''nL-t]5BgV#d3XO>na43l<UQ/mx=[9S>6[v[.M'EYcMGnow7:R5s.SHdM:x8Ks-0oMs7;?69KeMKs78DpTVX[_846ljS8-75?-"
    "`^+pA:cT18MW1E4lC+j1'G2q14PXD-C2Dt[4$lk0lQ=fN5PneOu&uCPIbo(P%w-51oIhWRbE751fGWEOP&QY[mP(HO6TUs.Ifim1Q%DwKua&<A#FmA%Aw`29[s#?6x&W&5Zk8?6c94<$"
    "tV#HXt6N@0G-w;JAtGm#s-kpLr>T>.c^b>-$ldm%E]72NT:R51qZ?t72+3v6>o/Z-8tsr$B@etqM*)tq5`t8T^2$6#NYWI)AcjA1lj4g7:R#t7WCD>#(p.^1.bg88]@QkLeZWmL(BB49"
    "`f-@-#[a5B@,@U2Lm@g%'p1d8rAhD9W=)##`S,<-jS,<-(r(9.J9?Q#wNC2:CHNX()Q3O+Y+vlL30BnL%+p+Mo5KnLH:)=-)2#HM$B^nLB53>57g<,<$]bA#tsVG<U)OX(tnIb%*K8)="
    "6:P&#$Lx>-vS,<-%V,<-BIrIM+mGoL6:)=-6u@Y-]+(F.$+Jb%%0Pb%7&4O+15L>?CHNX(9,4O+VoYlLC;)pL%+p+M)A2pLH:)=-92#HM6Lv81X4LmC-]i5M7@$T4V*bSA^Gu`43(JpA"
    "lS*)*E8[V[c1M9.?G(W3[9^PBeQHX14hx'&sP+I-5kx'&Y^uMCh?xeNW7]6MS&c?-F5g,MZL4RM]nHlLf?8:1gvr[0=<l$'^,RGE_'C[0=-#(&`9Md2C:bW[D@f2$lwu`[AqKO1otq+;"
    "LmLV=/JM>#x+?h%9fSiBn$@8.>1R8.K3P49YNIh,V2S4;2ZR%@&g6c+aq-vA4rkL>7,AV%e.Jm#H1Pd&]'89]gj/I$3?0I$W<:>]HBw29#;l)'dwrs[vV_v[x7@4=J3:gN6%]v#nlp/M"
    "eYrN$,7Dt-DLwoL]VvQ#^W,N$hYV##$gOFD4'<4#";

static const char* GetDefaultCompressedFontDataTTFBase85()
{
    return proggy_clean_ttf_compressed_data_base85;
}
