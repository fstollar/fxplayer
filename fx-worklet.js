// fx-worklet.js — AudioWorkletProcessor for the F/X module player.
//
// Thin audio sink only — no WASM, no computation.
// fx-worker.js (DedicatedWorkerGlobalScope) owns the engine and pre-renders
// Float32 stereo chunks.  This processor drains a queue of those chunks,
// 128 frames at a time, with zero allocations in process().

class FxWorkletProcessor extends AudioWorkletProcessor
{
    constructor(options)
    {
        super(options);

        // Queue of pending audio chunks from the Worker.
        // Each entry: { buf: Float32Array (stereo interleaved), frames, read }
        this._queue       = [];
        this._queueFrames = 0;

        // Hard cap to prevent unbounded memory growth on a stalled main thread.
        this._MAX_QUEUE_FRAMES = 96000;   // 2 s at 48 kHz

        this.port.addEventListener('message', (event) => this._handleMessage(event.data));
        this.port.start();
    }

    _handleMessage(msg)
    {
        switch (msg.type)
        {
            case 'audioPort':
                // MessageChannel port from the Worker — receives audio chunks directly,
                // bypassing the main thread for lower latency.
                this._audioPort = msg.port;
                this._audioPort.addEventListener('message', (event) => this._onChunk(event.data));
                this._audioPort.start();
                break;

            case 'flush':
                // New module loaded or playback stopped — discard buffered audio.
                this._queue       = [];
                this._queueFrames = 0;
                break;
        }
    }

    _onChunk(msg)
    {
        if (msg.type !== 'chunk') return;

        // Drop incoming chunks if we already have 2 seconds buffered (Worker too fast).
        if (this._queueFrames >= this._MAX_QUEUE_FRAMES) return;

        this._queue.push({
            buf:    new Float32Array(msg.buffer),
            frames: msg.frames,
            read:   0,
        });
        this._queueFrames += msg.frames;
    }

    process(inputs, outputs)
    {
        const leftOut  = outputs[0][0];
        const rightOut = outputs[0][1];
        const QUANTUM  = 128;   // AudioWorklet render quantum; always 128
        let written    = 0;

        // Drain queue into the output buffers, consuming exactly QUANTUM frames.
        while (written < QUANTUM && this._queue.length > 0)
        {
            const front  = this._queue[0];
            const avail  = front.frames - front.read;
            const toRead = Math.min(QUANTUM - written, avail);
            const base   = front.read;
            const buf    = front.buf;

            for (let frame = 0; frame < toRead; frame++)
            {
                leftOut[written + frame]  = buf[(base + frame) * 2];
                rightOut[written + frame] = buf[(base + frame) * 2 + 1];
            }

            front.read        += toRead;
            written           += toRead;
            this._queueFrames -= toRead;

            if (front.read >= front.frames)
                this._queue.shift();
        }

        // Any remaining output frames are already zero-filled by the browser.
        return true;
    }
}

registerProcessor('fx-worklet', FxWorkletProcessor);
