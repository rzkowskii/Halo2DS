#ifndef HALO_N3DS_ENGINE_TEXT_H
#define HALO_N3DS_ENGINE_TEXT_H
struct native_text_vertex { float position[2],color[4],uv[2]; };
int n3ds_gpu_text_initialize(void);
void n3ds_gpu_text_dispose(void);
void n3ds_gpu_text_frame_begin(void);
void n3ds_gpu_text_flush(void);
int n3ds_gpu_text_begin(float width,float height,unsigned int tint,int point,int wrapped);
int n3ds_gpu_text_draw(const struct native_text_vertex vertices[4]);
/* A batch shares one target, blend, projection and atlas. */
int n3ds_gpu_text_batch_begin(void);
int n3ds_gpu_text_batch_emit(void);
int n3ds_gpu_text_batch_end(void);
/* Untilted texture-target coordinates; no HUD layout or stereo duplication. */
int n3ds_gpu_text_offscreen_begin(float width,float height);
#ifdef HALO_N3DS_PROBE_RADAR_INLINE_CLEAR
void n3ds_gpu_text_offscreen_clear(float size);
#endif
void n3ds_gpu_text_offscreen_batch_begin(void);
int n3ds_gpu_text_offscreen_batch_end(void *texture,unsigned int border);
int n3ds_gpu_text_offscreen_draw(const struct native_text_vertex vertices[4]);
void n3ds_gpu_world_state_restore(void);
int n3ds_engine_text_active(void);
int halo_engine_text_fixture_prepare(void);
void halo_engine_text_fixture_draw(void);
#endif
