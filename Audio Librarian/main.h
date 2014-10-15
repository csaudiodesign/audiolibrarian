//
//  main.h
//  Audio Librarian
//
//  Created by Cedric Spindler on 29.08.14.
//  Copyright (c) 2014 Zedcat. All rights reserved.
//

#ifndef cs_init_main_h
#define cs_init_main_h

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include "sndfile.h"
#include "cmdline.h"

//#include <CoreServices/CoreServices.h>
#include <Accelerate/Accelerate.h>
//#include <Carbon/Carbon.h>

//void split(struct gengetopt_args_info args);

typedef struct alib {
	char        *file_name;
    int         sample_rate;
    int         block_size;         // ms
    int         block_size_frames;
    int         hop_size;           // ms
    int         hop_size_frames;
    int         overlap;
    float       *block_memory;
    float       *proc_memory;
    float       *hop_memory;
    long        idx;                // Index for the grains
    long        slice_idx;          // Index for the detected slices
    SNDFILE     *sound_file;
    SF_INFO     sfinfo;
    float       *file_buffer;
    
	char        *target_file_name;
    SNDFILE     *target_file;
    SF_INFO     target_info;
    
    char        *tags;
    
    char        *output_path;
    bool        dryrun;
} t_alib;

typedef struct analysis {
    float       *rms;
    float       *rms_average;
    
} t_analysis;

/* highpass filter coeffs */
typedef struct hip {
    /* coeffs */
    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    
    /* feedback memory */
    double x1;
    double x2;
    double y1;
    double y2;
    
    float _two_pi_over_sf;
} t_hip_setup;

/* function prototypes */
void loop(t_alib * setup);
void alib_free(t_alib * setup);
void init();

#endif
