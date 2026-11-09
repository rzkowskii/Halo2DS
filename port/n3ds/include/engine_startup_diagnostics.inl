/* The dedicated launcher drives the real campaign with the same fixed-step
 * input, checkpoint and render paths as ordinary play. */
#include "engine_files.h"
#include "engine_state_path.h"
#include "interface/player_ui.h"
#include "objects/objects.h"
#include "saved games/player_profile.h"
#include "text/unicode.h"

enum {
    STARTUP_DIAGNOSTIC_MAX_LOOPS = 3000,
    STARTUP_DIAGNOSTIC_MAX_SECONDS = 300,
    STARTUP_DIAGNOSTIC_MENU_WARMUP = 30,
    STARTUP_DIAGNOSTIC_ROUTE_SETTLE_TICKS = 150,
    STARTUP_DIAGNOSTIC_RECOVERY_TICKS = 105,
    STARTUP_DIAGNOSTIC_HELD_WARMUP = 30,
    STARTUP_DIAGNOSTIC_HELD_SAMPLES = 300
};

enum startup_diagnostics_stage {
    DIAGNOSTIC_STARTUP, DIAGNOSTIC_LOAD, DIAGNOSTIC_ROUTE, DIAGNOSTIC_HELD, DIAGNOSTIC_GPU
};

static const char *const startup_diagnostics_stages[] = {
    "startup", "campaign-load", "route", "held-view", "gpu-probe"
};
static const char startup_diagnostics_map[] = "levels\\a30\\a30";
static const float startup_diagnostics_waypoints[][3] = {
    {31.1f,-100.3f}, {30.6f,-98.5f}, {28.9f,-98.2f},
    {25.f,-98.3f}, {20.8f,-98.4f}, {15.f,-98.4f},
    {9.f,-98.4f}, {3.f,-98.3f}, {0.f,-98.f},
    {-1.7f,-96.45f,58.8f}, {-2.67f,-94.53f,59.24f}, {-2.54f,-92.41f,59.35f},
    {-3.f,-90.34f,60.06f}, {-2.83f,-88.32f,59.34f}, {-1.92f,-86.38f,59.10f},
    {-1.03f,-82.19f,58.68f}, {-.49f,-80.15f,58.73f}, {.65f,-76.19f,59.46f},
    {1.23f,-74.23f,58.68f}, {1.95f,-71.88f,60.64f}, {2.66f,-69.79f,61.21f},
    {3.57f,-67.23f,61.57f}, {4.71f,-66.27f,62.05f}
};

static struct {
    long long started;
    long waypoint_started, last_tick, loops;
    unsigned int waypoint, recoveries, held_loops;
    enum startup_diagnostics_stage stage;
    const char *status, *reason;
} startup_diagnostics;

long long n3ds_engine_tick_frequency(void);

static void startup_diagnostics_stop(const char *status, const char *reason)
{
    startup_diagnostics.status = status;
    startup_diagnostics.reason = reason;
}

void n3ds_startup_diagnostics_finish(int engine_result)
{
    if (engine_result)
        startup_diagnostics_stop("partial", "frontend-error");
    else if (!startup_diagnostics.status)
        startup_diagnostics_stop("cancelled", "app-exit");
    char line[256];
    snprintf(line, sizeof(line),
        "STARTUP DIAGNOSTICS RESULT: status=%s reason=%s stage=%s loops=%ld tick=%ld waypoints=%u/%u recoveries=%u held_loops=%u",
        startup_diagnostics.status, startup_diagnostics.reason, startup_diagnostics_stages[startup_diagnostics.stage],
        startup_diagnostics.loops, startup_diagnostics.last_tick, startup_diagnostics.waypoint,
        (unsigned int)NUMBEROF(startup_diagnostics_waypoints), startup_diagnostics.recoveries,
        startup_diagnostics.held_loops);
    n3ds_log(line);
}

