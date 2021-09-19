#pragma once

#include <vector>
#include "constants.h"

typedef std::vector<unsigned char> bytes;

short get_entry_type(bytes buffer, unsigned int *pos);
short get_entry_length(bytes buffer, unsigned int *pos);
unsigned int skip_entry(bytes buffer, unsigned int pos);

class SmlBase {
 public:
  short type;
  short length;
  SmlBase(bytes buffer, unsigned int pos);
  unsigned int pos;

 protected:
  bytes buffer;
};

class SmlNode : public SmlBase {
 public:
  bytes value_bytes;
  bool is_list();
  SmlNode(bytes buffer, unsigned int pos);
  SmlNode node(unsigned int idx);
  std::vector<SmlNode> nodes();

 protected:
  std::vector<unsigned int> stack;
};

class SmlFile {
 public:
  bool buffer_valid = false;
  SmlFile(bytes buffer);
  std::vector<SmlNode> messages;
  bool check_buffer(bytes buffer);

 protected:
  unsigned int pos = 0;
};

class ObisInfo {
 public:
  ObisInfo(bytes server_id, SmlNode val_list);
  bytes server_id;
  bytes code;
  bytes status;
  SmlNode *val_time;
  unsigned char unit;
  char scaler;
  bytes value;
  short value_type;
  std::string code_repr();
};

unsigned short calc_crc(bytes buffer);

std::string bytes_repr(bytes buffer);

uint64_t bytes_to_uint(bytes buffer);

int64_t bytes_to_int(bytes buffer);

std::string bytes_to_string(bytes buffer);

std::vector<ObisInfo> get_obis_info(SmlFile sml_file);
