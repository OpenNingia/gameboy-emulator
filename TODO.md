● Il piano typed-crunching-eagle.md è completo: PR1 → PR5 tutti spediti. Restano un paio di stub piccoli che il piano
  non considerava strettamente parte di PR5, ma li elenco per chiarezza:

  - Reset button del pannello CPU — ancora disabilitato (il commento nel codice diceva "PR5: re-init core + clear
  RAM/VRAM", ma non era listato nello scope di PR5).
  - Step Over del pannello CPU — alias di Step in attesa di un vero step-over (richiede disasm_one per calcolare
  l'indirizzo dopo CALL/RST). Annotato fin da PR3.

  Cose oltre il piano (eventuali PR futuri):
  - PPU: sprite + window (oggi solo BG).
  - APU: l'audio è solo scaffolding (audio_ring_buffer); nessun emulatore di canale.
  - MBC: solo no_mbc e mbc1; MBC2/3/5 non implementati — i test ROM oltre cpu_instrs.gb (es. Pokémon) ne hanno bisogno.

  ## Refactor: sub-instruction (M-cycle) timing

  Attualmente `core::step()` (`src/core.cpp:50-74`) esegue l'intera istruzione via `cpu.step()` e *poi* avanza
  PPU/APU/timer del totale dei T-cycle dell'istruzione. Questo modello è troppo grossolano per i test Blargg
  `mem_timing` e `mem_timing-2`, che misurano su quale M-cycle interno di un'istruzione cade un read/write.

  **Sintomo concreto**: `mem_timing.gb` fallisce perché TIMA viene incrementato in blocco *dopo* l'istruzione,
  invece di poter incrementare tra un M-cycle e l'altro (es. fra l'opcode fetch e il memory read di `LD A,(HL)`).
  Test che invece dipendono solo dalle **durate totali** delle istruzioni (`instr_timing.gb`) dovrebbero già
  passare con l'architettura attuale.

  **Strada per il refactor (alta densità di cambiamenti, va fatto come PR a sé)**:
  - Esporre un `core::tick(uint8_t m_cycles)` che avanza timer/PPU/APU di `m_cycles * 4` T-cycle.
  - Modificare `mmu::read_u8`/`write_u8` (o introdurre wrapper sul lato CPU) in modo che ogni accesso in
    memoria emetta prima un `core::tick(...)` proporzionale agli M-cycle accumulati dall'ultimo accesso.
  - Spezzare il body di ogni opcode in `inc/opcodes.hpp` / `src/opcodes.cpp` in modo che le fasi (fetch operand,
    read, ALU, write, internal) emettano tick negli M-cycle corretti — `cpu::extra_cycles` come somma postuma
    diventa obsoleto.
  - Aggiornare `irq::dispatch()` per spalmare i suoi 5 M-cycle in tick discreti (2 wait, push hi, push lo, jump).
  - Test di non-regressione: `cpu_instrs.gb`, `instr_timing.gb` devono restare verdi; `mem_timing.gb` /
    `mem_timing-2.gb` diventano il nuovo target.

  Test Blargg attualmente da marcare come "skip" finché il refactor non c'è:
  - `interrupt_time/interrupt_time.gb` — `REQUIRE_CGB 1` (CGB-only, indipendente da questo refactor).
  - `mem_timing/mem_timing.gb` e sotto-test `01-read_timing.gb` / `02-write_timing.gb` / `03-modify_timing.gb`.
  - `mem_timing-2/mem_timing-2.gb` se presente.
  - `oam_bug/oam_bug.gb` e i sub-ROM `1-lcd_sync.gb` … `8-instr_effect.gb`. Il test verifica la corruzione
    OAM DMG durante incrementi 16-bit di registri pair con valore in $FE00-$FEFF nei primi 20 cicli di una
    scanline visibile — dipende dal modello M-cycle del refactor sopra e da un PPU che esponga la finestra
    di mode-2 OAM scan al ciclo esatto. Storicamente nessun gioco commerciale dipende da questo quirk, quindi
    è un test "nice-to-have" da affrontare *dopo* il refactor M-cycle.

  ## halt_bug

  Test: `halt_bug.gb` (root della test-rom suite Blargg). Verifica un quirk noto del CPU DMG/CGB.

  **Comportamento corretto**: quando `HALT` viene eseguito con `IME = 0` *e* `IF & IE & 0x1F != 0`,
  il CPU NON entra in halt mode. Al posto di entrare in halt:
  - il byte all'indirizzo `PC+1` (la prossima istruzione) viene letto **due volte**: la prima fetch
    non incrementa `PC`, la seconda sì. In pratica l'istruzione dopo HALT viene eseguita due volte
    (o, se è un'istruzione multi-byte, il primo byte viene rieseguito come opcode e il resto va
    fuori sincrono).

  **Stato attuale**: `IMPL_INSTR(halt)` in `src/opcodes.cpp:3559` fa solo `cpu.halted = true;`, senza
  controllare `IME` né `IF & IE`. Quindi sui Blargg quel test ci finisce sempre dentro `halted=true`,
  poi `cpu::step()` (`src/cpu.cpp:28-36`) rileva `pending` e clear-a `halted`, ma intanto ha consumato
  4 cicli e — soprattutto — il PC è già avanzato oltre il byte da rieseguire.

  **Fix proposto** (piccolo, autonomo, non richiede il refactor M-cycle):
  1. In `halt` opcode: controlla `cpu.interrupt_enabled` e `mmu.hwr_if() & mmu.hwr_ie() & 0x1F`.
     - Se IME=1 → entra normalmente in halt (`cpu.halted = true;`).
     - Se IME=0 e pending == 0 → entra in halt (la CPU si fermerà finché un IRQ pendente non lo sveglia).
     - Se IME=0 e pending != 0 → **NON** entrare in halt; settare un nuovo flag `cpu.halt_bug = true`.
  2. In `cpu::step()`: se `halt_bug` è set, dopo la `fetch()` dell'istruzione successiva, decrementare
     `regs.pc` di 1 (per opcode non-CB) e resettare il flag. La logica esatta è "la prima fetch dopo
     HALT non incrementa PC".
  3. Aggiungere `halt_bug` ai membri di `cpu` (`inc/cpu.h`).
  4. Validare con `halt_bug.gb` headless.