static void startup_diagnostics_stage(enum startup_diagnostics_stage stage)
{
    startup_diagnostics.stage = stage;
    char line[128];
    snprintf(line, sizeof(line), "STARTUP DIAGNOSTICS STAGE: %s tick=%ld",
        startup_diagnostics_stages[stage], game_time_get());
    n3ds_log(line);
}

static int startup_diagnostics_profile(void)
{
    wchar_t name[] = L"Diagnostics";
    long indices[100], index = NONE;
    word count = NUMBEROF(indices);
    struct player_profile profile;
    player_profiles_enumerate_available_to_local_player_index(NONE, &count, indices, FALSE);
    for (unsigned int i = 0; i < count; ++i) {
        if (player_profile_get(indices[i], &profile) && !ustrcmp(profile.player_name, name)) {
            index = indices[i];
            break;
        }
    }
    if (index == NONE) index = player_profile_new(0, name);
    if (index == NONE || !player_profile_get(index, &profile)) return 0;
    player_ui_set_active_player_profile(0, index, &profile);

    /* This path resolves within diagnostic-state. The test must start at
     * mission entry even when a previous run finished writing a checkpoint. */
    char directory[256], path[256];
    if (!player_ui_get_path_to_local_player_profile_directory(0, directory)) return 0;
    static const char *const slots[] = {"savegame.n3ds", "savegame.n3ds.bak", "savegame.n3ds.tmp"};
    for (unsigned int i = 0; i < NUMBEROF(slots); ++i) {
        int length = snprintf(path, sizeof(path), "%s\\%s", directory, slots[i]);
        if (length < 0 || (unsigned int)length >= sizeof(path)) return 0;
        struct native_file_info info;
        if (n3ds_file_stat(path, &info) && !n3ds_file_delete(path, 0)) return 0;
    }
    return 1;
}

static void startup_diagnostics_begin(void)
{
    if (!n3ds_engine_map_available(startup_diagnostics_map)) {
        startup_diagnostics_stop("partial", "a30-data-unavailable");
        return;
    }
    if (!startup_diagnostics_profile()) {
        startup_diagnostics_stop("partial", "scratch-profile-unavailable");
        return;
    }
    /* Match the verified A30 route's Normal difficulty and input seed. The
     * Performance texture setting and live audio keep this a playable load. */
    n3ds_engine_world_texture_request(N3DS_WORLD_TEXTURE_32_MIPS);
    *get_global_local_random_seed_address() = 0x48524150u;
    main_set_difficulty(_game_difficulty_level_normal);
    main_set_map_name(startup_diagnostics_map);
    n3ds_log("STARTUP DIAGNOSTICS RUN: map=a30 difficulty=normal audio=normal texture=performance state=" N3DS_STATE_ROOT);
    startup_diagnostics_stage(DIAGNOSTIC_LOAD);
}

static int startup_diagnostics_running(long loops)
{
    startup_diagnostics.loops = loops;
    if (startup_diagnostics.status) return 0;
    if (loops >= STARTUP_DIAGNOSTIC_MAX_LOOPS ||
        n3ds_engine_ticks() - startup_diagnostics.started >=
            STARTUP_DIAGNOSTIC_MAX_SECONDS * n3ds_engine_tick_frequency()) {
        startup_diagnostics_stop("partial", startup_diagnostics.stage == DIAGNOSTIC_GPU ?
            "gpu-probe-timeout" : "route-timeout");
        return 0;
    }
    return 1;
}

int n3ds_startup_diagnostics_gpu_requested(void)
{
    return startup_diagnostics.stage == DIAGNOSTIC_GPU && !startup_diagnostics.status;
}

