/*
 * High quality image resampling with polyphase filters
 * Copyright (c) 2001 Gerard Lantau.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "imgresample.h"

#if !defined M_PI
#define M_PI		3.1415962
#endif

/*
static inline int get_phase(int pos)
{
    return ((pos) >> (POS_FRAC_BITS - PHASE_BITS)) & ((1 << PHASE_BITS) - 1);
}
*/
#define get_phase(pos) \
	((pos) >> (POS_FRAC_BITS - PHASE_BITS)) & ((1 << PHASE_BITS) - 1)

/* This function must be optimized */
static void h_resample_fast(unsigned char *dst, int dst_width, unsigned char *src, int src_width,
                            int src_start, int src_incr, short *filters)
{
    int src_pos, phase, sum, i;
    unsigned char *s;
    short *filter;

    src_pos = src_start;
    for(i=0;i<dst_width;i++) {
        s = src + (src_pos >> POS_FRAC_BITS);
        phase = get_phase(src_pos);
        filter = filters + phase * NB_TAPS;
#if NB_TAPS == 4
        sum = s[0] * filter[0] +
              s[1] * filter[1] +
              s[2] * filter[2] +
              s[3] * filter[3];
#else
        {
            int j;
            sum = 0;
            for(j=0;j<NB_TAPS;j++)
                sum += s[j] * filter[j];
        }
#endif
        sum = sum >> FILTER_BITS;
        if (sum < 0)
            sum = 0;
        else if (sum > 255)
            sum = 255;
        dst[0] = sum;
        src_pos += src_incr;
        dst++;
    }
}

/* This function must be optimized */
static void v_resample(unsigned char *dst, int dst_width, unsigned char *src, int wrap,
                       short *filter)
{
    int sum, i, wrap2, wrap3;
    unsigned char *s;

    wrap2 = wrap << 1;
    wrap3 = wrap * 3;
    s = src;
    for(i=0;i<dst_width;i++, s++, dst++) {
#if NB_TAPS == 4
        sum = s[0]     * filter[0] +
              s[wrap]  * filter[1] +
              s[wrap2] * filter[2] +
              s[wrap3] * filter[3];
#else
        {
            int j;
            unsigned char *s1 = s;

            sum = 0;
            for(j=0;j<NB_TAPS;j++) {
                sum += s1[0] * filter[j];
                s1 += wrap;
            }
        }
#endif
        sum = sum >> FILTER_BITS;
        if (sum < 0)
            sum = 0;
        else if (sum > 255)
            sum = 255;
        dst[0] = sum;
    }
}

/* slow version to handle limit cases. Does not need optimisation */
static void h_resample_slow(unsigned char *dst, int dst_width, unsigned char *src, int src_width,
                            int src_start, int src_incr, short *filters)
{
    int src_pos, phase, sum, j, v, i;
    unsigned char *s, *src_end;
    short *filter;

    src_end = src + src_width;
    src_pos = src_start;
    for(i=0;i<dst_width;i++) {
        s = src + (src_pos >> POS_FRAC_BITS);
        phase = get_phase(src_pos);
        filter = filters + phase * NB_TAPS;
        sum = 0;
        for(j=0;j<NB_TAPS;j++) {
            if (s < src)
                v = src[0];
            else if (s >= src_end)
                v = src_end[-1];
            else
                v = s[0];
            sum += v * filter[j];
            s++;
        }
        sum = sum >> FILTER_BITS;
        if (sum < 0)
            sum = 0;
        else if (sum > 255)
            sum = 255;
        dst[0] = sum;
        src_pos += src_incr;
        dst++;
    }
}

