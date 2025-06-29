#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <locale.h>
#include <map>
#include <math.h>
#include <string>
#include <vector>
#include <windows.h>
#include <stdexcept>
#include <sstream>

using namespace std;

namespace SystemConstants {
const bool NORMAL_STATUS = true;
const bool ALARM_STATUS = false;
const int ONE_SECOND_IN_MS = 1000;
const double INITIAL_TANK_CAPACITY = 20000.0;
const double MIXER_TANK_CAPACITY = 200.0;  // Mixer tank capacity as per requirements
const double INITIAL_BASE_TANK_LEVELS = 25.0;
const double INITIAL_MIXER_TANK_LEVEL = 0.0;
const double NORMAL_OPERATING_PRESSURE = 33.0;
const double HIGH_PRESSURE_THRESHOLD = 50.0;
const double LOW_PRESSURE_THRESHOLD = 20.0;
const double PRESSURE_INCREMENT = 3.0;
const double INITIAL_PRESSURE = 0.0;
const double DEFAULT_FLOW_RATE = 100.0;
const bool INITIAL_PUMP_STATE = false;
const bool INITIAL_FLOW_TRANSMITTER_STATE = NORMAL_STATUS;
const string CONFIG_FILE_PATH = "./tercer_parcial_config.txt";
const double BATCH_SIZE = 150.0;
static map<string, map<string, double> > create_color_recipes() {
    map<string, map<string, double> > recipes;
    
    map<string, double> az_marino;
    az_marino["Negro"] = 2.0 / 3.0;  // 2 parts Negro (100 lts for 150 lts batch)
    az_marino["Azul"] = 1.0 / 3.0;   // 1 part Azul (50 lts for 150 lts batch)
    recipes["AzMarino"] = az_marino;
    
    map<string, double> az_celeste;
    az_celeste["Azul"] = 1.0 / 3.0;
    az_celeste["Negro"] = 1.0 / 3.0;
    az_celeste["Blanco"] = 1.0 / 3.0;
    recipes["AzCeleste"] = az_celeste;
    
    return recipes;
}

const map<string, map<string, double> > COLOR_RECIPES = create_color_recipes();
} // namespace SystemConstants

struct SystemConfig {
    map<string, string> valve_states;
    string color_a_mezclar;
    string arranque_de_fabricacion;
};

class StringUtils {
  public:
    static string trim_whitespace(const string &s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }
};

class ConfigFileHandler {
  public:
    static void open_config_file(ifstream &file, const string &filename) {
        file.open(filename.c_str());
        if (!file.is_open()) {
            throw runtime_error("Error: Could not open config file: " +
                                filename);
        }
    }

    static bool
    parse_config_line(const string &line, string &key, string &value) {
        size_t eq_pos = line.find('=');
        if (eq_pos == string::npos)
            return false;
        key = StringUtils::trim_whitespace(line.substr(0, eq_pos));
        value = StringUtils::trim_whitespace(line.substr(eq_pos + 1));
        return !(key.empty() || value.empty());
    }

    static void create_default_config_file(
        const string &filename = SystemConstants::CONFIG_FILE_PATH) {
        const char *default_config =
            "# Valvulas de entrada \n"
            "# Valores Posibles: OPEN / CLOSE\n"
            "V201 = OPEN\n"
            "V202 = OPEN\n"
            "V203 = OPEN\n"
            "\n"
            "# Valvulas de salida\n"
            "# Valores Posibles: OPEN / CLOSE\n"
            "V401 = OPEN\n"
            "V402 = OPEN\n"
            "V403 = OPEN\n"
            "\n"
            "# Valores Posibles: AzMarino / AzCeleste\n"
            "COLOR_A_MEZCLAR = AzCeleste\n"
            "\n"
            "# Valores Posibles: ON / OFF   (Se debe apagar <OFF> y volver a "
            "encender <ON>\n"
            "# para comenzar un nuevo lote)\n"
            "ARRANQUE_DE_FABRICACION = OFF\n";

        ofstream out(filename.c_str(), ios::trunc);
        if (!out) {
            throw runtime_error("Could not write default config file: " +
                                filename);
        }
        out << default_config;
        out.close();
    }
};

static vector<string> create_known_valve_keys() {
    vector<string> keys;
    keys.push_back("V201");
    keys.push_back("V202");
    keys.push_back("V203");
    keys.push_back("V401");
    keys.push_back("V402");
    keys.push_back("V403");
    return keys;
}

class ConfigValidator {
  private:
    static const vector<string> KNOWN_VALVE_KEYS;
    static const string K_COLOR;
    static const string K_ARRANQUE;

  public:
    static void validate_and_set_config_pair(SystemConfig &config,
                                             const string &key,
                                             const string &value) {
        if (key == K_COLOR) {
            validate_color_value(value);
            config.color_a_mezclar = value;
        } else if (key == K_ARRANQUE) {
            validate_start_command_value(value);
            config.arranque_de_fabricacion = value;
        } else if (is_known_valve(key)) {
            validate_valve_value(key, value);
            config.valve_states[key] = value;
        } else {
            throw runtime_error("Invalid configuration key found: " + key);
        }
    }

    static void validate_complete_config(const SystemConfig &config) {
        if (config.color_a_mezclar.empty()) {
            throw runtime_error("Missing required setting: COLOR_A_MEZCLAR");
        }
        if (config.arranque_de_fabricacion.empty()) {
            throw runtime_error(
                "Missing required setting: ARRANQUE_DE_FABRICACION");
        }

        vector<string> known_valves = create_known_valve_keys();
        for (size_t i = 0; i < known_valves.size(); ++i) {
            const string& valve = known_valves[i];
            if (config.valve_states.find(valve) == config.valve_states.end()) {
                throw runtime_error("Missing required valve setting: " + valve);
            }
        }
    }

  private:
    static void validate_color_value(const string &value) {
        if (value != "AzMarino" && value != "AzCeleste") {
            throw runtime_error("Invalid value for COLOR_A_MEZCLAR: " + value);
        }
    }

    static void validate_start_command_value(const string &value) {
        if (value != "ON" && value != "OFF") {
            throw runtime_error("Invalid value for ARRANQUE_DE_FABRICACION: " +
                                value);
        }
    }

    static void validate_valve_value(const string &key, const string &value) {
        if (value != "OPEN" && value != "CLOSE") {
            throw runtime_error("Invalid value for valve " + key + ": " +
                                value);
        }
    }

    static bool is_known_valve(const string &key) {
        vector<string> known_valves = create_known_valve_keys();
        return find(known_valves.begin(), known_valves.end(), key) !=
               known_valves.end();
    }
};

const vector<string> ConfigValidator::KNOWN_VALVE_KEYS = create_known_valve_keys();
const string ConfigValidator::K_COLOR = "COLOR_A_MEZCLAR";
const string ConfigValidator::K_ARRANQUE = "ARRANQUE_DE_FABRICACION";

