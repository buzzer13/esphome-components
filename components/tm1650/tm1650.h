#pragma once

#include "esphome/core/component.h"
// #include "esphome/core/defines.h"
// #include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/time/real_time_clock.h"

// #include <vector>

namespace esphome {
namespace tm1650 {

#define TM1650_I2C_DCTL_BASE 0x24
#define TM1650_I2C_DISP_BASE 0x34
#define TM1650_MAX_DIGITS    16
#define TM1650_MAX_SEGMENTS  8

class TM1650Display;

using tm1650_writer_t = std::function<void(TM1650Display &)>;

class TM1650Display : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_writer(tm1650_writer_t &&writer);
  void set_intensity(uint8_t intensity);
  void set_mode(uint8_t mode);
  void set_segment_map(const char *segment_map);
  void set_power(bool power);
  void set_length(uint8_t length);

  float get_setup_priority() const override;
  void setup() override;
  void update() override;
  void dump_config() override;

  uint8_t printf(uint8_t pos, const char *format, ...) __attribute__((format(printf, 3, 4)));
  uint8_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  uint8_t print(uint8_t pos, const char *str);
  uint8_t print(const char *str);

  uint8_t strftime(uint8_t pos, const char *format, ESPTime time) __attribute__((format(strftime, 3, 0)));
  uint8_t strftime(const char *format, ESPTime time) __attribute__((format(strftime, 2, 0)));

 protected:
  uint8_t intensity_;
  uint8_t mode_ = 0;
  bool power_ = 1;
  uint8_t length_;
  optional<tm1650_writer_t> writer_{};
  uint8_t buffer_[TM1650_MAX_DIGITS] = {0};
  uint8_t segment_map_[TM1650_MAX_SEGMENTS] = {0};

  enum ErrorCode { NONE = 0, COMMUNICATION_FAILED } error_code_{NONE};
};

}  // namespace tm1650
}  // namespace esphome
