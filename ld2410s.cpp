#include "ld2410s.h"

#include <utility>
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#define highbyte(val) (uint8_t)((val) >> 8)
#define lowbyte(val) (uint8_t)((val) &0xff)

namespace esphome {
namespace ld2410s {

static const char *const TAG = "ld2410s";

static uint16_t two_byte_to_uint16(uint8_t firstbyte, uint8_t secondbyte) { 
  return (uint16_t) (secondbyte << 8) + firstbyte;
}

LD2410SComponent::LD2410SComponent() {}

void LD2410SComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2410S:");
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "TargetBinarySensor", this->target_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "MovingTargetBinarySensor", this->moving_target_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "StillTargetBinarySensor", this->still_target_binary_sensor_);
#endif
#ifdef USE_BUTTON
  LOG_BUTTON("  ", "QueryButton", this->query_button_);
#endif
#ifdef USE_SENSOR
  LOG_SENSOR("  ", "MovingTargetDistanceSensor", this->moving_target_distance_sensor_);
#endif
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR("  ", "VersionTextSensor", this->version_text_sensor_);
#endif
#ifdef USE_NUMBER
  LOG_NUMBER("  ", "TimeoutNumber", this->timeout_number_);
#endif
  // this->read_all_info();
  ESP_LOGCONFIG(TAG, "  Firmware Version : %s", const_cast<char *>(this->version_.c_str()));
}

void LD2410SComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LD2410...");
  this->read_all_info();
  ESP_LOGCONFIG(TAG, "Firmware Version : %s", const_cast<char *>(this->version_.c_str()));
  ESP_LOGCONFIG(TAG, "LD2410 setup complete.");
}

void LD2410SComponent::read_all_info() {
  this->set_config_mode_(true);
  this->get_version_();
  this->set_config_mode_(false);
}

void LD2410SComponent::loop() {
  const int max_line_length = 80;
  static uint8_t buffer[max_line_length];

  while (available()) {
    this->readline_(read(), buffer, max_line_length);
  }
}

void LD2410SComponent::send_command_(uint16_t command, const uint8_t *command_value, uint8_t command_value_len) {
  ESP_LOGV(TAG, "Sending COMMAND %02X", command);
  // frame start bytes
  this->write_array(CMD_FRAME_HEADER, 4);
  // length bytes
  int len = 2;
  if (command_value != nullptr)
    len += command_value_len;
  this->write_byte(lowbyte(len));
  this->write_byte(highbyte(len));

  // command
  this->write_byte(lowbyte(command));
  this->write_byte(highbyte(command));

  // command value bytes
  if (command_value != nullptr) {
    for (int i = 0; i < command_value_len; i++) {
      this->write_byte(command_value[i]);
    }
  }
  // frame end bytes
  this->write_array(CMD_FRAME_END, 4);
  // FIXME to remove
  delay(50);  // NOLINT
}

void LD2410SComponent::handle_periodic_data_(uint8_t *buffer, int len) {
  /* check packet len */
  if (len < 5)
    return;  // 1 frame start bytes + 2 length data bytes + 1 frame end bytes

  /*
    Reduce data update rate to prevent home assistant database size grow fast
  */
  int32_t current_millis = millis();
  if (current_millis - last_periodic_millis_ < this->throttle_)
    return;
  last_periodic_millis_ = current_millis;

#ifdef USE_BINARY_SENSOR
  /*
    Target states: 2th
    0x00 = No target
    0x01 = Moving targets
    0x02 = Still targets
    0x03 = Moving+Still targets
  */
  char target_state = buffer[TARGET_STATES];
  if (this->target_binary_sensor_ != nullptr) {
    this->target_binary_sensor_->publish_state(target_state != 0x00);
  }
  if (this->moving_target_binary_sensor_ != nullptr) {
    this->moving_target_binary_sensor_->publish_state(CHECK_BIT(target_state, 0));
  }
  if (this->still_target_binary_sensor_ != nullptr) {
    this->still_target_binary_sensor_->publish_state(CHECK_BIT(target_state, 1));
  }
#endif
  /*
    Moving target distance: 3~4th bytes
  */
#ifdef USE_SENSOR
  if (this->moving_target_distance_sensor_ != nullptr) {
    int new_moving_target_distance =
            this->two_byte_to_int_(buffer[MOVING_TARGET_LOW],
                                   buffer[MOVING_TARGET_HIGH]);
    if (this->moving_target_distance_sensor_->get_state() !=
        new_moving_target_distance)
      this->moving_target_distance_sensor_->publish_state(new_moving_target_distance);
  }
#endif
}

const char VERSION_FMT[] = "v%d.%d.%d";

static std::string format_version(uint8_t *buffer) {
  std::string::size_type version_size = 256;
  std::string version;
  version.resize(version_size + 1);
  version_size = std::snprintf(&version[0], version.size(), VERSION_FMT,
                               two_byte_to_uint16(buffer[9], buffer[8]),
                               two_byte_to_uint16(buffer[11], buffer[10]),
                               two_byte_to_uint16(buffer[13], buffer[12]));
  version.resize(version_size);
  return version;
}

