// fx-worker.js — DedicatedWorkerGlobalScope: owns the WASM instance.
//
// Runs on a Worker thread — no audio deadline, full CPU budget.
// Pre-renders audio in CHUNK_FRAMES-sized blocks and transfers ownership of
// Float32Array stereo chunks directly to the AudioWorklet via a MessageChannel
// port (set up once by the main thread).  The audio thread never calls WASM.

const CHUNK_FRAMES   = 4800;   // 100 ms at 48 kHz per chunk
const PREFILL_CHUNKS = 5;      // 500 ms sent immediately on load before the interval starts
const STATE_INTERVAL = 2;      // report playback state every 2 chunks ≈ 200 ms

let wasm           = null;
let s16View        = null;     // pre-allocated view into WASM render buffer
let stateView      = null;     // pre-allocated view over fx_playback_state
let audioPort      = null;     // MessageChannel port → AudioWorklet (receives chunks)
let playing        = false;
let volume         = 64;
let sampleRate     = 48000;
let lastModuleSize = 0;
let chunkCount     = 0;
let intervalHandle = null;

/* ── render one chunk and transfer it to the AudioWorklet ── */

function renderChunk()
{
    if (!wasm || !playing || !audioPort) return;

    const rendered = wasm.wasm_render(CHUNK_FRAMES);

    if (rendered > 0)
    {
        const f32   = new Float32Array(rendered * 2);
        const scale = 1.0 / 32768.0;
        for (let frame = 0; frame < rendered; frame++)
        {
            f32[frame * 2]     = s16View[frame * 2]     * scale;
            f32[frame * 2 + 1] = s16View[frame * 2 + 1] * scale;
        }
        // Transfer buffer ownership — zero-copy delivery to the audio thread.
        audioPort.postMessage({ type: 'chunk', buffer: f32.buffer, frames: rendered },
                              [f32.buffer]);
    }

    chunkCount++;
    if (chunkCount % STATE_INTERVAL === 0) reportState();

    if (rendered < CHUNK_FRAMES)
    {
        // Partial chunk: the engine reached end-of-song inside this render.
        stopInterval();
        playing = false;
        self.postMessage({ type: 'ended' });
    }
}

/* ── interval management ── */

function startInterval()
{
    if (intervalHandle) return;
    // Fire at exactly the real-time rate; the 500 ms prefill absorbs any jitter.
    intervalHandle = setInterval(renderChunk, CHUNK_FRAMES / sampleRate * 1000);
}

function stopInterval()
{
    if (intervalHandle) { clearInterval(intervalHandle); intervalHandle = null; }
}

/* ── state reporting ── */

function reportState()
{
    if (!wasm) return;
    wasm.wasm_get_state();
    const state = stateView;
    self.postMessage({
        type:            'state',
        playing,
        order:           state[0],
        order_count:     state[1],
        pattern:         state[2],
        row:             state[3],
        row_count:       state[4],
        channels:        state[5],
        channels_active: state[6],
        loops:           wasm.fx_song_loops(),
        volume:          wasm.fx_get_volume(),
    });
}

/* ── C string helper ── */

function readCString(ptr)
{
    const mem = new Uint8Array(wasm.memory.buffer);
    let result = '';
    for (let index = ptr; mem[index] !== 0; index++)
        result += String.fromCharCode(mem[index]);
    return result;
}

/* ── message handler ── */

self.onmessage = function(event)
{
    const msg = event.data;
    switch (msg.type)
    {
        case 'audioPort':
            // Direct port to the AudioWorklet; audio chunks go here, not via main thread.
            audioPort = msg.port;
            break;

        case 'init':
            WebAssembly.instantiate(msg.wasmBytes)
                .then(({ instance }) =>
                {
                    wasm = instance.exports;
                    // Pre-allocate typed-array views; WASM memory never grows (static arenas).
                    const frames = wasm.wasm_render_buf_frames();
                    s16View   = new Int16Array(wasm.memory.buffer,
                                              wasm.wasm_render_buf(), frames * 2);
                    stateView = new Uint32Array(wasm.memory.buffer,
                                               wasm.wasm_state_ptr(), 7);
                    self.postMessage({ type: 'ready' });
                })
                .catch((err) => self.postMessage({ type: 'error', message: err.message }));
            break;

        case 'load':
        {
            stopInterval();
            sampleRate = msg.sampleRate;

            const bytes = new Uint8Array(msg.moduleBytes);
            if (bytes.byteLength > wasm.wasm_module_buf_size())
            {
                self.postMessage({ type: 'error', message: 'Module too large (> 4 MB)' });
                break;
            }

            new Uint8Array(wasm.memory.buffer).set(bytes, wasm.wasm_module_buf());
            wasm.wasm_set_config(sampleRate, 1, 1);
            const loadErr = wasm.wasm_load(bytes.byteLength);
            if (loadErr !== 0)
            {
                self.postMessage({ type: 'error', message: `fx_load failed: ${loadErr}` });
                break;
            }

            wasm.fx_set_volume(volume);
            lastModuleSize = bytes.byteLength;
            chunkCount     = 0;
            playing        = true;

            // Flush any buffered audio from the previous module in the AudioWorklet.
            if (audioPort) audioPort.postMessage({ type: 'flush' });

            // Prefill: send PREFILL_CHUNKS immediately so the worklet queue is full
            // before the first AudioContext process() call renders audio.
            for (let chunk = 0; chunk < PREFILL_CHUNKS; chunk++) renderChunk();
            startInterval();

            self.postMessage({ type: 'loaded', title: readCString(wasm.fx_song_title()) });
            break;
        }

        case 'toggle':
            playing = !playing;
            if (playing) startInterval();
            else         stopInterval();
            self.postMessage({ type: playing ? 'playing' : 'paused' });
            reportState();
            break;

        case 'stop':
            stopInterval();
            playing = false;
            if (wasm && lastModuleSize > 0)
            {
                wasm.wasm_set_config(sampleRate, 1, 1);
                wasm.wasm_load(lastModuleSize);
                wasm.fx_set_volume(volume);
            }
            if (audioPort) audioPort.postMessage({ type: 'flush' });
            chunkCount = 0;
            self.postMessage({ type: 'stopped' });
            reportState();
            break;

        case 'volume':
            volume = msg.value;
            if (wasm) wasm.fx_set_volume(volume);
            break;

        case 'order':
            if (wasm) wasm.fx_order_jump(msg.delta);
            break;
    }
};
