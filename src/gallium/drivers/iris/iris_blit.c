/*
 * Copyright © 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/ralloc.h"
#include "intel/blorp/blorp.h"
#include "iris_context.h"
#include "iris_resource.h"
#include "iris_screen.h"

/**
 * Helper function for handling mirror image blits.
 *
 * If coord0 > coord1, swap them and return "true" (mirrored).
 */
static bool
apply_mirror(float *coord0, float *coord1)
{
   if (*coord0 > *coord1) {
      float tmp = *coord0;
      *coord0 = *coord1;
      *coord1 = tmp;
      return true;
   }
   return false;
}

/**
 * Compute the number of pixels to clip for each side of a rect
 *
 * \param x0 The rect's left coordinate
 * \param y0 The rect's bottom coordinate
 * \param x1 The rect's right coordinate
 * \param y1 The rect's top coordinate
 * \param min_x The clipping region's left coordinate
 * \param min_y The clipping region's bottom coordinate
 * \param max_x The clipping region's right coordinate
 * \param max_y The clipping region's top coordinate
 * \param clipped_x0 The number of pixels to clip from the left side
 * \param clipped_y0 The number of pixels to clip from the bottom side
 * \param clipped_x1 The number of pixels to clip from the right side
 * \param clipped_y1 The number of pixels to clip from the top side
 *
 * \return false if we clip everything away, true otherwise
 */
static inline bool
compute_pixels_clipped(float x0, float y0, float x1, float y1,
                       float min_x, float min_y, float max_x, float max_y,
                       float *clipped_x0, float *clipped_y0,
                       float *clipped_x1, float *clipped_y1)
{
   /* If we are going to clip everything away, stop. */
   if (!(min_x <= max_x &&
         min_y <= max_y &&
         x0 <= max_x &&
         y0 <= max_y &&
         min_x <= x1 &&
         min_y <= y1 &&
         x0 <= x1 &&
         y0 <= y1)) {
      return false;
   }

   if (x0 < min_x)
      *clipped_x0 = min_x - x0;
   else
      *clipped_x0 = 0;
   if (max_x < x1)
      *clipped_x1 = x1 - max_x;
   else
      *clipped_x1 = 0;

   if (y0 < min_y)
      *clipped_y0 = min_y - y0;
   else
      *clipped_y0 = 0;
   if (max_y < y1)
      *clipped_y1 = y1 - max_y;
   else
      *clipped_y1 = 0;

   return true;
}

/**
 * Clips a coordinate (left, right, top or bottom) for the src or dst rect
 * (whichever requires the largest clip) and adjusts the coordinate
 * for the other rect accordingly.
 *
 * \param mirror true if mirroring is required
 * \param src the source rect coordinate (for example src_x0)
 * \param dst0 the dst rect coordinate (for example dst_x0)
 * \param dst1 the opposite dst rect coordinate (for example dst_x1)
 * \param clipped_dst0 number of pixels to clip from the dst coordinate
 * \param clipped_dst1 number of pixels to clip from the opposite dst coordinate
 * \param scale the src vs dst scale involved for that coordinate
 * \param is_left_or_bottom true if we are clipping the left or bottom sides
 *        of the rect.
 */
static void
clip_coordinates(bool mirror,
                 float *src, float *dst0, float *dst1,
                 float clipped_dst0,
                 float clipped_dst1,
                 float scale,
                 bool is_left_or_bottom)
{
   /* When clipping we need to add or subtract pixels from the original
    * coordinates depending on whether we are acting on the left/bottom
    * or right/top sides of the rect respectively. We assume we have to
    * add them in the code below, and multiply by -1 when we should
    * subtract.
    */
   int mult = is_left_or_bottom ? 1 : -1;

   if (!mirror) {
      *dst0 += clipped_dst0 * mult;
      *src += clipped_dst0 * scale * mult;
   } else {
      *dst1 -= clipped_dst1 * mult;
      *src += clipped_dst1 * scale * mult;
   }
}

/**
 * Apply a scissor rectangle to blit coordinates.
 *
 * Returns true if the blit was entirely scissored away.
 */
