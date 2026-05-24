#include <apu.h>

using namespace gbemu;

namespace {
    constexpr std::uint64_t CPU_HZ = 4194304;

    // Frame sequencer ticks at 512 Hz. 4194304 / 512 = 8192 T-cycles/step.
    constexpr std::uint32_t FRAME_SEQ_PERIOD = 8192;

    // 8-step duty patterns. Bit ordering: MSB is step 0, LSB is step 7.
    constexpr std::uint8_t DUTY_TABLE[4] = {
        0b00000001, // 12.5%
        0b10000001, // 25%
        0b10000111, // 50%
        0b01111110, // 75%
    };

    constexpr std::uint16_t NR50_OFF = 0xFF24 - 0xFF00;
    constexpr std::uint16_t NR51_OFF = 0xFF25 - 0xFF00;
    constexpr std::uint16_t NR52_OFF = 0xFF26 - 0xFF00;

    // Bits forced to 1 on reads of $FF10-$FF25. Write-only fields and
    // unused address slots read back as all-1s on real hardware.
    constexpr std::uint8_t READ_MASKS[] = {
        0x80, 0x3F, 0x00, 0xFF, 0xBF, // NR10 NR11 NR12 NR13 NR14
        0xFF, 0x3F, 0x00, 0xFF, 0xBF, // unused NR21 NR22 NR23 NR24
        0x7F, 0xFF, 0x9F, 0xFF, 0xBF, // NR30 NR31 NR32 NR33 NR34
        0xFF, 0xFF, 0x00, 0x00, 0xBF, // unused NR41 NR42 NR43 NR44
        0x00, 0x00,                   // NR50 NR51
    };
    static_assert(sizeof(READ_MASKS) == (0xFF25 - 0xFF10 + 1));
} // namespace

apu::apu(mmu& mmu) : mmu_(mmu) {
    // CH1: NR10 (sweep), NR11-NR14 (square + length/envelope/freq/trigger).
    mmu_.add_mmio_write_handler(0xFF10, [this](std::uint8_t v) { on_nr10(v); });
    mmu_.add_mmio_write_handler(0xFF11, [this](std::uint8_t v) { on_nr11(v); });
    mmu_.add_mmio_write_handler(0xFF12, [this](std::uint8_t v) { on_nr12(v); });
    mmu_.add_mmio_write_handler(0xFF13, [this](std::uint8_t v) { on_nr13(v); });
    mmu_.add_mmio_write_handler(0xFF14, [this](std::uint8_t v) { on_nr14(v); });
    // CH2: NR21-NR24.
    mmu_.add_mmio_write_handler(0xFF16, [this](std::uint8_t v) { on_nr21(v); });
    mmu_.add_mmio_write_handler(0xFF17, [this](std::uint8_t v) { on_nr22(v); });
    mmu_.add_mmio_write_handler(0xFF18, [this](std::uint8_t v) { on_nr23(v); });
    mmu_.add_mmio_write_handler(0xFF19, [this](std::uint8_t v) { on_nr24(v); });
    // CH3: NR30-NR34 (wave: DAC / length / level / freq / trigger).
    mmu_.add_mmio_write_handler(0xFF1A, [this](std::uint8_t v) { on_nr30(v); });
    mmu_.add_mmio_write_handler(0xFF1B, [this](std::uint8_t v) { on_nr31(v); });
    mmu_.add_mmio_write_handler(0xFF1C, [this](std::uint8_t v) { on_nr32(v); });
    mmu_.add_mmio_write_handler(0xFF1D, [this](std::uint8_t v) { on_nr33(v); });
    mmu_.add_mmio_write_handler(0xFF1E, [this](std::uint8_t v) { on_nr34(v); });
    // CH4: NR41-NR44 (noise: length / envelope / polynomial / trigger).
    mmu_.add_mmio_write_handler(0xFF20, [this](std::uint8_t v) { on_nr41(v); });
    mmu_.add_mmio_write_handler(0xFF21, [this](std::uint8_t v) { on_nr42(v); });
    mmu_.add_mmio_write_handler(0xFF22, [this](std::uint8_t v) { on_nr43(v); });
    mmu_.add_mmio_write_handler(0xFF23, [this](std::uint8_t v) { on_nr44(v); });
    // $FF15 and $FF1F are unused / unmapped — reads always return 0xFF.
    // A trivial handler keeps the mmio byte pinned to 0xFF after any write.
    mmu_.add_mmio_write_handler(0xFF15, [this](std::uint8_t v) { apply_read_mask(0xFF15, v); });
    mmu_.add_mmio_write_handler(0xFF1F, [this](std::uint8_t v) { apply_read_mask(0xFF1F, v); });
    // Pre-stamp the read-mask bits across the entire NR10..NR51 range so
    // reads issued before any ROM-driven write return the canonical
    // "mostly 1s" byte for each register (matches DMG power-on state for
    // the unused / write-only bit positions).
    for (std::uint16_t a = 0xFF10; a <= 0xFF25; ++a) {
        mmu_.mmio[a - 0xFF00] = READ_MASKS[a - 0xFF10];
    }
    // Master / status — NR52 is the power switch + per-channel status.
    mmu_.add_mmio_write_handler(0xFF26, [this](std::uint8_t v) { on_nr52(v); });
    // DIV write quirk — only fires on ROM-driven writes now that timer::step
    // increments DIV via direct mmio write (bypassing write_u8). Resetting
    // the system counter on a ROM DIV write can produce an extra frame
    // sequencer step if the FS-clocking bit was high before the reset.
    mmu_.add_mmio_write_handler(0xFF04, [this](std::uint8_t v) { on_div_write(v); });
}

