#pragma once
#ifndef _H_AUDIO_RING_BUFFER_H_
#define _H_AUDIO_RING_BUFFER_H_

#include <atomic>
#include <thread>

namespace gbemu {
    class audio_ring_buffer {
        static constexpr int SIZE = 4096; // campioni, deve essere potenza di 2
        float buffer[SIZE * 2];           // stereo, quindi *2
        std::atomic<int> write_pos{0};
        std::atomic<int> read_pos{0};

    public:
        int available_write() {
            int w = write_pos.load();
            int r = read_pos.load();
            return (SIZE - 1) - ((w - r + SIZE) % SIZE);
        }

        void push(float left, float right) {
            // blocca se il buffer è pieno → backpressure
            while (available_write() < 2) {
                std::this_thread::yield(); // oppure sleep_for(1us)
            }
            int w = write_pos.load();
            buffer[w * 2] = left;
            buffer[w * 2 + 1] = right;
            write_pos.store((w + 1) % SIZE);
        }

        bool pop(float& left, float& right) {
            int r = read_pos.load();
            int w = write_pos.load();
            if (r == w)
                return false; // buffer vuoto (underrun)
            left = buffer[r * 2];
            right = buffer[r * 2 + 1];
            read_pos.store((r + 1) % SIZE);
            return true;
        }
    };
} // namespace gbemu

#endif /* _H_AUDIO_RING_BUFFER_H_ */