/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <stdlib.h>

#include "config/aom_dsp_rtcd.h"
#include "aom_ports/mem.h"

void aom_minmax_8x8_c(const uint8_t *s, int p, const uint8_t *d, int dp,
                      int *min, int *max) {
  int i, j;
  *min = 255;
  *max = 0;
  for (i = 0; i < 8; ++i, s += p, d += dp) {
    for (j = 0; j < 8; ++j) {
      int diff = abs(s[j] - d[j]);
      *min = diff < *min ? diff : *min;
      *max = diff > *max ? diff : *max;
    }
  }
}

unsigned int aom_avg_4x4_c(const uint8_t *s, int p) {
  int i, j;
  int sum = 0;
  for (i = 0; i < 4; ++i, s += p)
    for (j = 0; j < 4; sum += s[j], ++j) {
    }

  return (sum + 8) >> 4;
}

unsigned int aom_avg_8x8_c(const uint8_t *s, int p) {
  int i, j;
  int sum = 0;
  for (i = 0; i < 8; ++i, s += p)
    for (j = 0; j < 8; sum += s[j], ++j) {
    }

  return (sum + 32) >> 6;
}

void aom_avg_8x8_quad_c(const uint8_t *s, int p, int x16_idx, int y16_idx,
                        int *avg) {
  for (int k = 0; k < 4; k++) {
    const int x8_idx = x16_idx + ((k & 1) << 3);
    const int y8_idx = y16_idx + ((k >> 1) << 3);
    const uint8_t *s_tmp = s + y8_idx * p + x8_idx;
    avg[k] = aom_avg_8x8_c(s_tmp, p);
  }
}

#if CONFIG_AV1_HIGHBITDEPTH
unsigned int aom_highbd_avg_8x8_c(const uint8_t *s8, int p) {
  int i, j;
  int sum = 0;
  const uint16_t *s = CONVERT_TO_SHORTPTR(s8);
  for (i = 0; i < 8; ++i, s += p)
    for (j = 0; j < 8; sum += s[j], ++j) {
    }

  return (sum + 32) >> 6;
}

unsigned int aom_highbd_avg_4x4_c(const uint8_t *s8, int p) {
  int i, j;
  int sum = 0;
  const uint16_t *s = CONVERT_TO_SHORTPTR(s8);
  for (i = 0; i < 4; ++i, s += p)
    for (j = 0; j < 4; sum += s[j], ++j) {
    }

  return (sum + 8) >> 4;
}

void aom_highbd_minmax_8x8_c(const uint8_t *s8, int p, const uint8_t *d8,
                             int dp, int *min, int *max) {
  int i, j;
  const uint16_t *s = CONVERT_TO_SHORTPTR(s8);
  const uint16_t *d = CONVERT_TO_SHORTPTR(d8);
  *min = 255;
  *max = 0;
  for (i = 0; i < 8; ++i, s += p, d += dp) {
    for (j = 0; j < 8; ++j) {
      int diff = abs(s[j] - d[j]);
      *min = diff < *min ? diff : *min;
      *max = diff > *max ? diff : *max;
    }
  }
}
#endif  // CONFIG_AV1_HIGHBITDEPTH

static void hadamard_col4(const int16_t *src_diff, ptrdiff_t src_stride,
                          int16_t *coeff) {
  int16_t b0 = (src_diff[0 * src_stride] + src_diff[1 * src_stride]) >> 1;
  int16_t b1 = (src_diff[0 * src_stride] - src_diff[1 * src_stride]) >> 1;
  int16_t b2 = (src_diff[2 * src_stride] + src_diff[3 * src_stride]) >> 1;
  int16_t b3 = (src_diff[2 * src_stride] - src_diff[3 * src_stride]) >> 1;

  coeff[0] = b0 + b2;
  coeff[1] = b1 + b3;
  coeff[2] = b0 - b2;
  coeff[3] = b1 - b3;
}

void aom_hadamard_4x4_c(const int16_t *src_diff, ptrdiff_t src_stride,
                        tran_low_t *coeff) {
  int idx;
  int16_t buffer[16];
  int16_t buffer2[16];
  int16_t *tmp_buf = &buffer[0];
  for (idx = 0; idx < 4; ++idx) {
    hadamard_col4(src_diff, src_stride, tmp_buf);  // src_diff: 9 bit
                                                   // dynamic range [-255, 255]
    tmp_buf += 4;
    ++src_diff;
  }

  tmp_buf = &buffer[0];
  for (idx = 0; idx < 4; ++idx) {
    hadamard_col4(tmp_buf, 4, buffer2 + 4 * idx);  // tmp_buf: 12 bit
    // dynamic range [-2040, 2040]
    // buffer2: 15 bit
    // dynamic range [-16320, 16320]
    ++tmp_buf;
  }

  // Extra transpose to match SSE2 behavior(i.e., aom_hadamard_4x4_sse2).
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      coeff[i * 4 + j] = (tran_low_t)buffer2[j * 4 + i];
    }
  }
}

