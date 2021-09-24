#pragma once

#include <vector>
#include "constants.h"

using bytes = std::vector<char>;

uint16_t get_entry_length(const bytes &buffer, unsigned int &pos);

class SmlBase {
 public:
  short type;
  uint16_t length;
  SmlBase(const bytes &buffer, unsigned int &pos);

 protected:
  const bytes &buffer_;
  const unsigned int startpos_;
};

class SmlNode : public SmlBase {
 public:
  bool is_list();
  SmlNode(const bytes &buffer, unsigned int &pos);
  bytes value_bytes;
  std::vector<SmlNode> nodes;
};

class SmlFile {
 public:
  SmlFile(bytes buffer);
  std::vector<SmlNode> messages;
 protected:
   const bytes buffer_;
};

class ObisInfo {
 public:
  ObisInfo(bytes server_id, SmlNode val_list);
  bytes server_id;
  bytes code;
  bytes status;
  // SmlNode *val_time;
  char unit;
  char scaler;
  bytes value;
  uint16_t value_type;
  std::string code_repr();
};

bool check_sml_data(const bytes &buffer);

uint16_t calc_crc(const bytes &buffer);

std::string bytes_repr(const bytes &buffer);

uint64_t bytes_to_uint(const bytes &buffer);

int64_t bytes_to_int(const bytes &buffer);

std::string bytes_to_string(const bytes &buffer);

std::vector<ObisInfo> get_obis_info(SmlFile sml_file);