static void h_resample(unsigned char *dst, int dst_width, unsigned char *src, int src_width,
                       int src_start, int src_incr, short *filters)
{
    int n, src_end;

    if (src_start < 0) {
        n = (0 - src_start + src_incr - 1) / src_incr;
        h_resample_slow(dst, n, src, src_width, src_start, src_incr, filters);
        dst += n;
        dst_width -= n;
        src_start += n * src_incr;
    }
    src_end = src_start + dst_width * src_incr;
    if (src_end > ((src_width - NB_TAPS) << POS_FRAC_BITS)) {
        n = (((src_width - NB_TAPS + 1) << POS_FRAC_BITS) - 1 - src_start) /
            src_incr;
    } else {
        n = dst_width;
    }
    h_resample_fast(dst, n, src, src_width, src_start, src_incr, filters);
    if (n < dst_width) {
        dst += n;
        dst_width -= n;
        src_start += n * src_incr;
        h_resample_slow(dst, dst_width,
                        src, src_width, src_start, src_incr, filters);
    }
}


/* XXX: the following filter is quite naive, but it seems to suffice
   for 4 taps */
static void build_filter(short *filter, double factor)
{
    int ph, i, v;
    double x, y, tab[NB_TAPS], norm, mult;

    /* if upsampling, only need to interpolate, no filter */
    if (factor > 1.0)
        factor = 1.0;

    for(ph=0;ph<NB_PHASES;ph++) {
        norm = 0;
        for(i=0;i<NB_TAPS;i++) {

            x = M_PI * ((double)(i - FCENTER) - (double)ph / NB_PHASES) * factor;
            if (x == 0)
                y = 1.0;
            else
                y = sin(x) / x;
            tab[i] = y;
            norm += y;
        }

        /* normalize so that an uniform color remains the same */
        mult = (double)(1 << FILTER_BITS) / norm;
        for(i=0;i<NB_TAPS;i++) {
            v = (int)(tab[i] * mult);
            filter[ph * NB_TAPS + i] = v;
        }
    }
}

ImgReSampleContext *img_resample_init(int owidth, int oheight,
                                      int iwidth, int iheight)
{
    ImgReSampleContext *s;

    s = malloc(sizeof(ImgReSampleContext));
    if (!s)
        return NULL;
    memset( s, 0, sizeof(ImgReSampleContext) );

    s->line_buf = malloc (owidth * (LINE_BUF_HEIGHT + NB_TAPS));
    if (!s->line_buf)
        goto fail;
    memset( s->line_buf, 0, owidth * (LINE_BUF_HEIGHT + NB_TAPS) );

    s->owidth = owidth;
    s->oheight = oheight;
    s->iwidth = iwidth;
    s->iheight = iheight;

    s->h_incr = (iwidth * POS_FRAC) / owidth;
    s->v_incr = (iheight * POS_FRAC) / oheight;

    build_filter(&s->h_filters[0][0], (double)owidth / (double)iwidth);
    build_filter(&s->v_filters[0][0], (double)oheight / (double)iheight);

    return s;
 fail:
    free(s);
    return NULL;
}

void img_resample_close(ImgReSampleContext *s)
{
    free(s->line_buf);
    free(s);
}

void img_resample(ImgReSampleContext *s, void *imgout, int ostride, void *imgin, int istride )
{
	unsigned char *output = (unsigned char *)imgout;
	unsigned char *input = (unsigned char *)imgin;
    	int src_y, src_y1, last_src_y, ring_y, phase_y, y1, y;
    	unsigned char *new_line, *src_line;
	if ( ostride == 0 ) ostride = s->owidth;
	if ( istride == 0 ) istride = s->iwidth;
	last_src_y = - FCENTER - 1;
	/* position of the bottom of the filter in the source image */
	src_y = (last_src_y + NB_TAPS) * POS_FRAC;
	ring_y = NB_TAPS; /* position in ring buffer */
	for(y=0;y<s->oheight;y++) 
	{
	    /* apply horizontal filter on new lines from input if needed */
	    src_y1 = src_y >> POS_FRAC_BITS;
	    while (last_src_y < src_y1) 
	    {
		if (++ring_y >= LINE_BUF_HEIGHT + NB_TAPS)
                		ring_y = NB_TAPS;
		last_src_y++;
		/* handle limit conditions : replicate line (slighly inefficient because we filter multiple times */
		y1 = last_src_y;
		if (y1 < 0) 
                		y1 = 0;
		else if (y1 >= s->iheight)
			y1 = s->iheight - 1;
		src_line = input + y1 * istride;
		new_line = s->line_buf + ring_y * s->owidth;
		/* apply filter and handle limit cases correctly */
		h_resample(new_line, s->owidth, src_line, s->iwidth, - FCENTER * POS_FRAC, 
					s->h_incr,  &s->h_filters[0][0]);
		/* handle ring buffer wraping */
		if (ring_y >= LINE_BUF_HEIGHT) 
			memcpy(s->line_buf + (ring_y - LINE_BUF_HEIGHT) * s->owidth, new_line, s->owidth);
	    }
	    /* apply vertical filter */
	    phase_y = get_phase(src_y);
	    v_resample(output, s->owidth, s->line_buf + (ring_y - NB_TAPS + 1) * s->owidth, s->owidth,
          	             &s->v_filters[phase_y][0]);
	    src_y += s->v_incr;
	    output += ostride;
	}
}

