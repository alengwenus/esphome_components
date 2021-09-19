#include "esphome/core/log.h"
#include "sml_sensor.h"
#include "../sml_parser.h"

namespace esphome {
namespace sml {

static const char *const TAG = "sml_sensor";

SmlSensor::SmlSensor(const char *server_id, const char *obis) {
  this->server_id = std::string(server_id);
  this->obis = std::string(obis);
}

void SmlSensor::publish_val(ObisInfo obis_info) {
  switch (obis_info.value_type) {
    case SMLINT: {
      publish_state(bytes_to_int(obis_info.value));
      break;
    }
    case SMLBOOL:
    case SMLUINT: {
      publish_state(bytes_to_uint(obis_info.value));
      break;
    }
    case SMLOCTET: {
      ESP_LOGW(TAG, "No number conversion for (%s) %s. Consider using SML TextSensor instead.",
               bytes_repr(obis_info.server_id).c_str(), obis_info.code_repr().c_str());
      break;
    }
  }
}

void SmlSensor::dump_config() {
  std::string type;
  LOG_SENSOR("  ", "SML", this);
}
}  // namespace sml
}  // namespace esphome