static bool
apply_blit_scissor(const struct pipe_scissor_state *scissor,
                   float *src_x0, float *src_y0,
                   float *src_x1, float *src_y1,
                   float *dst_x0, float *dst_y0,
                   float *dst_x1, float *dst_y1,
                   bool mirror_x, bool mirror_y)
{
   float clip_dst_x0, clip_dst_x1, clip_dst_y0, clip_dst_y1;

   /* Compute number of pixels to scissor away. */
   if (!compute_pixels_clipped(*dst_x0, *dst_y0, *dst_x1, *dst_y1,
                               scissor->minx, scissor->miny,
                               scissor->maxx, scissor->maxy,
                               &clip_dst_x0, &clip_dst_y0,
                               &clip_dst_x1, &clip_dst_y1))
      return true;

   // XXX: comments assume source clipping, which we don't do

   /* When clipping any of the two rects we need to adjust the coordinates
    * in the other rect considering the scaling factor involved.  To obtain
    * the best precision we want to make sure that we only clip once per
    * side to avoid accumulating errors due to the scaling adjustment.
    *
    * For example, if src_x0 and dst_x0 need both to be clipped we want to
    * avoid the situation where we clip src_x0 first, then adjust dst_x0
    * accordingly but then we realize that the resulting dst_x0 still needs
    * to be clipped, so we clip dst_x0 and adjust src_x0 again.  Because we are
    * applying scaling factors to adjust the coordinates in each clipping
    * pass we lose some precision and that can affect the results of the
    * blorp blit operation slightly.  What we want to do here is detect the
    * rect that we should clip first for each side so that when we adjust
    * the other rect we ensure the resulting coordinate does not need to be
    * clipped again.
    *
    * The code below implements this by comparing the number of pixels that
    * we need to clip for each side of both rects considering the scales
    * involved.  For example, clip_src_x0 represents the number of pixels
    * to be clipped for the src rect's left side, so if clip_src_x0 = 5,
    * clip_dst_x0 = 4 and scale_x = 2 it means that we are clipping more
    * from the dst rect so we should clip dst_x0 only and adjust src_x0.
    * This is because clipping 4 pixels in the dst is equivalent to
    * clipping 4 * 2 = 8 > 5 in the src.
    */

   if (*src_x0 == *src_x1 || *src_y0 == *src_y1
       || *dst_x0 == *dst_x1 || *dst_y0 == *dst_y1)
      return true;

   float scale_x = (float) (*src_x1 - *src_x0) / (*dst_x1 - *dst_x0);
   float scale_y = (float) (*src_y1 - *src_y0) / (*dst_y1 - *dst_y0);

   /* Clip left side */
   clip_coordinates(mirror_x, src_x0, dst_x0, dst_x1,
                    clip_dst_x0, clip_dst_x1, scale_x, true);

   /* Clip right side */
   clip_coordinates(mirror_x, src_x1, dst_x1, dst_x0,
                    clip_dst_x1, clip_dst_x0, scale_x, false);

   /* Clip bottom side */
   clip_coordinates(mirror_y, src_y0, dst_y0, dst_y1,
                    clip_dst_y0, clip_dst_y1, scale_y, true);

   /* Clip top side */
   clip_coordinates(mirror_y, src_y1, dst_y1, dst_y0,
                    clip_dst_y1, clip_dst_y0, scale_y, false);

   /* Check for invalid bounds
    * Can't blit for 0-dimensions
    */
   return *src_x0 == *src_x1 || *src_y0 == *src_y1
      || *dst_x0 == *dst_x1 || *dst_y0 == *dst_y1;
}

void
iris_blorp_surf_for_resource(struct iris_vtable *vtbl,
                             struct blorp_surf *surf,
                             struct pipe_resource *p_res,
                             enum isl_aux_usage aux_usage,
                             unsigned level,
                             bool is_render_target)
{
   struct iris_resource *res = (void *) p_res;

   if (aux_usage == ISL_AUX_USAGE_HIZ &&
       !iris_resource_level_has_hiz(res, level))
      aux_usage = ISL_AUX_USAGE_NONE;

   *surf = (struct blorp_surf) {
      .surf = &res->surf,
      .addr = (struct blorp_address) {
         .buffer = res->bo,
         .offset = 0, // XXX: ???
         .reloc_flags = is_render_target ? EXEC_OBJECT_WRITE : 0,
         .mocs = vtbl->mocs(res->bo),
      },
      .aux_usage = aux_usage,
   };

   if (aux_usage != ISL_AUX_USAGE_NONE) {
      surf->aux_surf = &res->aux.surf;
      surf->aux_addr = (struct blorp_address) {
         .buffer = res->aux.bo,
         .offset = res->aux.offset,
         .reloc_flags = is_render_target ? EXEC_OBJECT_WRITE : 0,
         .mocs = vtbl->mocs(res->bo),
      };
      surf->clear_color =
         iris_resource_get_clear_color(res, NULL, NULL);
      surf->clear_color_addr = (struct blorp_address) {
         .buffer = res->aux.clear_color_bo,
         .offset = res->aux.clear_color_offset,
         .reloc_flags = 0,
         .mocs = vtbl->mocs(res->aux.clear_color_bo),
      };
   }

   // XXX: ASTC
}

