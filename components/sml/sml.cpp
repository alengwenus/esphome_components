#include <iostream>
#include <sstream>
#include "sml.h"
#include "esphome/core/log.h"
#include "sml_parser.h"

namespace esphome {
namespace sml {

static const char *const TAG = "sml";

const char START_BYTES[8] = {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
const char END_BYTES[5] = {0x1b, 0x1b, 0x1b, 0x1b, 0x1a};

const char START_BYTES_DETECTED = 1;
const char END_BYTES_DETECTED = 2;

char Sml::checkStartEndBytes_(char c) {
  // fill sml_file with incoming bytes
  for (int k = 1; k < 8; k++) {
    this->incomingBuffer_[k - 1] = this->incomingBuffer_[k];
  };
  this->incomingBuffer_[7] = c;

  if (memcmp(this->incomingBuffer_, START_BYTES, sizeof(START_BYTES)) == 0)
    return START_BYTES_DETECTED;
  if (memcmp(this->incomingBuffer_, END_BYTES, sizeof(END_BYTES)) == 0)
    return END_BYTES_DETECTED;
  return 0;
}

void Sml::loop() {
  while (available()) {
    const char c = read();

    if (this->record_)
      this->sml_data_.push_back(c);

    switch (this->checkStartEndBytes_(c)) {
      case START_BYTES_DETECTED: {
        this->record_ = true;
        this->sml_data_.assign(START_BYTES, START_BYTES + 8);
        break;
      };
      case END_BYTES_DETECTED: {
        this->record_ = false;
        this->process_sml_file_(this->sml_data_);
        break;
      };
    };
  }
}

void Sml::process_sml_file_(bytes sml_data) {
    // check bytes crc
  if (!check_sml_data(sml_data)) {
    ESP_LOGW(TAG, "Checksum error in received SML data.");
    return;
  }

  SmlFile sml_file = SmlFile(sml_data);
  std::vector<ObisInfo> obis_info = get_obis_info(sml_file);
  this->publish_obis_info_(obis_info);

  if (this->logging_)
    // this->log_obis_info_(obis_info);
    //  logSmlFile(sml_data);
    this->fire_obis_info_event_(obis_info);
}


void Sml::log_obis_info_(std::vector<ObisInfo> obis_info_vec) {
  int i = 0;
  for (auto obis_info : obis_info_vec) {
    std::ostringstream info_stream;
    info_stream << "SML_GetList.Res: " << i++ << std::endl;
    info_stream << "  Server id: " << bytes_repr(obis_info.server_id) << std::endl;
    info_stream << "  OBIS:      " << obis_info.code_repr() << std::endl;
    info_stream << "  value:     " << bytes_repr(obis_info.value) << std::endl;
    info_stream << "  unit:      " << unsigned(obis_info.unit);
    std::string info = info_stream.str();
    ESP_LOGD(TAG, "%s", info.c_str());
    delay(5);
  }
}

void Sml::fire_obis_info_event_(std::vector<ObisInfo> obis_info_vec) {
  std::map<std::string, std::string> data;
  for (auto obis_info : obis_info_vec) {
    std::ostringstream key;
    std::ostringstream value;
    key << "(" << bytes_repr(obis_info.server_id) << ") " << obis_info.code_repr();
    value << "0x" << bytes_repr(obis_info.value);
    data[key.str()] = value.str();
  }
  this->fire_homeassistant_event("esphome.sml_obisinfo", data);
}

void Sml::publish_obis_info_(std::vector<ObisInfo> obis_info_vec) {
  for (auto obis_info : obis_info_vec) {
    this->publish_value_(obis_info);
  }
}

void Sml::publish_value_(ObisInfo obis_info) {
  for (auto element : sml_listeners_) {
    if ((element->server_id != "") && (bytes_repr(obis_info.server_id) != element->server_id))
      continue;
    if (obis_info.code_repr() != element->obis)
      continue;
    element->publish_val(obis_info);
  }
}

void Sml::dump_config() { ESP_LOGCONFIG(TAG, "SmlInfo"); }

Sml::Sml(bool logging) { this->logging_ = logging; }

void Sml::register_sml_listener(SmlListener *listener) { sml_listeners_.push_back(listener); }

void logSmlFile(std::vector<unsigned char> sml_file) {
  for (int j = 0; j < (sml_file.size() / 16 + 1); j++) {
    char str_buffer[3 * 16 + 1]{0};
    for (int i = 0; i < 16; i++) {
      if (16 * j + i == sml_file.size())
        break;
      sprintf(&str_buffer[3 * i], "%02x ", sml_file.at(16 * j + i));
    };
    ESP_LOGD(TAG, "%s", str_buffer);
    delay(5);
  };
}

}  // namespace sml
}  // namespace esphome
