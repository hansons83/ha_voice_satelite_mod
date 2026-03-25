#include "resampler_microphone.h"
#include "esphome/core/log.h"

namespace esphome {
namespace resampler_mic {

static const char *const TAG = "resampler_mic";

void ResamplerMicrophone::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Resampler Microphone (48kHz -> 16kHz)...");
  
  // 15-tap FIR Low-pass filter coefficients (Cutoff ~8kHz for 48kHz sampling)
  // Designed using windowed sinc (Hamming)
  float coeffs[TAPS] = {
    0.0031, 0.0000, -0.0138, -0.0299, 0.0000, 0.1133, 0.2616, 0.3315,
    0.2616, 0.1133, 0.0000, -0.0299, -0.0138, 0.0000, 0.0031
  };
  
  for (int i = 0; i < TAPS; i++) {
    filter_coeffs_[i] = coeffs[i];
    history_[i] = 0;
  }

  this->source_mic_->add_data_callback([this](const std::vector<uint8_t> &data) {
    this->on_data(data);
  });
  
  this->output_.reserve(256*2);
}

void ResamplerMicrophone::start() {
  if (this->source_mic_ == nullptr) return;
  
  this->state_ = microphone::STATE_RUNNING;
  
  if (++this->ref_count_ == 1) {
    ESP_LOGD(TAG, "Starting source microphone (ref_count=1)");
    this->source_mic_->start();
  } else {
    ESP_LOGD(TAG, "Resampler already started, incrementing ref_count to %d", this->ref_count_.load());
  }
}

void ResamplerMicrophone::stop() {
  if (this->source_mic_ == nullptr) return;

  if (this->ref_count_ > 0) {
    if (--this->ref_count_ == 0) {
      ESP_LOGD(TAG, "Stopping source microphone (ref_count=0)");
      this->source_mic_->stop();
	  this->state_ = microphone::STATE_STOPPED;
    } else {
      ESP_LOGD(TAG, "Decrementing ref_count to %d", this->ref_count_.load());
    }
  } else {
    ESP_LOGW(TAG, "Stop called more times than start!");
  }
}

void ResamplerMicrophone::on_data(const std::vector<uint8_t> &data) {
  size_t num_samples = data.size() / 2;
  const int16_t *samples = reinterpret_cast<const int16_t *>(data.data());
  
  output_.clear();

  for (size_t i = 0; i < num_samples; i++) {
    int16_t sample = samples[i];
    
    // Update FIR history
    history_[history_idx_] = sample;
    
    // Decimate 3:1
    if (input_count_ == 0) {
      float filtered = 0;
      for (int j = 0; j < TAPS; j++) {
        int idx = (history_idx_ - j + TAPS) % TAPS;
        filtered += history_[idx] * filter_coeffs_[j];
      }
      
      // Apply gain
      filtered *= this->gain_;
      
      // Clamp and push
      if (filtered > 32767) filtered = 32767;
      if (filtered < -32768) filtered = -32768;
      
      int16_t result = (int16_t)filtered;
      output_.push_back(result & 0xFF);
      output_.push_back((result >> 8) & 0xFF);
    }

    history_idx_ = (history_idx_ + 1) % TAPS;
    input_count_ = (input_count_ + 1) % 3;
  }

  if (!output_.empty()) {
    this->data_callbacks_.call(output_);
  }
}

void ResamplerMicrophone::dump_config() {
  ESP_LOGCONFIG(TAG, "Resampler Microphone:");
  ESP_LOGCONFIG(TAG, "  Input Rate: 48000 Hz");
  ESP_LOGCONFIG(TAG, "  Output Rate: 16000 Hz");
  ESP_LOGCONFIG(TAG, "  Filter Type: FIR (15-tap)");
  ESP_LOGCONFIG(TAG, "  Gain: %.2f", this->gain_);
}

}  // namespace resampler_mic
}  // namespace esphome
