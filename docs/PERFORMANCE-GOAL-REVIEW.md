# Native 60 FPS goal: current review

An A30 terrain audit found 3,552 bump draws with the cutout test enabled
even though their base maps contained only opaque texels. This included
all 242 bump draws in the held-view window. The opt-in
`--probe-opaque-terrain-alpha` comparison caches that fact on the existing
immutable texture revision. It checks the bound sampler and every mip,
and retains the test for genuine cutouts, border sampling and unknown
textures. Its host fixture covered opaque and mixed mip chains, revision
changes and sampler fallback. A private route bypassed all 3,552 tests
and matched all seven 400×240 Azahar captures in RGBA and D24S8.
The built probe also completed all five paused phases at the held
dropship, with two eligible draws per frame and both bypassed only in
candidate phases. All five full color and depth/stencil captures matched.
The emulator sessions exited.

A separate lightmap-only experiment retained the canonical decoder and
packed the fully opaque texels as RGB8 before publication. All seven
captures matched in both planes. Eight lightmaps saved a total of 8 KiB
of resident payload on this route. The temporary packing allocation
and the CPU consumers that assume RGBA8 still need a broader review
before using this layout outside the fixture. The audit and both image
comparisons are under `build/performance-a30-terrain-texture-audit`.
The normal-audio alpha probe and its console instructions are under
`build/performance-a30-opaque-terrain-alpha`. Physical queue timing
remains pending.

The separate `--probe-late-sky` launcher compares the original sky with
the stencil-masked sky in five alternating phases at the same paused
400×240 view. Its guard checks the weapon and hands before deferring the
sky draw and preserves the original order for active camouflage and
immediate blended materials. The log distinguishes a timed fallback from
a timed candidate frame.

Two scripted Azahar/Vulkan runs reached the held A30 dropship view. With
the starting assault rifle, the guard kept the original order throughout.
After switching to the pistol, the candidate ran on all 24 timed frames
in its two phases. Five paused-view captures matched in RGBA, depth and
the original stencil bits. Only the new terrain stencil bit differed,
at 61,062 pixels in each candidate capture. Both runs exited. The
normal-audio console launcher, instructions and log comparison tool are
under `build/performance-a30-late-sky-probe`. Native timing and image
validation are pending.

Private A30 sky experiments deferred the sky's GPU draws until after the
first-person weapon and terrain. Sky animation, poses and lens-flare
queueing stayed in their original CPU phase. The first test missed the
separate sky water path and changed color at four route captures. With
the same visibility test applied to water, all seven regular 400×240
Azahar/Vulkan captures matched the reference in RGBA and D24S8.

A pre-sky depth readback found that all 96,000 pixels were already covered
in three approach samples. In the held dropship view, terrain covered
54,312 pixels and the weapon covered another 10,183, for 67.2% combined.
These are framebuffer coverage counts. They do not measure shaded sky
fragments or PICA queue savings.

A second private variant marked opaque terrain with stencil bit 1 while
preserving the weapon's bit 0. The sky then used both bits as a visibility
mask. This avoids treating the clear depth value as proof that no opaque
fragment was written. In all seven captures, RGBA, depth and the original
stencil bits matched. The added terrain bit stayed unchanged through the
rest of each sampled frame. All five emulator runs exited. The overlays,
route captures and ten-map weapon material audit are under
`build/performance-a30-late-sky`. Some first-person weapons contain
immediate blended materials that can depend on the sky already being
drawn. These experiments remain private pending an admission rule for
those materials and native timing and image checks.

The independent `--byte-model-indices` candidate packs recurring static
world-model triangle lists into the existing resident index arena. It
preserves triangle order and materials and falls back to the normal
16-bit path for wider indices or when residency is unavailable. The host
fixture exercised repeat admission, the 255/256 boundary, capacity, failed
cache flushes, epoch changes and low linear-memory headroom.

With the fixed 400×240 renderer suite, all seven A30 Azahar captures matched
the reference byte for byte in color and depth/stencil. Camera samples and
the final simulation tick also matched. Across 1,320 reported frames,
32,993 draws used byte indices for 10,317,147 elements. The resident arena
peak fell from 118,404 to 109,424 bytes, with 112 resident uploads in each
mission run. The element count measures submitted payload, which the GPU
can fetch and cache differently on hardware. Both emulator runs used fixed
inputs and muted audio and exited. The normal-audio console pair under
`build/performance-a30-byte-residency` keeps both launchers at fixed
400×240 with the regular scheduler. Hardware timing is pending.

An earlier private A30 test fed authored strips directly to PICA and
matched the same seven captures. It submitted about 37% fewer index
elements on the converted draws, alongside 12,984,607 degenerate connector
triangles over the route. The asset audit found 135,895 connector triangles
and 138,123 nondegenerate triangles across A30's 789 authored strips.
The resident byte candidate keeps the existing connector removal.

Renderer suite v3 combines the earlier model, shader and command changes
with hierarchical terrain culling. Both launchers use the same 400×240
settings, 4 MiB texture budget, touch controls and `--present-60` scheduler.
The candidate writes `halo-a30-present60-render-suite-v3.log`.

All seven regular route captures and both opening presentation captures
matched full RGBA and D24S8 in Azahar. At the held view the combined
candidate omitted 991 model and 1,716 terrain triangles per regular frame.
The fixtures used fixed inputs, muted audio and the New 2DS XL path in
Azahar. The opening in-between capture widened the deadline. The console
launchers use normal audio and the ordinary scheduling deadlines. All four
emulator sessions exited. The files under `build/performance-a30-render-suite-v3`
include a selected-window log comparison that checks camera and hardware
settings and counts acknowledgements separately from submissions. Native
frame delivery and image parity remain to be measured.

The opt-in `--algo-bsp-clip-hierarchy candidate` adds up to four 8-triangle
subgroups to each 32-triangle terrain group. Fully rejected groups are
skipped immediately. Intersecting groups test their subgroups only against
the relevant planes. Both variants compare projection bytes once per view
and reuse the retained index list across matching views.

Against the flat 32-triangle reference, the hierarchy omitted 725 more
triangles per frame on average across the 90 sampled approach frames. It
performed 71,111 plane tests, compared with 26,026. At the held view it
omitted 1,716 triangles per frame versus 1,042, with zero plane tests in
both steady windows after cache warm-up. The held metadata and index
allocations rose from 14,320 and 57,504 bytes to 71,600 and 59,262 bytes.
The four 400×240 Azahar snapshots matched every RGBA and D24S8 word. The
private route used fixed inputs and muted audio and both runs exited.
The fixed-resolution console pair under `build/performance-a30-bsp-clip-hierarchy`
isolates the CPU and GPU trade-off. Physical timing remains pending.

The opt-in `--algo-bsp-clip-blocks candidate` caches bounds for contiguous
terrain blocks after cluster filtering. A filtered-list revision invalidates
the bounds when its indices change. The cache reuses compacted indices for
matching views or rejection masks and protects same-frame GPU readers.
Admission covers ordinary New-model mono views and streams of 32 triangles
through 16,384 index elements. Metadata and index storage each have a 256 KiB
budget. The A30 held sample used 14,320 metadata bytes and 57,504 index bytes.

The host checks covered retained order, cluster-list changes, same-frame
buffer ownership, allocation fallback and the existing adjacent batch path.
The 400×240 Azahar pair matched all RGBA and D24S8 words at ticks 618, 660,
690 and 1305. In each steady 30-frame held window it omitted 31,260 triangles
and reused 330 cached lists with zero bounds builds. Both scripted runs used
fixed input and muted audio and exited cleanly. The fixed-400 console pair
is staged under `build/performance-a30-bsp-clip-blocks`, and the candidate
also compiles with `--present-60`. Native image parity and timing are pending.

A private terrain audit sampled actual submissions after cluster filtering,
material frustum checks and adjacent batching. At the held A30 dropship
view it found 2,522 of 4,223 triangles entirely beyond a common lateral
clip plane per triangle. A further 225 met the diagnostic back-face test.
Across ten sampled views, 25,128 of 40,948 submitted triangles met the
lateral-plane test. These are float32 diagnostic counts, with a guard band,
from ordinary mono terrain submissions. The audit retained the original
draws, and all four keyframes matched the v2 candidate in RGBA and D24S8.
Receipts are under `build/performance-a30-terrain-rejection-audit`.

A second private run omitted contiguous blocks of up to 32 triangles when
their entire vertex box was beyond a side plane. It copied surviving spans
in their original order into the existing per-frame index arena. It omitted
1,054 of 4,223 triangles at the held view and 12,925 of 40,948 across all
ten sampled views. The four keyframes again matched exactly in RGBA and
D24S8, and both emulator runs exited. Both used deterministic controls and
muted audio. The prototype rebuilt bounds at each sample. Those results
guided the retained candidate above.

The second A30 renderer suite adds the homogeneous model and effect shader
paths and cached model culling to the earlier prepared-view, compact-command
and shader candidates. Both native launchers use 400×240, a 4 MiB texture
cache, the same `--present-60` scheduler, normal audio and touch controls.
The candidate writes `halo-a30-present60-render-suite-v2.log`. The staged
pair and console instructions are in `build/performance-a30-render-suite-v2`.

The private Azahar runs matched full RGBA and D24S8 for the opening keyframe
and midpoint at tick 295 and the regular views at ticks 618, 660, 690 and
1305. The later captures were taken immediately after the regular submission.
The candidate reproduced the isolated culling counts at the moving and held
views. The fixtures used deterministic input, muted audio and enabled the
New 2DS XL presentation path in Azahar. The opening fixture widened the
optional frame deadline. All four emulator runs exited. The production
launchers use the ordinary deadlines and remain pending native verification.

The opt-in `--algo-model-clip-bounds candidate` retains one local box per
used palette slot for immutable model streams. The normal mono view tests
camera-relative boxes against the four lateral clip planes. A draw is
omitted only when every contributing bone is outside the same plane after
the arithmetic margin. Scenery culling operates on complete batches in
submission order. Admission is limited to immutable, ordinary world draws
with supported weights in regular mono views. The minimum is 32 triangles.

The host test passed 12,000 randomized boxes with directed float24
significand rounding models, including blend cases and explicit lateral
clip boundaries. The paired 400×240 A30 runs matched full RGBA and D24S8
at the first tractor-beam submission and the held dropship view. The
candidate omitted 298,279 triangles across the 30-frame window ending at
tick 690 and 991 per frame in the sampled held windows. Those are omitted
submissions and do not measure physical GPU or CPU time. The private runs
used deterministic input and muted audio and both exited. Native fixed-400
launchers with normal audio and the log guide are under
`build/performance-a30-model-clip-bounds`. Hardware parity and timing are
still required before default enablement.

A private A30 model audit used the same bone rows, camera rebasing and
projection as the ordinary rigid and skinned draw path. It sampled actual
indexed submissions and used a deliberately wide float32 clip guard. At
the first tractor-beam submission (tick 618), 30 of 33 sampled draws and
7,123 of 7,735 triangles were entirely beyond one clip plane. At the held
dropship view, five of 98 draws and 991 of 20,024 triangles met that test.
Sky, first-person, decal, CPU fallback and other passes were outside the
sample.

The private per-bone bounds prototype skipped a draw only when its bounds
and the per-vertex audit agreed on the rejecting plane. Across 16 sampled
views it skipped 94 draws and 26,560 triangles. At the beam it skipped all
30 eligible draws. At the held capture it skipped three draws containing
691 triangles. The full 400×240 color and D24S8 buffers matched at both
captures. Both deterministic runs disabled audio and exited cleanly.
The files under `build/performance-a30-model-clip-audit` contain the fixture,
counts and comparison. A production implementation still needs retained
bounds, a hardware arithmetic error bound and native CPU/GPU timing. The
diagnostic rebuilt bounds and checked every referenced vertex per sample.

The opt-in `--algo-effect-homogeneous candidate` uses PICA `DPHI` in packed
particles, Chicago effects, holograms, water and glass. Four programs remove
two position setup instructions per vertex. Glass removes one and retains
the temporary XYZ position shared by its posed and unposed branches.
Hologram projection and projective texture rows preserve their operands and
output masks. The shader switch composes with the existing water base and
Boolean candidates, and has its own benchmark pair.

The 400×240 Azahar pair matched every RGBA and D24S8 word across 48 synthetic
cases covering indexed and non-indexed particles, projective Chicago,
holograms, water and posed or unposed glass. The paired scripted route also
matched at tick 618, the first tractor-beam submission, and tick 1305, the
held dropship view. The route used fixed input and RNG with audio disabled.
The production pair uses normal audio and the regular presentation policy.
Both launchers and test instructions are under
`build/performance-a30-effect-homogeneous`. Native timing remains pending.
The emulator runs exited.

The opt-in `--algo-model-homogeneous candidate` replaces the bone and
projection position dot products in ten rigid and skinned PICA programs.
`DPHI` supplies the homogeneous 1 in the first operand, removing three
setup instructions on every vertex path. The assembled programs preserve
the original matrix rows, indexed bone addressing and destination masks.
Normals, skin weights, materials and output semantics use the original
paths. The candidate is independent of the A30 renderer suite.

At 400×240, the paired Azahar builds matched every RGBA and D24S8 word
across 32 lighting, detail and reflection cases, 48 basic/depth/cloak
transform cases, and the scripted held A30 dropship view at tick 1305.
The transform cases included zero and mixed weights and palette index 28.
The private A30 fixture disabled audio and used deterministic inputs.
The native fixed-resolution pair under `build/performance-a30-model-homogeneous`
keeps normal audio and uses the regular scheduler to isolate GPU queue
time. The hardware speedup remains unmeasured. The emulator runs exited.

The first `--a30-render-suite` benchmark combined prepared views, all five
compact command paths and the UV output, Boolean selector, water base,
model view, model quaternion, model reflection and Chicago map UV shader
candidates. The 400×240 reference and suite launchers both use the same
`--present-60` scheduler, touch controls and 4 MiB texture-cache budget.
Both are staged under `build/performance-a30-render-suite/SD-card/3ds`.

