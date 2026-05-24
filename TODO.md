# TODO

Lavoro residuo dopo PR1-PR5 (piano `typed-crunching-eagle.md` completato) e dopo i feat
successivi (sprite + window, joypad + bindings, HALT bug, APU base, gfx backend
abstraction).

---

## 1. Debugger — stub minori

Lasciati indietro da PR5; non bloccano nulla ma sono evidenti nel pannello CPU.

- **Reset button** (pannello CPU) — oggi è disabilitato. Deve fare re-init del
  `core` + clear RAM/VRAM. Il commento nel codice cita "PR5: re-init core + clear
  RAM/VRAM" ma non era nello scope ufficiale di PR5.
- **Step Over** (pannello CPU) — attualmente alias di Step. Per uno step-over
  vero serve `disasm_one` che restituisca la lunghezza dell'istruzione corrente,
  così da settare un breakpoint temporaneo a `PC + len` quando si è su
  `CALL` / `RST`.

---

## 2. Save & Load State

Feature richiesta da utente finale, autonoma rispetto al resto.

Approccio: serializzare lo stato di `registers`, `mmu` (tutte le regioni RAM +
mmio + flag `bios_accessible`), `cpu` (flag `halted`/`stopped`/`halt_bug`/
`interrupt_enabled`/`ime_pending`/`ei_just_executed`/`extra_cycles`), `ppu`
(framebuffer, `dot_counter`, contatori interni), `irq` e lo stato dell'MBC
corrente (current ROM/RAM bank, RAM-enable, mode bit). Formato: blob binario
versionato (`magic + version + sezioni`). Slot multipli + hotkey F5/F9.

---

## 3. Shader effetto green-LCD

Riferimento: <https://github.com/libretro/glsl-shaders/blob/master/handheld/gameboy.glslp>

Catena 5-pass libretro/RetroArch + 2 lookup texture (`COLOR_PALETTE`,
`BACKGROUND`).

**Pre-requisito**: il backend `gfx` (`inc/gfx.h`) è già astratto con l'intento
esplicito di accogliere un backend OpenGL3. Va aggiunto a fianco di
`sdl_backend_create` un `gl_backend_create` + ImGui backend `opengl3` al posto
di `sdlrenderer2`. Tutto il resto della UI (presenter, `imgui_id`) non cambia.

**Lavoro:**
- Parser `.glslp` minimale (chiavi: `shaders=N`, `shaderN`, `filter_linearN`,
  `scale_typeN`, `scaleN`, `aliasN`, `textures`, `<NAME>`, `<NAME>_linear`).
  ~150 righe. `#pragma parameter` ignorabile per la v1 (i default sono già
  nel ramo `#else` di ogni shader).
- Pipeline multi-pass con FBO ping-pong dimensionati per `scale_type`
  (`viewport` / `source` / `absolute`).
- **Frame history ring**: il pass 0 campiona `PrevTexture` … `Prev6Texture` —
  servono 7 copie storiche del primo input (simulazione ghosting LCD), ruotate
  ogni frame. È la parte non-ovvia: senza, lo shader sembra rotto (immagine
  flat senza scia).
- Cross-pass sampling: pass 4 campiona `Pass2Texture`. Trivial una volta che
  ogni pass ha il suo FBO.
- Lookup texture (`palette.png`, `background.png`) via `stb_image`.
- Uniform standard libretro: `MVPMatrix`, `FrameCount`, `FrameDirection`,
  `OutputSize`, `TextureSize`, `InputSize`, `Texture`. Compilare ogni file
  due volte con `-DVERTEX` / `-DFRAGMENT`.

**Stima**: 4-6 giorni. Il 70% del costo è lo switch a OpenGL; il resto è
meccanico ma con dettagli sensibili (rotazione del history ring, propagazione
del canale alpha attraverso i pass 1-3, Y-flip delle texcoord).

---

## 4. MBC2 / MBC3 / MBC5

Oggi `src/mbc.cpp` implementa solo `no_mbc` (56) e `mbc1` (116). Test ROM oltre
`cpu_instrs.gb` (es. Pokémon Red/Blue → MBC3, Pokémon Crystal → MBC3+RTC,
giochi tardi → MBC5) richiedono questi banchi.

Priorità:
1. **MBC3** — copre la maggior parte dei giochi più importanti. RTC opzionale
   alla prima iterazione (la maggior parte dei test funziona senza).
2. **MBC5** — banco lineare fino a 8 MB, semplice.
3. **MBC2** — meno comune, RAM integrata 512×4 bit.

