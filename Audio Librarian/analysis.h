//
//  analysis.h
//  Audio Librarian
//
//  Created by Cedric Spindler on 01.09.14.
//  Copyright (c) 2014 Zedcat. All rights reserved.
//

#ifndef cs_init_analysis_h
#define cs_init_analysis_h

#include "main.h"

/* function prototypes */
void analyze(t_alib * setup,sf_count_t length);
void search(t_alib * setup);
void processing_loop(t_alib * setup);
void hpcalc(float freq, float q, t_hip_setup * coeff);
void hip_filter(float * block, long length, int stride, t_hip_setup * coeff);
void write_files(t_alib * setup, long last_slice_pos, long slice_pos);

#endif