class ConfigManager {
  public:
    static SystemConfig
    read_config(const string &filename = SystemConstants::CONFIG_FILE_PATH) {
        SystemConfig config;
        ifstream file;
        ConfigFileHandler::open_config_file(file, filename);
        string line, key, value;
        int line_number = 0;

        while (getline(file, line)) {
            line_number++;
            string trimmed = StringUtils::trim_whitespace(line);

            if (trimmed.empty() || trimmed[0] == '#')
                continue;

            if (!ConfigFileHandler::parse_config_line(trimmed, key, value)) {
                cerr << "Warning: Malformed config line " << line_number << ": "
                     << line << endl;
                continue;
            }
            ConfigValidator::validate_and_set_config_pair(config, key, value);
        }

        ConfigValidator::validate_complete_config(config);
        return config;
    }

    static void repair_or_create_config_file(
        const string &filename = SystemConstants::CONFIG_FILE_PATH) {
        ConfigFileHandler::create_default_config_file(filename);
    }
};

class LevelTransmitter {
  private:
    string code_;
    double level_;

    void evaluate_level(double &current_capacity, double &max_capacity) {
        if (current_capacity > max_capacity) {
            current_capacity = max_capacity;
        } else if (current_capacity < 0) {
            current_capacity = 0;
        }
        level_ = (current_capacity / max_capacity) * 100.0;
    }

  public:
    explicit LevelTransmitter(const string &code)
        : code_(code), level_(SystemConstants::INITIAL_MIXER_TANK_LEVEL) {}

    const string &get_code() const { return code_; }

    double read_level(double &current_capacity, double &max_capacity) const {
        const_cast<LevelTransmitter *>(this)->evaluate_level(current_capacity,
                                                             max_capacity);
        return level_;
    }
};

class LiquidTank {
  private:
    string code_;
    string liquid_name_;
    double max_capacity_;
    double current_capacity_;
    LevelTransmitter level_transmitter_;

  public:
    LiquidTank(const string &tank_code,
               const string &level_transmitter_code,
               const string &liquid_name,
               double max_capacity,
               double current_capacity)
        : code_(tank_code), liquid_name_(liquid_name),
          max_capacity_(max_capacity), current_capacity_(current_capacity),
          level_transmitter_(level_transmitter_code) {}

    const string &get_liquid_in_tank_name() const { return liquid_name_; }
    const string &get_code() const { return code_; }
    double get_max_capacity() const { return max_capacity_; }
    double get_current_capacity() const { return current_capacity_; }

    double get_level() const {
        return level_transmitter_.read_level(
            const_cast<double &>(current_capacity_),
            const_cast<double &>(max_capacity_));
    }

    double drain(double amount) {
        if (amount <= 0) {
            return 0.0;
        }

        double drained_amount;
        if (current_capacity_ >= amount) {
            current_capacity_ -= amount;
            drained_amount = amount;
        } else {
            drained_amount = current_capacity_;
            current_capacity_ = 0.0;
        }
        return drained_amount;
    }
};

class FlowSwitch {
  private:
    string code_;
    bool status_;

  public:
    explicit FlowSwitch(
        const string &code,
        bool initial_status = SystemConstants::INITIAL_FLOW_TRANSMITTER_STATE)
        : code_(code), status_(initial_status) {
        if (code.empty()) {
            throw invalid_argument("FlowSwitch code cannot be empty");
        }
    }

    void evaluate_status(double flow_rate, bool pump_should_be_flowing) {
        // Flow switch goes to ALARM when:
        // 1. Pump should be flowing but flow rate is 0 (valve issues, blockage, etc.)
        // 2. Any flow anomaly when pump is running at expected capacity
        if (pump_should_be_flowing && flow_rate == 0) {
            status_ = SystemConstants::ALARM_STATUS;
        } else if (!pump_should_be_flowing) {
            // If pump shouldn't be flowing, flow switch should be normal
            status_ = SystemConstants::NORMAL_STATUS;
        } else {
            // Pump is flowing at expected rate, normal operation
            status_ = SystemConstants::NORMAL_STATUS;
        }
    }

    const string &get_code() const { return code_; }
    bool is_normal() const { return status_ == SystemConstants::NORMAL_STATUS; }
    bool is_alarm() const { return status_ == SystemConstants::ALARM_STATUS; }
};

class Valve {
  private:
    string code_;
    bool is_open_;

  public:
    explicit Valve(const string &code, bool is_open = true)
        : code_(code), is_open_(is_open) {}

    void toggle() { is_open_ = !is_open_; }
    void set_open(bool new_open_state) { is_open_ = new_open_state; }
    bool is_open() const { return is_open_; }
    const string &get_code() const { return code_; }
};

enum PumpState {
    STOPPED_LOW_PRESSURE,
    STOPPED_HIGH_PRESSURE,
    STOPPED_FLOW_ALARM,
    STOPPED_TARGET_REACHED,
    RUNNING
};

class LiquidPump {
  private:
    string code_;
    double flow_rate_lts_min_;
    double target_pump_duration_seconds_;
    double pump_elapsed_seconds_;
    bool is_on_;
    PumpState current_state_;

    void start() {
        is_on_ = true;
        current_state_ = RUNNING;
    }

    void stop(PumpState reason) {
        is_on_ = false;
        current_state_ = reason;
    }

    bool should_stop_for_alarm(const FlowSwitch &flow_switch) const {
        return flow_switch.is_alarm();
    }

    bool should_stop_for_high_pressure(double current_pressure) const {
        return current_pressure > SystemConstants::HIGH_PRESSURE_THRESHOLD;
    }

    bool should_stop_for_target_reached() const {
        return pump_elapsed_seconds_ >= target_pump_duration_seconds_;
    }

    bool can_start_for_pressure(double current_pressure) const {
        return current_pressure < SystemConstants::LOW_PRESSURE_THRESHOLD;
    }

    bool can_start_pumping(const FlowSwitch &flow_switch, double current_pressure, 
                          const Valve &enter_valve, const Valve &exit_valve) const {
        // Pump can start if:
        // 1. Flow switch is in normal state (no alarm)
        // 2. Pressure is below low threshold (20 psi)
        // 3. Pump has a valid target duration
        // 4. Pump hasn't reached its target yet
        // 5. Both enter and exit valves are open (essential for pump operation)
        return flow_switch.is_normal() && 
               can_start_for_pressure(current_pressure) &&
               target_pump_duration_seconds_ > 0.0 &&
               pump_elapsed_seconds_ < target_pump_duration_seconds_ &&
               enter_valve.is_open() && 
               exit_valve.is_open();
    }

  public:
    explicit LiquidPump(const string &code,
                        double flow_rate = SystemConstants::DEFAULT_FLOW_RATE)
        : code_(code), flow_rate_lts_min_(flow_rate),
          target_pump_duration_seconds_(0.0), pump_elapsed_seconds_(0.0),
          is_on_(SystemConstants::INITIAL_PUMP_STATE),
          current_state_(STOPPED_LOW_PRESSURE) {}

    bool is_on() const { return is_on_; }
    double get_flow_rate() const { return flow_rate_lts_min_; }
    const string &get_code() const { return code_; }
    PumpState get_state() const { return current_state_; }
    double get_elapsed_seconds() const { return pump_elapsed_seconds_; }
    double get_target_duration() const { return target_pump_duration_seconds_; }

