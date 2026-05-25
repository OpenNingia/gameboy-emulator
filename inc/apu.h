#pragma once
#ifndef _H_APU_H_
#    define _H_APU_H_

#    include <cstdint>

#    include <audio_ring_buffer.hpp>
#    include <mmu.h>

namespace gbemu {
    struct apu {
        // Output sample rate. Application opens SDL_AudioDevice with this so
        // the audio thread's drain rate matches the cadence at which step()
        // pushes samples into the ring buffer.
        static constexpr int SAMPLE_RATE = 48000;

        // Post-mix high-pass filter modes. Game Boy hardware has an analog
        // DC-blocking filter on the audio output; without it, square waves
        // with non-50% duty cycles produce a "thump" at note edges because
        // their DC component shifts the baseline. accurate emulates DMG/CGB
        // hardware (~60 Hz / ~120 Hz cutoff). preserve uses a much lower
        // cutoff (~5 Hz) so only the DC drift is removed and the perceived
        // bass content stays intact — a popular "gameplay" preset.
        enum class highpass_mode : std::uint8_t { off, accurate, preserve };

        explicit apu(mmu& mmu);

        // Advance the APU by `cycles` T-cycles. Drives the frame sequencer
        // (length / envelope / sweep), the per-channel frequency timers, and
        // the Bresenham sample emitter that pushes one stereo frame every
        // CPU_HZ / SAMPLE_RATE cycles. The push is blocking — when the ring
        // is full it yields until the audio thread drains, which is the
        // backpressure that paces the emulator to real time.
        void step(std::uint32_t cycles);

        // The ring buffer the SDL audio callback drains. Owned by the APU so
        // its lifetime tracks the rest of the audio pipeline.
        audio_ring_buffer& output() { return ring_; }

        // Toggled on by Application after SDL_OpenAudioDevice succeeds. While
        // off, step() advances internal counters but does not push, so
        // headless mode (no audio device, nobody draining the ring) doesn't
        // deadlock on the first full buffer.
        void enable_output(bool on) { output_enabled_ = on; }

        // Output-side controls. Independent of emulation state: the channels
        // keep advancing so cycle accounting stays accurate; only the values
        // pushed into the ring buffer are gated. `muted` is a hard zero
        // (separate from `set_volume(0)` so the user's chosen volume is
        // preserved across mute toggles). `master_gain` is the squared slider
        // value in [0..1], applied as a plain multiplier before the ring
        // push. `highpass` selects the post-mix DC-blocking filter — see
        // highpass_mode above. Changing modes does not zero the IIR memory
        // so the natural decay of the filter absorbs the alpha switch
        // without producing a pop.
        void set_muted(bool m) { muted_ = m; }
        bool is_muted() const { return muted_; }
        void set_master_gain(float g) { master_gain_ = g; }
        float master_gain() const { return master_gain_; }
        void set_highpass_mode(highpass_mode m);
        highpass_mode get_highpass_mode() const { return highpass_mode_; }

        // Default-construct every channel and clear the sample / frame
        // sequencer accumulators.  The audio ring buffer is left alone — the
        // SDL audio thread drains it from the other side and clearing it
        // would race; the next emit_sample() pushes silence anyway.  MMIO
        // write handlers (registered in the ctor) survive a reset because
        // they live in the MMU, not the APU.
        void reset();

    private:
        // Square wave with length counter + volume envelope. Used directly
        // by CH2; CH1 pairs an instance of this with a sweep_unit that
        // periodically rewrites freq_raw.
        struct square_channel {
            // Configuration (mirrors NRx1..NRx4 fields)
            std::uint8_t duty{0};
            std::uint16_t freq_raw{0};
            std::uint8_t length{0};
            bool length_enabled{false};
            std::uint8_t env_initial_vol{0};
            bool env_increase{false};
            std::uint8_t env_period{0};

            // Runtime state
            bool channel_enabled{false};
            bool dac_enabled{false};
            std::int32_t freq_timer{4};
            std::uint8_t duty_pos{0};
            std::uint8_t volume{0};
            std::uint8_t env_timer{0};

            // Register writes — shared between CH1 (NR11..NR14) and CH2
            // (NR21..NR24). load_nrx4 returns whether the trigger bit was
            // set; the caller decides when to invoke trigger() so CH1 can
            // also kick off its sweep unit at the right moment.
            void load_nrx1(std::uint8_t v);
            void load_nrx2(std::uint8_t v);
            void load_nrx3(std::uint8_t v);
            // next_step_clocks_length carries the FS phase needed for the
            // "extra length clock" quirk: writing NRx4 with length_enable
            // rising from 0->1 while the next FS step would NOT clock length
            // clocks length once immediately.
            bool load_nrx4(std::uint8_t v, bool next_step_clocks_length);