// --- square_channel: register loads -----------------------------------------

void apu::square_channel::load_nrx1(std::uint8_t v) {
    duty = (v >> 6) & 0x03;
    // Length load is the low 6 bits. We model it as count-down from
    // `64 - load`; equivalent to the hardware count-up to 64.
    length = 64 - (v & 0x3F);
}

void apu::square_channel::load_nrx2(std::uint8_t v) {
    env_initial_vol = (v >> 4) & 0x0F;
    env_increase = (v & 0x08) != 0;
    env_period = v & 0x07;
    // DAC enable = any of the upper 5 bits set. Disabling the DAC forces
    // the channel off immediately; a subsequent trigger cannot re-enable it
    // until the DAC is brought back on.
    dac_enabled = (v & 0xF8) != 0;
    if (!dac_enabled)
        channel_enabled = false;
}

void apu::square_channel::load_nrx3(std::uint8_t v) {
    freq_raw = (freq_raw & 0x0700) | v;
}

bool apu::square_channel::load_nrx4(std::uint8_t v) {
    freq_raw = (freq_raw & 0x00FF) | (std::uint16_t(v & 0x07) << 8);
    length_enabled = (v & 0x40) != 0;
    return (v & 0x80) != 0;
}

// --- square_channel: ticks --------------------------------------------------

void apu::square_channel::tick_frequency(std::uint32_t cycles) {
    freq_timer -= static_cast<std::int32_t>(cycles);
    while (freq_timer <= 0) {
        const std::int32_t period = (2048 - static_cast<std::int32_t>(freq_raw)) * 4;
        freq_timer += period;
        duty_pos = (duty_pos + 1) & 0x07;
    }
}

void apu::square_channel::tick_length() {
    if (length_enabled && length > 0) {
        if (--length == 0)
            channel_enabled = false;
    }
}

void apu::square_channel::tick_envelope() {
    if (env_period == 0)
        return;
    if (env_timer > 0)
        --env_timer;
    if (env_timer == 0) {
        env_timer = env_period;
        if (env_increase && volume < 15)
            ++volume;
        else if (!env_increase && volume > 0)
            --volume;
    }
}

void apu::square_channel::trigger() {
    channel_enabled = dac_enabled;
    if (length == 0)
        length = 64;
    freq_timer = (2048 - static_cast<std::int32_t>(freq_raw)) * 4;
    volume = env_initial_vol;
    env_timer = env_period;
    duty_pos = 0;
}

