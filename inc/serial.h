#pragma once
#ifndef _H_SERIAL_H_
#    define _H_SERIAL_H_

#    include <cstdint>

namespace gbemu {
    struct mmu;

    struct serial {
        explicit serial(mmu& m);

    private:
        mmu& mmu_;
        void on_sc_write(std::uint8_t val);
    };
} // namespace gbemu

#endif // _H_SERIAL_H_
