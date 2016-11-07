/*
 * Copyright 2016 Intel Corporation
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "isl.h"
#include "isl_priv.h"
#include "dev/gen_device_info.h"

#include "util/format_srgb.h"
#include "util/ralloc.h"

#include "main/macros.h"

/**
 * @brief Determine if a surface should be dumped.
 *
 * Since dumping a surface can produce a lot of data and be time consuming,
 * this function allows you to filter whether a surface should actually be
 * dumped. If the application is deterministic, then you can use the sequence
 * id number to filter output. Other examples are shown commented out.
 *
 * Return true when the surface should be dumped.
 */
static inline bool
filter_surface_dumping(uint64_t sequence_id,
                       const struct isl_surf *surf,
                       const void *map_addr,
                       unsigned int map_size,
                       const struct isl_surf *aux_surf,
                       const void *aux_map_addr,
                       unsigned int aux_map_size,
                       const char *basename)
{
   const uint64_t single_id = 0;
   const uint64_t min_id = 0;
   const uint64_t max_id = 0;
   return
      (min_id == 0 || sequence_id >= min_id) &&
      (max_id == 0 || sequence_id <= max_id) &&
      (single_id == 0 || sequence_id == single_id) &&
      /* surf->format == ISL_FORMAT_R8_UINT && */
      /* surf->msaa_layout == ISL_MSAA_LAYOUT_NONE && */
      true;
}

static const char *
tiling_name(enum isl_tiling tiling)
{
#define TILENAME(t) case ISL_TILING_##t: return #t
   switch(tiling) {
   TILENAME(LINEAR);
   TILENAME(W);
   TILENAME(X);
   TILENAME(Y0);
   TILENAME(Yf);
   TILENAME(Ys);
   TILENAME(HIZ);
   TILENAME(CCS);
   default:
      return NULL;
   }
}

static const char *
msaa_name(enum isl_msaa_layout layout)
{
#define MSAA_NAME(l) case ISL_MSAA_LAYOUT_##l: return #l
   switch(layout) {
   MSAA_NAME(NONE);
   MSAA_NAME(INTERLEAVED);
   MSAA_NAME(ARRAY);
   default:
      return NULL;
   }
}

static bool
dump_surf_info(const struct isl_device *dev,
               const struct isl_surf *surf,
               const char *filename)
{
   FILE *f = fopen(filename, "w");
   if (!f)
      goto fail_open;

   fprintf(f, "Format: %s\n", isl_format_get_name(surf->format));
   fprintf(f, "Tiling: %s\n", tiling_name(surf->tiling));
   fprintf(f, "Row pitch (B): %d\n", surf->row_pitch_B);
   fprintf(f, "Array pitch (q-pitch): %d\n", surf->array_pitch_el_rows);
   fprintf(f, "Bit6 swizzle: %sabled\n", dev->has_bit6_swizzling ? "En" : "Dis");
   fprintf(f, "Samples: %d\n", surf->samples);
   fprintf(f, "MSAA layout: %s\n", msaa_name(surf->msaa_layout));
   fprintf(f, "Logical size LOD0 (px): %d x %d\n",
           surf->logical_level0_px.w, surf->logical_level0_px.h);
   fprintf(f, "Physical size LOD0 (sa): %d x %d\n",
           surf->phys_level0_sa.w, surf->phys_level0_sa.h);

   fclose(f);
   return true;

 fail_open:
   fprintf(stderr, "%s:%s: couldn't open file: %s\n", __FILE__, __func__, filename);
   return false;
}

static bool
dump_surf_binary(const void *map_addr,
                 unsigned int map_size,
                 const char *filename)
{
   FILE *f = fopen(filename, "w");
   if (!f)
      goto fail_open;

   fwrite(map_addr, 1, map_size, f);

   fclose(f);
   return true;

 fail_open:
   fprintf(stderr, "%s:%s: couldn't open file: %s\n", __FILE__, __func__, filename);
   return false;
}

void
isl_surf_dump(const struct isl_device *dev,
              const struct isl_surf *surf,
              const void *map_addr,
              unsigned int map_size,
              const struct isl_surf *aux_surf,
              const void *aux_map_addr,
              unsigned int aux_map_size,
              const char *basename)
{
   int ret;

   char filename[1024];
   static uint64_t sequence_id = 0;

   sequence_id++;

   if (!filter_surface_dumping(sequence_id, surf, map_addr, map_size, aux_surf,
                               aux_map_addr, aux_map_size, basename))
      return;

   ret = snprintf(filename, sizeof(filename), "%04lu-%s.txt",
                  sequence_id, basename);
   if (ret < -1 || ret >= sizeof(filename))
      goto fail_basename_too_big;

   if (!dump_surf_info(dev, surf, filename))
      goto fail_dump_info;

   if (map_addr != NULL && map_size > 0) {
      ret = snprintf(filename, sizeof(filename), "%04lu-%s.bin",
                     sequence_id, basename);
      if (ret < -1 || ret >= sizeof(filename))
         goto fail_basename_too_big;

      if (!dump_surf_binary(map_addr, map_size, filename))
         goto fail_dump_info;
   }

   if (aux_surf) {
      ret = snprintf(filename, sizeof(filename), "%04lu-%s-aux.txt",
                     sequence_id, basename);
      if (ret < -1 || ret >= sizeof(filename))
         goto fail_basename_too_big;

      if (!dump_surf_info(dev, aux_surf, filename))
         goto fail_dump_info;

      if (aux_map_addr != NULL && aux_map_size > 0) {
         ret = snprintf(filename, sizeof(filename), "%04lu-%s-aux.bin",
                        sequence_id, basename);
         if (ret < -1 || ret >= sizeof(filename))
            goto fail_basename_too_big;

         if (!dump_surf_binary(aux_map_addr, aux_map_size, filename))
            goto fail_dump_info;
      }
   }

   return;

 fail_basename_too_big:
   fprintf(stderr, "%s:%s: basename too big\n", __FILE__, __func__);
   abort();

 fail_dump_info:
   fprintf(stderr, "%s:%s: failed to dump surface info\n", __FILE__, __func__);
   abort();
}