float apu::square_channel::sample() const {
    // Invariant: channel_enabled can only become true via trigger(), and
    // trigger() copies dac_enabled into channel_enabled. Disabling the DAC
    // in load_nrx2 clears channel_enabled in the same write. So
    // channel_enabled already implies dac_enabled.
    if (!channel_enabled)
        return 0.0f;
    const std::uint8_t bit = (DUTY_TABLE[duty] >> (7 - duty_pos)) & 0x01;
    const std::uint8_t dac_in = bit * volume; // 0..15
    return (static_cast<float>(dac_in) - 7.5f) / 7.5f;
}

// --- sweep_unit -------------------------------------------------------------

void apu::sweep_unit::load_nr10(std::uint8_t v) {
    period = (v >> 4) & 0x07;
    decrease = (v & 0x08) != 0;
    shift = v & 0x07;
}

bool apu::sweep_unit::trigger(std::uint16_t channel_freq) {
    shadow_freq = channel_freq;
    // Hardware quirk: a period of 0 reloads the timer as 8.
    timer = (period > 0) ? period : 8;
    enabled = (period > 0) || (shift > 0);
    // Immediate overflow check on trigger when shift>0. Result is NOT
    // written back on the trigger calculation — only the periodic tick does
    // the write-back.
    if (shift > 0) {
        std::uint16_t dummy;
        return calc(dummy);
    }
    return false;
}

bool apu::sweep_unit::tick(std::uint16_t& freq_inout) {
    if (timer > 0)
        --timer;
    if (timer != 0)
        return false;
    timer = (period > 0) ? period : 8;
    // A period of 0 means "keep counting but never fire" — the timer still
    // reloads (to 8) on each clock so the unit stays alive, but no
    // frequency calculation happens.
    if (!enabled || period == 0)
        return false;
    std::uint16_t new_freq;
    if (calc(new_freq))
        return true;
    if (shift > 0) {
        shadow_freq = new_freq;
        freq_inout = new_freq;
        // Second overflow check — does NOT write back. This catches the
        // case where the write-back result itself would overflow on the
        // next iteration.
        std::uint16_t dummy;
        if (calc(dummy))
            return true;
    }
    return false;
}

bool apu::sweep_unit::calc(std::uint16_t& out) const {
    const std::uint32_t delta = static_cast<std::uint32_t>(shadow_freq) >> shift;
    std::uint32_t new_freq;
    if (decrease) {
        // shadow_freq <= 2047 and delta <= shadow_freq → never underflows.
        new_freq = static_cast<std::uint32_t>(shadow_freq) - delta;
    } else {
        new_freq = static_cast<std::uint32_t>(shadow_freq) + delta;
    }
    out = static_cast<std::uint16_t>(new_freq & 0x07FF);
    return new_freq > 2047;
}

// --- wave_channel -----------------------------------------------------------

void apu::wave_channel::load_nr30(std::uint8_t v) {
    dac_enabled = (v & 0x80) != 0;
    if (!dac_enabled)
        channel_enabled = false;
}

void apu::wave_channel::load_nr31(std::uint8_t v) {
    // CH3 length is 8-bit (256-step) — full byte counts down toward 0.
    length = 256 - static_cast<std::uint16_t>(v);
}

void apu::wave_channel::load_nr32(std::uint8_t v) {
    output_level = (v >> 5) & 0x03;
}

void apu::wave_channel::load_nr33(std::uint8_t v) {
    freq_raw = (freq_raw & 0x0700) | v;
}

bool apu::wave_channel::load_nr34(std::uint8_t v) {
    freq_raw = (freq_raw & 0x00FF) | (std::uint16_t(v & 0x07) << 8);
    length_enabled = (v & 0x40) != 0;
    return (v & 0x80) != 0;
}

