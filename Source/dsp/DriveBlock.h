#pragma once

namespace holdover {

// Drive cascade: pre-gain -> CURVE pre-emphasis -> MAS -> SAT -> HEX ->
// CURVE de-emphasis -> output compensation -> (conditional) DC blocker.
// CURVE pre/de are exact first-order inverses, so with shapers off the response
// is flat. DRIVE always active; stages individually optional.
class DriveBlock {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setDrive(float pos0to10) noexcept;
    void setMas(int mode) noexcept;          // 0 off, 1 = 2nd, 2 = 3rd
    void setSat(bool on) noexcept;
    void setHex(bool on) noexcept;
    void setCurve(bool on) noexcept;
    void setCharacter(float amount01, float chSign) noexcept; // Class-A bias; chSign = +1/-1 per channel

    float processSample(float x) noexcept;

private:
    struct Shelf {
        float b0 = 1, b1 = 0, a1 = 0, x1 = 0, y1 = 0;
        void reset() noexcept { x1 = y1 = 0.0f; }
        float process(float x) noexcept {
            const float y = b0 * x + b1 * x1 - a1 * y1;
            x1 = x; y1 = y; return y;
        }
    };
    static Shelf makeEmphasis(double sr, float shelfDb, float cornerHz) noexcept;
    static Shelf invert(const Shelf& s) noexcept;

    float masStage(float x) const noexcept;
    static float satStage(float x) noexcept;
    static float hexStage(float x) noexcept;

    double sr_ = 48000.0;
    float preGain_ = 1.0f, outComp_ = 1.0f;
    int mas_ = 0; bool sat_ = false, hex_ = false, curve_ = true;
    float character_ = 0.0f, bias_ = 0.0f; // Class-A asymmetry; bias_ == 0 when character_ == 0
    Shelf pre_, de_;
    float dcX1_ = 0.0f, dcY1_ = 0.0f;
};

} // namespace holdover
