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

## 5. MBC2 / MBC3 / MBC5 — **fatto (RTC stubbed)**

`src/mbc.cpp` ora implementa `no_mbc`, `mbc1`, `mbc2`, `mbc3`, `mbc5`. Tutti i
cartridge type 0x00-0x1E mappati nel factory `make_mbc`. Pokémon Red/Blue
(type 0x13, MBC3+RAM+BATTERY) parte; il salvataggio in-gioco funziona ma resta
solo in RAM finché il processo è vivo (battery save → sezione 6).

**MBC2** (0x05, 0x06): 4-bit ROM bank, RAM built-in 512 nibbles mirrored, bit 8
dell'addr distingue RAM-enable vs ROM-select.

**MBC3** (0x0F-0x13): 7-bit ROM bank con 0→1 remap, RAM/RTC select a
$4000-$5FFF (banks 0-3 oppure RTC reg 0x08-0x0C), latch a $6000-$7FFF.
**RTC = stub**: read ritorna 0, write ignorate, no time-keeping. Gold/Crystal
girano con day/night cycle congelato — accettabile per la v1. RTC vero
richiede `time(NULL)` baseline + emulazione del DIV interno → addendum
opzionale che vive insieme alla battery-save (`.rtc` file).

**MBC5** (0x19-0x1E): 9-bit ROM bank (low8 a $2000-$2FFF, bit9 a $3000-$3FFF,
bank 0 valido, no remap), 4-bit RAM bank a $4000-$5FFF. Bit di rumble
ignorato (no haptics).

`mbc_debug_state::rom_bank` allargato a `uint16_t` perché MBC5 può
selezionare fino a bank 511. UI / debugger `dump mbc` già castano a
`unsigned` quindi nessun consumer rotto.

**Cosa resta:**
- Battery save persistente → sezione 6 (essenziale per giocare seriamente a
  Pokémon).
- RTC reale per MBC3 timer carts → addendum di sezione 6.

---

## 6. Battery save (SRAM persistente su cartuccia)

Giochi come Zelda: Link's Awakening (MBC1+RAM+BATTERY, type `0x03`), Pokémon
(MBC3+RAM+BATTERY+RTC) e in generale tutti i titoli con salvataggio interno
scrivono il progresso in RAM esterna mantenuta da una batteria sulla cartuccia.
Oggi `src/mbc.cpp:207-219` distingue già nei commenti i tipi BATTERY (0x03,
0x09) ma la RAM è solo allocata in memoria e muore al process exit.

**Cosa serve:**

- Rilevamento header: tipi battery-backed da gestire sono `0x03`, `0x06`,
  `0x09`, `0x0F`, `0x10`, `0x13`, `0x1B`, `0x1E` (alcuni dipendono da MBC
  ancora da implementare in sezione 5 — si avanza in parallelo).
- API sull'`mbc`: `ram_load(span<const uint8_t>)`, `ram_data() const`,
  `ram_dirty() const` (flag settato da ogni write a `$A000-$BFFF`),
  `ram_clear_dirty()`. Già virtuali sulla base in modo che no_mbc/mbc1/…
  possano fare override.
- All'avvio (`core::load(rom_file)`): se l'MBC è battery, cercare
  `<rom_path>.sav` accanto al ROM e caricarlo via `ram_load`. Dimensione
  attesa = quella decodificata da `ram_size_code`; mismatch → log warning +
  ignore.
- Persistenza: flush a chiusura applicazione (sempre) + flush periodico
  (~2 s se `ram_dirty`) per sopravvivere ai crash. Scrittura atomica
  (`.sav.tmp` + rename).
- Path: di default accanto al ROM (`zelda.gb` → `zelda.sav`); opzionale
  override via `cfg/gbemu.conf` (`save.directory`).

**Formato**: `.sav` = dump raw della SRAM, byte-per-byte. Compatibile con
BGB, mGBA, SameBoy, VBA-M, ecc. — il giocatore può portarsi il salvataggio
fra emulatori.

**RTC (MBC3)**: addendum opzionale, file `.rtc` separato (base time + ultimi
valori dei registri RTC). Molti emulatori non lo fanno e la RTC riparte da
zero — rimandabile, non blocca i giochi.

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

## 11. Menù bar funzionale

Oggi `src/ui.cpp` (`draw_main_dockspace`, ~riga 167) ha una sola voce **View**
che toggla i pannelli ImGui + Reset layout. Mancano tutte le entry "da
emulatore": caricamento ROM, controllo emulazione, slot di salvataggio, info.