void apu::wave_channel::tick_frequency(std::uint32_t cycles, const std::uint8_t* wave_ram) {
    freq_timer -= static_cast<std::int32_t>(cycles);
    while (freq_timer <= 0) {
        // Wave period is (2048 - freq) * 2 — half the square period because
        // CH3 advances one 4-bit sample per step, not one duty bit.
        freq_timer += (2048 - static_cast<std::int32_t>(freq_raw)) * 2;
        wave_pos = (wave_pos + 1) & 0x1F;
        const std::uint8_t byte = wave_ram[wave_pos >> 1];
        sample_buffer = (wave_pos & 1) ? (byte & 0x0F) : (byte >> 4);
    }
}

void apu::wave_channel::tick_length() {
    if (length_enabled && length > 0) {
        if (--length == 0)
            channel_enabled = false;
    }
}

void apu::wave_channel::trigger() {
    channel_enabled = dac_enabled;
    if (length == 0)
        length = 256;
    freq_timer = (2048 - static_cast<std::int32_t>(freq_raw)) * 2;
    wave_pos = 0;
    // sample_buffer intentionally NOT reset — matches DMG behavior where
    // the first sample after a trigger is the previously buffered nibble.
}

float apu::wave_channel::sample() const {
    // Mute output_level=0 returns 0 (not -1) so we don't inject a DC step
    // every time a channel mutes — keeps clicks out of the mixer.
    if (!channel_enabled || output_level == 0)
        return 0.0f;
    static constexpr std::uint8_t SHIFT_TABLE[4] = {4, 0, 1, 2};
    const std::uint8_t shifted = sample_buffer >> SHIFT_TABLE[output_level];
    return (static_cast<float>(shifted) - 7.5f) / 7.5f;
}

// --- noise_channel ----------------------------------------------------------

void apu::noise_channel::load_nr41(std::uint8_t v) {
    // Only the low 6 bits matter for length; bits 7-6 are unused.
    length = 64 - (v & 0x3F);
}

void apu::noise_channel::load_nr42(std::uint8_t v) {
    env_initial_vol = (v >> 4) & 0x0F;
    env_increase = (v & 0x08) != 0;
    env_period = v & 0x07;
    dac_enabled = (v & 0xF8) != 0;
    if (!dac_enabled)
        channel_enabled = false;
}

void apu::noise_channel::load_nr43(std::uint8_t v) {
    shift_clock = (v >> 4) & 0x0F;
    width_7bit = (v & 0x08) != 0;
    divisor_code = v & 0x07;
}

bool apu::noise_channel::load_nr44(std::uint8_t v) {
    length_enabled = (v & 0x40) != 0;
    return (v & 0x80) != 0;
}

void apu::noise_channel::tick_frequency(std::uint32_t cycles) {
    freq_timer -= static_cast<std::int32_t>(cycles);
    while (freq_timer <= 0) {
        // Period = (divisor_code == 0 ? 8 : divisor_code * 16) << shift_clock.
        const std::int32_t base = (divisor_code == 0) ? 8 : (divisor_code * 16);
        const std::int32_t period = base << shift_clock;
        freq_timer += period;

        // Clock the LFSR: xor bit 0 and bit 1, shift right, drop xor into
        // bit 14. In 7-bit width mode the xor result is ALSO written into
        // bit 6, which gives the LFSR a much shorter period and a more
        // metallic / pitched tone.
        const std::uint16_t xor_bit = (lfsr ^ (lfsr >> 1)) & 0x0001;
        lfsr = (lfsr >> 1) | (xor_bit << 14);
        if (width_7bit) {
            // Clear bit 6 then OR the xor result into it.
            lfsr = (lfsr & ~static_cast<std::uint16_t>(0x0040)) | (xor_bit << 6);
        }
    }
}

void apu::noise_channel::tick_length() {
    if (length_enabled && length > 0) {
        if (--length == 0)
            channel_enabled = false;
    }
}

void apu::noise_channel::tick_envelope() {
    if (env_period == 0)
        return;
    if (env_timer > 0)
        --env_timer;
    if (env_timer == 0) {
        env_timer = env_period;
        if (env_increase && volume < 15)
            ++volume;
        else if (!env_increase && volume > 0)
            --volume;
    }
}

