# TODO

Lavoro residuo dopo PR1-PR5 (piano `typed-crunching-eagle.md` completato) e dopo i feat
successivi (sprite + window, joypad + bindings, HALT bug, APU base, gfx backend
abstraction).

---

## 1. Debugger — stub minori — **fatto**

Lasciati indietro da PR5; ora entrambi chiusi.

- **Reset button** — **fatto** (commit `c3173df`). `reset()` è esposto da
  `core`, `cpu`, `mmu`, `ppu`, `apu`, `timer`, `mbc` e `debugger`; chiamato dal
  pulsante Reset del pannello CPU, da `Emulation → Reset` e dalla hotkey
  `Ctrl+R`. Su reset MBC corrente, RAM/VRAM/OAM/HRAM, registri PPU e flag CPU
  tornano allo stato post-BIOS (o zero quando il BIOS è caricato).
- **Step Over** — **fatto** (`debugger::step_over()` in `src/debugger.cpp:36`).
  Usa `disasm_one` per ottenere la lunghezza dell'istruzione e imposta una
  `stop_condition{pc_eq, PC+len}` con cap a 1M T-cycle quando l'opcode è
  `CALL` (incl. cc: `$CD`/`$C4`/`$CC`/`$D4`/`$DC`) o `RST` (`(op & 0xC7) ==
  0xC7`). Per tutto il resto (HALT, JR, JP, RET) si riduce a un singolo
  `step()`. Wired al pulsante "Step Over" del pannello CPU (`src/ui.cpp:415`).

---

## 2. Save & Load State

Feature richiesta da utente finale, autonoma rispetto al resto.

Approccio: serializzare lo stato di `registers`, `mmu` (tutte le regioni RAM +
mmio + flag `bios_accessible`), `cpu` (flag `halted`/`stopped`/`halt_bug`/
`interrupt_enabled`/`ime_pending`/`ei_just_executed`/`extra_cycles`), `ppu`
(framebuffer, `dot_counter`, contatori interni), `irq` e lo stato dell'MBC
corrente (current ROM/RAM bank, RAM-enable, mode bit). Formato: blob binario
versionato (`magic + version + sezioni`). Slot multipli + hotkey F5/F9.

Rispettare la specifica BESS: https://github.com/LIJI32/SameBoy/blob/master/BESS.md

---

## 3. Moltiplicatore di velocità — **fatto**

Sei preset discreti (0.25 / 0.5 / 1.0 / 1.5 / 2.0 / 4.0) selezionabili da
`Emulation → Speed`, hotkey `+` / `-` per step e `0` per snap a 1.0x; il
moltiplicatore corrente è persistito in `<base>/user/user.conf` sotto un
nuovo sub-block `emulation.speed`. **Fast-forward** legato a `Tab` (held):
flag transitorio su `Application::fast_forward_active_`, non persistito
(uno shutdown con Tab "stuck" non boot-pa a 8x al riavvio).

**Pacing** (in `Application::run`):
- `speed > 1.0` → `budget = round(CYCLES_PER_FRAME * speed)` passato a
  `debugger.run_until` (più emulazione per frame vsynced reale).
- `speed < 1.0` → `budget = CYCLES_PER_FRAME` e `SDL_Delay((1/speed - 1) *
  16.667 ms)` post run_until (cap del millisecondo OK per i bucket esposti).
