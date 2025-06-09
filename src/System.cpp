#include "System.h"
#include <stdexcept> // For std::invalid_argument, std::runtime_error
#include <iostream>  // For debugging, remove later
#include <numeric>   // For std::accumulate
#include <iomanip>   // For std::fixed and std::setprecision
#include <algorithm> // For std::min

// Helper to convert BasePaintType to string for logging/errors
std::string basePaintTypeToString(BasePaintType bpt) {
    if (bpt == BasePaintType::BLANCO) return "Blanco";
    if (bpt == BasePaintType::AZUL) return "Azul";
    if (bpt == BasePaintType::NEGRO) return "Negro";
    return "UnknownBasePaint";
}
std::string processStateToString(ProcessState ps) {
    if (ps == ProcessState::IDLE) return "IDLE";
    if (ps == ProcessState::PUMPING_BASE) return "PUMPING_BASE";
    if (ps == ProcessState::MIXING) return "MIXING";
    if (ps == ProcessState::EMPTYING) return "EMPTYING";
    if (ps == ProcessState::ERROR_STATE) return "ERROR_STATE";
    if (ps == ProcessState::WAITING_FOR_RECOVERY) return "WAITING_FOR_RECOVERY";
    return "UnknownProcessState";
}
std::string valveStatusToString(ValveStatus vs) {
    if (vs == ValveStatus::OPEN) return "OPEN";
    return "CLOSED";
}
std::string switchStatusToString(SwitchStatus ss) {
    if (ss == SwitchStatus::NORMAL) return "NORMAL";
    return "ALARM";
}
std::string pumpStatusToString(PumpStatus ps) {
    if (ps == PumpStatus::ON) return "ON";
    return "OFF";
}


System::System() :
    // Initialize Tanks
    tank_blanco("T201_Blanco", 1000.0, 250.0),
    tank_azul("T202_Azul", 1000.0, 250.0),
    tank_negro("T203_Negro", 1000.0, 250.0),
    tank_mixer_storage("M401_MixerTank", 200.0, 0.0),

    // Initialize Valves (All OPEN initially as per problem statement)
    v201_s("V201_S_Blanco_Suction", ValveStatus::OPEN),
    v201_d("V201_D_Blanco_Discharge", ValveStatus::OPEN),
    v202_s("V202_S_Azul_Suction", ValveStatus::OPEN),
    v202_d("V202_D_Azul_Discharge", ValveStatus::OPEN),
    v203_s("V203_S_Negro_Suction", ValveStatus::OPEN),
    v203_d("V203_D_Negro_Discharge", ValveStatus::OPEN),
    v401_drain("V401_Mixer_Drain", ValveStatus::OPEN),

    // Initialize Sensors
    pt201("PT201_Blanco", SensorType::PRESSURE_TRANSMITTER),
    fs201("FS201_Blanco", SensorType::FLOW_SWITCH),
    pt202("PT202_Azul", SensorType::PRESSURE_TRANSMITTER),
    fs202("FS202_Azul", SensorType::FLOW_SWITCH),
    pt203("PT203_Negro", SensorType::PRESSURE_TRANSMITTER),
    fs203("FS203_Negro", SensorType::FLOW_SWITCH),
    mixer_low_level_switch("LSL401_Mixer_LowLevel", SensorType::FLOW_SWITCH),

    // Initialize Pumps
    p201("P201_Blanco", v201_s, v201_d, pt201, fs201),
    p202("P202_Azul", v202_s, v202_d, pt202, fs202),
    p203("P203_Negro", v203_s, v203_d, pt203, fs203),

    // Initialize Mixer
    mixer("M401_Mixer", tank_mixer_storage, v401_drain),

    // Process Control Variables
    current_process_state(ProcessState::IDLE),
    selected_paint_type(PaintType::AZUL_CELESTE),
    start_command(OnOffStatus::OFF_COMMAND),
    batch_in_progress(false),
    total_target_batch_size_liters(150.0),
    current_pumping_paint(std::nullopt)
{
    initialize_components();
    update_recipes();
    log_message("System initialized. Initial state: IDLE, Paint: AZUL_CELESTE, StartCmd: OFF");
}

System::~System() {}

void System::log_message(const std::string& message) {
    system_logs.push_back(message);
    std::cout << "SYS_LOG: " << message << std::endl; // Keep cout for now
    last_error_message = message; // Update last_error_message as a general status/log display
}


