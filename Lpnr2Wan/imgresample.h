/*
 * 	imgresample.h
 */

#ifndef _IMGRESAMPLE_H_
#define _IMGRESAMPLE_H_

//#include "picture.h"

#define NB_COMPONENTS 3

#define PHASE_BITS 4
#define NB_PHASES  (1 << PHASE_BITS)
#define NB_TAPS    4
#define FCENTER    1  /* index of the center of the filter */

#define POS_FRAC_BITS 16
#define POS_FRAC      (1 << POS_FRAC_BITS)
/* 6 bits precision is needed for MMX */
#define FILTER_BITS   8

#define LINE_BUF_HEIGHT (NB_TAPS * 4)

#ifdef linux
#define __align8	__attribute__ ((aligned (8)))
#else
#define __align8
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ImgReSampleContext {
    int iwidth, iheight, owidth, oheight;
    int h_incr, v_incr;
    short h_filters[NB_PHASES][NB_TAPS] __align8; /* horizontal filters */
    short v_filters[NB_PHASES][NB_TAPS] __align8; /* vertical filters */
    unsigned char *line_buf;
} ImgReSampleContext;

// simple version image resample, only support single plane image.
extern ImgReSampleContext *img_resample_init(int output_width, int output_height,
                                      int input_width, int input_height);
extern void img_resample(ImgReSampleContext *s, void *imgout, int ostride, void *imgin, int istride );
extern void img_resample_close(ImgReSampleContext *s);

#ifdef __cplusplus
};
#endif

#endif
