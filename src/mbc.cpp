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
                }
                // Writes to 0x0000-0x7FFF on a no-MBC cart are ignored.
            }

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
                    if (ram_enabled_ && !ram_.empty())
                        ram_[ram_offset(addr)] = val;
                }
            }

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
                    if (ram_enabled_)
                        ram_[(addr - 0xA000) & 0x01FF] = val & 0x0F;
                }
                // 0x4000-0x7FFF writes are ignored on MBC2.
            }

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
        // - 0x6000-0x7FFF write: latch clock data.  A 0 followed by a 1
        //   latches the current RTC values into the read-back registers.
        //   We track the transition but the RTC itself is a stub.
        // - 0xA000-0xBFFF: external RAM bank or latched RTC register,
        //   depending on the current select.  Gated by `ram_enabled_`.
        //
        // RTC support is intentionally a stub: registers read back 0,
        // writes are ignored, no time-keeping is performed.  Pokémon
        // Red/Blue (type 0x13) has no RTC; Pokémon Gold/Crystal (types
        // 0x10) tolerate a frozen clock — the day/night cycle simply
        // doesn't advance.
        // ----------------------------------------------------------------
        class mbc3 final : public mbc {
        public:
            mbc3(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type, std::uint8_t cgb_flag)
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
                    // RTC register 0x08-0x0C.  Stubbed: reads return 0.
                    if (ram_rtc_select_ >= 0x08 && ram_rtc_select_ <= 0x0C)
                        return 0x00;
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
                    // Latch clock data write — RTC is stubbed, so the 0->1
                    // edge that would normally latch is a no-op here.
                } else if (addr >= 0xA000 && addr < 0xC000) {
                    if (!ram_enabled_)
                        return;
                    if (ram_rtc_select_ <= 0x03) {
                        if (!ram_.empty())
                            ram_[ram_offset(addr)] = val;
                    }
                    // RTC register writes ignored (stub).
                }
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

            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t rom_bank_{1};
            std::uint8_t ram_rtc_select_{0};
            bool ram_enabled_{false};
            std::uint16_t rom_bank_mask_{0};
            std::uint8_t type_{0};
            std::uint8_t cgb_flag_{0};
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
                    if (ram_enabled_ && !ram_.empty())
                        ram_[ram_offset(addr)] = val;
                }
                // 0x6000-0x7FFF writes are ignored on MBC5.
            }

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
        };

    } // namespace

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
