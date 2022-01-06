// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "suppression_conf.h"
#include "suppression.h"

#include "fileio.h"
#include "wav_utils.h"

#define MAX_CHANNELS 2

extern void sup_process_frame(sup_state_t * state,
                        int32_t output [SUP_FRAME_ADVANCE],
                        const int32_t input[SUP_FRAME_ADVANCE]);

void sup_task(const char *input_file_name, const char *output_file_name){
    file_t input_file, output_file;

    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open output wav file that will contain the Noise Suppressor output
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct, output_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in att_get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 32bit wav file
    if(input_header_struct.bit_depth != 32)
    {
        printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header_struct.bit_depth, input_file_name);
        _Exit(1);
    }
    // Ensure input wav file contains correct number of channels 
    if(input_header_struct.num_channels != MAX_CHANNELS){
        printf("Error: wav num channels(%d) does not match aec(%u)\n", input_header_struct.num_channels, MAX_CHANNELS);
        _Exit(1);
    }

    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / SUP_FRAME_ADVANCE;
    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            MAX_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count * SUP_FRAME_ADVANCE);

    int32_t input_read_buffer[SUP_FRAME_ADVANCE * MAX_CHANNELS] = {0}; // Array for storing interleaved input read from wav file
    int32_t output_write_buffer[SUP_FRAME_ADVANCE * MAX_CHANNELS];

    int32_t DWORD_ALIGNED frame[MAX_CHANNELS][SUP_FRAME_ADVANCE];
    
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    //Initialise noise suppressor
    sup_state_t DWORD_ALIGNED ch1_state;
    sup_state_t DWORD_ALIGNED ch2_state;

    sup_init_state(&ch1_state);
    sup_init_state(&ch2_state);

    for(int b = 0; b < block_count; b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * SUP_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame * SUP_FRAME_ADVANCE);
        // Deinterleave and copy samples to their respective buffers
        for(unsigned f = 0; f < SUP_FRAME_ADVANCE; f++){
            for(unsigned ch = 0; ch < MAX_CHANNELS; ch++){
                unsigned i =(f * MAX_CHANNELS) + ch;
                frame[ch][f] = input_read_buffer[i];
            }
        }
        // Call Noise Suppression functions to process SUP_FRAME_ADVANCE new samples of data
        // Reuse mic data memory for main filter output
        sup_process_frame(&ch1_state, frame[0], frame[0]);
        sup_process_frame(&ch1_state, frame[1], frame[1]);

        // Create interleaved output that can be written to wav file
        for (unsigned ch = 0; ch < MAX_CHANNELS; ch++){
            for(unsigned i = 0; i < SUP_FRAME_ADVANCE; i++){
                output_write_buffer[i*(MAX_CHANNELS) + ch] = frame[ch][i];
            }
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * SUP_FRAME_ADVANCE * MAX_CHANNELS);
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if X86_BUILD
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    sup_task(argv[1], argv[2]);
    return 0;
}
#endif