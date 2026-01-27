#include "HotWaterState.h"
#include "config.h"
#include "StandbyState.h"
#include "HeatingState.h"
#include "DefrostState.h"
#include "ErrorState.h"

State* HotWaterState::transition(uint16_t modbusStatus) {
    if (modbusStatus & ISG_STATUS_HEATING) {
        return new HeatingState();
    }
    if (modbusStatus & ISG_STATUS_DEFROSTING) {
        return new DefrostState();
    }
    if (!(modbusStatus & ISG_STATUS_HOT_WATER)) {
        return new StandbyState();
    }
    if (modbusStatus == ISG_MODBUS_READ_ERROR) {
        return new ErrorState();
    }
    return this;
}
