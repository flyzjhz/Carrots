//
//  SCOpusAPI_ios.c
//  SCOpusAPI_ios
//
//  Created by SongJian on 15/10/28.
//  Copyright (c) 2015年 SongJian. All rights reserved.
//

#include "SCOpusAPI_ios.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "opus.h"
#include "opus_types.h"
#include "opus_multistream.h"



// ---------- WAVE  ----------
// RIFF WAVE Chunk
typedef struct {
    char	wh_id[4];
    int		wh_size;
    char	wh_type[4];
} header_wave_st;

typedef struct {
    char			wh_id[4];			// 'fmt '
    unsigned int	wh_size;			// 1618, 18
    unsigned short	wh_format_tag;		// : 0x0001
    unsigned short	wh_channels;		// : 1, 2
    unsigned int	wh_samples_per_sec;	// 
    unsigned int	wh_avg_bytes_per_sec;	//  ( *  *  ) / 8
    unsigned short	wh_block_align;		//  ()   *  / 8
    unsigned short	wh_uibits_per_sample;	// : 8, 16  (bit)
} header_fmt_st;

typedef struct {
    char			wh_id[4];
    unsigned int	wh_size;
} header_data_st;



static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
    | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}



void encode_clean(FILE *fin, FILE *fout, OpusEncoder *enc, short *in, short *out, unsigned char *data, unsigned char *fbytes)
{
    if (enc != NULL) {
        opus_encoder_destroy(enc);
        enc = NULL;
    }
    if (fin != NULL) {
        fclose(fin);
        fin = NULL;
    }
    if (fout != NULL) {
        fclose(fout);
        fout = NULL;
    }
    if (in != NULL) {
        free(in);
        in = NULL;
    }
    if (out != NULL) {
        free(out);
        out = NULL;
    }
    if (fbytes != NULL) {
        free(fbytes);
        fbytes = NULL;
    }
    if (data != NULL) {
        free(data);
        data = NULL;
    }
    
    return;
}


/**
 *  音频编码
 *
 *  @param sampling_rate 48000
 *  @param channels      2
 *  @param bitrate_bps   12800
 *  @param in_file       原始音频文件
 *  @param out_file      编码后的文件
 *
 *  @return 0:成功    1:失败
 */