#ifdef ENABLE_TESTRESAMPLE

void *av_mallocz(int size)
{
    void *ptr;
    ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

/* input */
#define XSIZE 256
#define YSIZE 256
unsigned char img[XSIZE * YSIZE];

/* output */
#define XSIZE1 512
#define YSIZE1 512
unsigned char img1[XSIZE1 * YSIZE1];
unsigned char img2[XSIZE1 * YSIZE1];

void save_pgm(const char *filename, unsigned char *img, int xsize, int ysize)
{
    FILE *f;
    f=fopen(filename,"w");
    fprintf(f,"P5\n%d %d\n%d\n", xsize, ysize, 255);
    fwrite(img,1, xsize * ysize,f);
    fclose(f);
}

static void dump_filter(short *filter)
{
    int i, ph;

    for(ph=0;ph<NB_PHASES;ph++) {
        printf("%2d: ", ph);
        for(i=0;i<NB_TAPS;i++) {
            printf(" %5.2f", filter[ph * NB_TAPS + i] / 256.0);
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    int x, y, v, i, xsize, ysize;
    ImgReSampleContext *s;
    float fact, factors[] = { 1/2.0, 3.0/4.0, 1.0, 4.0/3.0, 16.0/9.0, 2.0 };
    char buf[256];

    /* build test image */
    for(y=0;y<YSIZE;y++) {
        for(x=0;x<XSIZE;x++) {
            if (x < XSIZE/2 && y < YSIZE/2) {
                if (x < XSIZE/4 && y < YSIZE/4) {
                    if ((x % 10) <= 6 &&
                        (y % 10) <= 6)
                        v = 0xff;
                    else
                        v = 0x00;
                } else if (x < XSIZE/4) {
                    if (x & 1)
                        v = 0xff;
                    else
                        v = 0;
                } else if (y < XSIZE/4) {
                    if (y & 1)
                        v = 0xff;
                    else
                        v = 0;
                } else {
                    if (y < YSIZE*3/8) {
                        if ((y+x) & 1)
                            v = 0xff;
                        else
                            v = 0;
                    } else {
                        if (((x+3) % 4) <= 1 &&
                            ((y+3) % 4) <= 1)
                            v = 0xff;
                        else
                            v = 0x00;
                    }
                }
            } else if (x < XSIZE/2) {
                v = ((x - (XSIZE/2)) * 255) / (XSIZE/2);
            } else if (y < XSIZE/2) {
                v = ((y - (XSIZE/2)) * 255) / (XSIZE/2);
            } else {
                v = ((x + y - XSIZE) * 255) / XSIZE;
            }
            img[y * XSIZE + x] = v;
        }
    }
    save_pgm("/tmp/in.pgm", img, XSIZE, YSIZE);
    for(i=0;i<sizeof(factors)/sizeof(float);i++) {
        fact = factors[i];
        xsize = (int)(XSIZE * fact);
        ysize = (int)(YSIZE * fact);
        s = img_resample_init(xsize, ysize, XSIZE, YSIZE);
        printf("Factor=%0.2f\n", fact);
        dump_filter(&s->h_filters[0][0]);
        img_resample(s, img1, img);
        img_resample_close(s);

        sprintf(buf, "./out%d.pgm", i);
        save_pgm(buf, img1, xsize, ysize);
    }
    return 0;
}

#endif
