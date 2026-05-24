#pragma once

#include <array>
#include <cstdint>

// Game Boy (DMG) hardware constants.
//
// Centralizes the magic numbers that would otherwise be scattered through
// the emulator: MMIO addresses, register bit positions, geometry, timing,
// IRQ vectors, and the DMG four-shade palette.  Names follow the official
// Nintendo Pan Docs.
//
// CGB-specific names are listed where the underlying DMG hardware shares
// the byte (e.g. OAM attribute bits 0-3 are unused on DMG but defined here
// for the day the PPU learns to honor them).
//
// Naming convention: snake_case for bit constants inside a sub-namespace
// (e.g. gb::lcdc::lcd_enable), SCREAMING_CASE for standalone scalars
// (gb::LCD_WIDTH).  Everything is constexpr and header-only.

namespace gb {

    // ------------------------------------------------------------------
    // Geometry
    // ------------------------------------------------------------------
    constexpr int LCD_WIDTH = 160;
    constexpr int LCD_HEIGHT = 144;
    constexpr int TILE_PIXELS = 8;        // a tile is 8x8 pixels
    constexpr int TILE_BYTES = 16;        // 8 rows * 2 bytes/row (2bpp planar)
    constexpr int TILES_PER_MAP_ROW = 32; // each BG map is 32x32 tiles

    constexpr int OAM_ENTRIES = 40;
    constexpr int OAM_BYTES_PER_ENTRY = 4;
    constexpr int OAM_TOTAL_BYTES = OAM_ENTRIES * OAM_BYTES_PER_ENTRY; // 160
    constexpr int SPRITES_PER_LINE = 10;                               // hardware OAM scan limit per scanline

    // ------------------------------------------------------------------
    // VRAM regions (DMG, single-bank addressing)
    // ------------------------------------------------------------------
    constexpr std::uint16_t VRAM_BASE = 0x8000;
    constexpr std::uint16_t VRAM_END = 0xA000; // exclusive
    constexpr std::uint16_t VRAM_SIZE = VRAM_END - VRAM_BASE;

    // LCDC.4 = 1 -> indices are unsigned bytes, base $8000.
    // LCDC.4 = 0 -> indices are signed int8, base $9000 (the actual tile data
    //               window is $8800-$97FF, but the indexing is centred at $9000).
    constexpr std::uint16_t TILE_DATA_UNSIGNED_BASE = 0x8000;
    constexpr std::uint16_t TILE_DATA_SIGNED_BASE = 0x9000;
    constexpr std::uint16_t BG_MAP_0 = 0x9800;
    constexpr std::uint16_t BG_MAP_1 = 0x9C00;

    constexpr std::uint16_t OAM_BASE = 0xFE00;

    // External cartridge RAM, mapped at $A000-$BFFF.  On a cart with a battery
    // this is where saves live; banking (when present) is handled by the MBC.
    constexpr std::uint16_t ERAM_BASE = 0xA000;
    constexpr std::uint16_t ERAM_SIZE = 0x2000;

    // Working RAM, $C000-$DFFF.  DMG is a flat 8 KB; CGB has 4 banks of 8 KB
    // selectable through SVBK ($FF70) and only WRAM_BASE..WRAM_BASE+0x1000
    // ($C000-$CFFF) is bank-0-fixed.
    constexpr std::uint16_t WRAM_BASE = 0xC000;
    constexpr std::uint16_t WRAM_SIZE = 0x2000;

    // Echo RAM, $E000-$FDFF — a mirror of WRAM minus the last 512 bytes.
    constexpr std::uint16_t ECHO_BASE = 0xE000;

    // I/O register region, $FF00-$FF7F.  HRAM picks up immediately after.
    constexpr std::uint16_t IO_REGION_SIZE = 0x0080;

    // High RAM (zero-page).  $FF80-$FFFE for the user; $FFFF is IE — we treat
    // both as a 128-byte window to keep the MMU dispatch trivial.
    constexpr std::uint16_t HRAM_BASE = 0xFF80;
    constexpr std::uint16_t HRAM_SIZE = 0x0080;

