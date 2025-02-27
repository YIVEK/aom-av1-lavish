/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include <assert.h>

#include "config/aom_config.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/txfm_common.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"
#include "av1/common/common.h"
#include "av1/common/arm/convolve_neon.h"

#if !defined(__aarch64__)
static INLINE void compute_avg_4x1(
    uint16x4_t res0, uint16x4_t d0, const uint16_t fwd_offset,
    const uint16_t bck_offset, const int16x4_t sub_const_vec,
    const int16_t round_bits, const int use_dist_wtd_comp_avg, uint8x8_t *t0) {
  int16x4_t tmp0;
  uint16x4_t tmp_u0;
  uint32x4_t sum0;
  int32x4_t dst0;
  int16x8_t tmp4;

  if (use_dist_wtd_comp_avg) {
    const int32x4_t round_bits_vec = vdupq_n_s32((int32_t)(-round_bits));

    sum0 = vmull_n_u16(res0, fwd_offset);
    sum0 = vmlal_n_u16(sum0, d0, bck_offset);

    sum0 = vshrq_n_u32(sum0, DIST_PRECISION_BITS);

    dst0 = vsubq_s32(vreinterpretq_s32_u32(sum0), vmovl_s16(sub_const_vec));

    dst0 = vqrshlq_s32(dst0, round_bits_vec);

    tmp0 = vmovn_s32(dst0);
    tmp4 = vcombine_s16(tmp0, tmp0);

    *t0 = vqmovun_s16(tmp4);
  } else {
    const int16x4_t round_bits_vec = vdup_n_s16(-round_bits);
    tmp_u0 = vhadd_u16(res0, d0);

    tmp0 = vsub_s16(vreinterpret_s16_u16(tmp_u0), sub_const_vec);

    tmp0 = vqrshl_s16(tmp0, round_bits_vec);

    tmp4 = vcombine_s16(tmp0, vdup_n_s16(0));

    *t0 = vqmovun_s16(tmp4);
  }
}

static INLINE void compute_avg_8x1(
    uint16x8_t res0, uint16x8_t d0, const uint16_t fwd_offset,
    const uint16_t bck_offset, const int16x4_t sub_const,
    const int16_t round_bits, const int use_dist_wtd_comp_avg, uint8x8_t *t0) {
  int16x8_t f0;
  uint32x4_t sum0, sum2;
  int32x4_t dst0, dst2;

  uint16x8_t tmp_u0;

  if (use_dist_wtd_comp_avg) {
    const int32x4_t sub_const_vec = vmovl_s16(sub_const);
    const int32x4_t round_bits_vec = vdupq_n_s32(-(int32_t)round_bits);

    sum0 = vmull_n_u16(vget_low_u16(res0), fwd_offset);
    sum0 = vmlal_n_u16(sum0, vget_low_u16(d0), bck_offset);
    sum0 = vshrq_n_u32(sum0, DIST_PRECISION_BITS);

    sum2 = vmull_n_u16(vget_high_u16(res0), fwd_offset);
    sum2 = vmlal_n_u16(sum2, vget_high_u16(d0), bck_offset);
    sum2 = vshrq_n_u32(sum2, DIST_PRECISION_BITS);

    dst0 = vsubq_s32(vreinterpretq_s32_u32(sum0), sub_const_vec);
    dst2 = vsubq_s32(vreinterpretq_s32_u32(sum2), sub_const_vec);

    dst0 = vqrshlq_s32(dst0, round_bits_vec);
    dst2 = vqrshlq_s32(dst2, round_bits_vec);

    f0 = vcombine_s16(vmovn_s32(dst0), vmovn_s32(dst2));

    *t0 = vqmovun_s16(f0);

  } else {
    const int16x8_t sub_const_vec = vcombine_s16(sub_const, sub_const);
    const int16x8_t round_bits_vec = vdupq_n_s16(-round_bits);

    tmp_u0 = vhaddq_u16(res0, d0);

    f0 = vsubq_s16(vreinterpretq_s16_u16(tmp_u0), sub_const_vec);

    f0 = vqrshlq_s16(f0, round_bits_vec);

    *t0 = vqmovun_s16(f0);
  }
}
#endif  // !defined(__arch64__)

static INLINE void compute_avg_4x4(
    uint16x4_t res0, uint16x4_t res1, uint16x4_t res2, uint16x4_t res3,
    uint16x4_t d0, uint16x4_t d1, uint16x4_t d2, uint16x4_t d3,
    const uint16_t fwd_offset, const uint16_t bck_offset,
    const int16x4_t sub_const_vec, const int16_t round_bits,
    const int use_dist_wtd_comp_avg, uint8x8_t *t0, uint8x8_t *t1) {
  int16x4_t tmp0, tmp1, tmp2, tmp3;
  uint16x4_t tmp_u0, tmp_u1, tmp_u2, tmp_u3;
  uint32x4_t sum0, sum1, sum2, sum3;

  int32x4_t dst0, dst1, dst2, dst3;
  int16x8_t tmp4, tmp5;

  if (use_dist_wtd_comp_avg) {
    const int32x4_t round_bits_vec = vdupq_n_s32((int32_t)(-round_bits));
    const int32x4_t const_vec = vmovl_s16(sub_const_vec);

    sum0 = vmull_n_u16(res0, fwd_offset);
    sum0 = vmlal_n_u16(sum0, d0, bck_offset);
    sum1 = vmull_n_u16(res1, fwd_offset);
    sum1 = vmlal_n_u16(sum1, d1, bck_offset);
    sum2 = vmull_n_u16(res2, fwd_offset);
    sum2 = vmlal_n_u16(sum2, d2, bck_offset);
    sum3 = vmull_n_u16(res3, fwd_offset);
    sum3 = vmlal_n_u16(sum3, d3, bck_offset);

    sum0 = vshrq_n_u32(sum0, DIST_PRECISION_BITS);
    sum1 = vshrq_n_u32(sum1, DIST_PRECISION_BITS);
    sum2 = vshrq_n_u32(sum2, DIST_PRECISION_BITS);
    sum3 = vshrq_n_u32(sum3, DIST_PRECISION_BITS);

    dst0 = vsubq_s32(vreinterpretq_s32_u32(sum0), const_vec);
    dst1 = vsubq_s32(vreinterpretq_s32_u32(sum1), const_vec);
    dst2 = vsubq_s32(vreinterpretq_s32_u32(sum2), const_vec);
    dst3 = vsubq_s32(vreinterpretq_s32_u32(sum3), const_vec);

    dst0 = vqrshlq_s32(dst0, round_bits_vec);
    dst1 = vqrshlq_s32(dst1, round_bits_vec);
    dst2 = vqrshlq_s32(dst2, round_bits_vec);
    dst3 = vqrshlq_s32(dst3, round_bits_vec);

    tmp4 = vcombine_s16(vmovn_s32(dst0), vmovn_s32(dst1));
    tmp5 = vcombine_s16(vmovn_s32(dst2), vmovn_s32(dst3));

    *t0 = vqmovun_s16(tmp4);
    *t1 = vqmovun_s16(tmp5);
  } else {
    const int16x4_t round_bits_vec = vdup_n_s16(-round_bits);
    tmp_u0 = vhadd_u16(res0, d0);
    tmp_u1 = vhadd_u16(res1, d1);
    tmp_u2 = vhadd_u16(res2, d2);
    tmp_u3 = vhadd_u16(res3, d3);

    tmp0 = vsub_s16(vreinterpret_s16_u16(tmp_u0), sub_const_vec);
    tmp1 = vsub_s16(vreinterpret_s16_u16(tmp_u1), sub_const_vec);
    tmp2 = vsub_s16(vreinterpret_s16_u16(tmp_u2), sub_const_vec);
    tmp3 = vsub_s16(vreinterpret_s16_u16(tmp_u3), sub_const_vec);

    tmp0 = vqrshl_s16(tmp0, round_bits_vec);
    tmp1 = vqrshl_s16(tmp1, round_bits_vec);
    tmp2 = vqrshl_s16(tmp2, round_bits_vec);
    tmp3 = vqrshl_s16(tmp3, round_bits_vec);

    tmp4 = vcombine_s16(tmp0, tmp1);
    tmp5 = vcombine_s16(tmp2, tmp3);

    *t0 = vqmovun_s16(tmp4);
    *t1 = vqmovun_s16(tmp5);
  }
}

static INLINE void compute_avg_8x4(
    uint16x8_t res0, uint16x8_t res1, uint16x8_t res2, uint16x8_t res3,
    uint16x8_t d0, uint16x8_t d1, uint16x8_t d2, uint16x8_t d3,
    const uint16_t fwd_offset, const uint16_t bck_offset,
    const int16x4_t sub_const, const int16_t round_bits,
    const int use_dist_wtd_comp_avg, uint8x8_t *t0, uint8x8_t *t1,
    uint8x8_t *t2, uint8x8_t *t3) {
  int16x8_t f0, f1, f2, f3;
  uint32x4_t sum0, sum1, sum2, sum3;
  uint32x4_t sum4, sum5, sum6, sum7;
  int32x4_t dst0, dst1, dst2, dst3;
  int32x4_t dst4, dst5, dst6, dst7;
  uint16x8_t tmp_u0, tmp_u1, tmp_u2, tmp_u3;

  if (use_dist_wtd_comp_avg) {
    const int32x4_t sub_const_vec = vmovl_s16(sub_const);
    const int32x4_t round_bits_vec = vdupq_n_s32(-(int32_t)round_bits);

    sum0 = vmull_n_u16(vget_low_u16(res0), fwd_offset);
    sum0 = vmlal_n_u16(sum0, vget_low_u16(d0), bck_offset);
    sum1 = vmull_n_u16(vget_low_u16(res1), fwd_offset);
    sum1 = vmlal_n_u16(sum1, vget_low_u16(d1), bck_offset);
    sum0 = vshrq_n_u32(sum0, DIST_PRECISION_BITS);
    sum1 = vshrq_n_u32(sum1, DIST_PRECISION_BITS);

    sum2 = vmull_n_u16(vget_high_u16(res0), fwd_offset);
    sum2 = vmlal_n_u16(sum2, vget_high_u16(d0), bck_offset);
    sum3 = vmull_n_u16(vget_high_u16(res1), fwd_offset);
    sum3 = vmlal_n_u16(sum3, vget_high_u16(d1), bck_offset);
    sum2 = vshrq_n_u32(sum2, DIST_PRECISION_BITS);
    sum3 = vshrq_n_u32(sum3, DIST_PRECISION_BITS);

    sum4 = vmull_n_u16(vget_low_u16(res2), fwd_offset);
    sum4 = vmlal_n_u16(sum4, vget_low_u16(d2), bck_offset);
    sum5 = vmull_n_u16(vget_low_u16(res3), fwd_offset);
    sum5 = vmlal_n_u16(sum5, vget_low_u16(d3), bck_offset);
    sum4 = vshrq_n_u32(sum4, DIST_PRECISION_BITS);
    sum5 = vshrq_n_u32(sum5, DIST_PRECISION_BITS);

    sum6 = vmull_n_u16(vget_high_u16(res2), fwd_offset);
    sum6 = vmlal_n_u16(sum6, vget_high_u16(d2), bck_offset);
    sum7 = vmull_n_u16(vget_high_u16(res3), fwd_offset);
    sum7 = vmlal_n_u16(sum7, vget_high_u16(d3), bck_offset);
    sum6 = vshrq_n_u32(sum6, DIST_PRECISION_BITS);
    sum7 = vshrq_n_u32(sum7, DIST_PRECISION_BITS);

    dst0 = vsubq_s32(vreinterpretq_s32_u32(sum0), sub_const_vec);
    dst1 = vsubq_s32(vreinterpretq_s32_u32(sum1), sub_const_vec);
    dst2 = vsubq_s32(vreinterpretq_s32_u32(sum2), sub_const_vec);
    dst3 = vsubq_s32(vreinterpretq_s32_u32(sum3), sub_const_vec);
    dst4 = vsubq_s32(vreinterpretq_s32_u32(sum4), sub_const_vec);
    dst5 = vsubq_s32(vreinterpretq_s32_u32(sum5), sub_const_vec);
    dst6 = vsubq_s32(vreinterpretq_s32_u32(sum6), sub_const_vec);
    dst7 = vsubq_s32(vreinterpretq_s32_u32(sum7), sub_const_vec);

    dst0 = vqrshlq_s32(dst0, round_bits_vec);
    dst1 = vqrshlq_s32(dst1, round_bits_vec);
    dst2 = vqrshlq_s32(dst2, round_bits_vec);
    dst3 = vqrshlq_s32(dst3, round_bits_vec);
    dst4 = vqrshlq_s32(dst4, round_bits_vec);
    dst5 = vqrshlq_s32(dst5, round_bits_vec);
    dst6 = vqrshlq_s32(dst6, round_bits_vec);
    dst7 = vqrshlq_s32(dst7, round_bits_vec);

    f0 = vcombine_s16(vmovn_s32(dst0), vmovn_s32(dst2));
    f1 = vcombine_s16(vmovn_s32(dst1), vmovn_s32(dst3));
    f2 = vcombine_s16(vmovn_s32(dst4), vmovn_s32(dst6));
    f3 = vcombine_s16(vmovn_s32(dst5), vmovn_s32(dst7));

    *t0 = vqmovun_s16(f0);
    *t1 = vqmovun_s16(f1);
    *t2 = vqmovun_s16(f2);
    *t3 = vqmovun_s16(f3);

  } else {
    const int16x8_t sub_const_vec = vcombine_s16(sub_const, sub_const);
    const int16x8_t round_bits_vec = vdupq_n_s16(-round_bits);

    tmp_u0 = vhaddq_u16(res0, d0);
    tmp_u1 = vhaddq_u16(res1, d1);
    tmp_u2 = vhaddq_u16(res2, d2);
    tmp_u3 = vhaddq_u16(res3, d3);

    f0 = vsubq_s16(vreinterpretq_s16_u16(tmp_u0), sub_const_vec);
    f1 = vsubq_s16(vreinterpretq_s16_u16(tmp_u1), sub_const_vec);
    f2 = vsubq_s16(vreinterpretq_s16_u16(tmp_u2), sub_const_vec);
    f3 = vsubq_s16(vreinterpretq_s16_u16(tmp_u3), sub_const_vec);

    f0 = vqrshlq_s16(f0, round_bits_vec);
    f1 = vqrshlq_s16(f1, round_bits_vec);
    f2 = vqrshlq_s16(f2, round_bits_vec);
    f3 = vqrshlq_s16(f3, round_bits_vec);

    *t0 = vqmovun_s16(f0);
    *t1 = vqmovun_s16(f1);
    *t2 = vqmovun_s16(f2);
    *t3 = vqmovun_s16(f3);
  }
}

