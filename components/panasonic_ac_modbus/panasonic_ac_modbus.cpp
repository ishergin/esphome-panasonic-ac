#include "panasonic_ac_modbus.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panasonic_ac_modbus {

static const char *const TAG = "panasonic_ac.modbus";

// Modbus function codes
static const uint8_t FUNC_READ_COILS = 0x01;
static const uint8_t FUNC_READ_HOLDING_REGISTERS = 0x03;
static const uint8_t FUNC_READ_INPUT_REGISTERS = 0x04;
static const uint8_t FUNC_WRITE_COIL = 0x05;
static const uint8_t FUNC_WRITE_REGISTER = 0x06;
static const uint8_t FUNC_WRITE_MULTIPLE_COILS = 0x0F;
static const uint8_t FUNC_WRITE_MULTIPLE_REGISTERS = 0x10;

// Exception codes
static const uint8_t EXC_ILLEGAL_FUNCTION = 0x01;
static const uint8_t EXC_ILLEGAL_DATA_ADDRESS = 0x02;
static const uint8_t EXC_ILLEGAL_DATA_VALUE = 0x03;

void PanasonicACModbus::setup() {
  ESP_LOGI(TAG, "Panasonic AC Modbus gateway started");
  this->load_device_address();
}

void PanasonicACModbus::loop() {
  // Check if address needs to be changed (done in loop to avoid issues during callback)
  if (this->address_changed_) {
    this->address_changed_ = false;
    if (this->new_address_ >= 1 && this->new_address_ <= 247) {
      ESP_LOGI(TAG, "Changing Modbus address from %d to %d", this->address_, this->new_address_);
      this->address_ = this->new_address_;
      this->save_device_address(this->new_address_);
    }
  }
}

void PanasonicACModbus::dump_config() {
  ESP_LOGCONFIG(TAG, "Panasonic AC Modbus:");
  ESP_LOGCONFIG(TAG, "  Device address: %d", this->address_);
  ESP_LOGCONFIG(TAG, "  Current temp: %.1f°C", this->current_temp_);
  ESP_LOGCONFIG(TAG, "  Outside temp: %.1f°C", this->outside_temp_);
  ESP_LOGCONFIG(TAG, "  Power consumption: %dW", this->power_consumption_);
}

void PanasonicACModbus::on_modbus_data(const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Ignoring generic Modbus callback with %u bytes", (unsigned int) data.size());
}

void PanasonicACModbus::on_modbus_read_registers(uint8_t function_code, uint16_t start_address,
                                                 uint16_t number_of_registers) {
  ESP_LOGV(TAG, "Server read request: function=0x%02X start=%u count=%u", function_code, start_address,
           number_of_registers);

  switch (function_code) {
    case FUNC_READ_HOLDING_REGISTERS:
      this->handle_read_holding_registers(start_address, number_of_registers);
      break;
    case FUNC_READ_INPUT_REGISTERS:
      this->handle_read_input_registers(start_address, number_of_registers);
      break;
    case FUNC_READ_COILS:
      this->handle_read_coils(start_address, number_of_registers);
      break;
    default:
      ESP_LOGW(TAG, "Unsupported read function code: 0x%02X", function_code);
      this->send_exception(function_code, EXC_ILLEGAL_FUNCTION);
      break;
  }
}

void PanasonicACModbus::on_modbus_write_registers(uint8_t function_code, const std::vector<uint8_t> &data) {
  ESP_LOGV(TAG, "Server write request: function=0x%02X payload=%u bytes", function_code, (unsigned int) data.size());

  if (function_code == FUNC_WRITE_REGISTER) {
    if (data.size() != 4) {
      ESP_LOGW(TAG, "Invalid write register payload size: %u", (unsigned int) data.size());
      this->send_exception(function_code, EXC_ILLEGAL_DATA_VALUE);
      return;
    }

    uint16_t address = (uint16_t(data[0]) << 8) | data[1];
    uint16_t value = (uint16_t(data[2]) << 8) | data[3];
    this->handle_write_register(address, value);
    return;
  }

  ESP_LOGW(TAG, "Unsupported write function code: 0x%02X", function_code);
  this->send_exception(function_code, EXC_ILLEGAL_FUNCTION);
}

