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

## 3. Moltiplicatore di velocità

Feature UX: x0.25, x0.5, x1.0, x1.5, x2.0, x4.0. Più fast-forward uncapped
"tieni premuto" come modalità separata.

**Stato**: vive sull'`Application` (è frame-pacing, non emulation state).
Hotkey suggerite: `+` / `-` per step discreti, `Tab` (tieni premuto) per
fast-forward uncapped. Entry esposta anche come combo nel menu ImGui
("Emulation → Speed") + indicatore corrente in titlebar.

**Implementazione**:
- Il main loop oggi esegue `frame_cycles = 70224` T-cycle per VSync tick.
  Per `multiplier > 1.0` → `frame_cycles = round(70224 * multiplier)` (più
  emulazione per frame reale). Per `multiplier < 1.0` → si lascia
  `frame_cycles = 70224` e si introduce un `SDL_Delay` di
  `(1/multiplier - 1) * frame_time_ms` (meno emulazione per frame reale).
- Fast-forward uncapped: scollegare dal VSync (`SDL_RenderSetVSync(0)` o
  swap-interval 0) e iterare quante volte possibile entro un budget wall-clock
  (es. 16 ms).

**Audio**: a velocità ≠ 1.0 il pitch cambia se non si resample. Scelta v1 =
mute automatico quando `multiplier != 1.0` (semplice e accettata). Resampling
(SoundTouch o simile) resta come follow-up.

**Interazione con il debugger**: in modalità Paused il moltiplicatore non ha
effetto (lo step è manuale). Su `Step` singolo idem. Il moltiplicatore si
applica solo nel ramo `run_until(stop_kind::none, frame_cycles)` del main loop.

**Headless**: non si applica — `script_runner` gira sempre full-speed (è il
suo punto).

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

## 5. MBC2 / MBC3 / MBC5 — **fatto (RTC live, persistence aperta)**

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

**Cosa resta:**
- Battery save persistente → sezione 6 (essenziale per giocare seriamente a
  Pokémon).
- Serializzazione RTC su disco per sopravvivere ai restart → sezione 6.

---

## 6. Battery save (SRAM persistente su cartuccia)

Giochi come Zelda: Link's Awakening (MBC1+RAM+BATTERY, type `0x03`), Pokémon
(MBC3+RAM+BATTERY+RTC) e in generale tutti i titoli con salvataggio interno
scrivono il progresso in RAM esterna mantenuta da una batteria sulla cartuccia.
Oggi `src/mbc.cpp` distingue già nei commenti i tipi BATTERY (0x03, 0x06, 0x09,
0x0F, 0x10, 0x13, 0x1B, 0x1E) ma la RAM è solo allocata in memoria e muore al
process exit.

**Cosa serve:**

- Rilevamento header: tipi battery-backed sono `0x03`, `0x06`, `0x09`, `0x0F`,
  `0x10`, `0x13`, `0x1B`, `0x1E`. Tutti gli MBC necessari (MBC1/2/3/5) sono già
  implementati — vedi §5.
- API sull'`mbc`: `ram_load(span<const uint8_t>)`, `ram_data() const`,
  `ram_dirty() const` (flag settato da ogni write a `$A000-$BFFF`),
  `ram_clear_dirty()`. Già virtuali sulla base in modo che no_mbc/mbc1/…
  possano fare override.
- All'avvio (`core::load(rom_file)`): se l'MBC è battery, cercare il file
  `.sav` sotto `<base>/savs/` e caricarlo via `ram_load`. Dimensione attesa =
  quella decodificata da `ram_size_code`; mismatch → log warning + ignore.
- Persistenza: flush a chiusura applicazione (sempre) + flush periodico
  (~2 s se `ram_dirty`) per sopravvivere ai crash. Scrittura atomica
  (`.sav.tmp` + rename).
- Path: lo schema è già definito dal **portable layout**
  (`<base>/savs/<fnv1a>.sav` — vedi `CLAUDE.md` § Portable layout). La
  chiave FNV1a sull'intero ROM rende il salvataggio rename-stable
  (rinominare `zelda.gb` non rompe il `.sav`). `paths::resolve_data_path` +
  `cfg.paths.savs_dir` sono già pronti — basta agganciare il caricamento /
  flush nel ciclo di vita di `core::load`.

**Formato**: `.sav` = dump raw della SRAM, byte-per-byte. Compatibile con
BGB, mGBA, SameBoy, VBA-M, ecc. dal punto di vista del *contenuto*; il
*nome file* è specifico (FNV1a invece di `<basename>.sav`), quindi
l'import/export richiede un rename manuale. Decisione consapevole — il
rename-stable degli FNV1a vale la perdita di drop-in compatibility.