    // Cartridge ROM area is fixed at $0000-$7FFF (32 KB visible at once).
    // The split into bank 0 ($0000-$3FFF) and the banked slot ($4000-$7FFF)
    // is an MBC detail; this constant covers the whole window.
    constexpr std::uint16_t ROM_BASE = 0x0000;
    constexpr std::uint16_t ROM_VISIBLE_SIZE = 0x8000;

    // ------------------------------------------------------------------
    // CPU timing
    // ------------------------------------------------------------------
    constexpr std::uint32_t CPU_HZ = 4'194'304;       // T-cycles/s (DMG)
    constexpr std::uint64_t CYCLES_PER_FRAME = 70224; // 154 lines * 456 dots
    constexpr std::uint32_t DIV_TICK_CYCLES = 256;    // DIV increments at CPU_HZ/256 = 16384 Hz

    // ------------------------------------------------------------------
    // MMIO addresses ($FF00-$FF7F and $FFFF)
    // ------------------------------------------------------------------
    namespace io {
        constexpr std::uint16_t BASE = 0xFF00;

        // Joypad
        constexpr std::uint16_t P1 = 0xFF00;

        // Serial
        constexpr std::uint16_t SB = 0xFF01;
        constexpr std::uint16_t SC = 0xFF02;

        // Timer
        constexpr std::uint16_t DIV = 0xFF04;
        constexpr std::uint16_t TIMA = 0xFF05;
        constexpr std::uint16_t TMA = 0xFF06;
        constexpr std::uint16_t TAC = 0xFF07;

        // Interrupts
        constexpr std::uint16_t IF = 0xFF0F;
        constexpr std::uint16_t IE = 0xFFFF;

        // Sound
        constexpr std::uint16_t NR10 = 0xFF10;
        constexpr std::uint16_t NR11 = 0xFF11;
        constexpr std::uint16_t NR12 = 0xFF12;
        constexpr std::uint16_t NR13 = 0xFF13;
        constexpr std::uint16_t NR14 = 0xFF14;
        constexpr std::uint16_t NR21 = 0xFF16;
        constexpr std::uint16_t NR22 = 0xFF17;
        constexpr std::uint16_t NR23 = 0xFF18;
        constexpr std::uint16_t NR24 = 0xFF19;
        constexpr std::uint16_t NR30 = 0xFF1A;
        constexpr std::uint16_t NR31 = 0xFF1B;
        constexpr std::uint16_t NR32 = 0xFF1C;
        constexpr std::uint16_t NR33 = 0xFF1D;
        constexpr std::uint16_t NR34 = 0xFF1E;
        constexpr std::uint16_t NR41 = 0xFF20;
        constexpr std::uint16_t NR42 = 0xFF21;
        constexpr std::uint16_t NR43 = 0xFF22;
        constexpr std::uint16_t NR44 = 0xFF23;
        constexpr std::uint16_t NR50 = 0xFF24;
        constexpr std::uint16_t NR51 = 0xFF25;
        constexpr std::uint16_t NR52 = 0xFF26;
        constexpr std::uint16_t WAVE_RAM_BASE = 0xFF30; // $FF30-$FF3F (16 bytes = 32 nibbles)
        constexpr std::uint16_t WAVE_RAM_END = 0xFF40;  // exclusive

        // PPU
        constexpr std::uint16_t LCDC = 0xFF40;
        constexpr std::uint16_t STAT = 0xFF41;
        constexpr std::uint16_t SCY = 0xFF42;
        constexpr std::uint16_t SCX = 0xFF43;
        constexpr std::uint16_t LY = 0xFF44;
        constexpr std::uint16_t LYC = 0xFF45;
        constexpr std::uint16_t DMA = 0xFF46; // OAM DMA source high byte
        constexpr std::uint16_t BGP = 0xFF47;
        constexpr std::uint16_t OBP0 = 0xFF48;
        constexpr std::uint16_t OBP1 = 0xFF49;
        constexpr std::uint16_t WY = 0xFF4A;
        constexpr std::uint16_t WX = 0xFF4B;

        // Boot ROM disable: any nonzero write unmaps BIOS from $0000-$00FF.
        constexpr std::uint16_t BOOT_OFF = 0xFF50;

