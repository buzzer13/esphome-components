#include "tm1650.h"
#include "tm1650font.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tm1650 {

static const char *const TAG = "display.tm1650";

void  TM1650Display::set_writer(tm1650_writer_t &&writer) { this->writer_ = writer; }
void  TM1650Display::set_intensity(uint8_t intensity)     { this->intensity_ = intensity; }
void  TM1650Display::set_mode(uint8_t mode)               { this->mode_ = mode; }
void  TM1650Display::set_power(bool power)                { this->power_ = power; }
void  TM1650Display::set_length(uint8_t length)           { this->length_ = length; }
float TM1650Display::get_setup_priority() const           { return setup_priority::PROCESSOR; }

void TM1650Display::set_segment_map(const char *segment_map) {
  auto map_len = strlen(segment_map);

  if (map_len > TM1650_MAX_SEGMENTS) {
    map_len = TM1650_MAX_SEGMENTS;
  }

  for (uint8_t i = 0; i < map_len; ++i) {
    char seg_map = segment_map[map_len - 1 - i];

    // 7 <- PABCDEFG <- 0
    if (seg_map <= 'G') {
      this->segment_map_[i] = 1 << ('G' - seg_map);
    } else {
      this->segment_map_[i] = TM1650_DOT_SEGMENT;
    }
  }
}

void TM1650Display::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TM1650...");

  auto err = this->write(nullptr, 0);

  if (err != i2c::ERROR_OK) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }
}

void TM1650Display::update() {
  uint8_t settings = ((this->intensity_ & 7) << 4)
    | ((this->mode_ & 1) << 3)
    | ((this->power_ & (this->intensity_ ? 1 : 0)));

  for (uint8_t &i : this->buffer_) {
    i = 0;
  }

  if (this->writer_.has_value()) {
    (*this->writer_)(*this);
  }

  this->set_i2c_address(TM1650_I2C_DCTL_BASE);
  this->write(&settings, 1);

  for (uint8_t i = 0; i < this->length_; ++i) {
    this->set_i2c_address(TM1650_I2C_DISP_BASE + i);
    this->write(this->buffer_ + i, 1);
  }
}

void TM1650Display::dump_config() {
  ESP_LOGCONFIG(TAG, "TM1650:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Intensity: %d", this->intensity_);
  ESP_LOGCONFIG(TAG, "  Mode: %d", this->mode_);
  ESP_LOGCONFIG(TAG, "  Power: %d", this->power_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
  LOG_UPDATE_INTERVAL(this);

  if (this->error_code_ == COMMUNICATION_FAILED) {
    ESP_LOGE(TAG, "Communication with TM1650 failed!");
  }
}

uint8_t TM1650Display::print(uint8_t start_pos, const char *str) {
  uint8_t pos = start_pos;

  for (; *str != '\0'; str++) {
    uint8_t char_data = TM1650_UNKNOWN_CHAR;

    if (*str >= ' ' && *str <= '~') {
      char_data = progmem_read_byte(&TM1650_ASCII_TO_RAW[*str - ' ']);
    }

    if (char_data == TM1650_UNKNOWN_CHAR) {
      ESP_LOGW(TAG, "Encountered character '%c' with no TM1650 representation while translating string!", *str);
    }

    // Activate dot if it is next character
    if (*(str + 1) == '.') {
      char_data |= TM1650_DOT_SEGMENT;
      str++;
    }

    // Remap character segments
    uint8_t data = 0;

    for (auto s = 0; s < TM1650_MAX_SEGMENTS; ++s) {
      data |= (char_data & (1 << s)) ? this->segment_map_[s] : 0;
    }

    // Save to buffer
    if (pos >= this->length_) {
      ESP_LOGE(TAG, "String is too long for the display!");
      break;
    }

    this->buffer_[pos] = data;

    pos++;
  }

  return pos - start_pos;
}

uint8_t TM1650Display::print(const char *str) {
  return this->print(0, str);
}

uint8_t TM1650Display::printf(uint8_t pos, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}

uint8_t TM1650Display::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(buffer);
  return 0;
}

uint8_t TM1650Display::strftime(uint8_t pos, const char *format, time::ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}

uint8_t TM1650Display::strftime(const char *format, time::ESPTime time) {
  return this->strftime(0, format, time);
}

}  // namespace tm1650
}  // namespace esphome