void System::initialize_components() {
    pt201.set_pressure_psi(0.0);
    fs201.set_flow_status(SwitchStatus::NORMAL);
    pt202.set_pressure_psi(0.0);
    fs202.set_flow_status(SwitchStatus::NORMAL);
    pt203.set_pressure_psi(0.0);
    fs203.set_flow_status(SwitchStatus::NORMAL);
    mixer_low_level_switch.set_flow_status(SwitchStatus::ALARM);

    base_pumps_map[BasePaintType::BLANCO] = &p201;
    base_pumps_map[BasePaintType::AZUL]   = &p202;
    base_pumps_map[BasePaintType::NEGRO]  = &p203;

    base_tanks_map[BasePaintType::BLANCO] = &tank_blanco;
    base_tanks_map[BasePaintType::AZUL]   = &tank_azul;
    base_tanks_map[BasePaintType::NEGRO]  = &tank_negro;

    base_discharge_valves_map[BasePaintType::BLANCO] = &v201_d;
    base_discharge_valves_map[BasePaintType::AZUL]   = &v202_d;
    base_discharge_valves_map[BasePaintType::NEGRO]  = &v203_d;

    base_suction_valves_map[BasePaintType::BLANCO] = &v201_s;
    base_suction_valves_map[BasePaintType::AZUL]   = &v202_s;
    base_suction_valves_map[BasePaintType::NEGRO]  = &v203_s;

    for (auto const& [paint_type, pump_ptr] : base_pumps_map) {
        pump_task_requires_completion_map[paint_type] = false;
        pump_run_times_seconds[paint_type] = 0.0;
        current_batch_pumped_liters[paint_type] = 0.0;
    }
    current_batch_pumped_liters[BasePaintType::BLANCO];
    current_batch_pumped_liters[BasePaintType::AZUL];
    current_batch_pumped_liters[BasePaintType::NEGRO];

    // Populate controllable_valves_map
    // These keys must match the names used in the input.txt file
    controllable_valves_map["V201_D"] = &v201_d;
    controllable_valves_map["V202_D"] = &v202_d;
    controllable_valves_map["V203_D"] = &v203_d;
    controllable_valves_map["V401_DRAIN"] = &v401_drain;
    controllable_valves_map["V201_S"] = &v201_s;
    controllable_valves_map["V202_S"] = &v202_s;
    controllable_valves_map["V203_S"] = &v203_s;

    log_message("Components initialized and maps populated.");
}

void System::load_commands_from_file(const std::string& filepath) {
    log_message("Attempting to load commands from file: " + filepath);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        log_message("ERROR: Could not open command file: " + filepath);
        // Depending on requirements, this could be a critical error that stops the system
        // or just a warning if the system can run with defaults or previous settings.
        // For now, just log and return. The system will use its current/default settings.
        return;
    }

    std::string line;
    int line_num = 0;
    while (std::getline(file, line)) {
        line_num++;
        // Trim whitespace from the beginning and end of the line
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == '#') { // Skip empty lines and comments
            continue;
        }

        std::istringstream iss(line);
        std::string command_type;
        iss >> command_type;

        if (command_type == "COLOR") {
            std::string color_str;
            iss >> color_str;
            if (color_str == "AZUL_MARINO") {
                set_selected_paint_type(PaintType::AZUL_MARINO); // Already logs internally
            } else if (color_str == "AZUL_CELESTE") {
                set_selected_paint_type(PaintType::AZUL_CELESTE); // Already logs internally
            } else {
                log_message("ERROR (file " + filepath + ", line " + std::to_string(line_num) + "): Invalid paint color '" + color_str + "'. No change made.");
            }
        } else if (command_type == "START_COMMAND") {
            std::string status_str;
            iss >> status_str;
            if (status_str == "ON") {
                set_start_command(OnOffStatus::ON_COMMAND); // Already logs internally
            } else if (status_str == "OFF") {
                set_start_command(OnOffStatus::OFF_COMMAND); // Already logs internally
            } else {
                log_message("ERROR (file " + filepath + ", line " + std::to_string(line_num) + "): Invalid START_COMMAND status '" + status_str + "'. No change made.");
            }
        } else if (command_type == "VALVE") {
            std::string valve_name_from_file;
            std::string status_str_from_file;
            iss >> valve_name_from_file >> status_str_from_file;
            ValveStatus desired_status;

            if (status_str_from_file == "OPEN") {
                desired_status = ValveStatus::OPEN;
            } else if (status_str_from_file == "CLOSED") {
                desired_status = ValveStatus::CLOSED;
            } else {
                log_message("ERROR (file " + filepath + ", line " + std::to_string(line_num) + "): Invalid VALVE status '" + status_str_from_file + "' for valve " + valve_name_from_file + ". No change made.");
                continue;
            }

            if (controllable_valves_map.count(valve_name_from_file)) {
                Valve* valve_ptr = controllable_valves_map.at(valve_name_from_file);
                if (desired_status == ValveStatus::OPEN) valve_ptr->open();
                else valve_ptr->close();
                log_message("Set valve " + valve_name_from_file + " to " + status_str_from_file + " from file (current actual: " + valveStatusToString(valve_ptr->get_status()) + ").");
            } else {
                log_message("ERROR (file " + filepath + ", line " + std::to_string(line_num) + "): Unknown valve name '" + valve_name_from_file + "' in input file. No change made.");
            }
        } else {
            log_message("ERROR (file " + filepath + ", line " + std::to_string(line_num) + "): Unknown command type '" + command_type + "'.");
        }
    }
    file.close();
    log_message("Finished processing command file: " + filepath);
}