            void tick_frequency(std::uint32_t cycles);
            void tick_length();
            void tick_envelope();
            void trigger(bool next_step_clocks_length);
            float sample() const;
        };

        // Wave channel — plays back 32 4-bit samples held in wave RAM
        // ($FF30-$FF3F). No envelope; output level is a fixed right-shift of
        // the fetched nibble.
        struct wave_channel {
            // Configuration (mirrors NR30..NR34 + NR32)
            std::uint16_t length{0}; // 8-bit width, 256-step counter
            bool length_enabled{false};
            std::uint8_t output_level{0}; // 0=mute, 1=100%, 2=50%, 3=25%
            std::uint16_t freq_raw{0};

            // Runtime state
            bool channel_enabled{false};
            bool dac_enabled{false}; // NR30 bit 7 — explicit DAC bit
            std::int32_t freq_timer{8};
            std::uint8_t wave_pos{0};      // 0..31 — index of NEXT nibble to fetch
            std::uint8_t sample_buffer{0}; // last fetched 4-bit nibble
            // Byte just fetched (= wave_ram[(wave_pos - 1) >> 1] in steady
            // state).  Tracked separately because the CGB read/write redirect
            // for $FF30-$FF3F needs the byte the channel is currently
            // accessing, which after the increment is at (wave_pos - 1) >> 1
            // rather than wave_pos >> 1.  Pre-first-fetch this holds residual
            // state (defaults to 0; cleared on CGB trigger alongside
            // sample_buffer).
            std::uint8_t current_sample_byte{0};
            std::uint8_t current_sample_byte_idx{0};

            void load_nr30(std::uint8_t v);
            void load_nr31(std::uint8_t v);
            void load_nr32(std::uint8_t v);
            void load_nr33(std::uint8_t v);
            bool load_nr34(std::uint8_t v, bool next_step_clocks_length);

            // Wave RAM is passed in by the apu (the MMU exposes a span over
            // its $FF30-$FF3F backing store via mmu::wave_ram). Keeping it
            // external avoids a back-pointer.
            void tick_frequency(std::uint32_t cycles, const std::uint8_t* wave_ram);
            void tick_length();
            void trigger(bool next_step_clocks_length, bool cgb_mode);
            float sample() const;
        };

        // Noise channel — LFSR-driven pseudo-random output with length
        // counter and volume envelope. Shares no state with square_channel:
        // the duty/freq stuff is replaced by an LFSR and a polynomial-style
        // divider.
        struct noise_channel {
            // Configuration (mirrors NR41..NR44 fields)
            std::uint8_t length{0};
            bool length_enabled{false};
            std::uint8_t env_initial_vol{0};
            bool env_increase{false};
            std::uint8_t env_period{0};
            std::uint8_t shift_clock{0};  // NR43 bits 7-4 — period multiplier
            bool width_7bit{false};       // NR43 bit 3 — narrows the LFSR
            std::uint8_t divisor_code{0}; // NR43 bits 2-0 — base period

            // Runtime state
            bool channel_enabled{false};
            bool dac_enabled{false};
            std::int32_t freq_timer{8};
            std::uint16_t lfsr{0x7FFF};
            std::uint8_t volume{0};
            std::uint8_t env_timer{0};

            void load_nr41(std::uint8_t v);
            void load_nr42(std::uint8_t v);
            void load_nr43(std::uint8_t v);
            bool load_nr44(std::uint8_t v, bool next_step_clocks_length); // returns trigger bit

            void tick_frequency(std::uint32_t cycles);
            void tick_length();
            void tick_envelope();
            void trigger(bool next_step_clocks_length);
            float sample() const;
        };

        // CH1's sweep unit. Periodically recomputes the channel's frequency
        // by adding/subtracting a right-shifted copy of itself, and disables
        // the channel on overflow (>2047). Clocked at 128 Hz off the frame
        // sequencer's steps 2 and 6.
        struct sweep_unit {
            // Configuration (from NR10)
            std::uint8_t period{0};
            bool decrease{false};
            std::uint8_t shift{0};

            // Runtime state
            std::uint16_t shadow_freq{0};
            std::uint8_t timer{0};
            bool enabled{false};
            // DMG quirk: once a sweep calculation has been performed in
            // negate (subtract) mode, leaving negate mode by writing NR10
            // with bit 3 cleared immediately disables the channel. Set by
            // calc() whenever it runs with `decrease` true; reset by
            // trigger().
            bool negate_used{false};

