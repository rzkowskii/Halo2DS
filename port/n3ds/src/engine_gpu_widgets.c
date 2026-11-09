#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine_widgets.h"
#include "engine_renderer.h"
#include "engine_textures.h"
#include "engine_widget_shader.h"
#include "engine_effect_packed_shader.h"
enum { WIDGET_VERTICES=6144, WIDGET_BLOCKS=4 };
static DVLB_s *shader;
static shaderProgram_s program;
static int ready,matrix_location;
static C3D_AttrInfo widget_attributes[2];
static DVLB_s *packed_shader;
static shaderProgram_s packed_program;
static int packed_ready,packed_matrix;
static int effect_state_valid,effect_state_packed,effect_state_blend;
static unsigned int effect_state_shader,effect_state_geometry,effect_state_reused;
static C3D_Mtx effect_projection;
void n3ds_gpu_effect_state_invalidate(void) {effect_state_valid=0;}

static struct native_widget_vertex *stream;
static unsigned int used,block;
static struct native_widget_vertex *blocks[WIDGET_BLOCKS];
static unsigned int budget_drops;
static unsigned int deferred,immediate,flushes;
static int dirty;
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
static unsigned short *resident_quad_indices;
#ifdef HALO_N3DS_PERFORMANCE_PROFILE
static unsigned long long resident_quad_batches,resident_quad_bytes;
#endif

const unsigned short *n3ds_gpu_effect_resident_quad_indices(void)
{
    return resident_quad_indices;
}