Azahar matched every RGBA and D24S8 word for the opening keyframe and
midpoint at tick 295 and the held dropship view at tick 1305. The private
image fixtures used fixed input and RNG, disabled audio and enabled the
New 2DS XL path in Azahar. Only the opening fixture widened the midpoint
deadline to obtain a matching pair. The console launchers use the normal
deadlines and sound. All four emulator runs exited. Compare acknowledged
presentations, GPU queue time and skipped midpoints on hardware using the
normal memory grant. The Dev1 memory test remains a separate comparison.

The rigid and skinned lit shaders now have an independent
`--algo-model-reflection-direct` candidate. One Boolean selects the output
path. Both paths write the original exported values directly, and reflective
vertices skip the unused diffuse defaults. Against the reference, the shader
executes four fewer instructions for diffuse/detail vertices and seven fewer
for reflective vertices. The same variant builds with the other model shader
candidates and `--present-60`.

The paired 400×240 Azahar fixture matched all 32 RGBA and D24S8 pairs,
including reflective-to-diffuse transitions across a command-list split.
The scripted A30 held view matched all color and depth/stencil words at tick
1305. The route used disabled audio for repeatability and recorded 118,851
lit draws, including 25,417 reflective draws. Those counts are neither vertex
invocations nor physical timing. The fixed-resolution pair is under
`build/performance-a30-model-reflection-direct`. The emulator runs exited.

The same route's skin-stream audit found that only about 14% of the sampled
held-view skinned indices used wholly single-influence streams. The existing
per-vertex shortcut still handles single-influence vertices in mixed streams.
An extra per-vertex selector for the whole-stream case was left out.

The opt-in `--algo-chicago-alpha-zero candidate` rejects fragments whose
final alpha is zero before ordinary source-alpha blending. Chicago masks
framebuffer alpha and depth writes. Authored cutout tests, the meter blend,
other blend modes and first-person stencil writers keep their original path.
The synthetic comparison enabled 41 additional alpha tests and matched all
color and raw D24S8 words across 16 cases, including filtered alpha edges,
rigid and skinned draws and same-frame stencil use.

Both scripted A30 runs rendered the dropship beam on the way to the held
camera and matched the full 400×240 buffers at tick 1305. They used disabled
audio for repeatability. At the held view the candidate tested 600 sky draws
per 120-frame window, with no eligible world draws. This counts eligible
draws, not rejected fragments or time saved. The paired fixed-resolution
launchers and an independent on-device image check are in
`build/performance-a30-chicago-alpha-zero`. Native results are pending.

The scripted A30 route now reaches the held dropship viewpoint through
ordinary movement. It rendered the tractor beam en route, reached the last
waypoint at tick 1185 and held until tick 1305. Both runs used the same
waypoints, fixed RNG seed and disabled audio. Neither used the fixture's
recovery warp. The final camera was within 0.28 world units of the logged
console position, with nearly identical aim. Both 400×240 captures matched
full RGBA and D24S8. These are comparable scenes, not identical console
and emulator simulation histories.

The independent `--probe-world-pixel-passes` launcher splits sky, opaque
terrain, models, late effects and coronas. The synthetic run passed all 11
phases. The live A30 fixture paused at the held view and completed all 132
timed queues with zero missing samples. Each phase recorded the same 133
main renderer draws and 88,452 indexed elements. Azahar's queue medians
were identical across phases and do not rank native raster costs. The
manual console launcher is in `build/performance-a30-world-pixel-passes/probe`.
All emulator processes exited. The native comparison remains pending.

The opt-in `--algo-chicago-neutral-draws candidate` omits a draw when its
quantized fade or complete one-map bake guarantees zero source alpha for
ordinary alpha blending, or zero source RGB for additive and subtractive
blending. Chicago masks depth and framebuffer alpha writes. First-person
stencil writers and stereo views retain their draws. The bake records a
bitwise summary with its pixels and updates it whenever a slot is reused.

The 400×240 Azahar fixture omitted seven of sixteen test draws and matched
full RGBA and D24S8 in all sixteen cases. It covered zero versus nonzero
fade, black and transparent colored texels, first-person stencil writes,
and cache hits. The scripted A30 opening matched both buffers at tick
301, with no omissions observed in its sampled windows. Native candidate
builds are under `build/performance-a30-chicago-neutral`.

A separate scripted route drew the authored A30 dropship tractor beam in
both builds and recorded no eligible omissions in the sampled windows.
Both runs baked 23,968 texels. The route stalled at its tenth terrain
waypoint before the Marines encounter. The final tick 1381 color and
depth/stencil buffers matched. This is evidence against prioritising
whole-draw neutral rejection for the sampled approach. It does not measure
native timing or cover the held Marines view. All emulator sessions exited.

The CPU material combiner now evaluates only products consumed by a
destination register or by that stage's selected sum/mux output. It reads
inputs before publishing any destinations and keeps the AB, CD and SUM
write order, component arithmetic and dot-product rules. The native A30
capture recorded 15,574 bake-cache misses across the route.

The linked ARM evaluator matched 4,848 reference/candidate outputs bit
for bit, including 376 authored generic packets found across the 24 prepared
maps and synthetic mux, dot and aliased-destination cases. The authored A30
tractor-beam packet used 24.1% fewer evaluator instructions. This count
covers the CPU combiner alone. Both Azahar A30 opening runs baked 4,816
texels and matched all 96,000 RGBA words and 96,000 depth/stencil words at
tick 301. The scripted run disabled sound for repeatability and ended
before the dropship encounter. Both emulator processes exited. Paired
fixed 400×240 console benchmarks are in `build/performance-generic-live-products`.
Native frame timing is pending.

The opt-in `--algo-chicago-map-uv candidate` changes the rigid and skinned
Chicago model programs. A Boolean map count selects which original UV dot
products execute. The transformed shaders export XY only, so they omit the
unused ZW writes and still initialize every exported lane. One-, two-, and
three-map draws execute four, two, and one fewer instructions respectively.
One- and two-map draws also set four and two fewer float uniform vectors.
The separate projective effect shader keeps its TEXCOORD0W export.

The assembled comparison changed only those two shaders and verified the
original arithmetic operands and output mapping. In Azahar, 16 paired
400×240 captures matched full RGBA and D24S8, including all three map
counts, same-frame transitions, a command-list split and first-person
stencil. Both native benchmark builds and the receipts are in
`build/performance-a30-chicago-map-uv`. `CHICAGO MAP PROFILE` reports draws
and indexed elements by map count. The scripted A30 opening matched color
and depth/stencil at tick 301, with sound disabled for repeatability. Both
runs recorded 8,908 one-map and 238 two-map draws over the two matching
120-frame gameplay windows. Indexed elements can reuse transformed vertices
and are not an instruction count. All emulator runs exited. Native timing
and the dropship view remain pending.

The four-map animated A30 sky could cancel every attempted midpoint
even with warm source textures. In a private Azahar run with the
New 2DS XL renderer gate enabled and the admission deadline extended,
43 rejected attempts reached the generic material-bake preflight.
The midpoint now has up to four 8×8 RGBA8 scratch textures allocated at
startup. A missing four-map sky bake uses the same material inputs and
pixel calculation as the ordinary path. Scratch stays outside the
regular bake cache and is reused after GPU acquisition. Missing sources
or exhausted slots still discard the optional frame.

The private A30 fixture captured a midpoint and keyframe at tick 295.
The comparison build baked the sky through the ordinary cache at the
same times. Camera records and all color and depth/stencil bytes matched
for both views. The midpoint and keyframe themselves differed, confirming
that the fixture captured two distinct views. The fixture disables audio
for repeatable effects and extends admission only for capture. The native
benchmark uses ordinary sound and deadlines. It is in
`build/performance-a30-midpoint-sky`.

An extended run reused the scratch across five acknowledged midpoints,
with no further texture rejections. In the last 30-tick window it
reported three midpoint acknowledgments and 27 busy skips. This run
still used the extended fixture deadline. The new acquisition counters
attribute all 27 skips to pending display acknowledgement, with zero
queue acquisition failures.

The opt-in swap-overlap candidate records a midpoint for the following
refresh when the previous swap is already posted and the regular-frame
budget still fits. Before submitting it requires display acknowledgement
and checks the deadline again. A controlled-clock fixture exercised
120,000 inputs for the ordinary and pending-swap paths. In the private
extended-deadline A30 test, acknowledged midpoints increased from 3 to 30
in the final 30 simulation ticks. Both builds acknowledged 30 regular
frames. Elapsed window time grew from 1.05 to 1.42 seconds. The capture
matched camera inputs, 400×240 RGBA and raw D24S8 for both frames at tick
295. The extended deadline, model gate and disabled audio are fixture
conditions. The production pair under `build/performance-a30-midpoint-swap-overlap`
uses the real deadline. Native delivery and input latency remain to be measured.

When the endpoint cameras match byte for byte, a single interpolation
now supplies culling and rasterization. Host differential tests matched
40,000 camera cases, including distinct views and disallowed transitions.
The new midpoint budget report classifies skip reasons and cooldown
triggers. A controlled-clock host fixture matched the old admission
and deadline in 120,000 cases. Console timing remains pending.

The `--algo-model-quaternion-direct candidate` path writes the lighting
quaternion directly in the shared rigid and skinned model programs.
It executes two fewer instructions for the normal case and one fewer
for the original degenerate-normal fallback. The uploaded shader size,
output mapping and arithmetic operands match the reference. Only the two
target programs changed in the matched build. Azahar matched full
400×240 color and depth/stencil in 32 model and 24 mixed-surface cases,
including exact and near-degenerate normals. Paired console builds are
in `build/performance-a30-model-quaternion`. Native GPU queue and display
timings remain pending.

The scripted A30 opening also matched full color and depth/stencil at
tick 301. That private comparison disabled sound for repeatable effects.
Two full campaign windows submitted about 130,000 indices per frame to
the affected programs. Index counts include reused vertices and cannot
be converted directly to executed shader instructions.

The opt-in `--probe-neutral-model-tint` comparison bypasses two PICA
combiner operations on lit, multipurpose model parts with white RGB tint.
The original stages interpolate between white and white, then multiply
the lit color by that white result. The candidate passes through the
lit color and retains the later buffer writes for reflection and detail.
Colored tint reinstalls its recipe on the next draw. Azahar runs
configured for Vulkan and software matched full 400×240 color and raw
depth/stencil across 18 pairs, including 144 bypassed parts, all detail-mask channels, alpha cutouts,
decals, first-person stencil and command-list splits. The fixed-resolution
probe alternates reference and candidate while holding the same paused
view. Both runs completed all five probe phases with 12 samples per
phase and bypasses confined to phases two and four. The binaries
are in `build/performance-a30-neutral-model-tint`. Physical timings are
pending.

The same scripted A30 executable ran with the candidate enabled and
disabled, using a private fixed-seed fixture with sound disabled for
repeatable effects. Both runs recorded 64,536 model parts. The candidate
bypassed 16,711 parts and matched all color and depth/stencil pixels at
tick 295. The draw count is an eligibility measure. The GPU cost depends
on coverage and how PICA handles passthrough stages.

Prepared views can now move frustum construction, portal visibility
and ordered object discovery ahead of GPU acquisition during ordinary
mono campaign frames. The old admission required a submitted midpoint.
The record is owned by the main thread and checked again against the
map generation, BSP, tick and previous leaf after acquisition. Texture
maintenance and GPU arena reuse retain their fences. Paused and debug
views use the ordinary late path. In the existing native A30 capture,
nearby object discovery averaged about 0.21 ms per frame across the
median profiling window, so this is a bounded CPU overlap opportunity.
Matched 30 Hz and 60 Hz pairs are in
`build/performance-a30-prepared-regular` and
`build/performance-a30-prepared-present60`.

The 30 Hz Azahar verifier matched the reference frusta, ordered clusters
and ordered objects in 295 admitted A30 views. A final frame with live
audio matched depth and stencil, while 16,409 of 96,000 color pixels
differed on the first candidate run. A second candidate run produced a
different color result. Disabling early preparation in the candidate
binary reproduced the reference byte for byte. Running early preparation
and drawing from the late results still changed color. Inserting a 1 ms
delay before acquisition with early preparation disabled also changed
color and effect triangle counts. Water geometry, textures, projection,
tint and wave inputs matched across the two instrumented runs. The sound
manager schedules work using elapsed time and draws from the local random
stream used by particles. Disabling sound in the private fixture before
seeding the campaign produced byte-identical color and depth/stencil at
tick 295 in the reference, candidate and 1 ms timing control, with
28,038 effect triangles across the route in each run.
The console builds use ordinary audio. The matched emulator frame
covers one point in the route and physical timings are pending.

The separate `--algo-model-view-pack candidate` shortens the shared
rigid and skinned reflect programs by one instruction. The assembled
output map continues to export the original XYZ view vector. The
fourth register lane carries no rasterizer semantic. Both lit and
reflective draws use these programs in A30. The paired builds retain
the previous shader and command options. All 56 model and mixed surface
cases matched color and depth/stencil, with seven discarded midpoint
submissions per build. The scripted A30 frame at tick 301 also matched
the prior water candidate. A full early gameplay queue window recorded
about 131,000 affected indices per frame. Native timing remains pending.
The paired builds are in `build/performance-a30-model-view-pack`.

The basic rigid and skin programs have an independent
`--algo-model-uv-pack candidate`. Texture units zero and one receive
the original UV products through separate lanes of one PICA output
register. Both programs lose three instructions and one output register.
The assembled regression test checks the libctru output semantics,
including the retained zero UV for unit two. All 32 Azahar cases matched
color and depth/stencil exactly across rigid, skinned, sky, cutout,
decal and cloak draws. The benchmark logs these programs' submitted
indices separately from the lit programs. Native timing remains pending.
The early scripted A30 run matched color and depth at tick 301 and
recorded zero basic shader indices in its full queue windows. This
candidate does not address that workload. Both builds are in
`build/performance-a30-model-uv-pack`.

The matched Boolean selector builds keep the UV and command suite
candidates enabled. Replacing the float comparisons with draw-level
Boolean uniforms reduces assembled model reflect and water programs by
two instructions each and the glass program by six. A third build
adds the water base branch. The multiply pass reaches the shader end
in eleven instructions, including the jump and output writes. The
reflection pass retains its existing calculation and pays for one
additional branch.

