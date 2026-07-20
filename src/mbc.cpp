#include <algorithm>
#include <chrono>
#include <cstddef>
#include <utility>

#include <exc.hpp>
#include <log.h>
#include <mbc.h>

namespace gbemu {
    namespace {

        constexpr std::uint16_t header_cgb_flag_addr = 0x0143;
        constexpr std::uint16_t header_type_addr = 0x0147;
        constexpr std::uint16_t header_rom_size_addr = 0x0148;
        constexpr std::uint16_t header_ram_size_addr = 0x0149;

        // Convert the header byte at 0x0149 into the actual external-RAM
        // size in bytes.  Values follow Pan Docs.
        std::size_t decode_ram_size(std::uint8_t code) {
            switch (code) {
                case 0x00:
                    return 0;
                case 0x01:
                    return 2 * 1024; // unofficial, present on some homebrew
                case 0x02:
                    return 8 * 1024;
                case 0x03:
                    return 32 * 1024;
                case 0x04:
                    return 128 * 1024;
                case 0x05:
                    return 64 * 1024;
                default:
                    throw gbemu_exception{"Invalid cartridge RAM size code"};
            }
        }

        // Round-up power-of-two mask for the 5-bit ROM bank number, based on
        // the actual ROM size.  cpu_instrs.gb has 4 banks so the mask is 0x03.
        std::uint8_t rom_bank_mask_for(std::size_t rom_bytes) {
            std::size_t banks = rom_bytes / 0x4000;
            if (banks <= 1)
                return 0;
            std::uint8_t mask = 1;
            while (mask < banks - 1)
                mask = static_cast<std::uint8_t>((mask << 1) | 1);
            return mask;
        }

        // Wider variant for MBC3/MBC5 — MBC5 carries a 9-bit bank field and
        // 8 MB carts (512 banks) overflow the 8-bit mask.
        std::uint16_t rom_bank_mask_for_u16(std::size_t rom_bytes) {
            std::size_t banks = rom_bytes / 0x4000;
            if (banks <= 1)
                return 0;
            std::uint16_t mask = 1;
            while (mask < banks - 1)
                mask = static_cast<std::uint16_t>((mask << 1) | 1);
            return mask;
        }

        // ----------------------------------------------------------------
        // No-MBC (cartridge type 0x00 / 0x08 / 0x09).
        //
        // Up to 32 KB of ROM directly mapped at 0x0000-0x7FFF.  An optional
        // 8 KB of cartridge RAM may live at 0xA000-0xBFFF.  Writes to ROM
        // are silently ignored — some games (and a few tests) write there
        // by accident.
        // ----------------------------------------------------------------
        class no_mbc final : public mbc {
        public:
            no_mbc(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type, std::uint8_t cgb_flag)
                : rom_(std::move(rom)), ram_(ram_bytes, 0u), type_(type), cgb_flag_(cgb_flag) {}

            std::uint8_t read(std::uint16_t addr) const override {
                if (addr < 0x8000) {
                    if (addr < rom_.size())
                        return rom_[addr];
                    return 0xFF;
                }
                if (addr >= 0xA000 && addr < 0xC000) {
                    if (ram_.empty())
                        return 0xFF;
                    return ram_[addr - 0xA000];
                }
                return 0xFF;
            }

            void write(std::uint16_t addr, std::uint8_t val) override {
                if (addr >= 0xA000 && addr < 0xC000 && !ram_.empty()) {
                    ram_[addr - 0xA000] = val;
                    ram_dirty_ = true;
                }
                // Writes to 0x0000-0x7FFF on a no-MBC cart are ignored.
            }

            bool has_battery() const override { return cartridge_type_has_battery(type_); }
            std::span<const std::uint8_t> ram_data() const override { return ram_; }
            void ram_load(std::span<const std::uint8_t> src) override {
                if (src.size() == ram_.size())
                    std::copy(src.begin(), src.end(), ram_.begin());
            }
            bool ram_dirty() const override { return ram_dirty_; }
            void ram_clear_dirty() override { ram_dirty_ = false; }

            mbc_debug_state debug_state() const override {
                // No banking: slot1 is fixed at bank 1, RAM (if present) at bank 0.
                // Without an enable line in hardware we treat any cart-RAM as always
                // accessible — matches what the read/write methods actually do.
                return mbc_debug_state{type_, /*rom_bank*/ static_cast<std::uint8_t>(1), /*ram_bank*/ 0,
                                       /*ram_enabled*/ !ram_.empty(),
                                       /*mode*/ 0};
            }

            std::uint8_t cgb_flag() const override { return cgb_flag_; }