**RTC (MBC3)**: lo stato live esiste già in `mbc3` (sezione 5) — manca solo
la serializzazione. Schema concreto:

- File `.rtc` separato accanto al `.sav` (stesso FNV1a key:
  `<base>/savs/<fnv1a>.rtc`). Tenerli separati semplifica l'import/export e
  permette di azzerare la RTC senza toccare la SRAM.
- Payload: `rtc_total_secs_` (int64 LE), `last_real_unix_` (int64 LE),
  `rtc_halted_` (uint8), `rtc_day_carry_` (uint8), `latched_*` (5 byte
  S/M/H/DL/DH). 24 byte totali — formato fisso, niente versioning per ora
  (è un file privato dell'emulatore).
- API sull'`mbc3` (e nulla sugli altri MBC): `rtc_load(span<const uint8_t>)`,
  `rtc_data() const -> array<uint8_t,24>`, oppure rendere virtuale sulla
  base `mbc::rtc_data() const -> std::optional<...>` che ritorna `nullopt`
  per chi non ha RTC.
- Su `rtc_load`: dopo aver letto i campi serializzati, calcolare
  `wall_gap = now_unix() - saved_last_real_unix` e — se non halted —
  sommarlo in `rtc_total_secs_` prima di resettare l'anchor a `now_unix()`.
  Così i giorni accumulati sopravvivono al restart e l'orologio "ha
  continuato a girare" mentre il processo era spento. Halted → `rtc_total_secs_`
  resta congelato.
- Persistenza: stessa cadenza del `.sav` (flush a chiusura + ogni ~2 s se
  qualcosa è cambiato). La RTC cambia ogni secondo quindi conviene gating
  su "ultimo dump > N secondi fa" invece di un dirty bit.
- Formato compatibility: il layout sopra **non** è interoperabile con BGB /
  VBA / SameBoy (ognuno usa un proprio formato `.rtc`). Decisione
  consapevole — la complessità non vale per la v1, l'utente può ricalibrare
  la RTC in-game al primo boot dopo un import.

Senza questo file, la RTC riparte all'epoch del `system_clock` corrente ad
ogni avvio: i secondi/minuti/ore puntano all'ora reale di sistema, ma il
counter "giorni dall'inizio della partita" si azzera ad ogni restart →
day/night funziona per la sessione corrente, non sopravvive ai riavvii.

**Distinzione da Save State (sezione 2)**: il battery save è solo la SRAM
del cart, è il salvataggio *del gioco* (il giocatore lo crea via menu
in-game, "Save and continue"). Save state = snapshot completo dell'emulatore
(hotkey). Le due feature coesistono.

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
- `interrupt_time/interrupt_time.gb` ❌ — `REQUIRE_CGB 1`, indipendente.
- `dmg_sound` 09:01 / 10:01 / 12:01 ❌ — wave RAM access bug del CH3,
  indipendente dall'M-cycle, vedi §8.

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

- Popolare `Speed ▸` con le radio entries (`0.25x`…`4.0x`) — dipende §3.
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

## 14. Cartella `user/` per i file di sessione

Oggi i file di stato della sessione vivono sparsi nel **CWD** del processo
(non sotto la base portable):

- `gbemu_window.state` — `src/app.cpp:45` (`WINDOW_STATE_FILE`), aperto come
  bare filename → finisce in `build/src/` quando si lancia da lì.
- `gbemu_recent.txt` — `src/ui.cpp:47` (`RECENT_ROMS_FILE`), stessa storia.
- `imgui.ini` — default di ImGui, sempre relativo al CWD.
- Palette attiva (`feature/palette`) — `ui::context.active_palette_name` non è
  ancora persistita; quando lo sarà, va nello stesso posto.

Conseguenza: cambiando CWD si "perdono" finestre, recents, layout, palette.
Non è coerente col portable layout di `cfg/`, `bios/`, `roms/`, `savs/`,
`sslots/`, `palettes/` che invece risolvono via
`gbemu::paths::resolve_base_dir()`.

**Cosa serve:**

- Aggiungere `cfg.paths.user_dir` con default `"user"` (in `inc/cfg.h` e
  `src/cfg.cpp`), creato da CMake accanto agli altri (`MAKE_DIRECTORY`).
- Helper `paths::user_file(base, cfg, name)` che restituisce
  `<base>/<user_dir>/<name>` (riusa `resolve_data_path`).

**Un solo `user/user.conf` invece di N parser**: tutto quello che è
serializzabile come chiave/valore (o lista di stringhe) finisce in un
unico file libconfig — riusa lo stesso `Config` / `Setting` machinery già
in piedi per `cfg/gbemu.conf`. Schema iniziale:

```
window:
{
  width = 1280;
  height = 720;
  x = 100;
  y = 100;
  maximized = false;
};

recent_roms = ( "C:/roms/zelda.gb", "C:/roms/pokemon_red.gb", ... );

display:
{
  active_palette = "dmg-green";
};
```

- API: `user_state` class in `inc/user_state.h` / `src/user_state.cpp`
  con `load(path)`, `save(path)`, getter/setter tipati. Save atomico
  (`.tmp` + rename), salvataggio on-change o a chiusura.
- Migrare i siti d'uso:
  - `src/app.cpp`: blocco `gbemu_window.state` → `user_state.window_*`.
  - `src/ui.cpp`: load/save di `gbemu_recent.txt` → `user_state.recent_roms`.
  - Palette attiva: `user_state.active_palette` — niente nuovo file.
- File futuri (§12 Player/Debugger UI split): aggiungere
  `window_player`/`window_debugger` come sub-block, **non** nuovi file.

**Eccezione: `imgui.ini`**. Formato proprietario di ImGui, lo gestisce lui;
non si può fondere in `user.conf`. Va però spostato sotto `user/` settando
`ImGui::GetIO().IniFilename` a una stringa owned (path assoluto risolto
via `paths::user_file`). Nello split §12: `user/imgui_player.ini` /
`user/imgui_debug.ini`.

**Migrazione**: rottura compat accettata (è v0.x). Niente fallback / move
automatico — chi aveva i file in CWD li ricrea al primo avvio. Documentare
in `CLAUDE.md` § Portable layout aggiungendo `user/` alla lista delle
sotto-directory.

**Distinzione da `cfg/`**: `cfg/gbemu.conf` è la **configurazione** editata
dall'utente (preferenze esplicite). `user/` è **stato di sessione**
generato e gestito dall'app (window pose, MRU, dock layout, ultima palette
selezionata). Tenerli separati evita di mescolare "cose che l'utente edita"
con "cose che l'app riscrive in continuazione".

