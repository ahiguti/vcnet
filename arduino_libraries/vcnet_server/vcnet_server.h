
#ifndef VCNET_SERVER_H
#define VCNET_SERVER_H

#include <Ethernet.h>

struct vcnet_server {
  vcnet_server(IPAddress const& addr0, uint16_t port0);
  vcnet_server(vcnet_server const&) = delete;
  vcnet_server& operator =(vcnet_server const&) = delete;
  void begin(int verbose0);
  void loop();
  uint8_t const *get_regarray() const { return &regarray[0]; }
  uint16_t const get_regarray_size() const { return sizeof(regarray); }
private:
  bool fill_read_buffer();
  void send_stat();
  void update_regarray(uint8_t const *p, uint16_t len);
  void parse_frame(uint8_t tag, uint8_t const *p, uint16_t len);
  void parse_frame();
  void parse_read_buffer();
private:
  int verbose = 0;
  IPAddress addr = { };
  uint16_t port = 0;
  EthernetServer server;
  EthernetClient cli;
  unsigned long cli_last_access = 0;
  uint8_t read_buffer[256] = { };
  int read_buffer_len = 0;
  uint8_t regarray[64] = { };
  uint32_t conn_cnt = 0;
  uint32_t send_cnt = 0;
  uint32_t recv_cnt = 0;
};

#endif