void PanasonicACModbus::handle_read_coils(uint16_t address, uint16_t quantity) {
  if (quantity == 0 || quantity > 2000) {
    send_exception(FUNC_READ_COILS, EXC_ILLEGAL_DATA_VALUE);
    return;
  }

  uint8_t byte_count = (quantity + 7) / 8;
  std::vector<uint8_t> response;
  response.reserve(1 + byte_count);
  response.push_back(byte_count);

  for (int i = 0; i < byte_count; i++) {
    uint8_t byte_value = 0;
    for (int j = 0; j < 8 && (i * 8 + j) < quantity; j++) {
      uint16_t coil_addr = address + i * 8 + j;
      bool value = false;
      switch (coil_addr) {
        case REG_POWER: value = this->ac_power_; break;
        case REG_ECO: value = this->ac_eco_; break;
        case REG_ECONAVI: value = this->ac_econavi_; break;
        case REG_MILD_DRY: value = this->ac_mild_dry_; break;
        case REG_NANOEX: value = this->ac_nanoex_; break;
        default: break;
      }
      if (value) byte_value |= (1 << j);
    }
    response.push_back(byte_value);
  }

  send_read_response(FUNC_READ_COILS, response);
  ESP_LOGD(TAG, "Read coils from %d, quantity %d", address, quantity);
}

void PanasonicACModbus::handle_read_holding_registers(uint16_t address, uint16_t quantity) {
  if (quantity == 0 || quantity > 125) {
    send_exception(FUNC_READ_HOLDING_REGISTERS, EXC_ILLEGAL_DATA_VALUE);
    return;
  }

  std::vector<uint8_t> response;
  response.reserve(1 + quantity * 2);
  response.push_back(quantity * 2);

  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t reg_addr = address + i;
    uint16_t value = 0;

    switch (reg_addr) {
      case REG_POWER: value = this->ac_power_ ? 1 : 0; break;
      case REG_MODE: value = this->ac_mode_; break;
      case REG_TARGET_TEMP: value = (uint16_t)(this->ac_target_temp_ * 10); break;
      case REG_FAN_MODE: value = this->ac_fan_mode_; break;
      case REG_SWING_VERTICAL: value = this->ac_swing_v_; break;
      case REG_SWING_HORIZONTAL: value = this->ac_swing_h_; break;
      case REG_ECO: value = this->ac_eco_ ? 1 : 0; break;
      case REG_ECONAVI: value = this->ac_econavi_ ? 1 : 0; break;
      case REG_MILD_DRY: value = this->ac_mild_dry_ ? 1 : 0; break;
      case REG_NANOEX: value = this->ac_nanoex_ ? 1 : 0; break;
      case REG_PRESET: value = this->ac_preset_; break;
      case REG_DEVICE_ADDRESS: value = this->address_; break;
      default: break;
    }

    response.push_back((value >> 8) & 0xFF);
    response.push_back(value & 0xFF);
  }

  send_read_response(FUNC_READ_HOLDING_REGISTERS, response);
  ESP_LOGD(TAG, "Read holding registers from %d, quantity %d", address, quantity);
}

void PanasonicACModbus::handle_read_input_registers(uint16_t address, uint16_t quantity) {
  if (quantity == 0 || quantity > 125) {
    send_exception(FUNC_READ_INPUT_REGISTERS, EXC_ILLEGAL_DATA_VALUE);
    return;
  }

  std::vector<uint8_t> response;
  response.reserve(1 + quantity * 2);
  response.push_back(quantity * 2);

  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t reg_addr = address + i;
    uint16_t value = 0;

    switch (reg_addr) {
      case REG_CURRENT_TEMP: value = (int16_t)(this->current_temp_ * 10); break;
      case REG_OUTSIDE_TEMP: value = (int16_t)(this->outside_temp_ * 10); break;
      case REG_POWER_CONSUMPTION: value = this->power_consumption_; break;
      case REG_STATUS: value = this->status_; break;
      case REG_FIRMWARE_VERSION: value = 250; break;  // v2.5.0 = 250
      case REG_DEVICE_ID: value = DEVICE_ID; break;   // 0xAC01
      default: break;
    }

    response.push_back((value >> 8) & 0xFF);
    response.push_back(value & 0xFF);
  }

  send_read_response(FUNC_READ_INPUT_REGISTERS, response);
  ESP_LOGD(TAG, "Read input registers from %d, quantity %d", address, quantity);
}