void apu::noise_channel::trigger() {
    channel_enabled = dac_enabled;
    if (length == 0)
        length = 64;
    volume = env_initial_vol;
    env_timer = env_period;
    lfsr = 0x7FFF;
    const std::int32_t base = (divisor_code == 0) ? 8 : (divisor_code * 16);
    freq_timer = base << shift_clock;
}

float apu::noise_channel::sample() const {
    if (!channel_enabled)
        return 0.0f;
    // Output is the inverted low bit of the LFSR — bit 0 == 0 produces a 1.
    const std::uint8_t bit = static_cast<std::uint8_t>((~lfsr) & 0x0001);
    const std::uint8_t dac_in = bit * volume; // 0..15
    return (static_cast<float>(dac_in) - 7.5f) / 7.5f;
}

// --- MMIO write handlers ----------------------------------------------------

// GATE: skip NRxx writes when the APU is powered off. The mmio byte the MMU
// just stored is rewritten with `0 | read_mask`, so subsequent reads return
// the mask-only value (matches DMG: write-only/unused bits still read 1 even
// with the master off).
#define GATE(addr)                                  \
    do {                                            \
        if (!powered_) {                            \
            apply_read_mask((addr), 0);             \
            return;                                 \
        }                                           \
    } while (0)

void apu::on_nr10(std::uint8_t v) {
    GATE(0xFF10);
    ch1_sweep_.load_nr10(v);
    apply_read_mask(0xFF10, v);
}

void apu::on_nr11(std::uint8_t v) {
    GATE(0xFF11);
    ch1_sq_.load_nrx1(v);
    apply_read_mask(0xFF11, v);
}

void apu::on_nr12(std::uint8_t v) {
    GATE(0xFF12);
    ch1_sq_.load_nrx2(v);
    apply_read_mask(0xFF12, v);
}

void apu::on_nr13(std::uint8_t v) {
    GATE(0xFF13);
    ch1_sq_.load_nrx3(v);
    apply_read_mask(0xFF13, v);
}

void apu::on_nr14(std::uint8_t v) {
    GATE(0xFF14);
    if (ch1_sq_.load_nrx4(v)) {
        ch1_sq_.trigger();
        if (ch1_sweep_.trigger(ch1_sq_.freq_raw))
            ch1_sq_.channel_enabled = false;
    }
    apply_read_mask(0xFF14, v);
}

void apu::on_nr21(std::uint8_t v) {
    GATE(0xFF16);
    ch2_.load_nrx1(v);
    apply_read_mask(0xFF16, v);
}

void apu::on_nr22(std::uint8_t v) {
    GATE(0xFF17);
    ch2_.load_nrx2(v);
    apply_read_mask(0xFF17, v);
}

void apu::on_nr23(std::uint8_t v) {
    GATE(0xFF18);
    ch2_.load_nrx3(v);
    apply_read_mask(0xFF18, v);
}

void apu::on_nr24(std::uint8_t v) {
    GATE(0xFF19);
    if (ch2_.load_nrx4(v))
        ch2_.trigger();
    apply_read_mask(0xFF19, v);
}

void apu::on_nr30(std::uint8_t v) {
    GATE(0xFF1A);
    ch3_.load_nr30(v);
    apply_read_mask(0xFF1A, v);
}

void apu::on_nr31(std::uint8_t v) {
    GATE(0xFF1B);
    ch3_.load_nr31(v);
    apply_read_mask(0xFF1B, v);
}

void apu::on_nr32(std::uint8_t v) {
    GATE(0xFF1C);
    ch3_.load_nr32(v);
    apply_read_mask(0xFF1C, v);
}

void apu::on_nr33(std::uint8_t v) {
    GATE(0xFF1D);
    ch3_.load_nr33(v);
    apply_read_mask(0xFF1D, v);
}

void apu::on_nr34(std::uint8_t v) {
    GATE(0xFF1E);
    if (ch3_.load_nr34(v))
        ch3_.trigger();
    apply_read_mask(0xFF1E, v);
}

void apu::on_nr41(std::uint8_t v) {
    GATE(0xFF20);
    ch4_.load_nr41(v);
    apply_read_mask(0xFF20, v);
}