        // CGB-only registers listed here so the MMU and future CGB code can
        // refer to them by name on DMG too (where they read open-bus).
        constexpr std::uint16_t KEY1 = 0xFF4D; // double-speed select
        constexpr std::uint16_t VBK = 0xFF4F;  // VRAM bank select
        constexpr std::uint16_t HDMA1 = 0xFF51;
        constexpr std::uint16_t HDMA2 = 0xFF52;
        constexpr std::uint16_t HDMA3 = 0xFF53;
        constexpr std::uint16_t HDMA4 = 0xFF54;
        constexpr std::uint16_t HDMA5 = 0xFF55;
        constexpr std::uint16_t BCPS = 0xFF68; // BG palette spec
        constexpr std::uint16_t BCPD = 0xFF69; // BG palette data
        constexpr std::uint16_t OCPS = 0xFF6A; // OBJ palette spec
        constexpr std::uint16_t OCPD = 0xFF6B; // OBJ palette data
        constexpr std::uint16_t SVBK = 0xFF70; // WRAM bank select
    }                                          // namespace io

    // Convert an absolute MMIO address to its offset in mmu::mmio[].
    constexpr std::uint16_t io_offset(std::uint16_t addr) {
        return static_cast<std::uint16_t>(addr - io::BASE);
    }

    // ------------------------------------------------------------------
    // LCDC ($FF40) bits
    // ------------------------------------------------------------------
    namespace lcdc {
        constexpr std::uint8_t bg_enable = 0x01;       // bit 0 (DMG: BG+window enable; CGB: master priority)
        constexpr std::uint8_t obj_enable = 0x02;      // bit 1
        constexpr std::uint8_t obj_size_8x16 = 0x04;   // bit 2 (0 = 8x8, 1 = 8x16)
        constexpr std::uint8_t bg_map_9c00 = 0x08;     // bit 3 (BG tile map: 0 = $9800, 1 = $9C00)
        constexpr std::uint8_t tile_data_8000 = 0x10;  // bit 4 (0 = $8800 signed, 1 = $8000 unsigned)
        constexpr std::uint8_t window_enable = 0x20;   // bit 5
        constexpr std::uint8_t window_map_9c00 = 0x40; // bit 6 (window tile map: 0 = $9800, 1 = $9C00)
        constexpr std::uint8_t lcd_enable = 0x80;      // bit 7
    }                                                  // namespace lcdc

    // ------------------------------------------------------------------
    // STAT ($FF41) bits
    // ------------------------------------------------------------------
    namespace stat {
        constexpr std::uint8_t mode_mask = 0x03;        // bits 0-1 (PPU mode, read-only by software)
        constexpr std::uint8_t lyc_coincidence = 0x04;  // bit 2 (set when LY == LYC, read-only)
        constexpr std::uint8_t mode0_irq_enable = 0x08; // bit 3 (HBLANK STAT IRQ source)
        constexpr std::uint8_t mode1_irq_enable = 0x10; // bit 4 (VBLANK STAT IRQ source)
        constexpr std::uint8_t mode2_irq_enable = 0x20; // bit 5 (OAM-scan STAT IRQ source)
        constexpr std::uint8_t lyc_irq_enable = 0x40;   // bit 6 (LYC=LY STAT IRQ source)
    }                                                   // namespace stat

    // ------------------------------------------------------------------
    // OAM attribute byte (sram[i*4 + 3]) bits
    // ------------------------------------------------------------------
    namespace oam_attr {
        constexpr std::uint8_t cgb_palette_mask = 0x07; // CGB only: bits 0-2 select OBJ palette 0-7
        constexpr std::uint8_t cgb_vram_bank = 0x08;    // CGB only: bit 3 picks VRAM bank for the tile
        constexpr std::uint8_t dmg_palette_obp1 = 0x10; // bit 4 (0 = OBP0, 1 = OBP1) — DMG only
        constexpr std::uint8_t x_flip = 0x20;           // bit 5
        constexpr std::uint8_t y_flip = 0x40;           // bit 6
        constexpr std::uint8_t bg_priority = 0x80;      // bit 7 (1 = OBJ behind BG colors 1-3)
    }                                                   // namespace oam_attr

