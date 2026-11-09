#ifndef N3DS_ENGINE_MODEL_CLIP_BOUNDS_H
#define N3DS_ENGINE_MODEL_CLIP_BOUNDS_H

#include <math.h>

struct n3ds_model_clip_box { float minimum[3], maximum[3]; };
struct n3ds_model_clip_plane { float coefficient[4], magnitude[4]; };
struct n3ds_model_clip_view {
    struct n3ds_model_clip_plane planes[4];
    float origin[3];
};

static inline int n3ds_model_clip_plane_side(const struct n3ds_model_clip_plane *plane,
    const float center[3],const float extent[3],const float magnitude[3])
{
    const float *p=plane->coefficient,*m=plane->magnitude;
    float distance=p[0]*center[0]+p[1]*center[1]+p[2]*center[2]+p[3];
    float reach=fabsf(p[0])*extent[0]+fabsf(p[1])*extent[1]+fabsf(p[2])*extent[2];
    float guard=(1.f+m[0]*magnitude[0]+m[1]*magnitude[1]+m[2]*magnitude[2]+m[3])*(1.f/256.f);
    return distance>reach+guard?1:distance<-(reach+guard)?-1:0;
}

static inline int n3ds_model_clip_plane_outside(const struct n3ds_model_clip_plane *plane,
    const float center[3],const float extent[3],const float magnitude[3])
{
    return n3ds_model_clip_plane_side(plane,center,extent,magnitude)>0;
}

static inline void n3ds_model_clip_include(struct n3ds_model_clip_box *box,
    unsigned int *mask, unsigned int slot, const float position[3])
{
    unsigned int bit=1u<<slot;
    for(unsigned int axis=0;axis<3;++axis) {
        float p=position[axis];
        if(!(*mask&bit) || p<box->minimum[axis]) box->minimum[axis]=p;
        if(!(*mask&bit) || p>box->maximum[axis]) box->maximum[axis]=p;
    }
    *mask|=bit;
}

static inline unsigned int n3ds_model_clip_outside(const struct n3ds_model_clip_view *view,
    const struct n3ds_model_clip_box *box, const float rows[12], unsigned int mask)
{
    float center[3],extent[3],absolute[3];
    for(unsigned int i=0;i<3;++i) {
        center[i]=(box->minimum[i]+box->maximum[i])*.5f;
        extent[i]=(box->maximum[i]-box->minimum[i])*.5f;
        float a=fabsf(box->minimum[i]),b=fabsf(box->maximum[i]);
        absolute[i]=a>b?a:b;
        if(!(absolute[i]<=65536.f)) return 0;
    }
    float position[3],radius[3],magnitude[3];
    for(unsigned int i=0;i<3;++i) {
        const float *r=rows+i*4;
        float t=r[3]-view->origin[i], x=fabsf(r[0]), y=fabsf(r[1]), z=fabsf(r[2]);
        position[i]=r[0]*center[0]+r[1]*center[1]+r[2]*center[2]+t;
        radius[i]=x*extent[0]+y*extent[1]+z*extent[2];
        magnitude[i]=x*absolute[0]+y*absolute[1]+z*absolute[2]+fabsf(t);
        if(!(magnitude[i]<=65536.f && x<=65536.f && y<=65536.f && z<=65536.f)) return 0;
    }
    /* Use the sum of absolute terms before cancellation for the margin.
     * It covers quantization and rounding of the affine, weighted and clip
     * dot products with ample room for the PICA float24 significand. The
     * constant floor also covers values that underflow or flush to zero. */
    unsigned int outside=0;
    while(mask) {
        unsigned int i=__builtin_ctz(mask),bit=1u<<i;
        mask&=~bit;
        if(n3ds_model_clip_plane_outside(view->planes+i,position,radius,magnitude)) outside|=bit;
    }
    return outside;
}

#endif