        private:
            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
            bool ram_dirty_{false};
        };

        // ----------------------------------------------------------------
        // MBC1 (cartridge types 0x01 / 0x02 / 0x03).
        //
        // - 0x0000-0x1FFF write: RAM enable (0x0A in low nibble enables).
        // - 0x2000-0x3FFF write: low 5 bits of the ROM bank number.  The
        //   value 0 is translated to 1 (mbc1 cannot select bank 0 in the
        //   high slot).
        // - 0x4000-0x5FFF write: 2 extra bits.  In ROM banking mode they
        //   are bits 5-6 of the ROM bank (matters for >= 1 MB carts); in
        //   RAM banking mode they pick the RAM bank.
        // - 0x6000-0x7FFF write: mode select (0 = ROM banking, 1 = RAM /
        //   advanced ROM banking).
        // - 0x4000-0x7FFF read : current banked ROM bank.
        // - 0xA000-0xBFFF read/write: external RAM, gated by ram_enabled_.
        //
        // The "zero bank" trick (mode 1 lets the upper bits also appear at
        // 0x0000-0x3FFF and 0xA000-0xBFFF) is implemented for completeness,
        // even though cpu_instrs.gb does not need it.
        // ----------------------------------------------------------------
        class mbc1 final : public mbc {
        public:
            mbc1(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type, std::uint8_t cgb_flag)
                : rom_(std::move(rom)),
                  ram_(ram_bytes, 0u),
                  rom_bank_mask_(rom_bank_mask_for(rom_.size())),
                  type_(type),
                  cgb_flag_(cgb_flag) {}

            std::uint8_t read(std::uint16_t addr) const override {
                if (addr < 0x4000) {
                    // In mode 1 the upper bits are visible here too; in
                    // mode 0 this slot is always bank 0.
                    std::uint32_t bank = mode_ ? (static_cast<std::uint32_t>(upper_bits_) << 5) : 0u;
                    return rom_byte((bank & rom_bank_mask_) * 0x4000 + addr);
                }
                if (addr < 0x8000) {
                    std::uint32_t bank =
                        (static_cast<std::uint32_t>(upper_bits_) << 5) | static_cast<std::uint32_t>(lower_bits_);
                    bank &= rom_bank_mask_;
                    return rom_byte(bank * 0x4000 + (addr - 0x4000));
                }
                if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_ || ram_.empty())
                        return 0xFF;
                    return ram_[ram_offset(addr)];
                }
                return 0xFF;
            }

            void write(std::uint16_t addr, std::uint8_t val) override {
                if (addr < 0x2000) {
                    ram_enabled_ = (val & 0x0F) == 0x0A;
                } else if (addr < 0x4000) {
                    lower_bits_ = val & 0x1F;
                    if (lower_bits_ == 0)
                        lower_bits_ = 1;
                } else if (addr < 0x6000) {
                    upper_bits_ = val & 0x03;
                } else if (addr < 0x8000) {
                    mode_ = val & 0x01;
                } else if (addr >= 0xA000 && addr < 0xC000) {
                    if (ram_enabled_ && !ram_.empty()) {
                        ram_[ram_offset(addr)] = val;
                        ram_dirty_ = true;
                    }
                }
            }

            bool has_battery() const override { return cartridge_type_has_battery(type_); }
            std::span<const std::uint8_t> ram_data() const override { return ram_; }
            void ram_load(std::span<const std::uint8_t> src) override {
                if (src.size() == ram_.size())
                    std::copy(src.begin(), src.end(), ram_.begin());
            }
            bool ram_dirty() const override { return ram_dirty_; }
            void ram_clear_dirty() override { ram_dirty_ = false; }

            mbc_debug_state debug_state() const override {
                const auto bank = static_cast<std::uint8_t>(((upper_bits_ << 5) | lower_bits_) &
                                                            (rom_bank_mask_ ? rom_bank_mask_ : 1));
                return mbc_debug_state{type_, bank, upper_bits_, ram_enabled_, mode_};
            }

            void reset() override {
                lower_bits_ = 1;
                upper_bits_ = 0;
                mode_ = 0;
                ram_enabled_ = false;
            }

            std::uint8_t cgb_flag() const override { return cgb_flag_; }

        private:
            std::uint8_t rom_byte(std::size_t offset) const {
                return offset < rom_.size() ? rom_[offset] : std::uint8_t{0xFF};
            }