// src_diff: first pass, 9 bit, dynamic range [-255, 255]
//           second pass, 12 bit, dynamic range [-2040, 2040]
static void hadamard_col8(const int16_t *src_diff, ptrdiff_t src_stride,
                          int16_t *coeff) {
  int16_t b0 = src_diff[0 * src_stride] + src_diff[1 * src_stride];
  int16_t b1 = src_diff[0 * src_stride] - src_diff[1 * src_stride];
  int16_t b2 = src_diff[2 * src_stride] + src_diff[3 * src_stride];
  int16_t b3 = src_diff[2 * src_stride] - src_diff[3 * src_stride];
  int16_t b4 = src_diff[4 * src_stride] + src_diff[5 * src_stride];
  int16_t b5 = src_diff[4 * src_stride] - src_diff[5 * src_stride];
  int16_t b6 = src_diff[6 * src_stride] + src_diff[7 * src_stride];
  int16_t b7 = src_diff[6 * src_stride] - src_diff[7 * src_stride];

  int16_t c0 = b0 + b2;
  int16_t c1 = b1 + b3;
  int16_t c2 = b0 - b2;
  int16_t c3 = b1 - b3;
  int16_t c4 = b4 + b6;
  int16_t c5 = b5 + b7;
  int16_t c6 = b4 - b6;
  int16_t c7 = b5 - b7;

  coeff[0] = c0 + c4;
  coeff[7] = c1 + c5;
  coeff[3] = c2 + c6;
  coeff[4] = c3 + c7;
  coeff[2] = c0 - c4;
  coeff[6] = c1 - c5;
  coeff[1] = c2 - c6;
  coeff[5] = c3 - c7;
}

void aom_hadamard_8x8_c(const int16_t *src_diff, ptrdiff_t src_stride,
                        tran_low_t *coeff) {
  int idx;
  int16_t buffer[64];
  int16_t buffer2[64];
  int16_t *tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    hadamard_col8(src_diff, src_stride, tmp_buf);  // src_diff: 9 bit
                                                   // dynamic range [-255, 255]
    tmp_buf += 8;
    ++src_diff;
  }

  tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    hadamard_col8(tmp_buf, 8, buffer2 + 8 * idx);  // tmp_buf: 12 bit
    // dynamic range [-2040, 2040]
    // buffer2: 15 bit
    // dynamic range [-16320, 16320]
    ++tmp_buf;
  }

  // Extra transpose to match SSE2 behavior(i.e., aom_hadamard_8x8_sse2).
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      coeff[i * 8 + j] = (tran_low_t)buffer2[j * 8 + i];
    }
  }
}

void aom_hadamard_lp_8x8_c(const int16_t *src_diff, ptrdiff_t src_stride,
                           int16_t *coeff) {
  int16_t buffer[64];
  int16_t buffer2[64];
  int16_t *tmp_buf = &buffer[0];
  for (int idx = 0; idx < 8; ++idx) {
    hadamard_col8(src_diff, src_stride, tmp_buf);  // src_diff: 9 bit
                                                   // dynamic range [-255, 255]
    tmp_buf += 8;
    ++src_diff;
  }

  tmp_buf = &buffer[0];
  for (int idx = 0; idx < 8; ++idx) {
    hadamard_col8(tmp_buf, 8, buffer2 + 8 * idx);  // tmp_buf: 12 bit
    // dynamic range [-2040, 2040]
    // buffer2: 15 bit
    // dynamic range [-16320, 16320]
    ++tmp_buf;
  }

  for (int idx = 0; idx < 64; ++idx) coeff[idx] = buffer2[idx];

  // Extra transpose to match SSE2 behavior(i.e., aom_hadamard_lp_8x8_sse2).
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      coeff[i * 8 + j] = buffer2[j * 8 + i];
    }
  }
}

void aom_hadamard_lp_8x8_dual_c(const int16_t *src_diff, ptrdiff_t src_stride,
                                int16_t *coeff) {
  for (int i = 0; i < 2; i++) {
    aom_hadamard_lp_8x8_c(src_diff + (i * 8), src_stride,
                          (int16_t *)coeff + (i * 64));
  }
}

