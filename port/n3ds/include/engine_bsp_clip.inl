/* Called only for regular mono submissions, after cluster filtering. */
enum { BSP_CLIP_BUDGET=256*1024 };
static unsigned int bsp_clip_metadata_bytes,bsp_clip_index_bytes;
static unsigned int bsp_clip_builds,bsp_clip_hits,bsp_clip_saved,bsp_clip_empty;
static unsigned int bsp_clip_plane_tests,bsp_clip_child_tests;
static float bsp_clip_projection[16];
static struct n3ds_model_clip_plane bsp_clip_planes[4];
static unsigned long long bsp_clip_view;
static int bsp_clip_active;

static void bsp_clip_begin(void)
{
    float projection[16];
    bsp_clip_active=n3ds_gpu_frame_is_mono() && !n3ds_presentation_midpoint_active() &&
        !n3ds_input_platform_original_model() && n3ds_gpu_world_projection(projection);
    if(!bsp_clip_active || (bsp_clip_view && !memcmp(projection,bsp_clip_projection,sizeof(projection)))) return;
    memcpy(bsp_clip_projection,projection,sizeof(projection));
    ++bsp_clip_view;
    for(unsigned int i=0;i<4;++i) for(unsigned int j=0;j<4;++j) {
        float s=bsp_clip_projection[(i/2)*4+3-j],w=bsp_clip_projection[12+3-j];
        bsp_clip_planes[i].coefficient[j]=(i&1?-s:s)-w;
        bsp_clip_planes[i].magnitude[j]=fabsf(s)+fabsf(w);
    }
}

static void bsp_clip_release(struct bsp_clip_cache *cache)
{
    free(cache->blocks);
    n3ds_gpu_geometry_free(cache->indices);
}

void n3ds_bsp_clip_report(void)
{
    extern void n3ds_log(const char *);
    char message[255];
    snprintf(message,sizeof(message),"BSP CLIP BLOCKS: builds=%u hits=%u triangles=%u empty=%u metadata_bytes=%u index_bytes=%u plane_tests=%u child_tests=%u",
        bsp_clip_builds,bsp_clip_hits,bsp_clip_saved,bsp_clip_empty,bsp_clip_metadata_bytes,bsp_clip_index_bytes,
        bsp_clip_plane_tests,bsp_clip_child_tests);
    n3ds_log(message);
    bsp_clip_builds=bsp_clip_hits=bsp_clip_saved=bsp_clip_empty=0;
    bsp_clip_plane_tests=bsp_clip_child_tests=0;
}

static void bsp_clip_set_block(struct bsp_clip_block *block,const struct n3ds_model_clip_box *box)
{
    block->valid=1;
    for(unsigned int j=0;j<3;++j) {
        float lo=box->minimum[j],hi=box->maximum[j],a=fabsf(lo),b=fabsf(hi);
        block->center[j]=(lo+hi)*.5f;
        block->extent[j]=(hi-lo)*.5f;
        block->magnitude[j]=a>b?a:b;
        block->valid &= block->magnitude[j]<=65536.f;
    }
}

static void bsp_clip_bounds(struct bsp_clip_cache *cache,const struct native_render_vertex *vertices,
    const unsigned short *source,unsigned int count,unsigned int revision)
{
    for(unsigned int start=0;start<count;start+=BSP_CLIP_ELEMENTS) {
        unsigned int end=start+MIN(count-start,BSP_CLIP_ELEMENTS),occupied=0;
        struct n3ds_model_clip_box box;
        struct bsp_clip_group *group=cache->blocks+start/BSP_CLIP_ELEMENTS;
#if HALO_N3DS_ALGO_BSP_CLIP_HIERARCHY == N3DS_ALGORITHM_CANDIDATE
        for(unsigned int child=start;child<end;child+=BSP_CLIP_LEAF_ELEMENTS) {
            unsigned int present=0,stop=child+MIN(end-child,BSP_CLIP_LEAF_ELEMENTS);
            struct n3ds_model_clip_box leaf;
            for(unsigned int i=child;i<stop;++i)
                n3ds_model_clip_include(&leaf,&present,0,vertices[source[i]].position);
            bsp_clip_set_block(group->children+(child-start)/BSP_CLIP_LEAF_ELEMENTS,&leaf);
            n3ds_model_clip_include(&box,&occupied,0,leaf.minimum);
            n3ds_model_clip_include(&box,&occupied,0,leaf.maximum);
        }
#else
        for(unsigned int i=start;i<end;++i)
            n3ds_model_clip_include(&box,&occupied,0,vertices[source[i]].position);
#endif
        bsp_clip_set_block(&group->bounds,&box);
    }
    cache->source=source;cache->count=count;cache->revision=revision;
    cache->ready=0;
    ++bsp_clip_builds;
}

enum { BSP_CLIP_ALL_PLANES=15, BSP_CLIP_OUTSIDE=16 };
static inline unsigned int bsp_clip_intersection(const struct bsp_clip_block *block,unsigned int planes,int parent)
{
    if(!block->valid) return planes;
    unsigned int intersects=planes;
    while(planes) {
        unsigned int i=__builtin_ctz(planes),bit=1u<<i;
        planes&=~bit;
#ifdef HALO_N3DS_A30_BENCHMARK
        ++bsp_clip_plane_tests;
#endif
        const struct n3ds_model_clip_plane *plane=bsp_clip_planes+i;
        if(parent) {
            int side=n3ds_model_clip_plane_side(plane,block->center,block->extent,block->magnitude);
            if(side>0) return BSP_CLIP_OUTSIDE;
            if(side<0) intersects&=~bit;
        } else if(n3ds_model_clip_plane_outside(plane,block->center,block->extent,block->magnitude))
            return BSP_CLIP_OUTSIDE;
    }
    return intersects;
}