    double get_actual_flow_rate(const Valve &enter_valve, const Valve &exit_valve) const {
        // Flow rate is 100 lts/min ONLY when:
        // 1. Pump is ON
        // 2. Both suction (enter) and discharge (exit) valves are open
        // If any valve is closed or pump is off, flow rate is 0
        if (is_on_ && enter_valve.is_open() && exit_valve.is_open()) {
            return flow_rate_lts_min_;
        }
        return 0.0; // No flow if pump is off or any valve is closed
    }

    // Fixed: Added valve state checking to prevent pumps from starting/restarting
    // when essential valves (enter/exit) are closed. This ensures pumps only
    // operate when they can actually function properly.
    void update_pump_state(const FlowSwitch &flow_switch,
                           double current_pressure,
                           const Valve &enter_valve,
                           const Valve &exit_valve) {
        // Check stop conditions first (in order of priority)
        
        // 1. Flow alarm - immediate stop
        if (should_stop_for_alarm(flow_switch)) {
            stop(STOPPED_FLOW_ALARM);
            return;
        }

        // 2. High pressure - stop to prevent damage  
        if (should_stop_for_high_pressure(current_pressure)) {
            stop(STOPPED_HIGH_PRESSURE);
            return;
        }

        // 3. Target reached - stop when pumping goal is achieved
        if (should_stop_for_target_reached()) {
            stop(STOPPED_TARGET_REACHED);
            return;
        }

        // Check restart conditions based on current stop reason
        if (!is_on_) {
            if (current_state_ == STOPPED_FLOW_ALARM) {
                // After flow alarm: restart only when flow is normal AND pressure is low AND valves are open
                // This ensures the flow issue has been resolved and valves are properly configured
                if (flow_switch.is_normal() && 
                    can_start_for_pressure(current_pressure) &&
                    enter_valve.is_open() && 
                    exit_valve.is_open()) {
                    start();
                }
            } else if (current_state_ == STOPPED_HIGH_PRESSURE) {
                // After high pressure: restart when pressure drops below low threshold AND valves are open
                // This implements the 20 psi restart logic from requirements
                if (can_start_for_pressure(current_pressure) &&
                    enter_valve.is_open() && 
                    exit_valve.is_open()) {
                    start();
                }
            } else if (current_state_ == STOPPED_LOW_PRESSURE) {
                // Initial state or after manual stop: start when pressure allows AND valves are open
                if (can_start_for_pressure(current_pressure) && 
                    flow_switch.is_normal() &&
                    enter_valve.is_open() && 
                    exit_valve.is_open()) {
                    start();
                }
            }
            // STOPPED_TARGET_REACHED pumps should not restart automatically
        }
    }

    void set_pump_target_liters(double amount_lts) {
        if (amount_lts > 0) {
            target_pump_duration_seconds_ =
                (amount_lts / flow_rate_lts_min_) * 60.0;
            pump_elapsed_seconds_ = 0.0;
        }
    }

    void increment_elapsed_time(double seconds) {
        if (is_on_) {
            pump_elapsed_seconds_ += seconds;
        }
    }
};

class PressureTransmitter {
  private:
    string code_;
    double pressure_;

  public:
    explicit PressureTransmitter(
        const string &code,
        double pressure = SystemConstants::INITIAL_PRESSURE)
        : code_(code), pressure_(pressure) {}

    const string &get_code() const { return code_; }
    double read_pressure() const { return pressure_; }

    void update_pressure(const Valve &enter_valve,
                         const Valve &exit_valve,
                         const LiquidPump &pump) {

        if (pump.is_on()) {
            // --- Pump is ON ---
            if (enter_valve.is_open() && exit_valve.is_open()) {
                // Normal operation: stabilize at 33 psi
                // Gradually approach normal pressure if not already there
                if (pressure_ < SystemConstants::NORMAL_OPERATING_PRESSURE) {
                    pressure_ += SystemConstants::PRESSURE_INCREMENT;
                    if (pressure_ > SystemConstants::NORMAL_OPERATING_PRESSURE) {
                        pressure_ = SystemConstants::NORMAL_OPERATING_PRESSURE;
                    }
                } else if (pressure_ > SystemConstants::NORMAL_OPERATING_PRESSURE) {
                    pressure_ -= SystemConstants::PRESSURE_INCREMENT;
                    if (pressure_ < SystemConstants::NORMAL_OPERATING_PRESSURE) {
                        pressure_ = SystemConstants::NORMAL_OPERATING_PRESSURE;
                    }
                }
            } else if (!exit_valve.is_open() && enter_valve.is_open()) { 
                // Exit valve closed, pump running - pressure builds up gradually
                // Flow rate immediately drops to 0 lts/min when discharge valve closes
                pressure_ += SystemConstants::PRESSURE_INCREMENT;
                // Pressure will build until it reaches HIGH_PRESSURE_THRESHOLD and pump stops
            } else { 
                // Enter valve closed - pump can't draw liquid, pressure drops to zero
                if (pressure_ > SystemConstants::INITIAL_PRESSURE) {
                    pressure_ -= SystemConstants::PRESSURE_INCREMENT;
                    if (pressure_ < SystemConstants::INITIAL_PRESSURE) {
                        pressure_ = SystemConstants::INITIAL_PRESSURE;
                    }
                } else {
                    pressure_ = SystemConstants::INITIAL_PRESSURE;
                }
            }
        } else {
            // --- Pump is OFF ---
            // Handle pressure behavior based on pump stop reason and valve states
            if (pump.get_state() == STOPPED_FLOW_ALARM) {
                // After flow alarm shutdown: pressure behavior depends on discharge valve
                if (exit_valve.is_open()) {
                    // Discharge valve open: pressure drops to 0 psi gradually
                    if (pressure_ > SystemConstants::INITIAL_PRESSURE) {
                        pressure_ -= SystemConstants::PRESSURE_INCREMENT;
                        if (pressure_ < SystemConstants::INITIAL_PRESSURE) {
                            pressure_ = SystemConstants::INITIAL_PRESSURE;
                        }
                    }
                }
                // If discharge valve closed: pressure maintains last value (no change)
            } else {
                // For other stop reasons (high pressure, target reached, etc.)
                if (exit_valve.is_open()) {
                    // Valve open, pressure decays gradually toward zero
                    if (pressure_ > SystemConstants::INITIAL_PRESSURE) {
                        pressure_ -= SystemConstants::PRESSURE_INCREMENT * 0.7; // Controlled decay
                        if (pressure_ < SystemConstants::INITIAL_PRESSURE) {
                            pressure_ = SystemConstants::INITIAL_PRESSURE;
                        }
                    }
                }
                // If exit valve closed and pump off, pressure remains at current level
            }
        }

        // Ensure pressure doesn't go below zero or above maximum safe limits
        if (pressure_ < SystemConstants::INITIAL_PRESSURE) {
            pressure_ = SystemConstants::INITIAL_PRESSURE;
        }
        // Note: No upper limit check here as high pressure should trigger pump shutdown
    }
};