// In place 16x16 2D Hadamard transform
void aom_hadamard_16x16_c(const int16_t *src_diff, ptrdiff_t src_stride,
                          tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 8 * src_stride + (idx & 0x01) * 8;
    aom_hadamard_8x8_c(src_ptr, src_stride, coeff + idx * 64);
  }

  // coeff: 15 bit, dynamic range [-16320, 16320]
  for (idx = 0; idx < 64; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[64];
    tran_low_t a2 = coeff[128];
    tran_low_t a3 = coeff[192];

    tran_low_t b0 = (a0 + a1) >> 1;  // (a0 + a1): 16 bit, [-32640, 32640]
    tran_low_t b1 = (a0 - a1) >> 1;  // b0-b3: 15 bit, dynamic range
    tran_low_t b2 = (a2 + a3) >> 1;  // [-16320, 16320]
    tran_low_t b3 = (a2 - a3) >> 1;

    coeff[0] = b0 + b2;  // 16 bit, [-32640, 32640]
    coeff[64] = b1 + b3;
    coeff[128] = b0 - b2;
    coeff[192] = b1 - b3;

    ++coeff;
  }

  coeff -= 64;
  // Extra shift to match AVX2 output (i.e., aom_hadamard_16x16_avx2).
  // Note that to match SSE2 output, it does not need this step.
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 4; j++) {
      tran_low_t temp = coeff[i * 16 + 4 + j];
      coeff[i * 16 + 4 + j] = coeff[i * 16 + 8 + j];
      coeff[i * 16 + 8 + j] = temp;
    }
  }
}

void aom_hadamard_lp_16x16_c(const int16_t *src_diff, ptrdiff_t src_stride,
                             int16_t *coeff) {
  for (int idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 8 * src_stride + (idx & 0x01) * 8;
    aom_hadamard_lp_8x8_c(src_ptr, src_stride, coeff + idx * 64);
  }

  for (int idx = 0; idx < 64; ++idx) {
    int16_t a0 = coeff[0];
    int16_t a1 = coeff[64];
    int16_t a2 = coeff[128];
    int16_t a3 = coeff[192];

    int16_t b0 = (a0 + a1) >> 1;  // (a0 + a1): 16 bit, [-32640, 32640]
    int16_t b1 = (a0 - a1) >> 1;  // b0-b3: 15 bit, dynamic range
    int16_t b2 = (a2 + a3) >> 1;  // [-16320, 16320]
    int16_t b3 = (a2 - a3) >> 1;

    coeff[0] = b0 + b2;  // 16 bit, [-32640, 32640]
    coeff[64] = b1 + b3;
    coeff[128] = b0 - b2;
    coeff[192] = b1 - b3;

    ++coeff;
  }
}

void aom_hadamard_32x32_c(const int16_t *src_diff, ptrdiff_t src_stride,
                          tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 9 bit, dynamic range [-255, 255]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 16 * src_stride + (idx & 0x01) * 16;
    aom_hadamard_16x16_c(src_ptr, src_stride, coeff + idx * 256);
  }

  // coeff: 15 bit, dynamic range [-16320, 16320]
  for (idx = 0; idx < 256; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[256];
    tran_low_t a2 = coeff[512];
    tran_low_t a3 = coeff[768];

    tran_low_t b0 = (a0 + a1) >> 2;  // (a0 + a1): 16 bit, [-32640, 32640]
    tran_low_t b1 = (a0 - a1) >> 2;  // b0-b3: 15 bit, dynamic range
    tran_low_t b2 = (a2 + a3) >> 2;  // [-16320, 16320]
    tran_low_t b3 = (a2 - a3) >> 2;

    coeff[0] = b0 + b2;  // 16 bit, [-32640, 32640]
    coeff[256] = b1 + b3;
    coeff[512] = b0 - b2;
    coeff[768] = b1 - b3;

    ++coeff;
  }
}