#if defined(__aarch64__) && defined(__ARM_FEATURE_MATMUL_INT8)

static INLINE void dist_wtd_convolve_2d_horiz_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter_s16, const int im_h, int w) {
  const int bd = 8;
  int16_t *dst_ptr = im_block;
  int dst_stride = im_stride;
  int width = w;
  int height = im_h;

  const int8x8_t x_filter = vmovn_s16(x_filter_s16);
  // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // The outermost -1 is needed because we halved the filter values.
  const int32x4_t horiz_const = vdupq_n_s32((1 << (bd + FILTER_BITS - 2)) +
                                            (1 << ((ROUND0_BITS - 1) - 1)));

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);
    uint8x16_t s0, s1, s2, s3;
    int32x4_t t0, t1, t2, t3;
    int16x4_t d0, d1, d2, d3;

    do {
      load_u8_16x4(src, src_stride, &s0, &s1, &s2, &s3);

      t0 = convolve8_4_usdot(s0, x_filter, permute_tbl, horiz_const);
      t1 = convolve8_4_usdot(s1, x_filter, permute_tbl, horiz_const);
      t2 = convolve8_4_usdot(s2, x_filter, permute_tbl, horiz_const);
      t3 = convolve8_4_usdot(s3, x_filter, permute_tbl, horiz_const);

      // We halved the convolution filter values so -1 from the right shift.
      d0 = vshrn_n_s32(t0, ROUND0_BITS - 1);
      d1 = vshrn_n_s32(t1, ROUND0_BITS - 1);
      d2 = vshrn_n_s32(t2, ROUND0_BITS - 1);
      d3 = vshrn_n_s32(t3, ROUND0_BITS - 1);

      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);
    const uint8_t *s;
    int16_t *d;
    uint8x16_t s0, s1, s2, s3;
    int16x8_t d0, d1, d2, d3;

    do {
      width = w;
      s = src;
      d = dst_ptr;

      do {
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_horiz_8_usdot(s0, x_filter, permute_tbl, horiz_const);
        d1 = convolve8_horiz_8_usdot(s1, x_filter, permute_tbl, horiz_const);
        d2 = convolve8_horiz_8_usdot(s2, x_filter, permute_tbl, horiz_const);
        d3 = convolve8_horiz_8_usdot(s3, x_filter, permute_tbl, horiz_const);

        store_s16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);

      src += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

#elif defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)

static INLINE void dist_wtd_convolve_2d_horiz_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter_s16, const int im_h, int w) {
  const int bd = 8;
  int16_t *dst_ptr = im_block;
  int dst_stride = im_stride;
  int width = w;
  int height = im_h;

  const int8x8_t x_filter = vmovn_s16(x_filter_s16);
  const int32_t horiz_const = (1 << (bd + FILTER_BITS - 2));
  // Dot product constants.
  const int16x8_t correct_tmp = vshlq_n_s16(x_filter_s16, 7);
  // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // The outermost -1 is needed because we halved the filter values.
  const int32x4_t correction = vdupq_n_s32(
      vaddlvq_s16(correct_tmp) + horiz_const + (1 << ((ROUND0_BITS - 1) - 1)));
  const uint8x16_t range_limit = vdupq_n_u8(128);

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);
    uint8x16_t s0, s1, s2, s3;
    int32x4_t t0, t1, t2, t3;
    int16x4_t d0, d1, d2, d3;

    do {
      load_u8_16x4(src, src_stride, &s0, &s1, &s2, &s3);

      t0 = convolve8_4_sdot(s0, x_filter, correction, range_limit, permute_tbl);
      t1 = convolve8_4_sdot(s1, x_filter, correction, range_limit, permute_tbl);
      t2 = convolve8_4_sdot(s2, x_filter, correction, range_limit, permute_tbl);
      t3 = convolve8_4_sdot(s3, x_filter, correction, range_limit, permute_tbl);

      // We halved the convolution filter values so -1 from the right shift.
      d0 = vshrn_n_s32(t0, ROUND0_BITS - 1);
      d1 = vshrn_n_s32(t1, ROUND0_BITS - 1);
      d2 = vshrn_n_s32(t2, ROUND0_BITS - 1);
      d3 = vshrn_n_s32(t3, ROUND0_BITS - 1);

      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);
    const uint8_t *s;
    int16_t *d;
    uint8x16_t s0, s1, s2, s3;
    int16x8_t d0, d1, d2, d3;

    do {
      width = w;
      s = src;
      d = dst_ptr;

      do {
        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_horiz_8_sdot(s0, x_filter, correction, range_limit,
                                    permute_tbl);
        d1 = convolve8_horiz_8_sdot(s1, x_filter, correction, range_limit,
                                    permute_tbl);
        d2 = convolve8_horiz_8_sdot(s2, x_filter, correction, range_limit,
                                    permute_tbl);
        d3 = convolve8_horiz_8_sdot(s3, x_filter, correction, range_limit,
                                    permute_tbl);

        // We halved the convolution filter values so -1 from the right shift.
        d0 = vshrq_n_s16(d0, ROUND0_BITS - 1);
        d1 = vshrq_n_s16(d1, ROUND0_BITS - 1);
        d2 = vshrq_n_s16(d2, ROUND0_BITS - 1);
        d3 = vshrq_n_s16(d3, ROUND0_BITS - 1);

        store_s16_8x4(d, dst_stride, d0, d1, d2, d3);

        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);

      src += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

#else  // !(defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD))

static INLINE void dist_wtd_convolve_2d_horiz_neon(
    const uint8_t *src, int src_stride, int16_t *im_block, const int im_stride,
    const int16x8_t x_filter, const int im_h, int w) {
  const int bd = 8;
  const uint8_t *s;
  int16_t *dst_ptr;
  int dst_stride;
  int width, height;

  dst_ptr = im_block;
  dst_stride = im_stride;
  height = im_h;
  width = w;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, d0;
    int16x8_t tt0;
    uint8x8_t t0;

    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x4_t horiz_const = vdup_n_s16((1 << (bd + FILTER_BITS - 2)) +
                                             (1 << ((ROUND0_BITS - 1) - 1)));

#if defined(__aarch64__)
    int16x4_t s8, s9, s10, d1, d2, d3;
    int16x8_t tt1, tt2, tt3;
    uint8x8_t t1, t2, t3;
#endif
    do {
      s = src;
      __builtin_prefetch(s + 0 * src_stride);
#if defined(__aarch64__)
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      load_u8_8x4(s, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);
      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      tt2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      tt3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s0 = vget_low_s16(tt0);
      s1 = vget_low_s16(tt1);
      s2 = vget_low_s16(tt2);
      s3 = vget_low_s16(tt3);
      s4 = vget_high_s16(tt0);
      s5 = vget_high_s16(tt1);
      s6 = vget_high_s16(tt2);
      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      s += 7;

      load_u8_8x4(s, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);
      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      tt2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      tt3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s7 = vget_low_s16(tt0);
      s8 = vget_low_s16(tt1);
      s9 = vget_low_s16(tt2);
      s10 = vget_low_s16(tt3);

      d0 = convolve8_horiz_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                   horiz_const);
      d1 = convolve8_horiz_4x4_s16(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                   horiz_const);
      d2 = convolve8_horiz_4x4_s16(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                   horiz_const);
      d3 = convolve8_horiz_4x4_s16(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                   horiz_const);

      transpose_s16_4x4d(&d0, &d1, &d2, &d3);

      store_s16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);

      src += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
#else
      t0 = vld1_u8(s);                            // a0 a1 a2 a3 a4 a5 a6 a7
      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7
      s0 = vget_low_s16(tt0);                     // a0 a1 a2 a3
      s4 = vget_high_s16(tt0);                    // a4 a5 a6 a7
      __builtin_prefetch(dst_ptr);
      s += 8;
      t0 = vld1_u8(s);  // a8 a9 a10 a11

      // a8 a9 a10 a11
      s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));

      s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
      s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
      s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
      s5 = vext_s16(s4, s7, 1);  // a5 a6 a7 a8
      s6 = vext_s16(s4, s7, 2);  // a6 a7 a8 a9
      s7 = vext_s16(s4, s7, 3);  // a7 a8 a9 a10

      d0 = convolve8_horiz_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                   horiz_const);

      vst1_s16(dst_ptr, d0);

      src += src_stride;
      dst_ptr += dst_stride;
      height -= 1;
