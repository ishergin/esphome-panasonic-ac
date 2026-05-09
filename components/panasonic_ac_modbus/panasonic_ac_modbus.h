#pragma once

#include "esphome/components/modbus/modbus.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace panasonic_ac_modbus {

// Modbus register addresses for Panasonic AC
static const uint16_t REG_POWER = 0x0000;           // Coil: 0=Off, 1=On
static const uint16_t REG_MODE = 0x0001;            // Holding: 0=Off, 1=Heat, 2=Cool, 3=Dry, 4=FAN, 5=Auto
static const uint16_t REG_TARGET_TEMP = 0x0002;     // Holding: 16-30 (Celsius), 0.5 step
static const uint16_t REG_FAN_MODE = 0x0003;        // Holding: 0=Auto, 1=1, 2=2, 3=3, 4=4, 5=5
static const uint16_t REG_SWING_VERTICAL = 0x0004;  // Holding: 0=Off, 1=Swing, 2=Up, 3=UpCenter, 4=Center, 5=DownCenter, 6=Down
static const uint16_t REG_SWING_HORIZONTAL = 0x0005;// Holding: 0=Off, 1=Auto, 2=Left, 3=LeftCenter, 4=Center, 5=RightCenter, 6=Right
static const uint16_t REG_ECO = 0x0006;             // Coil: 0=Off, 1=On
static const uint16_t REG_ECONAVI = 0x0007;         // Coil: 0=Off, 1=On
static const uint16_t REG_MILD_DRY = 0x0008;        // Coil: 0=Off, 1=On
static const uint16_t REG_NANOEX = 0x0009;          // Coil: 0=Off, 1=On
static const uint16_t REG_PRESET = 0x000A;          // Holding: 0=Normal, 1=Powerful, 2=Quiet

// Input registers for sensors (read-only)
static const uint16_t REG_CURRENT_TEMP = 0x0010;    // Input: Current room temperature (x10)
static const uint16_t REG_OUTSIDE_TEMP = 0x0011;    // Input: Outside temperature (x10)
static const uint16_t REG_POWER_CONSUMPTION = 0x0012; // Input: Power consumption in Watts
static const uint16_t REG_STATUS = 0x0013;          // Input: Status bits

// System registers
static const uint16_t REG_DEVICE_ADDRESS = 0x0070;  // Holding: Modbus device address (1-247)
static const uint16_t REG_FIRMWARE_VERSION = 0x0071; // Input: Firmware version (e.g., 250 = v2.5.0)
static const uint16_t REG_DEVICE_ID = 0x0072;       // Input: Device ID (0xAC01 = Panasonic AC)

// Status bits
static const uint16_t STATUS_POWER_ON = 0x0001;
static const uint16_t STATUS_COMPRESSOR_ON = 0x0002;
static const uint16_t STATUS_DEFROST = 0x0004;

// Device ID
static const uint16_t DEVICE_ID = 0xAC01;

class PanasonicACModbus : public Component, public modbus::ModbusDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Setters for linking to main AC component
  void set_ac_power(bool state);
  void set_ac_mode(uint8_t mode);
  void set_ac_target_temp(float temp);
  void set_ac_fan_mode(uint8_t mode);
  void set_ac_swing_vertical(uint8_t mode);
  void set_ac_swing_horizontal(uint8_t mode);
  void set_ac_eco(bool state);
  void set_ac_econavi(bool state);
  void set_ac_mild_dry(bool state);
  void set_ac_nanoex(bool state);
  void set_ac_preset(uint8_t preset);

  // Update sensor values from main AC
  void update_current_temp(float temp);
  void update_outside_temp(float temp);
  void update_power_consumption(uint16_t watts);
  void update_status(uint16_t status);
  void update_ac_state(bool power, uint8_t mode, float target_temp, uint8_t fan_mode,
                       uint8_t swing_v, uint8_t swing_h, bool eco, bool econavi, bool mild_dry, bool nanoex,
                       uint8_t preset);

  // Public method to change address from WebUI/API
  void set_address(uint8_t address);
  void set_state_change_callback(std::function<void(uint16_t)> callback) { this->on_state_change_ = std::move(callback); }

  bool get_ac_power() const { return this->ac_power_; }
  uint8_t get_ac_mode() const { return this->ac_mode_; }
  float get_ac_target_temp() const { return this->ac_target_temp_; }
  uint8_t get_ac_fan_mode() const { return this->ac_fan_mode_; }
  uint8_t get_ac_swing_vertical() const { return this->ac_swing_v_; }
  uint8_t get_ac_swing_horizontal() const { return this->ac_swing_h_; }
  bool get_ac_eco() const { return this->ac_eco_; }
  bool get_ac_econavi() const { return this->ac_econavi_; }
  bool get_ac_mild_dry() const { return this->ac_mild_dry_; }
  bool get_ac_nanoex() const { return this->ac_nanoex_; }
  uint8_t get_ac_preset() const { return this->ac_preset_; }

 protected:
  void on_modbus_data(const std::vector<uint8_t> &data) override;
  void on_modbus_read_registers(uint8_t function_code, uint16_t start_address,
                                uint16_t number_of_registers) override;
  void on_modbus_write_registers(uint8_t function_code, const std::vector<uint8_t> &data) override;
  void handle_read_coils(uint16_t address, uint16_t quantity);
  void handle_read_holding_registers(uint16_t address, uint16_t quantity);
  void handle_read_input_registers(uint16_t address, uint16_t quantity);
  void handle_write_coil(uint16_t address, bool state);
  void handle_write_register(uint16_t address, uint16_t value);
  void send_exception(uint8_t function_code, uint8_t exception_code);
  void send_read_response(uint8_t function_code, const std::vector<uint8_t> &data);

  // Current state mirrored from AC
  bool ac_power_ = false;
  uint8_t ac_mode_ = 0;        // 0=Off, 1=Heat, 2=Cool, 3=Dry, 4=FAN, 5=Auto
  float ac_target_temp_ = 24.0;
  uint8_t ac_fan_mode_ = 0;    // 0=Auto, 1-5=Speed
  uint8_t ac_swing_v_ = 0;     // 0=Off, 1=Swing, 2-6=Positions
  uint8_t ac_swing_h_ = 0;     // 0=Off, 1=Auto, 2-6=Positions
  bool ac_eco_ = false;
  bool ac_econavi_ = false;
  bool ac_mild_dry_ = false;
  bool ac_nanoex_ = false;
  uint8_t ac_preset_ = 0;      // 0=Normal, 1=Powerful, 2=Quiet

  // Sensor values
  float current_temp_ = 0.0;
  float outside_temp_ = 0.0;
  uint16_t power_consumption_ = 0;
  uint16_t status_ = 0;

  // Callback for sending commands to AC (will be set by main component)
  std::function<void(uint16_t)> on_state_change_;

  // Preferences for saving device address
  ESPPreferenceObject pref_;
  bool address_changed_ = false;
  uint8_t new_address_ = 0;

  void set_device_address(uint8_t address);
  void save_device_address(uint8_t address);
  void load_device_address();
};

}  // namespace panasonic_ac_modbus
}  // namespace esphome