    // ------------------------------------------------------------------
    // IF / IE ($FF0F / $FFFF) bits — same layout for both
    // ------------------------------------------------------------------
    namespace irq_bit {
        constexpr std::uint8_t vblank = 0x01;   // bit 0
        constexpr std::uint8_t lcd_stat = 0x02; // bit 1
        constexpr std::uint8_t timer = 0x04;    // bit 2
        constexpr std::uint8_t serial = 0x08;   // bit 3
        constexpr std::uint8_t joypad = 0x10;   // bit 4
        constexpr std::uint8_t all_mask = 0x1F; // bits 0-4
        // Bits 5-7 of IF are unimplemented and read as 1 on real DMG (open-bus
        // pull-ups). Blargg's halt_bug.gb checksums them, so reads of IF must
        // OR this in.
        constexpr std::uint8_t if_unimpl_high = 0xE0;
    } // namespace irq_bit

    // IRQ vector entry points (PC value after dispatch).
    constexpr std::uint16_t IRQ_VECTOR_BASE = 0x0040;
    constexpr std::uint16_t IRQ_VECTOR_STRIDE = 0x0008;

    // ------------------------------------------------------------------
    // P1 ($FF00) bits — joypad
    // The "_n" suffix marks active-low signals: bit cleared (0) means the
    // corresponding column is selected.
    // ------------------------------------------------------------------
    namespace p1 {
        constexpr std::uint8_t button_select_n = 0x20; // bit 5 (0 = A/B/Select/Start column live)
        constexpr std::uint8_t dpad_select_n = 0x10;   // bit 4 (0 = d-pad column live)
        constexpr std::uint8_t col_select_mask = 0x30; // bits 4-5 combined
        constexpr std::uint8_t input_mask = 0x0F;      // bits 0-3 (active-low input lines: 1 = released)
        constexpr std::uint8_t high_bits_set = 0xC0;   // bits 6-7 always read as 1
    }                                                  // namespace p1

    // ------------------------------------------------------------------
    // TAC ($FF07) bits + TIMA prescaler periods
    // ------------------------------------------------------------------
    namespace tac {
        constexpr std::uint8_t enable = 0x04;            // bit 2
        constexpr std::uint8_t clock_select_mask = 0x03; // bits 0-1
        // T-cycles per TIMA increment, indexed by TAC.clock_select:
        //   00 -> 1024 (4096 Hz), 01 -> 16 (262144 Hz),
        //   10 -> 64 (65536 Hz),  11 -> 256 (16384 Hz).
        constexpr std::array<std::uint16_t, 4> tima_period_cycles{1024, 16, 64, 256};
    } // namespace tac

    // ------------------------------------------------------------------
    // PPU per-scanline timing (dots = T-cycles).
    // Drawing length is the minimum 172 dots; on real hardware it can stretch
    // up to 289 with sprite/window penalties, which we don't model.
    // ------------------------------------------------------------------
    namespace ppu_timing {
        constexpr std::uint32_t OAM_SCAN_DOTS = 80;
        constexpr std::uint32_t DRAWING_DOTS = 172;
        constexpr std::uint32_t HBLANK_DOTS = 204;
        constexpr std::uint32_t SCANLINE_DOTS = OAM_SCAN_DOTS + DRAWING_DOTS + HBLANK_DOTS; // 456
        constexpr std::uint8_t VISIBLE_LINES = 144;
        constexpr std::uint8_t TOTAL_LINES = 154;
    } // namespace ppu_timing

    // ------------------------------------------------------------------
    // DMG four-shade palette (ARGB8888). The BGP/OBP0/OBP1 registers map a
    // 2-bit color index onto one of these four shades.
    // ------------------------------------------------------------------
    constexpr std::array<std::uint32_t, 4> DMG_PALETTE_ARGB = {
        0xFFFFFFFFu, // shade 0 (white)
        0xFFAAAAAAu, // shade 1 (light gray)
        0xFF555555u, // shade 2 (dark gray)
        0xFF000000u, // shade 3 (black)
    };

    // Open-bus byte returned when nothing drives the bus (missing cartridge,
    // disabled MBC RAM, reserved/unmapped MMIO).
    constexpr std::uint8_t OPEN_BUS = 0xFF;

} // namespace gb