#if CONFIG_AV1_HIGHBITDEPTH
static void hadamard_highbd_col8_first_pass(const int16_t *src_diff,
                                            ptrdiff_t src_stride,
                                            int16_t *coeff) {
  int16_t b0 = src_diff[0 * src_stride] + src_diff[1 * src_stride];
  int16_t b1 = src_diff[0 * src_stride] - src_diff[1 * src_stride];
  int16_t b2 = src_diff[2 * src_stride] + src_diff[3 * src_stride];
  int16_t b3 = src_diff[2 * src_stride] - src_diff[3 * src_stride];
  int16_t b4 = src_diff[4 * src_stride] + src_diff[5 * src_stride];
  int16_t b5 = src_diff[4 * src_stride] - src_diff[5 * src_stride];
  int16_t b6 = src_diff[6 * src_stride] + src_diff[7 * src_stride];
  int16_t b7 = src_diff[6 * src_stride] - src_diff[7 * src_stride];

  int16_t c0 = b0 + b2;
  int16_t c1 = b1 + b3;
  int16_t c2 = b0 - b2;
  int16_t c3 = b1 - b3;
  int16_t c4 = b4 + b6;
  int16_t c5 = b5 + b7;
  int16_t c6 = b4 - b6;
  int16_t c7 = b5 - b7;

  coeff[0] = c0 + c4;
  coeff[7] = c1 + c5;
  coeff[3] = c2 + c6;
  coeff[4] = c3 + c7;
  coeff[2] = c0 - c4;
  coeff[6] = c1 - c5;
  coeff[1] = c2 - c6;
  coeff[5] = c3 - c7;
}

// src_diff: 16 bit, dynamic range [-32760, 32760]
// coeff: 19 bit
static void hadamard_highbd_col8_second_pass(const int16_t *src_diff,
                                             ptrdiff_t src_stride,
                                             int32_t *coeff) {
  int32_t b0 = src_diff[0 * src_stride] + src_diff[1 * src_stride];
  int32_t b1 = src_diff[0 * src_stride] - src_diff[1 * src_stride];
  int32_t b2 = src_diff[2 * src_stride] + src_diff[3 * src_stride];
  int32_t b3 = src_diff[2 * src_stride] - src_diff[3 * src_stride];
  int32_t b4 = src_diff[4 * src_stride] + src_diff[5 * src_stride];
  int32_t b5 = src_diff[4 * src_stride] - src_diff[5 * src_stride];
  int32_t b6 = src_diff[6 * src_stride] + src_diff[7 * src_stride];
  int32_t b7 = src_diff[6 * src_stride] - src_diff[7 * src_stride];

  int32_t c0 = b0 + b2;
  int32_t c1 = b1 + b3;
  int32_t c2 = b0 - b2;
  int32_t c3 = b1 - b3;
  int32_t c4 = b4 + b6;
  int32_t c5 = b5 + b7;
  int32_t c6 = b4 - b6;
  int32_t c7 = b5 - b7;

  coeff[0] = c0 + c4;
  coeff[7] = c1 + c5;
  coeff[3] = c2 + c6;
  coeff[4] = c3 + c7;
  coeff[2] = c0 - c4;
  coeff[6] = c1 - c5;
  coeff[1] = c2 - c6;
  coeff[5] = c3 - c7;
}

// The order of the output coeff of the hadamard is not important. For
// optimization purposes the final transpose may be skipped.
void aom_highbd_hadamard_8x8_c(const int16_t *src_diff, ptrdiff_t src_stride,
                               tran_low_t *coeff) {
  int idx;
  int16_t buffer[64];
  int32_t buffer2[64];
  int16_t *tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    // src_diff: 13 bit
    // buffer: 16 bit, dynamic range [-32760, 32760]
    hadamard_highbd_col8_first_pass(src_diff, src_stride, tmp_buf);
    tmp_buf += 8;
    ++src_diff;
  }

  tmp_buf = &buffer[0];
  for (idx = 0; idx < 8; ++idx) {
    // buffer: 16 bit
    // buffer2: 19 bit, dynamic range [-262080, 262080]
    hadamard_highbd_col8_second_pass(tmp_buf, 8, buffer2 + 8 * idx);
    ++tmp_buf;
  }

  for (idx = 0; idx < 64; ++idx) coeff[idx] = (tran_low_t)buffer2[idx];
}

// In place 16x16 2D Hadamard transform
void aom_highbd_hadamard_16x16_c(const int16_t *src_diff, ptrdiff_t src_stride,
                                 tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 13 bit, dynamic range [-4095, 4095]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 8 * src_stride + (idx & 0x01) * 8;
    aom_highbd_hadamard_8x8_c(src_ptr, src_stride, coeff + idx * 64);
  }

  // coeff: 19 bit, dynamic range [-262080, 262080]
  for (idx = 0; idx < 64; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[64];
    tran_low_t a2 = coeff[128];
    tran_low_t a3 = coeff[192];

    tran_low_t b0 = (a0 + a1) >> 1;
    tran_low_t b1 = (a0 - a1) >> 1;
    tran_low_t b2 = (a2 + a3) >> 1;
    tran_low_t b3 = (a2 - a3) >> 1;

    // new coeff dynamic range: 20 bit
    coeff[0] = b0 + b2;
    coeff[64] = b1 + b3;
    coeff[128] = b0 - b2;
    coeff[192] = b1 - b3;

    ++coeff;
  }
}

