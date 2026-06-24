// fx-worklet.js — AudioWorkletProcessor for the F/X module player.
// The Wasm instance lives here on the audio thread. No SharedArrayBuffer needed.
//
// Android Chrome performance notes:
//   - process() runs ~375×/sec; any per-call allocation causes GC stalls that
//     exceed the 2.67 ms deadline → all typed-array views are pre-allocated in
//     _init() once and reused forever.
//   - We render LOOKAHEAD_FRAMES (2048) from WASM at a time and serve 128-frame
//     quanta from a Float32 lookahead buffer. This reduces the WASM call rate
//     16× and keeps each process() call extremely cheap (array copy only).

class FxWorkletProcessor extends AudioWorkletProcessor
{
    constructor(options)
    {
        super(options);
        this._wasm           = null;
        this._playing        = false;
        this._blockCount     = 0;
        this._lastModuleSize = 0;
        this._volume         = 64;   /* mirrors fx_get_volume(); restored after each load */

        // Report state every ~19 quanta ≈ 50 ms at 128 frames/quantum.
        this._STATE_INTERVAL = 19;

        // Pre-allocated views — set in _init() once WASM memory is live.
        this._s16View      = null;   /* Int16Array into WASM render buffer              */
        this._stateView    = null;   /* Uint32Array over fx_playback_state in WASM mem  */
        this._lookaheadF32 = null;   /* Float32Array: decoded ahead-of-playback frames  */
        this._LOOKAHEAD    = 0;      /* = wasm_render_buf_frames() (2048)               */
        this._laRead       = 0;      /* read head inside _lookaheadF32                  */
        this._laAvail      = 0;      /* frames ready to consume in _lookaheadF32        */

        this.port.onmessage = (event) => this._handleMessage(event.data);
    }

    _handleMessage(msg)
    {
        switch (msg.type)
        {
            case 'init':   this._init(msg.wasmBytes);   break;
            case 'load':   this._load(msg.moduleBytes); break;
            case 'toggle':
                this._playing = !this._playing;
                // Immediately report new state so the button flips without delay.
                if (this._wasm) this._reportState();
                this.port.postMessage({ type: this._playing ? 'playing' : 'paused' });
                break;
            case 'stop':
                this._playing = false;
                this._laRead  = 0;
                this._laAvail = 0;
                if (this._wasm && this._lastModuleSize > 0)
                {
                    // Re-call wasm_load() — module bytes still live in g_module_buf.
                    this._wasm.wasm_load(this._lastModuleSize);
                    this._wasm.fx_set_volume(this._volume);
                }
                if (this._wasm) this._reportState();
                this.port.postMessage({ type: 'stopped' });
                break;
            case 'volume':
                this._volume = msg.value;
                if (this._wasm) this._wasm.fx_set_volume(this._volume);
                break;
            case 'order':
                if (this._wasm) this._wasm.fx_order_jump(msg.delta);
                break;
        }
    }

    _init(wasmBytes)
    {
        WebAssembly.instantiate(wasmBytes)
            .then(({ instance }) =>
            {
                this._wasm = instance.exports;
                const wasm = this._wasm;

                // Pre-allocate all typed-array views into WASM linear memory.
                // The memory buffer never grows (static arenas), so these views
                // remain valid for the lifetime of the worklet.
                const lookaheadFrames  = wasm.wasm_render_buf_frames();   // 2048
                this._LOOKAHEAD        = lookaheadFrames;
                this._s16View          = new Int16Array(
                    wasm.memory.buffer, wasm.wasm_render_buf(), lookaheadFrames * 2);
                this._stateView        = new Uint32Array(
                    wasm.memory.buffer, wasm.wasm_state_ptr(), 7);
                this._lookaheadF32     = new Float32Array(lookaheadFrames * 2);

                this.port.postMessage({ type: 'ready' });
            })
            .catch((err) =>
            {
                this.port.postMessage({ type: 'error', message: err.message });
            });
    }