void System::update_recipes() {
    target_liters_for_paint_type.clear();
    std::string recipe_name;

    if (selected_paint_type == PaintType::AZUL_MARINO) {
        recipe_name = "Azul Marino";
        target_liters_for_paint_type[BasePaintType::NEGRO] = 100.0;
        target_liters_for_paint_type[BasePaintType::AZUL]  = 50.0;
        target_liters_for_paint_type[BasePaintType::BLANCO] = 0.0;
    } else if (selected_paint_type == PaintType::AZUL_CELESTE) {
        recipe_name = "Azul Celeste";
        target_liters_for_paint_type[BasePaintType::BLANCO] = 75.0;
        target_liters_for_paint_type[BasePaintType::AZUL]   = 75.0;
        target_liters_for_paint_type[BasePaintType::NEGRO]  = 0.0;
    } else {
        handle_error_state("Unknown paint type selected for recipe.");
        return;
    }

    double sum_liters = 0;
    for(const auto& pair : target_liters_for_paint_type){ sum_liters += pair.second; }
    if (abs(sum_liters - total_target_batch_size_liters) > 1e-6 && current_process_state != ProcessState::ERROR_STATE) { // Added abs and epsilon
        handle_error_state("Recipe for " + recipe_name + " sums to " + std::to_string(sum_liters) +
                           "L but target batch size is " + std::to_string(total_target_batch_size_liters) + "L.");
        return;
    }

    log_message("Recipes updated for: " + recipe_name + ". Targets (L) - Blanco: "
              + std::to_string(target_liters_for_paint_type[BasePaintType::BLANCO]) + ", Azul: "
              + std::to_string(target_liters_for_paint_type[BasePaintType::AZUL]) + ", Negro: "
              + std::to_string(target_liters_for_paint_type[BasePaintType::NEGRO]));
}

void System::set_start_command(OnOffStatus command) {
    log_message("System received Start/Stop Command: " + std::string(command == OnOffStatus::ON_COMMAND ? "ON" : "OFF"));
    start_command = command;
}

void System::set_selected_paint_type(PaintType type) {
    if (batch_in_progress) {
        log_message("Cannot change paint type while a batch is in progress.");
        return;
    }
    selected_paint_type = type;
    log_message("Selected paint type changed to: " + std::string(type == PaintType::AZUL_MARINO ? "Azul Marino" : "Azul Celeste"));
    update_recipes();
}

void System::process_valve_command(const std::string& valve_name, ValveStatus requested_status) {
    log_message("Processing valve command: " + valve_name + " -> " + valveStatusToString(requested_status));
    Valve* target_valve = nullptr;
    if (valve_name == "V201") target_valve = &v201_d;
    else if (valve_name == "V202") target_valve = &v202_d;
    else if (valve_name == "V203") target_valve = &v203_d;
    else if (valve_name == "V401") target_valve = &v401_drain;
    else if (valve_name == "V201_S") target_valve = &v201_s;
    else if (valve_name == "V202_S") target_valve = &v202_s;
    else if (valve_name == "V203_S") target_valve = &v203_s;
    else { log_message("WARNING: Unknown valve name in command: " + valve_name); return; }

    if (target_valve) {
        requested_status == ValveStatus::OPEN ? target_valve->open() : target_valve->close();
        log_message("Valve " + target_valve->get_name() + " set to " + valveStatusToString(target_valve->get_status()));
    }
}

bool System::can_start_new_batch() const {
    if (batch_in_progress) {
        // log_message("Attempted to start batch while one is in progress."); // Logged by caller if needed
        return false;
    }
    if (mixer_low_level_switch.get_flow_status() != SwitchStatus::ALARM) {
        return false;
    }
    // start_command is checked by the caller (System::update in IDLE state)
    if (current_process_state != ProcessState::IDLE && current_process_state != ProcessState::WAITING_FOR_RECOVERY) {
        return false;
    }
    for(const auto& [paint, target_L] : target_liters_for_paint_type){
        if (target_L > 1e-6 && (!base_tanks_map.count(paint) || base_tanks_map.at(paint)->get_current_level_liters() < target_L)) {
            // Caller (update loop) should use this failure to set last_error_message.
            // log_message("Insufficient base material for " + basePaintTypeToString(paint) + ". Cannot start batch.");
            return false;
        }
    }
    return true;
}