class PumpLine {
  private:
    LiquidPump pump_;
    Valve enter_valve_;
    Valve exit_valve_;
    FlowSwitch flow_switch_;
    PressureTransmitter pressure_transmitter_;
    LiquidTank tank_;

PumpLine(const string &pump_code, // Private constructor
             const string &enter_valve_code,
             const string &exit_valve_code,
             const string &flow_switch_code,
             const string &pressure_transmitter_code,
             const string &tank_code,
             const string &liquid_name,
             double max_capacity,
             double current_capacity,
             const string &level_transmitter_code)
        : pump_(pump_code), enter_valve_(enter_valve_code),
          exit_valve_(exit_valve_code), flow_switch_(flow_switch_code),
          pressure_transmitter_(pressure_transmitter_code),
          tank_(tank_code,
                level_transmitter_code,
                liquid_name,
                max_capacity,
                current_capacity) {}

  public:
    // Single creation method for standard paint lines - SIMPLIFIED
    static PumpLine create_standard_paint_line(const string &pump_code,
                                               const string &liquid_name) {
        if (pump_code.empty())
            throw invalid_argument("Pump code required");
        if (liquid_name.empty())
            throw invalid_argument("Liquid name required");

        string numeric_pump_code = pump_code.substr(1); // "201" from "P201"
        string last_two_digits = pump_code.substr(2);   // "01" from "P201"

        string enter_valve = "V" + numeric_pump_code;
        string exit_valve = "V4" + last_two_digits;
        string flow_switch = "FS" + numeric_pump_code;
        string pressure_transmitter = "PT4" + last_two_digits;
        string tank = "TQ" + numeric_pump_code;
        string level_transmitter = "LT" + numeric_pump_code;

        double max_capacity = SystemConstants::INITIAL_TANK_CAPACITY;
        double current_capacity =
            max_capacity * SystemConstants::INITIAL_BASE_TANK_LEVELS / 100.0;

        return PumpLine(pump_code, enter_valve, exit_valve, flow_switch,
                        pressure_transmitter, tank, liquid_name, max_capacity,
                        current_capacity, level_transmitter);
    }

    const LiquidPump &get_pump() const { return pump_; }
    const Valve &get_enter_valve() const { return enter_valve_; }
    const Valve &get_exit_valve() const { return exit_valve_; }
    const FlowSwitch &get_flow_switch() const { return flow_switch_; }
    const PressureTransmitter &get_pressure_transmitter() const {
        return pressure_transmitter_;
    }
    const LiquidTank &get_tank() const { return tank_; }

    LiquidPump &get_pump_mutable() { return pump_; }
    Valve &get_enter_valve_mutable() { return enter_valve_; }
    Valve &get_exit_valve_mutable() { return exit_valve_; }
    LiquidTank &get_tank_mutable() { return tank_; }

    void update_system_state() {
        // Calculate actual flow rate based on current pump and valve states
        double physical_flow = pump_.get_actual_flow_rate(enter_valve_, exit_valve_);
        bool pump_should_be_flowing = pump_.is_on();
        
        // Evaluate flow switch status based on physical flow and pump expectation
        // Flow switch will alarm if pump is on but no flow due to valve closure or blockage
        flow_switch_.evaluate_status(physical_flow, pump_should_be_flowing);

        // Order of updates matters for proper simulation:
        // 1. Update pressure based on current valve/pump states (BEFORE pump state changes)
        pressure_transmitter_.update_pressure(enter_valve_, exit_valve_, pump_);
        
        // 2. Update pump state based on flow switch status and new pressure readings
        pump_.update_pump_state(flow_switch_, pressure_transmitter_.read_pressure(), enter_valve_, exit_valve_);

        // 3. Increment elapsed time if pump is (still) running after state update
        if (pump_.is_on()) {
            pump_.increment_elapsed_time(1.0); // Increment by 1 second
        }
    }

    bool need_to_pump() const {
        // Check if pump has reached its target duration
        if (pump_.get_elapsed_seconds() >= pump_.get_target_duration()) {
            return false; // Target reached, no more pumping needed
        }
        
        // Check if pump has a valid target (greater than 0)
        if (pump_.get_target_duration() <= 0.0) {
            return false; // No target set, no pumping needed
        }
        
        // Check pump state and conditions
        PumpState state = pump_.get_state();
        
        // Pump needs to continue if it's currently running
        if (state == RUNNING) {
            return true; // Currently pumping and hasn't reached target
        }
        
        // For stopped pumps, check if they can potentially restart
        if (state == STOPPED_FLOW_ALARM || state == STOPPED_HIGH_PRESSURE || state == STOPPED_LOW_PRESSURE) {
            // These states can potentially restart if conditions improve
            // Also check if valves are in proper state for pumping
            if (enter_valve_.is_open() && exit_valve_.is_open()) {
                return true; // Conditions allow potential restart
            }
        }
        
        // If stopped due to target reached, no more pumping needed
        return false;
    }
};

class LowLevelSwitch {
  private:
    string code_;
    bool status_;

  public:
    explicit LowLevelSwitch(const string &code,
                            bool initial_status = SystemConstants::ALARM_STATUS)
        : code_(code), status_(initial_status) {
        if (code.empty()) {
            throw invalid_argument("LowLevelSwitch code cannot be empty");
        }
    }

    void set_status(bool new_status) { status_ = new_status; }
    bool is_normal() const { return status_ == SystemConstants::NORMAL_STATUS; }
    bool is_alarm() const { return status_ == SystemConstants::ALARM_STATUS; }
    const string &get_code() const { return code_; }
};

class MixerMotor {
    ;

  private:
    string code_;
    bool is_on_;
    double elapsed_time_;
    double target_time_;

  public:
    explicit MixerMotor(const string &code,
                        bool initial_state = false,
                        double target_time = 30)
        : code_(code), is_on_(initial_state), elapsed_time_(0),
          target_time_(target_time) {
        if (code.empty()) {
            throw invalid_argument("MixerMotor code cannot be empty");
        }
    }

    void start() {
        is_on_ = true;
        elapsed_time_ = 0.0; // Reset elapsed time when starting
    }
    void stop() { is_on_ = false; }
    void reset() { 
        is_on_ = false; 
        elapsed_time_ = 0.0; // Reset elapsed time completely
    }
    bool is_running() const { return is_on_; }
    const string &get_code() const { return code_; }
    double get_elapsed_time() const { return elapsed_time_; }
    double get_target_time() const { return target_time_; }
    double get_time_left() const { return target_time_ - elapsed_time_; }
    double update_mixing_progress(double elapsed_seconds) {
        if (is_on_) {
            elapsed_time_ += elapsed_seconds;
            if (elapsed_time_ >= target_time_) {
                elapsed_time_ = target_time_;
                stop();
            }
        }
        return get_time_left();
    }
};

class MixerTank {
  private:
    LevelTransmitter level_transmitter_;
    LowLevelSwitch low_level_switch_;
    MixerMotor mixer_motor_;
    double current_capacity_;
    double max_capacity_;
    string code_;
    // Emptying timer variables
    bool emptying_active_;
    double emptying_elapsed_time_;
    double emptying_rate_percent_per_second_;

    void update_low_level_switch() {
        double level_percent = (current_capacity_ / max_capacity_) * 100.0;
        if (level_percent < 10.0) { // 10% threshold for alarm
            low_level_switch_.set_status(SystemConstants::ALARM_STATUS);
        } else {
            low_level_switch_.set_status(SystemConstants::NORMAL_STATUS);
        }
    }