            void load_nr10(std::uint8_t v);

            // Called from CH1's trigger event. Captures shadow_freq, reloads
            // the timer, sets `enabled`, and if shift>0 runs an immediate
            // overflow check. Returns true on overflow — caller should
            // disable the channel.
            bool trigger(std::uint16_t channel_freq);

            // 128 Hz tick. May write a new frequency back through
            // `freq_inout`. Returns true on overflow — caller should disable
            // the channel.
            bool tick(std::uint16_t& freq_inout);

        private:
            // Not const: may set negate_used as a side effect.
            bool calc(std::uint16_t& out);
        };

        mmu& mmu_;
        audio_ring_buffer ring_{};
        bool output_enabled_{false};

        // Output-side state (mute / volume / highpass). Defaults: not
        // muted, unity gain, accurate filter — same audible behavior as
        // before the menu was added. `hp_alpha_` is derived from
        // highpass_mode_ in recompute_hp_alpha and refreshed on reset;
        // the IIR memory (prev_in/prev_out) is also zeroed there so a
        // ROM swap or soft reset doesn't leak the prior session's DC
        // bias into the first samples of the new run.
        bool muted_{false};
        float master_gain_{1.0f};
        highpass_mode highpass_mode_{highpass_mode::accurate};
        float hp_alpha_{0.0f};
        float hp_prev_in_l_{0.0f};
        float hp_prev_in_r_{0.0f};
        float hp_prev_out_l_{0.0f};
        float hp_prev_out_r_{0.0f};

        void recompute_hp_alpha();

        // Bresenham sample-rate accumulator.
        std::uint64_t sample_acc_{0};
        // Frame sequencer: ticks at 512 Hz (every 8192 T-cycles).
        std::uint32_t frame_seq_acc_{0};
        std::uint8_t frame_seq_step_{0};

        square_channel ch1_sq_{};
        sweep_unit ch1_sweep_{};
        square_channel ch2_{};
        wave_channel ch3_{};
        noise_channel ch4_{};

        // NR52 bit 7: APU master power. While off, writes to NRxx (except
        // length loads on DMG, which we don't model separately) are ignored
        // and the channels are held disabled.
        bool powered_{true};

        // MMIO write handlers installed by the ctor.
        void on_nr10(std::uint8_t v);
        void on_nr11(std::uint8_t v);
        void on_nr12(std::uint8_t v);
        void on_nr13(std::uint8_t v);
        void on_nr14(std::uint8_t v);
        void on_nr21(std::uint8_t v);
        void on_nr22(std::uint8_t v);
        void on_nr23(std::uint8_t v);
        void on_nr24(std::uint8_t v);
        void on_nr30(std::uint8_t v);
        void on_nr31(std::uint8_t v);
        void on_nr32(std::uint8_t v);
        void on_nr33(std::uint8_t v);
        void on_nr34(std::uint8_t v);
        void on_nr41(std::uint8_t v);
        void on_nr42(std::uint8_t v);
        void on_nr43(std::uint8_t v);
        void on_nr44(std::uint8_t v);
        void on_nr50(std::uint8_t v);
        void on_nr51(std::uint8_t v);
        void on_nr52(std::uint8_t v);
        void on_div_write(std::uint8_t v);

        // Wipe channel + register state when NR52 bit 7 is cleared. Length
        // counters are preserved on DMG (the channels keep counting toward
        // disable even with the master off — Blargg test 8 exercises this).
        void power_off();

        // Refresh NR52's read-only channel-enabled bits so reads return the
        // live state of each channel.
        void refresh_nr52_status();

        // Stamp the NRxx read mask onto the just-stored mmio byte so reads
        // return `v | mask` (write-only / unused bits read as 1). `addr`
        // must be in $FF10-$FF25.
        void apply_read_mask(std::uint16_t addr, std::uint8_t v);

        void advance_frame_sequencer(std::uint32_t cycles);
        void do_frame_seq_step();
        void emit_sample();

        // True when the next FS step would clock the length counter (i.e.
        // the currently-pending step is one of 0/2/4/6). Used by NRx4
        // writes / triggers to decide whether the "extra length clock"
        // quirk should fire.
        bool next_step_clocks_length() const { return (frame_seq_step_ & 1) == 0; }
    };
} // namespace gbemu

#endif /* _H_APU_H_ */