The three builds matched 32 model and 24 mixed surface captures byte
for byte in full-resolution color and depth/stencil on Azahar. The
surface fixture alternates waves, glass poses and material passes,
splits command lists, and rejects seven recorded midpoints per build.
The previous framebuffer remains intact on each rejection. The
benchmarks report recorded water and glass indices separately from
the model work. At tick 301 of the scripted A30 run, the combined
candidate matched the prior UV-only build's full color and depth/stencil.
The run exercised two-pass BSP water with GPU waves and logged 73,440
water indices and 23,040 glass indices in each 120-frame gameplay window.
Native timing remains pending. The A30 pair and water candidate are in
`build/performance-a30-boolean-selectors` and `build/performance-a30-water-base`.

The `--algo-uv-outputs candidate` benchmark removes one instruction
from each of the four rigid and skinned lit/reflect PICA programs. Only
those four generated shader binaries differ. The dedicated
lit programs were selected explicitly in the validation fixture, while
normal play uses the shared reflect programs for both lit and reflective
materials. Azahar matched full 400×240 RGBA8 and D24S8 buffers across 32
cases, including two-bone transforms, independent detail UV scales,
reflections, alpha test and decals. The paired console binaries use the
same 60 Hz presentation and graphics command suite. `MODEL UV WORK`
counts indices recorded on the affected paths per GPU queue window.
Index reuse and discarded midpoint work prevent converting that count
directly to executed instructions. Two early A30 gameplay windows each
recorded about 130,000 affected indices per frame in both Azahar runs.
The complete RGBA8 and D24S8 captures at tick 301 matched. The emulator
reported a New 3DS XL. Native New 2DS XL timing and midpoint behavior are
pending. The pair is in `build/performance-a30-uv-outputs`.

Each radar history slot now computes its yaw sine and cosine on its first
drawn contact and reuses them for subsequent contacts in that slot. A
2 KiB cache reuses the distance exponent result for identical positive
normal float inputs. The ARM fixture checked cold and repeated lookups
against the original `pow` expression. The scripted 120-frame A30 runs
produced identical packets for all 1,025 contacts, including positions,
sizes, colors and the final composite parameters. Those contacts shared
one position in this route, so the observed 1,024 hits and one miss do
not establish a hit rate for the full encounter. The 400×240 A30 and 60 Hz
builds are in `build/performance-radar-distance`.

The radar now precomputes ten contact history fade/radius pairs during
initialization. The original expressions and contact drawing order are
preserved. The ARM object no longer calls `pow` in
`motion_sensor_draw_screen`. Per-contact distance mapping still uses its
original arithmetic. Azahar matched the runtime ARM formula at every
history age and completed the scripted A30 smoke run with 120 radar
frames, 1,025 contacts and 107 contact draws. Updated A30 and 60 Hz builds are in
`build/performance-radar-history`.

The opt-in `--probe-radar-inline-clear` compares the radar's separate GX
fill with a zero-RGBA overwrite within the PICA draw list. The latter
uses immediate vertices and keeps the existing framebuffer flush and
invalidate on target change. It removes one command-list split and one
GX fill in exchange for a draw of the square radar target. In 24 Azahar
pairs at 16, 32 and 64 pixels, the radar and both LCD buffers matched
byte for byte, including target contents seeded with nonzero color and
alpha. The probe sampled all five phases and confirmed one fill per
reference and one in-list clear per candidate frame. The native queue
trade-off still needs measuring. The probe is in
`build/performance-radar-inline-clear/probe`.

The motion tracker now submits one draw for each adjacent run of contacts
using the same texture. The original contact order, per-contact vertices,
blend, sweep and mask are preserved. A native-size Azahar fixture matched
every byte of the offscreen radar and final top framebuffer across 24
pairs at 16, 32 and 64 pixels. The workload covered overlapping contacts,
empty frames, alternating textures, and runs of up to 128 contacts.
Contact submissions fell from 795 to 168 and the measured command span
fell from 44,224 to 21,636 words. Actual contact runs and frame-time
effects depend on gameplay. An isolated Azahar A30 campaign run submitted
1,025 contacts in 107 draws across 120 radar frames and exited normally.
Azahar reported a New 3DS XL and did not cover New 2DS XL midpoint timing.
Fixed 400×240 A30 and 60 Hz candidate builds are in
`build/performance-radar-contact-batch`.

The touch HUD batches the lower controls and Reload into one mono draw
and limits the lower strip to its populated atlas bounds. At native
320×240, this removes 2,112 transparent pixels from the lower strip's
raster area per draw. A full bottom-target fixture matched every RGBA8
pixel across eight atlas revisions, including the per-quad fallback.
The 400×240 A30 build is in `build/performance-touch-panel-crop/candidate`.

The separate `--probe-pixel-passes` launcher alternates the full paused
view with a one-pixel scissor on the 3D scene, later effects, top overlays,
and bottom LCD in turn. Geometry, clears, transfers and render order
remain in the queue. The queue deltas measure sensitivity to coverage
within each selected group, including effects on later draws that consume
the same depth or color buffers. They do not add up to independent pass
times. The Azahar fixture completed all nine phases with 12 queue samples
each. Its captures changed only the expected rectangle on the expected
target, including after a command-list split and offscreen target return.
The native probe is in `build/performance-a30-pixel-passes/probe`.

The matched `--a30-benchmark --present-60 --performance-profile` builds
in `build/performance-a30-present60-command-suite` compare the combined
vertex, TEV, effect, shader and uniform command reductions against the
stock command paths. Both request native 400×240 on New 2DS XL. Azahar
matched all 16 synthetic RGBA8 captures byte for byte. Each case rejected
a recorded midpoint before the next visible frame, with command-list
splits in eight cases. The candidate exercised all five paths, including
96 skipped uniform vectors. Both emulator sessions exited. Native frame
pacing and the cost of the combined paths remain to be measured.

The opt-in `--compact-uniform-uploads` A30 build skips whole runs of
vertex uniforms that match the current command list bit for bit. It scans
four clean dirty flags at a time. Float24 constants keep their SDK
uploads and invalidate the affected shadow entries. Integer and Boolean
values, restore flags, and geometry shader uniforms follow SDK behavior.
Frame acquisition, list splits and rewinds discard the shadow.

The linked ARM fixture matched uniform RAM and restore flags across
6,000 mixed sequences plus dense and boundary cases. A repeated matrix
used zero command words instead of 20 and 569 instructions instead of
933. The mixed fixture used 8.1% fewer words and 16.5% fewer instructions.
Azahar matched all 16 captures byte for byte while skipping 192 vector
uploads. Both emulator sessions exited. The fixed 400×240 builds are in
`build/performance-a30-uniform-uploads`. Native scene hit rates and frame
times remain pending.

The opt-in `--compact-shader-uploads` A30 build retains the vertex code
already uploaded in the current command list. Chicago draws restore the
world program as pending state. When the next draw selects Chicago again
before any world draw, Citro3D can request a duplicate upload. The wrapper
skips only that upload and still runs the SDK's entry-point and output-map
configuration. Geometry shader configurations take the stock path. The
shadow is invalidated on frame begin, command-list splits and rewinds.

The linked ARM fixture matched code RAM, operand descriptors and other
configuration writes across 6,000 bind sequences. One repeated Chicago
transition used 52 command words instead of 82. The mixed fixture used
4,347 uploads instead of 5,166, with an instruction reduction of about
0.35% across context updates. Azahar matched all 16 captures while
skipping 48 uploads. Both sessions exited cleanly. Physical A30 timing
and the actual upload hit rate are pending. The matched benchmarks are in
`build/performance-a30-shader-uploads`.

The opt-in `--probe-texture-rebinds` build checks whether a pending Citro3D
texture bind matches the immutable 2D texture and sampler already recorded
in the current command list. It drops only matching dirty bits. The SDK
still handles changed descriptors, explicit texture status, framebuffer
transitions and any draw with a mutable or unrecognised bound texture.
When all pending bits match, the stock texture-cache clear is also
unnecessary. The probe reports eligible rebinds, skipped rebinds and skipped
clears in five alternating phases.

In 12,000 linked ARM updates, the final texture registers matched. The
synthetic stream went from 262,968 to 96,666 command words and from 11,568
to 5,847 cache-clear commands. Azahar matched all 16 image comparisons
while actually skipping 16 clears. The emulator exited cleanly. The
actual rebind frequency and queue timing on PICA remain to be measured.
The probe and instructions are in `build/performance-a30-texture-rebinds`.

The opt-in `--compact-effect-commands` A30 build reduces Citro3D's
depth, stencil, cull and blend state packets. It writes the complete
34-word SDK packet at a list boundary and skips ordinary register
groups that already match the recorded state. Gas, shadow and early-depth
writes keep the stock order. The linked ARM comparison matched all
18 affected registers in 12,000 context updates, including masked fields,
stencil changes, command-list splits and rewinds. The fixture used
210,922 command words versus 408,000 and 18.33 million instructions
versus 25.82 million. An Azahar/Vulkan fixture elided 1,680 words and
matched all 16 RGBA8 captures byte for byte. Both emulator sessions
exited. Both fixed 400×240 benchmarks are in
`build/performance-a30-effect-commands`. Console queue and frame timings
are pending.

The opt-in `--compact-tev-commands` A30 build tracks the five register words
per Citro3D combiner stage within the current command list. It skips an
identical stage and writes the SDK's six-word packet for a new stage. List
splits, rewinds and frame acquisition invalidate the shadow. A linked ARM
fixture ran 12,000 mixed context updates and checked all 30 stage registers
after each one. The mixed workload went from 325,218 to 129,918 combiner
command words and from 22.67 to 20.96 million ARM instructions in context
updates. An Azahar/Vulkan fixture that forced repeated stage setup elided
288 packets and matched all pixels across 16 reference and candidate
captures. Both emulator sessions exited. Actual scene hit rates, PICA
timing and frame pacing need a console comparison. Both fixed 400×240
benchmarks are in `build/performance-a30-tev-commands`.

