#pragma once
#ifndef _H_ALU_H_
#    define _H_ALU_H_

#    include <registers.h>

namespace gbemu {
    struct alu {
        alu(registers& r) : regs(r) {}

        std::uint8_t add(std::uint8_t a, std::uint8_t b) {
            auto r = static_cast<std::uint16_t>(a) + b;
            regs.c_flag(r > 0xFF);
            // set z_flag, n_flag, h_flag
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(false);
            regs.h_flag((a & 0xF) + (b & 0xF) > 0xF);
            return static_cast<std::uint8_t>(r);
        }

        std::uint16_t add_hl(std::uint16_t a, std::uint16_t b) {
            auto r = static_cast<std::uint32_t>(a) + b;
            regs.c_flag(r > 0xFFFF);
            regs.n_flag(false);
            regs.h_flag((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
            return static_cast<std::uint16_t>(r);
        }

        std::uint16_t add_sp_s8(std::uint16_t sp, std::int8_t s8) {
            auto u = static_cast<std::uint8_t>(s8);
            regs.z_flag(false);
            regs.n_flag(false);
            regs.h_flag(((sp & 0x0F) + (u & 0x0F)) > 0x0F);
            regs.c_flag(((sp & 0xFF) + (u & 0xFF)) > 0xFF);
            return static_cast<std::uint16_t>(sp + static_cast<std::int16_t>(s8));
        }

        std::uint16_t sub(std::uint16_t a, std::uint16_t b) {
            auto r = static_cast<std::uint32_t>(a) - b;
            regs.c_flag(a < b);
            // set z_flag, n_flag, h_flag
            regs.z_flag(static_cast<std::uint16_t>(r) == 0);
            regs.n_flag(true);
            regs.h_flag((a & 0xFFF) < (b & 0xFFF));
            return static_cast<std::uint16_t>(r);
        }

        std::uint8_t sub(std::uint8_t a, std::uint8_t b) {
            auto r = static_cast<std::uint16_t>(a) - b;
            regs.c_flag(a < b);
            // set z_flag, n_flag, h_flag
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(true);
            regs.h_flag((a & 0xF) < (b & 0xF));
            return static_cast<std::uint8_t>(r);
        }

        void cmp(std::uint8_t a, std::uint8_t b) { sub(a, b); }

        std::uint8_t inc(std::uint8_t a) {
            auto r = a + 1;
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(false);
            regs.h_flag((a & 0xF) == 0xF);
            return r;
        }

        std::uint16_t inc(std::uint16_t a) {
            auto r = a + 1;
            regs.z_flag(static_cast<std::uint16_t>(r) == 0);
            regs.n_flag(false);
            regs.h_flag((a & 0xFFF) == 0xFFF);
            return r;
        }

        std::uint8_t dec(std::uint8_t a) {
            auto r = a - 1;
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(true);
            regs.h_flag((a & 0xF) == 0x0);
            return r;
        }

        std::uint16_t dec(std::uint16_t a) {
            auto r = a - 1;
            regs.z_flag(static_cast<std::uint16_t>(r) == 0);
            regs.n_flag(true);
            regs.h_flag((a & 0xFFF) == 0x0);
            return r;
        }

        std::uint8_t rl(std::uint8_t a) {
            std::uint8_t old_carry = regs.c_flag() ? 1 : 0;
            std::uint8_t r = static_cast<std::uint8_t>((a << 1) | old_carry);
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x80) != 0);
            return r;
        }

        std::uint8_t rlc(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a << 1) | (a >> 7));
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x80) != 0);
            return r;
        }

        std::uint8_t rr(std::uint8_t a) {
            std::uint8_t old_carry = regs.c_flag() ? 0x80 : 0x00;
            std::uint8_t r = static_cast<std::uint8_t>((a >> 1) | old_carry);
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t rrc(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a >> 1) | (a << 7));
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t sla(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>(a << 1);
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x80) != 0);
            return r;
        }

        std::uint8_t sra(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a >> 1) | (a & 0x80));
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t srl(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>(a >> 1);
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t swap(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a << 4) | (a >> 4));
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag(false);
            return r;
        }

        std::uint8_t rla(std::uint8_t a) {
            std::uint8_t old_carry = regs.c_flag() ? 1 : 0;
            std::uint8_t r = static_cast<std::uint8_t>((a << 1) | old_carry);
            regs.z_flag(false);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x80) != 0);
            return r;
        }

        std::uint8_t rlca(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a << 1) | (a >> 7));
            regs.z_flag(false);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x80) != 0);
            return r;
        }

        std::uint8_t rra(std::uint8_t a) {
            std::uint8_t old_carry = regs.c_flag() ? 0x80 : 0x00;
            std::uint8_t r = static_cast<std::uint8_t>((a >> 1) | old_carry);
            regs.z_flag(false);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t rrca(std::uint8_t a) {
            std::uint8_t r = static_cast<std::uint8_t>((a >> 1) | (a << 7));
            regs.z_flag(false);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag((a & 0x01) != 0);
            return r;
        }

        std::uint8_t or_(std::uint8_t a, std::uint8_t b) {
            auto r = a |= b;
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag(false);
            return r;
        }

        std::uint8_t adc(std::uint8_t a, std::uint8_t b) {
            auto cy = regs.c_flag() ? 1 : 0;
            auto r = static_cast<std::uint16_t>(a) + b + cy;
            regs.c_flag(r > 0xFF);
            // set z_flag, n_flag, h_flag
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(false);
            regs.h_flag((a & 0xF) + (b & 0xF) + cy > 0xF);
            return static_cast<std::uint8_t>(r);
        }

        std::uint8_t sbc(std::uint8_t a, std::uint8_t b) {
            auto cy = regs.c_flag() ? 1 : 0;
            auto r = static_cast<std::uint16_t>(a) - b - cy;
            regs.c_flag(a < b + cy);
            // set z_flag, n_flag, h_flag
            regs.z_flag(static_cast<std::uint8_t>(r) == 0);
            regs.n_flag(true);
            regs.h_flag((a & 0xF) < (b & 0xF) + cy);
            return static_cast<std::uint8_t>(r);
        }

        std::uint8_t xor_(std::uint8_t a, std::uint8_t b) {
            auto r = a ^= b;
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(false);
            regs.c_flag(false);
            return r;
        }

        std::uint8_t and_(std::uint8_t a, std::uint8_t b) {
            auto r = a &= b;
            regs.z_flag(r == 0);
            regs.n_flag(false);
            regs.h_flag(true);
            regs.c_flag(false);
            return r;
        }

        std::uint8_t daa(std::uint8_t a) {
            std::uint8_t correction = 0;
            if (regs.h_flag() || (!regs.n_flag() && (a & 0xF) > 9)) {
                correction |= 0x06;
            }
            if (regs.c_flag() || (!regs.n_flag() && a > 0x99)) {
                correction |= 0x60;
                regs.c_flag(true);
            }
            a += regs.n_flag() ? -correction : correction;
            regs.z_flag(a == 0);
            regs.h_flag(false);
            return a;
        }

        std::uint8_t cpl(std::uint8_t a) {
            auto r = ~a;
            regs.n_flag(true);
            regs.h_flag(true);
            return r;
        }

    private:
        registers& regs;
    };
} // namespace gbemu
#endif // _H_ALU_H_