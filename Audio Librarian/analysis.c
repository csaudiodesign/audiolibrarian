//
//  analysis.c
//  Audio Librarian
//
//  Created by Cedric Spindler on 01.09.14.
//  Copyright (c) 2014 Zedcat. All rights reserved.
//

#include "analysis.h"

void init(t_alib *setup)
{
    /* initialize setup values */
	setup->block_memory = NULL;
    setup->proc_memory = NULL;
    setup->hop_memory = NULL;
    
    setup->block_size = 400; // ms
    setup->overlap = 4;
    setup->idx = 0;
    setup->slice_idx = 0;
    
    /* load soundfile */
    strtok(setup->file_name, "\n");
    if (!(setup->sound_file = sf_open(setup->file_name, SFM_READ, &setup->sfinfo))) {
        fprintf(stderr, "Not able to open sound file \"%s\"!\n",setup->file_name);
        fprintf(stderr, "libsndfile error: %s\n", (sf_strerror(NULL)));
        exit(1);
    }
    
    setup->sample_rate = setup->sfinfo.samplerate;
    setup->block_size_frames = setup->block_size * setup->sample_rate / 1000;
    setup->hop_size = setup->block_size / setup->overlap;
    setup->hop_size_frames = setup->hop_size * setup->sample_rate / 1000;
    
    void *global_memory;
    global_memory = malloc(
        (setup->block_size_frames*setup->sfinfo.channels * sizeof(float)) + // block_memory
        (setup->block_size_frames*setup->sfinfo.channels * sizeof(float)) + // hop_memory
        (setup->sfinfo.frames / setup->hop_size_frames * sizeof(float)) + // proc memory
        (10*1024*1024*sizeof(float)) // file buffer
    );
    
    if (!global_memory) {
        printf("Could not allocate memory.");
        exit(1);
    }
    /*
     pointers to global memory:
     hop memory is located before proc memory, so it can be accessed in one call
     */
    setup->hop_memory = global_memory;
    global_memory = (void *) ((float *) global_memory + (setup->block_size_frames*setup->sfinfo.channels));
    
    setup->block_memory = global_memory;
    global_memory = (void *) ((float *) global_memory + (setup->block_size_frames*setup->sfinfo.channels));
    
    /* allocate processing buffer aka extracted values */
    setup->proc_memory = global_memory;
    global_memory = (void *) ((float *) global_memory + (setup->sfinfo.frames / setup->hop_size_frames));
    
    setup->file_buffer = global_memory;
    global_memory = (void *) ((float *) global_memory + 10*1024*1024);
    
    memset(setup->hop_memory, 0, setup->block_size_frames*setup->sfinfo.channels);
}

void loop(t_alib * setup)
{
    sf_count_t  length;
//    float       *block_memory_orig;
    float       factor_gain = 0.707106781186548;
    t_hip_setup hip;
    
    hip._two_pi_over_sf = (float)(2*3.14159265)/setup->sample_rate;
    hpcalc(300, 1, &hip);
    
    /* test file output for comparison */
//    SNDFILE     *compare_file;
//    SF_INFO     compare_file_info;
//    
//    compare_file_info.channels = setup->sfinfo.channels;
//    compare_file_info.samplerate = setup->sfinfo.samplerate;
//    compare_file_info.format = setup->sfinfo.format;
//    
//    if (!(compare_file = sf_open("compare2.aif", SFM_WRITE, &compare_file_info))) {
//        fprintf(stderr, "Not able to open target file \"compare.aif\"!\n");
//        fprintf(stderr, "libsndfile error: %s\n", (sf_strerror(NULL)));
//        exit(1);
//    }
    
    /* setup progress indicator */
    int progress_width = 70;
    float progress_factor = progress_width / (float)setup->sfinfo.frames;
    float progress = 0.0;
    
    while ((length = sf_readf_float (setup->sound_file, setup->block_memory, setup->block_size_frames)))
    {
        /* 
         implement vector summing of blockmemory, remove it in energygrain loop: this will eliminate all math regarding channelcount, and save memory.
        
         - is it better to have block_memory and window_memory separate, block for file io, window for analysis?
         - maybe calculate the correlation between channels here. problem: how to handle channelcount > 2? do we care?
         -- is the corellation bewtween channels better at places we want to split files?
        */
        
//        if (setup->sfinfo.channels>1) {
//            float sum;
//            for (int i=0; i<length*setup->sfinfo.channels; i+=setup->sfinfo.channels) {
//                sum = 0;
//                for (int c=0; c<setup->sfinfo.channels; c++) {
//                    sum += setup->block_memory[i+c];
//                }
//                setup->block_memory[i] = (float)sum/setup->sfinfo.channels;
//            }
//        }
        
        if (setup->sfinfo.channels == 2) {
            vDSP_vasm(setup->block_memory, 2, &setup->block_memory[1], 2, &factor_gain, setup->block_memory, 2, length); // summing, interleaved
        }
        
        hip_filter(setup->block_memory, length, setup->sfinfo.channels, &hip);
        
//        block_memory_orig = setup->block_memory;
//        sf_writef_float(compare_file, setup->block_memory, length);
//        setup->block_memory = block_memory_orig;
        
        analyze (setup, length);
        
        /* progress indicator output */
        progress += (float)setup->block_size_frames * progress_factor;
        if(setup->max_progress) printf("analyzing %.1f\n",progress*(100/progress_width));
        else {
            printf("[");
            for (int p=0; p < progress_width; ++p) {
                if(p <= progress) printf("=");
                else printf("-");
            }
            printf("] %.1f%% analyzing...\r",progress*(100/progress_width));
            fflush(stdout);
        }
    }
    
    /* progress indicator output */
    if(setup->max_progress) printf("analyzing 100.0\n");
    else {
        printf("[");
        for (int p=0; p < progress; ++p) printf("=");
        printf("] 100.0%% done.       \n");
    }
    
    search(setup);
    
    sf_close (setup->sound_file);
    sf_close (setup->target_file);
//    sf_close (compare_file);
}