void apu::on_nr42(std::uint8_t v) {
    GATE(0xFF21);
    ch4_.load_nr42(v);
    apply_read_mask(0xFF21, v);
}

void apu::on_nr43(std::uint8_t v) {
    GATE(0xFF22);
    ch4_.load_nr43(v);
    apply_read_mask(0xFF22, v);
}

void apu::on_nr44(std::uint8_t v) {
    GATE(0xFF23);
    if (ch4_.load_nr44(v))
        ch4_.trigger();
    apply_read_mask(0xFF23, v);
}

#undef GATE

void apu::on_nr52(std::uint8_t v) {
    const bool new_power = (v & 0x80) != 0;
    if (powered_ && !new_power) {
        power_off(); // wipes channel state and NR10..NR25 mmio storage
    } else if (!powered_ && new_power) {
        // Power on: frame sequencer step resets so the next clock starts a
        // fresh sequence. Channel state stays zeroed (cleared by the prior
        // power_off); only the master flag flips.
        frame_seq_step_ = 0;
        powered_ = true;
    }
    refresh_nr52_status();
}

void apu::on_div_write(std::uint8_t /*v*/) {
    // The frame sequencer is clocked by the falling edge of bit 12 of the
    // 16-bit system counter (= bit 4 of DIV). When DIV is written, the
    // system counter resets to 0; if that bit was 1 before the reset, the
    // FS sees a 1→0 transition and ticks once. Our 8192-cycle accumulator
    // mirrors counter mod 8192, so positions in [4096, 8191] map to
    // "bit 12 was high".
    if (frame_seq_acc_ >= 4096) {
        do_frame_seq_step();
    }
    frame_seq_acc_ = 0;
}

void apu::power_off() {
    // DMG quirk: length counters and their enables survive power-off. The
    // rest of channel state and the NRxx mmio storage are wiped.
    const std::uint8_t ch1_len = ch1_sq_.length;
    const bool ch1_le = ch1_sq_.length_enabled;
    const std::uint8_t ch2_len = ch2_.length;
    const bool ch2_le = ch2_.length_enabled;
    const std::uint16_t ch3_len = ch3_.length;
    const bool ch3_le = ch3_.length_enabled;
    const std::uint8_t ch4_len = ch4_.length;
    const bool ch4_le = ch4_.length_enabled;

    ch1_sq_ = square_channel{};
    ch1_sweep_ = sweep_unit{};
    ch2_ = square_channel{};
    ch3_ = wave_channel{};
    ch4_ = noise_channel{};

    ch1_sq_.length = ch1_len; ch1_sq_.length_enabled = ch1_le;
    ch2_.length = ch2_len;    ch2_.length_enabled = ch2_le;
    ch3_.length = ch3_len;    ch3_.length_enabled = ch3_le;
    ch4_.length = ch4_len;    ch4_.length_enabled = ch4_le;

    // Wipe NR10..NR25 (logical value = 0) but leave the read-mask bits set
    // so reads after power-off still return the canonical "mostly 1s" byte
    // for each address. Wave RAM ($FF30-$FF3F) is preserved.
    for (std::uint16_t a = 0xFF10; a <= 0xFF25; ++a) {
        mmu_.mmio[a - 0xFF00] = READ_MASKS[a - 0xFF10];
    }
    powered_ = false;
}

void apu::apply_read_mask(std::uint16_t addr, std::uint8_t v) {
    mmu_.mmio[addr - 0xFF00] = v | READ_MASKS[addr - 0xFF10];
}

void apu::refresh_nr52_status() {
    // Preserve the written master-power bit, force unused bits to 1, and
    // OR in the live channel-enabled status (bits 0-3).
    std::uint8_t v = (mmu_.mmio[NR52_OFF] & 0x80) | 0x70;
    if (ch1_sq_.channel_enabled) v |= 0x01;
    if (ch2_.channel_enabled)    v |= 0x02;
    if (ch3_.channel_enabled)    v |= 0x04;
    if (ch4_.channel_enabled)    v |= 0x08;
    mmu_.mmio[NR52_OFF] = v;
}

