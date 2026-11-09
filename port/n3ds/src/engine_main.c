#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "engine_textures.h"
#include "engine_renderer.h"
#include "engine_renderer_features.h"
#include "engine_algorithm_profile.h"
#include "engine_allocation_profile.h"
#include "engine_dependency_trace.h"
#include "engine_parallel.h"
#include "engine_action_replay.h"

#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
#define ENGINE_LOG_NAME "halo-startup-diagnostics.log"
#define ENGINE_LOG_PREVIOUS "halo-startup-diagnostics.previous.log"
#define ENGINE_STALL_NAME "halo-startup-diagnostics-stall.log"
#define ENGINE_FATAL_NAME "halo-startup-diagnostics-fatal.log"
#elif defined(HALO_N3DS_TEXTURE_VRAM_MIRRORS)
#define ENGINE_LOG_NAME "halo-texture-vram-mirrors.log"
#define ENGINE_LOG_PREVIOUS "halo-texture-vram-mirrors.previous.log"
#elif defined(HALO_N3DS_UNUSED_TEXTURE_UNITS)
#define ENGINE_LOG_NAME "halo-unused-texture-units.log"
#define ENGINE_LOG_PREVIOUS "halo-unused-texture-units.previous.log"
#elif defined(HALO_N3DS_OPAQUE_COLOR_READS)
#define ENGINE_LOG_NAME "halo-opaque-color-reads.log"
#define ENGINE_LOG_PREVIOUS "halo-opaque-color-reads.previous.log"
#elif defined(HALO_N3DS_FRAMEBUFFER_REUSE)
#define ENGINE_LOG_NAME "halo-framebuffer-reuse.log"
#define ENGINE_LOG_PREVIOUS "halo-framebuffer-reuse.previous.log"
#elif defined(HALO_N3DS_PROBE_EFFECT_ALPHA_ZERO)
#define ENGINE_LOG_NAME "halo-effect-alpha-zero.log"
#define ENGINE_LOG_PREVIOUS "halo-effect-alpha-zero.previous.log"
#elif defined(HALO_N3DS_PROBE_NEUTRAL_MODEL_TINT)
#define ENGINE_LOG_NAME "halo-neutral-model-tint.log"
#define ENGINE_LOG_PREVIOUS "halo-neutral-model-tint.previous.log"
#elif defined(HALO_N3DS_PROBE_TEXTURE_REBINDS)
#define ENGINE_LOG_NAME "halo-texture-rebinds.log"
#define ENGINE_LOG_PREVIOUS "halo-texture-rebinds.previous.log"
#elif defined(HALO_N3DS_PROBE_WORLD_PIXEL_PASSES)
#define ENGINE_LOG_NAME "halo-world-pixel-passes.log"
#define ENGINE_LOG_PREVIOUS "halo-world-pixel-passes.previous.log"
#elif defined(HALO_N3DS_PROBE_LATE_SKY)
#define ENGINE_LOG_NAME "halo-late-sky.log"
#define ENGINE_LOG_PREVIOUS "halo-late-sky.previous.log"
#elif defined(HALO_N3DS_PROBE_OPAQUE_TERRAIN_ALPHA)
#define ENGINE_LOG_NAME "halo-opaque-terrain-alpha.log"
#define ENGINE_LOG_PREVIOUS "halo-opaque-terrain-alpha.previous.log"
#elif defined(HALO_N3DS_PROBE_PIXEL_PASSES)
#define ENGINE_LOG_NAME "halo-pixel-passes.log"
#define ENGINE_LOG_PREVIOUS "halo-pixel-passes.previous.log"
#elif defined(HALO_N3DS_PROBE_RADAR_INLINE_CLEAR)
#define ENGINE_LOG_NAME "halo-radar-inline-clear.log"
#define ENGINE_LOG_PREVIOUS "halo-radar-inline-clear.previous.log"
#elif defined(HALO_N3DS_GPU_PROBE)
#define ENGINE_LOG_NAME "halo-gpu-probe.log"
#define ENGINE_LOG_PREVIOUS "halo-gpu-probe.previous.log"
#elif defined(HALO_N3DS_BYTE_MODEL_INDICES)
#define ENGINE_LOG_NAME "halo-a30-byte-model-indices.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-byte-model-indices.previous.log"
#elif defined(HALO_N3DS_A30_RENDER_SUITE)
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-render-suite-v3.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-render-suite-v3.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-render-suite-v3.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-render-suite-v3.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MIDPOINT_SWAP_OVERLAP == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-present60-swap-overlap.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-swap-overlap.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_CHICAGO_NEUTRAL_DRAWS == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-chicago-neutral-draws.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-chicago-neutral-draws.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_CHICAGO_ALPHA_ZERO == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-chicago-alpha-zero.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-chicago-alpha-zero.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_EFFECT_HOMOGENEOUS == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-effect-homogeneous.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-effect-homogeneous.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_CLIP_BOUNDS == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-model-clip-bounds.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-clip-bounds.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_BSP_CLIP_BLOCKS == N3DS_ALGORITHM_CANDIDATE
#if HALO_N3DS_ALGO_BSP_CLIP_HIERARCHY == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-bsp-clip-hierarchy.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-bsp-clip-hierarchy.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-bsp-clip-blocks.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-bsp-clip-blocks.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_HOMOGENEOUS == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-model-homogeneous.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-homogeneous.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_REFLECTION_DIRECT == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-model-reflection-direct.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-reflection-direct.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_CHICAGO_MAP_UV == N3DS_ALGORITHM_CANDIDATE
#define ENGINE_LOG_NAME "halo-a30-chicago-map-uv.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-chicago-map-uv.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_PREPARED_VIEW == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-prepared-view.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-prepared-view.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-prepared-view.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-prepared-view.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_QUATERNION_DIRECT == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-model-quaternion.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-model-quaternion.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-model-quaternion.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-quaternion.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_VIEW_PACK == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-model-view-pack.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-model-view-pack.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-model-view-pack.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-view-pack.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_MODEL_UV_PACK == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-model-uv-pack.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-model-uv-pack.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-model-uv-pack.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-model-uv-pack.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_WATER_BASE == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-water-base.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-water-base.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-water-base.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-water-base.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_BOOLEAN_SELECTORS == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-boolean-selectors.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-boolean-selectors.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-boolean-selectors.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-boolean-selectors.previous.log"
#endif
#elif defined(HALO_N3DS_A30_BENCHMARK) && HALO_N3DS_ALGO_UV_OUTPUTS == N3DS_ALGORITHM_CANDIDATE
#ifdef HALO_N3DS_PRESENT_INTERPOLATION
#define ENGINE_LOG_NAME "halo-a30-present60-uv-outputs.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-uv-outputs.previous.log"
#else
#define ENGINE_LOG_NAME "halo-a30-uv-outputs.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-uv-outputs.previous.log"
#endif
#elif defined(HALO_N3DS_COMPACT_COMMAND_SUITE) && defined(HALO_N3DS_PRESENT_INTERPOLATION)
#define ENGINE_LOG_NAME "halo-a30-present60-command-suite.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60-command-suite.previous.log"
#elif defined(HALO_N3DS_COMPACT_COMMAND_SUITE)
#define ENGINE_LOG_NAME "halo-a30-command-suite.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-command-suite.previous.log"
#elif defined(HALO_N3DS_COMPACT_VERTEX_COMMANDS)
#define ENGINE_LOG_NAME "halo-a30-vertex-commands.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-vertex-commands.previous.log"
#elif defined(HALO_N3DS_COMPACT_TEV_COMMANDS)
#define ENGINE_LOG_NAME "halo-a30-tev-commands.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-tev-commands.previous.log"
#elif defined(HALO_N3DS_COMPACT_EFFECT_COMMANDS)
#define ENGINE_LOG_NAME "halo-a30-effect-commands.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-effect-commands.previous.log"
#elif defined(HALO_N3DS_COMPACT_SHADER_UPLOADS)
#define ENGINE_LOG_NAME "halo-a30-shader-uploads.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-shader-uploads.previous.log"
#elif defined(HALO_N3DS_COMPACT_UNIFORM_UPLOADS)
#define ENGINE_LOG_NAME "halo-a30-uniform-uploads.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-uniform-uploads.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK) && defined(HALO_N3DS_PRESENT_INTERPOLATION)
#define ENGINE_LOG_NAME "halo-a30-present60.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-present60.previous.log"
#elif defined(HALO_N3DS_A30_BENCHMARK)
#define ENGINE_LOG_NAME "halo-a30-benchmark.log"
#define ENGINE_LOG_PREVIOUS "halo-a30-benchmark.previous.log"
#else
#define ENGINE_LOG_NAME "halo-source-engine.log"
#define ENGINE_LOG_PREVIOUS "halo-source-engine.previous.log"
#endif
#ifndef ENGINE_FATAL_NAME
#define ENGINE_FATAL_NAME "halo-source-fatal.log"
#endif

