/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "gen_aux_map.h"
#include "gen_gem.h"

#include "dev/gen_device_info.h"

#include "util/list.h"
#include "util/ralloc.h"
#include "main/macros.h"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

static const bool aux_map_debug = false;

struct aux_map_buffer {
   struct list_head link;
   struct gen_buffer *buffer;
};

struct gen_aux_map_context {
   void *driver_ctx;
   struct gen_mapped_pinned_buffer_alloc *buffer_alloc;
   struct list_head buffers;
   uint64_t level3_base_addr;
   uint64_t *level3_map;
   uint32_t tail_offset, tail_remaining;
   uint64_t last_returned_state_num, state_num;
};

static bool
add_buffer(struct gen_aux_map_context *ctx)
{
   struct aux_map_buffer *buf = ralloc(ctx, struct aux_map_buffer);
   if (!buf)
      return false;

   const uint32_t size = 0x100000;
   buf->buffer = ctx->buffer_alloc->alloc(ctx->driver_ctx, size);
   if (!buf->buffer) {
      ralloc_free(buf);
      return false;
   }

   assert(buf->buffer->map != NULL);

   list_addtail(&buf->link, &ctx->buffers);
   ctx->tail_offset = 0;
   ctx->tail_remaining = size;

   return true;
}

static void
advance_current_pos(struct gen_aux_map_context *ctx, uint32_t size)
{
   assert(ctx->tail_remaining >= size);
   ctx->tail_remaining -= size;
   ctx->tail_offset += size;
}

static bool
verify_aligned_space(struct gen_aux_map_context *ctx, uint32_t size,
                     uint32_t align)
{
   if (ctx->tail_remaining < size)
      return false;

   struct aux_map_buffer *tail =
      list_last_entry(&ctx->buffers, struct aux_map_buffer, link);
   uint64_t gpu = tail->buffer->gpu + ctx->tail_offset;
   uint64_t aligned = ALIGN(gpu, align);

   if ((aligned - gpu) + size > ctx->tail_remaining) {
      return false;
   } else {
      if (aligned - gpu > 0)
         advance_current_pos(ctx, aligned - gpu);
      return true;
   }
}

static void
get_current_pos(struct gen_aux_map_context *ctx, uint64_t *gpu, uint64_t **map)
{
   assert(!list_empty(&ctx->buffers));
   struct aux_map_buffer *tail =
      list_last_entry(&ctx->buffers, struct aux_map_buffer, link);
   if (gpu)
      *gpu = tail->buffer->gpu + ctx->tail_offset;
   if (map)
      *map = (uint64_t*)((uint8_t*)tail->buffer->map + ctx->tail_offset);
}

static bool
add_sub_table(struct gen_aux_map_context *ctx, uint32_t size,
              uint32_t align, uint64_t *gpu, uint64_t **map)
{
   if (!verify_aligned_space(ctx, size, align)) {
      if (!add_buffer(ctx))
         return false;
      verify_aligned_space(ctx, size, align);
   }
   get_current_pos(ctx, gpu, map);
   memset(*map, 0, size);
   advance_current_pos(ctx, size);
   return true;
}

static void
new_state_condition(struct gen_aux_map_context *ctx)
{
   if (ctx->last_returned_state_num == ctx->state_num) {
      ctx->state_num++;
      if (aux_map_debug)
         fprintf(stderr, "AUX-MAP state num: 0x%"PRIx64"\n", ctx->state_num);
   }
}

uint64_t
gen_aux_map_get_state_num(struct gen_aux_map_context *ctx)
{
   uint64_t num = ctx->state_num;
   ctx->last_returned_state_num = ctx->state_num;
   return num;
}

struct gen_aux_map_context *
gen_aux_map_init(void *driver_ctx,
                 struct gen_mapped_pinned_buffer_alloc *buffer_alloc,
                 const struct gen_device_info *devinfo)
{
   struct gen_aux_map_context *ctx;
   if (devinfo->gen < 12)
      return NULL;

   ctx = ralloc(NULL, struct gen_aux_map_context);
   if (!ctx)
      return NULL;

   ctx->driver_ctx = driver_ctx;
   ctx->buffer_alloc = buffer_alloc;
   list_inithead(&ctx->buffers);
   ctx->tail_offset = 0;
   ctx->tail_remaining = 0;
   ctx->last_returned_state_num = 0;
   ctx->state_num = 0;

   if (add_sub_table(ctx, 32 * 1024, 32 * 1024, &ctx->level3_base_addr,
                     &ctx->level3_map)) {
      if (aux_map_debug)
         fprintf(stderr, "AUX-MAP L3: 0x%"PRIx64", map=%p\n",
                 ctx->level3_base_addr, ctx->level3_map);
      new_state_condition(ctx);
      return ctx;
   } else {
      ralloc_free(ctx);
      return NULL;
   }
}