void System::start_new_batch() {
    log_message("Starting new batch for paint type: " + std::string(selected_paint_type == PaintType::AZUL_MARINO ? "Azul Marino" : "Azul Celeste"));
    batch_in_progress = true;
    current_process_state = ProcessState::PUMPING_BASE;
    last_error_message = "";

    for (auto const& pair : base_pumps_map) { // Iterate map to ensure all paint types covered
        BasePaintType paint_type = pair.first;
        current_batch_pumped_liters[paint_type] = 0.0;
        pump_task_requires_completion_map[paint_type] = false;
        pump_run_times_seconds[paint_type] = 0.0;
    }
    current_pumping_paint = std::nullopt;
    update_recipes();
}

std::optional<BasePaintType> System::get_next_paint_to_pump() const {
    const BasePaintType order[] = {BasePaintType::BLANCO, BasePaintType::AZUL, BasePaintType::NEGRO};
    for (BasePaintType paint : order) {
        if (target_liters_for_paint_type.count(paint) && target_liters_for_paint_type.at(paint) > 1e-6) {
            if (current_batch_pumped_liters.at(paint) < target_liters_for_paint_type.at(paint) - 1e-6 &&
                (!pump_task_requires_completion_map.count(paint) || !pump_task_requires_completion_map.at(paint))) {
                return paint;
            }
        }
    }
    return std::nullopt;
}

void System::check_component_failures() {
    for (auto const& [paint_type, pump_ptr] : base_pumps_map) {
        if (pump_ptr->is_stopped_due_to_low_flow() || pump_ptr->is_stopped_due_to_overpressure()) {
            if (current_pumping_paint.has_value() && current_pumping_paint.value() == paint_type && !pump_task_requires_completion_map.at(paint_type)) {
                log_message("PUMP FAIL: Pump " + pump_ptr->get_name() + " for " + basePaintTypeToString(paint_type)
                          + " failed during its operation. Marking for recovery.");
                pump_task_requires_completion_map[paint_type] = true;
                pump_ptr->stop();
                current_pumping_paint = std::nullopt;
            } else if (pump_ptr->get_status() == PumpStatus::ON) {
                 log_message("PUMP ALERT: Pump " + pump_ptr->get_name() + " for " + basePaintTypeToString(paint_type)
                           + " is in fault but was not the designated active pump or already marked. Ensuring it's stopped.");
                 pump_ptr->stop();
            }
        }
    }
}

