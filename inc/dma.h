#pragma once
#ifndef _H_DMA_H_
#    define _H_DMA_H_

#    include <cstdint>

namespace gbemu {
    struct mmu;

    struct dma {
        explicit dma(mmu& m);

    private:
        mmu& mmu_;
        void trigger(std::uint8_t val);
    };
} // namespace gbemu

#endif // _H_DMA_H_
