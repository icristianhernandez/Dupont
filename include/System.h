#ifndef SYSTEM_H
#define SYSTEM_H

#include "Pump.h"
#include "Valve.h"
#include "Tank.h"
#include "Mixer.h"
#include "Sensor.h"
#include "Enums.h"

#include <vector>
#include <map>
#include <string>
#include <memory> // For std::unique_ptr if needed, though direct objects for now
#include <optional> // For std::optional

class System {
public:
    System();
    ~System();

    void update(double time_delta_seconds);
    void set_start_command(OnOffStatus command);
    void set_selected_paint_type(PaintType type);
    void load_commands_from_file(const std::string& filepath);

    std::string get_system_status_report() const;
    ProcessState get_current_process_state() const;
    bool is_batch_in_progress() const;
    const std::vector<std::string>& get_logs() const;
    void clear_logs();


    // Simplified input processing for now - process_valve_command might be superseded by load_commands
    void process_valve_command(const std::string& valve_name, ValveStatus status);
    // Add other input processing methods as needed, e.g., for pump commands if manual override is allowed

private:
    std::map<std::string, Valve*> controllable_valves_map;
    // Helper methods for internal logic
    void initialize_components();
    void update_recipes();
    void start_new_batch();
    void handle_idle_state();
    void handle_pumping_state(double time_delta_seconds);
    void handle_mixing_state(double time_delta_seconds);
    void handle_emptying_state(double time_delta_seconds);
    void handle_error_state(const std::string& error_message);
    void handle_waiting_for_recovery_state();

    void check_pump_conditions_and_operate();
    void check_component_failures(); // General check for any component that might put system in ERROR_STATE
    bool can_start_new_batch() const;
    BasePaintType get_next_paint_to_pump() const;


    // Component Members
    // Base Material Tanks (P201->Blanco, P202->Azul, P203->Negro based on typical order)
    Tank tank_blanco; // T201
    Tank tank_azul;   // T202
    Tank tank_negro;  // T203

    // Valves for base tanks & pumps
    // Suction valves (V201_S, V202_S, V203_S)
    Valve v201_s; // Suction for P201 (Blanco)
    Valve v202_s; // Suction for P202 (Azul)
    Valve v203_s; // Suction for P203 (Negro)

    // Discharge valves (V201_D, V202_D, V203_D) - these are the V201, V202, V203 from input file
    Valve v201_d; // Discharge for P201 (Blanco) to mixer
    Valve v202_d; // Discharge for P202 (Azul) to mixer
    Valve v203_d; // Discharge for P203 (Negro) to mixer

    // Sensors for Pumps
    Sensor pt201; Sensor fs201; // For P201
    Sensor pt202; Sensor fs202; // For P202
    Sensor pt203; Sensor fs203; // For P203

    // Pumps
    Pump p201; // Blanco Pump
    Pump p202; // Azul Pump
    Pump p203; // Negro Pump

    // Mixer Components
    Tank tank_mixer_storage; // M401's tank
    Valve v401_drain;      // Mixer drain valve (V401 from input file)
    Mixer mixer;
    Sensor mixer_low_level_switch; // LSL-401

    // Process Control Variables
    ProcessState current_process_state;
    PaintType selected_paint_type;
    OnOffStatus start_command;
    bool batch_in_progress;

    std::map<BasePaintType, double> current_batch_pumped_liters;
    std::map<BasePaintType, double> target_liters_for_paint_type; // Stores recipe for selected_paint_type

    // Maps for easier access by BasePaintType
    std::map<BasePaintType, Pump*> base_pumps_map;
    std::map<BasePaintType, Tank*> base_tanks_map;
    std::map<BasePaintType, Valve*> base_discharge_valves_map;
    std::map<BasePaintType, Valve*> base_suction_valves_map;


    // Pump specific states for recovery (Req 8)
    std::map<BasePaintType, bool> pump_task_requires_completion_map; // Maps BasePaintType to its "needs to complete" status
    std::map<BasePaintType, double> pump_run_times_seconds;          // Tracks run time for each base paint pump

    std::optional<BasePaintType> current_pumping_paint; // Which paint is being pumped in PUMPING_BASE state

    double total_target_batch_size_liters;
    std::string last_error_message; // Stores general error messages or important status updates
    std::vector<std::string> system_logs; // For storing a log of system events
};

#endif // SYSTEM_H