void System::handle_pumping_state(double time_delta_seconds) {
    // Req: "Si durante el proceso de fabricación de un lote se coloca en OFF el comando de arranque,
    // el proceso de fabricación en curso deberá completarse."
    // So, start_command == OFF does NOT stop pumping. It only prevents new batches.

    check_component_failures();

    if (!current_pumping_paint.has_value() ||
        (target_liters_for_paint_type.count(current_pumping_paint.value()) &&
         current_batch_pumped_liters.at(current_pumping_paint.value()) >= target_liters_for_paint_type.at(current_pumping_paint.value()) - 1e-6 && // Epsilon check
         (!pump_task_requires_completion_map.count(current_pumping_paint.value()) || !pump_task_requires_completion_map.at(current_pumping_paint.value())) ) ) {

        current_pumping_paint = get_next_paint_to_pump();
        if(current_pumping_paint.has_value()){
             log_message("PUMPING_BASE: Next paint to pump: " + basePaintTypeToString(current_pumping_paint.value()));
        }
    }

    if (current_pumping_paint.has_value()) {
        BasePaintType active_paint = current_pumping_paint.value();
        Pump& pump = *base_pumps_map.at(active_paint);
        Tank& source_tank = *base_tanks_map.at(active_paint);
        Valve& discharge_valve = *base_discharge_valves_map.at(active_paint);
        Valve& suction_valve = *base_suction_valves_map.at(active_paint);

        if (pump_task_requires_completion_map.at(active_paint)) {
            if (!pump.is_stopped_due_to_low_flow() && !pump.is_stopped_due_to_overpressure()) {
                log_message("PUMPING_BASE: Pump " + pump.get_name() + " for " + basePaintTypeToString(active_paint)
                          + " has recovered. Clearing recovery flag.");
                pump_task_requires_completion_map[active_paint] = false;
            } else {
                log_message("PUMPING_BASE: Pump " + pump.get_name() + " for " + basePaintTypeToString(active_paint)
                          + " is still in fault. Cannot complete its task yet.");
                current_pumping_paint = std::nullopt;
                return;
            }
        }

        double amount_needed_for_paint = target_liters_for_paint_type.at(active_paint) - current_batch_pumped_liters.at(active_paint);

        if (amount_needed_for_paint > 1e-6) {
            if (source_tank.get_current_level_liters() < 1e-6) {
                 handle_error_state("Source tank " + source_tank.get_name() + " is empty. Cannot pump " + basePaintTypeToString(active_paint));
                 pump.stop();
                 return;
            }

            if (discharge_valve.get_status() == ValveStatus::CLOSED) {
                discharge_valve.open();
                log_message("System automatically opened " + discharge_valve.get_name() + " for pumping " + basePaintTypeToString(active_paint) + ".");
            }
            if (suction_valve.get_status() == ValveStatus::CLOSED) {
                suction_valve.open();
                log_message("System automatically opened " + suction_valve.get_name() + " for pumping " + basePaintTypeToString(active_paint) + ".");
            }

            pump.start();

            if (pump.get_status() == PumpStatus::ON && pump.get_flow_rate_lpm() > 1e-6 && !pump.is_stopped_due_to_low_flow() && !pump.is_stopped_due_to_overpressure()) {
                double pumped_this_frame = (pump.get_flow_rate_lpm() / 60.0) * time_delta_seconds;
                pumped_this_frame = std::min(pumped_this_frame, amount_needed_for_paint);
                pumped_this_frame = std::min(pumped_this_frame, source_tank.get_current_level_liters());
                double mixer_space = tank_mixer_storage.get_capacity_liters() - tank_mixer_storage.get_current_level_liters();
                pumped_this_frame = std::min(pumped_this_frame, mixer_space);

                if (pumped_this_frame > 1e-6) {
                    current_batch_pumped_liters[active_paint] += pumped_this_frame;
                    source_tank.remove_liquid(pumped_this_frame);
                    tank_mixer_storage.add_liquid(pumped_this_frame);
                    pump_run_times_seconds[active_paint] += time_delta_seconds;
                } else if (mixer_space <= 1e-6 && amount_needed_for_paint > 1e-6) { // Only error if we needed to pump more
                    handle_error_state("Mixer tank is full, cannot add more liquid while pumping " + basePaintTypeToString(active_paint));
                    pump.stop();
                    return;
                }


                if (current_batch_pumped_liters.at(active_paint) >= target_liters_for_paint_type.at(active_paint) - 1e-6) {
                    log_message("PUMPING_BASE: Target reached for " + basePaintTypeToString(active_paint) + ". Stopping pump " + pump.get_name());
                    pump.stop();
                    // Optionally close valves if that's desired system behavior
                    // discharge_valve.close();
                    // suction_valve.close();
                    current_pumping_paint = std::nullopt;
                }
            } else if (pump.is_stopped_due_to_low_flow() || pump.is_stopped_due_to_overpressure()){
                if (!pump_task_requires_completion_map.at(active_paint)) {
                    log_message("PUMPING_BASE: Pump " + pump.get_name() + " for " + basePaintTypeToString(active_paint)
                              + " entered fault during operation. Marking for recovery.");
                    pump_task_requires_completion_map[active_paint] = true;
                    current_pumping_paint = std::nullopt;
                }
            }
        } else {
            pump.stop();
            current_pumping_paint = std::nullopt;
        }
    } else {
        bool all_tasks_truly_done = true;
        bool any_pump_still_recovering = false;

        for (auto const& [paint, target_L] : target_liters_for_paint_type) {
            if (target_L > 1e-6) {
                if (current_batch_pumped_liters.at(paint) < target_L - 1e-6) {
                    if (pump_task_requires_completion_map.at(paint)) {
                        any_pump_still_recovering = true;
                    }
                    all_tasks_truly_done = false;
                }
            }
        }

        if (all_tasks_truly_done) {
            log_message("PUMPING_BASE: All paints pumped to target. Transitioning to MIXING.");
            current_process_state = ProcessState::MIXING;
            mixer.set_target_mixing_time(30.0);
            mixer.start_motor();
            for (auto& pair : base_pumps_map) { pair.second->stop(); }
        } else if (any_pump_still_recovering) {
            log_message("PUMPING_BASE: Waiting for pump recovery. Current active_paint is none.");
        } else {
            handle_error_state("Pumping not complete, but no available pump or recovery path.");
        }
    }
}