static void startup_diagnostics_input(struct native_input_sample *sample)
{
    memset(sample, 0, sizeof(*sample));
    if (main_menu_is_active() || n3ds_startup_diagnostics_gpu_requested()) return;
    if (cinematic_can_be_skipped()) main_skip_cinematic();
    long tick = game_time_get();
    if (cinematic_in_progress() || tick <= STARTUP_DIAGNOSTIC_ROUTE_SETTLE_TICKS ||
        local_player_get_next(NONE) != 0) return;
    struct player_control *control = player_control_get(0);
    if (control->unit_index == NONE) return;
    if (startup_diagnostics.stage == DIAGNOSTIC_LOAD) {
        startup_diagnostics_stage(DIAGNOSTIC_ROUTE);
        startup_diagnostics.waypoint_started = tick;
    }
    if (startup_diagnostics.stage == DIAGNOSTIC_ROUTE) {
        const real_point3d *position = &unit_get(control->unit_index)->object.position;
        const float *waypoint = startup_diagnostics_waypoints[startup_diagnostics.waypoint];
        float dx = waypoint[0] - position->x, dy = waypoint[1] - position->y;
        if (dx*dx + dy*dy < .45f*.45f) {
            ++startup_diagnostics.waypoint;
            startup_diagnostics.waypoint_started = tick;
            char line[128];
            snprintf(line, sizeof(line), "STARTUP DIAGNOSTICS ROUTE: waypoint=%u tick=%ld",
                startup_diagnostics.waypoint, tick);
            n3ds_log(line);
            if (startup_diagnostics.waypoint == NUMBEROF(startup_diagnostics_waypoints))
                startup_diagnostics_stage(DIAGNOSTIC_HELD);
        } else if (waypoint[2] && tick - startup_diagnostics.waypoint_started >= STARTUP_DIAGNOSTIC_RECOVERY_TICKS) {
            real_point3d landing = {{waypoint[0], waypoint[1], waypoint[2]}};
            object_set_position(control->unit_index, &landing, NULL, NULL);
            startup_diagnostics.waypoint_started = tick;
            ++startup_diagnostics.recoveries;
            char line[128];
            snprintf(line, sizeof(line), "STARTUP DIAGNOSTICS ROUTE: recovered_waypoint=%u tick=%ld",
                startup_diagnostics.waypoint + 1, tick);
            n3ds_log(line);
        } else {
            float yaw = atan2f(dy, dx);
            control->desired_angles.yaw = yaw < 0 ? yaw + 2.f*(float)M_PI : yaw;
            control->desired_angles.pitch = 0;
            sample->circle_y = 150;
        }
    }
    if (startup_diagnostics.stage == DIAGNOSTIC_HELD) {
        control->desired_angles.yaw = .535f;
        control->desired_angles.pitch = .37f;
    }
}

static int startup_diagnostics_work_sample(void)
{
    /* Discard the held camera's settling frames and start a fresh percentile
     * window after the approach. Leave the route windows as recorded. */
    return startup_diagnostics.stage != DIAGNOSTIC_HELD ||
        startup_diagnostics.held_loops >= STARTUP_DIAGNOSTIC_HELD_WARMUP;
}

static void startup_diagnostics_end_loop(long unit)
{
    long tick = game_time_get();
    if (startup_diagnostics.stage >= DIAGNOSTIC_ROUTE &&
        (tick < startup_diagnostics.last_tick || main_menu_is_active())) {
        startup_diagnostics_stop("partial", "route-interrupted");
        return;
    }
    startup_diagnostics.last_tick = tick;
    if (startup_diagnostics.stage == DIAGNOSTIC_HELD) {
        if (unit == NONE || cinematic_in_progress() || game_time_get_paused() || ui_widgets_active()) {
            startup_diagnostics.held_loops = 0;
            return;
        }
        if (++startup_diagnostics.held_loops ==
            STARTUP_DIAGNOSTIC_HELD_WARMUP + STARTUP_DIAGNOSTIC_HELD_SAMPLES) {
            n3ds_trace_stop();
            game_time_set_paused(TRUE);
            sound_pause(TRUE);
            startup_diagnostics_stage(DIAGNOSTIC_GPU);
        }
    } else if (startup_diagnostics.stage == DIAGNOSTIC_GPU) {
        int result = n3ds_gpu_startup_diagnostics_status();
        if (result > 0) startup_diagnostics_stop("complete", "completed");
        else if (result < 0) startup_diagnostics_stop("partial", "gpu-probe-aborted");
    }
}