void analyze(t_alib * setup, sf_count_t length)
{
    /* work here on the input */
    float   energyGrainValue = 0;
    float   sumEnergy = 0;
    
    for (int i=0; i < setup->overlap; i++) {
        for (int j=0; j < length; j++) {
            energyGrainValue += pow(setup->hop_memory[((i*setup->hop_size_frames)+j)*setup->sfinfo.channels],2);
        }
        
        setup->proc_memory[setup->idx] = energyGrainValue;
        sumEnergy += energyGrainValue;
        
        energyGrainValue = 0;
        setup->idx++;
    }
    
    memcpy(setup->hop_memory, setup->block_memory, setup->block_size_frames*setup->sfinfo.channels*sizeof(float));
}

void search(t_alib * setup)
{
    long    j=0;
    float   averageEnergy=0;
    long    proc_memory_len = setup->sfinfo.frames / setup->hop_size_frames;
    long    slice_pos = 0;
    long    last_slice_pos = 0;
    
    while (j < proc_memory_len) {
        
        /* reset average energy */
        averageEnergy = 0;
        
        /* accum the surrounding 60 grains (this should be the surrounding 6 seconds) */
        for(int i = 0; i < 60; i++){

            if (j+30 < proc_memory_len){
                averageEnergy += setup->proc_memory[abs((int)(j-30+i))];
            }
        }
        
        /* calculate the average */
        averageEnergy /= (float)60.0;
        
        /* scale by magic number */
        averageEnergy *= 0.1;
        
        if (
            setup->proc_memory[j] <  averageEnergy
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 1)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 2)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 3)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 4)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 5)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 6)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 7)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 8)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 9)]
            && setup->proc_memory[j] < setup->proc_memory[abs((int)j - 10)]
            && setup->proc_memory[j] < setup->proc_memory[j + 1]
            && setup->proc_memory[j] < setup->proc_memory[j + 2]
            && setup->proc_memory[j] < setup->proc_memory[j + 3]
            && setup->proc_memory[j] < setup->proc_memory[j + 4]
            && setup->proc_memory[j] < setup->proc_memory[j + 5]
            && setup->proc_memory[j] < setup->proc_memory[j + 6]
            && setup->proc_memory[j] < setup->proc_memory[j + 7]
            && setup->proc_memory[j] < setup->proc_memory[j + 8]
            && setup->proc_memory[j] < setup->proc_memory[j + 9]
            && setup->proc_memory[j] < setup->proc_memory[j + 10])
        {
            /* do something */
            slice_pos = j*setup->hop_size_frames;

            /* find zero crossings */
            
            /* fade in and fade out */
            
            /* create directory for new files */
            if (slice_pos == 0 && !setup->dryrun) {
                if (mkdir(setup->output_path, 0777))
                    perror(setup->output_path);
            }
            
            if (slice_pos > 0) {
                printf("%ld, %ld; \n", setup->slice_idx, slice_pos);
                write_files(setup, last_slice_pos, slice_pos);
            }
            
            last_slice_pos = slice_pos;
            setup->slice_idx++;
        }
        j++;
    }
    
    /* write last slice, to the end of the file */
    printf("%ld, %ld; \n", setup->slice_idx, proc_memory_len*setup->hop_size_frames);
    write_files(setup, last_slice_pos, proc_memory_len*setup->hop_size_frames);
}

