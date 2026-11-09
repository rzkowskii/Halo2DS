#ifndef HALO_N3DS_ENGINE_SOUNDS_H
#define HALO_N3DS_ENGINE_SOUNDS_H
unsigned int n3ds_engine_sound_count(void);
unsigned int n3ds_engine_sound_bytes(void);
void n3ds_engine_sounds_unload(void);
int halo_engine_sound_tests(void);
struct sound_permutation;
void n3ds_engine_sound_stage_next(const struct sound_permutation *sound);

/* Game-thread API for private encoded SD reads. No tag or cache pointer
 * crosses into the worker. Linked hints use handle zero and match by range
 * within the same map generation. */
#define N3DS_SOUND_READ_LIMIT (128u*1024)
#define N3DS_SOUND_LINK_READ_LIMIT (64u*1024)
struct n3ds_sound_read_key {
    unsigned int map_generation,handle,offset,bytes;
};
int n3ds_sound_reader_supported(void);
void n3ds_sound_reader_offer(const char *path,const struct n3ds_sound_read_key *key);
void n3ds_sound_reader_predict(const char *path,const struct n3ds_sound_read_key *key);
void n3ds_sound_reader_predict_neighbor(const char *path,const struct n3ds_sound_read_key *key);
#ifdef HALO_N3DS_ASYNC_SOUND_IDLE
enum n3ds_sound_read_status {
    N3DS_SOUND_READ_UNAVAILABLE, N3DS_SOUND_READ_WAITING,
    N3DS_SOUND_READ_READY, N3DS_SOUND_READ_FAILED
};
/* Check the admitted pending request without consuming or cancelling it. */
enum n3ds_sound_read_status n3ds_sound_reader_status(const struct n3ds_sound_read_key *key);
#endif
/* A miss pauses new speculative chunks. Resume after the synchronous read. */
int n3ds_sound_reader_copy(const struct n3ds_sound_read_key *key,void *destination);
void n3ds_sound_reader_resume(void);
void n3ds_sound_reader_discard(unsigned int map_generation,unsigned int handle);
void n3ds_sound_reader_cancel(void);
void n3ds_sound_reader_shutdown(void);
#endif
