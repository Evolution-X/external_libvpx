/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef __INC_FINDNEARMV_H
#define __INC_FINDNEARMV_H

#include "mv.h"
#include "blockd.h"
#include "modecont.h"
#include "treecoder.h"
#include "onyxc_int.h"

#if CONFIG_NEWBESTREFMV
/* check a list of motion vectors by sad score using a number rows of pixels
 * above and a number cols of pixels in the left to select the one with best
 * score to use as ref motion vector
 */
void vp8_find_best_ref_mvs(MACROBLOCKD *xd,
                           unsigned char *ref_y_buffer,
                           int ref_y_stride,
                           int_mv *best_mv,
                           int_mv *nearest,
                           int_mv *near);
#endif

static void mv_bias(int refmb_ref_frame_sign_bias, int refframe, int_mv *mvp, const int *ref_frame_sign_bias) {
  MV xmv;
  xmv = mvp->as_mv;

  if (refmb_ref_frame_sign_bias != ref_frame_sign_bias[refframe]) {
    xmv.row *= -1;
    xmv.col *= -1;
  }

  mvp->as_mv = xmv;
}

#define LEFT_TOP_MARGIN (16 << 3)
#define RIGHT_BOTTOM_MARGIN (16 << 3)



static void vp8_clamp_mv(int_mv *mv,
                         int mb_to_left_edge,
                         int mb_to_right_edge,
                         int mb_to_top_edge,
                         int mb_to_bottom_edge) {
  mv->as_mv.col = (mv->as_mv.col < mb_to_left_edge) ?
                  mb_to_left_edge : mv->as_mv.col;
  mv->as_mv.col = (mv->as_mv.col > mb_to_right_edge) ?
                  mb_to_right_edge : mv->as_mv.col;
  mv->as_mv.row = (mv->as_mv.row < mb_to_top_edge) ?
                  mb_to_top_edge : mv->as_mv.row;
  mv->as_mv.row = (mv->as_mv.row > mb_to_bottom_edge) ?
                  mb_to_bottom_edge : mv->as_mv.row;
}

static void vp8_clamp_mv2(int_mv *mv, const MACROBLOCKD *xd) {
  vp8_clamp_mv(mv,
              xd->mb_to_left_edge - LEFT_TOP_MARGIN,
              xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN,
              xd->mb_to_top_edge - LEFT_TOP_MARGIN,
              xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN);
}



static unsigned int vp8_check_mv_bounds(int_mv *mv,
                                        int mb_to_left_edge,
                                        int mb_to_right_edge,
                                        int mb_to_top_edge,
                                        int mb_to_bottom_edge) {
  return (mv->as_mv.col < mb_to_left_edge) ||
         (mv->as_mv.col > mb_to_right_edge) ||
         (mv->as_mv.row < mb_to_top_edge) ||
         (mv->as_mv.row > mb_to_bottom_edge);
}

void vp8_find_near_mvs
(
  MACROBLOCKD *xd,
  const MODE_INFO *here,
  const MODE_INFO *lfhere,
  int_mv *nearest, int_mv *nearby, int_mv *best,
  int near_mv_ref_cts[4],
  int refframe,
  int *ref_frame_sign_bias
);

vp8_prob *vp8_mv_ref_probs(VP8_COMMON *pc,
                           vp8_prob p[VP8_MVREFS - 1], const int near_mv_ref_ct[4]
                          );

extern const unsigned char vp8_mbsplit_offset[4][16];


static int left_block_mv(const MODE_INFO *cur_mb, int b) {
  if (!(b & 3)) {
    /* On L edge, get from MB to left of us */
    --cur_mb;

    if (cur_mb->mbmi.mode != SPLITMV)
      return cur_mb->mbmi.mv[0].as_int;
    b += 4;
  }

  return (cur_mb->bmi + b - 1)->as_mv.first.as_int;
}

static int left_block_second_mv(const MODE_INFO *cur_mb, int b) {
  if (!(b & 3)) {
    /* On L edge, get from MB to left of us */
    --cur_mb;

    if (cur_mb->mbmi.mode != SPLITMV)
      return cur_mb->mbmi.second_ref_frame ? cur_mb->mbmi.mv[1].as_int : cur_mb->mbmi.mv[0].as_int;
    b += 4;
  }

  return cur_mb->mbmi.second_ref_frame ? (cur_mb->bmi + b - 1)->as_mv.second.as_int : (cur_mb->bmi + b - 1)->as_mv.first.as_int;
}

static int above_block_mv(const MODE_INFO *cur_mb, int b, int mi_stride) {
  if (!(b >> 2)) {
    /* On top edge, get from MB above us */
    cur_mb -= mi_stride;

    if (cur_mb->mbmi.mode != SPLITMV)
      return cur_mb->mbmi.mv[0].as_int;
    b += 16;
  }

  return (cur_mb->bmi + b - 4)->as_mv.first.as_int;
}

static int above_block_second_mv(const MODE_INFO *cur_mb, int b, int mi_stride) {
  if (!(b >> 2)) {
    /* On top edge, get from MB above us */
    cur_mb -= mi_stride;

    if (cur_mb->mbmi.mode != SPLITMV)
      return cur_mb->mbmi.second_ref_frame ? cur_mb->mbmi.mv[1].as_int : cur_mb->mbmi.mv[0].as_int;
    b += 16;
  }

  return cur_mb->mbmi.second_ref_frame ? (cur_mb->bmi + b - 4)->as_mv.second.as_int : (cur_mb->bmi + b - 4)->as_mv.first.as_int;
}

static B_PREDICTION_MODE left_block_mode(const MODE_INFO *cur_mb, int b) {
  if (!(b & 3)) {
    /* On L edge, get from MB to left of us */
    --cur_mb;
    switch (cur_mb->mbmi.mode) {
      case DC_PRED:
        return B_DC_PRED;
      case V_PRED:
        return B_VE_PRED;
      case H_PRED:
        return B_HE_PRED;
      case TM_PRED:
        return B_TM_PRED;
      case I8X8_PRED:
      case B_PRED:
        return (cur_mb->bmi + b + 3)->as_mode.first;
      default:
        return B_DC_PRED;
    }
  }
  return (cur_mb->bmi + b - 1)->as_mode.first;
}

static B_PREDICTION_MODE above_block_mode(const MODE_INFO
                                          *cur_mb, int b, int mi_stride) {
  if (!(b >> 2)) {
    /* On top edge, get from MB above us */
    cur_mb -= mi_stride;

    switch (cur_mb->mbmi.mode) {
      case DC_PRED:
        return B_DC_PRED;
      case V_PRED:
        return B_VE_PRED;
      case H_PRED:
        return B_HE_PRED;
      case TM_PRED:
        return B_TM_PRED;
      case I8X8_PRED:
      case B_PRED:
        return (cur_mb->bmi + b + 12)->as_mode.first;
      default:
        return B_DC_PRED;
    }
  }

  return (cur_mb->bmi + b - 4)->as_mode.first;
}

#endif