int encode_with_opus(opus_int32 sampling_rate, int channels, unsigned long bitrate_bps, char *in_file, char *out_file)
{
    // check invalid
    if (sampling_rate != 8000 && sampling_rate != 12000
        && sampling_rate != 16000 && sampling_rate != 24000
        && sampling_rate != 48000) {
        
        fprintf(stderr, "Supported sampling rates are 8000, 12000, "
                "16000, 24000 and 48000.\n");
        return 1;
    }
    if (channels < 1 || channels > 2) {
        fprintf(stderr, "Opus_demo supports only 1 or 2 channels.\n");
        return 1;
    }
    if (in_file == NULL || out_file == NULL) {
        fprintf(stderr, "in_file or out_file is null");
        return 1;
    }
    
    
    FILE *fin = NULL;
    FILE *fout = NULL;
    OpusEncoder *enc = NULL;
    short *in = NULL, *out = NULL;
    unsigned char *data = NULL;
    unsigned char *fbytes = NULL;
    
    int err;
    int stop = 0;
    int application = OPUS_APPLICATION_VOIP;
    int frame_size = (int)sampling_rate/50;
    int max_payload_bytes = MAX_PACKET;
    int max_frame_size = 2*48000;
    int remaining = 0;
    int curr_read = 0;
    opus_uint64 tot_in, tot_out;
    tot_in = tot_out = 0;
    int len;
    int nb_encoded = 0;
    opus_uint32 enc_final_range;
    int nb_modes_in_list = 0;
    int curr_mode = 0;
    int curr_mode_count = 0;
    int mode_switch_time = 48000;
    double tot_samples = 0;
    int lost = 0, lost_prev = 1;
    double bits=0.0, bits_max=0.0, bits_act=0.0, bits2=0.0, nrg;
    opus_int32 count=0, count_act=0;
    int k;
    
    
    fin = fopen(in_file, "rb");
    if (fin == NULL) {
        fprintf(stderr, "Could not open input file %s\n", in_file);
        
        encode_clean(fin, fout, enc, in, out, data, fbytes);
        return 1;
    }
    
    fout = fopen(out_file, "wb+");
    if (fout == NULL) {
        fprintf(stderr, "Could not open output file %s\n", out_file);
        
        
        encode_clean(fin, fout, enc, in, out, data, fbytes);
        return 1;
    }
    
    
    enc = opus_encoder_create((opus_int32)sampling_rate, channels, application, &err);
    if (err != OPUS_OK) {
        fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(err));
        
        encode_clean(fin, fout, enc, in, out, data, fbytes);
        return 1;
    }
    
    
    int bandwidth = -1;
    int use_vbr = 1;
    int cvbr = 0;
    int complexity = 10;
    int use_inbandfec = 0;
    int forcechannels = OPUS_AUTO;
    int use_dtx = 0;
    int packet_loss_perc = 0;
    opus_int32 skip = 0;
    int variable_duration = OPUS_FRAMESIZE_ARG;
    const char *bandwidth_string = "auto";
    
    opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
    opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(bandwidth));
    opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
    opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
    opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(use_inbandfec));
    opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
    opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
    opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));
    
    opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
    opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration));
    
    
    fprintf(stderr, "Encoding %ld Hz input at %.3f kb/s "
            "in %s mode with %d-sample frames.\n",
            (long)sampling_rate, bitrate_bps*0.001,
            bandwidth_string, frame_size);
    
    
    in = (short *)malloc(max_frame_size * channels * sizeof(short));
    out = (short *)malloc(max_frame_size * channels * sizeof(short));
    fbytes = (unsigned char *)malloc(max_frame_size * channels * sizeof(short));
    data = (unsigned char *)calloc(max_payload_bytes, sizeof(char));
    
    if (in == NULL || out == NULL || fbytes == NULL || data == NULL) {
        fprintf(stderr, "malloc fail\n");
        
        encode_clean(fin, fout, enc, in, out, data, fbytes);
        return 1;
    }
    
    
    while (!stop) {
        int i;
        err = (int)fread(fbytes, sizeof(short) * channels, frame_size - remaining, fin);
        curr_read = err;
        tot_in += curr_read;
        for (i=0; i<curr_read*channels; i++) {
            opus_int32 s = fbytes[2*i+1] << 8 | fbytes[2*i];
            s = ((s & 0xFFFF) ^ 0x8000) - 0x8000;
            in[i+remaining*channels] = s;
        }
        
        if (curr_read + remaining < frame_size) {
            for (i=(curr_read + remaining)*channels; i<frame_size*channels; i++)
                in[i] = 0;
            
            stop = 1;
        }
        
        len = opus_encode(enc, in, frame_size, data, max_payload_bytes);
        nb_encoded = opus_packet_get_samples_per_frame(data, (opus_int32)sampling_rate) * opus_packet_get_nb_frames(data, len);
        remaining = frame_size - nb_encoded;
        
        for (i=0; i<remaining*channels; i++)
            in[i] = in[nb_encoded*channels+i];
        
        
        opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
        
        if (len < 0) {
            fprintf (stderr, "opus_encode() returned %d\n", len);
            
            encode_clean(fin, fout, enc, in, out, data, fbytes);
            return 1;
        }
        
        curr_mode_count += frame_size;
        if (curr_mode_count > mode_switch_time && curr_mode < nb_modes_in_list-1) {
            curr_mode++;
            curr_mode_count = 0;
        }
        
        
        unsigned char int_field[4];
        int_to_char(len, int_field);
        if (fwrite(int_field, 1, 4, fout) != 4) {
            fprintf(stderr, "Error writing.\n");
            
            encode_clean(fin, fout, enc, in, out, data, fbytes);
            return 1;
        }
        
        int_to_char(enc_final_range, int_field);
        if (fwrite(int_field, 1, 4, fout) != 4) {
            fprintf(stderr, "Error writing.\n");
            
            encode_clean(fin, fout, enc, in, out, data, fbytes);
            return 1;
        }
        
        if (fwrite(data, 1, len, fout) != (unsigned)len) {
            fprintf(stderr, "Error writing.\n");
            
            encode_clean(fin, fout, enc, in, out, data, fbytes);
            return 1;
        }
        
        tot_samples += nb_encoded;
        
        lost_prev = lost;
        
        
        // count bits
        bits += len * 8;
        bits_max = (len * 8 > bits_max) ? len * 8 : bits_max;
        if ( count >= use_inbandfec ) {
            nrg = 0.0;
            for (k=0; k<frame_size*channels; k++) {
                nrg += in[k] * (double)in[k];
            }
            
            if ( (nrg/(frame_size * channels)) > 1e5 ) {
                bits_act += len * 8;
                count_act++;
            }
            
            // Variance
            bits2 += len * len * 64;
        }
        
        count++;
        
    }
    
    fprintf (stderr, "average bitrate:             %7.3f kb/s\n",
             1e-3*bits*sampling_rate/tot_samples);
    fprintf (stderr, "maximum bitrate:             %7.3f kb/s\n",
             1e-3*bits_max*sampling_rate/frame_size);
    fprintf (stderr, "active bitrate:              %7.3f kb/s\n",
             1e-3*bits_act*sampling_rate/(frame_size*(double)count_act));
    fprintf (stderr, "bitrate standard deviation:  %7.3f kb/s\n",
             1e-3*sqrt(bits2/count - bits*bits/(count*(double)count))*sampling_rate/frame_size);
    
    
    encode_clean(fin, fout, enc, in, out, data, fbytes);

    return 0;
    
}




