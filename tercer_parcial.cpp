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

using namespace std;

namespace SystemConstants {
const bool NORMAL_STATUS = true;
const bool ALARM_STATUS = false;
const int ONE_SECOND_IN_MS = 1000;
const double INITIAL_TANK_CAPACITY = 20000.0;
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
const map<string, map<string, double>> COLOR_RECIPES = {
    {"AzMarino", {{"Azul", 2.0 / 3.0}, {"Negro", 1.0 / 3.0}}}, // Reverted proportions
    {"AzCeleste",
     {{"Azul", 1.0 / 3.0}, {"Negro", 1.0 / 3.0}, {"Blanco", 1.0 / 3.0}}}};
} // namespace SystemConstants

struct SystemConfig {
    map<string, string> valve_states;
    string color_a_mezclar;
    string arranque_de_fabricacion;
};

class StringUtils {
  public:
    static string trim_whitespace(const string &s) {
        auto start = s.find_first_not_of(" \t\r\n");
        auto end = s.find_last_not_of(" \t\r\n");
        if (start == string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }
};

class ConfigFileHandler {
  public:
    static void open_config_file(ifstream &file, const string &filename) {
        file.open(filename);
        if (!file.is_open()) {
            throw runtime_error("Error: Could not open config file: " +
                                filename);
        }
    }

    static bool
    parse_config_line(const string &line, string &key, string &value) {
        auto eq_pos = line.find('=');
        if (eq_pos == string::npos)
            return false;
        key = StringUtils::trim_whitespace(line.substr(0, eq_pos));
        value = StringUtils::trim_whitespace(line.substr(eq_pos + 1));
        return !(key.empty() || value.empty());
    }

    static void create_default_config_file(
        const string &filename = SystemConstants::CONFIG_FILE_PATH) {
        const char *default_config =
            "# Válvulas de entrada \n"
            "# Valores Posibles: OPEN / CLOSE\n"
            "V201 = OPEN\n"
            "V202 = OPEN\n"
            "V203 = OPEN\n"
            "\n"
            "# Válvulas de salida\n"
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
            "ARRANQUE_DE_FABRICACIÓN = OFF\n";

        ofstream out(filename, ios::trunc);
        if (!out) {
            throw runtime_error("Could not write default config file: " +
                                filename);
        }
        out << default_config;
        out.close();
    }
};

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
                "Missing required setting: ARRANQUE_DE_FABRICACIÓN");
        }

        for (const auto &valve : KNOWN_VALVE_KEYS) {
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
            throw runtime_error("Invalid value for ARRANQUE_DE_FABRICACIÓN: " +
                                value);
        }
    }

    static void validate_valve_value(const string &key, const string &value) {
        if (value != "OPEN" && value != "CLOSED") {
            throw runtime_error("Invalid value for valve " + key + ": " +
                                value);
        }
    }

    static bool is_known_valve(const string &key) {
        return find(KNOWN_VALVE_KEYS.begin(), KNOWN_VALVE_KEYS.end(), key) !=
               KNOWN_VALVE_KEYS.end();
    }
};

const vector<string> ConfigValidator::KNOWN_VALVE_KEYS = {
    "V201", "V202", "V203", "V401", "V402", "V403"};