/**
 * The pipe->blit() driver hook.
 *
 * This performs a blit between two surfaces, which copies data but may
 * also perform format conversion, scaling, flipping, and so on.
 */
static void
iris_blit(struct pipe_context *ctx, const struct pipe_blit_info *info)
{
   struct iris_context *ice = (void *) ctx;
   struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   const struct gen_device_info *devinfo = &screen->devinfo;
   struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
   enum blorp_batch_flags blorp_flags = 0;
   struct iris_resource *src_res = (void *) info->src.resource;
   struct iris_resource *dst_res = (void *) info->dst.resource;

   /* We don't support color masking. */
   assert((info->mask & PIPE_MASK_RGBA) == PIPE_MASK_RGBA ||
          (info->mask & PIPE_MASK_RGBA) == 0);

   if (info->render_condition_enable) {
      if (ice->state.predicate == IRIS_PREDICATE_STATE_DONT_RENDER)
         return;

      if (ice->state.predicate == IRIS_PREDICATE_STATE_USE_BIT)
         blorp_flags |= BLORP_BATCH_PREDICATE_ENABLE;
   }

   struct iris_format_info src_fmt =
      iris_format_for_usage(devinfo, info->src.format,
                            ISL_SURF_USAGE_TEXTURE_BIT);
   enum isl_aux_usage src_aux_usage =
      iris_resource_texture_aux_usage(ice, src_res, src_fmt.fmt, 0);

   if (src_aux_usage == ISL_AUX_USAGE_HIZ)
      src_aux_usage = ISL_AUX_USAGE_NONE;

   bool src_clear_supported = src_aux_usage != ISL_AUX_USAGE_NONE &&
                              src_res->surf.format == src_fmt.fmt;

   iris_resource_prepare_access(ice, batch, src_res, info->src.level, 1,
                                info->src.box.z, info->src.box.depth,
                                src_aux_usage, src_clear_supported);

   struct iris_format_info dst_fmt =
      iris_format_for_usage(devinfo, info->dst.format,
                            ISL_SURF_USAGE_RENDER_TARGET_BIT);
   enum isl_aux_usage dst_aux_usage =
      iris_resource_render_aux_usage(ice, dst_res, dst_fmt.fmt, false, false);
   bool dst_clear_supported = dst_aux_usage != ISL_AUX_USAGE_NONE;

   struct blorp_surf src_surf, dst_surf;
   iris_blorp_surf_for_resource(&ice->vtbl, &src_surf, info->src.resource,
                                src_aux_usage, info->src.level, false);
   iris_blorp_surf_for_resource(&ice->vtbl, &dst_surf, info->dst.resource,
                                dst_aux_usage, info->dst.level, true);

   iris_resource_prepare_access(ice, batch, dst_res, info->dst.level, 1,
                                info->dst.box.z, info->dst.box.depth,
                                dst_aux_usage, dst_clear_supported);

   float src_x0 = info->src.box.x;
   float src_x1 = info->src.box.x + info->src.box.width;
   float src_y0 = info->src.box.y;
   float src_y1 = info->src.box.y + info->src.box.height;
   float dst_x0 = info->dst.box.x;
   float dst_x1 = info->dst.box.x + info->dst.box.width;
   float dst_y0 = info->dst.box.y;
   float dst_y1 = info->dst.box.y + info->dst.box.height;
   bool mirror_x = apply_mirror(&src_x0, &src_x1);
   bool mirror_y = apply_mirror(&src_y0, &src_y1);
   enum blorp_filter filter;

   if (info->scissor_enable) {
      bool noop = apply_blit_scissor(&info->scissor,
                                     &src_x0, &src_y0, &src_x1, &src_y1,
                                     &dst_x0, &dst_y0, &dst_x1, &dst_y1,
                                     mirror_x, mirror_y);
      if (noop)
         return;
   }

   if (abs(info->dst.box.width) == abs(info->src.box.width) &&
       abs(info->dst.box.height) == abs(info->src.box.height)) {
      if (src_surf.surf->samples > 1 && dst_surf.surf->samples <= 1) {
         /* The OpenGL ES 3.2 specification, section 16.2.1, says:
          *
          *    "If the read framebuffer is multisampled (its effective
          *     value of SAMPLE_BUFFERS is one) and the draw framebuffer
          *     is not (its value of SAMPLE_BUFFERS is zero), the samples
          *     corresponding to each pixel location in the source are
          *     converted to a single sample before being written to the
          *     destination.  The filter parameter is ignored.  If the
          *     source formats are integer types or stencil values, a
          *     single sample’s value is selected for each pixel.  If the
          *     source formats are floating-point or normalized types,
          *     the sample values for each pixel are resolved in an
          *     implementation-dependent manner.  If the source formats
          *     are depth values, sample values are resolved in an
          *     implementation-dependent manner where the result will be
          *     between the minimum and maximum depth values in the pixel."
          *
          * When selecting a single sample, we always choose sample 0.
          */
         if (util_format_is_depth_or_stencil(info->src.format) ||
             util_format_is_pure_integer(info->src.format)) {
            filter = BLORP_FILTER_SAMPLE_0;
         } else {
            filter = BLORP_FILTER_AVERAGE;
         }
      } else {
         /* The OpenGL 4.6 specification, section 18.3.1, says:
          *
          *    "If the source and destination dimensions are identical,
          *     no filtering is applied."
          *
          * Using BLORP_FILTER_NONE will also handle the upsample case by
          * replicating the one value in the source to all values in the
          * destination.
          */
         filter = BLORP_FILTER_NONE;
      }
   } else if (info->filter == PIPE_TEX_FILTER_LINEAR) {
      filter = BLORP_FILTER_BILINEAR;
   } else {
      filter = BLORP_FILTER_NEAREST;
   }

   struct blorp_batch blorp_batch;
   blorp_batch_init(&ice->blorp, &blorp_batch, batch, blorp_flags);

   unsigned main_mask;
   if (util_format_is_depth_or_stencil(info->dst.format))
      main_mask = PIPE_MASK_Z;
   else
      main_mask = PIPE_MASK_RGBA;

   if (info->mask & main_mask) {
      for (int slice = 0; slice < info->dst.box.depth; slice++) {
         iris_batch_maybe_flush(batch, 1500);

         blorp_blit(&blorp_batch,
                    &src_surf, info->src.level, info->src.box.z + slice,
                    src_fmt.fmt, src_fmt.swizzle,
                    &dst_surf, info->dst.level, info->dst.box.z + slice,
                    dst_fmt.fmt, dst_fmt.swizzle,
                    src_x0, src_y0, src_x1, src_y1,
                    dst_x0, dst_y0, dst_x1, dst_y1,
                    filter, mirror_x, mirror_y);
      }
   }

   if ((info->mask & PIPE_MASK_S) &&
       util_format_has_stencil(util_format_description(info->dst.format)) &&
       util_format_has_stencil(util_format_description(info->src.format))) {
      struct iris_resource *src_res, *dst_res, *junk;
      iris_get_depth_stencil_resources(info->src.resource, &junk, &src_res);
      iris_get_depth_stencil_resources(info->dst.resource, &junk, &dst_res);
      iris_blorp_surf_for_resource(&ice->vtbl, &src_surf, &src_res->base,
                                   ISL_AUX_USAGE_NONE, info->src.level, false);
      iris_blorp_surf_for_resource(&ice->vtbl, &dst_surf, &dst_res->base,
                                   ISL_AUX_USAGE_NONE, info->dst.level, true);

      for (int slice = 0; slice < info->dst.box.depth; slice++) {
         iris_batch_maybe_flush(batch, 1500);

         blorp_blit(&blorp_batch,
                    &src_surf, info->src.level, info->src.box.z + slice,
                    ISL_FORMAT_R8_UINT, ISL_SWIZZLE_IDENTITY,
                    &dst_surf, info->dst.level, info->dst.box.z + slice,
                    ISL_FORMAT_R8_UINT, ISL_SWIZZLE_IDENTITY,
                    src_x0, src_y0, src_x1, src_y1,
                    dst_x0, dst_y0, dst_x1, dst_y1,
                    filter, mirror_x, mirror_y);
      }
   }

   blorp_batch_finish(&blorp_batch);

   iris_resource_finish_write(ice, dst_res, info->dst.level, info->dst.box.z,
                              info->dst.box.depth, dst_aux_usage);

   iris_flush_and_dirty_for_history(ice, batch, (struct iris_resource *)
                                    info->dst.resource);
}

