/*
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AOM_DSP_BUTTERAUGLI_H_
#define AOM_AOM_DSP_BUTTERAUGLI_H_

#include "aom_scale/yv12config.h"
#include <jxl/butteraugli.h>
#include <jxl/thread_parallel_runner.h>

struct AV1_COMP;

// Returns a boolean that indicates success/failure.
int aom_calc_butteraugli(struct AV1_COMP *cpi, const YV12_BUFFER_CONFIG *source,
                         const YV12_BUFFER_CONFIG *distorted, int bit_depth,
                         aom_matrix_coefficients_t matrix_coefficients,
                         aom_color_range_t color_range, float *dist_map, int target_intensity, int hf_asymmetry);

#endif  // AOM_AOM_DSP_BUTTERAUGLI_H_