static void initialize_quad_indices(void)
{
    static const unsigned short corners[]={0,1,2,0,2,3};
    const unsigned int bytes=N3DS_EFFECT_QUAD_INDICES*sizeof(*resident_quad_indices);
    unsigned short *indices=linearAlloc(bytes);
    if(!indices) return;
    for(unsigned int i=0;i<N3DS_EFFECT_QUAD_INDICES;i+=6) {
        unsigned int base=i/6*4;
        for(unsigned int j=0;j<6;++j) indices[i+j]=base+corners[j];
    }
    /* The renderer waits for queued GPU references before disposing widgets. */
    if(R_FAILED(GSPGPU_FlushDataCache(indices,bytes))) {linearFree(indices);return;}
    resident_quad_indices=indices;
}
#endif
#if HALO_N3DS_RENDERER_TESTS
/* Test-only hardware input precision and legacy control. Never enabled in play. */
static int glow_test_quantize,glow_test_legacy;
static void glow_test_input(float position[3])
{
    if(!glow_test_quantize) return;
    for(int c=0;c<3;++c) if(position[c]!=0) {
        unsigned int p=f32tof24(position[c]);
        unsigned int bits=((p&0x800000u)<<8)|((((p>>16)&127u)+64u)<<23)|((p&65535u)<<7);
        memcpy(position+c,&bits,4);
    }
}
#endif
/* Keep world-space callers/caches intact; only the GPU upload is rebased. */
static int effect_relative_projection(C3D_Mtx *projection,float origin[3])
{
#if HALO_N3DS_RENDERER_TESTS
    if(glow_test_legacy) {memset(origin,0,3*sizeof(float));return n3ds_gpu_world_projection((float *)projection);}
#endif
    return n3ds_gpu_model_projection((float *)projection,origin);
}
void n3ds_log(const char *message);
static void flush_written(unsigned int count)
{
    /* Direct Citro3D tests do not call native present, so keep those immediate. */
    if(n3ds_gpu_frame_active()) {dirty=1;++deferred;}
    else {GSPGPU_FlushDataCache(stream+used,count*sizeof(*stream));++immediate;}
}
void n3ds_gpu_widgets_flush(void)
{ if(dirty && used) {GSPGPU_FlushDataCache(stream,used*sizeof(*stream));++flushes;}dirty=0; }
void n3ds_gpu_widgets_dispose(void)
{
    char message[128];snprintf(message,sizeof(message),"WIDGET TRANSFERS: deferred=%u immediate=%u frame_flushes=%u effect_state_reuses=%u",deferred,immediate,flushes,effect_state_reused);
    n3ds_log(message);deferred=immediate=flushes=0;dirty=0;
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
#ifdef HALO_N3DS_PERFORMANCE_PROFILE
    if(resident_quad_indices) {
        snprintf(message,sizeof(message),"EFFECT RESIDENT QUAD INDICES: batches=%llu index_upload_bytes_avoided=%llu",resident_quad_batches,resident_quad_bytes);
        n3ds_log(message);
    }
    resident_quad_batches=resident_quad_bytes=0;
#endif
    linearFree(resident_quad_indices);resident_quad_indices=NULL;
#endif
    n3ds_gpu_program_park();
    if(packed_ready) shaderProgramFree(&packed_program);
    if(packed_shader) DVLB_Free(packed_shader);packed_shader=NULL;packed_ready=0;
    if(ready) shaderProgramFree(&program);
    if(shader) DVLB_Free(shader);
    for(unsigned int i=0;i<WIDGET_BLOCKS;++i) {linearFree(blocks[i]);blocks[i]=NULL;}
    stream=NULL;shader=NULL;ready=0;used=block=0;
}
int n3ds_gpu_widgets_initialize(void)
{
    if(shader || stream) return 0;
    shader=DVLB_ParseFile((u32 *)engine_widget_shader,sizeof(engine_widget_shader));
    stream=blocks[0]=linearAlloc(WIDGET_VERTICES*sizeof(*stream));
    if(!shader || !stream) goto fail;
    shaderProgramInit(&program);ready=1;
    if(R_FAILED(shaderProgramSetVsh(&program,&shader->DVLE[0]))) goto fail;
    matrix_location=shaderInstanceGetUniformLocation(program.vertexShader,"projection");
    if(matrix_location<0) goto fail;
    packed_shader=DVLB_ParseFile((u32 *)engine_effect_packed_shader,sizeof(engine_effect_packed_shader));
    if(!packed_shader) goto fail;
    shaderProgramInit(&packed_program);packed_ready=1;
    if(R_FAILED(shaderProgramSetVsh(&packed_program,&packed_shader->DVLE[0]))) goto fail;
    packed_matrix=shaderInstanceGetUniformLocation(packed_program.vertexShader,"projection");
    if(packed_matrix<0) goto fail;
    for(int packed=0;packed<2;++packed) {
        C3D_AttrInfo *attributes=&widget_attributes[packed];
        AttrInfo_Init(attributes);
        AttrInfo_AddLoader(attributes,0,GPU_FLOAT,3);
        AttrInfo_AddLoader(attributes,1,GPU_FLOAT,packed?2:4);
        AttrInfo_AddLoader(attributes,2,packed?GPU_UNSIGNED_BYTE:GPU_FLOAT,packed?4:2);
    }
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
    initialize_quad_indices();
#ifdef HALO_N3DS_PERFORMANCE_PROFILE
    n3ds_log(resident_quad_indices?"EFFECT QUAD INDICES: candidate ready":"EFFECT QUAD INDICES: candidate unavailable; transient fallback");
#endif
#elif defined(HALO_N3DS_PERFORMANCE_PROFILE)
    n3ds_log("EFFECT QUAD INDICES: reference");
#endif
    return 1;
fail:n3ds_gpu_widgets_dispose();return 0;
}
void n3ds_gpu_widgets_frame_begin(void) {effect_state_valid=0;used=0;dirty=0;block=0;stream=blocks[0];}
void n3ds_gpu_widgets_map_release(void)
{
    for(unsigned int i=1;i<WIDGET_BLOCKS;++i) {linearFree(blocks[i]);blocks[i]=NULL;}
    used=dirty=block=0;stream=blocks[0];
}
/* A draw retains its address until the next waited frame. Spill into bounded
 * blocks; never wrap or realloc a vertex buffer already referenced by the GPU. */
