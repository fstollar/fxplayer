# MOD Format Variants — Magic Tags and Detection

All MOD variants share the `.mod` extension. Detection depends on the 4-byte tag at offset 0x0438 in 31-sample files, or its absence in 15-sample files.

## Detection algorithm

```
1. Seek to offset 0x0438
2. Read 4 bytes
3. If any byte < 0x20 or > 0x7E → 15-sample SoundTracker (no tag)
4. Otherwise → match against the table below
```

If the tag is not recognized, default to 4-channel playback and treat as ProTracker.

## Known magic tags

| Tag | Channels | Notes |
|-----|----------|-------|
| *(no tag, 15-sample)* | 4 | Ultimate SoundTracker / early SoundTracker clones |
| `M.K.` | 4 | ProTracker standard (≤ 64 patterns). The tag was added by Unknown/D.O.C., not Mahoney & Kaktus, and stands for their initials. |
| `M!K!` | 4 | ProTracker (> 64 patterns). Pattern indices in order list can reach 127. |
| `M&K!` | 4 | Unknown tracker ("fleg's module train-er"). Only one known file: `echobea3.mod`. Load as standard 4-channel. |
| `FLT4` | 4 | StarTrekker 4-channel. Standard layout. |
| `FLT8` | 8 | StarTrekker 8-channel. **Special:** load like 4-channel, but play order[N] and order[N]+1 simultaneously (merge two 4-ch patterns into one 8-ch row). |
| `2CHN` | 2 | FastTracker (PC). Standard layout, 2 channels. |
| `6CHN` | 6 | 6-channel. Common PC extension. |
| `8CHN` | 8 | 8-channel. Common PC extension. |
| `10CH`–`32CH` | 10–32 | PC multi-channel. Tag is `xxCH` where xx is a **decimal** number. Even numbers only (FastTracker constraint). |
| `xxCN` | varies | TakeTracker multi-channel variant; xx is decimal. |
| `TDZ1` | 1 | TakeTracker 1-channel. |
| `TDZ2` | 2 | TakeTracker 2-channel. |
| `TDZ3` | 3 | TakeTracker 3-channel. |
| `5CHN` | 5 | TakeTracker 5-channel. |
| `7CHN` | 7 | TakeTracker 7-channel. |
| `9CHN` | 9 | TakeTracker 9-channel. |
| `CD81` | 8 | Oktalyzer (Atari ST). |
| `OKTA` | 8 | Oktalyzer (Atari ST). |
| `OCTA` | 8 | OctaMED (Amiga 8-channel). |

### Parsing `xxCH` and `xxCN` tags

```c
if (tag[2] == 'C' && tag[3] == 'H') {
    int channels = (tag[0] - '0') * 10 + (tag[1] - '0');
    /* channels: 10, 12, 14, ..., 32 */
}
```

FLT8 pattern interleaving:
```c
/* Order list entry N: play patterns N and N+1 merged */
for (row = 0; row < 64; row++) {
    read_4ch_row(pattern[order[pos]],   row, channels 0-3);
    read_4ch_row(pattern[order[pos]+1], row, channels 4-7);
}
```

## Restart byte at offset 0x0197

| Value | Tracker | Interpretation |
|-------|---------|----------------|
| `0x78` (120) | SoundTracker (original) | Traditional placeholder; ignore |
| `0x7F` (127) | ProTracker | Placeholder; ignore |
| Other | NoiseTracker, FastTracker | Song restart position (order index to loop back to at song end) |

## 15-sample vs 31-sample layout difference

15-sample files:
- 15 sample descriptors instead of 31 → header is 450 bytes shorter
- No magic tag field
- Maximum 64 patterns
- Order list entries cap at 63

The `song_positions` byte at 0x0196 (31-sample) or 0x01C6 (15-sample) is the sole indicator of song length; scan the full 128-byte order table regardless to find the true maximum pattern index.

## References

- [MultimediaWiki — file format and all tags](https://wiki.multimedia.cx/index.php/Protracker_Module#File_Format)
- [aes.id.au — M.K. origin note](https://aes.id.au/modformat.html)
