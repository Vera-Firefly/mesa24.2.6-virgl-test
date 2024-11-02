/*
 * Copyright © 2019 Intel Corporation
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

#include "anv_nir.h"
#include "nir_builder.h"
#include "compiler/brw_nir.h"
#include "util/mesa-sha1.h"

#define sizeof_field(type, field) sizeof(((type *)0)->field)

void
anv_nir_compute_push_layout(nir_shader *nir,
                            const struct anv_physical_device *pdevice,
                            enum brw_robustness_flags robust_flags,
                            bool fragment_dynamic,
                            struct brw_stage_prog_data *prog_data,
                            struct anv_pipeline_bind_map *map,
                            const struct anv_pipeline_push_map *push_map,
                            enum anv_descriptor_set_layout_type desc_type,
                            void *mem_ctx)
{
   const struct brw_compiler *compiler = pdevice->compiler;
   const struct intel_device_info *devinfo = compiler->devinfo;
   memset(map->push_ranges, 0, sizeof(map->push_ranges));

   bool has_const_ubo = false;
   unsigned push_start = UINT_MAX, push_end = 0;
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
         nir_foreach_instr(instr, block) {
            if (instr->type != nir_instr_type_intrinsic)
               continue;

            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            switch (intrin->intrinsic) {
            case nir_intrinsic_load_ubo:
               if (brw_nir_ubo_surface_index_is_pushable(intrin->src[0]) &&
                   nir_src_is_const(intrin->src[1]))
                  has_const_ubo = true;
               break;

            case nir_intrinsic_load_push_constant: {
               unsigned base = nir_intrinsic_base(intrin);
               unsigned range = nir_intrinsic_range(intrin);
               push_start = MIN2(push_start, base);
               push_end = MAX2(push_end, base + range);
               break;
            }

            case nir_intrinsic_load_desc_set_address_intel:
            case nir_intrinsic_load_desc_set_dynamic_index_intel: {
               unsigned base = offsetof(struct anv_push_constants,
                                        desc_surface_offsets);
               push_start = MIN2(push_start, base);
               push_end = MAX2(push_end, base +
                  sizeof_field(struct anv_push_constants,
                               desc_surface_offsets));

               if (desc_type == ANV_PIPELINE_DESCRIPTOR_SET_LAYOUT_TYPE_BUFFER &&
                   !pdevice->uses_ex_bso) {
                  base = offsetof(struct anv_push_constants,
                                  surfaces_base_offset);
                  push_start = MIN2(push_start, base);
                  push_end = MAX2(push_end, base +
                                  sizeof_field(struct anv_push_constants,
                                               surfaces_base_offset));
               }
               break;
            }

            default:
               break;
            }
         }
      }
   }

   const bool has_push_intrinsic = push_start <= push_end;

   const bool push_ubo_ranges =
      has_const_ubo && nir->info.stage != MESA_SHADER_COMPUTE &&
      !brw_shader_stage_requires_bindless_resources(nir->info.stage);

   if (push_ubo_ranges && (robust_flags & BRW_ROBUSTNESS_UBO)) {
      /* We can't on-the-fly adjust our push ranges because doing so would
       * mess up the layout in the shader.  When robustBufferAccess is
       * enabled, we push a mask into the shader indicating which pushed
       * registers are valid and we zero out the invalid ones at the top of
       * the shader.
       */
      const uint32_t push_reg_mask_start =
         offsetof(struct anv_push_constants, push_reg_mask[nir->info.stage]);
      const uint32_t push_reg_mask_end = push_reg_mask_start + sizeof(uint64_t);
      push_start = MIN2(push_start, push_reg_mask_start);
      push_end = MAX2(push_end, push_reg_mask_end);
   }

   if (nir->info.stage == MESA_SHADER_FRAGMENT && fragment_dynamic) {
      const uint32_t fs_msaa_flags_start =
         offsetof(struct anv_push_constants, gfx.fs_msaa_flags);
      const uint32_t fs_msaa_flags_end = fs_msaa_flags_start + sizeof(uint32_t);
      push_start = MIN2(push_start, fs_msaa_flags_start);
      push_end = MAX2(push_end, fs_msaa_flags_end);
   }

   if (nir->info.stage == MESA_SHADER_COMPUTE && devinfo->verx10 < 125) {
      /* For compute shaders, we always have to have the subgroup ID.  The
       * back-end compiler will "helpfully" add it for us in the last push
       * constant slot.  Yes, there is an off-by-one error here but that's
       * because the back-end will add it so we want to claim the number of
       * push constants one dword less than the full amount including
       * gl_SubgroupId.
       */
      assert(push_end <= offsetof(struct anv_push_constants, cs.subgroup_id));
      push_end = offsetof(struct anv_push_constants, cs.subgroup_id);
   }

   /* Align push_start down to a 32B boundary and make it no larger than
    * push_end (no push constants is indicated by push_start = UINT_MAX).
    */
   push_start = MIN2(push_start, push_end);
   push_start = ROUND_DOWN_TO(push_start, 32);

   /* For scalar, push data size needs to be aligned to a DWORD. */
   const unsigned alignment = 4;
   nir->num_uniforms = ALIGN(push_end - push_start, alignment);
   prog_data->nr_params = nir->num_uniforms / 4;
   prog_data->param = rzalloc_array(mem_ctx, uint32_t, prog_data->nr_params);

   struct anv_push_range push_constant_range = {
      .set = ANV_DESCRIPTOR_SET_PUSH_CONSTANTS,
      .start = push_start / 32,
      .length = ALIGN(push_end - push_start, devinfo->grf_size) / 32,
   };

   if (has_push_intrinsic) {
      nir_foreach_function_impl(impl, nir) {
         nir_builder build = nir_builder_create(impl);
         nir_builder *b = &build;

         nir_foreach_block(block, impl) {
            nir_foreach_instr_safe(instr, block) {
               if (instr->type != nir_instr_type_intrinsic)
                  continue;

               nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
               switch (intrin->intrinsic) {
               case nir_intrinsic_load_push_constant: {
                  /* With bindless shaders we load uniforms with SEND
                   * messages. All the push constants are located after the
                   * RT_DISPATCH_GLOBALS. We just need to add the offset to
                   * the address right after RT_DISPATCH_GLOBALS (see
                   * brw_nir_lower_rt_intrinsics.c).
                   */
                  unsigned base_offset =
                     brw_shader_stage_requires_bindless_resources(nir->info.stage) ? 0 : push_start;
                  intrin->intrinsic = nir_intrinsic_load_uniform;
                  nir_intrinsic_set_base(intrin,
                                         nir_intrinsic_base(intrin) -
                                         base_offset);
                  break;
               }

               case nir_intrinsic_load_desc_set_address_intel: {
                  assert(brw_shader_stage_requires_bindless_resources(nir->info.stage));
                  b->cursor = nir_before_instr(&intrin->instr);
                  nir_def *desc_offset = nir_load_uniform(b, 1, 32,
                     nir_imul_imm(b, intrin->src[0].ssa, sizeof(uint32_t)),
                     .base = offsetof(struct anv_push_constants,
                                      desc_surface_offsets),
                     .range = sizeof_field(struct anv_push_constants,
                                           desc_surface_offsets),
                     .dest_type = nir_type_uint32);
                  desc_offset = nir_iand_imm(b, desc_offset, ANV_DESCRIPTOR_SET_OFFSET_MASK);
                  if (desc_type == ANV_PIPELINE_DESCRIPTOR_SET_LAYOUT_TYPE_BUFFER &&
                      !pdevice->uses_ex_bso) {
                     nir_def *bindless_base_offset = nir_load_uniform(
                        b, 1, 32,
                        nir_imm_int(b, 0),
                        .base = offsetof(struct anv_push_constants,
                                         surfaces_base_offset),
                        .range = sizeof_field(struct anv_push_constants,
                                              surfaces_base_offset),
                        .dest_type = nir_type_uint32);
                     desc_offset = nir_iadd(b, bindless_base_offset, desc_offset);
                  }
                  nir_def *desc_addr =
                     nir_pack_64_2x32_split(
                        b, desc_offset,
                        nir_load_reloc_const_intel(
                           b,
                           desc_type == ANV_PIPELINE_DESCRIPTOR_SET_LAYOUT_TYPE_BUFFER ?
                           BRW_SHADER_RELOC_DESCRIPTORS_BUFFER_ADDR_HIGH :
                           BRW_SHADER_RELOC_DESCRIPTORS_ADDR_HIGH));
                  nir_def_rewrite_uses(&intrin->def, desc_addr);
                  break;
               }

               case nir_intrinsic_load_desc_set_dynamic_index_intel: {
                  b->cursor = nir_before_instr(&intrin->instr);
                  nir_def *pc_load = nir_load_uniform(b, 1, 32,
                     nir_imul_imm(b, intrin->src[0].ssa, sizeof(uint32_t)),
                     .base = offsetof(struct anv_push_constants,
                                      desc_surface_offsets),
                     .range = sizeof_field(struct anv_push_constants,
                                           desc_surface_offsets),
                     .dest_type = nir_type_uint32);
                  pc_load = nir_iand_imm(
                     b, pc_load, ANV_DESCRIPTOR_SET_DYNAMIC_INDEX_MASK);
                  nir_def_rewrite_uses(&intrin->def, pc_load);
                  break;
               }

               default:
                  break;
               }
            }
         }
      }
   }

   if (push_ubo_ranges) {
      brw_nir_analyze_ubo_ranges(compiler, nir, prog_data->ubo_ranges);

      const unsigned max_push_regs = 64;

      unsigned total_push_regs = push_constant_range.length;
      for (unsigned i = 0; i < 4; i++) {
         if (total_push_regs + prog_data->ubo_ranges[i].length > max_push_regs)
            prog_data->ubo_ranges[i].length = max_push_regs - total_push_regs;
         total_push_regs += prog_data->ubo_ranges[i].length;
      }
      assert(total_push_regs <= max_push_regs);

      int n = 0;

      if (push_constant_range.length > 0)
         map->push_ranges[n++] = push_constant_range;

      if (robust_flags & BRW_ROBUSTNESS_UBO) {
         const uint32_t push_reg_mask_offset =
            offsetof(struct anv_push_constants, push_reg_mask[nir->info.stage]);
         assert(push_reg_mask_offset >= push_start);
         prog_data->push_reg_mask_param =
            (push_reg_mask_offset - push_start) / 4;
      }

      unsigned range_start_reg = push_constant_range.length;

      for (int i = 0; i < 4; i++) {
         struct brw_ubo_range *ubo_range = &prog_data->ubo_ranges[i];
         if (ubo_range->length == 0)
            continue;

         if (n >= 4) {
            memset(ubo_range, 0, sizeof(*ubo_range));
            continue;
         }

         assert(ubo_range->block < push_map->block_count);
         const struct anv_pipeline_binding *binding =
            &push_map->block_to_descriptor[ubo_range->block];

         map->push_ranges[n++] = (struct anv_push_range) {
            .set = binding->set,
            .index = binding->index,
            .dynamic_offset_index = binding->dynamic_offset_index,
            .start = ubo_range->start,
            .length = ubo_range->length,
         };

         /* We only bother to shader-zero pushed client UBOs */
         if (binding->set < MAX_SETS &&
             (robust_flags & BRW_ROBUSTNESS_UBO)) {
            prog_data->zero_push_reg |= BITFIELD64_RANGE(range_start_reg,
                                                         ubo_range->length);
         }

         range_start_reg += ubo_range->length;
      }
   } else {
      /* For Ivy Bridge, the push constants packets have a different
       * rule that would require us to iterate in the other direction
       * and possibly mess around with dynamic state base address.
       * Don't bother; just emit regular push constants at n = 0.
       *
       * In the compute case, we don't have multiple push ranges so it's
       * better to just provide one in push_ranges[0].
       */
      map->push_ranges[0] = push_constant_range;
   }

   if (nir->info.stage == MESA_SHADER_FRAGMENT && fragment_dynamic) {
      struct brw_wm_prog_data *wm_prog_data =
         container_of(prog_data, struct brw_wm_prog_data, base);

      const uint32_t fs_msaa_flags_offset =
         offsetof(struct anv_push_constants, gfx.fs_msaa_flags);
      assert(fs_msaa_flags_offset >= push_start);
      wm_prog_data->msaa_flags_param =
         (fs_msaa_flags_offset - push_start) / 4;
   }