static int reserve_vertices(unsigned int count)
{
    if(count>WIDGET_VERTICES) return 0;
    if(count<=WIDGET_VERTICES-used) return 1;
    if(block+1>=WIDGET_BLOCKS) return 0;
    if(!blocks[block+1]) blocks[block+1]=linearAlloc(WIDGET_VERTICES*sizeof(*stream));
    if(!blocks[block+1]) return 0;
    n3ds_gpu_widgets_flush();stream=blocks[++block];used=0;
    return 1;
}
int n3ds_gpu_widget_draw(const struct native_widget_vertex vertices[4],int world,
    float width,float height,float tint,unsigned int flags)
{
    effect_state_valid=0;
    static const unsigned int order[6]={0,1,2,0,2,3};
    C3D_Tex *texture=n3ds_gpu_texture_bound(0);C3D_Mtx projection;
    if(!ready || !texture || !vertices || !reserve_vertices(6) || n3ds_gpu_command_usage()>.95f || (flags&~3U) ||
       !isfinite(tint) || !isfinite(width) || !isfinite(height) || width<=0 || height<=0) {
        extern void n3ds_log(const char *);char message[192];
        snprintf(message,sizeof(message),"WIDGET RESOURCE REJECT: ready=%d texture=%p vertices=%p used=%u commands=%.6f flags=%u world=%d width=%.3f height=%.3f tint=%.6f",ready,(void *)texture,(void *)vertices,used,n3ds_gpu_command_usage(),flags,world,width,height,tint);
        n3ds_log(message);return 0;
    }
    for(unsigned int i=0;i<4;++i) {
        for(unsigned int j=0;j<3;++j) if(!isfinite(vertices[i].position[j])) return 0;
        for(unsigned int j=0;j<4;++j) if(!isfinite(vertices[i].color[j])) return 0;
        for(unsigned int j=0;j<2;++j) if(!isfinite(vertices[i].uv[j])) return 0;
    }
    float origin[3]={0};
    if(world) {if(!effect_relative_projection(&projection,origin)) return 0;}
    else Mtx_OrthoTilt(&projection,0,width,height,0,0,1,true);
    C3D_BindProgram(&program);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,matrix_location,&projection);
    C3D_SetAttrInfo(&widget_attributes[0]);
    for(unsigned int i=0;i<6;++i) {
        C3D_TexEnv *env=C3D_GetTexEnv(i);C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env,C3D_Alpha,GPU_REPLACE);
    }
    /* Original sprite RGB: C*T + (1-C)*tint*T^6. Texture alpha is unused;
     * source vertex alpha weights additive blending, framebuffer alpha stays. */
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,GPU_TEXTURE0,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvBufColor(0);C3D_TexEnvBufUpdate(C3D_Both,0);C3D_TexEnvBufUpdate(C3D_RGB,1);
    env=C3D_GetTexEnv(1);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    env=C3D_GetTexEnv(2);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    env=C3D_GetTexEnv(3);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR);
    C3D_TexEnvOpRgb(env,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    unsigned int t=(unsigned int)(fminf(1,fmaxf(0,tint))*255.f+.5f);
    env=C3D_GetTexEnv(4);C3D_TexEnvColor(env,t|t<<8|t<<16|0xff000000U);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_CONSTANT,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    env=C3D_GetTexEnv(5);
    C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,GPU_PRIMARY_COLOR,GPU_PREVIOUS);C3D_TexEnvFunc(env,C3D_RGB,GPU_MULTIPLY_ADD);
    C3D_DepthTest((flags&1)!=0,GPU_GEQUAL,(GPU_WRITE_COLOR&~GPU_WRITE_ALPHA)|((flags&2)?GPU_WRITE_DEPTH:0));
    C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE,GPU_SRC_ALPHA,GPU_ONE);
    u32 sampler=texture->param,border=texture->border;
    C3D_TexSetFilter(texture,GPU_LINEAR,GPU_LINEAR);C3D_TexSetWrap(texture,GPU_CLAMP_TO_BORDER,GPU_CLAMP_TO_BORDER);texture->border=0;
    n3ds_gpu_texture_bind(0,texture);
    for(unsigned int i=0;i<6;++i) {
        stream[used+i]=vertices[order[i]];
        if(world) for(int c=0;c<3;++c) stream[used+i].position[c]-=origin[c];
#if HALO_N3DS_RENDERER_TESTS
        glow_test_input(stream[used+i].position);
#endif
    }
    flush_written(6);
    int success=n3ds_gpu_vertex_buffer_bind(stream,sizeof(*stream),3,0x210);
    if(success) {
        C3D_DrawArrays(GPU_TRIANGLES,used,6);
        if(!world && n3ds_gpu_overlay_other_begin()) {C3D_DrawArrays(GPU_TRIANGLES,used,6);n3ds_gpu_overlay_other_end();}
        used+=6;
    }
    texture->param=sampler;texture->border=border;n3ds_gpu_texture_bind(0,texture);
    return success;
}

