#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"

#include <map>

namespace esphome {
namespace ld2410s {

#define CHECK_BIT(var, pos) (((var) >> (pos)) & 1)

// Commands
static const uint8_t CMD_SET_REPORT_MODE = 0x7A;
static const uint8_t CMD_GET_FIRMWARE_VER = 0x00;
static const uint8_t CMD_ENABLE_CONF = 0xFF;
static const uint8_t CMD_DISABLE_CONF = 0xFE;
static const uint8_t CMD_SET_SN = 0x10;
static const uint8_t CMD_GET_SN = 0x11;
static const uint8_t CMD_SET_COMMON_PARAMS = 0x70;
static const uint8_t CMD_GET_COMMON_PARAMS = 0x71;
static const uint8_t CMD_SET_TRIGGER_GATE_THRESHOLD = 0x72;
static const uint8_t CMD_GET_TRIGGER_GATE_THRESHOLD = 0x73;
static const uint8_t CMD_SET_STILL_GATE_THRESHOLD = 0x76;
static const uint8_t CMD_GET_STILL_GATE_THRESHOLD = 0x77;


// Commands values
static const uint8_t CMD_MAX_MOVE_VALUE = 0x0000;
static const uint8_t CMD_MAX_STILL_VALUE = 0x0001;
static const uint8_t CMD_DURATION_VALUE = 0x0002;
// Command Header & Footer
static const uint8_t CMD_FRAME_HEADER[4] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t CMD_FRAME_END[4] = {0x04, 0x03, 0x02, 0x01};
// Data Header & Footer
static const uint8_t DATA_FRAME_HEADER[4] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_FRAME_END[4] = {0xF8, 0xF7, 0xF6, 0xF5};

enum PeriodicDataStructure : uint8_t {
  TARGET_STATES = 2,
  MOVING_TARGET_LOW = 3,
  MOVING_TARGET_HIGH = 4,
};
enum PeriodicDataValue : uint8_t { HEAD = 0XAA, END = 0x55, CHECK = 0x00 };

enum AckDataStructure : uint8_t { COMMAND = 6, COMMAND_STATUS = 7 };

//  char cmd[2] = {enable ? 0xFF : 0xFE, 0x00};
class LD2410SComponent : public Component, public uart::UARTDevice {
#ifdef USE_SENSOR
  SUB_SENSOR(moving_target_distance)
  SUB_SENSOR(still_target_distance)
  SUB_SENSOR(moving_target_energy)
  SUB_SENSOR(still_target_energy)
  SUB_SENSOR(detection_distance)
#endif
#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(target)
  SUB_BINARY_SENSOR(moving_target)
  SUB_BINARY_SENSOR(still_target)
#endif
#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(version)
  SUB_TEXT_SENSOR(sn)
  SUB_TEXT_SENSOR(report_mode)
  SUB_TEXT_SENSOR(detecion_speed)
#endif
#ifdef USE_BUTTON
  SUB_BUTTON(query)
#endif
#ifdef USE_NUMBER
  SUB_NUMBER(max_distance)
  SUB_NUMBER(min_distance)
  SUB_NUMBER(timeout)
  SUB_NUMBER(state_report_freq)
  SUB_NUMBER(distance_report_freq)
#endif

 public:
  LD2410SComponent();
  void setup() override;
  void dump_config() override;
  void loop() override;
#ifdef USE_NUMBER
  void set_gate_still_threshold_number(int gate, number::Number *n);
  void set_gate_move_threshold_number(int gate, number::Number *n);
  void set_max_distances_timeout();
  void set_gate_threshold(uint8_t gate);
#endif
#ifdef USE_SENSOR
  void set_gate_move_sensor(int gate, sensor::Sensor *s);
  void set_gate_still_sensor(int gate, sensor::Sensor *s);
#endif
  void set_throttle(uint16_t value) { this->throttle_ = value; };
  void read_all_info();
  void set_distance_resolution(const std::string &state);

 protected:
  int two_byte_to_int_(char firstbyte, char secondbyte) { return (int16_t) (secondbyte << 8) + firstbyte; }
  void send_command_(uint16_t command_str, const uint8_t *command_value, uint8_t command_value_len);
  void set_config_mode_(bool enable);
  void handle_periodic_data_(uint8_t *buffer, int len);
  bool handle_ack_data_(uint8_t *buffer, int len);
  void readline_(int readch, uint8_t *buffer, int len);
  void query_parameters_();
  void get_version_();

  int32_t last_periodic_millis_ = millis();
  int32_t last_engineering_mode_change_millis_ = millis();
  uint16_t throttle_;
  std::string version_;
  std::string sn_;
#ifdef USE_NUMBER
  std::vector<number::Number *> gate_still_threshold_numbers_ = std::vector<number::Number *>(9);
  std::vector<number::Number *> gate_move_threshold_numbers_ = std::vector<number::Number *>(9);
#endif
#ifdef USE_SENSOR
  std::vector<sensor::Sensor *> gate_still_sensors_ = std::vector<sensor::Sensor *>(9);
  std::vector<sensor::Sensor *> gate_move_sensors_ = std::vector<sensor::Sensor *>(9);
#endif
};

}  // namespace ld2410s
}  // namespace esphome
