//
//  SCOpusAPI_ios.h
//  SCOpusAPI_ios
//
//  Created by SongJian on 15/10/28.
//  Copyright (c) 2015年 SongJian. All rights reserved.
//

#ifndef __SCOpusAPI_ios__SCOpusAPI_ios__
#define __SCOpusAPI_ios__SCOpusAPI_ios__


#include "opus.h"

#define MAX_PACKET  1500
#define SAMPLE_RATE 48000
#define CHANNELS    2
#define BIT_RATE    12800


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
int encode_with_opus(opus_int32 sampling_rate, int channels, unsigned long bitrate_bps, char *in_file, char *out_file);



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
int decode_with_opus(opus_int32 sampling_rate, int channels, int is_add_header, char *in_file, char *out_file);




#endif /* defined(__SCOpusAPI_ios__SCOpusAPI_ios__) */