  public:
    MixerTank(const string &code,
              const string &level_transmitter_code,
              double max_capacity = SystemConstants::MIXER_TANK_CAPACITY,
              double initial_capacity = 0.0)
        : level_transmitter_(level_transmitter_code),
          low_level_switch_(code, SystemConstants::ALARM_STATUS),
          mixer_motor_(code), current_capacity_(initial_capacity),
          max_capacity_(max_capacity), code_(code),
          emptying_active_(false), emptying_elapsed_time_(0.0),
          emptying_rate_percent_per_second_(4.0) {
        if (code.empty()) {
            throw invalid_argument("MixerTank code cannot be empty");
        }
        if (max_capacity <= 0) {
            throw invalid_argument("MixerTank max capacity must be positive");
        }
        if (initial_capacity < 0 || initial_capacity > max_capacity) {
            throw invalid_argument(
                "MixerTank initial capacity must be between 0 and max "
                "capacity");
        }
    }

    const string &get_code() const { return code_; }
    const LevelTransmitter &get_level_transmitter() const {
        return level_transmitter_;
    }
    const LowLevelSwitch &get_low_level_switch() const {
        return low_level_switch_;
    }
    const MixerMotor &get_mixer_motor() const { return mixer_motor_; }
    double get_current_capacity() const { return current_capacity_; }
    double get_max_capacity() const { return max_capacity_; }
    double get_level() const {
        return level_transmitter_.read_level(
            const_cast<double &>(current_capacity_),
            const_cast<double &>(max_capacity_));
    }

    void add_liquid(double liters) {
        current_capacity_ += liters;
        if (current_capacity_ > max_capacity_) {
            current_capacity_ = max_capacity_;
        }
        update_low_level_switch();
    }

    void start_emptying() {
        emptying_active_ = true;
        emptying_elapsed_time_ = 0.0;
    }

    void stop_emptying() {
        emptying_active_ = false;
        emptying_elapsed_time_ = 0.0;
    }

    bool is_emptying() const {
        return emptying_active_;
    }

    double get_emptying_elapsed_time() const {
        return emptying_elapsed_time_;
    }

    double update_emptying_progress(double elapsed_seconds) {
        if (!emptying_active_ || current_capacity_ <= 0) {
            if (current_capacity_ <= 0) {
                stop_emptying();
            }
            return 0.0;
        }
        
        emptying_elapsed_time_ += elapsed_seconds;
        
        double amount_to_drain = (max_capacity_ * emptying_rate_percent_per_second_ / 100.0) * elapsed_seconds;
        double actually_drained;
        
        if (current_capacity_ >= amount_to_drain) {
            actually_drained = amount_to_drain;
            current_capacity_ -= amount_to_drain;
        } else {
            actually_drained = current_capacity_;
            current_capacity_ = 0.0;
            stop_emptying(); // Automatically stop when empty
        }
        
        update_low_level_switch();
        return actually_drained;
    }

    double empty_tank(double empty_rate_percent_per_second = 4.0) {
        if (current_capacity_ <= 0) {
            return 0.0;
        }
        
        double amount_to_drain = (max_capacity_ * empty_rate_percent_per_second / 100.0);
        double actually_drained;
        
        if (current_capacity_ >= amount_to_drain) {
            actually_drained = amount_to_drain;
            current_capacity_ -= amount_to_drain;
        } else {
            actually_drained = current_capacity_;
            current_capacity_ = 0.0;
        }
        
        update_low_level_switch();
        return actually_drained;
    }

    bool is_empty() const {
        return current_capacity_ <= 0.0;
    }

    void reset_emptying() {
        emptying_active_ = false;
        emptying_elapsed_time_ = 0.0;
    }

    LowLevelSwitch &get_low_level_switch_mutable() { return low_level_switch_; }
    MixerMotor &get_mixer_motor_mutable() { return mixer_motor_; }
};

class Factory {
  private:
    map<string, PumpLine> pump_lines_;
    bool batch_in_process_;
    bool emptying_in_process_;
    MixerTank mixer_tank_;

    // Private constructor ensures controlled initialization
    explicit Factory(const vector<PumpLine> &pump_lines)
        : batch_in_process_(false), emptying_in_process_(false),
          mixer_tank_("M401",
                      "LT401",
                      SystemConstants::MIXER_TANK_CAPACITY,
                      SystemConstants::INITIAL_MIXER_TANK_LEVEL) {
        if (pump_lines.empty()) {
            throw runtime_error("Factory must have at least one pump line");
        }
        for (size_t i = 0; i < pump_lines.size(); ++i) {
            const PumpLine& line = pump_lines[i];
            pump_lines_.insert(make_pair(line.get_pump().get_code(), line));
        }
    }

  public:
    static Factory create_dupont_paint_factory() { // Standard Dupont setup
        vector<PumpLine> pump_lines;

        pump_lines.push_back(
            PumpLine::create_standard_paint_line("P201", "Blanco"));
        pump_lines.push_back(
            PumpLine::create_standard_paint_line("P202", "Azul"));
        pump_lines.push_back(
            PumpLine::create_standard_paint_line("P203", "Negro"));

        return Factory(pump_lines);
    }

    // Static factory method for custom configurations
    static Factory create_custom_factory(const vector<PumpLine> &pump_lines) {
        return Factory(pump_lines);
    }

    const MixerTank &get_mixer_tank() const { return mixer_tank_; }
    MixerTank &get_mixer_tank_mutable() { return mixer_tank_; }

    bool need_to_mix() const {
        return mixer_tank_.get_low_level_switch().is_alarm() &&
               !batch_in_process_;
    }

    bool is_batch_in_process() const { return batch_in_process_; }

    bool is_emptying_in_process() const { return emptying_in_process_; }

    bool is_batch_complete() const {
        return !batch_in_process_ && !emptying_in_process_ && 
               mixer_tank_.is_empty() && !pump_lines_need_to_pump();
    }

    void set_batch_in_process() {
        if (!batch_in_process_) {
            batch_in_process_ = true;
        };
    }

    const PumpLine &get_pump_line(const string &pump_code) const {
        map<string, PumpLine>::const_iterator it = pump_lines_.find(pump_code);
        if (it == pump_lines_.end()) {
            throw runtime_error("Pump line not found: " + pump_code);
        }
        return it->second;
    }

    PumpLine &get_pump_line_mutable(const string &pump_code) {
        map<string, PumpLine>::iterator it = pump_lines_.find(pump_code);
        if (it == pump_lines_.end()) {
            throw runtime_error("Pump line not found: " + pump_code);
        }
        return it->second;
    }

    const map<string, PumpLine> &get_all_pump_lines() const {
        return pump_lines_;
    }

    void transfer_liquid_to_mixer(double seconds = 1.0) {
        for (map<string, PumpLine>::iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            PumpLine& pump_line = it->second;
            LiquidPump& pump = pump_line.get_pump_mutable();
            LiquidTank& tank = pump_line.get_tank_mutable();
            // Check pump state AND valve states for actual liquid transfer
            if (pump.is_on() &&
                pump_line.get_enter_valve().is_open() && // Check inlet valve
                pump_line.get_exit_valve().is_open() &&  // Check outlet valve
                (pump.get_elapsed_seconds() < pump.get_target_duration())) {
                double flow_rate = pump.get_flow_rate(); // lts/min
                double liters_this_cycle = flow_rate / 60.0 * seconds;
                double drained = tank.drain(liters_this_cycle);
                mixer_tank_.add_liquid(drained);
            }
        }
    }