void PanasonicACModbus::handle_write_coil(uint16_t address, bool state) {
  bool state_changed = false;

  switch (address) {
    case REG_POWER:
      if (this->ac_power_ != state) {
        this->ac_power_ = state;
        state_changed = true;
      }
      break;
    case REG_ECO:
      if (this->ac_eco_ != state) {
        this->ac_eco_ = state;
        state_changed = true;
      }
      break;
    case REG_ECONAVI:
      if (this->ac_econavi_ != state) {
        this->ac_econavi_ = state;
        state_changed = true;
      }
      break;
    case REG_MILD_DRY:
      if (this->ac_mild_dry_ != state) {
        this->ac_mild_dry_ = state;
        state_changed = true;
      }
      break;
    case REG_NANOEX:
      if (this->ac_nanoex_ != state) {
        this->ac_nanoex_ = state;
        state_changed = true;
      }
      break;
    default:
      send_exception(FUNC_WRITE_COIL, EXC_ILLEGAL_DATA_ADDRESS);
      return;
  }

  // Send back the request as confirmation (standard Modbus response)
  std::vector<uint8_t> response;
  response.push_back((address >> 8) & 0xFF);
  response.push_back(address & 0xFF);
  response.push_back(state ? 0xFF : 0x00);
  response.push_back(0x00);
  send_read_response(FUNC_WRITE_COIL, response);

  if (state_changed && this->on_state_change_) {
    this->on_state_change_(address);
  }

  ESP_LOGD(TAG, "Write coil %d = %s", address, state ? "ON" : "OFF");
}

void PanasonicACModbus::handle_write_register(uint16_t address, uint16_t value) {
  bool state_changed = false;

  switch (address) {
    case REG_POWER:
      if (value <= 1 && this->ac_power_ != (value != 0)) {
        this->ac_power_ = (value != 0);
        state_changed = true;
      } else if (value > 1) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_MODE:
      if (value <= 5 && this->ac_mode_ != value) {
        this->ac_mode_ = value;
        if (value == 0) this->ac_power_ = false;
        else this->ac_power_ = true;
        state_changed = true;
      }
      break;
    case REG_TARGET_TEMP: {
      float temp = value / 10.0f;
      if (temp >= 16.0 && temp <= 30.0 && this->ac_target_temp_ != temp) {
        this->ac_target_temp_ = temp;
        state_changed = true;
      }
      break;
    }
    case REG_FAN_MODE:
      if (value <= 5 && this->ac_fan_mode_ != value) {
        this->ac_fan_mode_ = value;
        state_changed = true;
      }
      break;
    case REG_SWING_VERTICAL:
      if (value <= 6 && this->ac_swing_v_ != value) {
        this->ac_swing_v_ = value;
        state_changed = true;
      }
      break;
    case REG_SWING_HORIZONTAL:
      if (value <= 6 && this->ac_swing_h_ != value) {
        this->ac_swing_h_ = value;
        state_changed = true;
      }
      break;
    case REG_ECO:
      if (value <= 1 && this->ac_eco_ != (value != 0)) {
        this->ac_eco_ = (value != 0);
        state_changed = true;
      } else if (value > 1) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_ECONAVI:
      if (value <= 1 && this->ac_econavi_ != (value != 0)) {
        this->ac_econavi_ = (value != 0);
        state_changed = true;
      } else if (value > 1) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_MILD_DRY:
      if (value <= 1 && this->ac_mild_dry_ != (value != 0)) {
        this->ac_mild_dry_ = (value != 0);
        state_changed = true;
      } else if (value > 1) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_NANOEX:
      if (value <= 1 && this->ac_nanoex_ != (value != 0)) {
        this->ac_nanoex_ = (value != 0);
        state_changed = true;
      } else if (value > 1) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_PRESET:
      if (value <= 2 && this->ac_preset_ != value) {
        this->ac_preset_ = value;
        state_changed = true;
      } else if (value > 2) {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    case REG_DEVICE_ADDRESS:
      if (value >= 1 && value <= 247) {
        if (this->address_ != value) {
          this->new_address_ = value;
          this->address_changed_ = true;
          ESP_LOGI(TAG, "Modbus address change requested: %d", value);
        }
      } else {
        send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_VALUE);
        return;
      }
      break;
    default:
      send_exception(FUNC_WRITE_REGISTER, EXC_ILLEGAL_DATA_ADDRESS);
      return;
  }

  // Send back the request as confirmation (standard Modbus response)
  std::vector<uint8_t> response;
  response.push_back((address >> 8) & 0xFF);
  response.push_back(address & 0xFF);
  response.push_back((value >> 8) & 0xFF);
  response.push_back(value & 0xFF);
  send_read_response(FUNC_WRITE_REGISTER, response);

  if (state_changed && this->on_state_change_) {
    this->on_state_change_(address);
  }

  ESP_LOGD(TAG, "Write register %d = %d", address, value);
}

void PanasonicACModbus::send_exception(uint8_t function_code, uint8_t exception_code) {
  std::vector<uint8_t> response;
  response.reserve(3);
  response.push_back(this->address_);
  response.push_back(function_code | 0x80); // Exception function code is original + 0x80
  response.push_back(exception_code);
  this->send_raw(response);
  ESP_LOGV(TAG, "Queued Modbus exception: addr=%u function=0x%02X exception=0x%02X", this->address_, function_code,
           exception_code);
  ESP_LOGW(TAG, "Exception: function 0x%02X, code 0x%02X", function_code, exception_code);
}