void System::handle_mixing_state(double time_delta_seconds) {
    // `mixer.update_state(time_delta_seconds)` is called in main System::update
    if (!mixer.is_motor_on() && current_process_state == ProcessState::MIXING) {
        // This condition means the mixer has completed its timed cycle and stopped itself.
        log_message("Mixing complete (Duration: " + std::to_string(mixer.get_current_mixing_duration()) + "s). Starting to empty mixer.");
        current_process_state = ProcessState::EMPTYING;
        if (mixer.get_drain_valve().get_status() == ValveStatus::CLOSED) {
            mixer.get_drain_valve().open();
            log_message("System automatically opened " + mixer.get_drain_valve().get_name() + " for emptying.");
        }
    }
    // Note: No direct usage of time_delta_seconds here as mixer's own update_state handles its timing.
}

void System::handle_emptying_state(double time_delta_seconds) {
    if (current_process_state != ProcessState::EMPTYING) {
        return;
    }

    Tank& mix_tank = tank_mixer_storage; // Use reference for brevity

    if (mix_tank.get_current_level_liters() > 1e-6 && mixer.get_drain_valve().get_status() == ValveStatus::OPEN) {
        double capacity = mix_tank.get_capacity_liters();
        // Ensure capacity is positive to prevent issues with empty_rate_lps calculation
        double empty_rate_lps = (capacity > 0) ? (capacity * 0.04) : 5.0; // 4% of capacity per second, or a default 5 LPS if capacity is 0 (should not happen)
        if (empty_rate_lps <= 1e-6) empty_rate_lps = 5.0; // ensure a minimum drain rate if capacity is very small

        double amount_to_remove = empty_rate_lps * time_delta_seconds;
        mix_tank.remove_liquid(amount_to_remove);
        // log_message("Emptying mixer: " + std::to_string(mix_tank.get_current_level_liters()) + " L remaining."); // Can be too noisy
    } else if (mix_tank.get_current_level_liters() <= 1e-6) { // Effectively empty
        if (mixer.get_drain_valve().get_status() == ValveStatus::OPEN) {
             mixer.get_drain_valve().close();
             log_message("System automatically closed " + mixer.get_drain_valve().get_name() + " as mixer is empty.");
        }
        log_message("Mixer empty. Batch complete. System transitioning to IDLE.");
        batch_in_progress = false;
        current_process_state = ProcessState::IDLE;
        // pump_run_times_seconds are reset at the start of a new batch.
    } else if (mixer.get_drain_valve().get_status() == ValveStatus::CLOSED && mix_tank.get_current_level_liters() > 1e-6) {
        log_message("EMPTYING_STATE: Mixer drain valve is closed but tank not empty. Opening drain valve.");
        mixer.get_drain_valve().open(); // Ensure drain valve is open if we are in this state and not empty
    }
}


void System::update(double time_delta_seconds) {
    for (auto& pair : base_pumps_map) { pair.second->update_state(); }
    mixer.update_state(time_delta_seconds);

    if (tank_mixer_storage.get_current_level_liters() <= 1e-6) {
        mixer_low_level_switch.set_flow_status(SwitchStatus::ALARM);
    } else {
        mixer_low_level_switch.set_flow_status(SwitchStatus::NORMAL);
    }

    switch (current_process_state) {
        case ProcessState::IDLE:
            if (start_command == OnOffStatus::ON_COMMAND) {
                if (can_start_new_batch()) {
                    start_new_batch();
                    // As per Req 11: "Esta condición será verificada y el comando de arranque (Start_command) pasará a OFF".
                    // This implies the system itself should reset the command after processing it.
                    start_command = OnOffStatus::OFF_COMMAND;
                    log_message("Start command processed and consumed by IDLE state for new batch.");
                } else {
                    // Log why it couldn't start only if the command is fresh
                    if (!batch_in_progress && mixer_low_level_switch.get_flow_status() != SwitchStatus::ALARM) {
                        log_message("Cannot start new batch: Mixer is not empty (LSL-401 not in ALARM).");
                    } else if (batch_in_progress) {
                        log_message("Cannot start new batch: A batch is already in progress.");
                    } else {
                         // Generic message if other conditions in can_start_new_batch failed (e.g. insufficient material)
                         log_message("Cannot start new batch: Preconditions not met (e.g. insufficient materials or system not ready). Check last_error_message for details if available from can_start_new_batch logic.");
                         // To make this more specific, can_start_new_batch could return an enum or string with the reason.
                    }
                     // If it's a persistent ON command, this might spam. Consider consuming it even on failure to start.
                     // For now, let's assume operator needs to toggle OFF then ON again if start fails.
                }
            }
            break;

        case ProcessState::PUMPING_BASE:
            handle_pumping_state(time_delta_seconds);
            break;

        case ProcessState::MIXING:
            handle_mixing_state(time_delta_seconds);
            break;

        case ProcessState::EMPTYING:
            handle_emptying_state(time_delta_seconds);
            break;

        case ProcessState::ERROR_STATE:
            // The system stays in ERROR_STATE until manually reset or specific recovery.
            // No automatic transition out. Pumps and mixer are stopped by handle_error_state.
            break;

        case ProcessState::WAITING_FOR_RECOVERY:
            // Similar to IDLE, might check if recovery conditions are met to transition back to PUMPING_BASE
            // Or wait for a manual "retry" command.
            log_message("System in WAITING_FOR_RECOVERY state.");
            // Simple recovery: if no pumps need completion, try to go to PUMPING_BASE to re-evaluate.
            bool needs_recovery = false;
            for(const auto& pair : pump_task_requires_completion_map){
                if(pair.second) {needs_recovery = true; break;}
            }
            if(!needs_recovery && batch_in_progress){ // If batch was active and no more tasks need recovery
                log_message("WAITING_FOR_RECOVERY: All pump tasks marked as not requiring completion. Attempting to return to PUMPING_BASE.");
                current_process_state = ProcessState::PUMPING_BASE;
            } else if (!batch_in_progress) { // If batch was somehow stopped.
                current_process_state = ProcessState::IDLE;
            }
            break;

        default:
            handle_error_state("Unknown process state encountered: " + std::to_string(static_cast<int>(current_process_state)));
            break;
    }
}