#if 0
   fprintf(stderr, "stage=%s push ranges:\n", gl_shader_stage_name(nir->info.stage));
   for (unsigned i = 0; i < ARRAY_SIZE(map->push_ranges); i++)
      fprintf(stderr, "   range%i: %03u-%03u set=%u index=%u\n", i,
              map->push_ranges[i].start,
              map->push_ranges[i].length,
              map->push_ranges[i].set,
              map->push_ranges[i].index);
#endif

   /* Now that we're done computing the push constant portion of the
    * bind map, hash it.  This lets us quickly determine if the actual
    * mapping has changed and not just a no-op pipeline change.
    */
   _mesa_sha1_compute(map->push_ranges,
                      sizeof(map->push_ranges),
                      map->push_sha1);
}

void
anv_nir_validate_push_layout(const struct anv_physical_device *pdevice,
                             struct brw_stage_prog_data *prog_data,
                             struct anv_pipeline_bind_map *map)
{
#ifndef NDEBUG
   unsigned prog_data_push_size = ALIGN(prog_data->nr_params, pdevice->info.grf_size / 4) / 8;

   for (unsigned i = 0; i < 4; i++)
      prog_data_push_size += prog_data->ubo_ranges[i].length;

   unsigned bind_map_push_size = 0;
   for (unsigned i = 0; i < 4; i++)
      bind_map_push_size += map->push_ranges[i].length;

   /* We could go through everything again but it should be enough to assert
    * that they push the same number of registers.  This should alert us if
    * the back-end compiler decides to re-arrange stuff or shrink a range.
    */
   assert(prog_data_push_size == bind_map_push_size);
#endif
}