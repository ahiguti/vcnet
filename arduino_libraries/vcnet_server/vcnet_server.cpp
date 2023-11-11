
#define VCNET_SERIAL Serial
  // ログ出力先

#include <vcnet_server.h>
#include <vcnet_ir.h>

vcnet_server::vcnet_server(IPAddress const& addr0, uint16_t port0)
  : addr(addr0), port(port0), server(port)
{
}

void
vcnet_server::begin(int verbose0)
{
  verbose = verbose0;
  byte mac_addr[] = { 0x02, 0x00, addr[0], addr[1], addr[2], addr[3] };
  IPAddress ip(addr[0], addr[1], addr[2], addr[3]);
  Ethernet.begin(mac_addr, ip);
  server.begin();
  IPAddress localip = Ethernet.localIP();
  VCNET_SERIAL.print("ip ");
  VCNET_SERIAL.print(localip);
  VCNET_SERIAL.print(" ");
  VCNET_SERIAL.println(port);
  vcnet_ir::begin();
}

bool
vcnet_server::fill_read_buffer()
{
  if (!cli) {
    return false;
  }
  int len = cli.available();
  if (!len) {
    return false;
  }
  len = min(len, sizeof(read_buffer) - read_buffer_len);
  if (len == 0) {
    return false;
  }
  int rlen = cli.read(&read_buffer[read_buffer_len], len);
  if (rlen > 0) {
    read_buffer_len += rlen;
    cli_last_access = millis();
    if (verbose > 1) {
      VCNET_SERIAL.print("fill ");
      VCNET_SERIAL.println(rlen);
    }
    ++recv_cnt;
    return true;
  }
  return false;
}

void
vcnet_server::send_stat()
{
  if (!cli) {
    return;
  }
  // { tag0[31:0], tag1[31:0] }
  // tag0 = 0, tag1 = 0x01
  byte status_msg[] = { 0, 0, 0, 0, 1, 0, 0, 0 };
  cli.write(&status_msg[0], 8);
  cli.flush();
  ++send_cnt;
  if (verbose > 0) {
    VCNET_SERIAL.print("send stat c=");
    VCNET_SERIAL.print(conn_cnt);
    VCNET_SERIAL.print(" r=");
    VCNET_SERIAL.print(recv_cnt);
    VCNET_SERIAL.print(" s=");
    VCNET_SERIAL.println(send_cnt);
  }
}

void
vcnet_server::update_regarray(uint8_t const *p, uint16_t len)
{
  if (len != 6) {
    return;
  }
  uint8_t word_offset = p[0]; // regarray中オフセット、32bit単位
  if (word_offset >= sizeof(regarray) / 4) {
    return;
  }
  memcpy(&regarray[word_offset * 4], &p[2], 4);
  if (verbose > 1) {
    VCNET_SERIAL.print(" regarray ");
    VCNET_SERIAL.print(word_offset);
    VCNET_SERIAL.print(" ");
    VCNET_SERIAL.print(p[2], HEX);
    VCNET_SERIAL.print(" ");
    VCNET_SERIAL.print(p[3], HEX);
    VCNET_SERIAL.print(" ");
    VCNET_SERIAL.print(p[4], HEX);
    VCNET_SERIAL.print(" ");
    VCNET_SERIAL.print(p[5], HEX);
    VCNET_SERIAL.println("");
  }
}

void
vcnet_server::parse_frame(uint8_t tag, uint8_t const *p, uint16_t len)
{
  if (verbose > 1) {
    VCNET_SERIAL.print("parse tag=");
    VCNET_SERIAL.print(tag);
    VCNET_SERIAL.print(" len=");
    VCNET_SERIAL.println(len);
  }
  switch (tag) {
  case 0x00:
    send_stat();
    break;
  case 0x03:
    // IR 旧フォーマット
    if (len > 0) {
      vcnet_ir::ir_out(0, p, len);
    }
    break;
  case 0x06:
    // レジスタ配列の更新
    update_regarray(p, len);
    break;
  case 0x08:
    // IR 新フォーマット
    if (len > 2) {
      uint16_t const *p16 = reinterpret_cast<uint16_t const *>(p);
      vcnet_ir::ir_out(p16[0], p + 2, len - 2);
    }
    break;
  default:
    break;
  }
}

void
vcnet_server::parse_read_buffer()
{
  // format: (datalen:1, typ:1, data:?)
  uint16_t p = 0;
  while (p < read_buffer_len) {
    uint16_t datalen = read_buffer[p];
    if (datalen + 2 <= read_buffer_len - p) {
      parse_frame(read_buffer[p + 1], &read_buffer[p + 2], datalen);
      p += datalen + 2;
    } else {
      break;
    }
  }
  if (p != 0) {
    if (p < read_buffer_len) {
      memmove(&read_buffer[0], &read_buffer[p], read_buffer_len - p);
      read_buffer_len -= p;
    } else {
      read_buffer_len = 0;
    }
  }
}

void
vcnet_server::loop()
{
  unsigned long t_now = millis();
  if (!cli) {
    cli = server.available();
    if (cli) {
      VCNET_SERIAL.println("got new client");
      cli_last_access = t_now;
      ++conn_cnt;
      recv_cnt = 0;
      send_cnt = 0;
    }
  } else {
    if (!cli.connected()) {
      cli.stop();
      cli = { };
      VCNET_SERIAL.println("client disconnected");
    } else {
      unsigned long t_diff = t_now - cli_last_access;
      if (t_diff >= 10000) {
        cli.stop();
        cli = { };
        VCNET_SERIAL.println("client timed out");
      }
    }
  }
  if (!cli) {
    return;
  }
  if (verbose > 1) {
    VCNET_SERIAL.println("client available");
  }
  while (fill_read_buffer()) {
    parse_read_buffer();
  }
}

