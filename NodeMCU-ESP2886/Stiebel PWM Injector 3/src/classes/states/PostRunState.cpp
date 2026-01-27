#include "PostRunState.h"
#include "config.h"
#include "StandbyState.h"
#include "ErrorState.h"

State* PostRunState::transition(uint16_t modbusStatus) {
    if (modbusStatus == ISG_MODBUS_READ_ERROR) {
        return new ErrorState();
    }
    return new StandbyState();
}