- `fast_forward_active_` → `SDL_RenderSetVSync(renderer, 0)` (toggle solo
  sull'edge), poi loop interno che esegue fino a 64 frame nominali o
  10 ms wall-clock per iterazione del main loop, quale dei due arriva
  prima.  Il "cap" effettivo è la throughput dell'host.

**Audio**: auto-mute quando `speed != 1.0 || fast_forward_active_`. Pattern
"due flag": `user_state_.audio_muted` (preferenza dell'utente, persistita) e
il flag derivato in `apply_effective_mute_(core)` che ORizza i due e pusha
nel APU.  Un Mute attivo a 2x rimane attivo dopo il ritorno a 1x.

**UI**:
- `Emulation → Speed` con 6 voci radio + voce informativa
  "Fast-forward (hold Tab)" disabilitata (Tab è catturato dal SDL event loop,
  non da ImGui, quindi cliccarla non avrebbe senso).
- Titlebar dinamica: `GbEmu` (a 1.0x, no FF), `GbEmu - 2.0x` (preset != 1),
  `GbEmu - FF` (fast-forward attivo).  Aggiornata in `apply_speed_state_`.

**Watcher di coerenza UI/main-loop**: il submenu Speed scrive direttamente
`user_state_.speed_multiplier`; il main loop confronta con
`last_applied_speed_` ogni iterazione e richiama `apply_speed_state_` se
diverso (one-frame-lag per il menu, zero-lag per le hotkey che già chiamano
il helper inline).

**Headless**: non toccato — `script_runner` gira sempre alla velocità
naturale, il branch fast-path nel main loop non si attiva.

**Out of scope (follow-up)**:
- Resampling audio a multiplier ≠ 1.0 (SoundTouch o simile).  Today il mute
  evita la sgradevole pitch-shift.
- Persistenza dello stato fast-forward su shutdown — deliberatamente
  *non* persistito.

---

## 4. Shader effetto green-LCD

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

## 5. MBC2 / MBC3 / MBC5 — **fatto**

`src/mbc.cpp` ora implementa `no_mbc`, `mbc1`, `mbc2`, `mbc3`, `mbc5`. Tutti i
cartridge type 0x00-0x1E mappati nel factory `make_mbc`. Pokémon Red/Blue
(type 0x13, MBC3+RAM+BATTERY) parte; il salvataggio in-gioco funziona ma resta
solo in RAM finché il processo è vivo (battery save → sezione 6).

**MBC2** (0x05, 0x06): 4-bit ROM bank, RAM built-in 512 nibbles mirrored, bit 8
dell'addr distingue RAM-enable vs ROM-select.

**MBC3** (0x0F-0x13): 7-bit ROM bank con 0→1 remap, RAM/RTC select a
$4000-$5FFF (banks 0-3 oppure RTC reg 0x08-0x0C), latch a $6000-$7FFF. **RTC
live**: `rtc_total_secs_` ancorato a `std::chrono::system_clock`, `catch_up()`
somma `(now - last_real_unix_)` nell'accumulatore quando non halted; 9-bit day
counter con wrap mod 512 e sticky carry su DH.7; halt su DH.6 freeza
l'accumulatore. Letture passano sempre per lo shadow latched ($6000-$7FFF
edge 0→1 lo aggiorna). Pokémon Gold/Silver/Crystal day/night cycle ora avanza
correttamente in real-time. **Persistenza fra restart del processo** → vedi
sezione 6 (lo stato live non viene serializzato oggi, quindi ogni avvio l'epoch
torna al system_clock corrente).

**MBC5** (0x19-0x1E): 9-bit ROM bank (low8 a $2000-$2FFF, bit9 a $3000-$3FFF,
bank 0 valido, no remap), 4-bit RAM bank a $4000-$5FFF. Bit di rumble
ignorato (no haptics).

`mbc_debug_state::rom_bank` allargato a `uint16_t` perché MBC5 può
selezionare fino a bank 511. UI / debugger `dump mbc` già castano a
`unsigned` quindi nessun consumer rotto.

Battery save + RTC persistence → chiusi in §6 (formato BESS).

---

## 6. Battery save (SRAM persistente su cartuccia) — **fatto**

Giochi come Zelda: Link's Awakening (MBC1+RAM+BATTERY, type `0x03`), Pokémon
(MBC3+RAM+BATTERY+RTC) e in generale tutti i titoli con salvataggio interno
scrivono il progresso in RAM esterna mantenuta da una batteria sulla cartuccia.
Implementato in formato **BESS** (Best Effort Save State, SameBoy spec:
<https://github.com/LIJI32/SameBoy/blob/master/BESS.md>) — un solo file `.sav`
per cartuccia contenente SRAM raw + footer BESS opzionale con blocco RTC.

**Layout `.sav`:**

```
[ SRAM raw, byte-per-byte ]                  ← prefix interop-compatibile
[ NAME block ]   id="NAME", payload "GbEmu vX.Y"
[ RTC  block ]   id="RTC ", payload 48 byte (solo MBC3+TIMER, type 0x0F/0x10)
[ END  block ]   id="END ", size = 0
[ u32 LE: offset al primo blocco ]
[ "BESS" 4 byte ASCII ]                      ← ultimi 8 byte del file
```

Il footer probe è all'estremità del file, quindi un emulatore che non
conosce BESS (BGB/mGBA/VBA-M) legge il `.sav` come SRAM raw e ignora la
coda. Analogamente noi accettiamo `.sav` di altri emulatori: se il footer
magic manca o l'offset è bogus, `bess::parse_sav` fa fallback a
"tutto-il-file-è-SRAM".

**File:**

- `inc/bess.h` + `src/bess.cpp` — modulo BESS minimale (NAME / RTC / END).
- API estesa su `inc/mbc.h`: `has_battery()`, `ram_data()`, `ram_load()`,
  `ram_dirty()`, `ram_clear_dirty()`, e per MBC3+TIMER: `rtc_blob() ->
  optional<array<uint8_t, 48>>`, `rtc_load_blob()`. Le ultime due tornano
  `nullopt` / no-op sugli altri MBC.
- `Application::load_rom_` (in `src/app.cpp`) calcola FNV1a sul ROM
  *prima* di `core::load()` (che fa `std::move(rf.data)` in `make_mbc`),
  risolve `<base>/<savs_dir>/<fnv1a>.sav`, fa parse_sav e applica
  `ram_load` + `rtc_load_blob`.
- `Application::flush_battery_save_` scrive il blob via `bess::write_sav`
  in modo atomico (`.sav.tmp` + `std::filesystem::rename`).
- Trigger flush:
  - ogni ~2 s nel main loop se `ram_dirty()` (gated su SDL_GetTicks64);
  - sempre a shutdown (anche se non dirty — sposta l'anchor `saved_unix`
    della RTC al momento di uscita);
  - in headless mode prima del return da `Application::run`;
  - prima di un hot-swap ROM (per scrivere il vecchio cart prima di
    sostituirlo).

**RTC blob (48 byte, layout BESS)**:

- 0x00, 0x04, 0x08, 0x0C, 0x10: S/M/H/DL/DH live (1 byte + 3 padding ciascuno).
- 0x14, 0x18, 0x1C, 0x20, 0x24: S/M/H/DL/DH latched.
- 0x28: int64 LE = `last_real_unix_` (timestamp UNIX al momento del dump).

Al load (`rtc_load_blob`): si ricostruisce `rtc_total_secs_` dai
S/M/H/DL/DH live, si ripristinano i `latched_*`, si legge `saved_unix` e
si calcola `wall_gap = now - saved_unix`. Se la RTC non era halted al
momento del dump, il gap viene foldato nel total — così l'orologio ha
"continuato a girare" mentre il processo era spento. Halted → time
congelato. **Comportamento simmetrico fra pausa overnight e shutdown
overnight**: in entrambi i casi il gioco vede l'ora corretta al resume.

**Edit collaterale**: `mbc3::reset()` non azzera più `latched_*` —
nell'hardware reale lo shadow è battery-backed sul chip RTC del cart e
sopravvive a un soft reset CPU. Necessario perché l'hot-swap path è
`load_rom_` (che chiama `rtc_load_blob`) → `core::reset()` (che chiama
`mbc::reset()`), e se quest'ultimo azzerasse i latched perderemmo i
valori appena caricati. Nessun gioco osserva la differenza perché tutti
fanno una sequenza di latch prima di leggere.

**Interop**:

- Contenuto SRAM compatibile con BGB / mGBA / VBA-M / SameBoy.
- Nome file specifico (`<fnv1a>.sav` invece di `<rom_basename>.sav`) →
  l'import/export richiede un rename manuale. Decisione consapevole: il
  rename-stable di FNV1a vale la perdita di drop-in compat sul nome.
- L'RTC block in formato BESS è leggibile da SameBoy. BGB/mGBA usano
  formati proprietari → l'orologio non si trasferisce automaticamente
  ma SRAM sì.

**Distinzione da Save State (sezione 2)**: il battery save è solo la SRAM
del cart, è il salvataggio *del gioco* (il giocatore lo crea via menu
in-game). Save state = snapshot completo dell'emulatore (hotkey). Le due
feature coesistono e §2 può riusare lo stesso reader/writer BESS aggiungendo
i blocchi CORE/MBC/IORG.

---

## 7. DMG OAM bug emulation

Blargg `oam_bug.gb` fallisce: dipende da un hardware quirk DMG documentato
ma non implementato. Quando un'istruzione fa un 16-bit INC/DEC su un register
pair (`INC rr` / `DEC rr` / `LD A,(HL+)` / `PUSH rr` / `POP rr` / ecc.) e il
*valore precedente* del pair è in `$FE00-$FEFF` *durante PPU mode 2* (OAM
scan, primi 20 M-cycle di una scanline visibile), il datapath interno della
CPU corrompe una riga OAM con un pattern specifico. Il bug non è triggerato
da nessun gioco commerciale — è un test puro di accuratezza hardware.

Suite (8 sotto-ROM, `1-lcd_sync.gb` … `8-instr_effect.gb`):
- `1-lcd_sync` — calibrazione: assicura che il test possa allinearsi al
  boundary di mode 2.
- `2-causes` — istruzioni che CAUSANO la corruzione.
- `3-non_causes` — controprova: istruzioni simili che NON la causano (es. un
  `INC r` 8-bit non triggera).
- `4-scanline_timing` — finestra temporale di mode 2 entro la scanline.
- `5-timing_bug` — quando il bug si attiva entro l'istruzione.
- `6-timing_no_bug` — controprova fuori finestra.
- `7-timing_effect` — pattern di corruzione (quale byte OAM viene alterato
  e con quale valore).
- `8-instr_effect` — effetto per categoria di istruzione.

**Cosa serve:**

1. **PPU espone la modalità corrente con granularità M-cycle**: già
   parzialmente vero (`hwr_stat` ha la mode aggiornata in `ppu::step`), ma
   serve un getter dedicato `bool ppu::oam_scan_active() const` consultabile
   da CPU senza un round-trip via MMIO.
2. **Hook nei body delle istruzioni "cause"**: `inc_bc` / `inc_de` / `inc_hl`
   / `inc_sp` (+ `dec_*`), `ld_a__hlp__` / `ld_a__hlm__` / `ld__hlp__a` /
   `ld__hlm__a`, `pop_*`, `push_*` e affini — prima del bump del registro,
   se il valore corrente è in `$FE00-$FEFF` AND `ppu.oam_scan_active()`,
   eseguire la corruzione documentata.
3. **Logica di corruzione OAM** (Pan Docs §"OAM Corruption Bug"):
   - Pattern read-modify-write su una "riga OAM" da 8 byte (40 sprite × 4
     byte → 20 righe da 8 byte ciascuna).
   - Il tipo di corruzione varia per istruzione: alcune fanno glitch di
     tipo "read", altre "write", altre "read-write".
   - `INC HL` e `LD A,(HLI)` hanno comportamenti leggermente diversi.
4. **Verifica**: ogni sotto-ROM stampa un risultato confrontabile con il
   reference mGBA/SameBoy. Procedere in ordine 1-8: il `1-lcd_sync` va
   chiuso per primo per validare che le altre sotto-ROM possano partire.

**Costo stimato**: 1-2 giorni di lavoro mirato, oracolo = i test stessi.

**Priorità**: bassa. Nessun gioco commerciale rompe senza questo. Ha senso
solo se l'obiettivo è "DMG-accurate al 100%" sul piano dei quirk hardware.

---

## 8. DMG APU wave RAM access bug (CH3)

Blargg `dmg_sound` fallisce su tre sotto-ROM, tutte sintomo dello stesso
hardware quirk DMG sul canale 3 (wave): l'accesso CPU a `$FF30-$FF3F`
mentre il canale è attivo segue regole speciali, e ri-triggerare il canale
mentre è attivo corrompe wave RAM.

Sotto-ROM fallite:
- `09-wave_read_while_on:01` — read di `$FF30-$FF3F` mentre CH3 on. Atteso
  DMG: `0xFF` salvo nell'M-cycle in cui CH3 fa il fetch del nibble (in
  quel caso ritorna il byte di wave RAM corrente).
- `12-wave_write_while_on:01` — speculare per le write: ignorate, tranne
  nella stessa finestra di allineamento.
- `10-wave_trigger_while_on:01` — il "wave RAM corruption bug":
  ri-triggerare CH3 mentre sta suonando, in una finestra precisa rispetto
  al prossimo sample fetch, copia il byte (o 4 byte allineati) che CH3
  sta per leggere nei primi 1 o 4 byte di wave RAM.

**Cosa c'è oggi:**
- `wave_channel` (`inc/apu.h:82` + `src/apu.cpp:285`) traccia `wave_pos`
  (0..31) e `sample_buffer` ma NON il momento esatto in cui ha fatto
  l'ultimo fetch.
- `mmu::wave_ram()` espone uno span sui 16 byte. Il routing di
  `$FF30-$FF3F` passa per il normale path mmio: nessun gating in funzione
  di `channel_enabled`, nessuna logica di corruzione su trigger.

**Cosa serve:**

1. **Gating delle read/write a `$FF30-$FF3F`** (chiude i `:01` di 09 e 12):
   - Esporre `bool apu::ch3_active() const`.
   - Negli mmio handler di `$FF30-$FF3F`: se `ch3_active()`, read ritorna
     `0xFF` e write è ignorata. Su CGB (quando supportato) il comportamento
     è diverso — read/write redirigono sempre al byte attualmente fetchato.
2. **Wave RAM corruption on trigger** (chiude `10:01`):
   - In `wave_channel::trigger()`, prima del reset di `wave_pos`, se
     `channel_enabled` era già true E `freq_timer` è vicino a 0 (il prossimo
     fetch è imminente — finestra esatta da derivare dai Pan Docs),
     eseguire la corruzione:
     - `next_pos = (wave_pos + 1) & 0x1F; next_byte = next_pos >> 1;`
     - se `next_byte ∈ 0..3`: `wave_ram[0] = wave_ram[next_byte]`.
     - se `next_byte ∈ 4..15`: copia 4 byte allineati a `next_byte & 0x0C`
       in `wave_ram[0..3]`.
3. **Sub-test timing (sotto-ROM `:02` e oltre)**: per chiudere i sotto-test
   non-`:01` serve esporre il timing del fetch CH3 con granularità M-cycle
   e gestire la finestra di "aligned access" (2-cycle window in cui
   read/write redirigono a `wave_ram[wave_pos >> 1]` invece di `0xFF` /
   ignore). Lavoro più sottile, condizionato all'accuratezza APU che si
   vuole.

**Costo stimato**: ~mezza giornata per i tre `:01`, 1-2 giorni per chiudere
tutti i sotto-test di 09 / 10 / 12.

**Priorità**: bassa. Nessun gioco commerciale dipende da queste sequenze
(scrivono wave RAM con CH3 disabilitato — primo passo di qualsiasi setup
sensato). Solo accuratezza hardware DMG sul piano audio.

---

## 9. Refactor: sub-instruction (M-cycle) timing — **fatto (hybrid, PPU bulk)**

Implementato il modello tick-driven: ogni accesso bus (`cpu::bus_read` /
`bus_write` / `bus_read_u16` / `bus_write_u16` / `bus_read_i8`) chiama
`cpu::tick(4)` prima del trasferimento e propaga al `tick_fn` installato da
`core::core()`. I cicli interni non coperti dagli accessi (preparazione SP
di PUSH, ALU 16-bit di ADD HL,rr, branch PC update, ecc.) sono
*bulk-ticked* alla fine di `cpu::step()` con `tick(target - step_cycles)`
dove `target = e.cycles + extra_cycles`. `irq::dispatch()` ticca i suoi 20 T
(8 entry + 8 push + 4 jump-internal) via `cpu.tick`.

**APU / timer / total_cycles** avanzano a granularità M-cycle dentro il
callback. **Il PPU no**: i tick destinati al PPU sono accumulati in
`core::pending_ppu_t` e svuotati in un unico `ppu.step(pending_ppu_t)` a
fine `core::step()`. La ragione è che il nostro PPU renderizza la scanline
in modo atomico in `enter_drawing()`: forwardare i tick mid-istruzione
sposta l'ordine relativo fra scritture CPU e transizioni di modo del PPU,
rompendo i giochi che ritunano SCX/SCY/LCDC in HBlank STAT IRQ stretti
(sintomo originale: status bar di Super Mario Land che flickerava di una
scanline a intervalli irregolari — bisect ha incolpato il commit che
attivava il tick PPU intra-istruzione).

**Accuratezza PPU sub-istruzione** (Mooneye PPU suite, FIFO emulation,
trucchi mid-scanline su LCDC, BG-priority quirks) richiederebbe un PPU
dot-driven vero con latching dei registri al momento corretto. Lavoro
separato e grosso — non è una "pulizia" del modello attuale, è un
rewrite della pipeline.

**Stato test Blargg:**
- `cpu_instrs.gb` ✅
- `instr_timing.gb` ✅
- `mem_timing.gb` ✅
- `mem_timing-2.gb` ✅
- `halt_bug.gb` ✅
- `oam_bug.gb` ❌ — quirk DMG indipendente dall'M-cycle, vedi §7.
- `interrupt_time/interrupt_time.gb` ✅ (richiede CGB; passa ora che il branch `gbc` ha attivato il path CGB).
- `dmg_sound` 09:01 / 10:01 / 12:01 ❌ — wave RAM access bug del CH3,
  indipendente dall'M-cycle, vedi §8.
- `cgb_sound` 08:01 / 09:01 / 11:04 / 12:02 ✅ — chiusi dai branch CGB-gate
  su `power_off`, dal redirect MMU $FF30-$FF3F installato dall'APU e dal
  ramo CGB di `wave_channel::trigger`. Dettaglio in §16. I sotto-test `:02+`
  di altri gruppi `cgb_sound` non sono ancora stati ispezionati.

---

## 10. Release pipeline

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

## 11. Menù bar funzionale — **scaffolding fatto, voci §2/§3 ancora da popolare**

Stato attuale (commit `b756075` + `c3173df` + `40eb54f`): menu bar a quattro voci
in `src/ui.cpp` (`draw_menu_bar`), hotkey nel main loop di `Application::run`,
boundary UI→host via `ui::host_actions` (drained ogni frame dopo
`SDL_RenderPresent`).

**Fatto:**

- **File**
  - `Load ROM…` (`Ctrl+O`) — apre `GetOpenFileNameW` nativo Windows,
    parented alla SDL window. Filtro `*.gb;*.gbc`. Selezione → propaga
    `host_actions::pending_rom_load`; Application fa `core.load(rf);
    core.reset();` e `ui::add_recent_rom(...)`. Niente dipendenza
    `nfd`/`tinyfiledialogs` (Windows-only finché non serve Linux/macOS
    in §10).
  - `Recent ROMs ▸` — MRU a 8 entry, persistita in `gbemu_recent.txt`
    accanto a `imgui.ini` (una riga per path). Submenu disabled +
    "(none)" quando la lista è vuota. Voce `Clear list` in fondo.
  - `Exit` — push `SDL_QUIT` via `host_actions::quit_requested`.

- **Emulation**
  - `Resume`/`Pause` (`Space`) — toggle su `debugger::pause()` /
    `debugger::resume()`. Gated su `has_rom(c)` (`mmu.cart() != nullptr`)
    — senza cartuccia il MMU restituisce open-bus, "Run" eseguirebbe
    solo 0xFF (RST 38h).
  - `Reset` (`Ctrl+R`) — `debugger::reset()` → `core::reset()` (vedi §1).
    Gated su `has_rom(c)`.
  - `Speed ▸` — submenu **disabled** (placeholder, attende §3).
  - `Save State ▸` / `Load State ▸` — submenu **disabled** (attendono §2).
  - `Toggle Fullscreen` (`F11`) — `SDL_SetWindowFullscreen` con
    `SDL_WINDOW_FULLSCREEN_DESKTOP`, checkmark sullo stato corrente.

- **View** — toggle per i 9 pannelli (Display, CPU, Disassembly, Memory,
  Breakpoints, PPU, MBC, Serial, PC ring) + `Reset layout`. Era già in
  piedi.

- **About**
  - `About GbEmu…` — popup modale con nome ("GbEmu"), tagline ("Game Boy
    emulator"), versione hardcoded `0.1.0-dev`, build timestamp
    (`__DATE__ __TIME__`), versione runtime SDL2 e ImGui. Pulsante
    `Copy version info` (clipboard stringa diagnostica) + `OK`. Pattern
    "request flag → OpenPopup nel frame" per non finire in conflitto con
    lo scope del DockSpace.

**Cosa resta:**

- ~~Popolare `Speed ▸` con le radio entries (`0.25x`…`4.0x`) — dipende §3.~~
  **fatto** insieme a §3 (vedi sopra).
- Popolare `Save State ▸` / `Load State ▸` con slot 0..9 + timestamp /
  "empty" — dipende §2. Hotkey suggerite: `F5` save / `F9` load sullo
  slot corrente; `Shift+0..9` per cambiare slot.
- Versione hardcoded `0.1.0-dev`: va sostituita con un define generato
  dal CMake (dipende §10 versioning).

**Note implementative (per non perdere il pattern):**

- File dialog **non** può girare dentro `NewFrame/Render`: si setta
  `host_actions::load_rom_dialog_requested`, Application lo apre dopo
  `SDL_RenderPresent`. Pattern simmetrico per `pending_rom_load`.
- Le hotkey nel main loop sono gated su `!imgui_captured` (restituito da
  `ui::process_event`) per non triggerare quando un text field ImGui ha
  focus.

---

## 12. Player UI (default) + `--debug` per la UI debugger

Oggi `src/ui.cpp` è l'unica UI: dock-space ImGui con 9 pannelli (Display,
CPU, Disassembly, Memory, Breakpoints, PPU, MBC, Serial, PC ring). È una
UI da *sviluppatore di emulatori*, non da giocatore. Va affiancata da una
**Player UI** che diventerà l'esperienza di default, mentre la UI attuale
verrà nascosta dietro `--debug`.

**CLI** (`src/gbemu.cpp`):

- Aggiungere `cli.add_flag("--debug", debug_ui, "Launch with the debugger UI (panels, breakpoints, disasm)")`.
- Default (`--debug` non passato): Player UI.
- `--headless` è ortogonale e prevale su entrambe (no SDL window).

**Player UI — caratteristiche:**

- **Solo schermo di gioco + cornice Game Boy**. La cornice (chassis DMG
  grigio, logo "Nintendo GAME BOY", griglia speaker, croce direzionale +
  A/B + Start/Select disegnati ma decorativi) è renderizzata come immagine
  statica di sfondo; lo schermo 160×144 è il `gfx::presenter` esistente
  (già usato dalla Display panel) piazzato esattamente nel rettangolo
  della "finestra LCD" sulla cornice. Asset: PNG ad alta risoluzione
  (es. 1200×1800), caricato via `stb_image` (già disponibile o da
  aggiungere a `vcpkg.json`).
- **Nessun pannello, nessun docking, nessuna disasm/memory/breakpoint/PC-ring**.
  Tutta la superficie della finestra ospita la cornice + schermo. La
  Player UI usa ImGui *solo* per la menu bar e i popup (About, file dialog
  via flag come da sezione 11), non per pannelli dockabili.
- **Menu bar** = quella della sezione 11, ma con la voce **View** rimossa
  (non ci sono pannelli da togglare). Restano File / Emulation / About.
  Le hotkey funzionano tutte (Load ROM, Pause, Reset, Speed, Save State,
  Fullscreen).
- **Scaling**: lo schermo 160×144 viene scalato preservando l'aspect ratio
  in modo da entrare esattamente nella finestra LCD della cornice. La
  cornice stessa viene scalata uniformemente per riempire la window SDL,
  centrata (letterbox/pillarbox se l'aspect ratio della window differisce).
- **Fullscreen**: in fullscreen la cornice resta visibile (centrata su
  sfondo nero) — è parte dell'estetica. Hotkey alternativa "screen-only"
  (`F10`?) che nasconde la cornice e ingrandisce lo schermo al massimo
  per chi preferisce gameplay puro.

**Debugger UI (`--debug`)** — è la UI attuale (`src/ui.cpp`), nessun
cambio funzionale. Mantiene tutti i pannelli e la menu bar completa
(File / Emulation / View / About).

**Implementazione:**

- Estrarre un'interfaccia minima `gbemu::ui::frontend` con i tre punti
  di contatto attuali — `init(...)`, `process_event(SDL_Event const&) -> bool`,
  `render_frame(...)`, `shutdown()` — e fornire due implementazioni:
  `player_frontend` e `debugger_frontend` (rinomina cosmetica dell'attuale
  `src/ui.cpp`). `Application::run` istanzia l'una o l'altra in base al
  flag CLI.
- I due frontend condividono **codice comune** (menu bar + handler delle
  hotkey + flag pattern per file dialog) tramite header dedicato
  (`inc/ui_common.h`?). La menu bar della Player UI è quella della Debugger
  UI meno la voce View — meglio una funzione `draw_menu_bar(context&,
  bool show_view_menu)` riusata da entrambi.
- La cornice è uno `static` cached al primo render (path es.
  `assets/dmg_frame.png`, copiato in `build/assets/` come si fa per
  `cfg/gbemu.conf`). Caricamento via `stb_image_load` → upload in una
  texture SDL → `ImGui::GetBackgroundDrawList()->AddImage(...)` per
  renderizzarla dietro lo schermo del GB.
- **Save state / Load ROM / Speed / Pause** restano disponibili da menu
  e hotkey: la Player UI non *implementa* il debugger ma usa lo stesso
  `core` + `debugger` sotto. Il `debugger` (con breakpoint/watchpoint
  vuoti) gira lo stesso — l'utente non se ne accorge ma il backend è
  uno.

**Considerazioni:**

- F12 (interactive toggle) oggi è solo nella debugger UI. In Player UI
  non ha senso — va rimosso da quel frontend.
- Le dimensioni iniziali della window (`gbemu_window.state` accanto a
  `imgui.ini`) vanno separate fra Player e Debugger UI: hanno aspect
  ratio molto diversi (cornice GB ≈ 2:3 vs dock-space orizzontale).
  Suggerito: due file (`gbemu_window_player.state`,
  `gbemu_window_debugger.state`) caricati in base al flag.
- L'assetto `imgui.ini` della Debugger UI non interferisce con la
  Player UI (la Player UI non usa docking, quindi non scrive nulla di
  utile in `imgui.ini`). Va comunque tenuto un file separato per
  evitare che il dock layout debugger sovrascriva eventuali stati
  Player futuri — `imgui_player.ini` / `imgui_debug.ini`.

**Dipendenze**: nessuna bloccante. La sezione 11 (menu bar) è il
prerequisito naturale — meglio chiuderla prima così entrambi i frontend
partono con la stessa menu bar. Estrazione `frontend` interface può
avvenire in parallelo.

**Asset cornice**: serve un disegno/foto del DMG su sfondo trasparente.
Opzioni: (a) commissionare/disegnare in-house; (b) usare asset CC0 da
OpenGameArt o simili (verificare licenza); (c) generare a partire da una
foto del DMG fisico con background removal. Decisione da prendere prima
di iniziare l'implementazione UI vera e propria.

---

## 13. Rebranding — scelta nome + rinomina superfici pubbliche

`GbEmu` è un placeholder generico. Prima di toccare la release pipeline (§10),
l'About dialog (§11) e la Player UI (§12) serve scegliere un nome definitivo
e propagarlo nelle parti pubbliche del codebase.

**Vincoli per il nome:**

- Non in collisione con emulatori esistenti (BGB, mGBA, SameBoy, Gambatte,
  GBE+, BizHawk, …).
- Non in collisione con marchi Nintendo / "Game Boy" / "GB".
- Idealmente breve (≤ 8 char), pronunciabile, dominio plausibilmente
  disponibile.
- Candidati proposti (verificati su emulator-zone, Emulation General Wiki,
  GitHub topic `gameboy-emulator`, awesome-gbdev), in ordine decrescente
  di pulizia:
  - **BoringBoy** — clean: nessuna collisione né come emulatore né in altri
    settori rilevanti.
  - **RageBoy** — nessun emulatore con questo nome; resta collisione con il
    meme "RageGuy / FFFFFFUUUUUU" (settore diverso, accettabile).
  - **StarBoy** — nessun emulatore con questo nome, ma c'è un gioco indie
    pixel-art "StarBoy" su itch.io / Steam (mrkdji) + brano di The Weeknd.
    Stesso ecosistema retro-gaming → rischio confusione medio.
  - **BoyBoy** — nessun emulatore esattamente con questo nome, ma esiste
    **My Boy!** (emulatore GBA Android molto popolare). Fonetica/dominio
    troppo vicini → da escludere.
- La scelta del nome è la parte lenta, il rename meccanico è banale una
  volta deciso.

**Superfici pubbliche da rinominare** (path concreti, da grep su `GbEmu` /
`gbemu`):

- **Eseguibile / build system**
  - `CMakeLists.txt:3` e `src/CMakeLists.txt:3,16,17,19,30` — `project(GbEmu)`
    + target `GbEmu` → produce `GbEmu.exe`.
- **CLI / window title**
  - `src/gbemu.cpp:14` — `CLI::App cli{"GbEmu — Game Boy emulator"}` (mostrato
    in `--help`).
  - `src/app.cpp:235` — `SDL_CreateWindow("GbEmu", …)` (titlebar).
  - Filename `src/gbemu.cpp` (entry-point) — rinomina opzionale, va in coppia
    con `add_executable(...)`.
- **File su disco** (rottura compat accettata, è una v0.x)
  - `cfg/gbemu.conf` — path hardcoded in `src/app.cpp:146`, copy rule in
    `src/CMakeLists.txt:33-34`.
  - `gbemu_window.state` — costante `WINDOW_STATE_FILE` in `src/app.cpp:34`.
  - File futuri menzionati in §11/§12 (`gbemu_recent.txt`,
    `gbemu_window_player.state`, `gbemu_window_debugger.state`,
    `imgui_player.ini` / `imgui_debug.ini`): applicare il nuovo prefisso
    direttamente quando si implementano — niente migrazione ex-post.
- **ImGui dock IDs persistiti**
  - `src/ui.cpp:152,155` — `"##GbEmuDockHost"` / `"##GbEmuDockSpace"`. Sono
    ID interni ma vengono serializzati in `imgui.ini`: rinominarli invalida
    il layout salvato dell'utente. Il fallback `DockBuilder*` ricostruisce
    il default → si perde solo la personalizzazione.
- **Namespace C++**
  - `namespace gbemu { … }` (~46 file). Rename meccanico via IDE/sed.
    Opzione alternativa: tenere `gbemu` come namespace interno e usare il
    nuovo nome solo sulle superfici esterne — legittima se il nuovo nome
    non si presta a uno short identifier C++.
- **Variabili d'ambiente**
  - Oggi **nessuna** env var `GBEMU_*` è letta dal codice (verificato: zero
    match). Quando se ne aggiungeranno (es. override path save directory
    §6, override config path §10) usare direttamente il nuovo prefisso.
- **Documentazione**
  - `CLAUDE.md` (riga 17, 21, 111, 113, 164 — riferimenti a `GbEmu.exe` e
    `gbemu.conf`).
  - `TODO.md` §11 ("About GbEmu…") e §10 (`cfg/gbemu.conf`).
  - Script di smoke (`scripts/dbg_smoke.dbg`, `scripts/dmg_sound_check.dbg`)
    — commenti con `.\GbEmu.exe`.
  - `claude_review.md` — riferimenti storici, lasciabili.

**Cosa NON rinominare** (intenzionalmente):

- Directory del repo (`gameboy-emulator`) — è il nome del repo Git, non del
  prodotto.
- Logger root name `"root"` (`src/log.cpp`) — non è branding.
- Mnemonic opcode, costanti hardware (`hwr_*`), nomi delle ROM Blargg —
  terminologia di dominio, non branding.

**Ordine consigliato:**

1. Decidere il nome.
2. Aggiornare CMake target + window title + CLI description (3 punti).
   Build, verifica `GbEmu.exe` → `<NewName>.exe`.
3. Rinominare i file su disco (`cfg/gbemu.conf`, `gbemu_window.state`),
   aggiornare costanti in `app.cpp` e `configure_file` in `src/CMakeLists.txt`.
4. Rinominare ImGui dockspace IDs.
5. (Opzionale, PR separata) rinominare `namespace gbemu`.
6. Aggiornare `CLAUDE.md` e script `.dbg`.

**Dipendenze**: nessuna in ingresso. Bloccante per §10 (nome exe → zip /
packaging), §11 (label "About …"), §12 (titlebar Player UI + file di stato
dedicati). Va chiuso prima di queste tre.

**Costo stimato**: 1-2 ore di rename meccanico una volta scelto il nome.

---

## 14. Cartella `user/` per i file di sessione — **fatto**

Prima di questo intervento i file di stato della sessione vivevano nel
**CWD** del processo: `gbemu_window.state` (window pose) e
`gbemu_recent.txt` (MRU recents) come bare filenames, `imgui.ini` per
default sempre relativo al CWD. Conseguenza: lanciando l'eseguibile da una
directory diversa, finestra/recents/layout si "perdevano". Incoerente col
portable layout già in piedi per `cfg/`, `bios/`, `roms/`, `savs/`,
`sslots/`, `palettes/`.

**Layout finale:**

```
<base>/user/
  user.conf      # libconfig — window pose + recent_roms + display.active_palette
  imgui.ini      # ImGui dock/window layout (formato proprietario ImGui)
```

**File:**

- `inc/cfg.h` + `src/cfg.cpp`: nuovo `cfg.paths.user_dir{"user"}`, parsato
  nel blocco `paths` (default kicks in se mancante). `cfg/gbemu.conf`
  espone la riga `user_dir = "user"`.
- `src/CMakeLists.txt`: aggiunto `${CMAKE_CURRENT_BINARY_DIR}/user` al
  `file(MAKE_DIRECTORY ...)`.
- `inc/user_state.h` + `src/user_state.cpp`: nuovo modulo `gbemu::user_state`
  con `window_w`, `window_h`, `recent_roms`, `active_palette`. `load(path)`
  swallows missing-file/parse errors (ritorna false, lascia defaults).
  `save(path)` scrive atomicamente via libconfig `.tmp` +
  `std::filesystem::rename`, best-effort (logga e ritorna false su failure
  — niente throw).
- `Application` (in `inc/app.h` + `src/app.cpp`): nuovo membro
  `user_state_` + `user_conf_path_` risolto in `run()` via
  `paths::resolve_data_path(base_, cfg.paths.user_dir, "user.conf")`. Load
  all'avvio (prima della `SDL_CreateWindow`), save a shutdown con
  `SDL_GetWindowSize` → `user_state_.window_*`. `WINDOW_STATE_FILE`,
  `load_window_state`, `save_window_state` rimossi.
- `gbemu::ui::init` ora prende `user_state&` + `std::string const& imgui_ini_path`.
  `context::user` punta a `Application::user_state_`. Il path imgui.ini è
  stoccato in `context::imgui_ini_path` (string owned, ImGui salva il
  puntatore) e applicato a `ImGui::GetIO().IniFilename` subito dopo
  `ImGui::CreateContext()`. `RECENT_ROMS_FILE`, `load_recent_roms`,
  `save_recent_roms` rimossi; il menu "Recent ROMs" ora opera su
  `c.user->recent_roms`.

**Trigger di salvataggio:**

- Window pose: solo a shutdown (`Application::run` snapshotta `SDL_GetWindowSize`
  e chiama `user_state_.save`).
- Recents: `ui::add_recent_rom` muta `user->recent_roms` e setta
  `host_actions::save_user_state_requested`. Application drena il flag
  dopo il blocco `pending_rom_load` (così un add_recent_rom triggerato
  da un ROM hot-swap nel frame corrente persiste nello stesso frame
  invece di aspettare il successivo).
- Active palette: schema esposto ma non ancora wirato — il futuro palette
  switcher setterà `user_state_.active_palette` e alzerà lo stesso flag.

**Eccezione `imgui.ini`**: formato proprietario di ImGui, non foldable in
`user.conf`. Spostato sotto `user/` settando `io.IniFilename =
ctx->imgui_ini_path.c_str()` dopo `CreateContext` ma prima del primo
`NewFrame` (ImGui carica le settings lazily da `Initialize()` dentro
`NewFrame`). Il backing `std::string` vive sul `context` per outliveare
ImGui stesso.

**Migrazione**: rottura compat accettata (è v0.x). Niente fallback / move
automatico — chi aveva `gbemu_window.state` / `gbemu_recent.txt` /
`imgui.ini` in CWD li ricrea al primo avvio. Documentato in `CLAUDE.md`
§ Portable layout.

**Distinzione da `cfg/`**: `cfg/gbemu.conf` è la **configurazione** editata
dall'utente (preferenze esplicite). `user/` è **stato di sessione**
generato e gestito dall'app (window pose, MRU, dock layout, palette
selezionata). Tenerli separati evita di mescolare "cose che l'utente
edita" con "cose che l'app riscrive in continuazione".

**Estensione futura §12 (Player/Debugger UI split)**: il sub-block
`window` di `user.conf` può crescere in `window_player` / `window_debugger`
senza migrazione (sub-block mancante → default). `imgui.ini` si splitterà
in `user/imgui_player.ini` / `user/imgui_debug.ini` passando la stringa
giusta a `ui::init` in base al flag CLI `--debug`.

---

## 15. Menu Audio — abilita/disabilita audio — **fatto**

Menu **Audio** fra **Emulation** e **View** (`src/ui.cpp` `draw_menu_bar`):

- `Mute` checkable — hotkey `M` (gated su `!imgui_captured`, niente collisione
  con i tasti joypad).
- `Volume ▸` — slider 0-100%, hotkey `Ctrl+Up`/`Ctrl+Down` che fanno snap al
  bucket 10% strettamente superiore/inferiore (47% → UP → 50%; 50% → UP → 60%).
- `Highpass Filter ▸` — radio `Off` / `Accurate` / `Preserve waveform`.

Stato persistito sotto un sub-block `audio` in `<base>/user/user.conf`:

```
audio: { muted = false; volume = 1.0; highpass = "off"|"accurate"|"preserve"; };
```

Default: non muted, volume 100%, filtro `accurate` (~60 Hz su DMG, ~120 Hz su
CGB — il branch viene scelto in `apu::recompute_hp_alpha` consultando
`mmu.cgb_mode()`).

**Implementazione attuale (produttore-gate):** mute è un hard-zero nel push del
ring (`apu::emit_sample` in `src/apu.cpp`), il consumer SDL non viene mai
messo in pausa. Niente drain del ring necessario, niente pop a unmute (il
ring si svuota naturalmente con campioni a 0). Volume = slider lineare 0-1 →
APU riceve `volume²` per una curva percettivamente lineare (-12 dB al
midpoint). Highpass = IIR 1° ordine `y[n] = α (y[n-1] + x[n] - x[n-1])` con
α derivata da `exp(-1/(SAMPLE_RATE · τ))`. La memoria del filtro viene
azzerata da `apu::reset` (incluso quindi hot-swap) ma sopravvive al cambio
di modalità (il decadimento naturale assorbe il salto di α senza pop).

**Cosa NON è stato fatto rispetto al piano originale:**

- Toggle per-canale (CH1..CH4) — restano da §15 follow-up, utile solo per
  debug APU.
- Persistenza con §3 (speed multiplier): quando arriverà §3, l'auto-mute a
  multiplier ≠ 1.0 dovrà coesistere con `user_state_.audio_muted`. Pattern
  suggerito (non implementato): tenere due flag separati (`user_muted` vs
  `speed_muted`), OR per il gating in APU. Oggi c'è solo il primo.

---

## 16. CGB APU quirks — **fatto** (08:01 / 09:01 / 11:04 / 12:02 chiusi sul ramo `:01`)

Sul branch `gbc` il bring-up CGB ha lasciato l'APU come copia 1:1 della DMG.
Blargg `cgb_sound` esercita quattro divergenze hardware DMG → CGB che oggi non
modelliamo. Tutte risolvibili interrogando `mmu_.cgb_mode()` (già usato in
PPU/HDMA): la knob esiste, manca solo applicarla nei posti giusti dentro
`src/apu.cpp`.

**08-len_ctr_during_power:01 — length counters azzerati a power-off — fatto**

- Su DMG i length counters dei 4 canali sopravvivono al clear di NR52 bit 7 e
  continuano a contare; su CGB vengono **azzerati**.
- `apu::power_off()` ora calcola `preserve_lengths = !mmu_.cgb_mode()` e
  reinietta i quattro `length` solo nel ramo DMG; su CGB cadono a zero con
  il resto dello stato di canale.

**09-wave_read_while_on:01 / 12-wave_write_while_on — accesso wave RAM con CH3 attivo — fatto**

- Su DMG la read/write di `$FF30-$FF3F` mentre CH3 è on restituisce/scrive
  fuori target salvo durante la finestra di 2 T-cycle del prossimo fetch CH3
  (cfr. §8 per la versione DMG, ancora aperta).
- Su CGB l'accesso **redirige sempre** al byte che CH3 sta correntemente
  fetchando, cioè `wave_ram[wave_pos >> 1]`, indipendentemente dall'indirizzo
  richiesto. Non c'è la finestra di blackout DMG.
- Implementazione (read): `mmu::add_mmio_read_handler(addr, fn)` simmetrico
  al write handler — `mmio_read_fn(addr, current) -> uint8_t`, chained in
  `read_u8` dopo `mmio_read_masked`.
- Implementazione (write): `mmu::add_mmio_write_redirect(addr, fn)` con
  signature `(uint16_t addr) -> uint16_t` — viene consultata in `write_u8`
  prima dello store mmio e ritorna l'eventuale indirizzo redirezionato (i
  write handler post-store firano sul target redirezionato). L'APU registra
  entrambe le hook su tutto `$FF30-$FF3F`: in modalità CGB con
  `ch3_.channel_enabled`, read e write puntano a `wave_ram[wave_pos >> 1]`,
  altrimenti pass-through. Lo stesso entrypoint può poi chiudere il lato DMG
  di §8 con un branch sul modello (window di 2 T-cycle + blackout 0xFF /
  ignore della write).