    _load(moduleBytes)
    {
        try
        {
            const wasm = this._wasm;
            if (!wasm)
            {
                this.port.postMessage({ type: 'error', message: 'Wasm not ready' });
                return;
            }

            if (moduleBytes.byteLength > wasm.wasm_module_buf_size())
            {
                this.port.postMessage({ type: 'error', message: 'Module too large (> 4 MB)' });
                return;
            }

            // Copy module bytes into Wasm linear memory, then load.
            const bufPtr = wasm.wasm_module_buf();
            new Uint8Array(wasm.memory.buffer).set(new Uint8Array(moduleBytes), bufPtr);

            // sampleRate is a global in AudioWorkletGlobalScope.
            wasm.wasm_set_config(sampleRate, 1, 1);

            const loadErr = wasm.wasm_load(moduleBytes.byteLength);
            if (loadErr !== 0)
            {
                this.port.postMessage({ type: 'error', message: `fx_load failed: ${loadErr}` });
                return;
            }

            // Restore volume — wasm_load() resets engine state to default (64).
            wasm.fx_set_volume(this._volume);

            // Flush lookahead so stale frames from the previous module are not played.
            this._laRead  = 0;
            this._laAvail = 0;

            const title = this._readCString(wasm.fx_song_title());
            this._lastModuleSize = moduleBytes.byteLength;
            this._playing        = true;
            this._blockCount     = 0;
            this.port.postMessage({ type: 'loaded', title });
        }
        catch (err)
        {
            this.port.postMessage({ type: 'error', message: `_load threw: ${err}` });
        }
    }

    // Read a null-terminated C string from Wasm linear memory at ptr.
    // AudioWorklet scope lacks TextDecoder; decode as Latin-1 with fromCharCode.
    _readCString(ptr)
    {
        const mem = new Uint8Array(this._wasm.memory.buffer);
        let result = '';
        for (let index = ptr; mem[index] !== 0; index++)
            result += String.fromCharCode(mem[index]);
        return result;
    }

    _reportState()
    {
        const wasm = this._wasm;
        wasm.wasm_get_state();
        // _stateView is a pre-allocated Uint32Array over fx_playback_state.
        const state = this._stateView;
        this.port.postMessage({
            type:            'state',
            playing:         this._playing,
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

    process(inputs, outputs)
    {
        if (!this._wasm || !this._playing)
            return true;

        const leftOut  = outputs[0][0];
        const rightOut = outputs[0][1];
        const QUANTUM  = 128;   // AudioWorklet render quantum; always 128

        // Refill the lookahead buffer from WASM when it can no longer satisfy
        // a full quantum. Rendering LOOKAHEAD frames at a time amortises the
        // WASM call cost across 16 quanta instead of paying it every call.
        if (this._laAvail < QUANTUM)
        {
            const rendered = this._wasm.wasm_render(this._LOOKAHEAD);

            if (rendered > 0)
            {
                const scale = 1.0 / 32768.0;
                const s16   = this._s16View;
                const f32   = this._lookaheadF32;
                for (let frame = 0; frame < rendered; frame++)
                {
                    f32[frame * 2]     = s16[frame * 2]     * scale;
                    f32[frame * 2 + 1] = s16[frame * 2 + 1] * scale;
                }
                this._laRead  = 0;
                this._laAvail = rendered;
            }

            if (rendered < this._LOOKAHEAD && this._laAvail === 0)
            {
                // Engine reached the end and the lookahead is fully drained.
                this._playing = false;
                this.port.postMessage({ type: 'ended' });
                return true;
            }
        }

        // Copy up to QUANTUM frames from the lookahead to the output buffers.
        const frames = Math.min(QUANTUM, this._laAvail);
        const read   = this._laRead;
        const f32    = this._lookaheadF32;
        for (let frame = 0; frame < frames; frame++)
        {
            leftOut[frame]  = f32[(read + frame) * 2];
            rightOut[frame] = f32[(read + frame) * 2 + 1];
        }
        this._laRead  += frames;
        this._laAvail -= frames;

        // Periodic state report — every ~19 quanta ≈ 50 ms.
        this._blockCount++;
        if (this._blockCount >= this._STATE_INTERVAL)
        {
            this._blockCount = 0;
            this._reportState();
        }

        return true;
    }
}

registerProcessor('fx-worklet', FxWorkletProcessor);