    void update_all_pump_lines() {
        for (map<string, PumpLine>::iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            it->second.update_system_state();
        }

        transfer_liquid_to_mixer();
    }

    void set_pump_times(const std::string &target_color) {
        if (pump_lines_.empty()) {
            throw runtime_error("No pump lines available to set times");
        }

        map<string, map<string, double> > color_recipes = SystemConstants::create_color_recipes();
        map<string, map<string, double> >::iterator color_recipe = color_recipes.find(target_color);

        // for to get the pump in each pumpline, compare if the color content of
        // the line is one of the targeted and if is, set the time with the
        // proportions

        for (map<string, PumpLine>::iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            PumpLine& pump_line = it->second;
            const LiquidTank& tank = pump_line.get_tank();

            if (color_recipe != color_recipes.end()) {
                const map<string, double>& recipe = color_recipe->second;
                map<string, double>::const_iterator recipe_it = recipe.find(tank.get_liquid_in_tank_name());
                if (recipe_it != recipe.end()) {
                    double target_liters =
                        SystemConstants::BATCH_SIZE * recipe_it->second;
                    pump_line.get_pump_mutable().set_pump_target_liters(
                        target_liters);
                } else {
                    pump_line.get_pump_mutable().set_pump_target_liters(0);
                }
            } else {
                pump_line.get_pump_mutable().set_pump_target_liters(0);
            }
        }
    }

    void reset() {
        for (map<string, PumpLine>::iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            PumpLine& pump_line = it->second;
            pump_line.get_pump_mutable().set_pump_target_liters(0);
            pump_line.get_pump_mutable().increment_elapsed_time(
                -pump_line.get_pump().get_elapsed_seconds());
            pump_line.get_enter_valve_mutable().set_open(true);
            pump_line.get_exit_valve_mutable().set_open(true);
        }
        emptying_in_process_ = false;
        // Reset mixer motor completely
        mixer_tank_.get_mixer_motor_mutable().reset();
        // Reset emptying timer
        mixer_tank_.reset_emptying();
    }

    bool pump_lines_need_to_pump() const {
        for (map<string, PumpLine>::const_iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            const PumpLine& pump_line = it->second;
            if (pump_line.need_to_pump()) {
                return true;
            }
        }
        return false;
    }

    bool all_required_pumps_completed() const {
        // Check if all pumps that have a target have either:
        // 1. Reached their target duration (STOPPED_TARGET_REACHED), OR
        // 2. Are permanently unable to continue (valves closed, etc.)
        // 
        // IMPORTANT: Pumps that are temporarily stopped (flow alarms, pressure issues)
        // should NOT be considered completed, as they may restart and continue pumping
        
        for (map<string, PumpLine>::const_iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            const PumpLine& pump_line = it->second;
            const LiquidPump& pump = pump_line.get_pump();
            
            // Skip pumps with no target (target duration = 0)
            if (pump.get_target_duration() <= 0) {
                continue;
            }
            
            // Check if this pump has actually completed its target
            if (pump.get_state() == STOPPED_TARGET_REACHED) {
                continue; // This pump is truly done
            }
            
            // Check if pump is permanently prevented from continuing due to valve configuration
            if (!pump_line.get_enter_valve().is_open() || !pump_line.get_exit_valve().is_open()) {
                // Pump can't continue due to closed valves - consider it "completed" for mixing purposes
                continue;
            }
            
            // If pump is temporarily stopped (flow alarm, pressure issues) but hasn't reached target,
            // it should not be considered completed as it may restart
            if (pump.get_state() == STOPPED_FLOW_ALARM || 
                pump.get_state() == STOPPED_HIGH_PRESSURE || 
                pump.get_state() == STOPPED_LOW_PRESSURE) {
                // Pump is paused but could potentially restart - not completed
                return false;
            }
            
            // If pump is currently running and hasn't reached target, it's not completed
            if (pump.get_state() == RUNNING && pump.get_elapsed_seconds() < pump.get_target_duration()) {
                return false;
            }
        }
        
        return true; // All required pumps are completed or permanently unable to continue
    }

    void update_mix() {
        // ENHANCED MIXING CONTROL: Complete check to avoid premature mixing 
        // Ensures all base colors are fully pumped before mixing starts
        // Addresses issue where mixer could start when pumps are paused (flow alarms, pressure issues)
        if (can_start_mixing() && !mixer_tank_.get_mixer_motor().is_running()) {
            // Start mixing if there's liquid in the tank and batch is in process
            if (mixer_tank_.get_current_capacity() > 0 && batch_in_process_) {
                mixer_tank_.get_mixer_motor_mutable().start();
            }
        }
        
        // If mixer is running, update its progress (advance one second)
        if (mixer_tank_.get_mixer_motor().is_running()) {
            mixer_tank_.get_mixer_motor_mutable().update_mixing_progress(1.0);
            
            // Check if mixing is complete - ONLY start emptying after mixing is finished
            if (!mixer_tank_.get_mixer_motor().is_running() && mixer_tank_.get_current_capacity() > 0) {
                emptying_in_process_ = true;
                mixer_tank_.start_emptying(); // Start the emptying timer
            }
        }
    }

    void update_emptying() {
        if (emptying_in_process_) {
            // Use the new timer-based emptying system
            mixer_tank_.update_emptying_progress(1.0);
            
            // Check if tank is empty - finish batch process
            if (mixer_tank_.is_empty()) {
                emptying_in_process_ = false;
                batch_in_process_ = false;
                // Batch completion will be handled by UI notification
            }
        }
    }

    void apply_valve_configuration(const SystemConfig &config) {
        for (map<string, string>::const_iterator it = config.valve_states.begin(); 
             it != config.valve_states.end(); ++it) {
            const string& valve_name = it->first;
            const string& valve_state = it->second;
            bool should_be_open = (valve_state == "OPEN");
                // TODO: Consider a more robust way to map valve names to components if more valves are added.
                // For now, direct mapping is used.
            if (valve_name == "V201") {
                get_pump_line_mutable("P201").get_enter_valve_mutable().set_open(should_be_open);
            } else if (valve_name == "V202") {
                get_pump_line_mutable("P202").get_enter_valve_mutable().set_open(should_be_open);
            } else if (valve_name == "V203") {
                get_pump_line_mutable("P203").get_enter_valve_mutable().set_open(should_be_open);
            } else if (valve_name == "V401") {
                get_pump_line_mutable("P201").get_exit_valve_mutable().set_open(should_be_open);
            } else if (valve_name == "V402") {
                get_pump_line_mutable("P202").get_exit_valve_mutable().set_open(should_be_open);
            } else if (valve_name == "V403") {
                get_pump_line_mutable("P203").get_exit_valve_mutable().set_open(should_be_open);
            }
        }
    }