void PanasonicACModbus::send_read_response(uint8_t function_code, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> response;
  response.reserve(2 + data.size());
  response.push_back(this->address_);  // Modbus RTU response MUST start with slave address
  response.push_back(function_code);
  response.insert(response.end(), data.begin(), data.end());
  this->send_raw(response);
  ESP_LOGV(TAG, "Queued Modbus response: addr=%u function=0x%02X payload=%u bytes", this->address_, function_code,
           (unsigned int) data.size());
}

// State setters called from main AC component
void PanasonicACModbus::set_ac_power(bool state) {
  this->ac_power_ = state;
}

void PanasonicACModbus::set_ac_mode(uint8_t mode) {
  this->ac_mode_ = mode;
}

void PanasonicACModbus::set_ac_target_temp(float temp) {
  this->ac_target_temp_ = temp;
}

void PanasonicACModbus::set_ac_fan_mode(uint8_t mode) {
  this->ac_fan_mode_ = mode;
}

void PanasonicACModbus::set_ac_swing_vertical(uint8_t mode) {
  this->ac_swing_v_ = mode;
}

void PanasonicACModbus::set_ac_swing_horizontal(uint8_t mode) {
  this->ac_swing_h_ = mode;
}

void PanasonicACModbus::set_ac_eco(bool state) {
  this->ac_eco_ = state;
}

void PanasonicACModbus::set_ac_econavi(bool state) {
  this->ac_econavi_ = state;
}

void PanasonicACModbus::set_ac_mild_dry(bool state) {
  this->ac_mild_dry_ = state;
}

void PanasonicACModbus::set_ac_nanoex(bool state) {
  this->ac_nanoex_ = state;
}

void PanasonicACModbus::set_ac_preset(uint8_t preset) {
  this->ac_preset_ = preset;
}

// Sensor updates from main AC component
void PanasonicACModbus::update_current_temp(float temp) {
  this->current_temp_ = temp;
}

void PanasonicACModbus::update_outside_temp(float temp) {
  this->outside_temp_ = temp;
}

void PanasonicACModbus::update_power_consumption(uint16_t watts) {
  this->power_consumption_ = watts;
}

void PanasonicACModbus::update_status(uint16_t status) {
  this->status_ = status;
}

void PanasonicACModbus::update_ac_state(bool power, uint8_t mode, float target_temp, uint8_t fan_mode,
                                        uint8_t swing_v, uint8_t swing_h, bool eco, bool econavi,
                                        bool mild_dry, bool nanoex, uint8_t preset) {
  this->ac_power_ = power;
  this->ac_mode_ = mode;
  this->ac_target_temp_ = target_temp;
  this->ac_fan_mode_ = fan_mode;
  this->ac_swing_v_ = swing_v;
  this->ac_swing_h_ = swing_h;
  this->ac_eco_ = eco;
  this->ac_econavi_ = econavi;
  this->ac_mild_dry_ = mild_dry;
  this->ac_nanoex_ = nanoex;
  this->ac_preset_ = preset;
}

// Public method to set address from WebUI/API
void PanasonicACModbus::set_address(uint8_t address) {
  if (address >= 1 && address <= 247) {
    if (this->address_ != address) {
      ESP_LOGI(TAG, "Setting Modbus address from WebUI: %d -> %d", this->address_, address);
      this->new_address_ = address;
      this->address_changed_ = true;
    }
  } else {
    ESP_LOGW(TAG, "Invalid address requested from WebUI: %d", address);
  }
}

void PanasonicACModbus::save_device_address(uint8_t address) {
  // Use a fixed hash key for this component type (0xAC01 = Panasonic AC)
  this->pref_ = global_preferences->make_preference<uint8_t>(0xAC010001);
  if (this->pref_.save(&address)) {
    ESP_LOGI(TAG, "Saved Modbus address %d to flash", address);
  } else {
    ESP_LOGW(TAG, "Failed to save Modbus address");
  }
}

void PanasonicACModbus::load_device_address() {
  uint8_t saved_address;
  this->pref_ = global_preferences->make_preference<uint8_t>(0xAC010001);
  if (this->pref_.load(&saved_address)) {
    if (saved_address >= 1 && saved_address <= 247 && saved_address != this->address_) {
      ESP_LOGI(TAG, "Loaded Modbus address %d from flash", saved_address);
      this->address_ = saved_address;
    }
  }
}

}  // namespace panasonic_ac_modbus
}  // namespace esphome
