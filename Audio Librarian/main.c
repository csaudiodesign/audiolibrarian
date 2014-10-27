//
//  main.c
//  Audio Librarian
//
//  Created by Cedric Spindler on 29.08.14.
//  Copyright (c) 2014 Zedcat. All rights reserved.
//

#include "main.h"

int main(int argc, const char * argv[])
{
    struct gengetopt_args_info args;
	struct cmdline_parser_params *params;
	params = cmdline_parser_params_create();
	params->initialize = 1;
	params->override = 0;
	params->check_required = 0;
	
	if (cmdline_parser_ext(argc, argv, &args, params) != 0) {
		exit(EXIT_SUCCESS);
	}
    
    /* initialize setup structure */
    t_alib *setup = (t_alib *)malloc(sizeof(t_alib));
    
    /* arg: input file */
    if (args.input_file_given) {
        setup->file_name =  args.input_file_arg;
    } else {
        printf("need input file!\n");
        exit(1);
    }
    
    /* arg: target file name */
    if (args.target_given) {
        setup->target_file_name = args.target_arg;
    } else {
        // TODO: use input filename as base to create output filenames
    }
    
    /* arg: ouput directory name */
    setup->output_path = args.output_path_arg;
    
    /* arg: dry run flag */
    if (args.dry_run_given) setup->dryrun=true;
    else setup->dryrun=false;
    
    /* arg: max/msp progress compatibility flag */
    if (args.max_progress_given) setup->max_progress=true;
    else setup->max_progress=false;
    
    /* arg: tags */
    if (args.tags_given) setup->tags = args.tags_arg;
    else setup->tags = NULL;
    
    init(setup);

    loop(setup);
    
    alib_free(setup);
    
    return 0;
}

void alib_free(t_alib * setup)
{
    free(setup->hop_memory); // here starts the global memory, no need to free other pointers to this part of mem.
}