#endif
    } while (height > 0);
  } else {
    int16_t *d_tmp;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    int16x8_t res0;
    uint8x8_t t0;

    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x8_t horiz_const = vdupq_n_s16((1 << (bd + FILTER_BITS - 2)) +
                                              (1 << ((ROUND0_BITS - 1) - 1)));
    do {
#if defined(__aarch64__)
      uint8x8_t t1, t2, t3, t4, t5, t6, t7;
      int16x8_t s8, s9, s10, s11, s12, s13, s14;
      int16x8_t res1, res2, res3, res4, res5, res6, res7;
      __builtin_prefetch(src + 0 * src_stride);
      __builtin_prefetch(src + 1 * src_stride);
      __builtin_prefetch(src + 2 * src_stride);
      __builtin_prefetch(src + 3 * src_stride);
      __builtin_prefetch(src + 4 * src_stride);
      __builtin_prefetch(src + 5 * src_stride);
      __builtin_prefetch(src + 6 * src_stride);
      __builtin_prefetch(src + 7 * src_stride);
      load_u8_8x8(src, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      width = w;
      s = src + 7;
      d_tmp = dst_ptr;
      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      do {
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        res0 = convolve8_horiz_8x8_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                       horiz_const);
        res1 = convolve8_horiz_8x8_s16(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                       horiz_const);
        res2 = convolve8_horiz_8x8_s16(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                       horiz_const);
        res3 = convolve8_horiz_8x8_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                       x_filter, horiz_const);
        res4 = convolve8_horiz_8x8_s16(s4, s5, s6, s7, s8, s9, s10, s11,
                                       x_filter, horiz_const);
        res5 = convolve8_horiz_8x8_s16(s5, s6, s7, s8, s9, s10, s11, s12,
                                       x_filter, horiz_const);
        res6 = convolve8_horiz_8x8_s16(s6, s7, s8, s9, s10, s11, s12, s13,
                                       x_filter, horiz_const);
        res7 = convolve8_horiz_8x8_s16(s7, s8, s9, s10, s11, s12, s13, s14,
                                       x_filter, horiz_const);

        transpose_s16_8x8(&res0, &res1, &res2, &res3, &res4, &res5, &res6,
                          &res7);

        store_s16_8x8(d_tmp, dst_stride, res0, res1, res2, res3, res4, res5,
                      res6, res7);
        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8;
        d_tmp += 8;
        width -= 8;
      } while (width > 0);
      src += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      height -= 8;
#else
      int16x8_t temp_0;
      t0 = vld1_u8(src);
      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      width = w;
      s = src + 8;
      d_tmp = dst_ptr;
      __builtin_prefetch(dst_ptr);

      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        temp_0 = s0;
        s0 = s7;

        s1 = vextq_s16(temp_0, s7, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        s2 = vextq_s16(temp_0, s7, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        s3 = vextq_s16(temp_0, s7, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        s4 = vextq_s16(temp_0, s7, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        s5 = vextq_s16(temp_0, s7, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        s6 = vextq_s16(temp_0, s7, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        s7 = vextq_s16(temp_0, s7, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        res0 = convolve8_horiz_8x8_s16(temp_0, s1, s2, s3, s4, s5, s6, s7,
                                       x_filter, horiz_const);
        vst1q_s16(d_tmp, res0);

        s += 8;
        d_tmp += 8;
        width -= 8;
      } while (width > 0);
      src += src_stride;
      dst_ptr += dst_stride;
      height -= 1;
#endif
    } while (height > 0);
  }
}

#endif  // defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)

static INLINE void dist_wtd_convolve_2d_vert_6tap_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t sub_const = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                            (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));

  const int16_t round_bits =
      2 * FILTER_BITS - ROUND0_BITS - COMPOUND_ROUND1_BITS;
  const int offset = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset);
  const int16x4_t sub_const_vec = vdup_n_s16(sub_const);
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int do_average = conv_params->do_average;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5;
    uint16x4_t dd0, d0;
    uint8x8_t d01_u8;

#if defined(__aarch64__)
    int16x4_t s6, s7, s8;
    uint16x4_t dd1, dd2, dd3, d1, d2, d3;
    uint8x8_t d23_u8;
#endif

    load_s16_4x5(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4);
    src_ptr += 5 * src_stride;

    do {
#if defined(__aarch64__)
      load_s16_4x4(src_ptr, src_stride, &s5, &s6, &s7, &s8);

      d0 = convolve6_4_s32(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
      d1 = convolve6_4_s32(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
      d2 = convolve6_4_s32(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
      d3 = convolve6_4_s32(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

      if (do_average) {
        load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

        compute_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                        bck_offset, sub_const_vec, round_bits,
                        use_dist_wtd_comp_avg, &d01_u8, &d23_u8);

        store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
        store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
        store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
        store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
        dst8_ptr += 4 * dst8_stride;
      } else {
        store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else
      s5 = vld1_s16(src_ptr);

      d0 = convolve6_4_s32(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

      if (do_average) {
        dd0 = vld1_u16(dst_ptr);

        compute_avg_4x1(dd0, d0, fwd_offset, bck_offset, sub_const_vec,
                        round_bits, use_dist_wtd_comp_avg, &d01_u8);

        store_u8_4x1(dst8_ptr, d01_u8, 0);
        dst8_ptr += dst8_stride;
      } else {
        vst1_u16(dst_ptr, d0);
      }

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif
    } while (h > 0);

  } else {
    int16x8_t s0, s1, s2, s3, s4, s5;
    uint16x8_t dd0, d0;
    uint8x8_t d0_u8;

#if defined(__aarch64__)
    int16x8_t s6, s7, s8;
    uint16x8_t dd1, dd2, dd3, d1, d2, d3;
    uint8x8_t d1_u8, d2_u8, d3_u8;
#endif

    do {
      int16_t *s = src_ptr;
      uint16_t *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      load_s16_8x5(s, src_stride, &s0, &s1, &s2, &s3, &s4);
      s += 5 * src_stride;

      do {
#if defined(__aarch64__)
        load_s16_8x4(s, src_stride, &s5, &s6, &s7, &s8);

        d0 = convolve6_8_s32(s0, s1, s2, s3, s4, s5, y_filter, offset_const);
        d1 = convolve6_8_s32(s1, s2, s3, s4, s5, s6, y_filter, offset_const);
        d2 = convolve6_8_s32(s2, s3, s4, s5, s6, s7, y_filter, offset_const);
        d3 = convolve6_8_s32(s3, s4, s5, s6, s7, s8, y_filter, offset_const);

        if (do_average) {
          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                          bck_offset, sub_const_vec, round_bits,
                          use_dist_wtd_comp_avg, &d0_u8, &d1_u8, &d2_u8,
                          &d3_u8);

          store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
          d_u8 += 4 * dst8_stride;
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else
        s5 = vld1q_s16(s);

        d0 = convolve6_8_s32(s0, s1, s2, s3, s4, s5, y_filter, offset_const);

        if (do_average) {
          dd0 = vld1q_u16(d);

          compute_avg_8x1(dd0, d0, fwd_offset, bck_offset, sub_const_vec,
                          round_bits, use_dist_wtd_comp_avg, &d0_u8);

          vst1_u8(d_u8, d0_u8);
          d_u8 += dst8_stride;
        } else {
          vst1q_u16(d, d0);
        }

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;

        s += src_stride;
        d += dst_stride;
        height--;
#endif
      } while (height > 0);

      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

static INLINE void dist_wtd_convolve_2d_vert_8tap_neon(
    int16_t *src_ptr, const int src_stride, uint8_t *dst8_ptr, int dst8_stride,
    ConvolveParams *conv_params, const int16x8_t y_filter, int h, int w) {
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;

  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int16_t sub_const = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                            (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));

  const int16_t round_bits =
      2 * FILTER_BITS - ROUND0_BITS - COMPOUND_ROUND1_BITS;
  const int offset = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int32x4_t offset_const = vdupq_n_s32(1 << offset);
  const int16x4_t sub_const_vec = vdup_n_s16(sub_const);
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int do_average = conv_params->do_average;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;

  if (w == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x4_t dd0, d0;
    uint8x8_t d01_u8;

#if defined(__aarch64__)
    int16x4_t s8, s9, s10;
    uint16x4_t dd1, dd2, dd3, d1, d2, d3;
    uint8x8_t d23_u8;
#endif

    load_s16_4x7(src_ptr, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
    src_ptr += 7 * src_stride;

    do {
#if defined(__aarch64__)
      load_s16_4x4(src_ptr, src_stride, &s7, &s8, &s9, &s10);

      d0 = convolve8_4_s32(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                           offset_const);
      d1 = convolve8_4_s32(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                           offset_const);
      d2 = convolve8_4_s32(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                           offset_const);
      d3 = convolve8_4_s32(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                           offset_const);

      if (do_average) {
        load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

        compute_avg_4x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                        bck_offset, sub_const_vec, round_bits,
                        use_dist_wtd_comp_avg, &d01_u8, &d23_u8);

        store_u8_4x1(dst8_ptr + 0 * dst8_stride, d01_u8, 0);
        store_u8_4x1(dst8_ptr + 1 * dst8_stride, d01_u8, 1);
        store_u8_4x1(dst8_ptr + 2 * dst8_stride, d23_u8, 0);
        store_u8_4x1(dst8_ptr + 3 * dst8_stride, d23_u8, 1);
        dst8_ptr += 4 * dst8_stride;
      } else {
        store_u16_4x4(dst_ptr, dst_stride, d0, d1, d2, d3);
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      h -= 4;
#else
      s7 = vld1_s16(src_ptr);

      d0 = convolve8_4_s32(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                           offset_const);

      if (do_average) {
        dd0 = vld1_u16(dst_ptr);

        compute_avg_4x1(dd0, d0, fwd_offset, bck_offset, sub_const_vec,
                        round_bits, use_dist_wtd_comp_avg, &d01_u8);

        store_u8_4x1(dst8_ptr, d01_u8, 0);
        dst8_ptr += dst8_stride;
      } else {
        vst1_u16(dst_ptr, d0);
      }

      s0 = s1;
      s1 = s2;
      s2 = s3;
      s3 = s4;
      s4 = s5;
      s5 = s6;
      s6 = s7;
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      h--;
#endif
    } while (h > 0);

  } else {
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t dd0, d0;
    uint8x8_t d0_u8;

#if defined(__aarch64__)
    int16x8_t s8, s9, s10;
    uint16x8_t dd1, dd2, dd3, d1, d2, d3;
    uint8x8_t d1_u8, d2_u8, d3_u8;
#endif

    do {
      int16_t *s = src_ptr;
      uint16_t *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      load_s16_8x7(s, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6);
      s += 7 * src_stride;

      do {
#if defined(__aarch64__)
        load_s16_8x4(s, src_stride, &s7, &s8, &s9, &s10);

        d0 = convolve8_8_s32(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                             offset_const);
        d1 = convolve8_8_s32(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                             offset_const);
        d2 = convolve8_8_s32(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                             offset_const);
        d3 = convolve8_8_s32(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                             offset_const);

        if (do_average) {
          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_8x4(dd0, dd1, dd2, dd3, d0, d1, d2, d3, fwd_offset,
                          bck_offset, sub_const_vec, round_bits,
                          use_dist_wtd_comp_avg, &d0_u8, &d1_u8, &d2_u8,
                          &d3_u8);

          store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
          d_u8 += 4 * dst8_stride;
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
#else
        s7 = vld1q_s16(s);

        d0 = convolve8_8_s32(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                             offset_const);

        if (do_average) {
          dd0 = vld1q_u16(d);

          compute_avg_8x1(dd0, d0, fwd_offset, bck_offset, sub_const_vec,
                          round_bits, use_dist_wtd_comp_avg, &d0_u8);

          vst1_u8(d_u8, d0_u8);
          d_u8 += dst8_stride;
        } else {
          vst1q_u16(d, d0);
        }

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        height--;
#endif
      } while (height > 0);

      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

void av1_dist_wtd_convolve_2d_neon(const uint8_t *src, int src_stride,
                                   uint8_t *dst8, int dst8_stride, int w, int h,
                                   const InterpFilterParams *filter_params_x,
                                   const InterpFilterParams *filter_params_y,
                                   const int subpel_x_qn, const int subpel_y_qn,
                                   ConvolveParams *conv_params) {
  assert(!(w % 4));
  assert(!(h % 4));

  DECLARE_ALIGNED(16, int16_t,
                  im_block[(MAX_SB_SIZE + HORIZ_EXTRA_ROWS) * MAX_SB_SIZE]);

  const int y_filter_taps = get_filter_tap(filter_params_y, subpel_y_qn);
  const int clamped_y_taps = y_filter_taps < 6 ? 6 : y_filter_taps;

  const int im_h = h + filter_params_y->taps - 1;
  const int im_stride = MAX_SB_SIZE;
  const int vert_offset = filter_params_y->taps / 2 - 1;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - vert_offset * src_stride - horiz_offset;
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);

  dist_wtd_convolve_2d_horiz_neon(src_ptr, src_stride, im_block, im_stride,
                                  x_filter, im_h, w);

  if (clamped_y_taps == 6) {
    dist_wtd_convolve_2d_vert_6tap_neon(im_block + im_stride, im_stride, dst8,
                                        dst8_stride, conv_params, y_filter, h,
                                        w);
  } else {
    dist_wtd_convolve_2d_vert_8tap_neon(im_block, im_stride, dst8, dst8_stride,
                                        conv_params, y_filter, h, w);
  }
}

void av1_dist_wtd_convolve_2d_copy_neon(const uint8_t *src, int src_stride,
                                        uint8_t *dst8, int dst8_stride, int w,
                                        int h, ConvolveParams *conv_params) {
  uint8x8_t res0_8, res1_8, res2_8, res3_8, tmp_shift0, tmp_shift1, tmp_shift2,
      tmp_shift3;
  uint16x8_t res_q0, res_q1, res_q2, res_q3, tmp_q0, tmp_q1, tmp_q2, tmp_q3;
  uint16x4_t tmp4, tmp5, tmp6, tmp7, res4, res5, res6, res7;
  const uint8_t *src1, *src2;
  uint8_t *dst8_1;
  CONV_BUF_TYPE *dst = conv_params->dst, *dst_1, *dst_2;
  const int dst_stride = conv_params->dst_stride;
  int x, y;
  const int16_t bits =
      FILTER_BITS * 2 - conv_params->round_1 - conv_params->round_0;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
  const int round_offset = (1 << (offset_bits - conv_params->round_1)) +
                           (1 << (offset_bits - conv_params->round_1 - 1));
  const int16x4_t sub_const_vec = vdup_n_s16((int16_t)round_offset);
  const uint16x8_t dup_round_offset16x8 = vdupq_n_u16((uint16_t)round_offset);
  const int16x4_t dup_bits16x4 = vdup_n_s16(bits);
  const int16x8_t dup_bits16x8 = vdupq_n_s16(bits);

  if (!(w & 0x07)) {
    for (y = 0; y < (h >> 2); ++y) {
      src1 = src;
      dst8_1 = dst8;
      dst_1 = dst;
      for (x = 0; x < (w >> 3); ++x) {
        src2 = src1;
        load_u8_8x4(src2, src_stride, &res0_8, &res1_8, &res2_8, &res3_8);

        res_q0 = vaddq_u16(vshlq_u16(vmovl_u8(res0_8), dup_bits16x8),
                           dup_round_offset16x8);
        res_q1 = vaddq_u16(vshlq_u16(vmovl_u8(res1_8), dup_bits16x8),
                           dup_round_offset16x8);
        res_q2 = vaddq_u16(vshlq_u16(vmovl_u8(res2_8), dup_bits16x8),
                           dup_round_offset16x8);
        res_q3 = vaddq_u16(vshlq_u16(vmovl_u8(res3_8), dup_bits16x8),
                           dup_round_offset16x8);

        if (conv_params->do_average) {
          dst_2 = dst_1;
          load_u16_8x4(dst_2, dst_stride, &tmp_q0, &tmp_q1, &tmp_q2, &tmp_q3);

          compute_avg_8x4(tmp_q0, tmp_q1, tmp_q2, tmp_q3, res_q0, res_q1,
                          res_q2, res_q3, conv_params->fwd_offset,
                          conv_params->bck_offset, sub_const_vec, bits,
                          conv_params->use_dist_wtd_comp_avg, &tmp_shift0,
                          &tmp_shift1, &tmp_shift2, &tmp_shift3);

          store_u8_8x4(dst8_1, dst8_stride, tmp_shift0, tmp_shift1, tmp_shift2,
                       tmp_shift3);
        } else {
          store_u16_8x4(dst_1, dst_stride, res_q0, res_q1, res_q2, res_q3);
        }
        src1 = src1 + 8;
        dst_1 = dst_1 + 8;
        dst8_1 = dst8_1 + 8;
      }
      src += src_stride * 4;
      dst8 += dst8_stride * 4;
      dst += dst_stride * 4;
    }
  } else if (!(w & 0x03)) {
    for (y = 0; y < (h >> 2); ++y) {
      src1 = src;
      dst8_1 = dst8;
      dst_1 = dst;

      load_u8_8x4(src1, src_stride, &res0_8, &res1_8, &res2_8, &res3_8);

      res4 = vadd_u16(vshl_u16(vget_low_u16(vmovl_u8(res0_8)), dup_bits16x4),
                      vreinterpret_u16_s16(sub_const_vec));
      res5 = vadd_u16(vshl_u16(vget_low_u16(vmovl_u8(res1_8)), dup_bits16x4),
                      vreinterpret_u16_s16(sub_const_vec));
      res6 = vadd_u16(vshl_u16(vget_low_u16(vmovl_u8(res2_8)), dup_bits16x4),
                      vreinterpret_u16_s16(sub_const_vec));
      res7 = vadd_u16(vshl_u16(vget_low_u16(vmovl_u8(res3_8)), dup_bits16x4),
                      vreinterpret_u16_s16(sub_const_vec));
      if (conv_params->do_average) {
        load_u16_4x4(dst_1, dst_stride, &tmp4, &tmp5, &tmp6, &tmp7);

        compute_avg_4x4(tmp4, tmp5, tmp6, tmp7, res4, res5, res6, res7,
                        conv_params->fwd_offset, conv_params->bck_offset,
                        sub_const_vec, bits, conv_params->use_dist_wtd_comp_avg,
                        &tmp_shift0, &tmp_shift1);

        store_u8_4x1(dst8_1 + 0 * dst8_stride, tmp_shift0, 0);
        store_u8_4x1(dst8_1 + 1 * dst8_stride, tmp_shift0, 1);
        store_u8_4x1(dst8_1 + 2 * dst8_stride, tmp_shift1, 0);
        store_u8_4x1(dst8_1 + 3 * dst8_stride, tmp_shift1, 1);
      } else {
        store_u16_4x4(dst_1, dst_stride, res4, res5, res6, res7);
      }
      src += src_stride * 4;
      dst += dst_stride * 4;
      dst8 += dst8_stride * 4;
    }
  }
}

#if defined(__aarch64__) && defined(__ARM_FEATURE_MATMUL_INT8)

void av1_dist_wtd_convolve_x_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_x,
                                  const int subpel_x_qn,
                                  ConvolveParams *conv_params) {
  assert(!(w % 4));
  assert(!(h % 4));

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                           (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int round_bits = FILTER_BITS - ROUND0_BITS;
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;
  const int16x4_t round_offset64 = vdup_n_s16(round_offset);
  const int16x8_t round_offset128 = vdupq_n_s16(round_offset);

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);
  // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // The outermost -1 is needed because we halved the filter values.
  const int32x4_t horiz_const = vdupq_n_s32(1 << ((ROUND0_BITS - 1) - 1));

  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst = conv_params->dst;
  CONV_BUF_TYPE *dst_ptr = dst;
  uint8_t *dst_u8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int width = w;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      int32x4_t d0, d1, d2, d3;
      int16x8_t d01, d23;
      uint16x4_t dd0, dd1, dd2, dd3;
      uint8x8_t d01_u8, d23_u8;

      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      d0 = convolve8_4_usdot(s0, x_filter, permute_tbl, horiz_const);
      d1 = convolve8_4_usdot(s1, x_filter, permute_tbl, horiz_const);
      d2 = convolve8_4_usdot(s2, x_filter, permute_tbl, horiz_const);
      d3 = convolve8_4_usdot(s3, x_filter, permute_tbl, horiz_const);

      d01 = vcombine_s16(vmovn_s32(d0), vmovn_s32(d1));
      d23 = vcombine_s16(vmovn_s32(d2), vmovn_s32(d3));

      // We halved the convolution filter values so -1 from the right shift.
      d01 = vshrq_n_s16(d01, ROUND0_BITS - 1);
      d23 = vshrq_n_s16(d23, ROUND0_BITS - 1);

      d01 = vaddq_s16(d01, round_offset128);
      d23 = vaddq_s16(d23, round_offset128);

      if (conv_params->do_average) {
        load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

        compute_avg_4x4(dd0, dd1, dd2, dd3,
                        vreinterpret_u16_s16(vget_low_s16(d01)),
                        vreinterpret_u16_s16(vget_high_s16(d01)),
                        vreinterpret_u16_s16(vget_low_s16(d23)),
                        vreinterpret_u16_s16(vget_high_s16(d23)), fwd_offset,
                        bck_offset, round_offset64, round_bits,
                        use_dist_wtd_comp_avg, &d01_u8, &d23_u8);

        store_u8_4x1(dst_u8_ptr + 0 * dst8_stride, d01_u8, 0);
        store_u8_4x1(dst_u8_ptr + 1 * dst8_stride, d01_u8, 1);
        store_u8_4x1(dst_u8_ptr + 2 * dst8_stride, d23_u8, 0);
        store_u8_4x1(dst_u8_ptr + 3 * dst8_stride, d23_u8, 1);
      } else {
        store_u16_4x4(dst_ptr, dst_stride,
                      vreinterpret_u16_s16(vget_low_s16(d01)),
                      vreinterpret_u16_s16(vget_high_s16(d01)),
                      vreinterpret_u16_s16(vget_low_s16(d23)),
                      vreinterpret_u16_s16(vget_high_s16(d23)));
      }

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst_u8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst_u8_ptr;
      width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        int16x8_t d0, d1, d2, d3;
        uint16x8_t dd0, dd1, dd2, dd3;
        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;

        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_horiz_8_usdot(s0, x_filter, permute_tbl, horiz_const);
        d1 = convolve8_horiz_8_usdot(s1, x_filter, permute_tbl, horiz_const);
        d2 = convolve8_horiz_8_usdot(s2, x_filter, permute_tbl, horiz_const);
        d3 = convolve8_horiz_8_usdot(s3, x_filter, permute_tbl, horiz_const);

        d0 = vaddq_s16(d0, round_offset128);
        d1 = vaddq_s16(d1, round_offset128);
        d2 = vaddq_s16(d2, round_offset128);
        d3 = vaddq_s16(d3, round_offset128);

        if (conv_params->do_average) {
          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_8x4(dd0, dd1, dd2, dd3, vreinterpretq_u16_s16(d0),
                          vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                          vreinterpretq_u16_s16(d3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &d0_u8, &d1_u8, &d2_u8, &d3_u8);

          store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        } else {
          store_u16_8x4(d, dst_stride, vreinterpretq_u16_s16(d0),
                        vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                        vreinterpretq_u16_s16(d3));
        }

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width > 0);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst_u8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height > 0);
  }
}

#elif defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)

void av1_dist_wtd_convolve_x_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_x,
                                  const int subpel_x_qn,
                                  ConvolveParams *conv_params) {
  assert(!(w % 4));
  assert(!(h % 4));

  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                           (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int round_bits = FILTER_BITS - ROUND0_BITS;
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;
  const int16x4_t round_offset64 = vdup_n_s16(round_offset);
  const int16x8_t round_offset128 = vdupq_n_s16(round_offset);

  // Horizontal filter.
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int8x8_t x_filter = vshrn_n_s16(vld1q_s16(x_filter_ptr), 1);
  // Dot-product constants.
  const uint8x16_t range_limit = vdupq_n_u8(128);
  const int32_t correction_s32 = vaddlvq_s16(vshll_n_s8(x_filter, 7));
  // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
  // shifts - which are generally faster than rounding shifts on modern CPUs.
  // The outermost -1 is needed because we halved the filter values.
  int32x4_t correction =
      vdupq_n_s32(correction_s32 + (1 << ((ROUND0_BITS - 1) - 1)));

  const uint8_t *src_ptr = src - horiz_offset;
  CONV_BUF_TYPE *dst = conv_params->dst;
  CONV_BUF_TYPE *dst_ptr = dst;
  uint8_t *dst_u8_ptr = dst8;
  int dst_stride = conv_params->dst_stride;
  int width = w;
  int height = h;

  if (w == 4) {
    const uint8x16x2_t permute_tbl = vld1q_u8_x2(dot_prod_permute_tbl);

    do {
      uint8x16_t s0, s1, s2, s3;
      int32x4_t d0, d1, d2, d3;
      int16x8_t d01, d23;
      uint16x4_t dd0, dd1, dd2, dd3;
      uint8x8_t d01_u8, d23_u8;

      load_u8_16x4(src_ptr, src_stride, &s0, &s1, &s2, &s3);

      d0 = convolve8_4_sdot(s0, x_filter, correction, range_limit, permute_tbl);
      d1 = convolve8_4_sdot(s1, x_filter, correction, range_limit, permute_tbl);
      d2 = convolve8_4_sdot(s2, x_filter, correction, range_limit, permute_tbl);
      d3 = convolve8_4_sdot(s3, x_filter, correction, range_limit, permute_tbl);

      d01 = vcombine_s16(vmovn_s32(d0), vmovn_s32(d1));
      d23 = vcombine_s16(vmovn_s32(d2), vmovn_s32(d3));

      // We halved the convolution filter values so -1 from the right shift.
      d01 = vshrq_n_s16(d01, ROUND0_BITS - 1);
      d23 = vshrq_n_s16(d23, ROUND0_BITS - 1);

      d01 = vaddq_s16(d01, round_offset128);
      d23 = vaddq_s16(d23, round_offset128);

      if (conv_params->do_average) {
        load_u16_4x4(dst_ptr, dst_stride, &dd0, &dd1, &dd2, &dd3);

        compute_avg_4x4(dd0, dd1, dd2, dd3,
                        vreinterpret_u16_s16(vget_low_s16(d01)),
                        vreinterpret_u16_s16(vget_high_s16(d01)),
                        vreinterpret_u16_s16(vget_low_s16(d23)),
                        vreinterpret_u16_s16(vget_high_s16(d23)), fwd_offset,
                        bck_offset, round_offset64, round_bits,
                        use_dist_wtd_comp_avg, &d01_u8, &d23_u8);

        store_u8_4x1(dst_u8_ptr + 0 * dst8_stride, d01_u8, 0);
        store_u8_4x1(dst_u8_ptr + 1 * dst8_stride, d01_u8, 1);
        store_u8_4x1(dst_u8_ptr + 2 * dst8_stride, d23_u8, 0);
        store_u8_4x1(dst_u8_ptr + 3 * dst8_stride, d23_u8, 1);
      } else {
        store_u16_4x4(dst_ptr, dst_stride,
                      vreinterpret_u16_s16(vget_low_s16(d01)),
                      vreinterpret_u16_s16(vget_high_s16(d01)),
                      vreinterpret_u16_s16(vget_low_s16(d23)),
                      vreinterpret_u16_s16(vget_high_s16(d23)));
      }

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst_u8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height > 0);
  } else {
    const uint8x16x3_t permute_tbl = vld1q_u8_x3(dot_prod_permute_tbl);

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst_u8_ptr;
      width = w;

      do {
        uint8x16_t s0, s1, s2, s3;
        int16x8_t d0, d1, d2, d3;
        uint16x8_t dd0, dd1, dd2, dd3;
        uint8x8_t d0_u8, d1_u8, d2_u8, d3_u8;

        load_u8_16x4(s, src_stride, &s0, &s1, &s2, &s3);

        d0 = convolve8_horiz_8_sdot(s0, x_filter, correction, range_limit,
                                    permute_tbl);
        d1 = convolve8_horiz_8_sdot(s1, x_filter, correction, range_limit,
                                    permute_tbl);
        d2 = convolve8_horiz_8_sdot(s2, x_filter, correction, range_limit,
                                    permute_tbl);
        d3 = convolve8_horiz_8_sdot(s3, x_filter, correction, range_limit,
                                    permute_tbl);

        // We halved the convolution filter values so -1 from the right shift.
        d0 = vshrq_n_s16(d0, ROUND0_BITS - 1);
        d1 = vshrq_n_s16(d1, ROUND0_BITS - 1);
        d2 = vshrq_n_s16(d2, ROUND0_BITS - 1);
        d3 = vshrq_n_s16(d3, ROUND0_BITS - 1);

        d0 = vaddq_s16(d0, round_offset128);
        d1 = vaddq_s16(d1, round_offset128);
        d2 = vaddq_s16(d2, round_offset128);
        d3 = vaddq_s16(d3, round_offset128);

        if (conv_params->do_average) {
          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_8x4(dd0, dd1, dd2, dd3, vreinterpretq_u16_s16(d0),
                          vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                          vreinterpretq_u16_s16(d3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &d0_u8, &d1_u8, &d2_u8, &d3_u8);

          store_u8_8x4(d_u8, dst8_stride, d0_u8, d1_u8, d2_u8, d3_u8);
        } else {
          store_u16_8x4(d, dst_stride, vreinterpretq_u16_s16(d0),
                        vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                        vreinterpretq_u16_s16(d3));
        }

        s += 8;
        d += 8;
        d_u8 += 8;
        width -= 8;
      } while (width > 0);

      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst_u8_ptr += 4 * dst8_stride;
      height -= 4;
    } while (height > 0);
  }
}

#else  // !(defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD))

static INLINE int16x4_t
convolve8_x_4x4_s16(const int16x4_t s0, const int16x4_t s1, const int16x4_t s2,
                    const int16x4_t s3, const int16x4_t s4, const int16x4_t s5,
                    const int16x4_t s6, const int16x4_t s7,
                    const int16x8_t filter, const int16x4_t horiz_const) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x4_t sum = horiz_const;

  sum = vmla_lane_s16(sum, s0, filter_lo, 0);
  sum = vmla_lane_s16(sum, s1, filter_lo, 1);
  sum = vmla_lane_s16(sum, s2, filter_lo, 2);
  sum = vmla_lane_s16(sum, s3, filter_lo, 3);
  sum = vmla_lane_s16(sum, s4, filter_hi, 0);
  sum = vmla_lane_s16(sum, s5, filter_hi, 1);
  sum = vmla_lane_s16(sum, s6, filter_hi, 2);
  sum = vmla_lane_s16(sum, s7, filter_hi, 3);

  return sum;
}

static INLINE int16x8_t
convolve8_x_8x8_s16(const int16x8_t s0, const int16x8_t s1, const int16x8_t s2,
                    const int16x8_t s3, const int16x8_t s4, const int16x8_t s5,
                    const int16x8_t s6, const int16x8_t s7,
                    const int16x8_t filter, const int16x8_t horiz_const) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x8_t sum = horiz_const;

  sum = vmlaq_lane_s16(sum, s0, filter_lo, 0);
  sum = vmlaq_lane_s16(sum, s1, filter_lo, 1);
  sum = vmlaq_lane_s16(sum, s2, filter_lo, 2);
  sum = vmlaq_lane_s16(sum, s3, filter_lo, 3);
  sum = vmlaq_lane_s16(sum, s4, filter_hi, 0);
  sum = vmlaq_lane_s16(sum, s5, filter_hi, 1);
  sum = vmlaq_lane_s16(sum, s6, filter_hi, 2);
  sum = vmlaq_lane_s16(sum, s7, filter_hi, 3);

  return sum;
}

void av1_dist_wtd_convolve_x_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_x,
                                  const int subpel_x_qn,
                                  ConvolveParams *conv_params) {
  assert(!(w % 4));
  assert(!(h % 4));

  CONV_BUF_TYPE *dst = conv_params->dst;
  int dst_stride = conv_params->dst_stride;
  const int horiz_offset = filter_params_x->taps / 2 - 1;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                           (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int round_bits = FILTER_BITS - ROUND0_BITS;
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;

  // horizontal filter
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);

  const uint8_t *src_ptr = src - horiz_offset;

  // Filter values are even, so downshift by 1 to reduce intermediate precision
  // requirements.
  const int16x8_t x_filter = vshrq_n_s16(vld1q_s16(x_filter_ptr), 1);

  const uint8_t *s;
  uint8_t *d_u8;
  uint8_t *dst_u8_ptr;
  CONV_BUF_TYPE *d, *dst_ptr;
  int width, height;
  uint8x8_t t0;
#if defined(__aarch64__)
  uint8x8_t t1, t2, t3, t4, t5, t6, t7;
#endif
  s = src_ptr;
  dst_ptr = dst;
  dst_u8_ptr = dst8;
  width = w;
  height = h;

  if ((w == 4) || (h == 4)) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, d0;
    int16x8_t tt0;
    uint16x4_t res4;
#if defined(__aarch64__)
    const int16x8_t round_offset_vec = vdupq_n_s16(round_offset);
    int16x4_t s8, s9, s10, d1, d2, d3;
    int16x8_t tt1, tt2, tt3, t01, t23;
    uint16x4_t res5, res6, res7;
    int16x8_t u0, u1;
#else
    const int16x4_t round_offset_vec = vdup_n_s16(round_offset);
    int16x4_t temp_0;
#endif
    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x4_t horiz_const = vdup_n_s16(1 << ((ROUND0_BITS - 1) - 1));

    do {
      s = src_ptr;
      d = dst_ptr;
      d_u8 = dst_u8_ptr;
      width = w;
      __builtin_prefetch(s + 0 * src_stride);
#if defined(__aarch64__)
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      load_u8_8x4(s, src_stride, &t0, &t1, &t2, &t3);
      transpose_u8_8x4(&t0, &t1, &t2, &t3);
      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      tt2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      tt3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s0 = vget_low_s16(tt0);
      s1 = vget_low_s16(tt1);
      s2 = vget_low_s16(tt2);
      s3 = vget_low_s16(tt3);
      s4 = vget_high_s16(tt0);
      s5 = vget_high_s16(tt1);
      s6 = vget_high_s16(tt2);
      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);
      s += 7;
      do {
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);

        transpose_u8_4x4(&t0, &t1);
        u0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        u1 = vreinterpretq_s16_u16(vmovl_u8(t1));

        s7 = vget_low_s16(u0);
        s8 = vget_low_s16(u1);
        s9 = vget_high_s16(u0);
        s10 = vget_high_s16(u1);

        d0 = convolve8_x_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                 horiz_const);
        d1 = convolve8_x_4x4_s16(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                 horiz_const);
        d2 = convolve8_x_4x4_s16(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                 horiz_const);
        d3 = convolve8_x_4x4_s16(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                 horiz_const);

        t01 = vcombine_s16(d0, d1);
        t23 = vcombine_s16(d2, d3);

        // We halved the convolution filter values so -1 from the right shift.
        t01 = vshrq_n_s16(t01, ROUND0_BITS - 1);
        t23 = vshrq_n_s16(t23, ROUND0_BITS - 1);

        t01 = vaddq_s16(t01, round_offset_vec);
        t23 = vaddq_s16(t23, round_offset_vec);

        d0 = vget_low_s16(t01);
        d1 = vget_high_s16(t01);
        d2 = vget_low_s16(t23);
        d3 = vget_high_s16(t23);

        transpose_s16_4x4d(&d0, &d1, &d2, &d3);

        if (conv_params->do_average) {
          __builtin_prefetch(d + 0 * dst_stride);
          __builtin_prefetch(d + 1 * dst_stride);
          __builtin_prefetch(d + 2 * dst_stride);
          __builtin_prefetch(d + 3 * dst_stride);

          __builtin_prefetch(d_u8 + 0 * dst8_stride);
          __builtin_prefetch(d_u8 + 1 * dst8_stride);
          __builtin_prefetch(d_u8 + 2 * dst8_stride);
          __builtin_prefetch(d_u8 + 3 * dst8_stride);

          load_u16_4x4(d, dst_stride, &res4, &res5, &res6, &res7);

          compute_avg_4x4(res4, res5, res6, res7, vreinterpret_u16_s16(d0),
                          vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                          vreinterpret_u16_s16(d3), fwd_offset, bck_offset,
                          vget_low_s16(round_offset_vec), round_bits,
                          use_dist_wtd_comp_avg, &t0, &t1);

          store_u8_4x1(d_u8 + 0 * dst8_stride, t0, 0);
          store_u8_4x1(d_u8 + 1 * dst8_stride, t0, 1);
          store_u8_4x1(d_u8 + 2 * dst8_stride, t1, 0);
          store_u8_4x1(d_u8 + 3 * dst8_stride, t1, 1);
        } else {
          store_u16_4x4(d, dst_stride, vreinterpret_u16_s16(d0),
                        vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                        vreinterpret_u16_s16(d3));
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width > 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      dst_u8_ptr += 4 * dst8_stride;
      height -= 4;
#else
      t0 = vld1_u8(s);                            // a0 a1 a2 a3 a4 a5 a6 a7
      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7
      s0 = vget_low_s16(tt0);                     // a0 a1 a2 a3
      s4 = vget_high_s16(tt0);                    // a4 a5 a6 a7
      __builtin_prefetch(d);

      s += 8;
      do {
        t0 = vld1_u8(s);  // a8 a9 a10 a11

        // a8 a9 a10 a11
        s7 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(t0)));
        temp_0 = s7;
        s1 = vext_s16(s0, s4, 1);  // a1 a2 a3 a4
        s2 = vext_s16(s0, s4, 2);  // a2 a3 a4 a5
        s3 = vext_s16(s0, s4, 3);  // a3 a4 a5 a6
        s5 = vext_s16(s4, s7, 1);  // a5 a6 a7 a8
        s6 = vext_s16(s4, s7, 2);  // a6 a7 a8 a9
        s7 = vext_s16(s4, s7, 3);  // a7 a8 a9 a10

        d0 = convolve8_x_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                 horiz_const);
        // We halved the convolution filter values so -1 from the right shift.
        d0 = vshr_n_s16(d0, ROUND0_BITS - 1);
        d0 = vadd_s16(d0, round_offset_vec);
        s0 = s4;
        s4 = temp_0;
        if (conv_params->do_average) {
          __builtin_prefetch(d);
          __builtin_prefetch(d_u8);

          res4 = vld1_u16(d);

          compute_avg_4x1(res4, vreinterpret_u16_s16(d0), fwd_offset,
                          bck_offset, round_offset_vec, round_bits,
                          use_dist_wtd_comp_avg, &t0);

          store_u8_4x1(d_u8, t0, 0);
        } else {
          vst1_u16(d, vreinterpret_u16_s16(d0));
        }

        s += 4;
        d += 4;
        d_u8 += 4;
        width -= 4;
      } while (width > 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst_u8_ptr += dst8_stride;
      height--;
#endif
    } while (height > 0);
  } else {
    CONV_BUF_TYPE *d_tmp;
    uint8_t *d_u8_tmp;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    int16x8_t res0;
    uint16x8_t res8;
    const int16x8_t round_offset128 = vdupq_n_s16(round_offset);
    const int16x4_t round_offset64 = vdup_n_s16(round_offset);
    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x8_t horiz_const = vdupq_n_s16(1 << ((ROUND0_BITS - 1) - 1));

    d = dst_ptr = dst;
    d_u8 = dst_u8_ptr = dst8;
    do {
#if defined(__aarch64__)
      int16x8_t s11, s12, s13, s14;
      int16x8_t s8, s9, s10;
      int16x8_t res1, res2, res3, res4, res5, res6, res7;
      uint16x8_t res9, res10, res11;
      __builtin_prefetch(src_ptr + 0 * src_stride);
      __builtin_prefetch(src_ptr + 1 * src_stride);
      __builtin_prefetch(src_ptr + 2 * src_stride);
      __builtin_prefetch(src_ptr + 3 * src_stride);
      __builtin_prefetch(src_ptr + 4 * src_stride);
      __builtin_prefetch(src_ptr + 5 * src_stride);
      __builtin_prefetch(src_ptr + 6 * src_stride);
      __builtin_prefetch(src_ptr + 7 * src_stride);
      load_u8_8x8(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      width = w;
      s = src_ptr + 7;
      d = dst_ptr;
      d_u8_tmp = dst_u8_ptr;

      __builtin_prefetch(dst_ptr + 0 * dst_stride);
      __builtin_prefetch(dst_ptr + 1 * dst_stride);
      __builtin_prefetch(dst_ptr + 2 * dst_stride);
      __builtin_prefetch(dst_ptr + 3 * dst_stride);
      __builtin_prefetch(dst_ptr + 4 * dst_stride);
      __builtin_prefetch(dst_ptr + 5 * dst_stride);
      __builtin_prefetch(dst_ptr + 6 * dst_stride);
      __builtin_prefetch(dst_ptr + 7 * dst_stride);

      do {
        d_u8 = d_u8_tmp;
        d_tmp = d;

        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        transpose_u8_8x8(&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        res0 = convolve8_x_8x8_s16(s0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                   horiz_const);
        res1 = convolve8_x_8x8_s16(s1, s2, s3, s4, s5, s6, s7, s8, x_filter,
                                   horiz_const);
        res2 = convolve8_x_8x8_s16(s2, s3, s4, s5, s6, s7, s8, s9, x_filter,
                                   horiz_const);
        res3 = convolve8_x_8x8_s16(s3, s4, s5, s6, s7, s8, s9, s10, x_filter,
                                   horiz_const);
        res4 = convolve8_x_8x8_s16(s4, s5, s6, s7, s8, s9, s10, s11, x_filter,
                                   horiz_const);
        res5 = convolve8_x_8x8_s16(s5, s6, s7, s8, s9, s10, s11, s12, x_filter,
                                   horiz_const);
        res6 = convolve8_x_8x8_s16(s6, s7, s8, s9, s10, s11, s12, s13, x_filter,
                                   horiz_const);
        res7 = convolve8_x_8x8_s16(s7, s8, s9, s10, s11, s12, s13, s14,
                                   x_filter, horiz_const);

        // We halved the convolution filter values so -1 from the right shift.
        res0 = vshrq_n_s16(res0, ROUND0_BITS - 1);
        res1 = vshrq_n_s16(res1, ROUND0_BITS - 1);
        res2 = vshrq_n_s16(res2, ROUND0_BITS - 1);
        res3 = vshrq_n_s16(res3, ROUND0_BITS - 1);
        res4 = vshrq_n_s16(res4, ROUND0_BITS - 1);
        res5 = vshrq_n_s16(res5, ROUND0_BITS - 1);
        res6 = vshrq_n_s16(res6, ROUND0_BITS - 1);
        res7 = vshrq_n_s16(res7, ROUND0_BITS - 1);

        res0 = vaddq_s16(res0, round_offset128);
        res1 = vaddq_s16(res1, round_offset128);
        res2 = vaddq_s16(res2, round_offset128);
        res3 = vaddq_s16(res3, round_offset128);
        res4 = vaddq_s16(res4, round_offset128);
        res5 = vaddq_s16(res5, round_offset128);
        res6 = vaddq_s16(res6, round_offset128);
        res7 = vaddq_s16(res7, round_offset128);

        transpose_s16_8x8(&res0, &res1, &res2, &res3, &res4, &res5, &res6,
                          &res7);

        if (conv_params->do_average) {
          load_u16_8x4(d_tmp, dst_stride, &res8, &res9, &res10, &res11);
          d_tmp += 4 * dst_stride;

          compute_avg_8x4(res8, res9, res10, res11, vreinterpretq_u16_s16(res0),
                          vreinterpretq_u16_s16(res1),
                          vreinterpretq_u16_s16(res2),
                          vreinterpretq_u16_s16(res3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;

          load_u16_8x4(d_tmp, dst_stride, &res8, &res9, &res10, &res11);
          d_tmp += 4 * dst_stride;

          compute_avg_8x4(res8, res9, res10, res11, vreinterpretq_u16_s16(res4),
                          vreinterpretq_u16_s16(res5),
                          vreinterpretq_u16_s16(res6),
                          vreinterpretq_u16_s16(res7), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;
        } else {
          store_u16_8x8(
              d_tmp, dst_stride, vreinterpretq_u16_s16(res0),
              vreinterpretq_u16_s16(res1), vreinterpretq_u16_s16(res2),
              vreinterpretq_u16_s16(res3), vreinterpretq_u16_s16(res4),
              vreinterpretq_u16_s16(res5), vreinterpretq_u16_s16(res6),
              vreinterpretq_u16_s16(res7));
          d_tmp += 8 * dst_stride;
        }

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8;
        d += 8;
        d_u8_tmp += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 8 * src_stride;
      dst_ptr += 8 * dst_stride;
      dst_u8_ptr += 8 * dst8_stride;
      height -= 8;
#else
      int16x8_t temp_0;
      __builtin_prefetch(src_ptr);
      t0 = vld1_u8(src_ptr);
      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));  // a0 a1 a2 a3 a4 a5 a6 a7

      width = w;
      s = src_ptr + 8;
      d = dst_ptr;
      d_u8_tmp = dst_u8_ptr;

      __builtin_prefetch(dst_ptr);

      do {
        d_u8 = d_u8_tmp;
        d_tmp = d;

        t0 = vld1_u8(s);  // a8 a9 a10 a11 a12 a13 a14 a15
        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        temp_0 = s0;
        s0 = s7;

        s1 = vextq_s16(temp_0, s7, 1);  // a1 a2 a3 a4 a5 a6 a7 a8
        s2 = vextq_s16(temp_0, s7, 2);  // a2 a3 a4 a5 a6 a7 a8 a9
        s3 = vextq_s16(temp_0, s7, 3);  // a3 a4 a5 a6 a7 a8 a9 a10
        s4 = vextq_s16(temp_0, s7, 4);  // a4 a5 a6 a7 a8 a9 a10 a11
        s5 = vextq_s16(temp_0, s7, 5);  // a5 a6 a7 a8 a9 a10 a11 a12
        s6 = vextq_s16(temp_0, s7, 6);  // a6 a7 a8 a9 a10 a11 a12 a13
        s7 = vextq_s16(temp_0, s7, 7);  // a7 a8 a9 a10 a11 a12 a13 a14

        res0 = convolve8_x_8x8_s16(temp_0, s1, s2, s3, s4, s5, s6, s7, x_filter,
                                   horiz_const);
        // We halved the convolution filter values so -1 from the right shift.
        res0 = vshrq_n_s16(res0, ROUND0_BITS - 1);
        res0 = vaddq_s16(res0, round_offset128);

        if (conv_params->do_average) {
          res8 = vld1q_u16(d_tmp);
          d_tmp += dst_stride;

          compute_avg_8x1(res8, vreinterpretq_u16_s16(res0), fwd_offset,
                          bck_offset, round_offset64, round_bits,
                          use_dist_wtd_comp_avg, &t0);

          vst1_u8(d_u8, t0);
          d_u8 += dst8_stride;
        } else {
          vst1q_u16(d_tmp, vreinterpretq_u16_s16(res0));
          d_tmp += dst_stride;
        }

        s += 8;
        d += 8;
        width -= 8;
        d_u8_tmp += 8;
      } while (width > 0);
      src_ptr += src_stride;
      dst_ptr += dst_stride;
      dst_u8_ptr += dst8_stride;
      height--;
#endif
    } while (height > 0);
  }
}

#endif  // defined(__aarch64__) && defined(__ARM_FEATURE_DOTPROD)

static INLINE int16x4_t
convolve6_y_4x4_s16(const int16x4_t s0, const int16x4_t s1, const int16x4_t s2,
                    const int16x4_t s3, const int16x4_t s4, const int16x4_t s5,
                    const int16x8_t y_filter_0_7, const int16x4_t vert_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter_0_7);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter_0_7);
  int16x4_t sum = vert_const;

  // Filter values at indices 0 and 7 are 0.
  sum = vmla_lane_s16(sum, s0, y_filter_0_3, 1);
  sum = vmla_lane_s16(sum, s1, y_filter_0_3, 2);
  sum = vmla_lane_s16(sum, s2, y_filter_0_3, 3);
  sum = vmla_lane_s16(sum, s3, y_filter_4_7, 0);
  sum = vmla_lane_s16(sum, s4, y_filter_4_7, 1);
  sum = vmla_lane_s16(sum, s5, y_filter_4_7, 2);

  // We halved the convolution filter values so -1 from the right shift.
  return vshr_n_s16(sum, ROUND0_BITS - 1);
}

static INLINE int16x8_t
convolve6_y_8x4_s16(const int16x8_t s0, const int16x8_t s1, const int16x8_t s2,
                    const int16x8_t s3, const int16x8_t s4, const int16x8_t s5,
                    const int16x8_t y_filter_0_7, const int16x8_t vert_const) {
  const int16x4_t y_filter_0_3 = vget_low_s16(y_filter_0_7);
  const int16x4_t y_filter_4_7 = vget_high_s16(y_filter_0_7);
  int16x8_t sum = vert_const;

  // Filter values at indices 0 and 7 are 0.
  sum = vmlaq_lane_s16(sum, s0, y_filter_0_3, 1);
  sum = vmlaq_lane_s16(sum, s1, y_filter_0_3, 2);
  sum = vmlaq_lane_s16(sum, s2, y_filter_0_3, 3);
  sum = vmlaq_lane_s16(sum, s3, y_filter_4_7, 0);
  sum = vmlaq_lane_s16(sum, s4, y_filter_4_7, 1);
  sum = vmlaq_lane_s16(sum, s5, y_filter_4_7, 2);

  // We halved the convolution filter values so -1 from the right shift.
  return vshrq_n_s16(sum, ROUND0_BITS - 1);
}

static INLINE int16x4_t
convolve8_y_4x4_s16(const int16x4_t s0, const int16x4_t s1, const int16x4_t s2,
                    const int16x4_t s3, const int16x4_t s4, const int16x4_t s5,
                    const int16x4_t s6, const int16x4_t s7,
                    const int16x8_t filter, const int16x4_t vert_const) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x4_t sum = vert_const;

  sum = vmla_lane_s16(sum, s0, filter_lo, 0);
  sum = vmla_lane_s16(sum, s1, filter_lo, 1);
  sum = vmla_lane_s16(sum, s2, filter_lo, 2);
  sum = vmla_lane_s16(sum, s3, filter_lo, 3);
  sum = vmla_lane_s16(sum, s4, filter_hi, 0);
  sum = vmla_lane_s16(sum, s5, filter_hi, 1);
  sum = vmla_lane_s16(sum, s6, filter_hi, 2);
  sum = vmla_lane_s16(sum, s7, filter_hi, 3);

  return sum;
}

static INLINE int16x8_t
convolve8_y_8x8_s16(const int16x8_t s0, const int16x8_t s1, const int16x8_t s2,
                    const int16x8_t s3, const int16x8_t s4, const int16x8_t s5,
                    const int16x8_t s6, const int16x8_t s7,
                    const int16x8_t filter, const int16x8_t vert_const) {
  const int16x4_t filter_lo = vget_low_s16(filter);
  const int16x4_t filter_hi = vget_high_s16(filter);
  int16x8_t sum = vert_const;

  sum = vmlaq_lane_s16(sum, s0, filter_lo, 0);
  sum = vmlaq_lane_s16(sum, s1, filter_lo, 1);
  sum = vmlaq_lane_s16(sum, s2, filter_lo, 2);
  sum = vmlaq_lane_s16(sum, s3, filter_lo, 3);
  sum = vmlaq_lane_s16(sum, s4, filter_hi, 0);
  sum = vmlaq_lane_s16(sum, s5, filter_hi, 1);
  sum = vmlaq_lane_s16(sum, s6, filter_hi, 2);
  sum = vmlaq_lane_s16(sum, s7, filter_hi, 3);

  // We halved the convolution filter values so -1 from the right shift.
  return vshrq_n_s16(sum, ROUND0_BITS - 1);
}

void dist_wtd_convolve_y_6tap_neon(const uint8_t *src_ptr, int src_stride,
                                   uint8_t *dst8_ptr, const int dst8_stride,
                                   int w, int h, const int16x8_t y_filter,
                                   ConvolveParams *conv_params) {
  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                           (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int round_bits = FILTER_BITS - ROUND0_BITS;
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;

  if (w <= 4 || h == 4) {
    int16x4_t s0, s1, s2, s3, s4, s5, d0;
    uint16x4_t dd0;
    uint8x8_t t0 = vdup_n_u8(0);
    uint8x8_t t1 = vdup_n_u8(0);
    uint8x8_t t2 = vdup_n_u8(0);
    int16x8_t tt0, tt1, tt2;
    uint8x8_t d01;
#if defined(__aarch64__)
    int16x4_t s6, s7, s8, d1, d2, d3;
    uint16x4_t dd1, dd2, dd3;
    uint8x8_t d23;
#endif

    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x4_t vert_const = vdup_n_s16(1 << ((ROUND0_BITS - 1) - 1));
    const int16x4_t round_offset64 = vdup_n_s16(round_offset);
    int width = w;

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      load_unaligned_u8_3x8(s, src_stride, &t0, &t1, &t2);

      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      tt2 = vreinterpretq_s16_u16(vmovl_u8(t2));

      s0 = vget_low_s16(tt0);
      s1 = vget_high_s16(tt0);
      s2 = vget_low_s16(tt1);
      s3 = vget_high_s16(tt1);
      s4 = vget_low_s16(tt2);

      s += 5 * src_stride;
      do {
#if defined(__aarch64__)
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);

        tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));

        s5 = vget_low_s16(tt0);
        s6 = vget_high_s16(tt0);
        s7 = vget_low_s16(tt1);
        s8 = vget_high_s16(tt1);

        d0 = convolve6_y_4x4_s16(s0, s1, s2, s3, s4, s5, y_filter, vert_const);
        d1 = convolve6_y_4x4_s16(s1, s2, s3, s4, s5, s6, y_filter, vert_const);
        d2 = convolve6_y_4x4_s16(s2, s3, s4, s5, s6, s7, y_filter, vert_const);
        d3 = convolve6_y_4x4_s16(s3, s4, s5, s6, s7, s8, y_filter, vert_const);

        d0 = vadd_s16(d0, round_offset64);
        d1 = vadd_s16(d1, round_offset64);
        d2 = vadd_s16(d2, round_offset64);
        d3 = vadd_s16(d3, round_offset64);

        if (conv_params->do_average) {
          load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_4x4(dd0, dd1, dd2, dd3, vreinterpret_u16_s16(d0),
                          vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                          vreinterpret_u16_s16(d3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &d01, &d23);

          store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
          store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
          store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
          store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);
        } else {
          store_u16_4x4(d, dst_stride, vreinterpret_u16_s16(d0),
                        vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                        vreinterpret_u16_s16(d3));
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else   // !defined(__aarch64__)
        t0 = load_unaligned_u8_4x1(s);
        tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s5 = vget_low_s16(tt0);

        d0 = convolve6_y_4x4_s16(s0, s1, s2, s3, s4, s5, y_filter, vert_const);
        d0 = vadd_s16(d0, round_offset64);

        if (conv_params->do_average) {
          dd0 = vld1_u16(d);

          compute_avg_4x1(dd0, vreinterpret_u16_s16(d0), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &d01);

          store_u8_4x1(d_u8, d01, 0);
        } else {
          vst1_u16(d, vreinterpret_u16_s16(d0));
        }

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif  // defined(__aarch64__)
      } while (height > 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8_ptr += 4;
      width -= 4;
    } while (width > 0);
  } else {
    int16x8_t s0, s1, s2, s3, s4, s5, d0;
    uint16x8_t d8;
    uint8x8_t t0, t1, t2, t3, t4;

    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x8_t vert_const = vdupq_n_s16(1 << ((ROUND0_BITS - 1) - 1));
    const int16x8_t round_offset128 = vdupq_n_s16(round_offset);
    const int16x4_t round_offset64 = vdup_n_s16(round_offset);
#if defined(__aarch64__)
    int16x8_t s6, s7, s8, s9, s10, s11, s12, d1, d2, d3, d4, d5, d6, d7;
    uint16x8_t d9, d10, d11;
    uint8x8_t t5, t6, t7;
#endif
    int width = w;

    do {
      const uint8_t *s = src_ptr + (5 * src_stride);
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8_ptr;
      int height = h;

      load_u8_8x5(src_ptr, src_stride, &t0, &t1, &t2, &t3, &t4);

      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s4 = vreinterpretq_s16_u16(vmovl_u8(t4));

      do {
#if defined(__aarch64__)
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        s5 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s6 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s7 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s11 = vreinterpretq_s16_u16(vmovl_u8(t6));
        s12 = vreinterpretq_s16_u16(vmovl_u8(t7));

        d0 = convolve6_y_8x4_s16(s0, s1, s2, s3, s4, s5, y_filter, vert_const);
        d1 = convolve6_y_8x4_s16(s1, s2, s3, s4, s5, s6, y_filter, vert_const);
        d2 = convolve6_y_8x4_s16(s2, s3, s4, s5, s6, s7, y_filter, vert_const);
        d3 = convolve6_y_8x4_s16(s3, s4, s5, s6, s7, s8, y_filter, vert_const);
        d4 = convolve6_y_8x4_s16(s4, s5, s6, s7, s8, s9, y_filter, vert_const);
        d5 = convolve6_y_8x4_s16(s5, s6, s7, s8, s9, s10, y_filter, vert_const);
        d6 =
            convolve6_y_8x4_s16(s6, s7, s8, s9, s10, s11, y_filter, vert_const);
        d7 = convolve6_y_8x4_s16(s7, s8, s9, s10, s11, s12, y_filter,
                                 vert_const);

        d0 = vaddq_s16(d0, round_offset128);
        d1 = vaddq_s16(d1, round_offset128);
        d2 = vaddq_s16(d2, round_offset128);
        d3 = vaddq_s16(d3, round_offset128);
        d4 = vaddq_s16(d4, round_offset128);
        d5 = vaddq_s16(d5, round_offset128);
        d6 = vaddq_s16(d6, round_offset128);
        d7 = vaddq_s16(d7, round_offset128);

        if (conv_params->do_average) {
          load_u16_8x4(d, dst_stride, &d8, &d9, &d10, &d11);
          d += 4 * dst_stride;

          compute_avg_8x4(d8, d9, d10, d11, vreinterpretq_u16_s16(d0),
                          vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                          vreinterpretq_u16_s16(d3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;

          load_u16_8x4(d, dst_stride, &d8, &d9, &d10, &d11);
          d += 4 * dst_stride;

          compute_avg_8x4(d8, d9, d10, d11, vreinterpretq_u16_s16(d4),
                          vreinterpretq_u16_s16(d5), vreinterpretq_u16_s16(d6),
                          vreinterpretq_u16_s16(d7), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;
        } else {
          store_u16_8x8(d, dst_stride, vreinterpretq_u16_s16(d0),
                        vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                        vreinterpretq_u16_s16(d3), vreinterpretq_u16_s16(d4),
                        vreinterpretq_u16_s16(d5), vreinterpretq_u16_s16(d6),
                        vreinterpretq_u16_s16(d7));
          d += 8 * dst_stride;
        }

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s += 8 * src_stride;
        height -= 8;
#else   // !defined(__aarch64__)
        s5 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        d0 = convolve6_y_8x4_s16(s0, s1, s2, s3, s4, s5, y_filter, vert_const);
        d0 = vaddq_s16(d0, round_offset128);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;

        if (conv_params->do_average) {
          d8 = vld1q_u16(d);
          d += dst_stride;

          compute_avg_8x1(d8, vreinterpretq_u16_s16(d0), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0);

          vst1_u8(d_u8, t0);
          d_u8 += dst8_stride;
        } else {
          vst1q_u16(d, vreinterpretq_u16_s16(d0));
          d += dst_stride;
        }

        s += src_stride;
        height--;
#endif  // defined(__aarch64__)
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8_ptr += 8;
      width -= 8;
    } while (width > 0);
  }
}

void av1_dist_wtd_convolve_y_neon(const uint8_t *src, int src_stride,
                                  uint8_t *dst8, int dst8_stride, int w, int h,
                                  const InterpFilterParams *filter_params_y,
                                  const int subpel_y_qn,
                                  ConvolveParams *conv_params) {
  assert(!(w % 4));
  assert(!(h % 4));

  // vertical filter
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);
  // Filter values are even, so downshift by 1 to reduce intermediate
  // precision requirements.
  const int16x8_t y_filter = vshrq_n_s16(vld1q_s16(y_filter_ptr), 1);

  const int vert_offset = filter_params_y->taps / 2 - 1;
  const uint8_t *src_ptr = src - (vert_offset * src_stride);

  if (get_filter_tap(filter_params_y, subpel_y_qn) <= 6) {
    dist_wtd_convolve_y_6tap_neon(src_ptr + src_stride, src_stride, dst8,
                                  dst8_stride, w, h, y_filter, conv_params);
    return;
  }

  CONV_BUF_TYPE *dst_ptr = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  const int bd = 8;
  const int offset_bits = bd + 2 * FILTER_BITS - ROUND0_BITS;
  const int round_offset = (1 << (offset_bits - COMPOUND_ROUND1_BITS)) +
                           (1 << (offset_bits - COMPOUND_ROUND1_BITS - 1));
  const int round_bits = FILTER_BITS - ROUND0_BITS;
  const uint16_t fwd_offset = conv_params->fwd_offset;
  const uint16_t bck_offset = conv_params->bck_offset;
  const int use_dist_wtd_comp_avg = conv_params->use_dist_wtd_comp_avg;

  if ((w == 4) || (h == 4)) {
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, d0;
    uint16x4_t dd0;
    uint8x8_t t0 = vdup_n_u8(0);
    uint8x8_t t1 = vdup_n_u8(0);
    uint8x8_t t2 = vdup_n_u8(0);
    uint8x8_t t3 = vdup_n_u8(0);
    int16x8_t tt0, tt1, tt2, tt3;
    uint8x8_t d01;

#if defined(__aarch64__)
    const int16x8_t round_offset64 = vdupq_n_s16(round_offset);
    int16x4_t s8, s9, s10, d1, d2, d3;
    uint16x4_t dd1, dd2, dd3;
    int16x8_t t01, t23;
    uint8x8_t d23;
#else
    const int16x4_t round_offset64 = vdup_n_s16(round_offset);
#endif
    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x4_t vert_const = vdup_n_s16(1 << ((ROUND0_BITS - 1) - 1));
    int width = w;

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);

      load_unaligned_u8_4x8(s, src_stride, &t0, &t1, &t2, &t3);

      tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      tt2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      tt3 = vreinterpretq_s16_u16(vmovl_u8(t3));

      s0 = vget_low_s16(tt0);
      s1 = vget_high_s16(tt0);
      s2 = vget_low_s16(tt1);
      s3 = vget_high_s16(tt1);
      s4 = vget_low_s16(tt2);
      s5 = vget_high_s16(tt2);
      s6 = vget_low_s16(tt3);

      __builtin_prefetch(d + 0 * dst_stride);
      __builtin_prefetch(d + 1 * dst_stride);
      __builtin_prefetch(d + 2 * dst_stride);
      __builtin_prefetch(d + 3 * dst_stride);

      s += 7 * src_stride;
      do {
#if defined(__aarch64__)
        load_unaligned_u8_4x4(s, src_stride, &t0, &t1);

        tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        tt1 = vreinterpretq_s16_u16(vmovl_u8(t1));

        s7 = vget_low_s16(tt0);
        s8 = vget_high_s16(tt0);
        s9 = vget_low_s16(tt1);
        s10 = vget_high_s16(tt1);

        d0 = convolve8_y_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                 vert_const);
        d1 = convolve8_y_4x4_s16(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                 vert_const);
        d2 = convolve8_y_4x4_s16(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                 vert_const);
        d3 = convolve8_y_4x4_s16(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                 vert_const);

        t01 = vcombine_s16(d0, d1);
        t23 = vcombine_s16(d2, d3);

        // We halved the convolution filter values so -1 from the right shift.
        t01 = vshrq_n_s16(t01, ROUND0_BITS - 1);
        t23 = vshrq_n_s16(t23, ROUND0_BITS - 1);

        t01 = vaddq_s16(t01, round_offset64);
        t23 = vaddq_s16(t23, round_offset64);

        d0 = vget_low_s16(t01);
        d1 = vget_high_s16(t01);
        d2 = vget_low_s16(t23);
        d3 = vget_high_s16(t23);

        if (conv_params->do_average) {
          __builtin_prefetch(d + 0 * dst_stride);
          __builtin_prefetch(d + 1 * dst_stride);
          __builtin_prefetch(d + 2 * dst_stride);
          __builtin_prefetch(d + 3 * dst_stride);

          __builtin_prefetch(d_u8 + 0 * dst8_stride);
          __builtin_prefetch(d_u8 + 1 * dst8_stride);
          __builtin_prefetch(d_u8 + 2 * dst8_stride);
          __builtin_prefetch(d_u8 + 3 * dst8_stride);

          load_u16_4x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);

          compute_avg_4x4(dd0, dd1, dd2, dd3, vreinterpret_u16_s16(d0),
                          vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                          vreinterpret_u16_s16(d3), fwd_offset, bck_offset,
                          vget_low_s16(round_offset64), round_bits,
                          use_dist_wtd_comp_avg, &d01, &d23);

          store_u8_4x1(d_u8 + 0 * dst8_stride, d01, 0);
          store_u8_4x1(d_u8 + 1 * dst8_stride, d01, 1);
          store_u8_4x1(d_u8 + 2 * dst8_stride, d23, 0);
          store_u8_4x1(d_u8 + 3 * dst8_stride, d23, 1);
        } else {
          store_u16_4x4(d, dst_stride, vreinterpret_u16_s16(d0),
                        vreinterpret_u16_s16(d1), vreinterpret_u16_s16(d2),
                        vreinterpret_u16_s16(d3));
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        d_u8 += 4 * dst8_stride;
        height -= 4;
#else
        t0 = load_unaligned_u8_4x1(s);
        tt0 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s7 = vget_low_s16(tt0);

        d0 = convolve8_y_4x4_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                 vert_const);
        // We halved the convolution filter values so -1 from the right shift.
        d0 = vshr_n_s16(d0, ROUND0_BITS - 1);
        d0 = vadd_s16(d0, round_offset64);

        if (conv_params->do_average) {
          __builtin_prefetch(d);

          dd0 = vld1_u16(d);

          compute_avg_4x1(dd0, vreinterpret_u16_s16(d0), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &d01);

          store_u8_4x1(d_u8, d01, 0);
        } else {
          vst1_u16(d, vreinterpret_u16_s16(d0));
        }

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;
        s += src_stride;
        d += dst_stride;
        d_u8 += dst8_stride;
        height--;
#endif
      } while (height > 0);
      src_ptr += 4;
      dst_ptr += 4;
      dst8 += 4;
      width -= 4;
    } while (width > 0);
  } else {
    const int16x8_t round_offset128 = vdupq_n_s16(round_offset);
    const int16x4_t round_offset64 = vdup_n_s16(round_offset);
    // This shim of 1 << ((ROUND0_BITS - 1) - 1) enables us to use non-rounding
    // shifts - which are generally faster than rounding shifts on modern CPUs.
    // The outermost -1 is needed because we halved the filter values.
    const int16x8_t vert_const = vdupq_n_s16(1 << ((ROUND0_BITS - 1) - 1));
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, d0;
    uint16x8_t dd0;
    uint8x8_t t0, t1, t2, t3, t4, t5, t6;

#if defined(__aarch64__)
    int16x8_t s8, s9, s10, s11, s12, s13, s14, d1, d2, d3, d4, d5, d6, d7;
    uint16x8_t dd1, dd2, dd3;
    uint8x8_t t7;
#endif
    int width = w;

    do {
      const uint8_t *s = src_ptr;
      CONV_BUF_TYPE *d = dst_ptr;
      uint8_t *d_u8 = dst8;
      int height = h;

      __builtin_prefetch(s + 0 * src_stride);
      __builtin_prefetch(s + 1 * src_stride);
      __builtin_prefetch(s + 2 * src_stride);
      __builtin_prefetch(s + 3 * src_stride);
      __builtin_prefetch(s + 4 * src_stride);
      __builtin_prefetch(s + 5 * src_stride);
      __builtin_prefetch(s + 6 * src_stride);
      __builtin_prefetch(s + 7 * src_stride);
      load_u8_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);

      s0 = vreinterpretq_s16_u16(vmovl_u8(t0));
      s1 = vreinterpretq_s16_u16(vmovl_u8(t1));
      s2 = vreinterpretq_s16_u16(vmovl_u8(t2));
      s3 = vreinterpretq_s16_u16(vmovl_u8(t3));
      s4 = vreinterpretq_s16_u16(vmovl_u8(t4));
      s5 = vreinterpretq_s16_u16(vmovl_u8(t5));
      s6 = vreinterpretq_s16_u16(vmovl_u8(t6));

      s += 7 * src_stride;

      do {
#if defined(__aarch64__)
        load_u8_8x8(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        s7 = vreinterpretq_s16_u16(vmovl_u8(t0));
        s8 = vreinterpretq_s16_u16(vmovl_u8(t1));
        s9 = vreinterpretq_s16_u16(vmovl_u8(t2));
        s10 = vreinterpretq_s16_u16(vmovl_u8(t3));
        s11 = vreinterpretq_s16_u16(vmovl_u8(t4));
        s12 = vreinterpretq_s16_u16(vmovl_u8(t5));
        s13 = vreinterpretq_s16_u16(vmovl_u8(t6));
        s14 = vreinterpretq_s16_u16(vmovl_u8(t7));

        __builtin_prefetch(dst_ptr + 0 * dst_stride);
        __builtin_prefetch(dst_ptr + 1 * dst_stride);
        __builtin_prefetch(dst_ptr + 2 * dst_stride);
        __builtin_prefetch(dst_ptr + 3 * dst_stride);

        d0 = convolve8_y_8x8_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                 vert_const);
        d1 = convolve8_y_8x8_s16(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                 vert_const);
        d2 = convolve8_y_8x8_s16(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                 vert_const);
        d3 = convolve8_y_8x8_s16(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                 vert_const);
        d4 = convolve8_y_8x8_s16(s4, s5, s6, s7, s8, s9, s10, s11, y_filter,
                                 vert_const);
        d5 = convolve8_y_8x8_s16(s5, s6, s7, s8, s9, s10, s11, s12, y_filter,
                                 vert_const);
        d6 = convolve8_y_8x8_s16(s6, s7, s8, s9, s10, s11, s12, s13, y_filter,
                                 vert_const);
        d7 = convolve8_y_8x8_s16(s7, s8, s9, s10, s11, s12, s13, s14, y_filter,
                                 vert_const);

        d0 = vaddq_s16(d0, round_offset128);
        d1 = vaddq_s16(d1, round_offset128);
        d2 = vaddq_s16(d2, round_offset128);
        d3 = vaddq_s16(d3, round_offset128);
        d4 = vaddq_s16(d4, round_offset128);
        d5 = vaddq_s16(d5, round_offset128);
        d6 = vaddq_s16(d6, round_offset128);
        d7 = vaddq_s16(d7, round_offset128);

        if (conv_params->do_average) {
          __builtin_prefetch(d + 0 * dst8_stride);
          __builtin_prefetch(d + 1 * dst8_stride);
          __builtin_prefetch(d + 2 * dst8_stride);
          __builtin_prefetch(d + 3 * dst8_stride);

          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);
          d += 4 * dst_stride;

          compute_avg_8x4(dd0, dd1, dd2, dd3, vreinterpretq_u16_s16(d0),
                          vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                          vreinterpretq_u16_s16(d3), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;

          load_u16_8x4(d, dst_stride, &dd0, &dd1, &dd2, &dd3);
          d += 4 * dst_stride;

          compute_avg_8x4(dd0, dd1, dd2, dd3, vreinterpretq_u16_s16(d4),
                          vreinterpretq_u16_s16(d5), vreinterpretq_u16_s16(d6),
                          vreinterpretq_u16_s16(d7), fwd_offset, bck_offset,
                          round_offset64, round_bits, use_dist_wtd_comp_avg,
                          &t0, &t1, &t2, &t3);

          store_u8_8x4(d_u8, dst8_stride, t0, t1, t2, t3);
          d_u8 += 4 * dst8_stride;
        } else {
          store_u16_8x8(d, dst_stride, vreinterpretq_u16_s16(d0),
                        vreinterpretq_u16_s16(d1), vreinterpretq_u16_s16(d2),
                        vreinterpretq_u16_s16(d3), vreinterpretq_u16_s16(d4),
                        vreinterpretq_u16_s16(d5), vreinterpretq_u16_s16(d6),
                        vreinterpretq_u16_s16(d7));
          d += 8 * dst_stride;
        }

        s0 = s8;
        s1 = s9;
        s2 = s10;
        s3 = s11;
        s4 = s12;
        s5 = s13;
        s6 = s14;
        s += 8 * src_stride;
        height -= 8;
#else
        s7 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(s)));

        __builtin_prefetch(dst_ptr);

        d0 = convolve8_y_8x8_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                 vert_const);
        d0 = vaddq_s16(d0, round_offset128);

        s0 = s1;
        s1 = s2;
        s2 = s3;
        s3 = s4;
        s4 = s5;
        s5 = s6;
        s6 = s7;

        if (conv_params->do_average) {
          __builtin_prefetch(d);

          dd0 = vld1q_u16(d);
          d += dst_stride;

          compute_avg_8x1(dd0, vreinterpretq_u16_s16(d0), fwd_offset,
                          bck_offset, round_offset64, round_bits,
                          use_dist_wtd_comp_avg, &t0);

          vst1_u8(d_u8, t0);
          d_u8 += dst8_stride;
        } else {
          vst1q_u16(d, vreinterpretq_u16_s16(d0));
          d += dst_stride;
        }

        s += src_stride;
        height--;
#endif
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      dst8 += 8;
      width -= 8;
    } while (width > 0);
  }
}