// --- frame sequencer + step + emit_sample -----------------------------------

void apu::do_frame_seq_step() {
    switch (frame_seq_step_) {
        case 0:
        case 4:
            ch1_sq_.tick_length();
            ch2_.tick_length();
            ch3_.tick_length();
            ch4_.tick_length();
            break;
        case 2:
        case 6:
            ch1_sq_.tick_length();
            ch2_.tick_length();
            ch3_.tick_length();
            ch4_.tick_length();
            if (ch1_sweep_.tick(ch1_sq_.freq_raw))
                ch1_sq_.channel_enabled = false;
            break;
        case 7:
            ch1_sq_.tick_envelope();
            ch2_.tick_envelope();
            ch4_.tick_envelope();
            break;
        default:
            break;
    }
    frame_seq_step_ = (frame_seq_step_ + 1) & 0x07;
}

void apu::advance_frame_sequencer(std::uint32_t cycles) {
    frame_seq_acc_ += cycles;
    while (frame_seq_acc_ >= FRAME_SEQ_PERIOD) {
        frame_seq_acc_ -= FRAME_SEQ_PERIOD;
        do_frame_seq_step();
    }
}

void apu::step(std::uint32_t cycles) {
    advance_frame_sequencer(cycles);
    ch1_sq_.tick_frequency(cycles);
    ch2_.tick_frequency(cycles);
    // CH3 reads its waveform directly from MMU-backed wave RAM at $FF30.
    ch3_.tick_frequency(cycles, &mmu_.mmio[0xFF30 - 0xFF00]);
    ch4_.tick_frequency(cycles);

    // Keep NR52's read-back channel-enabled bits in sync so ROMs polling
    // NR52 see length/sweep/envelope-driven disables in close to real time.
    refresh_nr52_status();

    sample_acc_ += static_cast<std::uint64_t>(cycles) * SAMPLE_RATE;
    while (sample_acc_ >= CPU_HZ) {
        sample_acc_ -= CPU_HZ;
        emit_sample();
    }
}

void apu::emit_sample() {
    if (!output_enabled_)
        return;

    const std::uint8_t nr52 = mmu_.mmio[NR52_OFF];
    if ((nr52 & 0x80) == 0) {
        ring_.push(0.0f, 0.0f);
        return;
    }

    const float ch1 = ch1_sq_.sample();
    const float ch2 = ch2_.sample();
    const float ch3 = ch3_.sample();
    const float ch4 = ch4_.sample();

    const std::uint8_t nr51 = mmu_.mmio[NR51_OFF];
    float l = 0.0f, r = 0.0f;
    if (nr51 & 0x10) l += ch1; // bit 4: CH1 left
    if (nr51 & 0x01) r += ch1; // bit 0: CH1 right
    if (nr51 & 0x20) l += ch2; // bit 5: CH2 left
    if (nr51 & 0x02) r += ch2; // bit 1: CH2 right
    if (nr51 & 0x40) l += ch3; // bit 6: CH3 left
    if (nr51 & 0x04) r += ch3; // bit 2: CH3 right
    if (nr51 & 0x80) l += ch4; // bit 7: CH4 left
    if (nr51 & 0x08) r += ch4; // bit 3: CH4 right

    const std::uint8_t nr50 = mmu_.mmio[NR50_OFF];
    const float vol_l = static_cast<float>(((nr50 >> 4) & 0x07) + 1) / 8.0f;
    const float vol_r = static_cast<float>((nr50 & 0x07) + 1) / 8.0f;

    l *= vol_l;
    r *= vol_r;

    // 4 channels each up to ±1 → sum up to ±4 after the master gain. /4 here
    // reserves headroom so the eventual full mix in Phase 4/5 doesn't clip.
    // With only CH1+CH2 active the peak is ±0.5 — noticeably quieter than
    // Phase 2's single-channel output, intentional tradeoff for consistency.
    l *= 0.25f;
    r *= 0.25f;

    ring_.push(l, r);
}
