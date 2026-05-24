#include <cstddef>
#include <utility>

#include <exc.hpp>
#include <log.h>
#include <mbc.h>

namespace gbemu {
    namespace {

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
            no_mbc(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type)
                : rom_(std::move(rom)), ram_(ram_bytes, 0u), type_(type) {}

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

        private:
            std::vector<std::uint8_t> rom_;
            std::vector<std::uint8_t> ram_;
            std::uint8_t type_{0};
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
            mbc1(std::vector<std::uint8_t> rom, std::size_t ram_bytes, std::uint8_t type)
                : rom_(std::move(rom)),
                  ram_(ram_bytes, 0u),
                  rom_bank_mask_(rom_bank_mask_for(rom_.size())),
                  type_(type) {}

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
        };

    } // namespace

    std::unique_ptr<mbc> make_mbc(std::vector<std::uint8_t> rom) {
        if (rom.size() < 0x0150)
            throw gbemu_exception{"ROM too small to contain a header"};

        std::uint8_t type = rom[header_type_addr];
        std::uint8_t rom_size_code = rom[header_rom_size_addr];
        std::uint8_t ram_size_code = rom[header_ram_size_addr];

        std::size_t expected_rom_size = std::size_t{32 * 1024} << rom_size_code;
        std::size_t ram_size = decode_ram_size(ram_size_code);

        LOG_INFO(log::root(), "Cartridge: type=0x{:02x} rom_size=0x{:02x} ({} KB) ram_size=0x{:02x} ({} KB)", type,
                 rom_size_code, expected_rom_size / 1024, ram_size_code, ram_size / 1024);

        switch (static_cast<cartridge_type>(type)) {
            case cartridge_type::rom_only:
                return std::make_unique<no_mbc>(std::move(rom), 0, type);
            case cartridge_type::rom_ram:
            case cartridge_type::rom_ram_battery:
                return std::make_unique<no_mbc>(std::move(rom), ram_size, type);
            case cartridge_type::mbc1:
            case cartridge_type::mbc1_ram:
            case cartridge_type::mbc1_ram_battery:
                return std::make_unique<mbc1>(std::move(rom), ram_size, type);
            default:
                throw gbemu_exception{"Unsupported cartridge type"};
        }
    }

} // namespace gbemu
