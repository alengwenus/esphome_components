#include <iostream>
#include <sstream>
#include <iomanip>
#include "constants.h"
#include "sml_parser.h"

using namespace std;

short get_entry_type(const bytes &buffer, unsigned int pos) {
  short type = buffer[pos] >> 4;
  return type & 0x07;
}

short get_entry_length(const bytes &buffer, unsigned int pos) {
  short type = buffer[pos] >> 4;
  short length = buffer[pos] & 0x0f;

  if (type & 0x08) {  // we have a long list/value (>15 entries)
    length = (length << 4) + (buffer[pos + 1] & 0x0f);
  };
  return length;
}

unsigned int skip_entry(const bytes &buffer, unsigned int pos) {
  short entry_length = get_entry_length(buffer, pos);
  if (buffer[pos] == 0x00) {  // end of message
    pos += 1;
  } else if (get_entry_type(buffer, pos) != SMLLIST) {
    pos += entry_length;
  } else {  // list
    pos++;
    for (int i = 0; i != entry_length; i++) {
      pos = skip_entry(buffer, pos);
    };
  };
  return pos;
}

SmlBase::SmlBase(bytes buffer, unsigned int pos) {
  this->buffer = buffer;
  this->pos = pos;
  this->type = buffer[this->pos] >> 4;
  this->length = get_entry_length(buffer, pos);
}

SmlNode::SmlNode(bytes buffer, unsigned int pos) : SmlBase(buffer, pos) {
  if (!is_list()) {
    this->value_bytes = bytes(this->buffer.begin() + this->pos + 1, this->buffer.begin() + this->pos + this->length);
  }
}

SmlNode SmlNode::node(unsigned int idx) {
  // Todo: check if is_list()
  // Todo: check length
  unsigned int cur = this->pos + 1;
  for (int i = 0; i != idx; i++) {
    cur = skip_entry(this->buffer, cur);
  };
  return SmlNode(this->buffer, cur);
}

std::vector<SmlNode> SmlNode::nodes() {
  std::vector<SmlNode> nodes;
  if (this->is_list()) {
    unsigned int cur = this->pos + 1;
    for (int i = 0; i != this->length; i++) {
      nodes.push_back(SmlNode(this->buffer, cur));
      cur = skip_entry(this->buffer, cur);
    }
  }
  return nodes;
}

bool SmlNode::is_list() { return (this->type & 0x07) == SMLLIST; }

SmlFile::SmlFile(const bytes &buffer) {
  // slice buffer to remove start bytes and end bytes
  bytes file_buffer = bytes(buffer.begin() + 8, buffer.end() - 8);

  // extract messages
  unsigned int cur = 0;
  while (cur < file_buffer.size()) {
    if (file_buffer[cur] == 0x00)
      break;  // fill byte detected -> no more messages

    this->messages.push_back(SmlNode(file_buffer, cur));
    cur = skip_entry(file_buffer, cur);
  }
}

bool check_sml_data(const bytes &buffer) {
  // Todo: check start and end bytes
  unsigned short crc_received = (buffer.at(buffer.size() - 2) << 8) | buffer.at(buffer.size() - 1);
  return (crc_received == calc_crc(buffer));
}

unsigned short calc_crc(const bytes &buffer) {
  unsigned short crcsum = 0xffff;
  int len = buffer.size() - 2;
  int idx = 0;

  while (len--) {
    crcsum = (crcsum >> 8) ^ CRC16_X25_TABLE[(crcsum & 0xff) ^ buffer.at(idx++)];
  }

  crcsum ^= 0xffff;
  crcsum = (crcsum >> 8) | ((crcsum & 0xff) << 8);
  return crcsum;
}

string bytes_repr(const bytes &buffer) {
  ostringstream bytes_stream;
  for (int i = 0; i != buffer.size(); i++) {
    bytes_stream << setfill('0') << setw(2) << hex << (buffer[i] & 0xff);
  }
  return bytes_stream.str();
}

uint64_t bytes_to_uint(const bytes &buffer) {
  uint64_t val = 0;
  for (int i = 0; i != buffer.size(); i++) {
    val = (val << 8) + buffer.at(i);
  }
  return val;
}

int64_t bytes_to_int(const bytes &buffer) {
  uint64_t tmp = bytes_to_uint(buffer);
  int64_t val;

  switch (buffer.size()) {
    case 1:   // int8
      val = (int8_t)tmp;
      break;
    case 2:   // int16
      val = (int16_t)tmp;
      break;
    case 4:   // int32
      val = (int32_t)tmp;
      break;
    default:  // int64
      val = (int64_t)tmp;
  }
  return val;
}

string bytes_to_string(const bytes &buffer) { return string(buffer.begin(), buffer.end()); }

ObisInfo::ObisInfo(bytes server_id, SmlNode val_list_entry) {
  this->server_id = server_id;
  this->code = val_list_entry.node(0).value_bytes;
  this->status = val_list_entry.node(1).value_bytes;
  // // this->val_time = &val_list_entry.node(2);
  this->unit = bytes_to_uint(val_list_entry.node(3).value_bytes);
  this->scaler = bytes_to_int(val_list_entry.node(4).value_bytes);
  SmlNode value_node = val_list_entry.node(5);
  this->value = value_node.value_bytes;
  this->value_type = value_node.type;
}

string ObisInfo::code_repr() {
  ostringstream code_stream;
  code_stream << (uint) this->code[0];
  code_stream << "-";
  code_stream << (uint) this->code[1];
  code_stream << ":";
  code_stream << (uint) this->code[2];
  code_stream << ".";
  code_stream << (uint) this->code[3];
  code_stream << ".";
  code_stream << (uint) this->code[4];
  return code_stream.str();
}

std::vector<ObisInfo> get_obis_info(SmlFile sml_file) {
  std::vector<ObisInfo> obis_info;
  for (int i = 0; i != sml_file.messages.size(); i++) {
    SmlNode message = sml_file.messages[i];
    SmlNode message_body = message.node(3);
    unsigned short message_type = bytes_to_uint(message_body.node(0).value_bytes);
    if (message_type != SML_GET_LIST_RES)
      continue;

    SmlNode get_list_response = message_body.node(1);
    bytes server_id = get_list_response.node(1).value_bytes;
    SmlNode val_list = get_list_response.node(4);

    std::vector<SmlNode> nodes = val_list.nodes();
    for (int j = 0; j != nodes.size(); j++) {
      obis_info.push_back(ObisInfo(server_id, nodes[j]));
    }
  }
  return obis_info;
}
