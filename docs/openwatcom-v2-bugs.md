# Open Watcom V2 — Observed Bugs (for GitHub issue reporting)

> Found during: cross-compilation of FX Player (1998 DOS/32-bit pmodew project)  
> OW V2 build: `Current-build` tag, June 19 2026  
> Host: Linux x86-64  
> Target: DOS 32-bit flat model, pmodew extender

---

## Bug 1: `@@` local labels not recognized in wasm TASM mode (`-zcm=tasm`)

**Tool:** `wasm`  
**Severity:** Blocker — any TASM source using `@@` local labels cannot assemble  

### Reproduction

```asm
.486p
.model flat
LOCALS

.code
testproc proc
    xor ecx, ecx
@@loop:
    inc ecx
    cmp ecx, 10
    jnz @@loop
    ret
endp
END
```

```
wasm -zcm=tasm -ml -5 -mf test.asm
```

**Expected:** assembles cleanly — `@@loop` is a standard TASM local label syntax, enabled by the `LOCALS` directive.  
**Actual:** `Error! E251: Symbol @@loop is not defined` on the `jnz @@loop` reference.

### Notes

- Scope: `@@` labels should be proc-local, resetting at each new non-`@@` label, exactly as documented in the TASM manual.
- The `LOCALS` directive is parsed without error (no complaint about an unknown directive), but the `@@` label mechanism does not function.
- Workaround: rename `@@` labels to unique proc-scoped names (e.g. `procname_loop:`).
- The `-zcm=ideal` mode was not tested.

---

## Bug 2: Compiler flag `-wcd=<n>` silently accepts and then rejects message numbers at compile time

**Tool:** `wpp386` (and presumably `wcc386`)  
**Severity:** Minor / confusing — no error at invocation, but compile-time `E557` breaks the build

### Reproduction

```
wpp386 -wcd=737 source.cpp
```

**Expected:** either silently ignores an unknown diagnostic number (like GCC), or rejects it immediately at startup.  
**Actual:** The flag is accepted without complaint, but during compilation the compiler emits:
```
Error! E557: message number '737' is invalid
```
This causes the compile to fail even if the source file itself has no errors. The `-wce=<n>` variant presumably has the same issue.

### Impact

Build scripts that attempt to suppress diagnostics by number can silently break compilation of all files that use the flag.

---

## Bug 3: C++ name mangling case inconsistency between fresh-compiled objects and pre-built libraries (informational)

**Tool:** `wpp386` + `wlink`  
**Severity:** Informational — not a wasm bug, but worth noting for context

### Observation

- Objects compiled with `wpp386` (OW V2) generate **uppercase** Watcom C++ mangled symbols (e.g. `W?$NAN(UI)PNV` for `operator new[]`).
- Pre-compiled OBJ files produced by older Watcom toolchains (OW 1.x) may contain **lowercase** mangled symbols (e.g. `W?$nan(ui)pnv`).
- `wlink` with `option caseexact` performs case-sensitive symbol matching, so these are treated as different symbols → unresolved references.
- Workaround: recompile all OBJ files with OW V2 (do not mix old and new OBJ files), and avoid `option caseexact` if mixing is unavoidable.

This is not a bug in OW V2 per se — it is expected that name mangling may differ across compiler versions — but it is a breaking silent failure mode when migrating legacy projects.

---

## Bug 4: `model flat,prolog` directive rejected in wasm TASM mode

**Tool:** `wasm`  
**Severity:** Minor blocker — `model flat,prolog` is valid TASM syntax

### Reproduction

```asm
.486p
model flat,prolog
```

```
wasm -zcm=tasm -ml -5 -mf test.asm
```

**Expected:** `model flat,prolog` accepted — in TASM, the optional second argument to `MODEL` sets the prologue type (`prolog` = standard stack frame prologue for `PROC` bodies).  
**Actual:** `Error! E022: Invalid label definition` on the `model flat,prolog` line.

### Notes

- Workaround: remove the `,prolog` qualifier — `model flat` assembles cleanly.  
- The TASM `prolog` option enabled automatic `push ebp / mov ebp, esp / pop ebp` prologues. Without it, procedures must manage their own stack frames. For flat-model 32-bit code that doesn't use `ENTER`/`LEAVE`, this makes no functional difference.

