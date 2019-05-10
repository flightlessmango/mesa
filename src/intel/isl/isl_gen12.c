/*
 * Copyright (c) 2018 Intel Corporation
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include "isl_gen9.h"
#include "isl_gen12.h"
#include "isl_priv.h"

void
isl_gen12_choose_image_alignment_el(const struct isl_device *dev,
                                    const struct isl_surf_init_info *restrict info,
                                    enum isl_tiling tiling,
                                    enum isl_dim_layout dim_layout,
                                    enum isl_msaa_layout msaa_layout,
                                    struct isl_extent3d *image_align_el)
{
   /* Handled by isl_choose_image_alignment_el */
   assert(info->format != ISL_FORMAT_HIZ);

   const struct isl_format_layout *fmtl = isl_format_get_layout(info->format);
   if (fmtl->txc == ISL_TXC_CCS) {
      /* Compressed main surfaces have an alignment of 16px x 4 rows. A CCS
       * element is 256b x 4 rows.
       *
       * This corresponds to an alignment of:
       *  * < 1 CCS el for 8bpp
       *  * 1 CCS el for 16bpp
       *  * 2 CCS el for 32bpp
       *  * 4 CCS el for 64bpp
       *  * 8 CCS el for 128bpp
       *
       * The 8bpp case is safe as long as the horizontal alignment doesn't
       * actually come into play. We disallow miplevels greater than 2 in
       * isl_surf_get_ccs_surf() to avoid it.
       */
      assert(fmtl->bh == 4);
      *image_align_el = isl_extent3d(MAX(16 / fmtl->bw, 1), 1, 1);
      return;
   }

   /* Arctic sound has new alignment requirements:
    */
   if (dev->info->gen > 12 || dev->info->is_arctic_sound) {
      if (info->samples > 1) {
         /* Vertical and horizontal alignment fields in the RENDER_SURFACE_STATE
          * are ignored for Tile64.  In the case of TileS the alignment
          * requirements are fixed and are provided for by the tables below for 2D
          * and CUBE surface.
          *
          *       bPP HALIGN VALIGN
          *       128  64    64
          *        64 128    64
          *        32 128    128
          *        16 256    128
          *         8 256    256
          *
          * For MSFMT_MSS type multi-sampled Tile64 surfaces, the alignments given
          * above must be divided by the appropriate value from the table below. 
          *
          *       Samples Hdiv Vdiv
          *        2       2    1 
          *        4       2    2 
          *        8       4    2 
          *       16       4    4 
          */
         /* Samples | ffs(s) | ffs(s) & ~1 | s / (ffs(s) & ~1)
          *       2     2          2                1
          *       4     3          2                2
          *       8     4          4                2
          *      16     5          4                4
          */
         const uint32_t hdiv = ffs(info->samples) & ~1;
         const uint32_t vdiv = info->samples / hdiv;

         /* base_valign = logical_el.h
          * base_halign = logical_el.w
          */
         const struct isl_format_layout *fmtl = isl_format_get_layout(info->format);
         const uint32_t bs = fmtl-> bpb / 8;
         const uint32_t base_halign = 256 * MAX(ffs(bs) & ~1, 1) / bs;
         const uint32_t base_valign = 256 / MAX(ffs(bs) & ~1, 1);
         if (bs ==  2) assert(base_halign == 256 && base_valign == 128);
         if (bs ==  4) assert(base_halign == 128 && base_valign == 128);
         if (bs ==  8) assert(base_halign == 128 && base_valign ==  64);
         if (bs == 16) assert(base_halign ==  64 && base_valign ==  64);
         const uint32_t halign_el = base_halign / hdiv;
         const uint32_t valign_el = base_valign / vdiv;

         *image_align_el = isl_extent3d(halign_el, valign_el, 1);
         return;
      } else {
         /* - 16b Depth Surfaces Must Be HALIGN=16Bytes (8texels)
          * - 32b Depth Surfaces Must Be HALIGN=32Bytes (8texels)
          * - Stencil Surfaces (8b) Must be HALIGN=16Bytes (16texels)
          * - Losslessly Compressed Surfaces Must be HALIGN=128 for all supported Bpp
          * - Linear Surfaces for 8,16,32, 64 and 128bpp surfaces must be HALIGN=128
          *   (including 1D which is always Linear)
          * - 24bpp, 48bpp and 96bpp surfaces must use HALIGN=16
          *
          *
          * These valign requirements have stayed the same:
          *
          *    ProgrammingNotes
          *    This field is intended to be set to VALIGN_4 if the
          *    surface was rendered as a depth buffer, for a multisampled (4x) render
          *    target, or for a multisampled (8x) render target, since these surfaces
          *    support only alignment of 4. Use of VALIGN_4 for other surfaces is
          *    supported, but increases memory usage.
          *
          *    ProgrammingNotes
          *    This field is intended to be set to VALIGN_8 only if
          *    the surface was rendered as a stencil buffer, since stencil buffer
          *    surfaces support only alignment of 8. If set to VALIGN_8, Surface
          *    Format must be R8_UINT.
          */
         const struct isl_format_layout *fmtl = isl_format_get_layout(info->format);
         const uint32_t bs = fmtl-> bpb / 8;
         if (isl_surf_usage_is_depth(info->usage)) {
            *image_align_el =
               info->format != ISL_FORMAT_R16_UNORM ?
               isl_extent3d(8, 4, 1) :
               isl_extent3d(8, 8, 1);
         } else if (isl_surf_usage_is_stencil(info->usage) || info->format == ISL_FORMAT_R8_UINT) {
            *image_align_el = isl_extent3d(16, 8, 1);
         } else if (isl_is_pow2(bs)) {
            *image_align_el = isl_extent3d(128 / bs, 4, 1);
         } else {
            *image_align_el = isl_extent3d(16, 4, 1);
         }
         return;
      }
   }

   if (isl_surf_usage_is_depth(info->usage)) {
      /* The alignment parameters for depth buffers are summarized in the
       * following table:
       *
       *     Surface Format  |    MSAA     | Align Width | Align Height
       *    -----------------+-------------+-------------+--------------
       *       D16_UNORM     | 1x, 4x, 16x |      8      |      8
       *     ----------------+-------------+-------------+--------------
       *       D16_UNORM     |   2x, 8x    |     16      |      4
       *     ----------------+-------------+-------------+--------------
       *         other       |     any     |      8      |      4
       *    -----------------+-------------+-------------+--------------
       */
      assert(isl_is_pow2(info->samples));
      *image_align_el =
         info->format != ISL_FORMAT_R16_UNORM ?
         isl_extent3d(8, 4, 1) :
         (info->samples == 2 || info->samples == 8 ?
          isl_extent3d(16, 4, 1) : isl_extent3d(8, 8, 1));
   } else if (isl_surf_usage_is_stencil(info->usage)) {
      *image_align_el = isl_extent3d(16, 8, 1);
   } else {
      isl_gen9_choose_image_alignment_el(dev, info, tiling, dim_layout,
                                         msaa_layout, image_align_el);
   }
}