**11-regs_after_power:04 — NRx1 ignorato a power-off su CGB — fatto**

- Su DMG le scritture *solo della length* a `NR11`/`NR21`/`NR31`/`NR41`
  sono accettate anche con APU spenta (Pan Docs / Blargg note "length is
  unaffected by power"). Su CGB **anche** NRx1 è ignorata mentre la APU è
  off.
- I quattro `on_nrX1` ora skippano la length write nel ramo `!powered_`
  quando `mmu_.cgb_mode()`: il valore di `length` non viene toccato e
  l'mmio byte resta forzato al suo read-mask come prima.

**12-wave_write_while_on — accesso CGB con CH3 attivo — fatto**

Tre fix combinati per chiudere 09:01 + 12:0x:

1. **Increment-after-fetch in `wave_channel::tick_frequency`** — prima
   `wave_pos` veniva incrementato PRIMA della fetch, quindi byte 0 veniva
   letto una sola volta per ciclo (solo low nibble a `wave_pos=1`), mentre
   tutti gli altri byte due volte. SameBoy / Pan Docs: ogni byte high+low
   nibble, partendo da byte 0 high.

2. **`current_sample_byte` / `current_sample_byte_idx` separati** — il
   redirect non può più usare `wave_ram[wave_pos>>1]` perché dopo il flip
   dell'increment quel valore punta al PROSSIMO byte da fetchare; quindi
   il channel registra il byte appena letto + il suo indice e il redirect
   read/write li consuma.

3. **Trigger: `freq_timer = 6` su CGB** — invece di `(2048-freq)*2`. Il
   primo fetch avviene 6 T-cycle dopo trigger (non un periodo intero),
   poi i fetch successivi si svolgono ogni `(2048-freq)*2`. Senza questo,
   il pre-fetch tiene `current_sample_byte` a 0 (residual) per un periodo
   intero di nibble, producendo ~2 read di residual extra che spostano il
   pattern leading di 09:01 (visto come 6×00 invece di 4×00). Su DMG il
   formato resta `(2048-freq)*2`: il quirk di gating della wave RAM (§8)
   interagisce diversamente con il timing di trigger e i test dmg_sound
   che passano oggi non sopravviverebbero al cambio.

Su trigger CGB anche `current_sample_byte` e `_idx` vengono azzerati
oltre a `sample_buffer` (simmetria); su DMG tutti restano al valore
precedente (residual), com'è stato sempre.

**Interazione con §8**: il read-handler MMU per $FF30-$FF3F è ora in piedi
(registrato dall'APU); chiudere il lato DMG di §8 si riduce a un branch sul
modello dentro la stessa lambda (window di 2 T-cycle + blackout 0xFF).

---

## 17. Input refactor — hotkey manager + bindings configurabili — **step 1-2 fatti**

**Stato**: il refactor del dispatcher è chiuso (passi 1 e 2 della sezione
"Ordine consigliato" sotto). `inc/input.h` + `src/input.cpp` ospitano il
modulo `gbemu::input::manager` con `action` enum, `key_binding` (con
`kind::oneshot|hold` e `gate::always|rom_only`) e `joypad_binding`.
`Application::run` ora delega ogni `SDL_KEYDOWN/KEYUP/CONTROLLER*` al
manager e instrada il risultato attraverso `Application::handle_action_`,
sostituendo le ~90 righe inline che mescolavano hotkey applicativi e
joypad mapping. Le voci della menu bar (`src/ui.cpp::draw_menu_bar`)
leggono le shortcut label da `input::manager::shortcut_label(action)`
invece che da stringhe hardcoded, così un rebinding aggiorna le hint
senza un edit parallelo. Modifier mask ora è **strict** (`Ctrl+Shift+R`
non triggera `reset` — comportamento intenzionale, diverso dal vecchio
loose match).

**Cosa resta:**

- **Step 3** — persistenza in `user_state_.input` (libconfig sub-block
  `input`). Default kicks in se mancante; binding invalidi → log + skip.
  `SDL_GetKeyFromName` / `SDL_GetKeyName` chiudono la conversione
  testo↔keycode. Stima ~2-3 ore.
- **Step 4** — UI di rebinding (modal "Press a key…"). Deferita a §12
  così la Player UI può decidere dove vive il menu Settings.

**Discrepanza spec/codice trovata durante l'implementazione**: §17 spec
originale diceva "joypad ignora ImGui focus", ma il codice precedente
gating-eva il joypad su `!imgui_captured` esattamente come gli hotkey.
La versione refactorata preserva il comportamento attuale (joypad
gated su `!imgui_captured`). Riconciliare in un follow-up se il
"joypad pass-through senza gate" è il comportamento desiderato.

---

## 17-archived (specs originali, mantenute per memoria storica)

`Application::run` oggi mescola in un unico `while (SDL_PollEvent)` (~90 righe
in `src/app.cpp:498-592`) **quattro responsabilità diverse**:

1. **Hotkey applicativi** inline: F11 (fullscreen), Space (pause toggle),
   Ctrl+R (reset), Ctrl+O (load ROM), M (mute), Ctrl+Up/Down (volume nudge).
   Ognuno è un `else if` annidato con il proprio gating (`!imgui_captured`,
   `!e.key.repeat`, `rom_loaded`, modifier mask) e tocca direttamente
   `debugger`, `core.apu`, `user_state_`, `ui::actions(ui_ctx)`,
   `SDL_SetWindowFullscreen` — l'orchestrazione vive a mano nel main loop.
2. **Mapping joypad keyboard → GB button** in `map_keycode_to_button`
   (`src/app.cpp:126-156`): switch hardcoded su `SDLK_UP`/`Z`/`X`/…
3. **Mapping joypad controller → GB button** in
   `map_controller_button_to_button` (`src/app.cpp:164-194`): switch
   hardcoded su `SDL_CONTROLLER_BUTTON_*`.
4. **Hot-plug controller** (`SDL_CONTROLLERDEVICEADDED/REMOVED`): apri/chiudi
   `SDL_GameController*`.

**Problemi attuali:**

- Le shortcut sono *replicate* nei tooltip della menu bar (`src/ui.cpp`
  `draw_menu_bar`): qualunque rebinding richiederebbe due edit sincronizzati.
- Nessun supporto per modifiers diversi da Ctrl, niente chord (es. `G,P` per
  "Goto PC"), niente disambiguazione fra hotkey con/senza ROM caricata
  (oggi `rom_loaded` è hardcoded a poche voci).
- Le mappe joypad sono *fisse*: chi vuole WASD invece delle frecce, o
  rimappare A/B sulla tastiera, deve ricompilare. Speedrun layout (rebind
  Start/Select) idem.
- L'API `Application` non ha un punto di osservazione per "questa azione è
  stata invocata": telemetria/log delle hotkey non c'è (utile per
  debuggare "perché l'utente dice che la pause non funziona?").
- Pre-requisito **non bloccante ma fortemente correlato** a §12 (Player UI
  vs Debugger UI): la Player UI dovrà esporre un menu "Settings → Input"
  per il rebinding, e i due frontend condivideranno lo stesso input
  manager. Meglio chiudere §17 prima così §12 non duplica scaffolding.

**Architettura proposta:**

Nuovo modulo `inc/input.h` + `src/input.cpp` con tre concetti separati:

```cpp
namespace gbemu::input {

    // (1) Azioni applicative semantiche — l'enum vive in input.h e
    // viene consumata sia dall'event dispatcher (questo modulo) sia
    // dalla UI per labellare le voci di menu / disegnare le shortcut.
    enum class action {
        toggle_pause, reset, load_rom, toggle_fullscreen,
        toggle_mute, volume_up, volume_down,
        // futuri (§3): speed_up, speed_down, speed_reset, fast_forward_hold
        // futuri (§2): save_state, load_state, slot_next, slot_prev
        // futuri (§12): toggle_frame
    };

    // (2) Binding: chiave SDL + modifier mask -> action.  Modifier mask
    // è uint16 (SDL_Keymod), permette Shift/Ctrl/Alt/Gui in qualunque
    // combinazione.  `kind` discrimina hotkey one-shot dal "tieni
    // premuto" (fast_forward).
    struct key_binding {
        SDL_Keycode key;
        std::uint16_t mod_mask;
        action act;
        enum class kind { oneshot, hold } kind = kind::oneshot;
    };

    // (3) Binding joypad: SDL key/controller button -> GB joypad button.
    // Due tabelle separate (keyboard vs controller).
    struct joypad_binding {
        SDL_Keycode key;                          // 0 se controller-only
        SDL_GameControllerButton ctrl_btn;        // _INVALID se keyboard-only
        gbemu::joypad::button gb_btn;
    };

    // Configurazione completa, serializzata in user.conf (sezione 14).
    struct config {
        std::vector<key_binding> hotkeys;     // default popolato da defaults()
        std::vector<joypad_binding> joypad;   // idem
        static config defaults();             // valori hardcoded di oggi
    };

    // Dispatcher: prende SDL_Event + stato (ROM caricato? imgui catturato?)
    // e ritorna `optional<action>` per gli hotkey, oppure aggiorna lo stato
    // del joypad direttamente (callback-based per non ricostruire l'enum).
    class manager {
    public:
        explicit manager(config cfg);
        // Ritorna l'azione triggerata (se ce n'è una), altrimenti nullopt.
        // `imgui_captured` salta le hotkey ma lascia passare il joypad
        // pass-through (lo facciamo già oggi).
        std::optional<action> on_key_down(SDL_Keycode, std::uint16_t mod,
                                          bool repeat, bool imgui_captured,
                                          bool rom_loaded);
        // I joypad event chiamano direttamente joypad::set_button via
        // callback installato a costruzione (evita di accoppiare input
        // manager a core).
        bool on_key_for_joypad(SDL_Keycode, bool pressed);
        bool on_controller_button(SDL_GameControllerButton, bool pressed);
        // Per il rebinding UI: enumerare/sostituire/aggiungere binding.
        config const& cfg() const;
        void set_cfg(config);
    private:
        config cfg_;
        std::function<void(gbemu::joypad::button, bool)> joypad_sink_;
    };

} // namespace gbemu::input
```

**Wiring in `Application::run`:**

- Costruire `input::manager mgr{ input::config::defaults() }` (più avanti:
  load da `user_state_.input` se presente).
- Sostituire i ~90 righe dell'event loop con un dispatch table:
  ```cpp
  if (e.type == SDL_KEYDOWN) {
      const auto act = mgr.on_key_down(e.key.keysym.sym, e.key.keysym.mod,
                                       e.key.repeat, imgui_captured,
                                       core.mmu.cart() != nullptr);
      if (act) handle_action(*act);     // switch su action enum
      mgr.on_key_for_joypad(e.key.keysym.sym, true);
  }
  ```
  Il `handle_action(action)` è una funzione membro/lambda con il `switch`
  che oggi vive inline — concentra tutta l'orchestrazione (debugger /
  apu / user_state / ui::actions) in un punto solo.

