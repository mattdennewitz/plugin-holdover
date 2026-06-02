#pragma once
#include <atomic>

namespace holdover {

// Lock-free meter values written by the audio thread, read by the UI timer.
class MeterState {
public:
    void setSaturation(float v) noexcept { sat_.store(v, std::memory_order_relaxed); }
    void setGainReductionDb(float v) noexcept { gr_.store(v, std::memory_order_relaxed); }
    float saturation() const noexcept { return sat_.load(std::memory_order_relaxed); }
    float gainReductionDb() const noexcept { return gr_.load(std::memory_order_relaxed); }
private:
    std::atomic<float> sat_ { 0.0f };
    std::atomic<float> gr_  { 0.0f };
};

} // namespace holdover
