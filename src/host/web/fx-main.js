// fx-main.js — Main thread controller for the F/X web demo.
//
// Orchestrates three components:
//   fx-worker.js    — Web Worker; owns WASM; pre-renders audio chunks
//   fx-worklet.js   — AudioWorklet; drains the chunk queue; no WASM
//   MessageChannel  — direct Worker→Worklet audio pipe; bypasses main thread

class FxPlayer
{
    constructor()
    {
        this._ctx             = null;
        this._node            = null;
        this._worker          = null;
        this._readyResolve    = null;
        this._readyReject     = null;
        this._pendingFilename = '';   // fallback when the module has no embedded title
    }

    // Must be called from a user gesture (browser AudioContext policy).
    async init()
    {
        // 'playback' gives Chrome a larger internal buffer — critical on Android
        // where the tighter 'interactive' default causes GC-induced underruns.
        this._ctx = new AudioContext({ latencyHint: 'playback' });

        await this._ctx.audioWorklet.addModule('fx-worklet.js');

        // outputChannelCount: [2] ensures stereo output from the worklet.
        this._node = new AudioWorkletNode(this._ctx, 'fx-worklet', {
            outputChannelCount: [2],
        });
        this._node.connect(this._ctx.destination);
        this._node.port.addEventListener('message', (event) => this._handleWorkletMessage(event.data));
        this._node.port.start();

        // Spawn the Worker that will own WASM and pre-render audio.
        this._worker = new Worker('fx-worker.js');
        this._worker.onmessage = (event) => this._handleWorkerMessage(event.data);

        // Wire a direct MessageChannel between Worker and AudioWorklet so audio
        // chunks bypass the main thread entirely.
        const mc = new MessageChannel();
        this._node.port.postMessage({ type: 'audioPort', port: mc.port2 }, [mc.port2]);
        this._worker.postMessage({ type: 'audioPort', port: mc.port1 }, [mc.port1]);

        // Fetch Wasm and send to Worker for compilation.
        const wasmResp  = await fetch('fxcore.wasm');
        const wasmBytes = await wasmResp.arrayBuffer();

        await new Promise((resolve, reject) =>
        {
            this._readyResolve = resolve;
            this._readyReject  = reject;
            this._worker.postMessage({ type: 'init', wasmBytes }, [wasmBytes]);
        });

        // Resume AudioContext — browsers may suspend it after async init work.
        await this._ctx.resume();

        // Auto-load whichever module is selected in the dropdown.
        const selectedUrl = document.getElementById('module-select').value;
        await this.loadFromUrl(selectedUrl);
    }

    // Messages from the Worker (state, events, errors).
    _handleWorkerMessage(msg)
    {
        switch (msg.type)
        {
            case 'ready':
                if (this._readyResolve)
                {
                    this._readyResolve();
                    this._readyResolve = null;
                    this._readyReject  = null;
                }
                break;
            case 'loaded':
            {
                // Use the embedded module title; fall back to the filename (minus
                // path and extension) when the module has no title.
                const display = msg.title ||
                    this._pendingFilename.replace(/^.*[\\/]/, '').replace(/\.[^.]+$/, '');
                document.getElementById('fx-title').textContent = `Song: ${display}`;
            }
                document.getElementById('fx-status').textContent      = 'Playing';
                document.getElementById('btn-playpause').textContent  = 'Pause';
                break;
            case 'playing':
                document.getElementById('fx-status').textContent      = 'Playing';
                document.getElementById('btn-playpause').textContent  = 'Pause';
                break;
            case 'paused':
                document.getElementById('fx-status').textContent      = 'Paused';
                document.getElementById('btn-playpause').textContent  = 'Play';
                break;
            case 'stopped':
                document.getElementById('fx-status').textContent      = 'Stopped';
                document.getElementById('btn-playpause').textContent  = 'Play';
                break;
            case 'state':
                // State synced to playback arrives via _handleWorkletMessage instead.
                // Worker state messages still fire on pause/stop/ended for a final update.
                document.getElementById('fx-order').textContent    =
                    `${msg.order + 1} / ${msg.order_count}`;
                document.getElementById('fx-pattern').textContent  = msg.pattern;
                document.getElementById('fx-row').textContent      =
                    `${msg.row + 1} / ${msg.row_count}`;
                document.getElementById('fx-channels').textContent =
                    `${msg.channels_active} / ${msg.channels}`;
                document.getElementById('fx-loops').textContent    = msg.loops;
                break;
            case 'ended':
                document.getElementById('fx-status').textContent     = 'Ended';
                document.getElementById('btn-playpause').textContent = 'Play';
                break;
            case 'error':
                if (this._readyReject)
                {
                    this._readyReject(new Error(msg.message));
                    this._readyResolve = null;
                    this._readyReject  = null;
                }
                else
                {
                    document.getElementById('fx-status').textContent = `Error: ${msg.message}`;
                }
                break;
        }
    }

    // State updates arrive here from the AudioWorklet, synced to playback position.
    _handleWorkletMessage(msg)
    {
        if (msg.type !== 'state') return;
        document.getElementById('fx-order').textContent    =
            `${msg.order + 1} / ${msg.order_count}`;
        document.getElementById('fx-pattern').textContent  = msg.pattern;
        document.getElementById('fx-row').textContent      =
            `${msg.row + 1} / ${msg.row_count}`;
        document.getElementById('fx-channels').textContent =
            `${msg.channels_active} / ${msg.channels}`;
        document.getElementById('fx-loops').textContent    = msg.loops;
    }

    async loadFromUrl(url)
    {
        document.getElementById('fx-status').textContent = 'Loading…';
        const resp = await fetch(url);
        if (!resp.ok)
        {
            document.getElementById('fx-status').textContent =
                `Error: ${url} — HTTP ${resp.status}`;
            return;
        }
        const bytes = await resp.arrayBuffer();
        // Extract the bare filename from the URL path for use as title fallback.
        const filename = url.split('/').pop() || url;
        this.loadModule(bytes, filename);
    }

    // Load a module from an ArrayBuffer (file picker or drag-and-drop).
    // filename is used as the display title when the module has no embedded title.
    loadModule(arrayBuffer, filename = '')
    {
        this._pendingFilename = filename;
        document.getElementById('fx-status').textContent = 'Loading…';
        // Transfer the buffer to the Worker — it owns WASM and does the load.
        this._worker.postMessage(
            { type: 'load', moduleBytes: arrayBuffer, sampleRate: this._ctx.sampleRate },
            [arrayBuffer]);
    }

    // Toggle play/pause; also resumes AudioContext in case it was suspended.
    togglePlay()
    {
        if (this._ctx) this._ctx.resume();
        this._worker.postMessage({ type: 'toggle' });
    }

    stop()           { this._worker.postMessage({ type: 'stop' }); }
    setVolume(vol)   { this._worker.postMessage({ type: 'volume', value: vol }); }
    orderJump(delta) { this._worker.postMessage({ type: 'order', delta }); }
}