static int effect_draw(const void *input,unsigned int count,
    int blend,unsigned int shader_flags,unsigned int sampler_flags,unsigned int geometry_flags,
    int packed,const float *rows,const unsigned short *indices,unsigned int index_count)
{
    const struct native_widget_vertex *vertices=input;
    const struct native_packed_effect_vertex *raw=input;
    C3D_Tex *texture=n3ds_gpu_texture_bound(0);C3D_Mtx projection;
    float origin[3];
    if(!ready || !texture || !vertices || !count || (indices?(!index_count || index_count%3):count%3) ||
       blend<0 || blend>=NATIVE_BLEND_COUNT || (shader_flags&~7U) || (sampler_flags&~7U) ||
       !effect_relative_projection(&projection,origin)) {
        char message[160];snprintf(message,sizeof(message),"EFFECT REJECT: ready=%d texture=%p count=%u blend=%d shader=%x sampler=%x",ready,(void *)texture,count,blend,shader_flags,sampler_flags);n3ds_log(message);return 0;
    }
    /* Optional particles may exceed a fixed console budget. Keep queued draws
     * intact and leave command space for the HUD; never terminate gameplay. */
    if(n3ds_gpu_command_usage()>.85f || !reserve_vertices(count)) {
        if(!(budget_drops++%300)) n3ds_log("EFFECT BUDGET: particle batch omitted; gameplay and queued geometry retained");
        return 1;
    }
    /* Preserve complete original calls for MIN/MAX: overlapping weapon-mask
     * comparisons were not repeatable even between identical unbatched draws
     * in Azahar. Other modes have exact batch-vs-separate GPU comparisons. */
    if(!indices && (blend==NATIVE_BLEND_MIN || blend==NATIVE_BLEND_MAX) && count>6) {
        for(unsigned int offset=0;offset<count;offset+=6) {
            unsigned int chunk=count-offset>6?6:count-offset;
            if(!effect_draw(packed?(const void *)(raw+offset):(const void *)(vertices+offset),chunk,blend,shader_flags,sampler_flags,geometry_flags,packed,rows,NULL,0)) return 0;
        }
        return 1;
    }
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
    int resident=indices && indices==resident_quad_indices;
    if(resident) {
        if(!packed || count%4 || count>N3DS_EFFECT_BATCH_VERTICES || index_count!=count/4*6) return 0;
    } else
#endif
    for(unsigned int i=0;i<index_count;++i) if(!indices || indices[i]>=count) return 0;
    for(unsigned int i=0;i<count;++i) {
        const float *p=packed?raw[i].position:vertices[i].position;
        const float *uv=packed?raw[i].uv:vertices[i].uv;
        for(unsigned int j=0;j<3;++j) if(!isfinite(p[j])) return 0;
        for(unsigned int j=0;j<2;++j) if(!isfinite(uv[j])) return 0;
        if(!packed) for(unsigned int j=0;j<4;++j) if(!isfinite(vertices[i].color[j])) return 0;
    }
    if(rows) {
        C3D_Mtx affine,combined;Mtx_Identity(&affine);
        for(unsigned int i=0;i<12;++i) if(!isfinite(rows[i])) return 0;
        /* Local particle vertices stay local. Cancel the world translation on
         * the CPU before the limited-precision vertex shader sees it. */
        for(unsigned int r=0;r<3;++r) affine.r[r]=FVec4_New(rows[r*4],rows[r*4+1],rows[r*4+2],rows[r*4+3]-origin[r]);
        Mtx_Multiply(&combined,&projection,&affine);projection=combined;
    }
    int retained=effect_state_valid && effect_state_packed==packed;
    if(!retained) C3D_BindProgram(packed?&packed_program:&program);
    if(!retained || memcmp(&effect_projection,&projection,sizeof(projection))) C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,packed?packed_matrix:matrix_location,&projection);
    effect_projection=projection;
    if(!retained) C3D_SetAttrInfo(&widget_attributes[!!packed]);
    int same=retained && effect_state_blend==blend && effect_state_shader==shader_flags && effect_state_geometry==geometry_flags;
