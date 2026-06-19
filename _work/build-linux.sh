#!/usr/bin/env bash
# Build FX Player original source with Open Watcom V2 on Linux.
# Produces DOS 32-bit EXE (PMode/W) -- run with DOSBox.

OW2=/home/fst/_claude/fxplayer/_watcom2/ow2
export WATCOM=$OW2
export PATH=$OW2/binl64:$PATH
LIBDOS=$OW2/lib386/dos

WPP="wpp386"
WASM="wasm"
WLINK="wlink"

# Compiler flags -- use '-' prefix (Linux wpp386 doesn't accept '/')
# -i=. = current dir (local headers)  -i=$OW2/h = OW system headers
CFLAGS="-oneatx -ob -oh -oi+ -ol+ -ei -zp8 -5r -fp5 -fpi87 -mf -s -d0 -i=. -i=$OW2/h"

# Assembler: TASM compat, case-sensitive labels, Pentium, flat model
AFLAGS="-zcm=tasm -ml -5 -mf -zq"

WORK=$(dirname "$(realpath "$0")")
cd "$WORK"

CPP_FILES=(
    DEVICE.CPP DEV_SB.CPP DEV_WSS.CPP
    CONVERT.CPP AMPTABLE.CPP MIXER.CPP
    FORMAT.CPP DAT_CALC.CPP
    DAT_WAV.CPP
    DAT_S3M.CPP EFC_S3M.CPP
    DAT_669.CPP EFC_669.CPP
    DAT_MOD.CPP EFC_MOD.CPP
    DMA.CPP IRQ.CPP
    COMMAND.CPP KEYCTRL.CPP
    FX.CPP
)

ASM_FILES=(MIXR_16N.ASM MIXR_32N.ASM MIXR_32I.ASM)

ERRORS=0

echo "=== Compiling C++ files ==="
for f in "${CPP_FILES[@]}"; do
    printf "  %-20s" "$f"
    OUT=$($WPP $CFLAGS "$f" -fo="${f%.CPP}.OBJ" 2>&1)
    NERR=$(echo "$OUT" | grep -c ": Error!" || true)
    NWRN=$(echo "$OUT" | grep -c ": Warning!" || true)
    echo " errors=$NERR warnings=$NWRN"
    if [ "$NERR" -gt 0 ]; then
        echo "$OUT" | grep ": Error!" | head -5
        ERRORS=$((ERRORS+1))
    fi
done

echo ""
echo "=== Assembling ASM files ==="
for f in "${ASM_FILES[@]}"; do
    printf "  %-20s" "$f"
    OUT=$($WASM $AFLAGS "$f" 2>&1)
    NERR=$(echo "$OUT" | grep -c "Error!" || true)
    echo " errors=$NERR"
    if [ "$NERR" -gt 0 ]; then
        echo "$OUT" | grep "Error!" | head -5
        ERRORS=$((ERRORS+1))
    fi
done

echo ""
echo "=== Linking ==="
OBJS=""
for f in "${CPP_FILES[@]}"; do OBJS="$OBJS ${f%.CPP}.OBJ"; done
for f in "${ASM_FILES[@]}"; do OBJS="$OBJS ${f%.ASM}.OBJ"; done

LINK_OUT=$(wlink system pmodew \
    library "$LIBDOS/plib3r.lib" \
    name FX.EXE \
    file { $OBJS } \
    option map=FX.MAP 2>&1)
echo "$LINK_OUT" | grep -v "^Open Watcom\|^Version\|^Copyright\|^Portions\|^Source\|^See"
echo "$LINK_OUT" | grep -q "Error!" && ERRORS=$((ERRORS+1)) || true

echo ""
if [ $ERRORS -eq 0 ]; then
    echo "=== SUCCESS ==="
    ls -lh FX.EXE 2>/dev/null
else
    echo "=== DONE with $ERRORS step(s) having errors ==="
fi
