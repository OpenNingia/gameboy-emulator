#pragma once
#ifndef _H_INSTRUCTION_H_
#    define _H_INSTRUCTION_H_

#    include <cstdint>
#    include <string>

namespace gbemu {

    struct cpu;
    struct instruction {
        instruction(std::string m, std::uint8_t c, std::uint8_t ct)
            : mnemonic(std::move(m)), cycles(c), cycles_taken(ct) {}
        //
        std::string mnemonic;
        std::uint8_t cycles;
        std::uint8_t cycles_taken; // == cycles tranne sui branch condizionali

        void operator()(cpu& cpu) { execute(cpu); };

    protected:
        virtual void execute(cpu& cpu) = 0;
    };
} // namespace gbemu

#endif // _H_INSTRUCTION_H_