The paused GPU probes now report the mean time from GX submission to the
last P3D command-list interrupt and from that interrupt to queue completion.
The second interval can include outstanding transfers, clears and callback
latency. The first can include work that overlaps the command lists. The
split is available only when the observed P3D count matches the number of
submitted command lists before the queue callback. The total queue samples
remain available when the split is missing. libctru invokes its queue
handler before the event callback, so a P3D that ends a queue arrives too
late for that split. Cumulative counts preserve attribution if the next
submission starts before that callback. [Pinned libctru event dispatch](https://github.com/devkitPro/libctru/blob/36fe1ada5b7ebe53ba4decda36d764a55f8fefb6/libctru/source/services/gspgpu.c#L374-L391)

The host fixture passed delayed final interrupts, multiple command lists,
transfer-only queues, clock wrap and unavailable samples. The Azahar/Vulkan
fixture completed 64 submissions, attributed 48 to P3D and passed the
sixteen alpha image comparisons. Console timing is pending.

An earlier paused capture at tick 1222 had a normal queue median around
79.8 ms, a 28.1 ms quarter-pixel median and a 79.3 ms median with the
particle draw path skipped. That establishes a strong sensitivity to pixel
coverage for that view. The draw and transfer split can show whether the
later dropship view spends a similar share of its queue before the final
command list finishes. The probe builds are in
`build/performance-a30-queue-breakdown`.

Synchronized frame acquisition now waits for VBlank once, then retries the
nonblocking GX queue acquisition. Citro3D's SYNCDRAW flag waits before it
checks queue readiness, so retrying with the flag could wait through an
extra refresh even after GPU work had finished. The mission's regular
frame already bypasses VBlank synchronization. Its held A30 capture at
time 1376 spent roughly 38.8 ms waiting for the queue, with around 0.08 ms
of other frame preparation. That wait reflects work in the previous
submission. The synchronized-path fix does not account for it.

A host timing fixture ran 40 cases against the extracted acquisition
loops, including pending and retired queues, midpoint rejection and the
queue watchdog. With a modelled 39 ms queue, synchronized acquisition
changed from 50.142 ms with three VBlank waits to 39.214 ms with one.
Console timing is pending. Builds and the fixture are under
`build/performance-frame-sync-once`.

The A30 capture includes a held view at mission time 1376 with simulation
around 0.06 ms and GPU queues averaging 38.9–39.5 ms across successive
120-frame windows. It is a useful view for isolating rendering cost. The
first tractor-beam report is earlier, and the held view has large effects
and active coronas, so the queue timings cannot be assigned to the beam.

The independent `--probe-effect-alpha-zero` build alternates standard
alpha-blended particle draws with and without a PICA alpha test of greater
than zero. The source and destination RGB factors make a final alpha of
zero an identity operation. This draw path masks alpha and depth writes
and keeps stencil. Other blend functions and separate transparent paths
are outside the experiment. The probe reports eligible and tested draws,
keeps the same geometry, and times five phases of the same paused view.
The separate validation launcher compares all 96,000 RGBA8 pixels for
sixteen synthetic cases with nearest and linear sampling, float and
indexed packed vertices, depth testing on and off, and world and weapon
pass setup. The fixture uses cleared depth and stencil and does not measure
their raw contents. All sixteen comparisons passed on Azahar/Vulkan, and
the emulator exited. The probe and validation binaries are under
`build/performance-a30-effect-alpha-zero`. Hardware image comparisons and
queue timings are pending.

The newer SD capture at `build/sd-logs/20261108T153539` is the fixed 400×240
A30 launcher at 804 MHz with L2 enabled, original 30 Hz pacing and the 64-pixel
texture cap selected for that map load. It reports 609 slow map reads. The
maximum map-lock wait was 0.004 ms. Among the reported reads, the median I/O
time was 33.84 ms during four checkpoint write windows and 17.48 ms outside
them. The slow-read logger has an 8 ms threshold, and save windows coincide
with changing gameplay, so this is an association. Seventeen of 68 logged
frame hitches fell in those windows. The 4 MiB gameplay cache logged 26 demand
evictions and 792 maintenance evictions, with zero admission allocation
failures or headroom pressure. The lowest sampled free linear memory was
11.10 MiB. GPU queue windows after the first tractor-beam report reached
27.59–39.51 ms on average in the moving scene.

The checkpoint worker now reuses a single optional 16 KiB stdio buffer across
the temporary save and validation of the prior save. It preserves the encoded
bytes, CRC validation, backup rotation and the fallback to default buffering.
The production file adapter passed host sanitizers and the linked ARM newlib
fixture with identical bytes, header backpatch, EOF and flush failure. Across
105 mock blocks the ARM fixture reduced backing writes from 177 to 26 and
reads from 240 to 25. SD service timings and concurrent stalls remain to be
measured on hardware. The matched fixed 400×240 profile builds are in
`build/performance-a30-memory-grant/cache4` (reference) and
`build/performance-a30-checkpoint-buffer/cache4` (candidate).

The earlier A30 capture reports a 124 MiB application region and about 20 MiB
of free linear memory after map load. The reported zero uncommitted region
bytes do not measure free space within either allocated heap. We have no
linear free-space sample at the largest hitch. The older benchmark links a
libctru allocator that caps the default linear arena at 32 MiB and puts the
rest in the ordinary heap. A Dev1 launch can then request more than the
kernel's 96 MiB ordinary-heap mapping limit before Halo starts. The dedicated
Dev1 benchmark uses the upstream heap-clamp fix, which redirects the excess
to the automatically sized linear heap. The gameplay texture cache retains
its own 4 MiB default budget and 256-slot limit under both grants.
[libctru heap-clamp fix](https://github.com/devkitPro/libctru/commit/5290d64375002802f3e5ea8bfc31ea33ec64456d)

At the held dropship view (simulation tick 1376), the same native capture
recorded 38.91 ms average GPU queue time and 14.33 ms average CPU preparation.
The cache held 3.75 MiB across 190 slots with zero demand or maintenance
evictions and zero allocation failures across the repeated held samples.
The top screen refresh interval was about 16.7 ms. A larger application
grant could help with assets or checkpoint allocations elsewhere in the
route. The held view already had a warm working set.

Two fixed 400×240 A30 builds in `build/performance-a30-memory-grant` vary only
the configured texture cache budget (4 or 8 MiB). Both log the application
region, process commit limit and heap capacities at startup. Compare the two
under the normal grant to measure the cache budget effect. To measure the
grant effect, use `build/performance-a30-dev1/cache4` for both grants. Match
the route, settings and CPU clock, then compare texture churn, read stalls,
checkpoint overlap and render time. The build and mocked startup allocator
passed, and a scripted Azahar A30 run passed with the normal process commit
limit. A Dev1 console boot and paired timing are pending.

Repeated native lighting draws now skip the ambient, distant and backlight
component checks when their copied input bytes and translucency match. The
renderer still binds the Citro3D environment, so switches and frame resets
restore the same dirty state. A linked ARM fixture kept the same SDK state
and PICA command words across 12,000 mixed draws. In the isolated fixture,
an identical-input bind fell from 321 to 235 instructions. A translucency
change rose from 461 to 477. The regular native build passed. The earlier
A30 capture counted 614,384 lit draws, but the new key's hit rate and
frame-time effect still need a physical run.

Native model lighting now keys its conversion cache on the 64-byte ambient
and distant-light prefix rather than the entire 116-byte lighting packet.
Reflection tint, point lights and shadow data still reach their own
consumers. The linked ARM comparison matched all output bytes and floating
point control bits across 12,000 mixed packets. It counted 369 to 226
instructions for an identical packet and 766 to 226 for a reflection-only
change. Synthetic exceptional float inputs can leave different cumulative
exception flags when the candidate skips a repeated conversion. Native
rendering does not consume those flags. Physical timing is pending.

The New 2DS XL A30 run completed all five texture-unit probe phases with
matching draw and element counts. The two candidate comparisons changed
median queue time by −0.04% and +0.24% relative to adjacent reference phases.
That provides no consistent benefit for this view. The camera and workload
differ from the earlier ~80 ms run, so the ~12 ms queue result cannot be
attributed to a change between builds.

The two SD launcher hashes match the integrated release and the frozen
corona-native probe receipts. Compared with that release, the probe skips
unused generic-shader vertex calculations, Xbox-only flare bookkeeping and
debug-only sprite coverage. It reuses the starting-cluster PVS during portal
traversal, specializes fully tied AI firing-position sorts and avoids allocation for
ordinary format strings. The probe also fixes the world at 400×240. Both
launchers use the original 30 Hz pacing and the same legacy texture profile in
the collected logs. The run does not isolate the individual CPU gains.

The logged Covenant dropship tractor-beam material first appears between
simulation samples 1770 and 1800. In surrounding windows, CPU render
preparation rose from 7.29 to 18.45 ms and GPU queue time from 5.59 to
14.19 ms. Vehicle update time increased from roughly 0.3 ms with one vehicle
to roughly 2–2.6 ms with three. The nearby 300-frame work average rose from
18.22 to 28.53 ms. These windows cover the wider encounter and moving camera,
so they do not measure the cost of the beam alone. The latter window logged
no hitches above 100 ms and occurred after the preceding checkpoint save
completed.

Before the paused probe, a sampled tick had 23 live bipeds and spent 5.07 ms
on node computation, up from 9 bipeds and 2.48 ms earlier. The late AI average
reached 8.22 ms over 30 updates. Fifteen of 17 logged hitches above 100 ms
after the initial map load fell between checkpoint capture and its background
completion record. The final checkpoint spent 30.44 ms capturing RAM and
11.12 ms preparing persistence on the game thread. The 14.27 s completion
time was elapsed background time. The largest hitch spent 1.51 s in rendering.
The shared file lock and SD traffic are plausible sources of the long hitches.
Their contribution needs per-read measurements on the console.

The private map stream now uses its own lock for publication, seek/read and
close. It no longer waits on the application's save-file lock. Both still
share the filesystem service and SD card. A host fixture passed 2,400 concurrent
seek/read checks with the save lock held, including invalid ranges and reopen
after a short read. The pinned libctru archive driver calls FSFILE_Read and
FSFILE_Write for separate file handles. Console service contention remains
unmeasured. Matched profile builds are in
`build/performance-a30-map-lock-reference/profile` and
`build/performance-a30-map-lock-candidate/profile`.

The background logger now requests a flush at the end of each bounded batch
instead of on every 4 KiB write. A mocked 18 KiB burst used one flush request
across five writes, with the same output order. New arrivals join the next
batch. Abrupt termination can leave up to one batch of routine log data
unflushed. The separate fatal log and the orderly shutdown flush remain in
place. The native build and host fixture passed. Physical SD timing is pending.

Performance builds now log slow map reads with file-lock wait and stream I/O
time separately. `build/performance-a30-encounter` contains a matched
instrumented pair that differs only in local hierarchy reuse. The ARM fixture
passes exact matrix, cache lifetime and fallback checks. Console timing for
this pair is pending. The default native source also skips a duplicate root
matrix validation after a successful check. Build and ARM disassembly confirm
the success branch goes directly to traversal, while failure diagnostics still
recheck the matrix. This change is in `build/performance-a30-root-validation`.

Native CRC now processes eight bytes per iteration, with independent lookup
chains for the two input words. This keeps checkpoint and map checksums exact
and adds 4 KiB of lookup tables. The compiled ARM implementation passed the
existing CRC fixture and 392 independent comparisons covering alignment,
initial CRC values, tails and split updates. An aligned 32 KiB checksum used
151,581 instructions versus 155,670 in the previous build. Checkpoint latency
still needs console timing, which captures cache and memory effects.

Native actor perception now enumerates set bits in the potentially visible
cluster mask. It skips empty words and visits the same objects in the same
order, including collideable objects before noncollideable objects in each
cluster. The native build and 20,520 ordered traversal comparisons passed.
The benefit depends on mask density and still needs active encounter timing.

Native path searches reuse ordered BSP edge geometry for surfaces with up to
eight edges. The 8.5 KiB cache keys entries by BSP identity, generation and
surface index. It rereads pathfinding flags on a hit and still checks whether
surfaces are broken during each search. Larger polygons and unsupported
neighbors take the original builder. Both native builds and the host fixture
passed. The profile build reports hits and builds so the next console capture
can establish how often the cache helps at A30.

Each path traversal now resolves its collision BSP before entering the
search loop and passes it to the edge builder, breakage check and point
projection. Surface index checks still run. The native build and edge fixture
passed. ARM disassembly shows four fewer tag accessor fallback call sites in
the inlined search. This does not measure the savings on a valid map.

The optional 60 Hz presentation path reuses the previous camera cluster
under the same BSP generation and floating-point mode. Identical camera
positions share a classification. Camera and cluster transitions keep their
existing rejection rules. A host fixture with a synthetic BSP classifier
matched 12,003 midpoint decisions and output windows. The native profile
build passed in `build/performance-a30-present60/profile`. It holds the
New 2DS XL world at 400×240. Sustained A30 midpoint delivery and image
quality still need console verification.

The `vfp_schedule` candidate now runs three independent scalar accumulators
per row and groups matrix loads and stores. The compiled kernel uses 61 ARM
instructions per call, down from 76. In an ARM CPU fixture, 16,384 cases per
mode matched output bits and FPSCR across in-place aliases, special floats
and rounding modes. The native reference, candidate and verify builds passed.
The scheduler remains opt-in pending physical cycle and encounter timing.

The same VFP mode now covers point and vector transforms. The point kernel
uses 23 instructions instead of 25 and the vector kernel uses 20 instead of
22. Grouped stores replace three scalar stores, and the vector routine loads
only the matrix fields it uses. The compiled ARM fixture matched 16,384 cases
per transform and mode, including aliases, return pointers and FPSCR. ARM's
VFP11 timing table lists longer result latency than issue throughput for
scalar arithmetic, which motivates interleaving. Physical cycle measurements
remain pending. [ARM VFP11 manual, table 4-17](https://documentation-service.arm.com/static/5e8e227c88295d1e18d377ac)

Grounded biped turns now reuse the exact sine and cosine of authored turn
speeds in a 192-byte cache. Unusual speeds and nonstandard floating-point
modes use the original calculation. The native build and 60,020 host
comparisons passed. ARM disassembly confirms that cache hits skip the
division and both libm calls. Encounter timing is pending.

Dynamic geometry reset now relies on its existing slot-count boundary and
generation checks instead of clearing each slot's redundant live flag. Native
build and host sanitizer checks of the existing dynamic-geometry fixture pass.
ARM comparison confirms one byte store removed per used slot; lock assertions
remain active in this native translation unit, so the loops are not eliminated.
No native frame-time improvement is claimed from this change.

Native flare submission now omits Xbox occlusion-identifier remapping and
history updates. Both original history readers are bypassed by native query/draw
entrypoints; native coronas consume none of those fields. Admission, packet copy,
order and statistics remain unchanged, and other platforms retain the code.
The native build passes (`build/performance-corona-history`); physical timing
and image validation remain pending. Occlusion-only index diagnostics no longer
run on native submission.

Native corona follow-up: compressed light direction is decoded only when the
angular-falloff branch uses it. The decoder is pure arithmetic; camera-vector
normalization remains because surface lift also consumes it. Native build passed
in `build/performance-corona-direction`. No physical timing improvement is yet
established. The existing core-only reflection policy, radius/draw/area limits,
and omission of the authored radius-scale function remain fidelity limitations.
ARM assembly comparison confirms the old decoder call preceded the range test;
the new range comparison branches past the call when angular falloff is inactive.
This verifies removed executed work on that path, not a measured frame-time gain.

### Native effect reachability audit

Source comparison and the `performance-weather-fade` link map establish the
following scope. This describes existing port behavior, not changes authorized
as performance tradeoffs. Preserving the current port and matching the original
renderer are distinct validation targets; neither has a completed native image
comparison for the current optimization set.

| Original stage | Current native evidence | Consequence |
| --- | --- | --- |
| Ordinary particles, particle systems, contrails | Direct calls in `engine_mission_render.c`, followed by the native transparent queue | Active optimization targets; keep submission order and midpoint guards |
| Weather particles | No native mission submission; original function discarded by linker | Existing missing visual feature; source-only weather optimizations cannot improve native FPS |
| BSP detail objects | No native mission submission found; original `structure_render_detail_objects` discarded | Existing missing stage, separate from model LOD choices |
| Layered BSP decals | Original mission loop explicitly submits several layers; native mission loop has no equivalent calls; original wrapper discarded | Decal storage and model decal materials do not establish BSP decal rendering |
| Lens flares | Native mission calls `rasterizer_lens_flares_draw` after transparency | Reachable; availability alone does not prove original parity |
| Scripted screen fade | Native `n3ds_cinematic_draw_screen` calls `player_effect_get_screen_flash(NONE, ...)` and draws a quad | Original screen-flash wrapper being discarded is not evidence this replacement is missing; player damage flashes need separate tracing |
| Screen fog | Original stage discarded; no equivalent mission submission identified | Existing gap; material-local fog calculations do not prove screen-fog parity |

The map is evidence for that specific launcher, not a substitute for checking
future build flags and replacement paths. No additional visual feature was
disabled by this audit.

Weather reachability follow-up: the native mission effect submission calls
ordinary particles, particle systems and contrails, but omits weather. The
latest link map places `.text.weather_particle_systems_render` in discarded
input sections. The proposed fade-calculation relocation was therefore removed:
it cannot improve native performance. The successful build remains historical
compile evidence, not evidence of an active optimization. Weather omission is
an existing visual-fidelity gap. Restoring it must also address render-coupled
random acceleration changes and fixed position jitter, which run even at zero
frame delta; simply invoking it on midpoint or stereo views changes behavior.

Review started from clean revision `3292af4` on 7 November 2026. This is a
coverage and evidence record, not a claim that every
source line has been reviewed or that native 60 FPS has been achieved.

## Findings that set priorities

The retained physical New 2DS XL captures report 804 MHz with L2 enabled.
The stationary A30 crop probe measured about 79.8 ms per normal GPU queue and
28.1 ms with quarter-pixel coverage at identical draw/element counts. A later
scene measured about 105.6 ms; skipping redundant framebuffer binds saved
only about 0.067 ms. Queue duration includes transfers and callback latency,
so it is not itself a scanout measurement. These results strongly prioritise
pixel work and bandwidth over CPU-only micro-optimisations for that workload.
See [the framebuffer experiment](FRAMEBUFFER-REUSE.md) and
[opaque reads](OPAQUE-COLOR-READS.md).

The display target is approximately 59.83 Hz (16.71 ms). Authoritative Halo
simulation remains 30 Hz. Current normal builds are paced at 30 Hz, and the
optional presentation path admits interpolated frames only on New 2DS XL
in eligible gameplay. Other model families and several modes remain outside
that path. Fast regular rendering alone cannot establish 60 distinct images
per second across all models and modes.

Original hardware already uses different resolution, framebuffer, material,
LOD and foliage policies. The New-model fixed-400 control does not remove
those Original-model policies. Further quality reductions do not satisfy this
goal. Each hardware comparison needs an explicit preserved baseline.

Older hardware build receipts differ from the integrated source in dozens
of translation units. They establish historical bottlenecks, not validation
of a new executable. New reference/candidate builds and exact source receipts
are retained privately under `build/performance-goal`.

## Review coverage and next work

The saved `build/performance-goal/review-coverage.json` tracks hash-backed
review scopes. The current snapshot inventories 999 code files and 458,523
lines, excluding build outputs, dependencies, documentation and configuration.
Retained receipts establish whole-file scopes for 206 files and partial scopes
for 78 files, covering 49,535 lines. Another 715 files lack a matching scope
in those receipts. The accounting requires a matching source hash and precise
line scope. The complete code review and runtime validation remain open.

The additional `ai-actions-review.json` covers complete alert, guard and hunter
action files. Guard's expensive position selection is already conditionally
timesliced. Alert's already-rejected positions still scan props, a small
possible exact CPU saving without measured relevance to the GPU bottleneck.
No AI scheduling, selection ordering or random-call timing was changed.

`shaders-review.json` records the complete 21-program PICA source review,
both variant generators, three assembler test files and relevant model input
and palette-limit ranges. Eight assembler checks passed. UV write combination,
Boolean selectors, weighted MAD, water base bypass and packed influence inputs
already exist as separate candidates. Their presence is not a native speedup
receipt. Chicago's smaller 28-bone palette is correctly guarded independently
of the general 29-bone limit. Physical output/timing evidence remains required
before candidate promotion.

The Chicago and GPU bridge reviews add full native material, allocator and
adapter scopes. Hologram draws now install the existing identical Chicago
vertex layout instead of rebuilding its five loaders every draw. The pinned
SDK copies the complete layout and marks the same attribute-state dirty flag;
the change does not skip state installation after other programs. The native
400×240 build passed, with frozen source and receipts under
`build/performance-hologram-layout`. This establishes a removed CPU setup step,
not a measured frame-time improvement. Index bounds scans remain because
index staging does not provide equivalent vertex-range validation.

The transparent/plasma review found that alternating plasma materials repeated
the material-name tag scan and log enqueue on every switch. Reporting now uses
a fixed 32-entry per-map set without eviction, preserving each admitted
material's first diagnostic and suppressing later repeats. A sanitised check
of the production helper covers 10,000 alternating calls, capacity and map
reset; the native-400 build passed. Receipts are under
`build/performance-plasma-report`. This removes diagnostic work, not shield
sampling or rendering. The inherited plasma volume-mip and vertex-sampling
approximation, and disabled effect depth shaping, remain baseline policies.

Water/glass and four additional AI action files also have complete review
receipts. Candidate fallback sample/index reuse needs separate validation;
ignored collision-query outputs and timer conditions cannot be removed without
checking their side effects. None of these source findings is a measured GPU
speedup.

| Area | Reviewed scope | Result / remaining work |
| --- | --- | --- |
| GPU | Native terrain/material recipes, sampler bindings, framebuffer adapter, pinned Citro3D context update and queue/state boundaries | Implemented independent [unused terrain samplers](UNUSED-TEXTURE-UNITS.md) candidate. Needs physical parity/timing. Further texture residency and per-material attribution remain open. |
| CPU | Selected animation, hierarchy, collision, matrix, audio cache/codec, worker, loading, AI perception and spatial partition paths | Many obvious reuse/parallelism candidates already exist. Repeated sound victim scans and speculative BSP/demand I/O contention remain measurable opportunities. Neither explains paused GPU queues. |
| Build/platform | Build selectors, dependencies, identities, model checks, speed/L2 reporting, pacing and midpoint eligibility | Added independent candidate controls and corrected probe frame-adapter receipt metadata. Broader model/mode presentation remains unfinished. |
| Tests/setup | Existing test entry points and build-control checks | Existing setup conversion was not changed. A complete setup/UI/network/AI/gameplay source audit remains outstanding. |

The next physical comparison uses the same stationary slow view, native
resolution and all materials, alternating sampler reference/candidate phases.
Do not promote the candidate based on host tests or emulator timing. Retain
the default renderer until correctness and a repeatable frame benefit pass.
Continue the broader code review and presentation work independently of that
hardware measurement; do not label unreviewed subsystems complete.

The AI review also found an inherited native decision/action throttle for
quiet distant actors in `source/ai/actors.c`; perception and movement remain
on their existing paths. Preserving the current simulation therefore means
preserving this fork's actual baseline, not asserting unchanged Xbox decision
cadence. No new simulation throttling was introduced. Likewise, replacing
perception sorting or skipping spatial reconnects requires care: tie ordering
and linked-list order can affect later gameplay and RNG consumption.

The 8 November continuation adds an independent
[exact VRAM texture mirror experiment](TEXTURE-VRAM-MIRRORS.md), targeting
the memory-location part of pixel cost while retaining CPU-readable originals.
The private `cpu-review.json` inventory covers 114 files, with 29 explicitly
inspected ranges/files; `remaining-review.json` records a further 22 native
service files and four caller/macro ranges. Neither inventory labels static
screening as a complete manual review. That pass also found optional thread
allocation and profile-write failure handling issues; these remain findings,
not implemented FPS gains.

The memory/math pass is recorded in `memory-math-review.json`, distinguishing
linked native replacements from unused upstream code. Its strongest new
allocation candidate was the CRT format adapter, which allocated and
copied even formats requiring no Windows length-modifier translation. The
adapter now borrows ordinary formats for the duration of the libc call and
retains the original conversion/allocation path for `I64` and `I32`. The
sanitised host fixture (`python3 tools/verify_crt_formats.py`) checks 50,000
parser cases against the unchanged converter, output/return/truncation and
stream parity, varargs, `%n`, long formats and allocation failure. A 10,000-call
ordinary-format workload performs zero adapter allocations; this does not
count libc's own allocations or establish a native frame-time improvement.
The call-site audit found no supported dependence on a copied format's
lifetime or aliasing. Visual, simulation and material policies are unchanged.
The fixed-400 native build passed with GPU experiments disabled; its frozen
source, build receipt and binary are in `build/performance-crt-formats`.
Complete memory-pool/LRU verification scans are another possible CPU cost, but removing
validation indiscriminately is not justified. These remain review findings.

The encoded-sound-cache review (`sound-review.json`) confirms that warm hits
avoid victim scans. Under budget pressure, admission plus `k` evictions visits
512 × (1 + k) slots. A local index heap could preserve unsigned-age ordering,
slot ties and allocation-attempt boundaries, but its construction may cost more
for a single victim. No measured eviction distribution establishes a frame-time
benefit, so this is not promoted ahead of the pixel-work bottleneck.

Early depth rejection requires a separate hardware characterisation. The
pinned Citro3D initial state disables it. The [Azahar early-depth issue](https://github.com/azahar-emu/azahar/issues/551)
was still open when checked on 8 November 2026; its original author reports
unexpected behaviour in physical-console experiments. That report is not a
specification, and emulator parity cannot establish this feature's hardware
semantics. Any experiment must check cutouts, equal-depth replacement, depth
writes and render-target transitions before drawing a quality conclusion.
The scoped source inventory is in `depth-review.json`. Terrain already draws
before world models, but its batches retain authored order rather than a
general front-to-back sort. Arbitrary opaque sorting is not established as
exact: `GEQUAL` lets later fragments replace earlier ones at equal quantised
depth. The next early-depth step is a standalone native quad fixture covering
ordering/equality, cutouts, stencil, target rebinds and command-list splits,
with raw colour/depth equality checked before any timing claim.
That first diagnostic is now implemented in
[`tests/early_depth`](../port/n3ds/tests/early_depth/README.md), using separate
private build overlays rather than enabling early-depth in gameplay. Ten
cases compare disabled reference/repeat and two zero-reference enabled modes.
It checks emitted register state, nonempty reference geometry, cutout holes,
depth-write effects and stencil side effects. The reference-only emulator
control checks the harness; enabled physical-console results remain missing.
Other reference values, weapon-projection transitions and a conservative
production invariant are still outstanding.

The presentation review (`pacing-review.json`) identifies two independent
New-2DS-XL-only gates: mission midpoint admission and GPU swap tracking. A
future New 3DS/XL mono extension must address both and verify each model's
latch, memory and timing behaviour. Stereo cannot use that simple extension:
the midpoint currently renders with a zero slider and needs paired-eye
rendering/retirement. Menus, paused views, cinematics and video also need
separate presentation handling. Raising the existing 30 Hz loop cap would
advance fixed-step simulation too quickly; it is not a valid 60 FPS fix.

The texture-maintenance review found no unconditional whole-cache upload or
flush in the inspected warm paused path. Archive-generation guards avoid
reopening unchanged data; eviction fences require pressure and an eligible
victim. Dynamic-bitmap collection returns immediately without retired objects.
Its nonempty retirement barrier bypasses the profiled `texture_fence()` wrapper,
however: that wait appears in maintenance time and `idle_ticks[0]`, not
`texture_profile.fence_ticks`. A zero fence counter does not rule out all
texture-related waits. This distinction matters when attributing physical
frame time; it is not itself an implemented performance gain.

The particle review also identifies inherited quality budgets on both model
families: detached cosmetic lifetime counts (48 Original / 144 New), detached
draw counts (64 / 192), dense-group thinning and overlapping-sprite coverage
limits. These exist in the starting native fork; this work does not tighten
them. Preserving this fork's current image is therefore a different claim from
reproducing every Xbox particle. Any future baseline comparison must name the
actual policies, rather than silently treating the native fork as unrestricted.

The water CPU fallback now omits per-vertex base-texture sampling in the
reflection pass when the base-alpha mask is disabled. The sampler has no
resource side effects, and that pass otherwise never consumes its result.
Texture admission, the base pass, masked reflections and blend ordering remain
unchanged. This only benefits fallback draws; no physical frame-time gain is
established. The fixed-400 build receipt is in
`build/performance-water-sampling`; the scoped proof is recorded in
`build/performance-goal/water-sampling-review.json`.

The glass CPU fallback now stages its triangle indices once at the first
active layer and reuses the immutable frame-arena range across later layers.
Previously, CPU-sourced indices could be copied three times. All layer vertices,
material state and draw ordering remain separate. The native glass path already
staged once. An ASan/UBSan check using the actual arena functions passed all
eight layer masks, interleaved allocations and failure boundaries; it checks
staging rather than rendered pixels. The fixed-400 native build passed.
Receipts are in `build/performance-glass-indices`; physical timing remains
unmeasured.

The object-render review removed two unused native lighting argument
calculations. Native lighting already ignores projected object size; model
selection and shadow size still compute their original detail values. ARM
disassembly confirms that two of four static detail-call sites disappeared.
The fixed-400 native build passed; this is evidence of removed CPU work, not
a measured frame-time improvement. Receipts are in
`build/performance-lighting-lod` and `render-objects-review.json`.

The whole structure-visibility review confirms native rendering already skips
the legacy per-surface list. Remaining candidates include caching portal
far-distance tests and resolving the starting-cluster PVS once per traversal.
Portal revisits cannot simply be pruned: different incoming hulls can expose
additional geometry. Downstream exact invisible-cluster triangle filtering
already exists. No new GPU-work reduction is established by this review.
The receipt is `build/performance-goal/structure-visibility-review.json`.

The traversal-local PVS candidate is now implemented: the starting cluster's
bitset is resolved once and passed unchanged through all recursive portal
paths. ARM disassembly confirms the recursive function no longer calls the
lookup. Portal predicates, clip bounds and visit ordering are unchanged.
The fixed-400 native build passed; receipts are in
`build/performance-visibility-pvs`. This removes repeated CPU work but has no
measured native frame-time result.

The terrain batch follow-up confirms filtered geometry is excluded from
full-list batching. Lowering the existing filtering threshold could save
indices while losing merged draws and adding per-eye memory traffic, so it
requires combined CPU/GPU timing. The portal-distance cache also remains a
candidate: the reference distance test often needs only its first qualifying
vertex, while a cache adds lookup/storage cost. Neither change is promoted
without workload evidence. The full batch review and precise source scopes
are recorded in `build/performance-goal/terrain-batch-review.json`.

The model-preparation review covered merging, runtime compaction and cooked
plan loading. These optimizations already exist and preserve complete vertex
tuples and triangle ordering. Cooked plans avoid cold hash search; warm draws
already reuse retained meshes. A possible remaining cost is repeated structural
rejection when a workload cycles through more than the32 cached failed keys,
but workload evidence is missing. These paths do not reduce fragment count.
Details and exact source scopes are in
`build/performance-goal/model-preparation-review.json`.

The scenery review separates unchanged foliage thinning from exact retained
vertex copying. Scenery instancing can also reorder opaque draws across groups;
matching material and vertex data alone does not prove equal-depth framebuffer
parity with unbatched rendering. A coplanar/interleaved reference capture is
needed before making that stronger claim. No thinning or batching policy was
changed. Source scopes and memory/failure behavior are recorded in
`build/performance-goal/scenery-review.json`.

A host check including the production scenery batch scheduler now reproduces
its reordering: input1(A),2(B),3(A) submits1(A),3(A),2(B). ASan/UBSan passed and
all stub allocations were released. This proves the scheduling behavior, not
a rendered-image difference; the existing environment equal-depth tests do
not exercise this scheduler. Evidence is in
`build/performance-scenery-order`. Production batching remains unchanged.

The model/animation review adds whole-file coverage for model submission and
pose sampling. Potential next work includes merging solids that are adjacent
in the executed pass despite skipped source-list entries, and retaining decoded
keys shared by adjacent animation intervals. These need downstream state and
bitwise sample checks. Existing distant odd-frame pose synthesis is approximate
and remains unchanged. Receipts are `render-models-review.json` and
`animation-runtime-review.json` under `build/performance-goal`.

Follow-through on expanded model merging found two additional constraints:
post-merge foliage island selection can retain different geometry, and merging
parts rigid to different bones can switch their GPU path to weighted skinning.
Therefore actual-pass adjacency alone is insufficient for a lossless merge.
The existing merge eligibility has not been expanded. A stable per-pass cache
must preserve current raw adjacency boundaries unless these cases are guarded
and output equivalence is established.

The optional prepared-animation-track path now reuses a decoded end key when
it becomes the next interval's first key. It keeps raw quaternion values and
normalizes only the existing output copy; other seeks retain normal fallback.
The production ARM candidate/verify differential harness passed, with new
assertions covering forward and default-key reuse. The feature remains
default-off, and its physical timing benefit is unmeasured. Receipts are in
`build/performance-animation-endpoint`.
The fixed-400 native build with track verification enabled also passed.

The pose/hierarchy cache review confirmed the current CPU skin color depends
on a fixed normal-based formula, so the cache is not missing a mutable light
list key. Hierarchy reuse compares full orientation and parent-matrix bytes
and requires the expected ARM floating-point mode. Its hit rate and compare
cost still need native timing before default changes. The scoped receipt is
`build/performance-goal/pose-hierarchy-review.json`; no runtime change was made.

The texture-prefetch review confirmed bounded single-job admission and
worker-buffer retirement after acknowledgment. Late demand can temporarily
overlap speculative disk/decode work, but cancellation cannot safely free
buffers immediately. Existing late/read/prepare counters should establish
whether this costs native frame time before increasing concurrency or staging.
No runtime policy changed. The review is recorded in
`build/performance-goal/texture-prefetch-review.json`.

The worker/thread review confirmed synchronous output ownership and separate
dedicated/shared-core scheduling policies. Deferred decode's main-time metric
includes intervening owner work before joining; it is not exclusive decoding
time. Kernel completion protects thread stack reclamation, and live mutex
waiters protect handles from deletion. No synchronization or scheduling policy
changed. Full source scopes are in
`build/performance-goal/worker-scheduling-review.json`.

The screen-effects review found no redundant unconditional resolve pass:
postprocessing is folded into the existing world resolve, and inactive direct
rendering skips it. Removing its command-list split would require framebuffer
coherence proof. Existing Original-model RGB565 and corona size/count budgets
are quality policies and remain unchanged. Quantized TEV arithmetic also
prevents assuming that algebraic stage fusion preserves every pixel. Details
are in `build/performance-goal/screen-effects-review.json`.

The flashlight review confirms lighting is already combined into existing
material passes and unchanged components avoid dirty state updates. The
lightmap upload filter has a possible exact integer optimization: validated
power-of-two dimensions allow shifts/masks in place of repeated coordinate
divisions, preserving averaging and fill order. This remains a cold-loading
candidate, not a demonstrated warm-frame gain. The source receipt is
`build/performance-goal/lighting-state-review.json`.

The lightmap coordinate candidate is implemented using validated power-of-two
shifts/masks. Pixel averaging and padding fill remain unchanged. A sanitizer
differential check passed 360 cases and 7,818,880 RGBA words plus distance cells,
covering up/downsampling and mixed/all padding. This removes integer coordinate
divisions during texture preparation; native timing remains unmeasured.
Evidence is in `build/performance-lightmap-shifts`.

The texture-layout review confirms compact representations remain restricted
by CPU-consumer and mip-layout contracts. Candidate archive warm selection
uses a retained index and binary search; whole-map identity hashing remains
offline. Broadening compact formats would need consumer and image-equivalence
proof, so no format policy changed. The source receipt is
`build/performance-goal/texture-layout-review.json`.

The archive and demand-loader review found bounded corruption fallback and
direct decoding into the unpublished GPU image. LZ4 decompression is confined
to the comparison build, so vendor codec tuning would not improve normal
gameplay. Warm textures already use an owner-checked direct slot before slow
lookup. Payload validation remains intact; no runtime change was justified.
The scoped receipt is `build/performance-goal/texture-archive-review.json`.

The framebuffer/depth follow-up confirms the existing model depth pass is
required for self-occluding transparent objects. Material restores already
suppress clears; queue-idle waits also protect completion callbacks. A general
opaque prepass is not established as a gain and depends on the pending native
early-depth experiment, including alpha and stencil correctness. No runtime
change was made. Exact scopes are recorded in
`build/performance-goal/frame-depth-followup-review.json`.

The terrain batch-visibility review confirms conservative handling of shared
and unknown triangle ownership, ordered material iteration, cached portal-mask
expansion, and bounded index-run storage. Surface bounds are validated before
ownership construction. More aggressive culling would need new visibility
proof; no quality policy changed. A theoretical 16-bit material-owner limit
remains noted without a shipped-map violation. The exact review scope is in
`build/performance-goal/batch-visibility-review.json`.

The generic model review found unused body-texture coordinate preparation in
the two-map hair hologram path. Hair now skips the bounds scan and third-UV
normalization; body retains its exact operations. The hair TEV recipe does not
consume texture2, and its vertex shader only copies that independent UV output.
The fixed-400 native build passed; physical timing and image comparison remain
unmeasured. Existing generic bake and cinematic shading approximations were
not changed. Receipts: `build/performance-hologram-uv` and
`build/performance-goal/generic-model-review.json`. The build exited normally;
no simulator or emulator was launched.

The full generic evaluator review confirms signed stage arithmetic, concurrent
input semantics, disabled-texture defaults, and conservative TEV lowering
boundaries. Existing CPU generic and 2,048-case menu-plasma tests passed under
ASan/UBSan. Their tolerances do not establish bitwise GPU parity or native
performance. No further arithmetic rewrite was made. Receipts are in
`build/performance-generic-review` and
`build/performance-goal/generic-evaluator-review.json`.

The Chicago submission review confirms GPU transforms precede CPU staging
when the material permits them, while animated bitmap and UV inputs prevent
material-only caching. The unchanged 43,264-vector CPU combiner test passed
under ASan/UBSan with production generic arithmetic. This does not establish
GPU equivalence or remove the existing bake approximation. No runtime edit
was made. Receipts: `build/performance-chicago-review` and
`build/performance-goal/chicago-model-review.json`.

The mutable-bitmap review confirms updates preserve immutable versions for
queued draws; collection refuses in-frame reclamation. Glyph uploads are
already batched, with overlap-triggered publication preserving the bilinear
sampling footprint. In-place updates or delayed overlap flushes would break
that contract. Existing GPU lifecycle tests were inspected, not rerun. No
runtime change was made. Scope: `build/performance-goal/mutable-bitmap-review.json`.

The text submission review confirms mono glyph batching, deferred stream
flush, dimension-cached projection construction, sampler restoration and
revision-based touch texture updates. Stereo and offscreen paths have distinct
target semantics, so their batch boundaries remain intact. No runtime change
was justified. Scope: `build/performance-goal/text-submission-review.json`.

Widget/effect vertex layouts now initialize once and install through the
pinned Citro3D setter, preserving exact loader definitions and dirty-state
behavior. This removes repeated layout construction; fixed-400 native build
passed. Native timing and screenshot comparison remain unmeasured. Existing
particle budgets were not tightened. Review also noted sampler restoration
on allocation/draw failure as a follow-up, not a demonstrated FPS issue.
Receipts: `build/performance-widget-attributes` and
`build/performance-goal/widget-submission-review.json`. The build exited
normally; no emulator or simulator was launched.

The particle submission follow-up confirms adjacent-only batching, exact
source-index reuse and single index staging. Failure-path review confirms
sampler restoration can be bypassed after upload/draw failure; release builds
can continue past the caller assertion. A focused recovery test must also
cover vertex lifetime after partial first-person submission before changing
that path. No runtime change this review. Receipt:
`build/performance-goal/effect-submission-followup-review.json`.

Effect failure recovery now restores the sampler after index-upload or draw
failure and reserves the vertex span before submission can begin. This
prevents reuse after a partial first-person draw. Seven failure-injection
cases using the extracted production submission tail passed ASan/UBSan.
This is a correctness fix, not a measured frame-time gain. Native build and
host harness receipts are in `build/performance-effect-recovery`.

HUD ownership review confirms state-keyed touch-panel regeneration and
content-ID-based grenade-icon bounds caching. Radar contact history belongs
to gameplay; its GPU target clears and redraws each begin, with a smaller
existing CPU allocation fallback. Skipping updates without full input
identity would change animation. No runtime policy changed. Exact scope:
`build/performance-goal/hud-owner-review.json`.

HUD bake-cache review confirms current-frame references prevent eviction,
full meter/multiply keys preserve consumed inputs, and mutable sources retain
exact byte checks. Existing menu glow and multiply-resolution approximations
were not tightened. Constant-UV GPU fixtures do not establish spatial parity
for those approximations. No runtime change was made. Scope:
`build/performance-goal/hud-bake-cache-review.json`.

Completed the remaining HUD renderer review. The extracted production meter
arithmetic and existing tests passed ASan/UBSan, including 8,192 varied
constant pixels and exact packed-output comparisons. GPU radar/meter test
sources were inspected but not rerun; their tolerances and CPU-only benchmark
scope do not establish native FPS. Receipts: `build/performance-hud-review`
and `build/performance-goal/hud-completion-review.json`.

Model meter/shadow review identified possible reuse of decoded UV bounds and
earlier optional-frame bake preflight; both remain unimplemented pending
exact-input checks. Projected object shadows are already unavailable in this
port, an existing fidelity limitation rather than a saving introduced here.
No runtime change was made. Scope: `build/performance-goal/meter-shadow-review.json`.

Model meters now reuse decoded UV bounds on cache hits, retaining the
original scan on misses. All 65,536 compressed UV values remained bitwise
identical through unit scaling in the host sanitizer check; bound construction
uses the same order and comparisons. Native fixed-400 build passed. Physical
timing and image comparison remain unmeasured. Evidence:
`build/performance-meter-bounds`. Build/test processes exited normally.

Cache reader review confirms bounded load-time I/O, explicit heap/linear
ownership and direct tag lookup. Relocation validates all records before
mutation; removing that pass would weaken failure behavior. Checksums are
load-time work, not repeated per draw. Remaining load/unload paths need
further review before allocation policy changes. Scope:
`build/performance-goal/cache-reader-review.json`.

Completed cache load/unload review. New-model raw BSP copies remain bounded
and revalidate on use; mandatory allocations can reclaim optional caches.
Original models retain a reusable BSP arena. Asset reads share one locked
file handle, avoiding repeated opens. Broader prefetch lacks workload evidence
and was not enabled. Scope: `build/performance-goal/bsp-cache-lifecycle-review.json`.

Native input review confirms one normal-frame poll, saturating hold counters,
release-gated menu/gameplay transitions and fresh-press touch toggles. Optional
presentation must not duplicate polling or alter simulation input cadence.
No input math or controls changed. Scope:
`build/performance-goal/native-input-review.json`.

Graphics policy review confirms AUTO targets roughly 28–30 FPS and can
reduce resolution plus Original-model detail. It is not evidence for 60 FPS
at unchanged quality. Saved preferences and policy thresholds remain intact;
comparisons must pin matching resolution, texture settings and hardware.
Scope: `build/performance-goal/graphics-policy-review.json`.

Native logging now copies ring-buffer spans in bulk while retaining the same
lock boundaries, newline insertion and whole-message drop policy. Routine SD
writes remain on the worker outside the lock. An extracted-production host
check matched the original byte-loop behavior across 30,000 mixed operations
under ASan/UBSan; the native fixed-400 build passed. Neither check establishes
a native timing gain or concurrency stress coverage. Bootstrap and logging
lifecycle review is recorded in `build/performance-goal/log-queue-review.json`;
build/check evidence is in `build/performance-log-copy`. Processes completed;
no simulator or emulator was started.

Action-replay review confirms instrumentation is excluded from normal builds
and operates at simulation-tick boundaries in separate correctness builds.
Its selected-state digest cannot establish full simulation equivalence or
visual parity; native timing and image comparisons remain separate gates.
Journal I/O is outside ordinary per-tick recording, with bounded records and
map/start-state checks. No runtime changes or new replay runs were made.
Scope: `build/performance-goal/action-replay-review.json`.

Native platform review confirms the 30 Hz outer-loop pacing boundary, cached
hardware queries, validated memory accounting and checkpoint cache reset.
System timer frequency must remain distinct from boosted CPU execution speed.
The watchdog is a best-effort stall diagnostic with no normal SD writes, not
a frame-time measurement tool. No justified runtime optimization was found
in this section; simulation cadence, numerical mode and memory budgets remain
unchanged. Scope: `build/performance-goal/platform-timing-review.json`.

Shader-program review confirms existing single-influence and depth-only
specialization. Transform order, normal degeneracy handling and program
lifetimes remain intact. Optional instruction variants stay gated: instruction
counts and linked-byte checks do not prove native visual parity or timing
gains. The linked-byte verifier requires scene presence but records rather
than mandates every optional shader's presence. No shader behavior changed.
Scope: `build/performance-goal/shader-program-review.json`.

Previously uncredited sky-render review found a selection mismatch: the code
validates the authored animation index but fetches the loop index. A targeted
fixture and asset inspection are needed before folding a correction into
performance comparisons. Sky pose reuse must preserve phase cadence and
camera-dependent matrices/flares; no approximation or draw removal was made.
Scope: `build/performance-goal/sky-render-review.json`.

Sky animation selection is now corrected to use the authored index already
validated by the surrounding guard. The extracted production selection failed
before the fix and passed 80 slot/index/period combinations under ASan/UBSan
afterward. Native fixed-400 build passed. This corrects a potential visual
defect; affected real-map prevalence, native visuals and timing remain
unmeasured. Evidence: `build/performance-sky-index`. Build/check processes
exited and no emulator/simulator was started.

First-person, flying and orbiting camera review confirms mutable control and
transition behavior that must not run twice for extra presentation frames.
Vehicle marker/axis handling and FOV transitions remain unchanged. Director
command initialization resolves apparent partial-output concerns; the flying
constructor's sole caller supplies the complete position. No justified math
shortcut was found. Scope: `build/performance-goal/camera-modes-review.json`.

Point-physics review preserves wind/medium integration order, three-contact
limit and location updates on collision queries. The shared query already
skips environment traversal when collision classes are disabled. No-hit
queries can still update spatial location, so speculative collision removal
is not established lossless. No runtime changes were made. Scope:
`build/performance-goal/point-physics-review.json`.

Completed vector-query follow-up: cluster/object deduplication and sphere
rejection already precede detailed collision tests. Strict nearest-hit
replacement preserves traversal-order ties, constraining sorting/parallel
reductions. Clarification: the no-environment branch still resolves endpoint
location, so disabled collision classes do not eliminate all spatial work.
No behavior changed. Scope:
`build/performance-goal/vector-collision-followup-review.json`.

BSP primitive review confirms iterative point lookup shared by collision,
visibility and lens flares, with exact on-plane child classification. Line
intersection initialization passed all nine abstract endpoint-side cases;
this is not a floating-point geometry test. Line/polygon clipping has no
external callers in the searched repository C sources, so no gameplay gain
is established for rewriting it. Scope:
`build/performance-goal/bsp-primitives-review.json`.

Runtime structure-decal review found an existing caller-side bitset gate:
unchanged active clusters skip decal updates entirely. A duplicate check
inside the update would not help. Creation/deletion order and authored
placement remain unchanged. BSP reconnect consumption depends on the caller
lifecycle and needs follow-up before changing admission. Scope:
`build/performance-goal/structure-decals-review.json`.

Decal reconnect follow-up identified a correctness risk: BSP reconnect sets
the decal flag but preserves object active-cluster bitsets. Identical bitsets
across BSPs can suppress the caller's decal update until visibility changes.
A targeted caller/callee reproduction is needed before altering admission;
no native defect occurrence or timing gain is claimed. Scope:
`build/performance-goal/decal-reconnect-review.json`.

Decal storage follow-up confirms reconnect only reattaches existing datums;
it does not instantiate the new BSP's authored decals. Permanent removal
clears ownership/count and rasterizer vertices rather than directly freeing
each datum. The pending-reconnect admission fix must retain the ordinary
unchanged-visibility fast path and object activation gate. Scope:
`build/performance-goal/decal-storage-review.json`.

Pending decal reconnect now bypasses only the unchanged-visibility decal
gate, leaving object activation admission intact. Existing update consumes
the flag, so subsequent unchanged ticks skip decal work. An extracted
production fixture passed 32 transition/reconnect cases and repeat updates
under ASan/UBSan; disabling the new branch reproduced the old failure.
Native fixed-400 build passed. This is a fidelity correction, not a measured
speedup; native visual confirmation remains outstanding. Evidence:
`build/performance-decal-reconnect`. All test/build processes exited.

Material-effect dispatch review preserves visual/sound ordering and current
distance policy. Footstep material selection depends on collision and medium
state, so position-only caching is insufficient. Upper-bound-only effect
index validation needs event-source tracing before treating negative inputs
as a demonstrated runtime defect. No behavior changed. Scope:
`build/performance-goal/material-effects-dispatch-review.json`.

Footstep caller tracing found nonnegative enum/Boolean effect selectors,
not a reachable negative-index defect. Resource prediction review confirms
cluster-change admission and existing native nonblocking texture queue
deduplication; a second prediction cache is not justified here. Sound-request
internals remain outside this review. Scope:
`build/performance-goal/resource-prediction-review.json`.

Sound prediction follow-up distinguishes deferred requests from guaranteed
asynchronous loading: sound-cache idle consumes one pending slot and falls
back to synchronous reads when staging misses. Sound processing invokes this
path, so pending loads can affect frame pacing. Removing fallback or delaying
publication changes audio readiness; no such change was made without timing
evidence. Scope: `build/performance-goal/sound-prediction-followup-review.json`.

Shared-sort review measured quadratic equal-key behavior in extracted
qsort_2byte: 4,096 equal entries require 8,390,648 comparisons. Sanitizers
passed; this is synthetic host evidence, not native frame timing. AI score
comparators permit ties, so replacement requires preserving consumed ordering
or proving tie insensitivity. No sort behavior changed. Evidence:
`build/performance-sort-review`; scope `build/performance-goal/shared-sort-review.json`.

AI sort-consumer review confirms tie order is observable through strict-best
selection and post-evaluation ordering. A 16-ID tied-key fixture puts legacy
ID1 first versus stable ID0; replacing the algorithm is not established
behavior-equivalent. This caller caps candidates at512, so the earlier4096
case is generic stress only. Native timing remains unmeasured. Evidence:
`build/performance-sort-review/ties.log`; scope
`build/performance-goal/ai-sort-consumer-review.json`.

An all-equivalent-key sort prototype replays the legacy swap permutation
while avoiding quadratic comparisons. Across6,000 host cases, exact output
matched; equal512 comparisons fell from131,320 to1,022 with sanitizers passing.
It remains outside production: AI float/nonfinite and validity/rejection
semantics need exact admission checks. This is comparison-count evidence,
not native timing. Evidence: `build/performance-sort-equal`.

Native AI now uses exact legacy swap replay for fully tied finite firing
position groups with matching validity/rejection fields. All other inputs
retain qsort. Extracted production comparator/helper matched12,000 exact
permutations under ASan/UBSan, including nonfinite and signed-zero inputs;
native fixed-400 build passed. Mixed inputs incur admission-scan overhead,
so native workload prevalence and timing still need measurement. Evidence:
`build/performance-sort-equal`; no60FPS result is claimed. Processes exited.

ARM inspection found isfinite emitted a library call in the AI fast-path
admission. Exact float exponent-bit classification now removes that call;
12,000 permutation checks and native rebuild passed. This confirms generated
code improvement, not frame-time gain. Assembly/build evidence remains in
`build/performance-sort-equal`. All processes exited.

Script object-list review confirms bounded pools and constant-time prepend
and count queries. Allocation failure can leave the head unchanged while
object_list_add still increments count; a targeted exhaustion fixture is
needed before correction. Iteration order and reference ownership constrain
any storage rewrite. No runtime change made. Scope:
`build/performance-goal/script-object-lists-review.json`.

Script object-list counts now increment only after a successful reference
insertion. The old code failed the exhaustion fixture; the fix preserves
128 insertions' order and correct counts across1,000 failed additions under
ASan/UBSan. Native fixed-400 build passed. This is a correctness correction,
not a measured frame-time gain. Evidence: `build/performance-object-list`.
All test/build processes exited; no simulator/emulator was started.

Datum lifecycle follow-up resolves the list-deletion concern: deletion
retains payload bytes until reuse, so next-link reads remain valid under
current synchronous traversal. Allocation zeroes payload and assigns a new
identifier; compaction would invalidate slot/pointer assumptions and is not
a transparent runtime optimization. No allocator change made. Scope:
`build/performance-goal/datum-lifetime-review.json`.

Device control/light-fixture review preserves live group-value activation,
on/off/denied effects and authored light parameters. Per-tick callbacks are
mostly lookup/validation, but removing them lacks measured cost and complete
lookup-contract evidence. No runtime behavior changed. Scope:
`build/performance-goal/device-controls-review.json`.

Machine-device review confirms staggered door proximity checks and stationary
elevator query avoidance. Door activation accumulates a Boolean result and
continues scanning after success; an early exit is a candidate pending getter
side-effect checks. Motion, group publication and passenger translation order
remain unchanged. Scope: `build/performance-goal/machine-devices-review.json`.

Automatic door search now exits after finding an eligible opener. Source
review confirms later iterations cannot change activation and only read
valid candidate state; downstream door/elevator updates remain intact. Native
fixed-400 build passed. This removes up to15 redundant candidate inspections
per successful query; native frame benefit is unmeasured and no behavioral
harness was run. Evidence: `build/performance-door-exit`. Build exited.

Scenery lifecycle review distinguishes ambient permutation selection from
script-controlled animation. Scripted frame advancement and remaining-time
queries are observable gameplay state, so cadence reduction is not a
lossless optimization. Graph/frame/mode changes also constrain caches. No
runtime behavior changed. Scope:
`build/performance-goal/scenery-animation-review.json`.

Game-global lookup review confirms direct material/difficulty table access.
Team difficulty depends on mutable allegiance, whose transitions update
friendliness and notify AI; team/difficulty-only caching would be stale.
No additional cache or behavior change was justified. Scope:
`build/performance-goal/game-global-lookups-review.json`.

Completed allegiance review: scans are capped at8 records and unchanged
nonpermanent transitions already avoid duplicate AI notifications. Incident
decay and restoration are tick-dependent; batching would change gameplay
timing. No optimization justified by the reviewed bounded work. Scope:
`build/performance-goal/allegiance-completion-review.json`.

HUD warning-sound review confirms repeated checks retry failed starts, so
warning-flags-only caching would suppress valid retries. Impulse and looping
sounds retain different stop policies. Authored12-sound bound matches caller
handle storage; no scan or cadence reduction was justified. Scope:
`build/performance-goal/hud-sound-dispatch-review.json`.

UI event-manager review found a likely enqueue shift-direction defect:
entries move toward0 then0 is overwritten, while dequeue scans7..0. A
production enqueue/dequeue fixture is needed before changing behavior. Stick
repeat/suppression histories constrain input batching; no runtime edit yet.
Scope: `build/performance-goal/ui-event-queue-review.json`.

Corrected UI event enqueue shift direction to preserve earlier events for
reverse-index dequeue. Production-code host fixture reproduced old event
loss and now passes FIFO, eight-entry overflow, suppression and flush with
ASan/UBSan. Native fixed-400 build passed. Native menu interaction remains
unverified; this is correctness work, not a measured FPS gain. Evidence:
`build/performance-event-queue`. All test/build processes exited.

### Durable input queue regression check

`python3 tools/verify_event_queue.py` now extracts the complete production event manager, its event record, and the input button/state declarations into a native-width host harness. ASan/UBSan checks FIFO order, eight-entry overflow, suppression, flush, controller isolation, simultaneous-button update order, the 250 ms stick repeat boundary, and disposal. A temporary copy with the original reversed queue shift fails the check as expected. The harness uses temporary files and bounded compiler/test subprocesses. No runtime code changed in this follow-up; no native FPS or visual-equivalence claim follows from this host check. Native neutral-stick/re-entry behavior remains a separate review item.

### Terrain pixel-work follow-up

The current plain/detail terrain path already renders base, detail, lighting and material tint in one draw per index chunk. Emissive terrain also uses one draw: stages 0–2 form the emission sum from texture 2, stage 3 forms lighting, stage 4 adds buffered emission, and stage 5 multiplies base texture 0. Base alpha remains independently authored. Reordering these stages would move saturation and rounding boundaries, so it is not an exact optimization.

The unused-texture-units candidate is deliberately scoped only to plain/detail terrain. Emissive terrain without a lightmap could potentially omit unit 1 (mask 5), but that extension is not implemented or validated by the existing plain-terrain comparisons. Any extension needs emission-to-plain/bump transition coverage and native measurements. No runtime change was made in this follow-up.

### Sampler transition matrix

Expanded the durable production-adapter host test to 384 combinations: four preceding masks × four following masks × eight stock binding subsets × three dirty-state modes. ASan/UBSan passed, including exact command and invalidation counts and restoration without descriptor changes. This closes a command-transition coverage gap relevant to a possible emissive mask-5 extension. It does not implement that extension, establish pixel equivalence, or measure native performance. All test subprocesses exited; no simulator or emulator was launched.

### Native fog reachability

The retained current native link map places all six Xbox environment-fog and fog-screen begin/draw/end entry points in discarded input sections, not linked text. Optimizing these old multipass draws cannot improve this executable's GPU time. Native camera setup explicitly records opaque atmospheric fog as unimplemented; selected generic material vertex inputs still compute atmospheric/planar visibility on the CPU. That partial support must not be mistaken for renderer-wide fog or removed as a performance shortcut. This review made no runtime changes.

### Skip unused generic fog coordinates

Generic material CPU vertex evaluation now accumulates eye depth only when atmospheric or planar fog consumes it, and plane coordinates only for planar fog. Active calculations retain their component order and run before reflection overwrites the eye vector. At source level the fog-free path skips both three-component accumulations. Production ARM inspection narrows the incremental saving: the compiler already gated plane arithmetic in the old version. The new version additionally skips three depth multiplies and three depth subtracts, and moves plane coefficient loads behind the planar branch. Extra branches and changed instruction scheduling mean net native time remains unmeasured.

An extracted production before/after harness passed 100,000 bit-exact comparisons across all four fog modes under ASan/UBSan, using the native build's `-ffp-contract=off` setting. The fixed-400 native build passed in 15.3 seconds (SHA-256 `55439af2f4fc975628ed3bae7861bc751e8e8270271baba532aa5e0141881e11`). No new native image or frame-time evidence is available. All test/build processes exited; no simulator or emulator was started.

### ARM verification of fog-coordinate change

Compiled both the pre-change retained snapshot and the current snapshot to ARM assembly using their production response files. Before, the depth accumulation executes before the atmospheric check; plane arithmetic is already conditional. After, an `orrs`/`beq` on the two fog flags bypasses both coordinate blocks when no fog is active. Assembly and reduced instruction listings are retained under `build/performance-generic-fog`. This confirms a real change in emitted work, not a frame-time result.

### Lit-lerp input specialization

The native lit-lerp branch now runs before generic facing/reflection evaluation. Its RGB remains posed diffuse lighting (white for sky), and alpha remains positional fog visibility multiplied by angular fade. A positional helper preserves the old eye subtraction and accumulation order. Normal streams are requested only when angular fade or other per-vertex generic shading needs them; lit lerp without angular fade avoids that cache lookup/normal transformation entirely. Texture transforms, binding, blend configuration and geometry stay the same.

The positional helper passed 100,000 bitwise comparisons with the old generic-view visibility across all four fog modes under ASan/UBSan. The fixed-400 native build passed in 14.07 seconds, SHA-256 `81072d0af561f440932de51e8127c8d9d6200e213102bac2c3cafd8ff91cdc05`. This validates finite generated inputs and compilation, not native image parity or frame time. All build/test processes exited; no simulator or emulator ran.

### Lit-lerp admission regression

`python3 tools/verify_generic_classification.py` extracts the production classifier and links the production generic evaluator. ASan/UBSan passed its existing combiner arithmetic self-tests, 12,288 type/flags/blend/fade combinations, cache-hit equivalence, 52 rejected opcode mutations and cache generation changes. Lit lerp excludes cube and screen-projected materials, requires blend 0 and the exact two-map/two-stage recipe, and does not overlap the hologram or one-stage native paths. The GPU consumer uses only the two textures and primary lighting/fog color; facing and reflection are not inputs. This supports the previous CPU specialization without claiming native frame-time or image validation.

### Non-cube shield input specialization

Non-cube shield materials now bypass generic facing/reflection evaluation and use the positional fog helper. The shield evaluator consumes only its material constants and sampled textures; final fog/fade consumes only visibility alpha. Cube shields retain reflection evaluation, and angular fade retains normal processing. Shared normal-stream admission follows those same requirements. Sampling, shield arithmetic and blend/fade ordering are unchanged.

The 100,000-case positional visibility comparison passed under ASan/UBSan. This checks the helper, not full shield rendering; branch and consumer inspection establish which values are used. Native fixed-400 compilation/linking passed in 14.63 seconds, SHA-256 `29e9053e049a27dc7a0d12862698a66006d28696c3c796cf15ebf0d7d1e7875d`. Native image parity and timing remain outstanding. Test/build processes exited; no simulator or emulator was launched.

### Remove generic-view clear call

The full generic-view helper now explicitly zeros its three RGB outputs instead of clearing eight floats and immediately overwriting five of them. All eight vertex outputs and three reflection outputs remain assigned. A 100,000-case before/after comparison with different buffer prefill patterns passed bitwise under ASan/UBSan. Native ARM assembly confirms the external clear call is removed (three to two `csmemset` call sites in the translation unit). The fixed-400 build passed in 14.13 seconds, SHA-256 `18634a6fb4e281818301645b556e89fb5c6e47bb73cb78f65a112c56afa7f0d6`. Hardware frame time remains unmeasured. All processes exited; no simulator/emulator started.

### Durable generic vertex regression coverage

`python3 tools/verify_generic_view.py` now extracts the current production fog/view helpers and compares them against `port/n3ds/tests/host/generic_view_reference.inc`, a frozen pre-optimization reference. It needs no retained build directory. The 100,000-case ASan/UBSan run covers all fog mode combinations, varying positive ranges/depths/distances, densities including zero and one, random geometry/camera/plane data, and different initial output-buffer patterns. Every vertex/reflection float and positional visibility matches bitwise. A temporary mutated source omitting RGB initialization fails, confirming that the verifier catches incomplete writes. The production classifier/arithmetic checks also passed again. This is host arithmetic coverage, not native FPS or rendering evidence; all processes exited.

### Refreshed native sampler probe

Built the current source with `--fixed-world-width 400 --unused-texture-units --gpu-probe`; compilation/linking passed in 15.87 seconds. The executable at `build/performance-current-native-probe/texture-units-probe/Halo3DS.3dsx` hashes to `b2de92931c76d06c4bc1e6aefd3ed1745523ee9ff90bc79ae9636c29f81cba95`, verified against its build receipt. Receipt confirms legacy textures, runtime geometry, float vertices, no VRAM mirrors, no opaque-read omission, no framebuffer reuse and no optional presentation.

`build/performance-current-native-probe/RUN-ON-CONSOLE.txt` supplies the bounded paused New 2DS XL comparison procedure and expected log. All phases include the recent CPU changes; their contrast measures only sampler enabling. No console SD volume or new probe log was visible during this check, and no native execution occurred. The build exited normally; no simulator/emulator started.

### Sprite construction and debug-only coverage

Reviewed the complete sprite builder and native effect-coverage helper. Native sprite admission already computes its tile bounds separately; the older frustum coverage calculation only feeds optional debug text. It now runs only when `debug_sprites` is enabled. Sprite admission, centroid updates, buffer lifetime, grouping, ordering and existing density budgets are unchanged. The first debug display after enabling can show zero for a preceding frame that was not measured.

Production ARM inspection confirms a conditional branch bypasses the legacy fraction call when debug is off. Native fixed-400 build passed in 16.25 seconds, SHA-256 `0a91cb469c95eedcac1ef2a66285850e2880ce18c500198ef0ad437b206b7b72`. Hardware benefit remains unmeasured. This postdates the delivered sampler probe; that probe's receipt still identifies its own exact earlier source. Compiler/build processes exited; no simulator/emulator started.

### Sprite caller follow-up

All six external sprite-batch call sites use zero flags or the first-person flag; none select screen-space mode. The unsupported screen-space transform branch therefore is not reached by these current callers. Rotational sprites are used by both particle-system state and transition rendering. Their sideways-animation negative-angle index can equal the sequence count when the source index is zero, causing the existing sprite bounds check to omit the frame. The intended replacement requires authored sequence evidence; no indexing change was guessed. Transition drawing also deliberately offsets view Z by 0.001, which must be preserved when considering batching. No runtime change or native performance claim in this follow-up.

### Simple-particle submission review

Reviewed the complete simple-particle submission function. Its 1,024-record stack array matches the allocator capacity. Sorting groups by definition, cluster and first-person attachment; equal-key order can affect transparency and the existing global native draw budget. An unstable-sort replacement or direct bucket traversal would therefore require exact selection/order evidence. The existing 512-group cap and native thinning/draw limits were not tightened.

Other preserved behavior includes regular-frame lifetime markers for visually omitted particles, parent-invalidity deletion outside midpoint previews, empty-group radius protection, and LOD admission before optional interpolated positions. No runtime change or performance claim follows from this audit.

### Particle sort implementation boundary

The native particle object has an `R_ARM_CALL qsort` relocation at offset 0x174. Its retained link map resolves `qsort` to devkitARM's ARMv6K/FPU `libc.a(libc_a-qsort.o)`, at 0x002fa398 for this build. This is distinct from Halo's custom `qsort_4byte`, whose equal-key permutation was used by the earlier AI shortcut. That shortcut's correctness and pathological comparison counts cannot be transferred to particle sorting. No sort replacement was made; exact ordering and cost would need separate evidence.

### Reuse particle radius inputs

Native particle rendering now calls the existing data-based radius helper with its already validated particle and group definition, avoiding a repeated datum lookup and tag lookup. The helper's arithmetic is unchanged; its declaration is now public. The non-native renderer retains its prior wrapper call. Group and particle definition lookup resolve the same absolute tag index; the live data are not mutated between acquisition and radius evaluation.

Native fixed-400 build passed in 15.73 seconds, SHA-256 `7989beecea1ae65023018f46f6045a2a8835893779f738ab23f9ed0ad336d5c8`. Object relocations confirm the renderer calls the data helper, which has no call relocations. This is source/dataflow and build evidence, not a hardware timing result. All subprocesses exited; no simulator/emulator started.

### Particle animation lifetime boundaries

Reviewed sequence/frame advancement and creation timing. Initial, looping and final sequences consume random draws at state transitions; missing sequences can emit an effect/sound and delete the particle. The update caller correctly gates physics on animation survival. A direct frame skip or cached access after a failed transition could therefore change random state, effects or lifetime. Creation samples FPS once and uses its reciprocal (or FLT_MAX for disabled animation); no simulation-rate reduction or loop cap was introduced. No runtime change in this follow-up.

### Particle impact-effect sentinel

The outer particle impact-effects gate used the material-effect tag index as a Boolean, despite the inner consumer using `NONE`. It now uses `index != NONE`: absent effects avoid unused magnitude/scale calculation, and tag index zero is not incorrectly excluded. Collision response, effect ordering, contact death, rest thresholds and physics remain unchanged.

An extracted production-condition test reproduces the old failure for two absent effects and passes all 25 index combinations after the correction under ASan/UBSan. Native fixed-400 build passed in 14.24 seconds, SHA-256 `9c5f833455f4ed0c7e5fc994d1ef6444f317e7a832aa4cc4a1e7dd79840b60cb`. Full collision replay, native image comparison and frame-time measurements remain outstanding. All subprocesses exited; no simulator/emulator started.

### Complete simple-particle lifecycle review

Completed the remaining creation, visibility, map lifecycle and budget sections of `particles.c`. Creation performs attachment transformation and visibility rejection before allocation/random sampling; reordering those steps changes later outcomes. Lighting and diffuse tint have independent flags. BSP reconnect resolves particle locations again and retires invalid attachments. Existing cosmetic lifetime limits remain unchanged.

The 15-unrendered-frame retirement threshold uses `render.frame_index`. The native mission path increments this counter only for regular frames (`!midpoint`), so optional extra presentation frames do not shorten that timeout. This is source-level timing evidence, not validation of native 60 FPS. No runtime change was made in this follow-up.

### Physics-variable review and receipt reconciliation

Reviewed physics-variable speed/position helpers and their header. Acceleration reversal, asymmetric speed limits, strict half-range wrap direction and target-crossing clamps are behaviorally significant; they must not be replaced with generic interpolation or modulo during frame-rate work. No simulation change was made. Restored whole-file particle-render review credit by matching the earlier reviewed snapshot hash and confirming its sole subsequent diff is the separately reviewed radius-call replacement.

### Vehicle control timing boundary

Reviewed vehicle control setup, braking, steering and upending. Speed/slide seek use per-tick steps, steering uses either a fixed step or `1/TICKS_PER_SECOND`, and upending is bounded by 30 update ticks. Vehicle updates are registered as object datum updates and reached through simulation `game_tick`/`objects_update`. Doubling simulation calls to reach a display-rate target would alter these behaviors; optional presentation must remain separate. No runtime change or update-rate reduction was made.

### Vehicle physics dispatch and resting updates

Reviewed vehicle-update dispatch through its final animation/blur handling. Resting vehicles can still require stopping and suspension updates; active physics is already gated separately. Slipping, suspension, conditional crash effects and airborne accounting have a defined order. Aircraft bounds, fall damage and unit animation use separate conditions, while blur region updates already run only on threshold transitions. No broader early-return or integration-rate shortcut was introduced.

### Suspension and crash-feedback semantics

Reviewed suspension probes and crash feedback. Suspension smooths each collision-derived shift with the prior quantized value, then quantizes again. Its Boolean return specifically signals emitted suspension sound, suppressing the caller's crash damage/sound pass for that tick. Crash feedback uses velocity-change magnitude and emits once for the first flagged mass point. Probe decimation, contact reordering or treating the return as generic ground contact would alter behavior. No such changes were made.

### Defer unused slipping-effect lookup

Vehicle slipping-effect generation now looks up each mass-point definition only after contact and speed (>0.03) admission. The definition's fields were unused otherwise. Effect order, position/normal calculations and scale remain unchanged for valid assets. Airborne counters already stop on the first ground contact and retain a distinct secondary-contact behavior; that path was left intact.

Native fixed-400 build passed in 13.88 seconds, SHA-256 `388a2e2bdb57e691e861ef4ea96ef002122e309f1a8d691c927e7d094ec3d3d7`. Validation is the scoped dataflow/diff review and native build, not a vehicle replay or measured hardware gain. The build exited; no simulator/emulator started.

### Reuse Ghost thruster reflection

Ghost thruster effects now copy the computed reflected vector into the midpoint marker instead of calling the same pure reflection function twice with unchanged inputs. Random draws, collision queries, marker positions and effect emission are unchanged. The stopping-path review also confirms that final position updates refresh map membership and node matrices; those updates were preserved.

Native build passed in 13.78 seconds, SHA-256 `9348f38b305cacf361503e3fa21951e1585dca0fc27103b9aa2e33b1835f22fb`. Production ARM reflection call sites decrease from three to two. Native rendering and frame-time validation remain outstanding. All compiler/build processes exited; no simulator/emulator started.

### Vehicle overlay inputs

Reviewed vehicle node preprocessing and continuous-overlay frame selection. Steering, velocity, wheel and suspension overlays mutate common node orientations in order. Wheel animation uses `frame_count`, while most other inputs use `frame_count-1`; the overlay helper's final-frame handling and next-frame wrap make that distinction observable. A vehicle-ID-only pose cache or generic normalized-index conversion would be insufficient. No animation timing, endpoint or ordering change was made.

### Plane integration and physics dispatch

Reviewed plane force/torque generation and shared physics entry. A suppression flag zeroes contact data but does not suppress subsequent thruster effects. Throttle squaring, hover smoothing, yaw and quaternion torque are per-tick behavior. Shared physics selects its old/new implementation using authored radius; the new path consumes and resets accumulated collision force/torque. These stateful paths cannot be merged as redundant calls. Fixed temporary-array capacity versus authored mass-point counts remains a separate validation trace. No runtime change was made.

### Mass-point capacity evidence gap

Vehicle temporaries contain 32 mass points and 32 powered points. The physics tag header declares variable tag blocks, and inspected consumers follow their counts. Repository/native-tool searches did not locate an explicit matching 32-count validation. This is an unresolved authored-data invariant, not evidence that existing assets overflow. Buffer shrinking, resizing or generalized integration must not assume a proven capacity contract until asset validation/count evidence is available. No runtime modification was made.
### BSP visibility span verification

The current production `bsp_visible_material_indices` function passes
`python3 tools/verify_bsp_visibility_spans.py`: 20,000 deterministic randomized
ownership-run layouts match an independent per-index oracle under AddressSanitizer
and UndefinedBehaviorSanitizer. The check compares count-only and copied outputs,
exact retained index order, and output sentinels, including shared/unassigned
owners. Stubbed legacy selection also checks the optional vertex-remap step.
The fixture now includes the complete production visibility-cache include.
Directed cases check cache hits, unchanged queued indices after a second view
in one frame, independent stereo buffers, next-frame rebuilds, empty/full
visibility, and allocation failure. Source review confirms the cache frame
counter advances after GPU frame acquisition; the midpoint reserved path has
already acquired the frame through `begin_frame(1)`.
These host checks do not validate PVS construction, actual GPU fence behavior,
physical GPU performance, or campaign images. No runtime behavior changed in
this follow-up.

The fixture now extracts the actual visibility descriptor declarations from
`engine_renderer.h`, replacing simplified host declarations. It also exercises
production `batch_add_surface` over all 4,096 pairs of clusters in its 64-cluster
test domain: repeated ownership stays unique, distinct ownership stays shared
after later duplicate visits, material masks accumulate both owners, and invalid
surface indices are rejected. The builder's source material ranges are checked
in `engine_mission_render.c` before ownership preparation. Parsing complete
native cluster/subcluster records remains outside this fixture's scope.