static std::string format_sn(uint8_t *buffer) {
  uint16_t sn_len = two_byte_to_uint16(buffer[11], buffer[10]);
  std::string sn;
  sn.resize(sn_len);
  sn.copy((char *)buffer, sn_len, 12);
  return sn;
}

static void parse_common_parameters(uint8_t *buffer) {
}

bool LD2410SComponent::handle_ack_data_(uint8_t *buffer, int len) {
  /* check len */
  if (len < 10) {
    ESP_LOGE(TAG, "Error with last command : incorrect length");
    return true;
  }

  /* check 4 frame start bytes */
  if (buffer[0] != 0xFD || buffer[1] != 0xFC
      || buffer[2] != 0xFB || buffer[3] != 0xFA) {
    ESP_LOGE(TAG, "Error with last command : incorrect Header");
    return true;
  }

  /* check command status, 0x01 is success*/
  if (buffer[COMMAND_STATUS] != 0x01) {
    ESP_LOGE(TAG, "Error with last command : status != 0x01");
    return true;
  }

  ESP_LOGV(TAG, "Handling ACK DATA for COMMAND %02X", buffer[COMMAND]);

  /* handle command ack */
  switch (buffer[COMMAND]) {
    case CMD_ENABLE_CONF:
      ESP_LOGV(TAG, "Handled Enable conf command");
      break;

    case CMD_DISABLE_CONF:
      ESP_LOGV(TAG, "Handled Disabled conf command");
      break;

    case CMD_SET_REPORT_MODE:
      ESP_LOGV(TAG, "Handled report mode set command");
      break;

    case CMD_GET_FIRMWARE_VER:
      ESP_LOGV(TAG, "Handler get firmware vision command");
      this->version_ = format_version(buffer);
#ifdef USE_TEXT_SENSOR
      if (this->version_text_sensor_)
        this->version_text_sensor_->publish_state(this->version_);
#endif
      break;

    case CMD_SET_SN:
      ESP_LOGV(TAG, "Handler set sn command");
      break;

    case CMD_GET_SN:
      ESP_LOGV(TAG, "Handler get sn command");
      this->sn_ = format_sn(buffer);
#ifdef USE_TEXT_SENSOR
      if (this->sn_text_sensor_)
        this->sn_test_sensor_->publish_state(this->sn_);
#endif
      break;

    case CMD_SET_COMMON_PARAMS:
      ESP_LOGV(TAG, "Handler set comman parameters command");
      break;

    case CMD_GET_COMMON_PARAMS:
      ESP_LOGV(TAG, "Handler get comman parameters command");
      break;

    case CMD_SET_TRIGGER_GATE_THRESHOLD:
      ESP_LOGV(TAG, "Handler set trigger gate threshold");
      break;

    case CMD_GET_TRIGGER_GATE_THRESHOLD:
      ESP_LOGV(TAG, "Handler get tigger gate threshold");
      break;

    case CMD_SET_STILL_GATE_THRESHOLD:
      ESP_LOGV(TAG, "Handler set trigger gate threshold");
      break;

    case CMD_GET_STILL_GATE_THRESHOLD:
      ESP_LOGV(TAG, "Handler get tigger gate threshold");
      break;

    default:
      break;
  }

  return true;
}

void LD2410SComponent::readline_(int readch, uint8_t *buffer, int len) {
  static int pos = 0;

  if (readch >= 0) {
    if (pos < len - 1) {
      buffer[pos++] = readch;
      buffer[pos] = 0;
    } else {
      pos = 0;
    }
    if (pos >= 4) {
      if (buffer[pos - 4] == 0xF8 && buffer[pos - 3] == 0xF7 &&
          buffer[pos - 2] == 0xF6 && buffer[pos - 1] == 0xF5) {
        ESP_LOGV(TAG, "Will handle Periodic Data");
        this->handle_periodic_data_(buffer, pos);
        pos = 0;  // Reset position index ready for next time
      } else if (buffer[pos - 4] == 0x04 && buffer[pos - 3] == 0x03 &&
                 buffer[pos - 2] == 0x02 && buffer[pos - 1] == 0x01) {
        ESP_LOGV(TAG, "Will handle ACK Data");
        if (this->handle_ack_data_(buffer, pos)) {
          pos = 0;  // Reset position index ready for next time
        } else {
          ESP_LOGV(TAG, "ACK Data incomplete");
        }
      }
    }
  }
}

void LD2410SComponent::set_config_mode_(bool enable) {
  uint8_t cmd = enable ? CMD_ENABLE_CONF : CMD_DISABLE_CONF;
  uint8_t cmd_value[2] = {0x01, 0x00};
  this->send_command_(cmd, enable ? cmd_value : nullptr, 2);
}

void LD2410SComponent::query_parameters_() {
}

void LD2410SComponent::get_version_() {
}

#ifdef USE_NUMBER
void LD2410SComponent::set_max_distances_timeout() {
}

void LD2410SComponent::set_gate_threshold(uint8_t gate) {

}

#endif

}  // namespace ld2410s
}  // namespace esphome
