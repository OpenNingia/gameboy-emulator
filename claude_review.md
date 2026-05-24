Code review — gbemu

  TL;DR

  Il codice è arrivato a uno stato funzionale solido. I tre obiettivi che hai dato (leggibilità, apertura a espansioni, naming) sono mediamente rispettati nelle parti scritte di recente (PPU, APU, MBC, joypad),
   ma c'è un mismatch fra la qualità di scrittura locale e l'architettura globale: i singoli file sono leggibili, ma i confini fra sottosistemi sono confusi e i moduli si parlano via byte grezzi nell'MMIO. Quel
   coupling è quello che ti morderà al passaggio OpenGL/CGB. I problemi peggiori non sono "bug" ma decisioni che pagi ogni volta che leggi il codice (magic numbers, dispatch infrastructure) o ogni volta che
  vuoi estenderlo (CGB banking, rendering backend, doppia velocità).

  Sotto il dettaglio. Numero le sezioni per riferimento, non per priorità.

  ---
  A. Architettura: le scelte grosse da rivedere

  A1. L'MMU è un bag of bytes pubblico, non un'astrazione di memoria

  Tutti i sottosistemi conoscono il layout interno dell'MMU e lo bypassano:

  - inc/mmu.h:41-55 — bios, vram, wram, sram, mmio, zram sono public.
  - src/timer.cpp:15,28 — mmu_.mmio[4] = 0; e mmu_.mmio[4]++; direttamente.
  - src/joypad.cpp:38,41,46,52 — manipola mmu_.mmio[0x0F] e mmu_.mmio[0] per IF e P1.
  - src/dma.cpp:13 — mmu_.sram[i] = ... (DMA scrive in OAM direttamente).
  - src/ppu.cpp:160,166,167,211,217,218,290,291 — m.vram[...] 8 volte; m.sram[i * 4 + ...] per OAM scan.
  - src/ui.cpp:548,573 — c.core->mmu.vram.data() per i tile-viewer.
  - src/apu.cpp (78 occorrenze 0xFF…) — wave RAM via puntatore in mmu.mmio + 0x30.

  I DEF_HWREG setter/getter sopra ci sono, ma per i sottosistemi interni non hanno significato: scrivere hwr_div(0) rientra nei write handler e ricade in ricorsione (vedi infatti il commento mesto in
  src/timer.cpp:14). La conseguenza è che esiste un'API "pubblica" read_u8/write_u8 (per CPU/debug) e un'API implicita "scrivo direttamente nell'array" per la circuiteria — non documentata, non centralizzata.

  Perché diventa un problema per CGB: in CGB la VRAM è doppia (banco 0/1 selezionato da VBK $FF4F), la WRAM è bancata 1-7 (SVBK $FF70), e ci sono nuovi spazi di paletta indicizzati (BGPD $FF69, OBPD $FF6B).
  Tutti i siti che oggi fanno mmu.vram[off] sono codice da rivedere uno per uno. Lo stesso vale se vuoi mai aggiungere savestate (oggi non c'è un punto unico da serializzare).

  Direzione: rendere vram/wram/sram/mmio/zram private. Esporre:
  std::uint8_t mmu::vram_read(std::uint16_t off, std::uint8_t bank = 0) const;
  void          mmu::vram_write(std::uint16_t off, std::uint8_t val, std::uint8_t bank = 0);
  std::span<const std::uint8_t> mmu::vram_bank(std::uint8_t bank) const; // per il PPU bulk-read
  std::uint8_t  mmu::oam_read(std::uint8_t idx) const;
  // ecc.
  Tutti i siti elencati sopra passano per quei metodi. Per CGB diventa un add di un parametro bank, non una sweep. Per savestate scrivi un serialize(archive) su mmu e basta.

  Lavoro grosso ma compounding: ogni feature dopo questa diventa più facile.

  A2. Rendering accoppiato a SDL_Renderer

  Hai un solo target grafico in mente — SDL_Renderer + ImGui SDLRenderer2 backend:

  - src/app.cpp:239,244,336,339-341 — renderer SDL, texture ARGB8888 streaming 160×144, SDL_UpdateTexture + SDL_RenderClear/Present.
  - src/ui.cpp:202 — ImGui::Image(reinterpret_cast<ImTextureID>(gb_texture), ...) casta direttamente un SDL_Texture*.
  - src/ui.cpp:540-585 — i tile-viewer e bg-map viewer creano SDL_Texture lazy in ui::context.

  ppu::framebuffer() ti restituisce un const uint32_t* ARGB — quello è un contratto pulito e non dipende dal backend. Ma tutto il resto della pipeline grafica è SDL_Renderer-only. Per andare a OpenGL/shaders
  devi:

  - Sostituire il backend ImGui (imgui_impl_sdlrenderer2 → imgui_impl_opengl3).
  - Sostituire il SDL_Texture* con un GLuint (o un handle astratto).
  - Avere un punto in cui applicare lo shader CRT/green-LCD (oggi non c'è — la texture va dritta a ImGui::Image).

  Direzione: introdurre un piccolo presenter opaco che incapsula "qui c'è un framebuffer 160×144, mostralo nella regione (x,y,w,h) di una superficie ImGui". I tile/map viewer del PPU panel diventano lo stesso
  ma con buffer 128×192 / 256×256.

  struct presenter; // opaque
  presenter* presenter_create(const presenter_desc& d);  // sdl_renderer | opengl | …
  void       presenter_upload(presenter*, const uint32_t* pixels, int w, int h);
  ImTextureID presenter_imgui_id(presenter*);            // l'astrazione che ti permette di passare a OpenGL senza toccare ui.cpp
  void       presenter_set_shader(presenter*, shader_handle); // futuro green-LCD
  La differenza visibile: ui.cpp non vede più SDL_Texture, e quando passi a OpenGL tocchi solo l'implementazione di presenter. Il punto è separare cosa viene visualizzato (un buffer ARGB) da come (backend +
  post-process).

  A3. PPU hardcoda DMG dappertutto

  - src/ppu.cpp:130,178,232 — la palette ARGB DMG è ricopiata 3 volte nello stesso file.
  - src/ui.cpp:515 — quarta copia (kDmgPalette).
  - Tutti i lcdc & 0x08, lcdc & 0x10, lcdc & 0x20, lcdc & 0x40 sono inline.
  - render_bg_scanline non ha modo di applicare gli attributi CGB BG (palette index, bank, priority, flip) che vivono nel banco 1 della VRAM.

  Per CGB la pipeline è abbastanza diversa: per ogni tile c'è un attribute byte nel banco 1, le palette diventano 8×4 indici a 15-bit RGB. Non si "aggiunge" — si refactora.

  Direzione minimale:
  - Estrarre una palette_resolver che produca un colore ARGB dato (palette_id, color_index). Su DMG legge BGP/OBP0/OBP1. Su CGB leggerà BGPD/OBPD. Un solo posto da cambiare.
  - Spezzare render_bg_scanline in: "per ogni pixel produci (color_index, palette_id, priority)" + "risolvi in ARGB". Il primo passo è facile da estendere CGB; il secondo è il palette_resolver.
  - Sostituire i 0x08/0x10/0x20/0x40 con costanti nominate (vedi B più sotto).

  A4. Dispatch CPU: 500 classi vuote, virtual call, heap-allocation in static init

  inc/opcodes.hpp:28-37 definisce DEF_INSTR come un intero tipo (struct nome : instruction { … }). src/opcodes.cpp istanzia 500 globali (gbemu::instructions::x##_{}), ciascuno con un std::string mnemonic
  (heap-allocated) e una virtual function. Costo: 500 heap allocs all'avvio + indirect call per ogni istruzione eseguita + un livello di tipo per ogni opcode (esplora instructions::ld_b_a, instructions::ld_b_c,
   … nello stesso namespace).

  A livello di leggibilità è il pattern più costoso del progetto: 500 dichiarazioni e altrettante istanze, divise fra header generato e cpp hand-maintained, sincronizzate tramite il preprocessore. Per capire
  dove sta il body di ld_b_a devi grep-are IMPL_INSTR(ld_b_a); per capire perché esiste devi seguire DEF_INSTR. Macro che si espandono in tipi.

  Direzione: una di queste due:

  (a) Free functions + table di puntatori. Le 500 classi diventano 500 funzioni libere void op_ld_b_a(cpu&);. La tabella diventa std::array<void(*)(cpu&), 256> dispatch{}; popolata da una funzione
  init_dispatch() chiamata in core::core() (o constexpr-popolata con designated initializers). Nessuna virtual, nessuno static-init, niente std::string per opcode. La mnemonic — se ti serve runtime — vive già
  in disasm_table.hpp.

  (b) Un singolo switch generato. Lo script gen_ops.py emette void cpu::dispatch(uint16_t op) con un switch (op & 0xFF) di 256 case che chiamano op_<name>(*this). Lo fanno mGBA, SameBoy, gambatte: è la forma
  più semplice da debuggare passo passo.

  In entrambi i casi: niente più macro che si espandono in tipi, niente più static-initialization globale.

  Costo: medio-alto, ma è codice che tocchi una volta sola e non guardi più.

  A5. Cycle accounting istruzione-per-istruzione, non M-cycle

  Già nel TODO.md. Lo riporto qui perché incrocia direttamente CGB: in CGB il double-speed mode (KEY1 $FF4D) raddoppia il clock CPU ma lascia PPU/APU/DMA al rate normale. Con core::step() (src/core.cpp:50-74)
  che avanza tutti i sottosistemi della stessa somma di T-cycles dopo che l'istruzione è completa, modellare quello è macchinoso (servirebbe un fattore moltiplicativo separato per CPU vs resto, e perdi il
  timing M-cycle interno).

  Se hai intenzione seria di puntare a CGB, M-cycle accounting è un prerequisito, non un miglioramento.

  A6. Subsistemi comunicano tramite bit MMIO grezzi

  Vedi i tanti mmu.hwr_if() | (1 << bit) sparsi:

  - src/ppu.cpp:323 — m.hwr_if(m.hwr_if() | (1 << bit)).
  - src/timer.cpp:43 — mmu_.hwr_if(mmu_.hwr_if() | 0x04).
  - src/joypad.cpp:41 — mmu_.mmio[0x0F] | 0x10 (e bypassa hwr_if).

  Ogni sottosistema sa che IF è a $FF0F, conosce il proprio bit, e va a scriverci direttamente. C'è già un irq ma fa solo dispatch, non request. Un'aggiunta semplice:

  enum class irq_source : uint8_t { vblank=0, lcd_stat=1, timer=2, serial=3, joypad=4 };
  void irq::request(irq_source s);   // OR del bit nel reg IF

  Niente magic bit, niente magic indirizzo, niente bypass del setter MMU. Il joypad cambia da mmu_.mmio[0x0F] | 0x10 a irq.request(irq_source::joypad). È un piccolo refactor ma elimina 5-6 punti di confusione
  che ricorrono sempre uguali.

  ---
  B. Magic numbers — il problema #3 della tua wishlist

  Hai detto esplicitamente che 0xFF40 per te è un numero magico. È sparso ovunque: 125 occorrenze di 0xFF.. in 12 file. Anche i singoli bit dei registri sono numeri nudi.

  Sintomi:

  - src/ppu.cpp:11 — if (!(lcdc & 0x80)) (LCD enable bit, ma non c'è scritto).
  - src/ppu.cpp:77,93,100,148,149,182,197,198,234,238,272,273,274,275,302,306 — almeno 16 bit-mask di LCDC/STAT/OAM-attr senza nome.
  - src/timer.cpp:33,35 — tac & 0x04 (enable), tac & 0x03 (prescaler index), [1024, 16, 64, 256] come literal.
  - src/joypad.cpp:38,46,48,50,52 — 0x30 (col-select mask), 0x10/0x20 (col-select dpad/btns), 0xC0 (high bits read 1), 0x0F (low nibble = active-low buttons).
  - src/irq.cpp:8,11,14 — 0x1F (5-bit IRQ mask), 0x40 (IRQ vector base), b * 8 (vector stride).
  - src/dma.cpp:12,7 — 0xFF46, val << 8, sram.size() (= 160 ma chiamato OAM size).
  - src/mmu.cpp:86 — v | 0xE0 (IF high bits = 1, commentato — buono).
  - src/ppu.cpp:248,252,260,265,277,284,286,289 — 40 (OAM sprite count), 10 (sprite per scanline), 16 (tile bytes), 8 (sprite width), 0x40/0x20/0x80/0x10 (OAM attr flags).

  I DEF_HWREG aiutano per nominare l'indirizzo (hwr_lcdc() invece di read_u8(0xFF40)), ma i bit dentro al registro sono ancora numeri.

  Direzione: un singolo header inc/gb_layout.h (o hw_regs.h, scegli) che dichiara:

  namespace gb {
      // MMIO register addresses
      namespace io {
          constexpr uint16_t P1   = 0xFF00;
          constexpr uint16_t SB   = 0xFF01;
          constexpr uint16_t SC   = 0xFF02;
          constexpr uint16_t DIV  = 0xFF04;
          constexpr uint16_t TIMA = 0xFF05;
          constexpr uint16_t TMA  = 0xFF06;
          constexpr uint16_t TAC  = 0xFF07;
          constexpr uint16_t IF   = 0xFF0F;
          // NR10..NR52 …
          constexpr uint16_t LCDC = 0xFF40;
          constexpr uint16_t STAT = 0xFF41;
          constexpr uint16_t SCY  = 0xFF42;
          // …
          constexpr uint16_t IE   = 0xFFFF;
      }

      // LCDC bits ($FF40)
      namespace lcdc {
          constexpr uint8_t bg_enable      = 0x01; // bit 0
          constexpr uint8_t obj_enable     = 0x02; // bit 1
          constexpr uint8_t obj_size_8x16  = 0x04; // bit 2 — 0=8x8, 1=8x16
          constexpr uint8_t bg_map_9c00    = 0x08; // bit 3 — 0=$9800, 1=$9C00
          constexpr uint8_t tile_data_8000 = 0x10; // bit 4 — 0=$8800-signed, 1=$8000-unsigned
          constexpr uint8_t window_enable  = 0x20; // bit 5
          constexpr uint8_t window_map_9c00= 0x40; // bit 6
          constexpr uint8_t lcd_enable     = 0x80; // bit 7
      }

      // STAT bits ($FF41)
      namespace stat {
          constexpr uint8_t mode_mask      = 0x03;
          constexpr uint8_t lyc_coincidence= 0x04;
          constexpr uint8_t mode0_irq_en   = 0x08;
          constexpr uint8_t mode1_irq_en   = 0x10;
          constexpr uint8_t mode2_irq_en   = 0x20;
          constexpr uint8_t lyc_irq_en     = 0x40;
      }

      // OAM attribute byte (sram[i*4+3])
      namespace oam_attr {
          constexpr uint8_t palette_obp1   = 0x10;
          constexpr uint8_t x_flip         = 0x20;
          constexpr uint8_t y_flip         = 0x40;
          constexpr uint8_t bg_priority    = 0x80;
      }

      // IRQ bits ($FF0F / $FFFF)
      namespace irq_bit {
          constexpr uint8_t vblank   = 0x01;
          constexpr uint8_t lcd_stat = 0x02;
          constexpr uint8_t timer    = 0x04;
          constexpr uint8_t serial   = 0x08;
          constexpr uint8_t joypad   = 0x10;
          constexpr uint8_t all_mask = 0x1F;
      }

      // Geometry
      constexpr int LCD_W = 160;
      constexpr int LCD_H = 144;
      constexpr int TILE_SIZE = 8;
      constexpr int TILE_BYTES = 16;
      constexpr int OAM_ENTRIES = 40;
      constexpr int OAM_BYTES_PER_ENTRY = 4;
      constexpr int OAM_TOTAL_BYTES = OAM_ENTRIES * OAM_BYTES_PER_ENTRY;
      constexpr int SPRITES_PER_LINE = 10;
      constexpr int SCANLINE_DOTS = 456;
      constexpr int VISIBLE_LINES = 144;
      constexpr int TOTAL_LINES = 154;

      // VRAM regions
      constexpr uint16_t VRAM_BASE     = 0x8000;
      constexpr uint16_t TILE_DATA_0   = 0x8000; // unsigned
      constexpr uint16_t TILE_DATA_1   = 0x9000; // signed
      constexpr uint16_t BG_MAP_0      = 0x9800;
      constexpr uint16_t BG_MAP_1      = 0x9C00;

      // CPU timing
      constexpr uint32_t CPU_HZ = 4'194'304;
      constexpr uint64_t CYCLES_PER_FRAME = 70224;
  } // namespace gb

  Una volta che esiste questo, render_bg_scanline si rilegge come pseudocodice:

  if (!(lcdc & gb::lcdc::bg_enable)) { … }
  const uint16_t map_base = (lcdc & gb::lcdc::bg_map_9c00) ? gb::BG_MAP_1 : gb::BG_MAP_0;
  const bool data_8000 = (lcdc & gb::lcdc::tile_data_8000) != 0;

  Tutti i siti che oggi hanno 0x80, 0x08, 0x10, 0x20, 0x40 smettono di essere indovinelli. È il singolo cambiamento con il rapporto leggibilità/sforzo più alto in tutto il progetto.

  Inoltre i comment del tipo // LCDC.0 = BG enable (presenti in più punti) diventano ridondanti — il nome lo dice già.

  ---
  C. Leggibilità minore (puntuali, ma riccorrenti)

  C1. struct mmu ha membri pubblici, e core ha membri che si chiamano come i loro tipi

  inc/core.h:40-49:
  cpu cpu;
  registers regs;
  mmu mmu;
  ppu ppu;
  irq irq;
  serial serial;
  dma dma;
  timer timer;
  apu apu;
  joypad joypad;
  Dentro core il nome mmu non è più il tipo ma il membro. Per ora va bene perché nessuno scrive core::mmu::read_u8 qualificato. Ma combina anche male con A1 (campi MMU pubblici).

  Suggerimento minimale: postfisso _ su tutti i membri non-banali (i sottosistemi lo fanno già — mmu_, dpad_, btns_, ring_). Lo facevi anche nei nuovi file. Sistemiamo core/cpu per coerenza.

  C2. ppu ha mmu& m; come membro privato

  inc/ppu.h:35. Una lettera è un nome cattivo per un campo che compare in 30 righe di m.vram[...], m.hwr_lcdc(), ecc. → mmu_.

  C3. Docstring duplicate negli opcode + sintassi rotta \*

  src/opcodes.cpp:593-595:
  \* Push the contents of register pair DE …
  Backslash invece di slash, quindi è in realtà testo libero, non un comment block. Sintassi rotta nelle docstring di push_bc/de/hl/af, pop_bc/de/hl/af (linee 580-651) e potenzialmente altre — vale la pena un
  grep \\\*. Inoltre quei testi sono presi quasi alla lettera dal manuale Nintendo, ridondanti rispetto alla disasm-table generata che già ha il mnemonic. Per i body banali (load reg-reg) il codice è
  auto-esplicativo, il commento è rumore: 500 commenti × 5 righe = ~2500 righe di boilerplate. Lascia i commenti solo dove c'è una sottigliezza non-ovvia (DAA, ADC con half-carry, HALT bug…).

  C4. instruction::mnemonic come std::string

  inc/instruction.h:15. Ogni istruzione globale alloca una stringa a startup. Per il dispatch non serve; per la disasm c'è già disasm_table.hpp. Se davvero ti serve runtime usa std::string_view su literal.
  Costo: 500 heap allocs evitate.

  C5. _H_NAME_H_ doppio guard + #pragma once

  Quasi tutti gli header hanno entrambi (es. inc/ppu.h:2-3, inc/core.h:2-4). #pragma once da solo basta sui compilatori che usi. Il CLAUDE.md cita "standardize include guards" ma sono ancora doppi. È un
  mass-edit di 2 minuti.

  C6. <optional> incluso ma non più usato in inc/cpu.h:4 e inc/core.h:6

  Dead include.

  C7. using namespace gbemu; nei .cpp

  src/cpu.cpp:7, src/irq.cpp:5, src/timer.cpp:3, ecc. Accettabile ma incrociandolo con mmu mmu; come membro rende un fastidio capire da dove vengono i nomi. Personalmente lo droppo nei file con namespace
  innestati.

  C8. ui.cpp a 840 righe con 9 panel + due tile-viewer

  Il commento al top (src/ui.cpp:5-7) dice "se passa 1.5k consider splitting" — sei a metà strada. Il file mescola due preoccupazioni molto diverse: il Display panel (rendering output finale) e i pannelli di
  debug. Considerando l'obiettivo A2 (rendering aperto a shader/OpenGL), questi due dovrebbero stare separati a prescindere — il Display panel è "produzione", i tile-viewer sono "debug". Splittare per pannello
  no, ma display_panel.cpp (~40 righe oggi) e debug_panels.cpp (~800 righe) sì.

  C9. core::step() chiama apu.step(total) con il totale comprensivo dei 20 cicli IRQ

  src/core.cpp:58-63:
  if (irq.dispatch()) {
      total += 20;
  }
  ppu.step(total);
  apu.step(total);
  timer.step(total);
  Funzionalmente è equivalente al modello "the IRQ dispatch took 20 cycles of bus time", ma se in futuro vorrai modellare bus timing fine (vedi A5) questo già non funziona. È compatibile col TODO M-cycle del
  TODO.md, ma annotalo qui per memoria.

  C10. instruction::execute body ripete ovunque il branch-taken delta

  I 7+ jump condizionali (jp_nz_a16, jp_z_a16, …) e i call/ret condizionali fanno tutti la stessa cosa:
  cpu.extra_cycles = this->cycles_taken - this->cycles;
  Helper: cpu.taken_branch_cost() con la sottrazione fatta lì (o, ancora meglio, l'istruzione non sa di cycles_taken — il dispatcher lo aggiunge automaticamente quando extra_cycles != 0). Riduzione di rumore in
   ~20 opcode.

  C11. pop_af's & 0xFFF0 magico

  src/opcodes.cpp:652. Il commento dice solo cosa fa, non perché. Aggiungi una riga: "i bit 0-3 di F (flag register) sono read-only zero — hardware li forza a 0".

  C12. c.core->mmu doppia indirezione nei panel UI

  src/ui.cpp:386-393, src/ui.cpp:497. Il pannello ha già c.core; spesso ti serve solo c.core->mmu. Un auto& m = c.core->mmu; in cima a ogni handler togliere rumore.

  C13. mbc.cpp:64,69,72,142,170 ritorna 0xFF magico come "open bus"

  Definisci constexpr uint8_t OPEN_BUS = 0xFF; da qualche parte. Lo stesso valore appare in mmu.cpp:35,46,56 (open-bus quando manca la cartuccia). Magari in gb_layout.h.

  C14. core::load(bios_file const&) setta bios_accessible = true, commentato come "skip bios"

  src/core.cpp:26 — il commento dice l'opposto di quello che fa. È vero che dopo il caricamento del BIOS lui è accessibile e quindi non skipped, ma il commento "skip bios" è ingannevole. Eliminalo, il setter
  parla da sé.

  C15. cfg/gbemu.conf con path assoluti hardcoded

  Ancora presente (CLAUDE.md non lo nega). Per condividere il repo o lavorare su un'altra macchina è una pietra al collo. Path relativi + un .example versionato, vero .conf in .gitignore. Già nel
  claude_review.md vecchio, ancora valido.

  C16. union u16reg con anonymous struct

  inc/registers.h:8-14. Già citato nel CLAUDE.md. UB stretto in C++ (legit in C), MSVC + GCC/Clang lo trattano de facto come legale, ma per portabilità futura puoi rifare con uint16_t puro + accessor lo() /
  hi() inline. Inoltre dipendi dall'endianness little — il static_assert(std::endian::native == std::endian::little) da qualche parte ti salva da una build silenziosa su una macchina big-endian.

  ---
  D. Cose già fatte bene (per onestà)

  - mbc come interfaccia polimorfica con make_mbc() factory — buona base per MBC3/5.
  - L'APU ha già una buona separazione channel-per-channel.
  - L'absl::InlinedVector per gli MMIO write handler — scelta sensata documentata in CLAUDE.md.
  - Debugger headless + UI condividono dump_*(ostream&) — ottimo.
  - disasm_table.hpp generato indipendente da opcodes.hpp — accoppiamento giusto.
  - Window state persistito separatamente da imgui.ini (gbemu_window.state) — pensato.
  - Joypad IRQ con edge-detect su 1→0 sulla colonna selezionata — comportamento corretto (il commento in joypad.cpp:36-37 è chiaro).
  - Wave RAM "via puntatore esterno" per evitare back-pointer su APU wave_channel. Pattern pulito.
  - ppu_stub_counter / ppu_stub_step membri (vedi inc/mmu.h:108-109) suggeriscono che la fase iniziale era PPU finta; ora è morto. Da rimuovere.

  ---
  E. Roadmap di refactor consigliata

  Ordine pensato per massimizzare il guadagno cumulativo:

  1. inc/gb_layout.h (B) — l'ora di lavoro più redditizia. Sostituzioni meccaniche, leggibilità a strati.
  2. MMU privata + accessor di subsistema (A1) — sblocca CGB banking e savestate.
  3. irq::request(irq_source) (A6) — minore, ma elimina la complicità trasversale fra moduli.
  4. presenter astrazione (A2) — prerequisito per shader/OpenGL.
  5. PPU: palette resolver + pipeline (idx, palette, prio) → argb (A3) — prerequisito CGB.
  6. Dispatch CPU senza classi virtuali (A4) — singolo PR grosso, ma poi non lo tocchi più.
  7. M-cycle accounting (A5, già in TODO.md) — prerequisito CGB double-speed e mem_timing Blargg.
  8. CGB feature vere (VRAM bank, WRAM bank, BCPS/BGPD, KEY1, HDMA) — finalmente abbordabili.

  I primi 3 sono lavoro contenuto e ad alto ritorno; dal 4 in poi è impegnativo ma il punto è che ognuno di questi step abilita gli altri in modo pulito, mentre farli con l'architettura attuale richiede
  iperestensioni in più posti.

  ---
  In sintesi

  Niente di rotto. Il problema è che ora hai messo dentro features (PPU completa, APU, MBC1, joypad, debugger, UI dock) costruite sopra un'astrazione MMU che è rimasta a "bag of bytes" del primissimo prototipo,
   e sopra un dispatch CPU progettato quando ancora non c'era niente da dispatchare. Le features funzionano; il fatto che funzionino nonostante l'API dell'MMU sia trasparente è merito tuo, non del design. Il
  prossimo step di valore (CGB / shader) farà fatica con l'architettura attuale, e ogni feature aggiuntiva paga il debito di B (magic numbers) e A1 (mmu pubblica) ogni volta.

  Le prime tre voci di E sono il vero "denaro facile". Le altre sono lavoro lungo ma diretto.