            std::size_t ram_offset(std::uint16_t addr) const {
                // 32 KB RAM carts use the upper bits to pick a 8 KB bank;
                // smaller carts ignore them.
                std::size_t banks = ram_.size() / 0x2000;
                std::size_t bank = (mode_ && banks > 1) ? (upper_bits_ % banks) : 0u;
                return bank * 0x2000 + (addr - 0xA000);
            }

            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t lower_bits_{1};
            std::uint8_t upper_bits_{0};
            std::uint8_t mode_{0};
            bool ram_enabled_{false};
            std::uint8_t rom_bank_mask_{0};
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
            bool ram_dirty_{false};
        };

        // ----------------------------------------------------------------
        // MBC2 (cartridge types 0x05 / 0x06).
        //
        // - 0x0000-0x3FFF write: bit 8 of the address selects between the
        //   two control registers — addr bit 8 == 0 is RAM enable (low
        //   nibble 0x0A enables), addr bit 8 == 1 is ROM bank select (low
        //   4 bits, 0 -> 1).
        // - 0x4000-0x7FFF read : current banked ROM bank.
        // - 0xA000-0xBFFF: built-in 512 x 4-bit RAM, mirrored every 0x200
        //   bytes through the whole window.  Only the low nibble of each
        //   byte is real storage; reads return 0xF0 in the upper nibble.
        // ----------------------------------------------------------------
        class mbc2 final : public mbc {
        public:
            mbc2(std::vector<std::uint8_t> rom, std::uint8_t type, std::uint8_t cgb_flag)
                : rom_(std::move(rom)),
                  ram_(512, 0u), // built-in 512 nibbles, low 4 bits used
                  rom_bank_mask_(rom_bank_mask_for(rom_.size())),
                  type_(type),
                  cgb_flag_(cgb_flag) {}

            std::uint8_t read(std::uint16_t addr) const override {
                if (addr < 0x4000) {
                    return rom_byte(addr);
                }
                if (addr < 0x8000) {
                    std::uint32_t bank = rom_bank_ ? rom_bank_ : 1u;
                    bank &= rom_bank_mask_;
                    return rom_byte(bank * 0x4000 + (addr - 0x4000));
                }
                if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_)
                        return 0xFF;
                    // 512 nibbles, mirrored.  Upper nibble of each byte
                    // floats — convention is to return 0xF in those bits.
                    std::uint8_t nibble = ram_[(addr - 0xA000) & 0x01FF] & 0x0F;
                    return static_cast<std::uint8_t>(0xF0 | nibble);
                }
                return 0xFF;
            }

            void write(std::uint16_t addr, std::uint8_t val) override {
                if (addr < 0x4000) {
                    // Address bit 8 picks the register.  All other bits in
                    // 0x0000-0x3FFF are don't-cares for MBC2.
                    if ((addr & 0x0100) == 0) {
                        // RAM enable.
                        ram_enabled_ = (val & 0x0F) == 0x0A;
                    } else {
                        // ROM bank select (low 4 bits).
                        std::uint8_t bank = val & 0x0F;
                        if (bank == 0)
                            bank = 1;
                        rom_bank_ = bank;
                    }
                } else if (addr >= 0xA000 && addr < 0xC000) {
                    if (ram_enabled_) {
                        ram_[(addr - 0xA000) & 0x01FF] = val & 0x0F;
                        ram_dirty_ = true;
                    }
                }
                // 0x4000-0x7FFF writes are ignored on MBC2.
            }

            bool has_battery() const override { return cartridge_type_has_battery(type_); }
            std::span<const std::uint8_t> ram_data() const override { return ram_; }
            void ram_load(std::span<const std::uint8_t> src) override {
                if (src.size() != ram_.size())
                    return;
                // Only the low nibble of each byte is real storage on MBC2.
                // Mask incoming bytes to the same convention so a round-trip
                // through a foreign emulator's .sav (which may store nibbles
                // in either high or low half) doesn't accidentally inject
                // garbage into the upper four bits.
                for (std::size_t i = 0; i < ram_.size(); ++i)
                    ram_[i] = src[i] & 0x0F;
            }
            bool ram_dirty() const override { return ram_dirty_; }
            void ram_clear_dirty() override { ram_dirty_ = false; }

            mbc_debug_state debug_state() const override {
                return mbc_debug_state{type_, static_cast<std::uint16_t>(rom_bank_ & rom_bank_mask_),
                                       /*ram_bank*/ 0, ram_enabled_,
                                       /*mode*/ 0};
            }

            void reset() override {
                rom_bank_ = 1;
                ram_enabled_ = false;
            }

            std::uint8_t cgb_flag() const override { return cgb_flag_; }

        private:
            std::uint8_t rom_byte(std::size_t offset) const {
                return offset < rom_.size() ? rom_[offset] : std::uint8_t{0xFF};
            }

            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t rom_bank_{1};
            bool ram_enabled_{false};
            std::uint8_t rom_bank_mask_{0};
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
            bool ram_dirty_{false};
        };

        // ----------------------------------------------------------------
        // MBC3 (cartridge types 0x0F-0x13).
        //
        // - 0x0000-0x1FFF write: RAM + RTC register enable (0x0A in low
        //   nibble enables both).
        // - 0x2000-0x3FFF write: 7-bit ROM bank number.  Bank 0 still maps
        //   to 1 here (MBC1 quirk preserved), no other remapping.
        // - 0x4000-0x5FFF write: 0x00-0x03 selects a RAM bank;
        //   0x08-0x0C selects an RTC register (S/M/H/DL/DH) visible at
        //   0xA000-0xBFFF.
        // - 0x6000-0x7FFF write: latch clock data.  A 0 → 1 sequence
        //   latches the current live RTC values into the read-back
        //   shadow.  Reads of the RTC registers via $A000-$BFFF always
        //   return the latched shadow, never the live state.
        // - 0xA000-0xBFFF: external RAM bank or latched RTC register,
        //   depending on the current select.  Gated by `ram_enabled_`.
        //
        // RTC implementation: anchored to the host's std::chrono real
        // time.  rtc_total_secs_ accumulates while not halted; catch_up()
        // adds (now - last_real_unix_) into it on every read/write.  The
        // 9-bit day counter wraps mod 512, with a sticky carry bit
        // (DH.7) set on overflow and cleared only by a DH register
        // write.  Halt (DH.6) freezes the accumulation.
        //
        // Pokémon Gold/Silver/Crystal exercise this for the day-night
        // cycle; without a real clock the world stays in whatever phase
        // was active at the last meaningful in-game checkpoint.
        // ----------------------------------------------------------------
        class mbc3 final : public mbc {
        public:
            mbc3(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type, std::uint8_t cgb_flag)
                : rom_(std::move(rom)),
                  ram_(ram_bytes, 0u),
                  rom_bank_mask_(rom_bank_mask_for_u16(rom_.size())),
                  type_(type),
                  cgb_flag_(cgb_flag) {
                last_real_unix_ = now_unix();
            }

            std::uint8_t read(std::uint16_t addr) const override {
                if (addr < 0x4000) {
                    return rom_byte(addr);
                }
                if (addr < 0x8000) {
                    std::uint16_t bank = rom_bank_ ? rom_bank_ : 1u;
                    bank &= rom_bank_mask_;
                    return rom_byte(static_cast<std::size_t>(bank) * 0x4000 + (addr - 0x4000));
                }
                if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_)
                        return 0xFF;
                    if (ram_rtc_select_ <= 0x03) {
                        if (ram_.empty())
                            return 0xFF;
                        return ram_[ram_offset(addr)];
                    }
                    // RTC registers always read back the latched shadow,
                    // never live state.  The shadow only updates on a
                    // 0 → 1 sequence at $6000-$7FFF.
                    switch (ram_rtc_select_) {
                        case 0x08:
                            return latched_s_;
                        case 0x09:
                            return latched_m_;
                        case 0x0A:
                            return latched_h_;
                        case 0x0B:
                            return latched_dl_;
                        case 0x0C:
                            return latched_dh_;
                    }
                    return 0xFF;
                }
                return 0xFF;
            }

            void write(std::uint16_t addr, std::uint8_t val) override {
                if (addr < 0x2000) {
                    ram_enabled_ = (val & 0x0F) == 0x0A;
                } else if (addr < 0x4000) {
                    std::uint8_t bank = val & 0x7F;
                    if (bank == 0)
                        bank = 1;
                    rom_bank_ = bank;
                } else if (addr < 0x6000) {
                    ram_rtc_select_ = val;
                } else if (addr < 0x8000) {
                    // 0 → 1 sequence on the latch port copies the live RTC
                    // into the readable shadow.  Any other transition
                    // (0→0, 1→1, 1→0) just updates the edge detector.
                    if (latch_prev_ == 0x00 && val == 0x01)
                        latch_now();
                    latch_prev_ = val;
                } else if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_)
                        return;
                    if (ram_rtc_select_ <= 0x03) {
                        if (!ram_.empty()) {
                            ram_[ram_offset(addr)] = val;
                            ram_dirty_ = true;
                        }
                        return;
                    }
                    if (ram_rtc_select_ >= 0x08 && ram_rtc_select_ <= 0x0C)
                        write_rtc_reg(ram_rtc_select_, val);
                }
            }

            bool has_battery() const override { return cartridge_type_has_battery(type_); }
            std::span<const std::uint8_t> ram_data() const override { return ram_; }
            void ram_load(std::span<const std::uint8_t> src) override {
                if (src.size() == ram_.size())
                    std::copy(src.begin(), src.end(), ram_.begin());
            }
            bool ram_dirty() const override { return ram_dirty_; }
            void ram_clear_dirty() override { ram_dirty_ = false; }

            // RTC serialization — only meaningful on MBC3 TIMER variants
            // (types 0x0F / 0x10).  Non-TIMER MBC3 carts (0x11/0x12/0x13)
            // return nullopt so the BESS writer doesn't emit a stale RTC
            // block for them.
            std::optional<std::array<std::uint8_t, 48>> rtc_blob() const override {
                if (type_ != 0x0F && type_ != 0x10)
                    return std::nullopt;
                // catch_up() folds elapsed wall-clock into rtc_total_secs_
                // before we serialize; decompose() routes through it.  We
                // const_cast because the operation is logically observational
                // even though the cached anchor moves forward — same pattern
                // as MMU MMIO read handlers that update side state.
                auto* self = const_cast<mbc3*>(this);
                std::uint8_t s = 0, m = 0, h = 0, dl = 0, dh = 0;
                self->decompose(s, m, h, dl, dh);
                std::array<std::uint8_t, 48> out{};
                // Live S/M/H/DL/DH at 0x00, 0x04, 0x08, 0x0C, 0x10 — each is
                // 1 data byte followed by 3 padding zeros (BESS convention,
                // matches SameBoy's layout).
                out[0x00] = s;
                out[0x04] = m;
                out[0x08] = h;
                out[0x0C] = dl;
                out[0x10] = dh;
                // Latched S/M/H/DL/DH at 0x14..0x24.
                out[0x14] = latched_s_;
                out[0x18] = latched_m_;
                out[0x1C] = latched_h_;
                out[0x20] = latched_dl_;
                out[0x24] = latched_dh_;
                // UNIX timestamp at 0x28 (int64 little-endian).  We write
                // last_real_unix_ — the moment the live state above was
                // taken — so a future load can compute the wall-clock gap.
                const std::int64_t ts = last_real_unix_;
                for (std::size_t i = 0; i < 8; ++i)
                    out[0x28 + i] = static_cast<std::uint8_t>((ts >> (8 * i)) & 0xFF);
                return out;
            }

            void rtc_load_blob(std::span<const std::uint8_t> src) override {
                if (src.size() != 48)
                    return;
                if (type_ != 0x0F && type_ != 0x10)
                    return;
                const std::uint8_t s = src[0x00];
                const std::uint8_t m = src[0x04];
                const std::uint8_t h = src[0x08];
                const std::uint8_t dl = src[0x0C];
                const std::uint8_t dh = src[0x10];
                latched_s_ = src[0x14];
                latched_m_ = src[0x18];
                latched_h_ = src[0x1C];
                latched_dl_ = src[0x20];
                latched_dh_ = src[0x24];
                std::int64_t saved_unix = 0;
                for (std::size_t i = 0; i < 8; ++i)
                    saved_unix |= static_cast<std::int64_t>(src[0x28 + i]) << (8 * i);
                rtc_halted_ = (dh & 0x40) != 0;
                rtc_day_carry_ = (dh & 0x80) != 0;
                const std::int64_t days = static_cast<std::int64_t>(dl) | (static_cast<std::int64_t>(dh & 0x01) << 8);
                rtc_total_secs_ = static_cast<std::int64_t>(s) + static_cast<std::int64_t>(m) * 60 +
                                  static_cast<std::int64_t>(h) * 3600 + days * 86400;
                // Wall-clock gap survival: if the cart was not halted at the
                // time of the dump, fold (now - saved) into the accumulator
                // so the in-game clock "kept ticking" while the emulator was
                // off.  Halted carts stay frozen — the player will see the
                // same time they paused at.
                const std::int64_t now = now_unix();
                if (!rtc_halted_ && now > saved_unix)
                    rtc_total_secs_ += (now - saved_unix);
                last_real_unix_ = now;
            }

            mbc_debug_state debug_state() const override {
                const auto bank = static_cast<std::uint16_t>(rom_bank_ & rom_bank_mask_);
                // ram_bank shows the raw select byte: 0x00-0x03 = RAM bank,
                // 0x08-0x0C = RTC S/M/H/DL/DH. The MBC panel doesn't decode
                // it further but the value is enough to confirm the latch
                // path is being driven correctly.
                return mbc_debug_state{type_, bank, ram_rtc_select_, ram_enabled_, /*mode*/ 0};
            }

            void reset() override {
                rom_bank_ = 1;
                ram_rtc_select_ = 0;
                ram_enabled_ = false;
                latch_prev_ = 0xFF;
                // RTC state — including the latched shadow — survives a
                // soft reset.  On real hardware the RTC chip lives on the
                // cart and is battery-backed, fully independent of the GB
                // CPU's reset line.  Preserving the latched values here
                // also matters for the ROM hot-swap path: load_rom_
                // restores .rtc blob bytes into latched_*, and the
                // immediately-following core.reset() must not wipe them.
                // Games will typically latch again before their first read
                // anyway, so this is a no-visible-effect change for
                // anything that exercises the latch port.
            }

            std::uint8_t cgb_flag() const override { return cgb_flag_; }

        private:
            std::uint8_t rom_byte(std::size_t offset) const {
                return offset < rom_.size() ? rom_[offset] : std::uint8_t{0xFF};
            }

            std::size_t ram_offset(std::uint16_t addr) const {
                // Up to 4 banks of 8 KB.  Smaller carts ignore the high
                // bits of the select.
                std::size_t banks = ram_.size() / 0x2000;
                std::size_t bank = banks > 1 ? (ram_rtc_select_ % banks) : 0u;
                return bank * 0x2000 + (addr - 0xA000);
            }

            // ---- RTC plumbing -------------------------------------------
            static std::int64_t now_unix() {
                using namespace std::chrono;
                return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            }

            // Move accumulated real time into rtc_total_secs_ and reset the
            // anchor.  No-op while halted (the anchor still slides forward
            // so the next un-halt doesn't pick up the gap).
            void catch_up() {
                const auto now = now_unix();
                if (!rtc_halted_)
                    rtc_total_secs_ += (now - last_real_unix_);
                last_real_unix_ = now;
            }

            // Break rtc_total_secs_ into S / M / H / DL / DH.  Sets the
            // sticky day-carry on overflow past 511 days.
            void decompose(std::uint8_t& s, std::uint8_t& m, std::uint8_t& h, std::uint8_t& dl, std::uint8_t& dh) {
                catch_up();
                std::int64_t secs = rtc_total_secs_;
                if (secs < 0)
                    secs = 0;
                s = static_cast<std::uint8_t>(secs % 60);
                std::int64_t mins = secs / 60;
                m = static_cast<std::uint8_t>(mins % 60);
                std::int64_t hours = mins / 60;
                h = static_cast<std::uint8_t>(hours % 24);
                std::int64_t days = hours / 24;
                if (days > 511) {
                    rtc_day_carry_ = true;
                    days %= 512;
                }
                dl = static_cast<std::uint8_t>(days & 0xFF);
                dh = static_cast<std::uint8_t>(((days >> 8) & 0x01) | (rtc_halted_ ? 0x40 : 0x00) |
                                               (rtc_day_carry_ ? 0x80 : 0x00));
            }

            void latch_now() { decompose(latched_s_, latched_m_, latched_h_, latched_dl_, latched_dh_); }

            void write_rtc_reg(std::uint8_t reg, std::uint8_t val) {
                // Refresh the live decomposition first so we can patch a
                // single field and recompose without losing the others.
                std::uint8_t s, m, h, dl, dh;
                decompose(s, m, h, dl, dh);
                switch (reg) {
                    case 0x08:
                        s = static_cast<std::uint8_t>(val & 0x3F);
                        break;
                    case 0x09:
                        m = static_cast<std::uint8_t>(val & 0x3F);
                        break;
                    case 0x0A:
                        h = static_cast<std::uint8_t>(val & 0x1F);
                        break;
                    case 0x0B:
                        dl = val;
                        break;
                    case 0x0C:
                        dh = static_cast<std::uint8_t>(val & 0xC1);
                        rtc_halted_ = (val & 0x40) != 0;
                        rtc_day_carry_ = (val & 0x80) != 0;
                        break;
                    default:
                        return;
                }
                const std::int64_t days = static_cast<std::int64_t>(dl) | (static_cast<std::int64_t>(dh & 0x01) << 8);
                rtc_total_secs_ = static_cast<std::int64_t>(s) + static_cast<std::int64_t>(m) * 60 +
                                  static_cast<std::int64_t>(h) * 3600 + days * 86400;
                last_real_unix_ = now_unix();
            }
            // -------------------------------------------------------------

            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t rom_bank_{1};
            std::uint8_t ram_rtc_select_{0};
            bool ram_enabled_{false};
            std::uint16_t rom_bank_mask_{0};
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
            bool ram_dirty_{false};

            // RTC live state.  catch_up() is only invoked from write paths
            // (latch_now via the $6000-$7FFF port, write_rtc_reg via the
            // RTC register slot at $A000-$BFFF), so these fields are
            // plain non-const members — the latched shadow is what
            // reads see.
            std::int64_t rtc_total_secs_{0};
            std::int64_t last_real_unix_{0};
            bool rtc_day_carry_{false};
            bool rtc_halted_{false};
            // Latch port edge detector ($6000-$7FFF write history).
            std::uint8_t latch_prev_{0xFF};
            // Latched shadow — what reads through $A000-$BFFF (with the
            // RTC register selected) return.
            std::uint8_t latched_s_{0};
            std::uint8_t latched_m_{0};
            std::uint8_t latched_h_{0};
            std::uint8_t latched_dl_{0};
            std::uint8_t latched_dh_{0};
        };

        // ----------------------------------------------------------------
        // MBC5 (cartridge types 0x19-0x1E).
        //
        // - 0x0000-0x1FFF write: RAM enable (0x0A in low nibble).
        // - 0x2000-0x2FFF write: low 8 bits of the 9-bit ROM bank.  Bank
        //   0 IS valid on MBC5 (no remap to 1).
        // - 0x3000-0x3FFF write: bit 9 of the ROM bank (only bit 0 of
        //   the value is used).
        // - 0x4000-0x5FFF write: 4-bit RAM bank select.  Bit 3 (0x08)
        //   is the rumble line on rumble carts (0x1C-0x1E); we ignore
        //   it since there are no haptics to drive.
        // - 0xA000-0xBFFF: external RAM bank, up to 16 x 8 KB.  Gated
        //   by `ram_enabled_`.
        // ----------------------------------------------------------------
        class mbc5 final : public mbc {
        public:
            mbc5(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type, std::uint8_t cgb_flag)
                : rom_(std::move(rom)),
                  ram_(ram_bytes, 0u),
                  rom_bank_mask_(rom_bank_mask_for_u16(rom_.size())),
                  type_(type),
                  cgb_flag_(cgb_flag) {}

            std::uint8_t read(std::uint16_t addr) const override {
                if (addr < 0x4000) {
                    return rom_byte(addr);
                }
                if (addr < 0x8000) {
                    std::uint16_t bank = rom_bank_ & rom_bank_mask_;
                    return rom_byte(static_cast<std::size_t>(bank) * 0x4000 + (addr - 0x4000));
                }
                if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_ || ram_.empty())
                        return 0xFF;
                    return ram_[ram_offset(addr)];
                }
                return 0xFF;
            }

            void write(std::uint16_t addr, std::uint8_t val) override {
                if (addr < 0x2000) {
                    ram_enabled_ = (val & 0x0F) == 0x0A;
                } else if (addr < 0x3000) {
                    rom_bank_ = static_cast<std::uint16_t>((rom_bank_ & 0x0100) | val);
                } else if (addr < 0x4000) {
                    rom_bank_ = static_cast<std::uint16_t>((rom_bank_ & 0x00FF) | ((val & 0x01) << 8));
                } else if (addr < 0x6000) {
                    ram_bank_ = val & 0x0F;
                } else if (addr >= 0xA000 && addr < 0xC000) {
                    if (ram_enabled_ && !ram_.empty()) {
                        ram_[ram_offset(addr)] = val;
                        ram_dirty_ = true;
                    }
                }
                // 0x6000-0x7FFF writes are ignored on MBC5.
            }

            bool has_battery() const override { return cartridge_type_has_battery(type_); }
            std::span<const std::uint8_t> ram_data() const override { return ram_; }
            void ram_load(std::span<const std::uint8_t> src) override {
                if (src.size() == ram_.size())
                    std::copy(src.begin(), src.end(), ram_.begin());
            }
            bool ram_dirty() const override { return ram_dirty_; }
            void ram_clear_dirty() override { ram_dirty_ = false; }

            mbc_debug_state debug_state() const override {
                return mbc_debug_state{type_, static_cast<std::uint16_t>(rom_bank_ & rom_bank_mask_), ram_bank_,
                                       ram_enabled_, /*mode*/ 0};
            }

            void reset() override {
                rom_bank_ = 1;
                ram_bank_ = 0;
                ram_enabled_ = false;
            }

            std::uint8_t cgb_flag() const override { return cgb_flag_; }

        private:
            std::uint8_t rom_byte(std::size_t offset) const {
                return offset < rom_.size() ? rom_[offset] : std::uint8_t{0xFF};
            }

            std::size_t ram_offset(std::uint16_t addr) const {
                std::size_t banks = ram_.size() / 0x2000;
                std::size_t bank = banks > 1 ? (ram_bank_ % banks) : 0u;
                return bank * 0x2000 + (addr - 0xA000);
            }

            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint16_t rom_bank_{1};
            std::uint8_t ram_bank_{0};
            bool ram_enabled_{false};
            std::uint16_t rom_bank_mask_{0};
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
            bool ram_dirty_{false};
        };

    } // namespace

    bool cartridge_type_has_battery(std::uint8_t type) {
        // Pan Docs cartridge-type bytes that carry a battery line.  The set
        // covers every official Nintendo MBC1/2/3/5 BATTERY variant plus the
        // ROM+RAM+BATTERY pseudo-type, MMM01+RAM+BATTERY, MBC7, and HuC1.
        switch (type) {
            case 0x03: // MBC1+RAM+BATTERY
            case 0x06: // MBC2+BATTERY
            case 0x09: // ROM+RAM+BATTERY
            case 0x0D: // MMM01+RAM+BATTERY
            case 0x0F: // MBC3+TIMER+BATTERY
            case 0x10: // MBC3+TIMER+RAM+BATTERY
            case 0x13: // MBC3+RAM+BATTERY
            case 0x1B: // MBC5+RAM+BATTERY
            case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
            case 0x22: // MBC7+SENSOR+RUMBLE+RAM+BATTERY
            case 0xFF: // HuC1+RAM+BATTERY
                return true;
            default:
                return false;
        }
    }

    std::unique_ptr<mbc> make_mbc(std::vector<std::uint8_t> rom) {
        if (rom.size() < 0x0150)
            throw gbemu_exception{"ROM too small to contain a header"};

        std::uint8_t type = rom[header_type_addr];
        std::uint8_t rom_size_code = rom[header_rom_size_addr];
        std::uint8_t ram_size_code = rom[header_ram_size_addr];
        std::uint8_t cgb_flag = rom[header_cgb_flag_addr];

        std::size_t expected_rom_size = std::size_t{32 * 1024} << rom_size_code;
        std::size_t ram_size = decode_ram_size(ram_size_code);

        const char* cgb_label = "DMG";
        switch (classify_cgb_flag(cgb_flag)) {
            case cgb_support::compat:
                cgb_label = "CGB-compat";
                break;
            case cgb_support::cgb_only:
                cgb_label = "CGB-only";
                break;
            case cgb_support::none:
                break;
        }
        LOG_INFO(log::root(),
                 "Cartridge: type=0x{:02x} rom_size=0x{:02x} ({} KB) ram_size=0x{:02x} ({} KB) cgb_flag=0x{:02x} ({})",
                 type, rom_size_code, expected_rom_size / 1024, ram_size_code, ram_size / 1024, cgb_flag, cgb_label);

        switch (static_cast<cartridge_type>(type)) {
            case cartridge_type::rom_only:
                return std::make_unique<no_mbc>(std::move(rom), 0, type, cgb_flag);
            case cartridge_type::rom_ram:
            case cartridge_type::rom_ram_battery:
                return std::make_unique<no_mbc>(std::move(rom), ram_size, type, cgb_flag);
            case cartridge_type::mbc1:
            case cartridge_type::mbc1_ram:
            case cartridge_type::mbc1_ram_battery:
                return std::make_unique<mbc1>(std::move(rom), ram_size, type, cgb_flag);
            case cartridge_type::mbc2:
            case cartridge_type::mbc2_battery:
                // MBC2 has built-in 512 nibbles — the header RAM size code is
                // typically 0x00 and we ignore `ram_size` here on purpose.
                return std::make_unique<mbc2>(std::move(rom), type, cgb_flag);
            case cartridge_type::mbc3_timer_battery:
            case cartridge_type::mbc3_timer_ram_battery:
            case cartridge_type::mbc3:
            case cartridge_type::mbc3_ram:
            case cartridge_type::mbc3_ram_battery:
                return std::make_unique<mbc3>(std::move(rom), ram_size, type, cgb_flag);
            case cartridge_type::mbc5:
            case cartridge_type::mbc5_ram:
            case cartridge_type::mbc5_ram_battery:
            case cartridge_type::mbc5_rumble:
            case cartridge_type::mbc5_rumble_ram:
            case cartridge_type::mbc5_rumble_ram_battery:
                return std::make_unique<mbc5>(std::move(rom), ram_size, type, cgb_flag);
            default:
                throw gbemu_exception{"Unsupported cartridge type"};
        }
    }

} // namespace gbemu
