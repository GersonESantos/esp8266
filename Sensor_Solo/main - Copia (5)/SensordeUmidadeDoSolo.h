#ifndef _SENSORDEUMIDADEDOSOLO_H_
#define _SENSORDEUMIDADEDOSOLO_H_

#include <SinricProDevice.h>
#include <Capabilities/RangeController.h>
#include <Capabilities/ModeController.h>

class SensordeUmidadeDoSolo 
: public SinricProDevice
, public RangeController<SensordeUmidadeDoSolo>
, public ModeController<SensordeUmidadeDoSolo> {
  friend class RangeController<SensordeUmidadeDoSolo>;
  friend class ModeController<SensordeUmidadeDoSolo>;
public:
  SensordeUmidadeDoSolo(const String &deviceId) : SinricProDevice(deviceId, "SensordeUmidadeDoSolo") {};
};

#endif