void decode_clean(FILE *fin, FILE *fout, OpusDecoder *dec, short *in, short *out, unsigned char *data, unsigned char *fbytes)
{
    if (dec != NULL) {
        opus_decoder_destroy(dec);
        dec = NULL;
    }
    if (fin != NULL) {
        fclose(fin);
        fin = NULL;
    }
    if (fout != NULL) {
        fclose(fout);
        fout = NULL;
    }
    if (in != NULL) {
        free(in);
        in = NULL;
    }
    if (out != NULL) {
        free(out);
        out = NULL;
    }
    if (fbytes != NULL) {
        free(fbytes);
        fbytes = NULL;
    }
    if (data != NULL) {
        free(data);
        data = NULL;
    }
    
    return ;
}


/**
 *  音频解码
 *
 *  @param sampling_rate 48000
 *  @param channels      2
 *  @param is_add_header 1: 是否加wav头文件
 *  @param in_file       编码后的文件
 *  @param out_file      解码后的文件
 *
 *  @return 0:成功    1:失败
 */
int decode_with_opus(opus_int32 sampling_rate, int channels, int is_add_header, char *in_file, char *out_file)
{
    // check invalid
    if (sampling_rate != 8000 && sampling_rate != 12000
        && sampling_rate != 16000 && sampling_rate != 24000
        && sampling_rate != 48000) {
        
        fprintf(stderr, "Supported sampling rates are 8000, 12000, "
                "16000, 24000 and 48000.\n");
        return 1;
    }
    if (channels < 1 || channels > 2) {
        fprintf(stderr, "Opus_demo supports only 1 or 2 channels.\n");
        return 1;
    }
    if (in_file == NULL || out_file == NULL) {
        fprintf(stderr, "in_file or out_file is null");
        return 1;
    }
    
    
    FILE *fin = NULL;
    FILE *fout = NULL;
    OpusDecoder *dec = NULL;
    short *in = NULL, *out = NULL;
    unsigned char *data = NULL;
    unsigned char *fbytes = NULL;
    
    int frame_size = (int)sampling_rate/50;
    
    int err;
    int max_payload_bytes = MAX_PACKET;
    //const char *bandwidth_string = "auto";
    int max_frame_size = 2 * 48000;
    int stop = 0;
    int len = 0;
    int lost = 0;
    int lost_prev = 1;
    int packet_loss_perc = 0;
    opus_int32 count = 0, count_act = 0;
    int use_inbandfec = 0;
    opus_int32 skip = 0;
    opus_uint64 tot_in, tot_out;
    double tot_samples = 0;
    double bits=0.0, bits_max=0.0, bits_act=0.0, bits2=0.0, nrg;
    opus_uint32 dec_final_range;
    
    tot_in = tot_out = 0;
    
    
    fin = fopen(in_file, "rb");
    if (fin == NULL) {
        fprintf(stderr, "Could not open input file %s\n", in_file);
        
        decode_clean(fin, fout, dec, in, out, data, fbytes);
        return 1;
    }
    
    fout = fopen(out_file, "wb+");
    if (fout == NULL) {
        fprintf(stderr, "Could not open output file %s\n", out_file);
        
        decode_clean(fin, fout, dec, in, out, data, fbytes);
        return 1;
    }
    
    // ----- add wave header -----
    header_wave_st pcm_header;
    header_fmt_st pcm_fmt;
    header_data_st pcm_data;
    if (is_add_header == 1) {
        // ------ wavHEADER ------
        // wavHEADER;
        // .wh_sizeData
        memcpy(pcm_header.wh_id, "RIFF", 4);
        memcpy(pcm_header.wh_type, "WAVE", 4);
        
        // HEADERwav;
        fseek(fout, sizeof(header_wave_st), SEEK_SET);
        // ------ wavHEADER ------
        
        // ------ wavFMT ------
        memcpy(pcm_fmt.wh_id, "fmt ", 4);
        pcm_fmt.wh_size = 16;
        pcm_fmt.wh_format_tag = 1;
        pcm_fmt.wh_channels = channels;
        pcm_fmt.wh_samples_per_sec = (opus_int32)sampling_rate;
        pcm_fmt.wh_uibits_per_sample = 16;
        
        pcm_fmt.wh_avg_bytes_per_sec = (pcm_fmt.wh_samples_per_sec * pcm_fmt.wh_uibits_per_sample * pcm_fmt.wh_channels) / 8;
        pcm_fmt.wh_block_align = (pcm_fmt.wh_channels * pcm_fmt.wh_uibits_per_sample) / 8;
        // ------ wavFMT ------
        
        //fwrite(&pcm_fmt, sizeof(header_fmt_st), 1, fout);
        fwrite(&pcm_fmt, sizeof(pcm_fmt), 1, fout);
        
        
        // ------ wavDATA ------
        // wavDATA;
        // DATA.dwsize.wav
        memcpy(pcm_data.wh_id, "data", 4);
        pcm_data.wh_size = 0;	// pcm_data.wh_size 0
        // ------ wavDATA ------
        
        // DATAwavDATA
        fseek(fout, sizeof(header_data_st), SEEK_CUR);
    }
    
    
    dec = opus_decoder_create((opus_int32)sampling_rate, channels, &err);
    if (err != OPUS_OK) {
        fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(err));
        
        decode_clean(fin, fout, dec, in, out, data, fbytes);
        return 1;
    }
    
    fprintf(stderr, "Decoding with %ld Hz output (%d channels)\n",(long)sampling_rate, channels);
    
    in = (short *)malloc(max_frame_size * channels * sizeof(short));
    out = (short *)malloc(max_frame_size * channels * sizeof(short));
    fbytes = (unsigned char *)malloc(max_frame_size * channels * sizeof(short));
    data = (unsigned char *)calloc(max_payload_bytes, sizeof(char));
    
    fprintf(stderr, "out:%p", out);
    
    if (in == NULL || out == NULL || fbytes == NULL || data == NULL) {
        fprintf(stderr, "malloc fail\n");
        
        decode_clean(fin, fout, dec, in, out, data, fbytes);
        return 1;
    }
    
    int i=0;
    while (!stop) {
        fprintf(stderr, "[%d]", i);
        i++;
        
        unsigned char ch[4];
        
        err = (int)fread(ch, 1, 4, fin);
        
        if (feof(fin))
            break;
        
        len = char_to_int(ch);
        if (len > max_payload_bytes || len < 0) {
            fprintf(stderr, "Invalid payload length: %d\n", len);
            
            decode_clean(fin, fout, dec, in, out, data, fbytes);
            return 1;
        }
        
        err = (int)fread(ch, 1, 4, fin);
        
        //enc_final_range = char_to_int(ch);
        
        err = (int)fread(data, 1, len, fin);
        if (err < len) {
            fprintf(stderr, "Ran out of input, expecting %d bytes got %d\n", len, err);
            
            decode_clean(fin, fout, dec, in, out, data, fbytes);
            return 1;
        }
        
        
        // ----
        int output_samples;
        lost = len == 0 || (packet_loss_perc > 0 && rand()%100 < packet_loss_perc);
        if (lost) {
            opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION(&output_samples));
        } else {
            output_samples = max_frame_size;
        }
        
        if (count >= use_inbandfec) {
            // delay by one packet when using in-band FEC
            output_samples = opus_decode(dec, lost ? NULL : data, len, out, output_samples, 0);
            
            if (output_samples > 0) {
                if (output_samples > skip) {
                    int i;
                    for (i=0; i<((output_samples - skip)*channels); i++) {
                        short s = out[i + (skip * channels)];
                        fbytes[2*i] = s & 0xFF;
                        fbytes[2*i+1] = (s >> 8) & 0xFF;
                    }
                    
                    int wn = (int)fwrite(fbytes, sizeof(short)*channels, output_samples - skip, fout);
                    if ( wn != (unsigned)(output_samples - skip) ) {
                        fprintf(stderr, "Error writing.\n");
                        
                        decode_clean(fin, fout, dec, in, out, data, fbytes);
                        return 1;
                    }
                    
                    if (is_add_header == 1)
                        pcm_data.wh_size += (sizeof(short)*channels) * (output_samples - skip);
                    
                    tot_out += output_samples - skip;
                    
                }
                
                if (output_samples < skip)
                    skip -= output_samples;
                else
                    skip = 0;
                
            } else {
                fprintf(stderr, "error decoding frame: %s output_samples:%d\n", opus_strerror(output_samples), output_samples);
            }
            
            tot_samples += output_samples;
        }
        
        
        opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(&dec_final_range));
        
        lost_prev = lost;
        
        // count bits
        bits += len * 8;
        bits_max = (len * 8 > bits_max) ? len * 8 : bits_max;
        if ( count >= use_inbandfec ) {
            nrg = 0.0;
            if ( (nrg / (frame_size * channels)) > 1e5 ) {
                bits_act += len * 8;
                count_act++;
            }
            
            // Variance
            bits2 += len * len * 64;
        }
        count++;
        // = (toggle + use_inbandfec) & 1;
    }
    
    fprintf (stderr, "average bitrate:             %7.3f kb/s\n",
             1e-3*bits*sampling_rate/tot_samples);
    fprintf (stderr, "maximum bitrate:             %7.3f kb/s\n",
             1e-3*bits_max*sampling_rate/frame_size);
    fprintf (stderr, "bitrate standard deviation:  %7.3f kb/s\n",
             1e-3*sqrt(bits2/count - bits*bits/(count*(double)count))*sampling_rate/frame_size);
    
    
    if (is_add_header == 1) {
        //  pcm_data.wh_size  pcm_header.wh_size 
        pcm_header.wh_size = 44 + pcm_data.wh_size;	// 271724
        fprintf(stderr, "header.wh_size:%d\n", pcm_header.wh_size);
        //pcm_header.wh_size = 1086764;
        
        rewind(fout);	//  fout .wavHEADERDATA;
        fwrite(&pcm_header, sizeof(header_wave_st), 1, fout);	// HEADER
        
        fseek(fout, sizeof(header_fmt_st), SEEK_CUR);	// FMT,FMT
        fwrite(&pcm_data, sizeof(header_data_st), 1, fout);		// DATA;
    }
    
    
    // Close any files to which intermediate results were stored
    
    decode_clean(fin, fout, dec, in, out, data, fbytes);
    
    return 0;
    
}