    bool can_start_batch(const string &target_color) const {
        // Check if required valves are open for the target color
        map<string, map<string, double> > color_recipes = SystemConstants::create_color_recipes();
        map<string, map<string, double> >::const_iterator color_recipe = color_recipes.find(target_color);
        if (color_recipe == color_recipes.end()) {
            return false;
        }

        // Check mixer tank low level switch
        if (!mixer_tank_.get_low_level_switch().is_alarm()) {
            return false; // Tank must be empty to start
        }

        // Check if required pump lines have both valves open
        for (map<string, PumpLine>::const_iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            const PumpLine& pump_line = it->second;
            const string& tank_liquid = pump_line.get_tank().get_liquid_in_tank_name();
            
            // Check if this liquid is needed for the recipe
            bool liquid_needed = color_recipe->second.find(tank_liquid) != color_recipe->second.end();
            
            if (liquid_needed) {
                // Both enter and exit valves must be open for required liquids
                if (!pump_line.get_enter_valve().is_open() || !pump_line.get_exit_valve().is_open()) {
                    return false;
                }
            }
        }
        
        return true;
    }

    bool can_start_mixing() const {
        // Complete check to avoid premature mixing before all base colors are fully pumped
        // This method enforces that mixing cannot start while pumps are in paused states
        
        for (map<string, PumpLine>::const_iterator it = pump_lines_.begin(); 
             it != pump_lines_.end(); ++it) {
            const PumpLine& pump_line = it->second;
            const LiquidPump& pump = pump_line.get_pump();
            
            // Skip pumps with no target (target duration = 0) - not required for this batch
            if (pump.get_target_duration() <= 0) {
                continue;
            }
            
            // CRITICAL CHECK: Ensure target time > 0 and elapsed time >= target time
            // This prevents mixing when pumps have not reached their pumping goals
            if (pump.get_elapsed_seconds() < pump.get_target_duration()) {
                return false; // This pump hasn't finished pumping its required amount
            }
            
            // CRITICAL CHECK: Ensure pumps are not just paused but actually completed
            // Pumps in STOPPED_FLOW_ALARM, STOPPED_HIGH_PRESSURE, or STOPPED_LOW_PRESSURE
            // are paused and may restart - mixing must wait for them to complete or be permanently stopped
            PumpState state = pump.get_state();
            if (state == STOPPED_FLOW_ALARM || 
                state == STOPPED_HIGH_PRESSURE || 
                state == STOPPED_LOW_PRESSURE) {
                // Pump is paused but not permanently stopped - could potentially restart
                // Check if pump can still continue (valves open and has remaining target time)
                if (pump_line.get_enter_valve().is_open() && 
                    pump_line.get_exit_valve().is_open()) {
                    return false; // Pump could restart, don't mix yet
                }
                // If valves are closed, pump is effectively permanently stopped
                // Continue checking other pumps
            }
            
            // CRITICAL CHECK: Running pumps must not be interrupted by mixing
            if (state == RUNNING) {
                return false; // Pump is still actively pumping
            }
            
            // At this point, pump has either:
            // 1. Reached target (STOPPED_TARGET_REACHED) - OK to mix
            // 2. Is permanently unable to continue due to closed valves - OK to mix
        }
        
        // All checks passed - safe to start mixing
        return true;
    }

    // ...existing code...
};

class UserInterface {
  private:
    bool last_batch_in_process_;
    
    void clear_screen() { system("cls"); }

  public:
    UserInterface() : last_batch_in_process_(false) {}
    
    void clear_display() { clear_screen(); }

    void show_simulation_status(const Factory &factory,
                                const SystemConfig &config) {
        clear_screen();
        
        // Check if batch just completed
        if (last_batch_in_process_ && !factory.is_batch_in_process() && 
            !factory.is_emptying_in_process()) {
            cout << "*** LOTE COMPLETADO EXITOSAMENTE ***" << endl;
            cout << "El lote de " << config.color_a_mezclar << " ha sido completado." << endl;
            cout << "El mezclador ha sido vaciado y esta listo para un nuevo lote." << endl;
            cout << "Presione Enter para continuar..." << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            clear_screen();
        }
        
        // Update tracking variable
        last_batch_in_process_ = factory.is_batch_in_process();
        
        cout << "=== Sistema de Mezcla de Pintura Dupont ===" << endl;
        cout << "Color a mezclar: " << config.color_a_mezclar << endl;
        cout << "Estado de fabricacion: " << config.arranque_de_fabricacion
             << endl;
        cout << "Lote en proceso: "
             << (factory.is_batch_in_process() ? "SI" : "NO") << endl;
        cout << "Vaciado en proceso: "
             << (factory.is_emptying_in_process() ? "SI" : "NO") << endl;
        
        // Show current batch phase
        if (factory.is_batch_in_process()) {
            cout << "Fase actual: ";
            if (!factory.all_required_pumps_completed()) {
                cout << "BOMBEANDO" << endl;
            } else if (factory.get_mixer_tank().get_mixer_motor().is_running()) {
                cout << "MEZCLANDO" << endl;
            } else if (factory.is_emptying_in_process()) {
                cout << "VACIANDO" << endl;
            } else {
                cout << "COMPLETANDO..." << endl;
            }
        }
        cout << endl;

        cout << "=== Estado de las Lineas de Bombeo ===" << endl;
        const map<string, PumpLine>& pump_lines = factory.get_all_pump_lines();
        for (map<string, PumpLine>::const_iterator it = pump_lines.begin(); 
             it != pump_lines.end(); ++it) {
            const PumpLine& pump_line = it->second;
            show_pump_line_status(pump_line);
        }

        cout << "=== Estado de Valvulas ===" << endl;
        show_valve_status(factory, config);

        cout << "=== Estado del Mezclador ===" << endl;
        show_mixer_status(factory.get_mixer_tank());
    }

  private:
    void show_pump_line_status(const PumpLine &pump_line) {
        const LiquidPump& pump = pump_line.get_pump();
        const LiquidTank& tank = pump_line.get_tank();
        const PressureTransmitter& pressure = pump_line.get_pressure_transmitter();

        cout << "Bomba " << pump.get_code() << " ("
             << tank.get_liquid_in_tank_name() << "):" << endl;
        cout << "  Estado: " << (pump.is_on() ? "ENCENDIDA" : "APAGADA")
             << endl;
        cout << "  Tiempo transcurrido: " << pump.get_elapsed_seconds() << "s"
             << endl;
        cout << "  Tiempo objetivo: " << pump.get_target_duration() << "s"
             << endl;
        cout << "  Nivel tanque: " << tank.get_level() << "%" << endl;
        cout << "  Presion: " << pressure.read_pressure() << " psi" << endl;
        const FlowSwitch& flow_switch = pump_line.get_flow_switch(); // Get the flow switch
        cout << "  Flujo Switch " << flow_switch.get_code() << ": "
             << (flow_switch.is_normal() ? "NORMAL" : "ALARMA") << endl;
        cout << endl;
    }

