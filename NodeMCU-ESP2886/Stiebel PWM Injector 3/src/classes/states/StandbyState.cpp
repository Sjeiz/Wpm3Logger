#include "StandbyState.h"
#include "config.h"
#include "HeatingState.h"
#include "HotWaterState.h"
#include "DefrostState.h"
#include "ErrorState.h"

State* StandbyState::transition(uint16_t modbusStatus) {
    if (modbusStatus & ISG_STATUS_HEATING) {
        return new HeatingState();
    }
    if (modbusStatus & ISG_STATUS_HOT_WATER) {
        return new HotWaterState();
    }
    if (modbusStatus & ISG_STATUS_DEFROSTING) {
        return new DefrostState();
    }
    if (modbusStatus == ISG_MODBUS_READ_ERROR) {
        return new ErrorState();
    }
    return this;
}