const string ConfigValidator::K_COLOR = "COLOR_A_MEZCLAR";
const string ConfigValidator::K_ARRANQUE = "ARRANQUE_DE_FABRICACIÓN";

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

    void evaluate_status(double flow_rate) {
        status_ = (flow_rate > 0) ? SystemConstants::NORMAL_STATUS
                                  : SystemConstants::ALARM_STATUS;
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

enum class PumpState {
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
        current_state_ = PumpState::RUNNING;
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

  public:
    explicit LiquidPump(const string &code,
                        double flow_rate = SystemConstants::DEFAULT_FLOW_RATE)
        : code_(code), flow_rate_lts_min_(flow_rate),
          target_pump_duration_seconds_(0.0), pump_elapsed_seconds_(0.0),
          is_on_(SystemConstants::INITIAL_PUMP_STATE),
          current_state_(PumpState::STOPPED_LOW_PRESSURE) {}

    bool is_on() const { return is_on_; }
    double get_flow_rate() const { return flow_rate_lts_min_; }
    const string &get_code() const { return code_; }
    PumpState get_state() const { return current_state_; }
    double get_elapsed_seconds() const { return pump_elapsed_seconds_; }
    double get_target_duration() const { return target_pump_duration_seconds_; }

    void update_pump_state(const FlowSwitch &flow_switch,
                           double current_pressure) {
        if (should_stop_for_alarm(flow_switch)) {
            stop(PumpState::STOPPED_FLOW_ALARM);
            return;
        }

        if (should_stop_for_high_pressure(current_pressure)) {
            stop(PumpState::STOPPED_HIGH_PRESSURE);
            return;
        }

        if (should_stop_for_target_reached()) {
            stop(PumpState::STOPPED_TARGET_REACHED);
            return;
        }

        if (!is_on_ && can_start_for_pressure(current_pressure)) {
            start();
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
                pressure_ = SystemConstants::NORMAL_OPERATING_PRESSURE;
            } else if (!exit_valve.is_open()) { // Exit valve closed, pump trying to run
                pressure_ += SystemConstants::PRESSURE_INCREMENT * 2; // Pressure builds
            } else { // Enter valve closed or other issue, pump trying to run
                pressure_ = SystemConstants::INITIAL_PRESSURE; // No inlet, pressure should be low/zero at pump
            }
        } else {
            // --- Pump is OFF ---
            if (exit_valve.is_open()) {
                // If pressure was high and pump just stopped, it should decrease.
                if (pressure_ > SystemConstants::LOW_PRESSURE_THRESHOLD) {
                    pressure_ = SystemConstants::LOW_PRESSURE_THRESHOLD - 1.0; // Force it below restart threshold
                } else if (pressure_ > SystemConstants::INITIAL_PRESSURE) {
                     pressure_ -= SystemConstants::PRESSURE_INCREMENT; // Gradual decay to 0
                }
                 if (pressure_ < SystemConstants::INITIAL_PRESSURE) {
                    pressure_ = SystemConstants::INITIAL_PRESSURE;
                }
            }
            // If exit_valve is closed and pump is off, pressure should ideally remain.
            // No specific change needed here for that case as pressure_ won't be modified by this block.
        }

        if (pressure_ < SystemConstants::INITIAL_PRESSURE) { // Ensure pressure doesn't go below initial (usually 0)
            pressure_ = SystemConstants::INITIAL_PRESSURE;
        }
        // Max pressure clamp can also be considered if pressure_ can exceed a physical limit
        // For now, high pressure stop is handled by the pump logic.
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
        double physical_flow = 0.0;
        if (pump_.is_on() && enter_valve_.is_open() && exit_valve_.is_open()) {
            physical_flow = pump_.get_flow_rate();
        }
        flow_switch_.evaluate_status(physical_flow); // Use physical_flow

        // Order of updates matters:
        // 1. Update pressure based on current valve/pump states (BEFORE pump state changes for this cycle)
        pressure_transmitter_.update_pressure(enter_valve_, exit_valve_, pump_);
        // 2. Update pump state based on flow and new pressure
        pump_.update_pump_state(flow_switch_, pressure_transmitter_.read_pressure());

        // 3. Increment time if pump is (still) on after state update
        if (pump_.is_on()) {
            pump_.increment_elapsed_time(
                SystemConstants::ONE_SECOND_IN_MS /
                static_cast<double>(SystemConstants::ONE_SECOND_IN_MS));
        }
    }

    bool need_to_pump() const {
        return pump_.get_elapsed_seconds() < pump_.get_target_duration();
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
              double max_capacity = SystemConstants::INITIAL_TANK_CAPACITY,
              double initial_capacity = 0.0)
        : level_transmitter_(level_transmitter_code),
          low_level_switch_(code, SystemConstants::ALARM_STATUS),
          mixer_motor_(code), current_capacity_(initial_capacity),
          max_capacity_(max_capacity), code_(code) {
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
                      SystemConstants::INITIAL_TANK_CAPACITY,
                      SystemConstants::INITIAL_MIXER_TANK_LEVEL) {
        if (pump_lines.empty()) {
            throw runtime_error("Factory must have at least one pump line");
        }
        for (const auto &line : pump_lines) {
            pump_lines_.emplace(line.get_pump().get_code(), line);
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

    void set_batch_in_process() {
        if (!batch_in_process_) {
            batch_in_process_ = true;
        };
    }

    const PumpLine &get_pump_line(const string &pump_code) const {
        auto it = pump_lines_.find(pump_code);
        if (it == pump_lines_.end()) {
            throw runtime_error("Pump line not found: " + pump_code);
        }
        return it->second;
    }

    PumpLine &get_pump_line_mutable(const string &pump_code) {
        auto it = pump_lines_.find(pump_code);
        if (it == pump_lines_.end()) {
            throw runtime_error("Pump line not found: " + pump_code);
        }
        return it->second;
    }

    const map<string, PumpLine> &get_all_pump_lines() const {
        return pump_lines_;
    }

    void transfer_liquid_to_mixer(double seconds = 1.0) {
        for (auto &[code, pump_line] : pump_lines_) {
            auto &pump = pump_line.get_pump_mutable();
            auto &tank = pump_line.get_tank_mutable();
            // Check pump state AND valve states for actual liquid transfer
            if (pump.is_on() &&
                pump_line.get_enter_valve().is_open() && // Check inlet valve
                pump_line.get_exit_valve().is_open() &&  // Check outlet valve
                (pump.get_elapsed_seconds() < pump.get_target_duration())) {
                double flow_rate = pump.get_flow_rate(); // lts/min
                double liters_this_cycle = flow_rate * (seconds / 60.0); // Assuming 'seconds' is the time step for the simulation cycle
                double drained = tank.drain(liters_this_cycle);
                mixer_tank_.add_liquid(drained);
            }
        }
    }

    void update_all_pump_lines() {
        for (auto &[code, pump_line] : pump_lines_) {
            pump_line.update_system_state();
        }

        transfer_liquid_to_mixer();
    }

    void set_pump_times(const std::string &target_color) {
        if (pump_lines_.empty()) {
            throw runtime_error("No pump lines available to set times");
        }

        auto color_recipe = SystemConstants::COLOR_RECIPES.find(target_color);

        // for to get the pump in each pumpline, compare if the color content of
        // the line is one of the targeted and if is, set the time with the
        // proportions

        for (auto &[code, pump_line] : pump_lines_) {
            const auto &pump = pump_line.get_pump();
            const auto &tank = pump_line.get_tank();

            if (color_recipe != SystemConstants::COLOR_RECIPES.end()) {
                const auto &recipe = color_recipe->second;
                auto it = recipe.find(tank.get_liquid_in_tank_name());
                if (it != recipe.end()) {
                    double target_liters =
                        SystemConstants::BATCH_SIZE * it->second;
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
        for (auto &[code, pump_line] : pump_lines_) {
            pump_line.get_pump_mutable().set_pump_target_liters(0);
            pump_line.get_pump_mutable().increment_elapsed_time(
                -pump_line.get_pump().get_elapsed_seconds());
            pump_line.get_enter_valve_mutable().set_open(true);
            pump_line.get_exit_valve_mutable().set_open(true);
        }
        emptying_in_process_ = false;
        mixer_tank_.get_mixer_motor_mutable().stop(); // Reset mixer motor
    }

    bool pump_lines_need_to_pump() const {
        for (const auto &[code, pump_line] : pump_lines_) {
            if (pump_line.need_to_pump()) {
                return true;
            }
        }
        return false;
    }

    void update_mix() {
        // Check if we should start mixing: all pumps finished and mixer not running
        if (!pump_lines_need_to_pump() && !mixer_tank_.get_mixer_motor().is_running()) {
            // Start mixing if there's liquid in the tank and batch is in process
            if (mixer_tank_.get_current_capacity() > 0 && batch_in_process_) {
                mixer_tank_.get_mixer_motor_mutable().start();
            }
        }
        
        // If mixer is running, update its progress (advance one second)
        if (mixer_tank_.get_mixer_motor().is_running()) {
            mixer_tank_.get_mixer_motor_mutable().update_mixing_progress(1.0);
            
            // Check if mixing is complete - start emptying phase
            if (!mixer_tank_.get_mixer_motor().is_running()) {
                emptying_in_process_ = true;
            }
        }
    }

    void update_emptying() {
        if (emptying_in_process_) {
            mixer_tank_.empty_tank(4.0); // 4% per second as specified
            
            // Check if tank is empty - finish batch process
            if (mixer_tank_.is_empty()) {
                emptying_in_process_ = false;
                batch_in_process_ = false;
            }
        }
    }

    void apply_valve_configuration(const SystemConfig &config) {
        for (const auto &[valve_name, valve_state] : config.valve_states) {
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
        auto color_recipe = SystemConstants::COLOR_RECIPES.find(target_color);
        if (color_recipe == SystemConstants::COLOR_RECIPES.end()) {
            return false;
        }

        // Check mixer tank low level switch
        if (!mixer_tank_.get_low_level_switch().is_alarm()) {
            return false; // Tank must be empty to start
        }

        // Check if required pump lines have both valves open
        for (const auto &[code, pump_line] : pump_lines_) {
            const auto &tank_liquid = pump_line.get_tank().get_liquid_in_tank_name();
            
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
};

class UserInterface {
  private:
    void clear_screen() { system("cls"); }

  public:
    void clear_display() { clear_screen(); }

    void show_simulation_status(const Factory &factory,
                                const SystemConfig &config) {
        clear_screen();
        cout << "=== Sistema de Mezcla de Pintura Dupont ===" << endl;
        cout << "Color a mezclar: " << config.color_a_mezclar << endl;
        cout << "Estado de fabricación: " << config.arranque_de_fabricacion
             << endl;
        cout << "Lote en proceso: "
             << (factory.is_batch_in_process() ? "SÍ" : "NO") << endl;
        cout << "Vaciado en proceso: "
             << (factory.is_emptying_in_process() ? "SÍ" : "NO") << endl;
        
        // Show current batch phase
        if (factory.is_batch_in_process()) {
            cout << "Fase actual: ";
            if (factory.pump_lines_need_to_pump()) {
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

        cout << "=== Estado de las Líneas de Bombeo ===" << endl;
        for (const auto &[code, pump_line] : factory.get_all_pump_lines()) {
            show_pump_line_status(pump_line);
        }

        cout << "=== Estado de Válvulas ===" << endl;
        show_valve_status(factory, config);

        cout << "=== Estado del Mezclador ===" << endl;
        show_mixer_status(factory.get_mixer_tank());
    }

  private:
    void show_pump_line_status(const PumpLine &pump_line) {
        const auto &pump = pump_line.get_pump();
        const auto &tank = pump_line.get_tank();
        const auto &pressure = pump_line.get_pressure_transmitter();

        cout << "Bomba " << pump.get_code() << " ("
             << tank.get_liquid_in_tank_name() << "):" << endl;
        cout << "  Estado: " << (pump.is_on() ? "ENCENDIDA" : "APAGADA")
             << endl;
        cout << "  Tiempo transcurrido: " << pump.get_elapsed_seconds() << "s"
             << endl;
        cout << "  Tiempo objetivo: " << pump.get_target_duration() << "s"
             << endl;
        cout << "  Nivel tanque: " << tank.get_level() << "%" << endl;
        cout << "  Presión: " << pressure.read_pressure() << " psi" << endl;
        const auto &flow_switch = pump_line.get_flow_switch(); // Get the flow switch
        cout << "  Flujo Switch " << flow_switch.get_code() << ": "
             << (flow_switch.is_normal() ? "NORMAL" : "ALARMA") << endl;
        cout << endl;
    }

    void show_valve_status(const Factory &factory, const SystemConfig &config) {
        // Display valve states from configuration and actual valve states
        for (const auto &[valve_name, config_state] : config.valve_states) {
            cout << "Válvula " << valve_name << ": ";
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
        const auto &mixer_motor = mixer_tank.get_mixer_motor();

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
        cerr << "Ha ocurrido un error leyendo la configuración: " << e.what()
             << endl;
        cout << "¿Quieres que se corrija la configuración con un archivo de "
                "configuración con las configuraciones iniciales? "
                "Escribe 'si', o escribe 'no' para cerrar el programa."
             << endl;

        cout << "Tu opción: ";
        string user_choice;
        cin >> user_choice;

        if (user_choice != "si") {
            cout << "El usuario eligió no reparar la configuración." << endl;
            return false;
        }
        return true;
    }

    void show_config_repair_success() {
        cout << "El archivo de configuración ha sido escrito en: "
             << SystemConstants::CONFIG_FILE_PATH << endl;
        cout << "Por favor, ajusta la configuración según sea necesario."
             << endl;
        cout << "Cuando hayas terminado, presiona Enter para continuar..."
             << endl;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    }

    void show_config_repair_error(const exception &ex) {
        cerr << "Error en reparar la configuración: " << ex.what() << endl;
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
                        "La configuración no pudo ser corregida con la "
                        "herramienta de reparación. El programa se cerrará.");
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

        Factory factory = Factory::create_dupont_paint_factory();

        ConfigurationUI config_ui;
        UserInterface main_ui;

        while (is_running) {
            try {
                user_config = config_ui.handle_config_loading();
            } catch (const runtime_error &e) {
                cerr << "Error crítico durante el manejo del archivo de "
                        "configuración: "
                     << e.what() << endl;
                system("pause");
                return 1;
            }

            main_ui.show_simulation_status(factory, user_config);

            // Apply valve configuration from config file
            factory.apply_valve_configuration(user_config);

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
                    cout << "El interruptor de bajo nivel del mezclador NO está en alarma (el tanque no está lo suficientemente vacío)." << endl;
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
                    if (factory.pump_lines_need_to_pump()) {
                        cout << "Bombeando líquidos..." << endl;
                    } else if (factory.get_mixer_tank().get_mixer_motor().is_running()) {
                        cout << "Mezclando..." << endl;
                    } else if (factory.is_emptying_in_process()) {
                        cout << "Vaciando mezclador..." << endl;
                    }
                    cout << "Presione Enter para continuar..." << endl;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cin.get();
                }
                
                if (factory.pump_lines_need_to_pump()) {
                    factory.update_all_pump_lines();
                }
            }

            previous_arranque_state = user_config.arranque_de_fabricacion; // Update previous state
            
            factory.update_mix(); // Update mixing process
            factory.update_emptying(); // Update emptying process
            
            Sleep(SystemConstants::ONE_SECOND_IN_MS); // Simulation delay
        }
    } catch (const exception &e) {
        cerr << "Error crítico en el programa: " << e.what() << endl;
        return 1;
    }

    return 0;
}
