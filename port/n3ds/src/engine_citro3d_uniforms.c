/* Keep the pinned SDK's uniform storage, setters and restore bookkeeping. */
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "engine_renderer.h"
#define C3D_UpdateUniforms stock_update_uniforms
#include <c3d/uniforms.h>
extern bool C3D_FVUnifDirty[2][C3D_FVUNIF_COUNT] __attribute__((aligned(4)));
#include "uniforms.c"
#undef C3D_UpdateUniforms

enum { VERTEX = GPU_VERTEX_SHADER };
_Static_assert(sizeof(bool)==1,"Uniform dirty flags are bytes");
_Static_assert(sizeof(C3D_FVec)==4*sizeof(u32),"PICA float vectors have four words");
_Static_assert(C3D_FVUNIF_COUNT%4==0,"Dirty flags group into complete words");
static struct {
    const u32 *list;
    u32 end;
    C3D_FVec value[C3D_FVUNIF_COUNT];
    bool valid[C3D_FVUNIF_COUNT];
    unsigned int sent,elided;
} uploads;

void n3ds_gpu_uniform_upload_reset(void)
{
    uploads.list=NULL;
}

void n3ds_gpu_uniform_upload_report(void)
{
    char message[128];
    extern void n3ds_log(const char *);
    snprintf(message,sizeof(message),"NATIVE UNIFORM UPLOADS: sent_vectors=%u elided_vectors=%u",
        uploads.sent,uploads.elided);
    n3ds_log(message);
    memset(&uploads,0,sizeof(uploads));
}

static inline bool same_vector(const C3D_FVec *a,const C3D_FVec *b)
{
    const u32 *left=(const u32 *)a,*right=(const u32 *)b;
    return left[0]==right[0] && left[1]==right[1] && left[2]==right[2] && left[3]==right[3];
}

void C3D_UpdateUniforms(GPU_SHADER_TYPE type)
{
    shaderProgram_s *program=C3Di_GetContext()->program;
    if(type!=VERTEX || !gpuCmdBuf || !n3ds_gpu_frame_active() || (program && program->geometryShader)) {
        if(type==VERTEX) n3ds_gpu_uniform_upload_reset();
        stock_update_uniforms(type);
        return;
    }
    if(uploads.list!=gpuCmdBuf || gpuCmdBufOffset<uploads.end) {
        uploads.list=gpuCmdBuf;
        memset(uploads.valid,0,sizeof(uploads.valid));
    }

    if(C3Di_ShaderFVecData[VERTEX].dirty) {
        for(int i=0;i<C3Di_ShaderFVecData[VERTEX].count;++i) {
            const float24Uniform_s *constant=&C3Di_ShaderFVecData[VERTEX].data[i];
            GPUCMD_AddIncrementalWrites(GPUREG_VSH_FLOATUNIFORM_CONFIG,(const u32 *)constant,4);
            C3D_FVUnifDirty[VERTEX][constant->id]=false;
            uploads.valid[constant->id]=false;
        }
        C3Di_ShaderFVecData[VERTEX].dirty=false;
    }

    const u32 *dirty_words=(const u32 *)C3D_FVUnifDirty[VERTEX];
    for(int first=0;first<C3D_FVUNIF_COUNT;) {
        if(!(first&3) && !dirty_words[first/4]) {
            first+=4;
            continue;
        }
        if(!C3D_FVUnifDirty[VERTEX][first]) {
            ++first;
            continue;
        }
        int end=first;
        bool unchanged=true;
        do {
            unchanged=unchanged && uploads.valid[end] &&
                same_vector(uploads.value+end,C3D_FVUnif[VERTEX]+end);
            ++end;
        } while(end<C3D_FVUNIF_COUNT && C3D_FVUnifDirty[VERTEX][end]);
        int count=end-first;
        if(unchanged) {
            uploads.elided+=count;
        } else {
            const C3D_FVec *values=C3D_FVUnif[VERTEX]+first;
            GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG,0x80000000u|first);
            GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA,(const u32 *)values,count*4);
            memcpy(uploads.value+first,values,count*sizeof(*values));
            memset(uploads.valid+first,1,count);
            uploads.sent+=count;
        }
        memset(C3D_FVUnifDirty[VERTEX]+first,0,count);
        memset(C3Di_FVUnifEverDirty[VERTEX]+first,1,count);
        first=end;
    }

    /* Preserve the SDK's integer and Boolean uploads and dirty flags. */
    for(int i=0;i<C3D_IVUNIF_COUNT;++i) {
        if(!C3D_IVUnifDirty[VERTEX][i]) continue;
        GPUCMD_AddWrite(GPUREG_VSH_INTUNIFORM_I0+i,C3D_IVUnif[VERTEX][i]);
        C3D_IVUnifDirty[VERTEX][i]=false;
        C3Di_IVUnifEverDirty[VERTEX][i]=false;
    }
    if(C3D_BoolUnifsDirty[VERTEX]) {
        GPUCMD_AddWrite(GPUREG_VSH_BOOLUNIFORM,0x7fff0000u|C3D_BoolUnifs[VERTEX]);
        C3D_BoolUnifsDirty[VERTEX]=false;
    }
    uploads.end=gpuCmdBufOffset;
}
