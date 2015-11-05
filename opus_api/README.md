ctmfile
=======

Intro
-----

[opus_api] is a api of use opus to encode/decode for audio.


Installation
------------

#### Build

1. Auto Download opus and build for ios
```bash
a. to change SDKVERSION, MINIOSVERSION in build-libopus.sh
b. bash ./build-libopus.sh
```

2. Build Create libSCOpusAPI static library
a. xcode ---> File ---> New ---> Project ---> iOS ---> Framework & Library ---> Cocoa Touch Static Library
b. drag SCOpusAPI_ios.c and SCOpusAPI_ios.h to your project
c. delete files which auto create.
d. change Build Settings ---> Architectures ---> Build Active Architecture Only ---> NO


### Usage

1. if you want to encode your voice to small size file
```bash
int encRes = encode_with_opus(CODEC_VOICE_SAMPLE_RATE, CODEC_VOICE_CHANNELS, OPUS_APPLICATION_AUDIO, (char *)[origAudioFile UTF8String], (char *)[compressFile UTF8String]);
```

2. if you want to decode the smaill size audio file to original audio
```bash
int encRes = decode_with_opus(CODEC_VOICE_SAMPLE_RATE, CODEC_VOICE_CHANNELS, (char *)[wavFileEnc UTF8String], (char *)[wavFileDec UTF8String]);
```



