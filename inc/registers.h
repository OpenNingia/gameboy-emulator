#pragma once
#ifndef _H_REGISTERS_H_
#    define _H_REGISTERS_H_

#    include <cstdint>

namespace gbemu {
    union u16reg {
        std::uint16_t u16;
        struct {
            std::uint8_t lo;
            std::uint8_t hi;
        };
    };

    struct registers {
        u16reg af;
        u16reg bc;
        u16reg de;
        u16reg hl;

        std::uint16_t sp;
        std::uint16_t pc;

        // flags
        // zero
        bool z_flag() const { return af.lo & 0x80; }
        void z_flag(bool v) {
            if (v)
                af.lo |= 0x80;
            else
                af.lo &= 0x7F;
        }

        // sub carry
        bool n_flag() const { return af.lo & 0x40; }
        void n_flag(bool v) {
            if (v)
                af.lo |= 0x40;
            else
                af.lo &= 0xBF;
        }

        // half carry
        bool h_flag() const { return af.lo & 0x20; }
        void h_flag(bool v) {
            if (v)
                af.lo |= 0x20;
            else
                af.lo &= 0xDF;
        }

        // carry
        bool c_flag() const { return af.lo & 0x10; }
        void c_flag(bool v) {
            if (v)
                af.lo |= 0x10;
            else
                af.lo &= 0xEF;
        }
    };
} // namespace gbemu
#endif // _H_REGISTERS_H_