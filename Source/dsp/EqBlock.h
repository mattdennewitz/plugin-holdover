#pragma once

namespace holdover {

// RBJ biquad cascade: low shelf -> presence bell (proportional Q) -> high shelf.
class EqBlock {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setBass(float gainDb, float freqHz) noexcept;       // low shelf, +/-18 dB
    void setPresence(float gainDb, float freqHz) noexcept;   // bell, +/-12 dB, prop-Q
    void setTreble(float gainDb, float freqHz) noexcept;     // high shelf, +/-15 dB

    float processSample(float x) noexcept;

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;                 // transposed direct form II
        void reset() noexcept { z1 = z2 = 0.0f; }
        float process(float x) noexcept {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    static Biquad lowShelf(float gainDb, float f, double sr) noexcept;
    static Biquad highShelf(float gainDb, float f, double sr) noexcept;
    static Biquad peaking(float gainDb, float f, float q, double sr) noexcept;
    static float presenceQ(float gainDb) noexcept;

    double sr_ = 48000.0;
    Biquad low_, mid_, high_;
};

} // namespace holdover