**Persistenza (dipende da §14, già chiusa):**

Nuovo sub-block `input` in `user/user.conf`:
```
input: {
  hotkeys = (
    { key = "F11";       mod = "";     action = "toggle_fullscreen"; },
    { key = "Space";     mod = "";     action = "toggle_pause"; },
    { key = "R";         mod = "Ctrl"; action = "reset"; },
    ...
  );
  joypad_keyboard = (
    { key = "Up";        gb = "up"; },
    { key = "Z";         gb = "a";  },
    ...
  );
};
```

Sub-block mancante → `config::defaults()` (no migration needed).
`SDL_GetKeyFromName(string)` / `SDL_GetKeyName(SDL_Keycode)` chiudono la
conversione testo↔keycode.

**UI di rebinding (deferred a §12 / dopo):**

- Pannello/finestra "Settings → Input" con tabella `[action | shortcut |
  Rebind…]`. Click su "Rebind…" → modal "Press a key…", il prossimo
  `SDL_KEYDOWN` cattura la combinazione e aggiorna `config`.
- Per joypad, stessa idea con `SDL_CONTROLLERBUTTONDOWN`.
- "Reset to defaults" overwrita con `config::defaults()`.
- Non bloccante per il refactor: i default fanno girare tutto come oggi,
  la UI può atterrare in un secondo momento (priorità più bassa dopo che
  §12 ha deciso dove vive il menu Settings).

