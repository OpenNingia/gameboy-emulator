#include <joypad.h>
#include <mmu.h>

using namespace gbemu;

joypad::joypad(mmu& m) : mmu_(m) {
    mmu_.add_mmio_write_handler(0xFF00, [this](std::uint8_t v) { on_p1_write(v); });
    refresh_p1();
}

void joypad::on_p1_write(std::uint8_t /*val*/) {
    // The MMU has already stored the written byte in mmio[0]; we only care
    // about bits 5-4 (column select). refresh_p1 reads the latched select
    // bits and rewrites the byte with the synthesized low nibble so the
    // next read returns a coherent value.
    refresh_p1();
}

void joypad::set_button(button b, bool pressed) {
    const auto bit = static_cast<std::uint8_t>(b);
    const bool is_dpad = bit < 4;
    auto& mask = is_dpad ? dpad_ : btns_;
    const auto idx = static_cast<std::uint8_t>(is_dpad ? bit : (bit - 4));
    const std::uint8_t flag = static_cast<std::uint8_t>(1u << idx);
    const bool was_pressed = (mask & flag) != 0;
    if (pressed)
        mask = static_cast<std::uint8_t>(mask | flag);
    else
        mask = static_cast<std::uint8_t>(mask & ~flag);

    refresh_p1();

    // Joypad IRQ (IF.4): real hardware raises it on a 1→0 transition of
    // any input line in the currently-selected column. We only fire on a
    // fresh press of a button that lives in a column the game is currently
    // polling — releases never raise it.
    if (pressed && !was_pressed) {
        const auto sel = static_cast<std::uint8_t>(mmu_.mmio[0] & 0x30);
        const bool col_selected = is_dpad ? ((sel & 0x10) == 0) : ((sel & 0x20) == 0);
        if (col_selected)
            mmu_.mmio[0x0F] = static_cast<std::uint8_t>(mmu_.mmio[0x0F] | 0x10);
    }
}

void joypad::refresh_p1() {
    const auto sel = static_cast<std::uint8_t>(mmu_.mmio[0] & 0x30);
    std::uint8_t low = 0x0F; // active-low: 1 = released
    if ((sel & 0x10) == 0)
        low = static_cast<std::uint8_t>(low & ~dpad_);
    if ((sel & 0x20) == 0)
        low = static_cast<std::uint8_t>(low & ~btns_);
    mmu_.mmio[0] = static_cast<std::uint8_t>(0xC0 | sel | low);
}