std::string System::get_system_status_report() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "\n--- System Status Report ---\n";
    ss << "Process State: " << processStateToString(current_process_state)
       << (batch_in_progress ? " (Batch In Progress)" : " (No Batch)") << "\n";
    ss << "Selected Paint: " << (selected_paint_type == PaintType::AZUL_MARINO ? "Azul Marino" : "Azul Celeste") << "\n";
    ss << "Start Command Input: " << (start_command == OnOffStatus::ON_COMMAND ? "ON" : "OFF") << "\n";

    ss << "\n--- Base Tanks ---\n";
    ss << tank_blanco.get_name() << ": " << tank_blanco.get_current_level_liters() << " L (" << tank_blanco.get_current_level_percentage() << "%)\n";
    ss << tank_azul.get_name() << ": " << tank_azul.get_current_level_liters() << " L (" << tank_azul.get_current_level_percentage() << "%)\n";
    ss << tank_negro.get_name() << ": " << tank_negro.get_current_level_liters() << " L (" << tank_negro.get_current_level_percentage() << "%)\n";

    ss << "\n--- Mixer ---\n";
    ss << mixer.get_name() << " (Tank: " << tank_mixer_storage.get_name() << "): " << tank_mixer_storage.get_current_level_liters() << " L (" << tank_mixer_storage.get_current_level_percentage() << "%)\n";
    ss << "Motor: " << (mixer.is_motor_on() ? "ON" : "OFF") << ", Mix Duration: " << mixer.get_current_mixing_duration() << "s / " << mixer.get_target_mixing_time() << "s\n";
    ss << "LSL401 (Low Level): " << switchStatusToString(mixer_low_level_switch.get_flow_status()) << "\n";
    // v401_drain is part of the mixer group, but also listed below for completeness in "All Key Valves"

    ss << "\n--- All Key Valves ---\n";
    ss << v201_s.get_name() << ": " << valveStatusToString(v201_s.get_status()) << "\n";
    ss << v201_d.get_name() << ": " << valveStatusToString(v201_d.get_status()) << "\n";
    ss << v202_s.get_name() << ": " << valveStatusToString(v202_s.get_status()) << "\n";
    ss << v202_d.get_name() << ": " << valveStatusToString(v202_d.get_status()) << "\n";
    ss << v203_s.get_name() << ": " << valveStatusToString(v203_s.get_status()) << "\n";
    ss << v203_d.get_name() << ": " << valveStatusToString(v203_d.get_status()) << "\n";
    ss << v401_drain.get_name() << ": " << valveStatusToString(v401_drain.get_status()) << "\n";


    ss << "\n--- Pumps & Associated Sensors ---\n";
    // Define order for consistent reporting
    const BasePaintType ordered_paints[] = {BasePaintType::BLANCO, BasePaintType::AZUL, BasePaintType::NEGRO};
    for(BasePaintType p_type : ordered_paints) {
        // Check if the paint type exists in the maps (it should, due to initialization)
        if (base_pumps_map.count(p_type) == 0) continue;

        const Pump& pump = *base_pumps_map.at(p_type);
        const Valve& s_valve = *base_suction_valves_map.at(p_type);
        const Valve& d_valve = *base_discharge_valves_map.at(p_type);

        // A bit awkward to get specific PT/FS sensors if not mapped by BasePaintType
        const Sensor* pt_sensor_ptr = nullptr;
        const Sensor* fs_sensor_ptr = nullptr;
        if (p_type == BasePaintType::BLANCO) { pt_sensor_ptr = &pt201; fs_sensor_ptr = &fs201;}
        else if (p_type == BasePaintType::AZUL) { pt_sensor_ptr = &pt202; fs_sensor_ptr = &fs202;}
        else if (p_type == BasePaintType::NEGRO) { pt_sensor_ptr = &pt203; fs_sensor_ptr = &fs203;}

        ss << pump.get_name() << " (" << basePaintTypeToString(p_type) << "): " << pumpStatusToString(pump.get_status())
           << ", Flow: " << pump.get_flow_rate_lpm() << " LPM, Pressure: " << pump.get_current_pressure_psi() << " PSI";
        if (pump_task_requires_completion_map.at(p_type)) ss << " [RECOVERY_NEEDED]";
        ss << "\n";

        // Suction and Discharge valve statuses are now part of "All Key Valves" but can be repeated here for context if desired.
        // For brevity, we'll rely on the section above for individual valve states.
        // ss << "  Suction (" << s_valve.get_name() << "): " << valveStatusToString(s_valve.get_status())
        //    << ", Discharge (" << d_valve.get_name() << "): " << valveStatusToString(d_valve.get_status()) << "\n";
        if (pt_sensor_ptr && fs_sensor_ptr) {
            ss << "  PT (" << pt_sensor_ptr->get_name() << "): " << pt_sensor_ptr->get_pressure_psi() << " PSI"
               << ", FS (" << fs_sensor_ptr->get_name() << "): " << switchStatusToString(fs_sensor_ptr->get_flow_status()) << "\n";
        } else {
            ss << "  Associated sensors not found for " << pump.get_name() << "\n";
        }
        if(pump.is_stopped_due_to_low_flow()) ss << "  FAULT: Stopped due to Low Flow\n";
        if(pump.is_stopped_due_to_overpressure()) ss << "  FAULT: Stopped due to Overpressure\n";
        ss << "  Recipe Target: " << target_liters_for_paint_type.at(p_type) << "L, Pumped this batch: " << current_batch_pumped_liters.at(p_type) << "L"
           << ", Total RunTime: " << pump_run_times_seconds.at(p_type) << "s\n";
    }

    if (!last_error_message.empty()) { // Changed from checking system_logs to last_error_message
        ss << "\nLAST MESSAGE/ERROR: " << last_error_message << "\n";
    }

    ss << "--- End of Report ---\n";
    return ss.str();
}

