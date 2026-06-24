#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fx/fx.h"

static void write_wav_header(FILE *f, uint32_t sample_rate,
                             uint16_t channels, uint32_t n_frames)
{
    uint32_t data_bytes  = n_frames * channels * 2u;
    uint32_t riff_size   = 36u + data_bytes;
    uint16_t bits        = 16;
    uint16_t fmt_pcm     = 1;
    uint32_t byte_rate   = sample_rate * channels * 2u;
    uint16_t block_align = (uint16_t)(channels * 2u);
    uint32_t fmt_size    = 16;

    fwrite("RIFF",        1, 4, f);
    fwrite(&riff_size,    4, 1, f);
    fwrite("WAVEfmt ",    1, 8, f);
    fwrite(&fmt_size,     4, 1, f);
    fwrite(&fmt_pcm,      2, 1, f);
    fwrite(&channels,     2, 1, f);
    fwrite(&sample_rate,  4, 1, f);
    fwrite(&byte_rate,    4, 1, f);
    fwrite(&block_align,  2, 1, f);
    fwrite(&bits,         2, 1, f);
    fwrite("data",        1, 4, f);
    fwrite(&data_bytes,   4, 1, f);
}

int main(int argc, char **argv)
{
    FILE    *fin, *fout;
    long     file_size;
    uint8_t *module_data;
    uint8_t *workspace;
    size_t   ws_bytes;
    fx_err   err;

    static const fx_config cfg = {
        48000, 2, FX_OUTPUT_S16, 1, 1
    };

#define CHUNK_FRAMES 1024
    static int16_t pcm_buf[CHUNK_FRAMES * 2];

    int16_t *all_pcm   = NULL;
    size_t   all_cap   = 0;
    size_t   all_used  = 0;
    size_t   rendered;
    uint32_t total_frames = 0;
    uint32_t max_frames   = 0;  /* 0 = no limit */

    if (argc < 3) {
        fprintf(stderr, "Usage: render <input.mod|.s3m|.669> <output.wav> [max_frames]\n");
        return 1;
    }
    if (argc >= 4) max_frames = (uint32_t)atol(argv[3]);

    fin = fopen(argv[1], "rb");
    if (!fin) { perror(argv[1]); return 1; }
    fseek(fin, 0, SEEK_END);
    file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    module_data = (uint8_t *)malloc((size_t)file_size);
    if (!module_data) { fputs("OOM\n", stderr); return 1; }
    if (fread(module_data, 1, (size_t)file_size, fin) != (size_t)file_size) {
        fputs("read error\n", stderr); return 1;
    }
    fclose(fin);

    err = fx_workspace_size(module_data, (size_t)file_size, &ws_bytes);
    if (err != FX_OK) {
        fprintf(stderr, "workspace_size error %d\n", (int)err); return 1;
    }
    workspace = (uint8_t *)malloc(ws_bytes);
    if (!workspace) { fputs("OOM workspace\n", stderr); return 1; }

    fx_init(&cfg);
    err = fx_load(module_data, (size_t)file_size, workspace, ws_bytes);
    if (err != FX_OK) {
        fprintf(stderr, "fx_load error %d\n", (int)err); return 1;
    }

    while ((rendered = fx_render_frames(pcm_buf, CHUNK_FRAMES, &cfg)) > 0) {
        size_t samples;
        if (max_frames > 0 && total_frames + (uint32_t)rendered > max_frames)
            rendered = max_frames - total_frames;
        if (rendered == 0) break;
        samples = rendered * cfg.channels;
        if (all_used + samples > all_cap) {
            all_cap = (all_cap == 0) ? (1u << 20) : all_cap * 2u;
            all_pcm = (int16_t *)realloc(all_pcm, all_cap * sizeof(int16_t));
            if (!all_pcm) { fputs("OOM pcm\n", stderr); return 1; }
        }
        memcpy(all_pcm + all_used, pcm_buf, samples * sizeof(int16_t));
        all_used      += samples;
        total_frames  += (uint32_t)rendered;
    }

    fout = fopen(argv[2], "wb");
    if (!fout) { perror(argv[2]); return 1; }
    write_wav_header(fout, cfg.sample_rate, cfg.channels, total_frames);
    fwrite(all_pcm, sizeof(int16_t), all_used, fout);
    fclose(fout);

    free(all_pcm);
    fx_close();
    free(workspace);
    free(module_data);

    fprintf(stderr, "Rendered %u frames -> %s\n", total_frames, argv[2]);
    return 0;
}
