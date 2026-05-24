#pragma once
#ifndef _H_JOYPAD_H_
#    define _H_JOYPAD_H_

#    include <cstdint>

namespace gbemu {
    struct mmu;

    // DMG joypad ($FF00, "P1/JOYP"). The host UI (Application) calls
    // set_button() on each SDL_KEYDOWN/SDL_KEYUP (and SDL_CONTROLLERBUTTON*)
    // event; we keep one bitmask per column and recompute the synthesized
    // FF00 byte every time either the column-select latch is written or a
    // key transitions, so reads via mmu::read_u8 always see a coherent
    // value.
    struct joypad {
        // Index encodes the bit position within the column nibble: D-pad
        // and face buttons each occupy bits 0..3 of the low nibble of $FF00
        // when the corresponding column is selected (bit cleared = active
        // low).
        enum class button : std::uint8_t {
            right = 0,
            left = 1,
            up = 2,
            down = 3,
            a = 4,
            b = 5,
            select = 6,
            start = 7,
        };

        explicit joypad(mmu& m);

        // Transition a single key. A 1→0 transition (press) on any line of
        // the currently-selected column also raises the joypad IRQ
        // ($IF.4) — Tetris doesn't enable IE.4 so this is dormant for it,
        // but it is needed for STOP-wake-on-keypress games.
        void set_button(button b, bool pressed);

    private:
        void on_p1_write(std::uint8_t val);
        void refresh_p1();

        mmu& mmu_;
        std::uint8_t dpad_{0}; // bit n set ⇒ button n (right/left/up/down) held
        std::uint8_t btns_{0}; // bit n set ⇒ button (n+4) (a/b/select/start) held
    };
} // namespace gbemu

#endif // _H_JOYPAD_H_