#ifdef HALO_N3DS_PROBE_EFFECT_ALPHA_ZERO
    /* The phase is fixed for the frame, and frame begin invalidates retained
     * state. With SRC_ALPHA / ONE_MINUS_SRC_ALPHA, zero final alpha leaves
     * RGB intact. This path masks alpha/depth writes and keeps stencil. */
    int reject_zero_alpha=blend==NATIVE_BLEND_ALPHA && n3ds_gpu_probe_effect_alpha_zero();
#endif
    if(!same) {
    C3D_TexEnvBufUpdate(C3D_Both,0);C3D_TexEnvBufColor(0);
    for(unsigned int i=0;i<6;++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(env,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_TEXTURE0,GPU_CONSTANT);
    C3D_TexEnvFunc(env,C3D_Alpha,GPU_REPLACE);
    /* Original effect tint: linear C*T, or nonlinear C*T+(1-C)*T^4.
     * The original sprite builder has already applied orientation fading. */
    C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,(shader_flags&2)?GPU_TEXTURE0:GPU_PRIMARY_COLOR,GPU_CONSTANT);
    C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
    if(shader_flags&2) {
        env=C3D_GetTexEnv(1);C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS,GPU_CONSTANT);
        C3D_TexEnvFunc(env,C3D_RGB,GPU_MODULATE);
        env=C3D_GetTexEnv(2);C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env,C3D_RGB,GPU_INTERPOLATE);
    }
    /* Match the final combiner's texture-alpha fade, at unit atmospheric
     * transmittance. Spatial fog is still a renderer-wide fidelity limitation. */
    if(!(geometry_flags&(1u<<2))) {
        env=C3D_GetTexEnv(3);
        int channel=blend==NATIVE_BLEND_ALPHA?C3D_Alpha:blend==NATIVE_BLEND_PREMULTIPLIED_ALPHA?C3D_Both:C3D_RGB;
        C3D_TexEnvSrc(env,channel,GPU_PREVIOUS,GPU_TEXTURE0,GPU_CONSTANT);
        C3D_TexEnvOpRgb(env,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(env,channel,GPU_MODULATE);
        if(blend==NATIVE_BLEND_MULTIPLY || blend==NATIVE_BLEND_MIN || blend==NATIVE_BLEND_DOUBLE_MULTIPLY) {
            env=C3D_GetTexEnv(4);
            C3D_TexEnvColor(env,blend==NATIVE_BLEND_DOUBLE_MULTIPLY?0x80808080U:0xffffffffU);
            C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
            C3D_TexEnvOpRgb(env,GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
            C3D_TexEnvFunc(env,C3D_RGB,GPU_MULTIPLY_ADD);
        }
    }
    C3D_CullFace(GPU_CULL_NONE);
#ifdef HALO_N3DS_PROBE_EFFECT_ALPHA_ZERO
    C3D_AlphaTest(reject_zero_alpha,reject_zero_alpha?GPU_GREATER:GPU_ALWAYS,0);
#else
    C3D_AlphaTest(false,GPU_ALWAYS,0);
#endif
    n3ds_gpu_framebuffer_blend(blend);
    } else ++effect_state_reused;
    effect_state_valid=1;effect_state_packed=packed;effect_state_blend=blend;
    effect_state_shader=shader_flags;effect_state_geometry=geometry_flags;
    u32 sampler=texture->param,border=texture->border;
    C3D_TexSetFilter(texture,(sampler_flags&1)?GPU_NEAREST:GPU_LINEAR,(sampler_flags&1)?GPU_NEAREST:GPU_LINEAR);
    C3D_TexSetWrap(texture,(sampler_flags&2)?GPU_CLAMP_TO_EDGE:GPU_REPEAT,(sampler_flags&4)?GPU_CLAMP_TO_EDGE:GPU_REPEAT);
    n3ds_gpu_texture_bind(0,texture);
    unsigned int stride=packed?sizeof(*raw):sizeof(*vertices);
    void *submitted=stream+used;
    memcpy(submitted,input,count*stride);
    for(unsigned int i=0;i<count;++i) {
        float *position=(float *)((unsigned char *)submitted+i*stride);
        if(!rows) for(int c=0;c<3;++c) position[c]-=origin[c];
#if HALO_N3DS_RENDERER_TESTS
        glow_test_input(position);
#endif
    }
    flush_written(count);
    int success=0;
    const unsigned short *gpu_indices=
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
        resident?indices:
#endif
        indices?n3ds_gpu_indices_upload(indices,index_count):NULL;
    if(indices && !gpu_indices) goto restore_sampler;
    success=indices || n3ds_gpu_vertex_buffer_bind(submitted,stride,3,0x210);
    if(success) {
        /* A later first-person pass can fail after an earlier pass queued
         * this span. Never let a subsequent draw overwrite those vertices. */
        used+=count;
        int first_person=(geometry_flags&(1u<<7))!=0;
        for(int pass=0;pass<(first_person?2:1);++pass) {
            if(first_person && !pass && (shader_flags&4)) continue;
            C3D_DepthTest(!(geometry_flags&(1u<<3)) && !pass,GPU_GEQUAL,GPU_WRITE_COLOR&~GPU_WRITE_ALPHA);
            C3D_StencilTest(true,GPU_EQUAL,first_person&&!pass?1:0,1,0);
            C3D_StencilOp(GPU_STENCIL_KEEP,GPU_STENCIL_KEEP,GPU_STENCIL_KEEP);
            if(indices) {
                int drawn=
#ifdef HALO_N3DS_RESIDENT_QUAD_INDICES
                    resident?n3ds_gpu_indexed_draw_resident(submitted,gpu_indices,index_count,stride,3,0x210):
#endif
                    n3ds_gpu_indexed_draw(submitted,gpu_indices,index_count,stride,3,0x210);
                if(!drawn) {success=0;goto restore_sampler;}
            } else C3D_DrawArrays(GPU_TRIANGLES,0,count);
#ifdef HALO_N3DS_PROBE_EFFECT_ALPHA_ZERO
            if(blend==NATIVE_BLEND_ALPHA) n3ds_gpu_probe_alpha_effect_draw(reject_zero_alpha);
#endif
        }
    }
#if defined(HALO_N3DS_RESIDENT_QUAD_INDICES) && defined(HALO_N3DS_PERFORMANCE_PROFILE)
    if(success && resident) {
        ++resident_quad_batches;
        resident_quad_bytes+=index_count*sizeof(*indices);
    }
#endif
restore_sampler:
    texture->param=sampler;texture->border=border;n3ds_gpu_texture_bind(0,texture);
    return success;
}

int n3ds_gpu_effect_draw(const struct native_widget_vertex *v,unsigned int n,int blend,
    unsigned int shader,unsigned int sampler,unsigned int geometry)
{return effect_draw(v,n,blend,shader,sampler,geometry,0,NULL,NULL,0);}
int n3ds_gpu_effect_packed_draw(const struct native_packed_effect_vertex *v,unsigned int n,int blend,
    unsigned int shader,unsigned int sampler,unsigned int geometry,const float *rows)
{return effect_draw(v,n,blend,shader,sampler,geometry,1,rows,NULL,0);}
#include "engine_effect_offload_tests.inl"

int n3ds_gpu_effect_packed_indexed_draw(const struct native_packed_effect_vertex *v,unsigned int n,
    const unsigned short *indices,unsigned int count,int blend,unsigned int shader,unsigned int sampler,unsigned int geometry,const float *rows)
{return effect_draw(v,n,blend,shader,sampler,geometry,1,rows,indices,count);}