ProcessState System::get_current_process_state() const { return current_process_state; }
bool System::is_batch_in_progress() const { return batch_in_progress; }

void System::handle_error_state(const std::string& error_message) {
    if (current_process_state != ProcessState::ERROR_STATE || this->last_error_message != error_message) {
        log_message("ERROR: " + error_message + ". System entering ERROR_STATE.");
    }
    current_process_state = ProcessState::ERROR_STATE;
    batch_in_progress = false;
    for(auto& pair : base_pumps_map) { pair.second->stop(); }
    mixer.stop_motor();
}
void System::handle_waiting_for_recovery_state() {
    log_message("System in WAITING_FOR_RECOVERY state.");
    // Check if any pump still needs recovery
    bool recovery_needed = false;
    for(const auto& pair : pump_task_requires_completion_map) {
        if (pair.second) { // If any pump task is marked as needing completion
            // Check if the corresponding pump is still in fault
            Pump* p = base_pumps_map.at(pair.first);
            if (p->is_stopped_due_to_low_flow() || p->is_stopped_due_to_overpressure()) {
                 recovery_needed = true;
                 break;
            }
        }
    }

    if (!recovery_needed && batch_in_progress) {
        log_message("WAITING_FOR_RECOVERY: All pumps recovered or tasks completed. Transitioning to PUMPING_BASE.");
        current_process_state = ProcessState::PUMPING_BASE;
        // Clear all recovery flags as we are retrying pumping
        for(auto& pair : pump_task_requires_completion_map) { pair.second = false; }
    } else if (!batch_in_progress) {
        log_message("WAITING_FOR_RECOVERY: Batch no longer in progress. Transitioning to IDLE.");
        current_process_state = ProcessState::IDLE;
    } else {
        log_message("WAITING_FOR_RECOVERY: Pumps still in fault or tasks pending. Holding state.");
    }
}
const std::vector<std::string>& System::get_logs() const {
    return system_logs;
}

void System::clear_logs() {
    // Only clear the main system_logs. last_error_message persists until overwritten by a new message/error.
    system_logs.clear();
}

// Adding missing get_target_mixing_time to Mixer to satisfy report (already present from previous step)
// double Mixer::get_target_mixing_time() const { return target_mixing_time_seconds;}