static FILE *log_file;
static char log_buffer[16*1024];
static int graphics_owned;
static int error_console;
static int loading_console=1;
#include "engine_log_queue.inl"
void n3ds_log_shutdown(void) { log_async_finish(); }
/* libctru applies this during startup: New 3DS 804 MHz CPU and L2 cache. */
bool __ctru_speedup = true;
#ifdef HALO_N3DS_LOW_MEMORY_TEST
/* Explicit developer fixture: approximate the original 64 MiB process after
 * executable/stack overhead, without borrowing Azahar's extra app memory. */
u32 __ctru_heap_size = 24U*1024*1024;
u32 __ctru_linear_heap_size = 32U*1024*1024;
#endif
int halo_engine_boot(void);
static void flush_console(void)
{
    if(!error_console && n3ds_gpu_bottom_owned()) graphics_owned=1;
    if(graphics_owned) return;
    u16 width, height;
    u8 *buffer=gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &width, &height);
    GSPGPU_FlushDataCache(buffer, width*height*gspGetBytesPerPixel(gfxGetScreenFormat(GFX_BOTTOM)));
    gfxScreenSwapBuffers(GFX_BOTTOM, false);
}
void n3ds_log(const char *message)
{
    /* Keep the original failure even if the next launch replaces its session
     * log. Fatal paths are rare; close immediately to commit to the SD card. */
    if(strstr(message,"FATAL:") || strstr(message,"FAIL:") || !strcmp(message,"RESULT: FAIL")) {
        FILE *fatal=fopen("sdmc:/" ENGINE_FATAL_NAME,"a");
        if(fatal) {fprintf(fatal,"[%lld] %s\n",(long long)time(NULL),message);fclose(fatal);}
    }
    if(!error_console && n3ds_gpu_bottom_owned()) graphics_owned=1;
    if(!graphics_owned && (!loading_console || error_console)) {printf("%s\n", message);fflush(stdout);}
    if(graphics_owned && !log_thread) log_async_start();
    if(log_thread) log_enqueue(message);
    else if (log_file) {
        fprintf(log_file, "%s\n", message);
        /* Batch routine diagnostics between the existing 30-frame heartbeat.
         * Flush startup, errors and final results immediately. This removes
         * dozens of SD transactions from each diagnostic burst while keeping
         * the wall-clock test observer's frame markers promptly visible. */
        if(!graphics_owned || !strncmp(message,"STARTUP:",8) ||
           !strncmp(message,"NATIVE MEMORY:",14) || !strncmp(message,"FRONTEND FRAME:",15) ||
           !strncmp(message,"MISSION TICK END:",17) ||
           !strncmp(message,"RESULT:",7) || !strncmp(message,"EXIT:",5) || strstr(message,"FATAL:") ||
           strstr(message,"FAIL") || strstr(message,"ERROR") || strstr(message,"ASSERT"))
            fflush(log_file);
    }
    flush_console();
}
#include "engine_log_tests.inl"
static void renderer_configuration_log(void)
{
    char message[320];
    snprintf(message,sizeof(message),
        "RENDERER MODES: schema=%u textures=%s(%u) geometry=%s(%u) vertices=%s(%u)",
        (unsigned)N3DS_RENDERER_FEATURE_SCHEMA,N3DS_TEXTURE_STORAGE_NAME,(unsigned)HALO_N3DS_TEXTURE_STORAGE,
        N3DS_GEOMETRY_SOURCE_NAME,(unsigned)HALO_N3DS_GEOMETRY_SOURCE,
        N3DS_VERTEX_LAYOUT_NAME,(unsigned)HALO_N3DS_VERTEX_LAYOUT);
    n3ds_log(message);
    snprintf(message,sizeof(message),
        "PERFORMANCE CONFIG: purpose=%s ticks=%ld diagnostics=%d readback=%d validation_fixture=%d profile=%d uncaptured_profile_eligible=%d",
        N3DS_RENDERER_RUN_PURPOSE,(long)N3DS_RENDERER_CONFIGURED_MISSION_TICKS,
        N3DS_RENDERER_DIAGNOSTICS_CONFIGURED,N3DS_RENDERER_READBACK_CONFIGURED,
        N3DS_RENDERER_VALIDATION_CONFIGURED,N3DS_RENDERER_PROFILE_CONFIGURED,
        N3DS_RENDERER_UNCAPTURED_PROFILE_ELIGIBLE);
    n3ds_log(message);
    n3ds_log("PERFORMANCE CONFIG: configuration only; external capture unverified; not validation or measured performance");
    snprintf(message,sizeof(message),
        "ALGORITHM CONFIG: schema=%u instrumentation=%d profile_loop_limit=%ld verification=%d",
        (unsigned)N3DS_ALGORITHM_FEATURE_SCHEMA,N3DS_ALGORITHM_PROFILE_CONFIGURED,
        (long)HALO_N3DS_PROFILE_FRAMES,N3DS_RENDERER_ALGORITHM_VERIFICATION);
    n3ds_log(message);
#define LOG_ALGORITHM(id, name) \
    snprintf(message,sizeof(message),"ALGORITHM MODE: %s=%u",name,(unsigned)HALO_N3DS_ALGO_ ## id); \
    n3ds_log(message);
    N3DS_IMPLEMENTED_ALGORITHMS(LOG_ALGORITHM)
#undef LOG_ALGORITHM
#ifdef HALO_N3DS_PERFORMANCE_PROFILE
#ifdef HALO_N3DS_FAST_CONTRAIL_VERIFY
    n3ds_log("CONTRAIL VERIFY: count checked during point update");
#else
    n3ds_log("CONTRAIL VERIFY: full point lists checked before and after update");
#endif
#endif
}
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
static void append_stall_report(void)
{
    FILE *stall = fopen("sdmc:/" ENGINE_STALL_NAME, "r");
    if (!stall) return;
    n3ds_log("STARTUP DIAGNOSTICS WATCHDOG:");
    char line[384];
    while (fgets(line, sizeof(line), stall)) {
        line[strcspn(line, "\r\n")] = '\0';
        n3ds_log(line);
    }
    fclose(stall);
}
#endif
int main(void)
{
    n3ds_allocation_profile_start();
    gfxInitDefault(); gfxSetDoubleBuffering(GFX_BOTTOM, false); consoleInit(GFX_BOTTOM, NULL);
    printf("\n\n             Halo3DS\n\n\n          Loading game data...\n\n    Initial loading may take a while.\n");
    fflush(stdout);flush_console();
    remove("sdmc:/" ENGINE_LOG_PREVIOUS);
    rename("sdmc:/" ENGINE_LOG_NAME,"sdmc:/" ENGINE_LOG_PREVIOUS);
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
    remove("sdmc:/halo-startup-diagnostics-stall.previous.log");
    rename("sdmc:/" ENGINE_STALL_NAME,"sdmc:/halo-startup-diagnostics-stall.previous.log");
    remove("sdmc:/halo-startup-diagnostics-fatal.previous.log");
    rename("sdmc:/" ENGINE_FATAL_NAME,"sdmc:/halo-startup-diagnostics-fatal.previous.log");
#endif
    log_file = fopen("sdmc:/" ENGINE_LOG_NAME, "w");
    if(log_file) setvbuf(log_file,log_buffer,_IOFBF,sizeof(log_buffer));
    n3ds_log("Halo native engine bootstrap");
    n3ds_log("BUILD: Halo3DS Original-model adaptive rendering 2026-10-05");
    renderer_configuration_log();
#ifndef HALO_N3DS_DEPENDENCY_TRACE
    n3ds_log("DEPENDENCY TRACE: enabled=0");
#endif
    n3ds_trace_initialize();
#ifdef HALO_N3DS_LOW_MEMORY_TEST
    n3ds_log("LOW MEMORY FIXTURE: 24 MiB application heap + 32 MiB linear heap");
#endif
    bool new_model=false;APT_CheckNew3DS(&new_model);
    n3ds_log(new_model?"MODEL: New 3DS family; high-speed CPU and L2 cache requested":"MODEL: original 3DS/2DS family; standard CPU speed");
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
    const char *preflight_error = !log_file ? "report-unavailable" : !new_model ? "unsupported-model" : NULL;
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS_OPTIMIZED
    const char *preset = "optimized";
#else
    const char *preset = "reference";
#endif
    char report_info[112];
    snprintf(report_info, sizeof(report_info), "STARTUP DIAGNOSTICS REPORT: schema=1 build_id=%08lx preset=%s",
        (unsigned long)HALO_N3DS_CHECKPOINT_BUILD, preset);
    n3ds_log(report_info);
    n3ds_log("STARTUP DIAGNOSTICS: automatic A30 route; regular gameplay sample followed by isolated paused GPU phases");
    n3ds_log("STARTUP DIAGNOSTICS: profiler overhead unmeasured; this run is for diagnostic attribution");
#endif
#if defined(HALO_N3DS_PERFORMANCE_PROFILE) || defined(HALO_N3DS_GPU_PROBE)
    s64 clock_mhz = -1, l2 = -1;
    /* Luma exposes the active mode separately from libctru's request. */
    if (new_model) {
        if (R_FAILED(svcGetSystemInfo(&clock_mhz, 0x10001, 0))) clock_mhz = -1;
        if (R_FAILED(svcGetSystemInfo(&l2, 0x10001, 2))) l2 = -1;
    }
    char model_info[128];
    snprintf(model_info, sizeof(model_info), "PERFORMANCE HARDWARE: model=%d clock_mhz=%lld l2=%lld (-1 unavailable)",
        n3ds_platform_console_model(), (long long)clock_mhz, (long long)l2);
    n3ds_log(model_info);
#endif
#ifdef HALO_N3DS_PERFORMANCE_PROFILE
    /* The application region, process limit, and libctru arenas are distinct.
     * A larger region does not necessarily increase GPU-readable memory. */
    extern u32 __ctru_heap_size, __ctru_linear_heap_size;
    s64 commit_limit = -1;
    Handle limits;
    ResourceLimitType commit_type = RESLIMIT_COMMIT;
    if (R_SUCCEEDED(svcGetResourceLimit(&limits, CUR_PROCESS_HANDLE))) {
        if (R_FAILED(svcGetResourceLimitLimitValues(&commit_limit, limits, &commit_type, 1)))
            commit_limit = -1;
        svcCloseHandle(limits);
    }
    char memory_info[192];
    snprintf(memory_info, sizeof(memory_info),
        "PERFORMANCE MEMORY: app_region_bytes=%lu commit_limit_bytes=%lld heap_capacity_bytes=%lu linear_capacity_bytes=%lu texture_cache_mib=%u",
        (unsigned long)osGetMemRegionSize(MEMREGION_APPLICATION), (long long)commit_limit,
        (unsigned long)__ctru_heap_size, (unsigned long)__ctru_linear_heap_size,
        (unsigned int)HALO_N3DS_TEXTURE_CACHE_MIB);
    n3ds_log(memory_info);
#endif
    /* Retain capacity for two A10 corridor views and their effects, while
     * keeping every draw and the live command-capacity checks. */
    n3ds_parallel_initialize();
    int gpu_ready =
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
        !preflight_error &&
#endif
        C3D_Init(8 * C3D_DEFAULT_CMDBUF_SIZE);
    if (gpu_ready) n3ds_log("GPU command buffer: 2097152 bytes (two-eye capacity)");
    int programs_ready = gpu_ready && n3ds_gpu_programs_initialize();
    int textures_ready = programs_ready && n3ds_gpu_textures_initialize();
    extern void n3ds_stall_watch_start(void),n3ds_stall_watch_stop(void);
    n3ds_stall_watch_start();
    int result = textures_ready ? halo_engine_boot() : 1;
    if(!n3ds_action_replay_finish(result==0)) result=1;
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
    n3ds_trace_stop();
#else
    n3ds_trace_finish();
#endif
    n3ds_gpu_algorithm_profile_finish();
    n3ds_algorithm_profile_stop();
    extern void n3ds_prefetch_shutdown(void);
    n3ds_prefetch_shutdown();
    n3ds_stall_watch_stop();
    if (textures_ready) {
        /* A frontend failure can bypass normal map teardown and then wait on
         * the error screen. Join texture preparation before freeing its private
         * buffers, close both archive indexes, and fence resident textures
         * while GPU services and their map owners are still available. This
         * is also safe after the successful frontend has already flushed. */
        texture_cache_flush();
        n3ds_gpu_textures_dispose();
    }
    if (gpu_ready) C3D_Fini();
    if (programs_ready) n3ds_gpu_programs_dispose();
    if(result) {
        /* Render targets may have taken over the bottom screen. Reclaim it
         * after GPU shutdown so a startup failure cannot look like a freeze. */
        error_console=1;graphics_owned=0;gfxSet3D(false);gfxSetDoubleBuffering(GFX_BOTTOM,false);
        consoleInit(GFX_BOTTOM,NULL);
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
        if (preflight_error && !new_model) printf("Halo Startup Diagnostics\n\nRequires a New 3DS or New 2DS XL.\n\nPress START to exit.\n");
        else if (preflight_error) printf("Halo Startup Diagnostics\n\nCannot open the SD report.\n\nPress START to exit.\n");
        else
#endif
        printf("Halo3DS stopped during startup.\n\nSee " ENGINE_LOG_NAME "\non the SD card for details.\n\nPress START to exit.\n");
        fflush(stdout);flush_console();
    }
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
    /* Put the terminal status after the async writer has drained. A slow SD
     * may drop routine lines, but the final summary has its own flush. */
    log_async_finish();
    if (!log_file && !preflight_error) log_file = fopen("sdmc:/" ENGINE_LOG_NAME, "a");
    n3ds_trace_finish();
    append_stall_report();
    if (preflight_error) {
        char message[144];
        snprintf(message, sizeof(message), "STARTUP DIAGNOSTICS RESULT: status=partial reason=%s", preflight_error);
        n3ds_log(message);
    } else {
        extern void n3ds_startup_diagnostics_finish(int engine_result);
        n3ds_startup_diagnostics_finish(result);
    }
    u64 error_wait_start = svcGetSystemTick();
#endif
    n3ds_log(result ? "RESULT: FAIL" : "RESULT: PASS");
    n3ds_parallel_shutdown();log_async_finish();
    if (log_file) {fclose(log_file);log_file=NULL;}
    while (result && aptMainLoop()
#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
        && svcGetSystemTick() - error_wait_start < 8ULL * SYSCLOCK_ARM11
#endif
    ) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        flush_console(); gspWaitForVBlank();
    }
    gfxExit(); return result;
}
