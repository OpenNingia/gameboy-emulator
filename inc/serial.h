#pragma once
#ifndef _H_SERIAL_H_
#    define _H_SERIAL_H_

#    include <cstdint>

namespace gbemu {
    struct mmu;
    struct irq;

    struct serial {
        serial(mmu& m, irq& i);

    private:
        mmu& mmu_;
        irq& irq_;
        void on_sc_write(std::uint8_t val);
    };
} // namespace gbemu

#endif // _H_SERIAL_H_