---

## 5. Refactor: sub-instruction (M-cycle) timing

Oggi `core::step()` (`src/core.cpp:50-74`) esegue l'intera istruzione via
`cpu.step()` e *poi* avanza PPU/APU/timer del totale dei T-cycle. Modello
troppo grossolano per i Blargg `mem_timing` / `mem_timing-2`, che misurano
su quale M-cycle interno di un'istruzione cade un read/write.

**Sintomo**: `mem_timing.gb` fallisce perché TIMA viene incrementato in blocco
*dopo* l'istruzione, invece di poter incrementare fra un M-cycle e l'altro
(es. tra l'opcode fetch e il memory read di `LD A,(HL)`). I test che dipendono
solo dalle durate **totali** (`instr_timing.gb`) dovrebbero già passare.

**Strada (PR a sé, alta densità di cambiamenti)**:

- Esporre `core::tick(uint8_t m_cycles)` che avanza timer/PPU/APU di
  `m_cycles * 4` T-cycle.
- Modificare `mmu::read_u8`/`write_u8` (o wrapper sul lato CPU) in modo che
  ogni accesso emetta prima un `core::tick(...)` proporzionale agli M-cycle
  accumulati dall'ultimo accesso.
- Spezzare il body di ogni opcode in `inc/opcodes.hpp` / `src/opcodes.cpp`
  così che le fasi (fetch operand, read, ALU, write, internal) emettano tick
  negli M-cycle corretti. `cpu::extra_cycles` come somma postuma diventa
  obsoleto.
- Aggiornare `irq::dispatch()` per spalmare i suoi 5 M-cycle in tick discreti
  (2 wait, push hi, push lo, jump).
- Non-regressione: `cpu_instrs.gb`, `instr_timing.gb` restano verdi;
  `mem_timing.gb` / `mem_timing-2.gb` diventano il nuovo target.

**Test Blargg da marcare "skip" finché il refactor non c'è:**
- `interrupt_time/interrupt_time.gb` — `REQUIRE_CGB 1` (CGB-only, indipendente
  dal refactor).
- `mem_timing/mem_timing.gb` + `01-read_timing.gb` / `02-write_timing.gb` /
  `03-modify_timing.gb`.
- `mem_timing-2/mem_timing-2.gb`.
- `oam_bug/oam_bug.gb` + sotto-ROM `1-lcd_sync.gb` … `8-instr_effect.gb`.
  Verifica la corruzione OAM DMG durante incrementi 16-bit di register pair
  con valore in `$FE00-$FEFF` nei primi 20 cicli di una scanline visibile —
  dipende dal modello M-cycle del refactor e da un PPU che esponga la finestra
  di mode-2 OAM scan al ciclo esatto. Nessun gioco commerciale dipende da
  questo quirk: "nice-to-have" da affrontare *dopo* il refactor M-cycle.

---

## 6. Release pipeline

Tutto da fare; vanno fatti in quest'ordine perché ognuno dipende dal precedente.

- **Versioning** — schema (SemVer? `0.x` finché instabile?), file `VERSION` o
  define in CMake, esposto in `--version` e nella titlebar.
- **Release notes** — `CHANGELOG.md` Keep-a-Changelog; convertito in body
  Release da CI.
- **Packaging** — portable zip Windows: exe + `cfg/gbemu.conf` con path
  *relativi* (oggi sono assoluti hardcoded — va sistemato a monte), DLL SDL2
  raccolte da vcpkg, README. Niente installer per ora.
- **CI** — GitHub Action: matrice (Debug + Release), configure via vcpkg
  manifest, build, smoke test headless (`--script scripts/dbg_smoke.dbg`).
  Su tag `v*` → packaging step → upload come asset di GitHub Release.
- **Linux support** — build CMake + vcpkg deve già funzionare; verificare
  e aggiungere a matrice CI. Packaging: AppImage o tar.gz portable.
  Distinguere SDL2 di sistema vs vendored.

---

## Note tecniche permanenti

- `cfg/gbemu.conf` shipped ha path **assoluti** Windows per ROM e BIOS — va
  reso relativo prima del packaging.
- `CMakeUserPresets.json` pinna `VCPKG_ROOT` locale: tenere fuori dai workflow
  cross-machine (già menzionato in `CLAUDE.md`).
- `inc/opcodes.hpp` è generato — modificare solo `scripts/gen_ops.py` o
  rigenerare; `src/opcodes.cpp` invece è hand-maintained (la stringa
  "Generated from ops_db.json" in cima è stale).