---

## 15. Menu Audio — abilita/disabilita audio

Oggi l'APU gira sempre e il device SDL audio è aperto in `Application` senza
controllo utente. Manca un toggle UI per silenziare l'output (use-case classico:
"sto ascoltando musica mentre gioco").

**Menu bar**: aggiungere voce **Audio** fra **Emulation** e **View** in
`draw_menu_bar` (`src/ui.cpp`). Voci iniziali:

- `Mute` (checkable) — toggle stato muted. Hotkey suggerita `M` (gated su
  `!imgui_captured`, non collide con joypad bindings).
- (Follow-up, non bloccanti per la v1) `Volume ▸` submenu con slider 0-100%,
  toggle per-canale `Channel 1/2/3/4` (utile per debug APU).

**Implementazione mute:**

- Stato vive sull'`Application` (è output-side, non emulation state — l'APU
  continua a girare per non desyncare la timeline cycle-accurate).
- Due approcci possibili:
  - **Gate sul producer**: nel callback di `apu::sample_pull` (o equivalente
    SPSC nel ring buffer) scrivere zeri quando muted. Pro: nessun pop
    all'attivazione (il ring già pieno suona ancora per ~1 frame, poi
    silenzio). Contro: spreca cicli APU.
  - **Gate sul consumer SDL**: `SDL_PauseAudioDevice(dev, muted ? 1 : 0)`.
    Pro: zero CPU. Contro: pop all'unpause se il ring ha campioni stale —
    serve un drain del ring su unmute.
  - Scelta consigliata v1: **consumer-gate** + drain. Pop occasionale a
    unmute è accettabile per un toggle utente; il risparmio CPU vale.
- Persistenza: stato muted in `user_state` (sezione 14) sotto `audio.muted`.
  Finché §14 non è chiusa, vivere in memoria e dimenticarsi al riavvio.

**Interazione con §3 (speed multiplier)**: la §3 prevede già "mute automatico
quando `multiplier != 1.0`". Tenere i due flag separati (`user_muted` vs
`speed_muted`, OR per il gating effettivo) così la velocità non sovrascrive
la scelta esplicita dell'utente.

**Dipendenze**: nessuna bloccante. Implementabile subito; integrazione con
`user_state` arriva con §14.

**Costo stimato**: ~1-2 ore per Mute+hotkey; il volume slider + per-channel
toggles aggiungono altre 2-3 ore se si vogliono fare bene (curva di volume
log, non lineare).

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