void
gen_aux_map_finish(struct gen_aux_map_context *ctx)
{
   if (!ctx)
      return;

   list_for_each_entry_rev(struct aux_map_buffer, buf, &ctx->buffers, link) {
      ctx->buffer_alloc->free(ctx->driver_ctx, buf->buffer);
   }
   list_del(&ctx->buffers);

   ralloc_free(ctx);
}

uint64_t
gen_aux_map_get_base(struct gen_aux_map_context *ctx)
{
   assert(!list_empty(&ctx->buffers));
   if (unlikely(list_empty(&ctx->buffers)))
      return 0;

   return ctx->level3_base_addr;
}

static struct aux_map_buffer *
find_buffer(struct gen_aux_map_context *ctx, uint64_t addr)
{
   list_for_each_entry(struct aux_map_buffer, buf, &ctx->buffers, link) {
      if (buf->buffer->gpu <= addr && buf->buffer->gpu_end > addr) {
         return buf;
      }
   }
   return NULL;
}

static uint64_t *
get_u64_entry_ptr(struct gen_aux_map_context *ctx, uint64_t addr)
{
   struct aux_map_buffer *buf = find_buffer(ctx, addr);
   assert(buf);
   uintptr_t map_offset = addr - buf->buffer->gpu;
   return (uint64_t*)((uint8_t*)buf->buffer->map + map_offset);
}

static uint8_t
get_format_encoding(const struct isl_surf *isl_surf)
{
   switch(isl_surf->format) {
   case ISL_FORMAT_R32G32B32A32_FLOAT: return 0x11;
   case ISL_FORMAT_R32G32B32X32_FLOAT: return 0x11;
   case ISL_FORMAT_R32G32B32A32_SINT: return 0x12;
   case ISL_FORMAT_R32G32B32A32_UINT: return 0x13;
   case ISL_FORMAT_R16G16B16A16_UNORM: return 0x14;
   case ISL_FORMAT_R16G16B16A16_SNORM: return 0x15;
   case ISL_FORMAT_R16G16B16A16_SINT: return 0x16;
   case ISL_FORMAT_R16G16B16A16_UINT: return 0x17;
   case ISL_FORMAT_R16G16B16A16_FLOAT: return 0x10;
   case ISL_FORMAT_R16G16B16X16_FLOAT: return 0x10;
   case ISL_FORMAT_R32G32_FLOAT: return 0x11;
   case ISL_FORMAT_R32G32_SINT: return 0x12;
   case ISL_FORMAT_R32G32_UINT: return 0x13;
   case ISL_FORMAT_B8G8R8A8_UNORM: return 0xA;
   case ISL_FORMAT_B8G8R8X8_UNORM: return 0xA;
   case ISL_FORMAT_B8G8R8A8_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_R10G10B10A2_UNORM: return 0x18;
   case ISL_FORMAT_R10G10B10A2_UNORM_SRGB: return 0x18;
   case ISL_FORMAT_R10G10B10_FLOAT_A2_UNORM: return 0x19;
   case ISL_FORMAT_R10G10B10A2_UINT: return 0x1A;
   case ISL_FORMAT_R8G8B8X8_UNORM: return 0xA;
   case ISL_FORMAT_R8G8B8A8_UNORM: return 0xA;
   case ISL_FORMAT_R8G8B8A8_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_R8G8B8A8_SNORM: return 0x1B;
   case ISL_FORMAT_R8G8B8A8_SINT: return 0x1C;
   case ISL_FORMAT_R8G8B8A8_UINT: return 0x1D;
   case ISL_FORMAT_R16G16_UNORM: return 0x14;
   case ISL_FORMAT_R16G16_SNORM: return 0x15;
   case ISL_FORMAT_R16G16_SINT: return 0x16;
   case ISL_FORMAT_R16G16_UINT: return 0x17;
   case ISL_FORMAT_R16G16_FLOAT: return 0x10;
   case ISL_FORMAT_B10G10R10A2_UNORM: return 0x18;
   case ISL_FORMAT_B10G10R10A2_UNORM_SRGB: return 0x18;
   case ISL_FORMAT_R11G11B10_FLOAT: return 0x1E;
   case ISL_FORMAT_R32_SINT: return 0x12;
   case ISL_FORMAT_R32_UINT: return 0x13;
   case ISL_FORMAT_R32_FLOAT: return 0x11;
   case ISL_FORMAT_B5G6R5_UNORM: return 0xA;
   case ISL_FORMAT_B5G6R5_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_B5G5R5A1_UNORM: return 0xA;
   case ISL_FORMAT_B5G5R5A1_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_B4G4R4A4_UNORM: return 0xA;
   case ISL_FORMAT_B4G4R4A4_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_R8G8_UNORM: return 0xA;
   case ISL_FORMAT_R8G8_SNORM: return 0x1B;
   case ISL_FORMAT_R8G8_SINT: return 0x1C;
   case ISL_FORMAT_R8G8_UINT: return 0x1D;
   case ISL_FORMAT_R16_UNORM: return 0x14;
   case ISL_FORMAT_R16_SNORM: return 0x15;
   case ISL_FORMAT_R16_SINT: return 0x16;
   case ISL_FORMAT_R16_UINT: return 0x17;
   case ISL_FORMAT_R16_FLOAT: return 0x10;
   case ISL_FORMAT_B5G5R5X1_UNORM: return 0xA;
   case ISL_FORMAT_B5G5R5X1_UNORM_SRGB: return 0xA;
   case ISL_FORMAT_A1B5G5R5_UNORM: return 0xA;
   case ISL_FORMAT_A4B4G4R4_UNORM: return 0xA;
   case ISL_FORMAT_R8_UNORM: return 0xA;
   case ISL_FORMAT_R8_SNORM: return 0x1B;
   case ISL_FORMAT_R8_SINT: return 0x1C;
   case ISL_FORMAT_R8_UINT: return 0x1D;
   case ISL_FORMAT_A8_UNORM: return 0xA;
   default:
      unreachable("Unsupported aux-map format!");
      return 0;
   }
}