**Conflitti / vincoli:**

- Hotkey *non devono* collidere con le hotkey joypad (oggi è verificato a
  mano: arrows/Z/X/Backspace/Enter non sono mappati a hotkey). Il
  manager va difeso con una validazione a load (se un binding hotkey usa
  una key già mappata sul joypad → log warning, skip). Stessa logica
  inversa.
- ImGui-captured: hotkey applicativi *sì* skippano (text field con focus),
  joypad *no* (il joypad pass-through ignora il focus ImGui — match con
  l'attuale comportamento).
- Modifier mask: oggi `Ctrl+R` controlla solo `KMOD_CTRL`, non discrimina
  Shift/Alt aggiuntivi. Decidere se il check è "mod == required_mask"
  (strict) o "(mod & required_mask) == required_mask" (allow extras).
  Strict è il default ragionevole — `Ctrl+Shift+R` *non* deve triggerare
  reset.

**Cosa NON è in scope:**

- Gesture multi-tasto / chord (`G,P` style) — overkill per v1, si può
  aggiungere come campo `key_binding.next` (key successiva) quando serve.
- Macro / replay input — feature separata, niente a che fare con
  bindings.
- Touch input (Android/iOS) — non shippiamo lì.

**Ordine consigliato:**

1. Estrarre `input::config` + `input::manager` con i default hardcoded di
   oggi (no persistenza). Sostituire le ~90 righe in `Application::run`
   con il nuovo dispatch. Verifica: hotkey + joypad si comportano
   identici a prima.
2. Spostare i tooltip di shortcut nella menu bar (`src/ui.cpp`) a leggere
   da `input::config` invece di stringhe hardcoded (es. `View →
   shortcut_label(mgr.cfg(), action::toggle_pause)` → `"Space"`). Tutto
   pronto per il rebinding UI senza duplicare verità.
3. Persistenza in `user_state_.input` (legge/scrive sub-block libconfig).
   Default kicks in se mancante; binding invalidi → log + skip.
4. (Più avanti, dopo §12) UI di rebinding nel menu Settings.

**Dipendenze:**

- Nessuna *in ingresso*: il refactor è autonomo. §14 (`user/user.conf`)
  facilita la persistenza ma non è bloccante — i passi 1-2 si chiudono
  senza toccare il filesystem.
- *In uscita*: §3 (speed multiplier) e §2 (save/load state) aggiungeranno
  facilmente le proprie `action` (`speed_up`/`speed_down`/`fast_forward_hold`,
  `save_state`/`load_state`/`slot_*`) senza altri edit in `app.cpp`. §12
  (Player UI) eredita lo stesso input manager senza scaffolding extra.

**Costo stimato**: ~mezza giornata per i passi 1-2 (refactor meccanico,
copertura test = "le hotkey funzionano come prima" verificata manualmente).
Passo 3 (persistenza + parsing libconfig) ~2-3 ore. Passo 4 (UI rebinding)
~mezza giornata, da fare insieme alla scelta del posto in cui vivere
(Settings panel? menu Edit → Preferences?).

---

## Note tecniche permanenti

- **Path config**: `cfg/gbemu.conf` non ha più path assoluti — il portable
  layout (`<base>/{cfg,bios,roms,savs,sslots}/`) è in piedi via
  `gbemu::paths::resolve_base_dir()` + `resolve_data_path()` (commit
  `ab7bb97`). Le quattro sub-directory sono user-overridable in
  `cfg.paths.{bios,roms,savs,sslots}_dir`.
- `CMakeUserPresets.json` pinna `VCPKG_ROOT` locale: tenere fuori dai workflow
  cross-machine (già menzionato in `CLAUDE.md`).
- `inc/opcodes.hpp` è generato — modificare solo `scripts/gen_ops.py` o
  rigenerare; `src/opcodes.cpp` invece è hand-maintained (la stringa
  "Generated from ops_db.json" in cima è stale).