static unsigned int bsp_clip_select(const struct bsp_clip_cache *cache,unsigned int count,
    unsigned int rejected[BSP_CLIP_WORDS])
{
    unsigned int kept=0;
    for(unsigned int start=0;start<count;start+=BSP_CLIP_ELEMENTS) {
        unsigned int size=MIN(count-start,BSP_CLIP_ELEMENTS);
        const struct bsp_clip_group *group=cache->blocks+start/BSP_CLIP_ELEMENTS;
        unsigned int planes=bsp_clip_intersection(&group->bounds,BSP_CLIP_ALL_PLANES,BSP_CLIP_CHILDREN>1);
        unsigned int first=start/BSP_CLIP_LEAF_ELEMENTS;
        if(planes==BSP_CLIP_OUTSIDE) {
            /* Four-leaf groups align to a four-bit boundary. */
            unsigned int leaves=(size+BSP_CLIP_LEAF_ELEMENTS-1)/BSP_CLIP_LEAF_ELEMENTS;
            rejected[first/32]|=((1u<<leaves)-1)<<(first%32);
            continue;
        }
#if HALO_N3DS_ALGO_BSP_CLIP_HIERARCHY == N3DS_ALGORITHM_CANDIDATE
        if(planes) {
            for(unsigned int i=0;i<size;i+=BSP_CLIP_LEAF_ELEMENTS) {
                unsigned int child=i/BSP_CLIP_LEAF_ELEMENTS,index=first+child;
#ifdef HALO_N3DS_A30_BENCHMARK
                ++bsp_clip_child_tests;
#endif
                if(bsp_clip_intersection(group->children+child,planes,0)==BSP_CLIP_OUTSIDE)
                    rejected[index/32]|=1u<<(index%32);
                else kept+=MIN(size-i,BSP_CLIP_LEAF_ELEMENTS);
            }
            continue;
        }
#endif
        kept+=size;
    }
    return kept;
}

static const unsigned short *bsp_clip_indices(struct bsp_clip_cache *cache,
    const struct native_render_vertex *vertices,const unsigned short *source,
    unsigned int *count,unsigned int capacity,unsigned int revision)
{
    if(!bsp_clip_active || *count<BSP_CLIP_ELEMENTS || capacity>BSP_CLIP_MAX_ELEMENTS || cache->disabled)
        return source;
    unsigned long long frame=n3ds_gpu_model_frame_serial();
    int same_source=cache->source==source && cache->count==*count && cache->revision==revision;
    if(!same_source || !cache->ready || cache->view!=bsp_clip_view) {
        /* Same-frame draws can still reference the last compacted list. */
        if(cache->used_frame==frame) return source;
        if(!cache->blocks) {
            unsigned int blocks=(capacity+BSP_CLIP_ELEMENTS-1)/BSP_CLIP_ELEMENTS;
            unsigned int bytes=blocks*sizeof(*cache->blocks);
            if(bytes<=BSP_CLIP_BUDGET-bsp_clip_metadata_bytes) cache->blocks=(malloc)(bytes);
            if(!cache->blocks) {cache->disabled=1;return source;}
            bsp_clip_metadata_bytes+=bytes;
        }
        if(!same_source) bsp_clip_bounds(cache,vertices,source,*count,revision);
        unsigned int rejected[BSP_CLIP_WORDS]={0};
        unsigned int kept=bsp_clip_select(cache,*count,rejected);
        /* A small rejected tail alone rarely pays for index copying. */
        if(kept && *count-kept<BSP_CLIP_ELEMENTS) kept=*count;
        if(kept && kept<*count) {
            if(!cache->indices) {
                unsigned int bytes=capacity*sizeof(*cache->indices);
                if(bytes<=BSP_CLIP_BUDGET-bsp_clip_index_bytes)
                    cache->indices=n3ds_gpu_geometry_optional_allocate(bytes);
                if(!cache->indices) {cache->disabled=1;return source;}
                bsp_clip_index_bytes+=bytes;
            }
            if(!cache->ready || cache->kept!=kept || memcmp(cache->rejected,rejected,sizeof(rejected))) {
                unsigned int output=0,span=0;
                for(unsigned int start=0;start<*count;start+=BSP_CLIP_LEAF_ELEMENTS) {
                    unsigned int block=start/BSP_CLIP_LEAF_ELEMENTS;
                    if(!(rejected[block/32]&(1u<<(block%32)))) continue;
                    if(start>span) {
                        memcpy(cache->indices+output,source+span,(start-span)*sizeof(*source));
                        output+=start-span;
                    }
                    span=start+MIN(*count-start,BSP_CLIP_LEAF_ELEMENTS);
                }
                if(*count>span) memcpy(cache->indices+output,source+span,(*count-span)*sizeof(*source));
                assert(output+*count-span==kept);
                n3ds_gpu_geometry_flush(cache->indices,kept*sizeof(*source));
            } else ++bsp_clip_hits;
        }
        cache->view=bsp_clip_view;
        memcpy(cache->rejected,rejected,sizeof(rejected));
        cache->kept=kept;cache->ready=1;
    } else ++bsp_clip_hits;
    bsp_clip_saved+=(*count-cache->kept)/3;
    if(cache->kept==*count) return source;
    *count=cache->kept;
    if(!*count) {++bsp_clip_empty;return source;}
    cache->used_frame=frame;
    return cache->indices;
}

static const unsigned short *bsp_clip_material_indices(struct native_geometry *owner,
    const unsigned short *indices,unsigned int *count)
{
    unsigned int revision=indices==owner->indices?0:owner->filter_revision[n3ds_gpu_stereo_view()];
    return bsp_clip_indices(&owner->clip,owner->vertices,indices,count,owner->index_count,revision);
}