    void show_valve_status(const Factory &factory, const SystemConfig &config) {
        // Display valve states from configuration and actual valve states
        for (map<string, string>::const_iterator it = config.valve_states.begin(); 
             it != config.valve_states.end(); ++it) {
            const string& valve_name = it->first;
            const string& config_state = it->second;
            cout << "Valvula " << valve_name << ": ";
            cout << "Config=" << config_state;
            
            // Show actual valve state
            bool actual_state = false;
            if (valve_name == "V201") {
                actual_state = factory.get_pump_line("P201").get_enter_valve().is_open();
            } else if (valve_name == "V202") {
                actual_state = factory.get_pump_line("P202").get_enter_valve().is_open();
            } else if (valve_name == "V203") {
                actual_state = factory.get_pump_line("P203").get_enter_valve().is_open();
            } else if (valve_name == "V401") {
                actual_state = factory.get_pump_line("P201").get_exit_valve().is_open();
            } else if (valve_name == "V402") {
                actual_state = factory.get_pump_line("P202").get_exit_valve().is_open();
            } else if (valve_name == "V403") {
                actual_state = factory.get_pump_line("P203").get_exit_valve().is_open();
            }
            
            cout << ", Estado=" << (actual_state ? "ABIERTA" : "CERRADA") << endl;
        }
        cout << endl;
    }

    void show_mixer_status(const MixerTank &mixer_tank) {
        const MixerMotor& mixer_motor = mixer_tank.get_mixer_motor();

        cout << "Mezclador " << mixer_tank.get_code() << ":" << endl;
        cout << "  Nivel: " << mixer_tank.get_level() << "%" << endl;
        cout << "  Capacidad actual: " << mixer_tank.get_current_capacity()
             << " litros" << endl;
        cout << "  Motor: "
             << (mixer_motor.is_running() ? "MEZCLANDO" : "DETENIDO") << endl;
        cout << "  Tiempo de mezcla transcurrido: "
             << mixer_motor.get_elapsed_time() << "s" << endl;
        cout << "  Tiempo objetivo de mezcla: " << mixer_motor.get_target_time()
             << "s" << endl;
        cout << "  Estado de vaciado: "
             << (mixer_tank.is_emptying() ? "VACIANDO" : "DETENIDO") << endl;
        cout << "  Tiempo de vaciado transcurrido: "
             << mixer_tank.get_emptying_elapsed_time() << "s" << endl;
        cout << "  Interruptor bajo nivel: "
             << (mixer_tank.get_low_level_switch().is_alarm() ? "ALARMA"
                                                              : "NORMAL")
             << endl;
        cout << endl;
    }
};

class ConfigurationUI {
  private:
    UserInterface ui_;

    bool prompt_config_repair(const runtime_error &e) {
        ui_.clear_display();
        cerr << "Ha ocurrido un error leyendo la configuracion: " << e.what()
             << endl;
        cout << "Quieres que se corrija la configuracion con un archivo de "
                "configuracion con las configuraciones iniciales? "
                "Escribe 'si', o escribe 'no' para cerrar el programa."
             << endl;

        cout << "Tu opcion: ";
        string user_choice;
        cin >> user_choice;

        if (user_choice != "si") {
            cout << "El usuario eligio no reparar la configuracion." << endl;
            return false;
        }
        return true;
    }

    void show_config_repair_success() {
        cout << "El archivo de configuracion ha sido escrito en: "
             << SystemConstants::CONFIG_FILE_PATH << endl;
        cout << "Por favor, ajusta la configuracion segun sea necesario."
             << endl;
        cout << "Cuando hayas terminado, presiona Enter para continuar..."
             << endl;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    }

    void show_config_repair_error(const exception &ex) {
        cerr << "Error en reparar la configuracion: " << ex.what() << endl;
    }

    bool attempt_config_repair() {
        try {
            ConfigManager::repair_or_create_config_file();
            show_config_repair_success();
            return true;
        } catch (const exception &ex) {
            show_config_repair_error(ex);
            return false;
        }
    }

    bool handle_config_error(const runtime_error &e) {
        if (!prompt_config_repair(e)) {
            return false;
        }
        return attempt_config_repair();
    }

  public:
    SystemConfig handle_config_loading() {
        while (true) {
            try {
                return ConfigManager::read_config();
            } catch (const runtime_error &e) {
                if (!handle_config_error(e)) {
                    throw runtime_error(
                        "La configuracion no pudo ser corregida con la "
                        "herramienta de reparacion. El programa se cerrara.");
                }
            }
        }
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);

    try {
        bool is_running = true;
        SystemConfig user_config;
        string previous_arranque_state = "OFF";
        string previous_color = ""; // Track previous color to detect changes

        Factory factory = Factory::create_dupont_paint_factory();

        ConfigurationUI config_ui;
        UserInterface main_ui;

        while (is_running) {
            try {
                user_config = config_ui.handle_config_loading();
            } catch (const runtime_error &e) {
                cerr << "Error critico durante el manejo del archivo de "
                        "configuracion: "
                     << e.what() << endl;
                system("pause");
                return 1;
            }

            main_ui.show_simulation_status(factory, user_config);

            // Apply valve configuration from config file
            factory.apply_valve_configuration(user_config);

            // Check for color change when not in batch process
            bool color_changed = (previous_color != user_config.color_a_mezclar);
            if (color_changed && !factory.is_batch_in_process()) {
                // Update pump times immediately when color changes (but not during batch)
                factory.set_pump_times(user_config.color_a_mezclar);
            }

            // Check for batch start command (OFF to ON transition)
            bool start_command_triggered = (previous_arranque_state == "OFF" && 
                                           user_config.arranque_de_fabricacion == "ON");

            if (!factory.is_batch_in_process()) {
                if (start_command_triggered && factory.get_mixer_tank().get_low_level_switch().is_alarm()) {
                    factory.set_batch_in_process();
                    factory.reset();
                    factory.set_pump_times(user_config.color_a_mezclar);
                } else if (start_command_triggered) { // Else if start was triggered but low level switch is NOT alarm
                    // Add message to user
                    cout << "ADVERTENCIA: No se puede iniciar un nuevo lote." << endl;
                    cout << "El interruptor de bajo nivel del mezclador NO esta en alarma (el tanque no esta lo suficientemente vacio)." << endl;
                    cout << "Presione Enter para continuar..." << endl;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cin.get();
                }
            } else {
                // Batch is currently running
                if (start_command_triggered) {
                    cout << "ADVERTENCIA: No se puede iniciar un nuevo lote." << endl;
                    cout << "Espere a que termine el lote actual antes de iniciar uno nuevo." << endl;
                    cout << "Estado actual: ";
                    if (!factory.all_required_pumps_completed()) {
                        cout << "Bombeando liquidos..." << endl;
                    } else if (factory.get_mixer_tank().get_mixer_motor().is_running()) {
                        cout << "Mezclando..." << endl;
                    } else if (factory.is_emptying_in_process()) {
                        cout << "Vaciando mezclador..." << endl;
                    }
                    cout << "Presione Enter para continuar..." << endl;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cin.get();
                }
                
                if (!factory.all_required_pumps_completed()) {
                    factory.update_all_pump_lines();
                }
            }

            previous_arranque_state = user_config.arranque_de_fabricacion; // Update previous state
            previous_color = user_config.color_a_mezclar; // Update previous color
            
            factory.update_mix(); // Update mixing process
            factory.update_emptying(); // Update emptying process
            
            Sleep(SystemConstants::ONE_SECOND_IN_MS); // Simulation delay
        }
    } catch (const exception &e) {
        cerr << "Error critico en el programa: " << e.what() << endl;
        return 1;
    }

    return 0;
}