static void
add_mapping(struct gen_aux_map_context *ctx, uint64_t address,
            uint64_t aux_address, const struct isl_surf *isl_surf)
{
   if (aux_map_debug)
      fprintf(stderr, "AUX-MAP 0x%"PRIx64" => 0x%"PRIx64"\n", address,
              aux_address);

   uint32_t l3_index = (address >> 36) & 0xfff;
   uint64_t *l3_entry = &ctx->level3_map[l3_index];

   uint64_t *l2_map;
   if ((*l3_entry & 1) == 0) {
      uint64_t l2_gpu;
      if (add_sub_table(ctx, 32 * 1024, 32 * 1024, &l2_gpu, &l2_map)) {
         if (aux_map_debug)
            fprintf(stderr, "AUX-MAP L3[0x%x]: 0x%"PRIx64", map=%p\n",
                    l3_index, l2_gpu, l2_map);
      } else {
         unreachable("Failed to add L2 Aux-Map Page Table!");
      }
      *l3_entry = (l2_gpu & 0xffffffff8000ULL) | 1;
   } else {
      uint64_t l2_addr = gen_canonical_address(*l3_entry & ~0x7fffULL);
      l2_map = get_u64_entry_ptr(ctx, l2_addr);
   }
   uint32_t l2_index = (address >> 24) & 0xfff;
   uint64_t *l2_entry = &l2_map[l2_index];

   uint64_t *l1_map;
   if ((*l2_entry & 1) == 0) {
      uint64_t l1_gpu;
      if (add_sub_table(ctx, 8 * 1024, 32 * 1024, &l1_gpu, &l1_map)) {
         if (aux_map_debug)
            fprintf(stderr, "AUX-MAP L2[0x%x]: 0x%"PRIx64", map=%p\n",
                    l2_index, l1_gpu, l1_map);
      } else {
         unreachable("Failed to add L1 Aux-Map Page Table!");
      }
      *l2_entry = (l1_gpu & 0xffffffff8000ULL) | 1;
   } else {
      uint64_t l1_addr = gen_canonical_address(*l2_entry & ~0x1fffULL);
      l1_map = get_u64_entry_ptr(ctx, l1_addr);
   }
   uint32_t l1_index = (address >> 16) & 0xff;
   uint64_t *l1_entry = &l1_map[l1_index];

   const struct isl_format_layout *fmt =
      isl_format_get_layout(isl_surf->format);
   uint16_t bpp = fmt->bpb;
   assert(fmt->bw == 1 && fmt->bh == 1 && fmt->bd == 1);
   if (aux_map_debug)
      fprintf(stderr, "AUX-MAP entry %s, bpp=%d\n",
              isl_format_get_name(isl_surf->format), bpp);

   uint8_t bpp_enc;
   switch (bpp) {
   case 16:  bpp_enc = 0; break;
   case 10:  bpp_enc = 1; break;
   case 12:  bpp_enc = 2; break;
   case 8:   bpp_enc = 4; break;
   case 32:  bpp_enc = 5; break;
   case 64:  bpp_enc = 6; break;
   case 128: bpp_enc = 7; break;
   default:
      unreachable("Unsupported bpp!");
   }

   const uint64_t l1_data =
      (aux_address & 0xffffffffff00ULL) |
      ((uint64_t)get_format_encoding(isl_surf) << 58) |
      ((uint64_t)bpp_enc << 54) |
      (1ULL /* Y tiling */ << 52) |
      1 /* Valid entry */;

   const uint64_t current_l1_data = *l1_entry;
   if ((current_l1_data & 1) == 0) {
      assert((aux_address & 0xffULL) == 0);
      if (aux_map_debug)
         fprintf(stderr, "AUX-MAP L1[0x%x] 0x%"PRIx64" -> 0x%"PRIx64"\n",
                 l1_index, current_l1_data, l1_data);
      /**
       * We use non-zero bits in 63:1 to indicate the entry had been filled
       * previously. If these bits are non-zero and they don't exactly match
       * what we want to program into the entry, then we must force the
       * aux-map tables to be flushed.
       */
      if (current_l1_data != 0 && (current_l1_data | 1) != l1_data)
         new_state_condition(ctx);
      *l1_entry = l1_data;
   } else {
      if (aux_map_debug)
         fprintf(stderr, "AUX-MAP L1[0x%x] is already marked valid!\n",
                 l1_index);
      assert(*l1_entry == l1_data);
   }
}