void write_files(t_alib * setup, long last_slice_pos, long slice_pos)
{
    long        length = slice_pos - last_slice_pos;
    long        read_length = 10*1024*1024/(setup->sfinfo.channels*sizeof(float));
    long        slice_length = 0;
    sf_count_t  write_length = 0;
    
    char        target_filename[512];
    char        target_comment[512];
    char        al_name[16] = "Audio Librarian";
    
    
    snprintf(target_filename, sizeof target_filename, "%s/%s %04ld %s.aif", setup->output_path, setup->target_file_name, setup->slice_idx, setup->tags);
    
    setup->target_info.channels = setup->sfinfo.channels;
    setup->target_info.samplerate = setup->sfinfo.samplerate;
    setup->target_info.format = setup->sfinfo.format;
    
    if (!setup->dryrun) {
        if (!(setup->target_file = sf_open(target_filename, SFM_WRITE, &setup->target_info))) {
            fprintf(stderr, "Not able to open target file \"%s\"!\n",setup->target_file_name);
            fprintf(stderr, "libsndfile error: %s\n", (sf_strerror(NULL)));
            exit(1);
        }
    }
    
//    snprintf(target_comment, sizeof target_comment, "Slice-Nr: %ld, Position in original: %ld", setup->slice_idx, slice_pos);
    snprintf(target_comment, sizeof target_comment, "%04ld: %s", setup->slice_idx, setup->tags); //A dd tags to the comments, from stdinput
    if (!setup->dryrun) {
        sf_set_string(setup->target_file, SF_STR_TITLE, setup->file_name);  // write original filename (or path) to "Title/Name"
        sf_set_string(setup->target_file, SF_STR_COMMENT, target_comment);  // write slice index and tags to "Comments/Annotation"
        sf_set_string(setup->target_file, SF_STR_ARTIST, al_name);          // write "Audio Librarian" to "Artist/Author"
    }
    
    sf_seek(setup->sound_file, last_slice_pos, SEEK_SET);
    
    if (read_length > length) {
        sf_readf_float(setup->sound_file, setup->file_buffer, length);
        if (!setup->dryrun) sf_writef_float(setup->target_file, setup->file_buffer, length);
    }
    else {
        while (slice_length < length) {
            
            sf_readf_float(setup->sound_file, setup->file_buffer, read_length);
            
            if ((length - slice_length) < read_length) write_length = length - slice_length;
            else write_length = read_length;
            
            if (!setup->dryrun) sf_writef_float(setup->target_file, setup->file_buffer, write_length);
            
            slice_length += read_length;
        }
    }
    
    if (!setup->dryrun) sf_close(setup->target_file);
    
}

void hpcalc(float freq, float q, t_hip_setup * coeff)
{
    float w = coeff->_two_pi_over_sf * freq;
    float cosw = (float) cos(w);
    float a = (float) sin(w) / q * .5;
    coeff->b1 = -1. - cosw;
    coeff->b0 = coeff->b1 * -.5;
    coeff->b2 = coeff->b0;
    coeff->a0 = 1. + a;
    coeff->a1 = -2. * cosw;
    coeff->a2 = 1. - a;
    
    coeff->b0 /= coeff->a0;
    coeff->b1 /= coeff->a0;
    coeff->b2 /= coeff->a0;
    coeff->a1 /= coeff->a0;
    coeff->a2 /= coeff->a0;
    
    coeff->x1 = 0;
    coeff->x2 = 0;
    coeff->y1 = 0;
    coeff->y2 = 0;
}

void hip_filter(float * block, long length, int stride, t_hip_setup * coeff)
{
    long    i = 0;
    double  y = 0.0;
    
    while (i < length*stride) {
        y = coeff->b0 * block[i] + coeff->b1 * coeff->x1 + coeff->b2 * coeff->x2 - coeff->a1 * coeff->y1 - coeff->a2 * coeff->y2;
        coeff->x2 = coeff->x1;
        coeff->x1 = block[i];
        coeff->y2 = coeff->y1;
        coeff->y1 = y;
        block[i] = (float) y;
        i += 2;
    }
}