static void
get_copy_region_aux_settings(const struct gen_device_info *devinfo,
                             struct iris_resource *res,
                             enum isl_aux_usage *out_aux_usage,
                             bool *out_clear_supported)
{
   switch (res->aux.usage) {
   case ISL_AUX_USAGE_MCS:
   case ISL_AUX_USAGE_CCS_E:
      *out_aux_usage = res->aux.usage;
      /* Prior to Gen9, fast-clear only supported 0/1 clear colors.  Since
       * we're going to re-interpret the format as an integer format possibly
       * with a different number of components, we can't handle clear colors
       * until Gen9.
       */
      *out_clear_supported = devinfo->gen >= 9;
      break;
   default:
      *out_aux_usage = ISL_AUX_USAGE_NONE;
      *out_clear_supported = false;
      break;
   }
}

/**
 * Perform a GPU-based raw memory copy between compatible view classes.
 *
 * Does not perform any flushing - the new data may still be left in the
 * render cache, and old data may remain in other caches.
 *
 * Wraps blorp_copy() and blorp_buffer_copy().
 */
void
iris_copy_region(struct blorp_context *blorp,
                 struct iris_batch *batch,
                 struct pipe_resource *dst,
                 unsigned dst_level,
                 unsigned dstx, unsigned dsty, unsigned dstz,
                 struct pipe_resource *src,
                 unsigned src_level,
                 const struct pipe_box *src_box)
{
   struct blorp_batch blorp_batch;
   struct iris_context *ice = blorp->driver_ctx;
   struct iris_screen *screen = (void *) ice->ctx.screen;
   const struct gen_device_info *devinfo = &screen->devinfo;
   struct iris_resource *src_res = (void *) src;
   struct iris_resource *dst_res = (void *) dst;

   enum isl_aux_usage src_aux_usage, dst_aux_usage;
   bool src_clear_supported, dst_clear_supported;
   get_copy_region_aux_settings(devinfo, src_res, &src_aux_usage,
                                &src_clear_supported);
   get_copy_region_aux_settings(devinfo, dst_res, &dst_aux_usage,
                                &dst_clear_supported);

   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      struct blorp_address src_addr = {
         .buffer = iris_resource_bo(src), .offset = src_box->x,
      };
      struct blorp_address dst_addr = {
         .buffer = iris_resource_bo(dst), .offset = dstx,
      };

      iris_batch_maybe_flush(batch, 1500);

      blorp_batch_init(&ice->blorp, &blorp_batch, batch, 0);
      blorp_buffer_copy(&blorp_batch, src_addr, dst_addr, src_box->width);
      blorp_batch_finish(&blorp_batch);

      iris_flush_and_dirty_for_history(ice, batch,
                                       (struct iris_resource *) dst);
   } else {
      // XXX: what about one surface being a buffer and not the other?

      struct blorp_surf src_surf, dst_surf;
      iris_blorp_surf_for_resource(&ice->vtbl, &src_surf, src, src_aux_usage,
                                   src_level, false);
      iris_blorp_surf_for_resource(&ice->vtbl, &dst_surf, dst, dst_aux_usage,
                                   dst_level, true);

      iris_resource_prepare_access(ice, batch, src_res, src_level, 1,
                                   src_box->z, src_box->depth,
                                   src_aux_usage, src_clear_supported);
      iris_resource_prepare_access(ice, batch, dst_res, dst_level, 1,
                                   dstz, src_box->depth,
                                   dst_aux_usage, dst_clear_supported);

      blorp_batch_init(&ice->blorp, &blorp_batch, batch, 0);

      for (int slice = 0; slice < src_box->depth; slice++) {
         iris_batch_maybe_flush(batch, 1500);

         blorp_copy(&blorp_batch, &src_surf, src_level, src_box->z + slice,
                    &dst_surf, dst_level, dstz + slice,
                    src_box->x, src_box->y, dstx, dsty,
                    src_box->width, src_box->height);
      }
      blorp_batch_finish(&blorp_batch);

      iris_resource_finish_write(ice, dst_res, dst_level, dstz,
                                 src_box->depth, dst_aux_usage);
   }
}


/**
 * The pipe->resource_copy_region() driver hook.
 *
 * This implements ARB_copy_image semantics - a raw memory copy between
 * compatible view classes.
 */
static void
iris_resource_copy_region(struct pipe_context *ctx,
                          struct pipe_resource *dst,
                          unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src,
                          unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct iris_context *ice = (void *) ctx;
   struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];

   iris_copy_region(&ice->blorp, batch, dst, dst_level, dstx, dsty, dstz,
                    src, src_level, src_box);
}

void
iris_init_blit_functions(struct pipe_context *ctx)
{
   ctx->blit = iris_blit;
   ctx->resource_copy_region = iris_resource_copy_region;
}