void
gen_aux_map_add_image(struct gen_aux_map_context *ctx,
                      const struct isl_surf *isl_surf, uint64_t address,
                      uint64_t aux_address)
{
   uint64_t map_addr = address;
   uint64_t dest_aux_addr = aux_address;
   assert(ALIGN(address, 64 * 1024) == address);
   assert(ALIGN(aux_address, 4 * 64) == aux_address);
   while (map_addr - address < isl_surf->size_B) {
      add_mapping(ctx, map_addr, dest_aux_addr, isl_surf);
      map_addr += 64 * 1024;
      dest_aux_addr += 4 * 64;
   }
}

/**
 * We mark the leaf entry as invalid, but we don't attempt to cleanup the
 * other levels of translation mappings. Since we attempt to re-use VMA
 * ranges, hopefully this will not lead to unbounded growth of the translation
 * tables.
 */
static void
remove_mapping(struct gen_aux_map_context *ctx, uint64_t address)
{
   uint32_t l3_index = (address >> 36) & 0xfff;
   uint64_t *l3_entry = &ctx->level3_map[l3_index];

   uint64_t *l2_map;
   if ((*l3_entry & 1) == 0) {
      return;
   } else {
      uint64_t l2_addr = gen_canonical_address(*l3_entry & ~0x7fffULL);
      l2_map = get_u64_entry_ptr(ctx, l2_addr);
   }
   uint32_t l2_index = (address >> 24) & 0xfff;
   uint64_t *l2_entry = &l2_map[l2_index];

   uint64_t *l1_map;
   if ((*l2_entry & 1) == 0) {
      return;
   } else {
      uint64_t l1_addr = gen_canonical_address(*l2_entry & ~0x7fffULL);
      l1_map = get_u64_entry_ptr(ctx, l1_addr);
   }
   uint32_t l1_index = (address >> 16) & 0xff;
   uint64_t *l1_entry = &l1_map[l1_index];

   const uint64_t current_l1_data = *l1_entry;
   const uint64_t l1_data = current_l1_data & ~1ull;

   if ((current_l1_data & 1) == 0) {
      return;
   } else {
      if (aux_map_debug)
         fprintf(stderr, "AUX-MAP [0x%x][0x%x][0x%x] L1 entry removed!\n",
                 l3_index, l2_index, l1_index);
      /**
       * We use non-zero bits in 63:1 to indicate the entry had been filled
       * previously. In the unlikely event that these are all zero, we force a
       * flush of the aux-map tables.
       */
      if (unlikely(l1_data == 0))
         new_state_condition(ctx);
      *l1_entry = l1_data;
   }
}

void
gen_aux_map_unmap_range(struct gen_aux_map_context *ctx, uint64_t address,
                        uint64_t size)
{
   if (aux_map_debug)
      fprintf(stderr, "AUX-MAP remove 0x%"PRIx64"-0x%"PRIx64"\n", address,
              address + size);

   uint64_t map_addr = address;
   assert(ALIGN(address, 64 * 1024) == address);
   while (map_addr - address < size) {
      remove_mapping(ctx, map_addr);
      map_addr += 64 * 1024;
   }
}