void aom_highbd_hadamard_32x32_c(const int16_t *src_diff, ptrdiff_t src_stride,
                                 tran_low_t *coeff) {
  int idx;
  for (idx = 0; idx < 4; ++idx) {
    // src_diff: 13 bit, dynamic range [-4095, 4095]
    const int16_t *src_ptr =
        src_diff + (idx >> 1) * 16 * src_stride + (idx & 0x01) * 16;
    aom_highbd_hadamard_16x16_c(src_ptr, src_stride, coeff + idx * 256);
  }

  // coeff: 20 bit
  for (idx = 0; idx < 256; ++idx) {
    tran_low_t a0 = coeff[0];
    tran_low_t a1 = coeff[256];
    tran_low_t a2 = coeff[512];
    tran_low_t a3 = coeff[768];

    tran_low_t b0 = (a0 + a1) >> 2;
    tran_low_t b1 = (a0 - a1) >> 2;
    tran_low_t b2 = (a2 + a3) >> 2;
    tran_low_t b3 = (a2 - a3) >> 2;

    // new coeff dynamic range: 20 bit
    coeff[0] = b0 + b2;
    coeff[256] = b1 + b3;
    coeff[512] = b0 - b2;
    coeff[768] = b1 - b3;

    ++coeff;
  }
}
#endif  // CONFIG_AV1_HIGHBITDEPTH

// coeff: 16 bits, dynamic range [-32640, 32640].
// length: value range {16, 64, 256, 1024}.
int aom_satd_c(const tran_low_t *coeff, int length) {
  int i;
  int satd = 0;
  for (i = 0; i < length; ++i) satd += abs(coeff[i]);

  // satd: 26 bits, dynamic range [-32640 * 1024, 32640 * 1024]
  return satd;
}

int aom_satd_lp_c(const int16_t *coeff, int length) {
  int satd = 0;
  for (int i = 0; i < length; ++i) satd += abs(coeff[i]);

  // satd: 26 bits, dynamic range [-32640 * 1024, 32640 * 1024]
  return satd;
}

// Integer projection onto row vectors.
// height: value range {16, 32, 64, 128}.
void aom_int_pro_row_c(int16_t *hbuf, const uint8_t *ref, const int ref_stride,
                       const int width, const int height, int norm_factor) {
  assert(height >= 2);
  for (int idx = 0; idx < width; ++idx) {
    hbuf[idx] = 0;
    // hbuf[idx]: 14 bit, dynamic range [0, 32640].
    for (int i = 0; i < height; ++i) hbuf[idx] += ref[i * ref_stride];
    // hbuf[idx]: 9 bit, dynamic range [0, 1020].
    hbuf[idx] >>= norm_factor;
    ++ref;
  }
}

// width: value range {16, 32, 64, 128}.
void aom_int_pro_col_c(int16_t *vbuf, const uint8_t *ref, const int ref_stride,
                       const int width, const int height, int norm_factor) {
  for (int ht = 0; ht < height; ++ht) {
    int16_t sum = 0;
    // sum: 14 bit, dynamic range [0, 32640]
    for (int idx = 0; idx < width; ++idx) sum += ref[idx];
    vbuf[ht] = sum >> norm_factor;
    ref += ref_stride;
  }
}

// ref: [0 - 510]
// src: [0 - 510]
// bwl: {2, 3, 4, 5}
int aom_vector_var_c(const int16_t *ref, const int16_t *src, int bwl) {
  int i;
  int width = 4 << bwl;
  int sse = 0, mean = 0, var;

  for (i = 0; i < width; ++i) {
    int diff = ref[i] - src[i];  // diff: dynamic range [-510, 510], 10 bits.
    mean += diff;                // mean: dynamic range 16 bits.
    sse += diff * diff;          // sse:  dynamic range 26 bits.
  }

  // (mean * mean): dynamic range 31 bits.
  // If width == 128, the mean can be 510 * 128 = 65280, and log2(65280 ** 2) ~=
  // 31.99, so it needs to be casted to unsigned int to compute its square.
  const unsigned int mean_abs = abs(mean);
  var = sse - ((mean_abs * mean_abs) >> (bwl + 2));
  return var;
}
