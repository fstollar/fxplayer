// fx-worklet.js — AudioWorkletProcessor for the F/X module player.
// The Wasm instance lives here on the audio thread. No SharedArrayBuffer needed.

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
        // Report state every ~19 blocks ≈ 50 ms at 44100 Hz / 128 frames per block.
        this._STATE_INTERVAL = 19;
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
        const statePtr = wasm.wasm_state_ptr();
        // fx_playback_state: 7 × uint32_t, naturally 4-byte aligned.
        const state = new Uint32Array(wasm.memory.buffer, statePtr, 7);
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

        const leftOut    = outputs[0][0];
        const rightOut   = outputs[0][1];
        const frameCount = leftOut.length;   // always 128

        const rendered = this._wasm.wasm_render(frameCount);

        if (rendered > 0)
        {
            const bufPtr = this._wasm.wasm_render_buf();
            // S16 interleaved stereo; convert to float32 in [-1, 1].
            const s16    = new Int16Array(this._wasm.memory.buffer, bufPtr, rendered * 2);
            const scale  = 1.0 / 32768.0;
            for (let frame = 0; frame < rendered; frame++)
            {
                leftOut[frame]  = s16[frame * 2]     * scale;
                rightOut[frame] = s16[frame * 2 + 1] * scale;
            }
        }

        // Song ended (engine returned fewer frames than requested).
        if (rendered < frameCount)
        {
            this._playing = false;
            this.port.postMessage({ type: 'ended' });
        }

        // Periodic state report.
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
