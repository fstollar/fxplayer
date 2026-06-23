// fx-main.js — Main thread controller for the F/X web demo.

class FxPlayer
{
    constructor()
    {
        this._ctx          = null;
        this._node         = null;
        this._readyResolve = null;
        this._readyReject  = null;
    }

    // Must be called from a user gesture (browser AudioContext policy).
    async init()
    {
        this._ctx = new AudioContext();

        await this._ctx.audioWorklet.addModule('fx-worklet.js');

        // outputChannelCount: [2] ensures stereo output from the worklet.
        this._node = new AudioWorkletNode(this._ctx, 'fx-worklet', {
            outputChannelCount: [2],
        });
        this._node.connect(this._ctx.destination);

        // Use addEventListener + explicit .start() — required for message delivery
        // on some browsers (onmessage alone is insufficient for AudioWorkletNode.port).
        this._node.port.addEventListener('message', (event) => this._handleMessage(event.data));
        this._node.port.start();

        // Fetch Wasm bytes and send to worklet for compilation.
        const wasmResp  = await fetch('fxcore.wasm');
        const wasmBytes = await wasmResp.arrayBuffer();

        await new Promise((resolve, reject) =>
        {
            this._readyResolve = resolve;
            this._readyReject  = reject;
            // Transfer the ArrayBuffer — the worklet now owns it.
            this._node.port.postMessage({ type: 'init', wasmBytes }, [wasmBytes]);
        });

        // Resume AudioContext — browsers may suspend it after async init work
        // even when the context was created inside a user-gesture handler.
        await this._ctx.resume();

        // Auto-load the bundled module.
        const modResp  = await fetch('modules/64mania.s3m');
        const modBytes = await modResp.arrayBuffer();
        this.loadModule(modBytes);
    }

    _handleMessage(msg)
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
                document.getElementById('fx-title').textContent  = msg.title || '(untitled)';
                document.getElementById('fx-status').textContent = 'Playing';
                break;
            case 'state':
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
                document.getElementById('fx-status').textContent = 'Ended';
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

    // Load a module from an ArrayBuffer (file picker or drag-drop).
    loadModule(arrayBuffer)
    {
        // Transfer the buffer — the worklet now owns it.
        this._node.port.postMessage({ type: 'load', moduleBytes: arrayBuffer }, [arrayBuffer]);
        document.getElementById('fx-status').textContent = 'Loading…';
    }

    // Resume AudioContext on Play in case it got suspended (e.g. tab backgrounded).
    play()
    {
        if (this._ctx) this._ctx.resume();
        this._node.port.postMessage({ type: 'play' });
    }

    pause()          { this._node.port.postMessage({ type: 'pause' }); }
    stop()           { this._node.port.postMessage({ type: 'stop' }); }
    setVolume(vol)   { this._node.port.postMessage({ type: 'volume', value: vol }); }
    orderJump(delta) { this._node.port.postMessage({ type: 'order', delta }); }
}
