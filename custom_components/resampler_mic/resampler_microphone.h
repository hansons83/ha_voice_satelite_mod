#pragma once

#include "esphome/core/component.h"
#include "esphome/components/microphone/microphone.h"
#include <vector>
#include <atomic>

namespace esphome {
namespace resampler_mic {

class ResamplerMicrophone : public microphone::Microphone, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void start() override;
  void stop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_source_mic(microphone::Microphone *source_mic) { source_mic_ = source_mic; }
  void set_gain(float gain) { gain_ = gain; }

 protected:
  void on_data(const std::vector<uint8_t> &data);

  microphone::Microphone *source_mic_{nullptr};
  std::atomic<int> ref_count_{0};
  float gain_{1.0f};
  std::vector<uint8_t> output_;
  
  // FIR Filter state
  static const int TAPS = 15;
  float filter_coeffs_[TAPS];
  int16_t history_[TAPS];
  int history_idx_{0};
  
  // Resampling state
  int input_count_{0};
};

}  // namespace resampler_mic
}  // namespace esphome
