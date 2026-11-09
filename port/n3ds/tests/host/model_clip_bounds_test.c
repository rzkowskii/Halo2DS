#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "engine_model_clip_bounds.h"

static uint32_t rng=0x184adc11;

static float sample(float magnitude)
{
    rng=rng*1664525u+1013904223u;
    return ((float)(rng>>8)*(1.f/8388608.f)-1.f)*magnitude;
}

/* Test both directions of directed significand rounding on a 1.7.16
 * model. Real console parity is still a separate test. */
static float f24(float value,int round_up)
{
    int exponent;
    float fraction=frexpf(value,&exponent)*131072.f;
    float rounded=round_up?ceilf(fraction):floorf(fraction);
    return ldexpf(rounded,exponent-17);
}

static float dot24(const float a[4],const float b[4],int round_up)
{
    float result=0.f;
    for(unsigned int i=0;i<4;++i)
        result=f24(result+f24(f24(a[i],round_up)*f24(b[i],round_up),round_up),round_up);
    return result;
}

int main(void)
{
    unsigned int outside_count=0,blended_count=0,mask=0;
    struct n3ds_model_clip_box box;
    const float a[]={1,2,-3},b[]={-1,3,5};
    n3ds_model_clip_include(&box,&mask,2,a);
    n3ds_model_clip_include(&box,&mask,2,b);
    const float expected[6]={-1,2,-3,1,3,5};
    assert(mask==4 && !memcmp(&box,expected,sizeof(expected)));
    struct n3ds_model_clip_view simple={
        .planes={{{1,0,0,-1},{1,0,0,1}},{{-1,0,0,-1},{1,0,0,1}},
                 {{0,1,0,-1},{0,1,0,1}},{{0,-1,0,-1},{0,1,0,1}}}
    };
    const float identity[12]={1,0,0,0,0,1,0,0,0,0,1,0};
    struct n3ds_model_clip_box tangent={{1,-.1f,-1},{1.5f,.1f,1}};
    assert(!n3ds_model_clip_outside(&simple,&tangent,identity,15));
    tangent.minimum[0]=1.001f;
    assert(!n3ds_model_clip_outside(&simple,&tangent,identity,15));
    tangent.minimum[0]=1.02f;
    assert(n3ds_model_clip_outside(&simple,&tangent,identity,15)==1);
    tangent.maximum[0]=INFINITY;
    assert(!n3ds_model_clip_outside(&simple,&tangent,identity,15));
    for(unsigned int test=0;test<12000;++test) {
        struct n3ds_model_clip_view view;
        float projection[3][4],rows[2][12];
        float scale=test&1?64.f:1.f;
        for(unsigned int i=0;i<3;++i) {
            float c=sample(4.f)*scale,e=fabsf(sample(scale));
            box.minimum[i]=c-e;box.maximum[i]=c+e;
            view.origin[i]=sample(16.f)*scale;
            for(unsigned int j=0;j<4;++j) projection[i][j]=sample(3.f);
        }
        for(unsigned int i=0;i<4;++i) for(unsigned int j=0;j<4;++j) {
            float s=projection[i/2][j],w=projection[2][j];
            view.planes[i].coefficient[j]=(i&1?-s:s)-w;
            view.planes[i].magnitude[j]=fabsf(s)+fabsf(w);
        }
        for(unsigned int bone=0;bone<2;++bone) for(unsigned int i=0;i<12;++i)
            rows[bone][i]=sample(i%4==3?16.f*scale:3.f);
        unsigned int first=n3ds_model_clip_outside(&view,&box,rows[0],15);
        unsigned int common=n3ds_model_clip_outside(&view,&box,rows[1],first);
        outside_count+=!!first;blended_count+=!!common;
        for(unsigned int corner=0;corner<8 && first;++corner) for(int up=0;up<2;++up) {
            float local[4],pose[2][4]={{0,0,0,1},{0,0,0,1}},blend[4]={0,0,0,1};
            for(unsigned int i=0;i<3;++i) local[i]=corner&(1u<<i)?box.maximum[i]:box.minimum[i];
            local[3]=1;
            for(unsigned int bone=0;bone<2;++bone) for(unsigned int i=0;i<3;++i) {
                float affine[4];memcpy(affine,rows[bone]+4*i,sizeof(affine));
                affine[3]-=view.origin[i];
                pose[bone][i]=dot24(affine,local,up);
            }
            for(unsigned int i=0;i<3;++i)
                blend[i]=f24(f24(pose[0][i]*f24(.35f,up),up)+f24(pose[1][i]*f24(.65f,up),up),up);
            for(unsigned int i=0;i<4;++i) {
                if(first&(1u<<i)) {
                    float side=dot24(projection[i/2],pose[0],up),w=dot24(projection[2],pose[0],up);
                    assert((i&1?-side:side)>w);
                }
                if(common&(1u<<i)) {
                    float side=dot24(projection[i/2],blend,up),w=dot24(projection[2],blend,up);
                    assert((i&1?-side:side)>w);
                }
            }
        }
    }
    assert(outside_count>6000 && blended_count>1000);
    printf("PASS: 12000 boxes, %u rigid and %u weighted rejections\n",outside_count,blended_count);
}
