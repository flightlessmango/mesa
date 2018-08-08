#ifndef GEN12_CONTEXT_H_
#define GEN12_CONTEXT_H_

static inline void gen12_compute_context_init(const struct gen_context_parameters *params,
                                              uint32_t *data, uint32_t *size)
{
   *size = CONTEXT_RENDER_SIZE;
   if (!data)
      return;
   *data++ = 0 /* MI_NOOP */;
   *data++ = MI_BATCH_BUFFER_END;
}

#endif /* GEN12_CONTEXT_H_ */
