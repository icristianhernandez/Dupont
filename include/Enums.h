#ifndef ENUMS_H
#define ENUMS_H

enum class PaintType {
    AZUL_MARINO,
    AZUL_CELESTE
};

enum class SwitchStatus {
    NORMAL,
    ALARM
};

enum class ValveStatus {
    OPEN,
    CLOSED
};

enum class PumpStatus {
    ON,
    OFF
};

enum class OnOffStatus {
    ON_COMMAND,
    OFF_COMMAND
};

enum class SensorType {
    FLOW_SWITCH,
    PRESSURE_TRANSMITTER,
    LEVEL_TRANSMITTER
};

enum class LevelStatus {
    EMPTY,
    LOW,
    NORMAL_LEVEL,
    HIGH
};

enum class ProcessState {
    IDLE,
    PUMPING_BASE,
    MIXING,
    EMPTYING,
    ERROR_STATE,
    WAITING_FOR_RECOVERY
};

enum class BasePaintType {
    NEGRO, // Corresponds to P203
    AZUL,  // Corresponds to P202
    BLANCO // Corresponds to P201
};

#endif // ENUMS_H
