#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "sml_text_sensor.h"
#include "../sml_parser.h"

namespace esphome {
namespace sml {

static const char *const TAG = "sml_text_sensor";

SmlTextSensor::SmlTextSensor(const char *server_id, const char *obis, const char *format) {
  this->server_id = std::string(server_id);
  this->obis = std::string(obis);
  this->format = std::string(format);
}

void SmlTextSensor::publish_val(ObisInfo obis_info) {
  short value_type;
  if(this->format == "hex")
    value_type = SMLHEX;
  else if (this->format == "text")
    value_type = SMLOCTET;
  else if (this->format == "bool")
    value_type = SMLBOOL;
  else if (this->format == "uint")
    value_type = SMLUINT;
  else if (this->format == "int")
    value_type = SMLINT;
  else
    value_type = obis_info.value_type;

  switch (value_type) {
    case SMLHEX: {
      publish_state("0x" + bytes_repr(obis_info.value));
      break;
    }
    case SMLINT: {
      publish_state(to_string(bytes_to_int(obis_info.value)));
      break;
    }
    case SMLBOOL:
      publish_state(bytes_to_uint(obis_info.value) ? "True" : "False");
      break;
    case SMLUINT: {
      publish_state(to_string(bytes_to_uint(obis_info.value)));
      break;
    }
    case SMLOCTET: {
      publish_state(std::string(obis_info.value.begin(), obis_info.value.end()));
      break;
    }
  }
}

void SmlTextSensor::dump_config() { LOG_TEXT_SENSOR("  ", "SML", this); }
}  // namespace sml
}  // namespace esphome