**Struttura proposta** (in ordine, da sinistra a destra):

- **File**
  - `Load ROM…` — apre file dialog nativo (Windows: `GetOpenFileNameW`;
    portabile: `nfd` / `tinyfiledialogs` da aggiungere a `vcpkg.json`).
    Filtro `*.gb;*.gbc`. Selezione → ricostruzione del `core` con la nuova
    ROM (oggi il path è fisso in `cfg/gbemu.conf`, va spezzato il vincolo:
    `core` deve poter essere ri-inizializzato a runtime — interagisce con
    il `Reset` button della sezione 1).
  - `Recent ROMs ▸` — submenu con gli ultimi N (8?) ROM caricati. Persistenza
    in un file accanto a `imgui.ini` (es. `gbemu_recent.txt`, una riga per
    path); aggiornato dopo ogni `Load ROM`. Voce `Clear list` in fondo.
  - `Exit` — `SDL_PushEvent(SDL_QUIT)`. Hotkey `Alt+F4` resta nativo.

- **Emulation**
  - `Start` / `Pause` — toggle che riusa `debugger::pause()` /
    `debugger::resume()` (oggi guidati dai pulsanti del pannello CPU).
    Hotkey `F5` o `Space` (configurabile).
  - `Reset` — re-init `core` + clear RAM/VRAM. Stessa azione del Reset
    button (sezione 1) — le due UI devono chiamare la *stessa* funzione,
    non duplicarla. Hotkey `Ctrl+R`.
  - `Speed ▸` — sub-menu con radio entries `0.25x` / `0.5x` / `1.0x` /
    `1.5x` / `2.0x` / `4.0x`. Indicatore corrente in titlebar. Dipende
    dalla sezione 3 (moltiplicatore di velocità).
  - `Save State ▸` / `Load State ▸` — sub-menu con slot `0..9`.
    Dipende dalla sezione 2 (save & load state). Hotkey `F5` (save) /
    `F9` (load) sullo slot corrente; `Shift+0..9` per cambiare slot.
    Le voci del menu mostrano timestamp / "empty" per ogni slot.
  - `Toggle Fullscreen` — `SDL_SetWindowFullscreen` con
    `SDL_WINDOW_FULLSCREEN_DESKTOP`. Hotkey `F11`.

- **About**
  - `About GbEmu…` — popup modale con nome, versione (dalla sezione 10),
    repo URL, build date, copyright. Pulsante `OK` + `Copy version info`
    che mette in clipboard una stringa diagnostica
    (`gbemu vX.Y.Z, SDL2 a.b.c, ImGui d.e.f, ...`) utile per le bug
    report.

**Considerazioni implementative:**

- Le hotkey vivono nello stesso punto in cui oggi è gestito F12 (interactive
  toggle): `Application::run` event loop. ImGui captura i tasti quando un
  text field ha focus — la branch va dopo `process_event` returning false.
- `Recent ROMs` e `Save State` slot list vanno costruiti dinamicamente in
  `BeginMenu` con `ImGui::MenuItem(label.c_str(), shortcut, false, enabled)`
  — il flag `enabled = false` per slot vuoti rende il menu auto-esplicativo.
- File dialog **non** può girare in fase di render ImGui se il dialog è
  modale-bloccante (rischia di starvare il loop e la GPU). Pattern:
  settare un flag (`c.load_rom_requested = true`), aprire il dialog
  fuori dal blocco `NewFrame / Render`, all'inizio del frame successivo.
- Tutte le voci che dipendono da feature non ancora implementate (Speed,
  Save State) vanno mostrate `disabled` finché la rispettiva sezione è
  chiusa — meglio che farle sparire (l'utente non capisce dove sono).

**Dipendenze**: sezione 1 (Reset), sezione 2 (save state), sezione 3
(speed). Lo scaffolding del menu si può fare prima — voci stub disabled
+ Load ROM + Exit + About — e popolare il resto man mano che le feature
sottostanti atterrano.

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

## Note tecniche permanenti

- `cfg/gbemu.conf` shipped ha path **assoluti** Windows per ROM e BIOS — va
  reso relativo prima del packaging.
- `CMakeUserPresets.json` pinna `VCPKG_ROOT` locale: tenere fuori dai workflow
  cross-machine (già menzionato in `CLAUDE.md`).
- `inc/opcodes.hpp` è generato — modificare solo `scripts/gen_ops.py` o
  rigenerare; `src/opcodes.cpp` invece è hand-maintained (la stringa
  "Generated from ops_db.json" in cima è stale).
