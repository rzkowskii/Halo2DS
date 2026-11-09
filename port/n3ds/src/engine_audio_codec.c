/* Xbox IMA: 36 bytes/channel per block, 64 output samples/channel.
 * Header predictor is sample0; the last high nibble is padding (unlike IMA WAV).
 * Stereo payload uses alternating four-byte groups. No heap allocation. */
#include <stdint.h>
#include "engine_algorithm_features.h"
#include "engine_audio.h"
#if HALO_N3DS_ALGO_ADPCM_LOOPS == N3DS_ALGORITHM_CANDIDATE && defined(__ARM_FEATURE_SAT)
#include <arm_acle.h>
#endif
static const int steps[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int adjust[8]={-1,-1,-1,-1,2,4,6,8};
#if HALO_N3DS_ALGO_ADPCM_TABLE == N3DS_ALGORITHM_CANDIDATE
static int32_t lookup[89][16];
#else
struct decode_step { int delta,index; };
static struct decode_step lookup[89][16];
#endif
static int lookup_ready;
static void initialize_lookup(void)
{
    if(lookup_ready) return;
    for(int index=0;index<89;++index) for(int code=0;code<16;++code) {
        int delta=((2*(code&7)+1)*steps[index])>>3;
        int next=index+adjust[code&7];if(next<0) next=0;if(next>88) next=88;
        if(code&8) delta=-delta;
#if HALO_N3DS_ALGO_ADPCM_TABLE == N3DS_ALGORITHM_CANDIDATE
        /* Signed delta needs 17 bits. Shift it above the low byte, which
         * holds the next step index. Multiplication also handles negatives. */
        lookup[index][code]=delta*256+next;
#else
        lookup[index][code]=(struct decode_step){delta,next};
#endif
    }
    lookup_ready=1;
}

static inline int decode_nibble(unsigned int code,int predictor,int *index)
{
#if HALO_N3DS_ALGO_ADPCM_TABLE == N3DS_ALGORITHM_CANDIDATE
    int32_t step=lookup[*index][code];
    *index=(uint32_t)step&255u;
    predictor+=step>>8;
#else
    const struct decode_step *step=&lookup[*index][code];
    *index=step->index;
    predictor+=step->delta;
#endif
#if HALO_N3DS_ALGO_ADPCM_LOOPS == N3DS_ALGORITHM_CANDIDATE && defined(__ARM_FEATURE_SAT)
    return __ssat(predictor,16);
#else
    if(predictor>32767) predictor=32767;
    if(predictor< -32768) predictor=-32768;
    return predictor;
#endif
}

#if HALO_N3DS_ALGO_ADPCM_LOOPS == N3DS_ALGORITHM_CANDIDATE
static inline void decode_pairs(const unsigned char *payload,short *out,
    int pairs,int stride,int *predictor,int *index)
{
    for(int i=0;i<pairs;++i) {
        unsigned int byte=payload[i];
        *predictor=decode_nibble(byte&15,*predictor,index);
        out[0]=*predictor;
        *predictor=decode_nibble(byte>>4,*predictor,index);
        out[stride]=*predictor;
        out+=2*stride;
    }
}

static void decode_mono_block(const unsigned char *block,short *out)
{
    int predictor=(short)(block[0]|(block[1]<<8)),index=block[2];
    out[0]=predictor;
    decode_pairs(block+4,out+1,31,1,&predictor,&index);
    out[63]=decode_nibble(block[35]&15,predictor,&index);
}

static void decode_stereo_block(const unsigned char *block,short *out)
{
    for(int channel=0;channel<2;++channel) {
        const unsigned char *header=block+4*channel;
        const unsigned char *payload=block+8+4*channel;
        short *samples=out+channel;
        int predictor=(short)(header[0]|(header[1]<<8)),index=header[2];
        samples[0]=predictor;
        samples+=2;
        for(int group=0;group<7;++group) {
            decode_pairs(payload,samples,4,2,&predictor,&index);
            payload+=8;
            samples+=16;
        }
        decode_pairs(payload,samples,3,2,&predictor,&index);
        samples[12]=decode_nibble(payload[3]&15,predictor,&index);
    }
}
#endif

unsigned int n3ds_xbox_adpcm_decode(const unsigned char *data,unsigned int bytes,int channels,short *pcm,unsigned int capacity)
{
    if(!data || !pcm || channels<1 || channels>2 || !bytes || bytes%(36*channels)) return 0;
    unsigned int blocks=bytes/(36*channels);
    if(blocks>capacity/(64*channels)) return 0;
    /* Reject invalid indices before publishing partial decoded output. */
    for(unsigned int b=0;b<blocks;++b) for(int c=0;c<channels;++c)
        if(data[b*36*channels+c*4+2]>88 || data[b*36*channels+c*4+3]) return 0;
    initialize_lookup();
#if HALO_N3DS_ALGO_ADPCM_LOOPS == N3DS_ALGORITHM_CANDIDATE
    if(channels==1) {
        for(unsigned int b=0;b<blocks;++b) decode_mono_block(data+b*36,pcm+b*64);
    } else {
        for(unsigned int b=0;b<blocks;++b) decode_stereo_block(data+b*72,pcm+b*128);
    }
#else
    for(unsigned int b=0;b<blocks;++b) for(int c=0;c<channels;++c) {
        const unsigned char *p=data+b*36*channels;
        int predictor=(short)(p[c*4]|(p[c*4+1]<<8)),index=p[c*4+2];
        short *out=pcm+b*64*channels+c;out[0]=predictor;
        for(int i=0;i<63;++i) {
            int v=p[4*channels+(i/8*channels+c)*4+(i%8)/2];
            predictor=decode_nibble((v>>((i&1)*4))&15,predictor,&index);
            out[(i+1)*channels]=predictor;
        }
    }
#endif
    return blocks*64;
}
int n3ds_audio_codec_tests(void)
{
    unsigned char b[72]={0};short pcm[128];
    b[0]=0xe8;b[1]=3;b[4]=0x18;b[5]=0xfc;
    if(n3ds_xbox_adpcm_decode(b,72,2,pcm,128)!=64) return 0;
    for(int i=0;i<64;++i) if(pcm[i*2]!=1000 || pcm[i*2+1]!=-1000) return 0;
    b[71]=0xf0; /* unused high nibble must not change the last sample */
    if(n3ds_xbox_adpcm_decode(b,72,2,pcm,128)!=64 || pcm[127]!=-1000) return 0;
    b[2]=89;if(n3ds_xbox_adpcm_decode(b,72,2,pcm,128)) return 0;b[2]=0;
    if(n3ds_xbox_adpcm_decode(b,71,2,pcm,128) || n3ds_xbox_adpcm_decode(b,72,2,pcm,127)) return 0;
    return 1;
}
int n3ds_audio_codec_reference_tests(void)
{
    unsigned char b[72];short pcm[128];
    /* Compare every initial index/sign/magnitude against scalar reference
     * decoding, including predictor clipping and stereo byte interleaving. */
    for(int channels=1;channels<=2;++channels) for(int initial=0;initial<89;++initial)
    for(int seed=0;seed<16;++seed) {
        for(int i=0;i<72;++i) b[i]=(unsigned char)(i*73+seed*17);
        for(int c=0;c<channels;++c) {
            b[c*4]=seed&1?0xf0:0x10;b[c*4+1]=seed&1?0x7f:0x80;
            b[c*4+2]=initial;b[c*4+3]=0;
        }
        if(n3ds_xbox_adpcm_decode(b,36*channels,channels,pcm,128)!=64) return 0;
        for(int c=0;c<channels;++c) {
            int value=(short)(b[c*4]|b[c*4+1]<<8),index=initial;
            if(pcm[c]!=value) return 0;
            for(int sample=0;sample<63;++sample) {
                int byte=b[4*channels+(sample/8*channels+c)*4+(sample%8)/2];
                int code=(byte>>((sample&1)*4))&15;
                int magnitude=((code&7)*2+1)*steps[index]/8;
                value+=(code&8)?-magnitude:magnitude;
                if(value>32767)value=32767;if(value< -32768)value=-32768;
                index+=adjust[code&7];if(index<0)index=0;if(index>88)index=88;
                if(pcm[(sample+1)*channels+c]!=value) return 0;
            }
        }
    }
    for(int channels=1;channels<=2;++channels) {
        unsigned int block_bytes=36*channels,block_samples=64*channels;
        unsigned char blocks[144]={0};
        short output[256];
        blocks[block_bytes]=1;
        if(n3ds_xbox_adpcm_decode(blocks,2*block_bytes,channels,output,256)!=128) return 0;
        for(unsigned int i=0;i<2*block_samples;++i)
            if(output[i]!=(i>=block_samples && i%channels==0)) return 0;
        /* A malformed later block must leave the entire output untouched. */
        for(int i=0;i<256;++i) output[i]=0x4a4a;
        blocks[block_bytes+4*(channels-1)+2]=89;
        if(n3ds_xbox_adpcm_decode(blocks,2*block_bytes,channels,output,256)) return 0;
        for(int i=0;i<256;++i) if(output[i]!=0x4a4a) return 0;
    }
    return 1;
}
