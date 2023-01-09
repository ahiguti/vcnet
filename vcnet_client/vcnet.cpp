// vim: sw=2 ts=8 ai

#ifdef _MSC_VER

#define _CRT_SECURE_NO_WARNINGS 1 // fopen
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#include <SDL_opengl.h>
#include <imm.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "opengl32.lib")
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "libfreetype-6.lib")

#else

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <SDL.h>
#include <SDL_ttf.h>
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#endif

#include <thread>
#include <vector>
#include <fcntl.h>
#include <errno.h>
#include <chrono>
#include <signal.h>
#include <stdint.h>
#include <assert.h>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <locale>
#include <codecvt>
#include <array>
#include <vector>
#include <deque>
#include <atomic>
#include <algorithm>
#include <list>
#include <thread>
#include <thread>
#include <mutex>
#include <iostream>

#ifdef _MSC_VER
#define __attribute__(x)
#define cast_win32(x) (x)
#define socklen_t int
#undef VCNET_USE_OVERLAPPED_IO
#else
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket(x) close(x)
#define SOCKET_ERROR (-1)
#define cast_win32(x)
typedef void *HANDLE;
#endif

namespace vcnet {

static FILE *logfile = nullptr;
static uint64_t logmask = 0x07;

static void
log_valist(const char *fmt, va_list *ap_p)
{
  {
    va_list ap;
    va_copy(ap, *ap_p);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
  }
  if (logfile == nullptr) {
    #ifdef _MSC_VER
    logfile = fopen("vcnet.log", "w");
    // fopen_s(&logfile, "vcnet.log", "a");
    #else
    logfile = fopen("vcnet.log", "w");
    #endif
  }
  if (logfile == nullptr) {
    return;
  }
  {
    va_list ap;
    va_copy(ap, *ap_p);
    vfprintf(logfile, fmt, ap);
    va_end(ap);
  }
  fflush(logfile);
}

static void __attribute__((format (printf, 2, 3)))
log(unsigned level, const char *fmt, ...)
{
  if ((logmask & (1ULL << level)) == 0) {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  log_valist(fmt, &ap);
  va_end(ap);
}

void
set_logmask(uint64_t v)
{
  logmask = v;
}

bool
get_logmask(unsigned b)
{
  return (logmask & (1ULL << b)) != 0;
}

int
get_last_error()
{
  #ifdef _MSC_VER
  return GetLastError();
  #else
  return errno;
  #endif
}

bool
is_retryable_error(int en)
{
  #ifdef _MSC_VER
  if (en == WSAEINTR || en == WSAEWOULDBLOCK || en == WSAENOTCONN) {
    return true;
  }
  #else
  if (en == EINTR || en == EWOULDBLOCK || en == EAGAIN ||
    en == EINPROGRESS) {
    return true;
  }
  #endif
  return false;
}

long
duration_ms(std::chrono::system_clock::time_point t1,
  std::chrono::system_clock::time_point t0)
{
  return (long)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
    .count();
}

std::string
remove_space(std::string s)
{
  while (!s.empty() && std::isspace(s[s.size() - 1u])) {
    s.pop_back();
  }
  while (!s.empty() && std::isspace(s[0])) {
    s = s.substr(1u);
  }
  return s;
}

template <typename T> std::map<std::string, std::string>
parse_map(T&& s, char delim0, char delim1)
{
  std::string line;
  std::map<std::string, std::string> m;
  while (std::getline(s, line, delim0)) {
    line = remove_space(line);
    auto const p = line.find(delim1);
    if (p == line.npos) {
      continue;
    }
    auto k = remove_space(line.substr(0, p));
    auto v = remove_space(line.substr(p + 1));
    m[k] += v;
  }
  return m;
}

std::string
to_hexstr(unsigned long long v)
{
  std::stringstream ss;
  ss << std::hex << v;
  return ss.str();
}

struct auto_socket {
private:
  SOCKET sd = INVALID_SOCKET;
public:
  auto_socket() { }
  auto_socket(int domain, int type, int proto)
    : sd(socket(domain, type, proto)) { }
  auto_socket(auto_socket const&) = delete;
  auto_socket& operator =(auto_socket const&) = delete;
  ~auto_socket() { close(); }
  void close() {
    if (sd != INVALID_SOCKET) {
      ::closesocket(sd);
      sd = INVALID_SOCKET;
    }
  }
  SOCKET get() const { return sd; }
  operator bool() const { return sd != INVALID_SOCKET; }
};

static bool
parse_macaddr(const char *s, std::array<uint8_t, 6>& macaddr_r)
{
  std::string tok;
  std::vector<unsigned char> v;
  std::stringstream sstr(s);
  while (std::getline(sstr, tok, ':')) {
    unsigned long w = strtoul(tok.c_str(), nullptr, 16);
    v.push_back((unsigned char)w);
  }
  if (v.size() != 6) {
    return false;
  }
  for (size_t i = 0; i < 6; ++i) {
    macaddr_r[i] = v[i];
  }
  return true;
}

void
vcnet_send_wol_magic_packet(const char *s)
{
  std::array<uint8_t, 6> macaddr { };
  if (!parse_macaddr(s, macaddr)) {
    log(0, "failed to parse mac address: '%s'\n", s);
    return;
  }
  auto_socket sock(AF_INET, SOCK_DGRAM, 0);
  if (!sock) {
    log(0, "socket: %d\n", get_last_error());
    return;
  }
  std::vector<uint8_t> pkt;
  for (size_t i = 0; i < 6; ++i) {
    pkt.push_back(0xff);
  }
  for (size_t i = 0; i < 16; ++i) {
    pkt.insert(pkt.end(), macaddr.begin(), macaddr.end());
  }
  {
    int v = 1;
    setsockopt(sock.get(), SOL_SOCKET, SO_BROADCAST,
      cast_win32(const char *)&v, (socklen_t)sizeof(v));
  }
  sockaddr_in addr { };
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7);
  addr.sin_addr.s_addr = 0xffffffff;
  log(2, "sending wol magic packet: macaddr=%s\n", s);
  auto e = sendto(sock.get(), cast_win32(const char *)&pkt[0],
    cast_win32(int)pkt.size(),
    0, (sockaddr const *)&addr, sizeof(addr));
  if (e == SOCKET_ERROR) {
    log(0, "sendto: %d\n", get_last_error());
    return;
  }
}

//////////////////////////////////////////////////////////////

struct vcnet_config {
  std::shared_ptr<std::map<std::string, std::string>> mapp;
public:
  vcnet_config();
  vcnet_config(std::ifstream&& f);
  vcnet_config(std::string const& s);
  vcnet_config(std::vector<std::string> const& fns);
  std::string get_str(std::string const& key, std::string const& defval)
    const;
  unsigned long long get_uint(std::string const& key,
    unsigned long long defval) const;
};

vcnet_config::vcnet_config()
{
}

vcnet_config::vcnet_config(std::ifstream&& f)
  : mapp(std::make_shared<std::map<std::string, std::string>>(
    parse_map(std::move(f), '\n', ':')))
{
}

vcnet_config::vcnet_config(std::string const& s)
  : mapp(std::make_shared<std::map<std::string, std::string>>(
    parse_map(std::istringstream(s), ',', '=')))
{
}

vcnet_config::vcnet_config(std::vector<std::string> const& fns)
  : mapp(std::make_shared<std::map<std::string, std::string>>())
{
  for (size_t i = 0; i < fns.size(); ++i) {
    auto m = parse_map(std::ifstream(fns[i]), '\n', ':');
    for (auto iter = m.begin(); iter != m.end(); ++iter) {
      (*mapp)[iter->first] = iter->second;
    }
  }
}

std::string
vcnet_config::get_str(std::string const& key, std::string const& defval) const
{
  if (mapp == nullptr) {
    return defval;
  }
  auto const i = mapp->find(key);
  return (i == mapp->end()) ? defval : i->second;
}

unsigned long long
vcnet_config::get_uint(std::string const& key, unsigned long long defval) const
{
  if (mapp == nullptr) {
    return defval;
  }
  auto const i = mapp->find(key);
  if (i == mapp->end()) {
    return defval;
  }
  return strtoull(i->second.c_str(), nullptr, 0);
}

//////////////////////////////////////////////////////////////

struct vcnet_pixels {
private:
  std::vector<unsigned char> pixels3;
  size_t pixels3_offset = 0;
  unsigned width = 0;
  unsigned height = 0;
  bool is_yuv422 = false;
public:
  void set_yuv422(bool v) { is_yuv422 = v; }
  bool get_yuv422() const { return is_yuv422; }
  unsigned get_bytes_per_pixel() const { return is_yuv422 ? 2 : 3; } 
  unsigned char *begin() { return pixels3.data(); }
  const unsigned char *begin() const { return pixels3.data(); }
  unsigned char *end() { return begin() + size(); }
  const unsigned char *end() const { return begin() + size(); }
  unsigned char *wrpos() { return begin() + pixels3_offset; }
  size_t size() const { return pixels3.size(); }
  size_t wrsize() const { return size() - pixels3_offset; }
  unsigned get_width() const { return width; }
  unsigned get_height() const { return height; }
  void set_wrpos(unsigned char *p);
  void resize_if(unsigned w, unsigned h);
};

void
vcnet_pixels::set_wrpos(unsigned char *p)
{
  size_t o = p - begin();
  if (o <= size()) {
    pixels3_offset = o;
  } else {
    pixels3_offset = 0;
  }
}

void
vcnet_pixels::resize_if(unsigned w, unsigned h)
{
  if (w == width && h == height) {
    return;
  }
  if (w < 2 || h < 2) {
    return;
  }
  width = w;
  height = h;
  log(2, "pixels3 resize %u %u\n", width, height);
  pixels3.resize(width * height * this->get_bytes_per_pixel());
  pixels3_offset = 0;
}

//////////////////////////////////////////////////////////////

struct vcnet_conn {
private:
  vcnet_config conf;
  vcnet_config device_conf;
  uint8_t spi_index = 0x00;
  std::string wol;
  std::string ir;
  bool absmouse = false;
  bool absmousebutton = false;
  bool wheel_dir = false;
  uint32_t ir_sent = 0;
  int socktype = 0;
  SOCKET sd = INVALID_SOCKET;
  std::chrono::system_clock::time_point last_open_time { };
  std::chrono::system_clock::time_point last_read_time { };
  std::chrono::system_clock::time_point last_write_time { };
  std::chrono::system_clock::time_point last_hb_time { };
  std::chrono::system_clock::time_point last_user_input_time { };
  std::chrono::system_clock::time_point last_server_stat_time { };
  std::deque<std::vector<unsigned char>> send_buffer;
  unsigned char recv_buffer[65536] = { };
  size_t recv_buffer_offset = 0;
  uint32_t stat_videoframe_count = 0;
  uint32_t stat_audio_sample_count = 0;
  uint32_t last_resize_videoframe_count = 0; // 信号不良の判定に使う
  bool video_signal_stable = true;
  uint32_t fps = 0;
  uint32_t audio_freq = 0;
  uint32_t prev_audio_sample = 0;
  bool incomplete_videoframe = true;
  uint32_t server_stat = 0;
  uint32_t server_flags = 0x01;
  #ifdef VCNET_USE_OVERLAPPED_IO
  struct ovl_buffer {
    WSAOVERLAPPED ovl { };
    char buffer[6000] { };
  };
  std::deque<std::shared_ptr<ovl_buffer>> ovl_reads;
  #endif
private:
  void open_socket();
  void close_socket();
  size_t write_socket();
  size_t read_socket(void *buf, size_t buflen);
  void parse_netframe(uint32_t tag0, uint32_t tag1, unsigned char *data,
    size_t datalen, vcnet_pixels& pix, bool& videoframe_done_r,
    std::vector<uint32_t>& audiobuf);
  size_t parse_recv_buffer_tcp(vcnet_pixels& pix, bool& videoframe_done_r,
    std::vector<uint32_t>& audiobuf);
  size_t parse_recv_buffer_udp(vcnet_pixels& pix, bool& videoframe_done_r,
    std::vector<uint32_t>& audiobuf);
  short wait_events(int timeout_ms);
  #ifdef VCNET_USE_OVERLAPPED_IO
  void fill_overlapped_reads();
  #endif
public:
  ~vcnet_conn();
  vcnet_conn(vcnet_config const& conf);
  vcnet_conn(vcnet_conn const&) = delete;
  vcnet_conn& operator =(vcnet_conn const&) = delete;
  bool set_target_device(unsigned idx);
  uint8_t get_spi_index() const;
  bool get_absmouse() const;
  bool get_absmousebutton() const;
  bool get_wheel_dir() const;
  void send_wol();
  void send_netframe(uint8_t typ, const void *data, uint8_t data_len,
    bool is_user_input = true);
  void send_raw(const void *data, size_t data_len, bool has_event);
  void send_heartbeat_if();
  void send_ir(const char *s);
  void send_system_reset();
  void send_server_flags(uint32_t v);
  void send_adv7611_i2c(uint8_t i2c_addr7, uint8_t reg, uint8_t val);
  uint32_t get_server_flags() const;
  void recv_data(vcnet_pixels& px, bool& videoframe_done_r, 
    bool& has_net_event_r, std::vector<uint32_t>& audiobuf);
  void increment_videoframe_count();
  void increment_audio_sample_count(uint32_t v);
  uint32_t get_server_stat() const;
  uint32_t get_fps() const;
  uint32_t get_audio_freq() const;
  bool is_udp() const;
  bool get_video_signal_stable() const;
};

vcnet_conn::~vcnet_conn()
{
  close_socket();
}

vcnet_conn::vcnet_conn(vcnet_config const& conf0)
  : conf(conf0)
{
  last_open_time = last_user_input_time = std::chrono::system_clock::now();
  last_open_time -= std::chrono::seconds(10);
  set_target_device(0);
}

bool
vcnet_conn::set_target_device(unsigned idx)
{
  auto s = conf.get_str("device" + std::to_string(idx), std::string());
  if (s.empty()) {
    return false;
  }
  device_conf = vcnet_config(s);
  spi_index = (uint8_t)device_conf.get_uint("spi", 0x00);
  wol = device_conf.get_str("wol", "");
  ir = device_conf.get_str("ir", "");
  absmouse = (bool)device_conf.get_uint("absmouse", 0);
  absmousebutton = (bool)device_conf.get_uint("absmousebutton", 0);
  wheel_dir = (bool)device_conf.get_uint("wheel_dir", 0);
  log(2, "vcnet_conn spi=%u wol=%s ir=%s absmouse=%u absmb=%u wheel=%u\n",
    spi_index, wol.c_str(), ir.c_str(), (unsigned)absmouse,
    (unsigned)absmousebutton, (unsigned)wheel_dir);
  send_wol();
  ir_sent = 0;
  return true;
}

uint8_t
vcnet_conn::get_spi_index() const
{
  return spi_index;
}

bool
vcnet_conn::get_absmouse() const
{
  return absmouse;
}

bool
vcnet_conn::get_absmousebutton() const
{
  return absmousebutton;
}

bool
vcnet_conn::get_wheel_dir() const
{
  return wheel_dir;
}

void
vcnet_conn::send_wol()
{
  if (!wol.empty()) {
    vcnet_send_wol_magic_packet(wol.c_str());
  }
}

void
vcnet_conn::close_socket()
{
  if (sd >= 0) {
    #ifdef VCNET_USE_OVERLAPPED_IO
    if (!CancelIoEx((HANDLE)sd, nullptr)) {
      int e = GetLastError();
      log(1, "cancelioex %d\n", e);
    }
    while (!ovl_reads.empty()) {
      auto& osp = ovl_reads.front();
      auto& o = *osp;
      DWORD flags = 0;
      DWORD len = 0;
      BOOL r = WSAGetOverlappedResult(sd, &o.ovl, &len, FALSE, &flags);
      if (!r) {
        int e = WSAGetLastError();
        if (e != ERROR_OPERATION_ABORTED) {
          log(1, "wsagetoverlappedresult %d\n", e);
        }
      }
      ovl_reads.pop_front();
    }
    #endif
    closesocket(sd);
  }
  log(2, "close_socket\n");
  sd = INVALID_SOCKET;
}

void
vcnet_conn::open_socket()
{
  log(2, "open_socket\n");
  last_open_time = last_read_time = last_write_time
    = last_hb_time = std::chrono::system_clock::now();
  recv_buffer_offset = 0;
  send_buffer.clear();
  stat_videoframe_count = 0;
  stat_audio_sample_count = 0;
  last_resize_videoframe_count = 0;
  video_signal_stable = true;
  fps = 0;
  audio_freq = 0;
  incomplete_videoframe = true;
  auto st = device_conf.get_str("socktype", "tcp");
  socktype = 0;
  if (st == "tcp") {
    socktype = SOCK_STREAM;
  } else if (st == "udp") {
    socktype = SOCK_DGRAM;
  } else {
    log(0, "socket: unknown type '%s'\n", st.c_str());
    return;
  }
  sd = socket(AF_INET, socktype, 0);
  if (sd == INVALID_SOCKET) {
    log(0, "socket: %d\n", get_last_error());
    close_socket();
    return;
  }
  sockaddr_in addr = { };
  addr.sin_family = AF_INET;
  unsigned port = (unsigned)device_conf.get_uint("port", 5001);
  addr.sin_port = htons(port);
  auto ip = device_conf.get_str("ip", "192.168.250.250");
  log(2, "vcnet_conn ip=%s port=%u\n", ip.c_str(), port);
  #ifdef _MSC_VER
  InetPtonA(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
  #else
  addr.sin_addr.s_addr = inet_addr(ip.c_str());
  #endif
  #ifdef _MSC_VER
  unsigned long mode = 1;
  if (ioctlsocket(sd, FIONBIO, &mode) != 0) {
      log(0, "setfl nonblock: %d\n", get_last_error());
      close_socket();
      return;
  }
  #else
  if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0) {
    log(0, "setfl nonblock: %d\n", get_last_error());
    close_socket();
    return;
  }
  #endif
  {
    if (socktype == SOCK_STREAM) {
      int v = 1024 * 1024;
      setsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v));
      v = 1;
      setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, cast_win32(const char *)&v,
        (socklen_t)sizeof(v));
    } else {
      int v = 1024 * 1024 * 1024;
      if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
      v = 0;
      socklen_t len = (socklen_t)sizeof(v);
      if (getsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(char *)&v,
        &len) != 0) {
        log(0, "getsockopt: %d\n", get_last_error());
      }
      log(2, "rcvbuf %d\n", v);
    }
  }
  if (connect(sd, (const sockaddr *)&addr, (socklen_t)sizeof(addr)) != 0) {
    int en = get_last_error();
    if (!is_retryable_error(en)) {
      log(0, "failed to connect\r\n");
      close_socket();
      return;
    }
  }
  log(2, "connected\n");
}

// 送信TCP/UDPフレームのフォーマット
// +0 payload_len[7:0]
// +1 type[7:0]
// +2 payload...
// typeの意味はvcnet_artyz7/vcnet/src/vcnet_main.c:vcnet_recv_cb()参照
// 0: stat送信要求
// 1: マイコンへのi2cコマンド(USBマウス等)(廃止)
// 2: サーバのリセット要求
// 3: irコマンド
// 4: サーバフラグの書き換え
// 5: サーバ内i2cコマンド(adv7611等)

void
vcnet_conn::send_netframe(uint8_t typ, const void *data, uint8_t data_len,
  bool is_user_input)
{
  const unsigned char *p = (const unsigned char *)data;
  std::vector<unsigned char> buf;
  buf.push_back(data_len);
  buf.push_back(typ);
  buf.insert(buf.end(), p, p + data_len);
  send_buffer.push_back(std::move(buf));
  if (is_user_input) {
    last_user_input_time = std::chrono::system_clock::now();
  }
}

void
vcnet_conn::send_raw(const void *data, size_t data_len, bool has_event)
{
  const unsigned char *const p = (const unsigned char *)data;
  std::vector<unsigned char> buf;
  buf.insert(buf.end(), p, p + data_len);
  send_buffer.push_back(std::move(buf));
  if (has_event) {
    // マウス操作などのイベントが有った。必要なら再接続がおきる。
    last_user_input_time = std::chrono::system_clock::now();
  }
}

void
vcnet_conn::send_heartbeat_if()
{
  auto t = std::chrono::system_clock::now();
  auto d = duration_ms(t, last_hb_time);
  if (d >= 1000) {
    send_netframe(0, nullptr, 0, false); // heartbeat
    last_hb_time = t;
    log(2, "%u fps %u hz %u\n", stat_videoframe_count, stat_audio_sample_count,
      (unsigned)d);
    fps = stat_videoframe_count;
    stat_videoframe_count = 0;
    audio_freq = stat_audio_sample_count;
    stat_audio_sample_count = 0;
    if (duration_ms(t, last_open_time) < 5000) {
      send_ir(ir.c_str());
    }
  }
}

void
vcnet_conn::send_system_reset()
{
  log(6, "sending system reset\n");
  send_netframe(2, nullptr, 0);
}

void
vcnet_conn::send_server_flags(uint32_t v)
{
  char buf[4];
  memcpy(buf, &v, 4);
  send_netframe(4, buf, 4);
  server_flags = v;
}

void
vcnet_conn::send_adv7611_i2c(uint8_t i2c_addr7, uint8_t reg, uint8_t val)
{
  char buf[3];
  buf[0] = i2c_addr7;
  buf[1] = reg;
  buf[2] = val;
  send_netframe(5, buf, 3);
}

uint32_t
vcnet_conn::get_server_flags() const
{
  return server_flags;
}

void
vcnet_conn::send_ir(const char *s)
{
  std::string tok;
  std::vector<uint16_t> vs;
  std::stringstream sstr(s);
  while (std::getline(sstr, tok, ':')) {
    unsigned long v = strtoul(tok.c_str(), nullptr, 10);
    if (v != 0) {
      vs.push_back((uint16_t)v);
    }
  }
  if (vs.size() > 120) {
    log(0, "ir command too long\n");
  } else {
    send_netframe(3, vs.data(), (uint8_t)(vs.size() * 2), false);
    log(2, "sent ir command %u\n", (unsigned)vs.size());
  }
}

void
vcnet_conn::increment_videoframe_count()
{
  ++stat_videoframe_count;
}

void
vcnet_conn::increment_audio_sample_count(uint32_t v)
{
  stat_audio_sample_count += v;
}

size_t
vcnet_conn::write_socket()
{
  if (sd == INVALID_SOCKET) {
    return 0;
  }
  size_t r = 0;
  while (!send_buffer.empty()) {
    auto& buf = send_buffer.front();
    auto e = send(sd, cast_win32(const char *)(buf.data()),
      cast_win32(int)buf.size(), 0);
    if (e < 0) {
      int en = get_last_error();
      if (is_retryable_error(en)) {
        return 0;
      } else {
        close_socket();
        log(0, "write_socket: %d\n", en);
        return 0;
      }
    } else if (e == 0) {
      log(0, "write_socket: closed\n");
      close_socket();
      return 0;
    } else {
      buf.erase(buf.begin(), buf.begin() + e);
      if (buf.empty()) {
        send_buffer.pop_front();
      }
      last_write_time = std::chrono::system_clock::now();
      log(7, "socket wrote %zd\n", e);
      ++r;
    }
  }
  return r;
}

#ifdef VCNET_USE_OVERLAPPED_IO
void
vcnet_conn::fill_overlapped_reads()
{
  if (sd == INVALID_SOCKET) {
    return;
  }
  while (ovl_reads.size() < 8192) {
    std::shared_ptr<ovl_buffer> sp = std::make_shared<ovl_buffer>();
    ovl_reads.push_back(sp);
    auto& o = *sp;
    WSABUF buf;
    buf.len = sizeof(o.buffer);
    buf.buf = o.buffer;
    DWORD flags = 0;
    if (WSARecv(sd, &buf, 1, nullptr, &flags, &o.ovl, nullptr) != 0) {
      int e = WSAGetLastError();
      if (e != ERROR_IO_PENDING) {
        log(1, "wsarecv: %d\n", WSAGetLastError());
        ovl_reads.pop_back();
        close_socket();
        return;
      }
    }
  }
}
#endif

size_t
vcnet_conn::read_socket(void *buf, size_t buflen)
{
  if (sd == INVALID_SOCKET) {
    return 0;
  }
  #ifdef VCNET_USE_OVERLAPPED_IO
  DWORD len = 0;
  if (!ovl_reads.empty()) {
    auto& sp = ovl_reads.front();
    auto& o = *sp;
    DWORD flags = 0;
    BOOL r = WSAGetOverlappedResult(sd, &o.ovl, &len, FALSE, &flags);
    if (!r) {
      int e = WSAGetLastError();
      if (e == WSA_IO_INCOMPLETE) {
        return 0;
      } else {
        log(1, "wsagetoverlappedresult %d\n", e);
        close_socket();
        return 0;
      }
    }
    if (size_t(len) > buflen) {
      len = (DWORD)buflen;
    }
    memcpy(buf, o.buffer, len);
    ovl_reads.pop_front();
  }
  fill_overlapped_reads();
  return size_t(len);
  #else
  auto e = recv(sd, cast_win32(char *)buf, cast_win32(int)buflen, 0);
  if (e < 0) {
    int en = get_last_error();
    if (is_retryable_error(en)) {
      return 0;
    } else {
      last_open_time = std::chrono::system_clock::now();
      close_socket();
      log(0, "read_socket: %d\n", en);
      return 0;
    }
  } else if (e == 0) {
    log(0, "read_socket: closed\n");
    close_socket();
    return 0;
  }
  return e;
  #endif
}

short
vcnet_conn::wait_events(int timeout_ms)
{
  if (sd == INVALID_SOCKET) {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    return 0;
  }
  short ev = POLLIN;
  if (!send_buffer.empty()) {
    ev |= POLLOUT;
  }
  pollfd fds[1] = { };
  fds[0].fd = sd;
  fds[0].events = ev;
  #ifdef _MSC_VER
  int e = WSAPoll(&fds[0], 1, timeout_ms);
  #else
  int e = poll(&fds[0], 1, timeout_ms);
  #endif
  if (e >= 1) {
    if ((fds[0].revents & ev) != 0) {
      return fds[0].revents;
    }
  }
  return 0;
}

void
vcnet_conn::parse_netframe(uint32_t tag0, uint32_t tag1,
  unsigned char *data, size_t datalen, vcnet_pixels& pix,
  bool& videoframe_done_r, std::vector<uint32_t>& audiobuf)
{
  if (tag0 != 0x01) {
    // TODO: implement
    if (tag0 == 0x00) {
      log(2, "got stat message %x\n", tag1);
      auto t = std::chrono::system_clock::now();
      last_server_stat_time = t;
      server_stat = tag1;
      auto video_locked = server_stat & 0x04;
      if (ir_sent < 1 ||
        (!video_locked && duration_ms(t, last_user_input_time) < 10000)) {
        send_server_flags(server_flags);
        send_ir(ir.c_str());
        send_wol();
        ++ir_sent;
      }
    }
    if (tag0 == 0x02) {
      log(3, "got audio %zu\n", datalen);
      uint32_t const *const p = (uint32_t *)data;
      size_t plen = datalen / 4;
      for (size_t i = 0; i < plen; ++i) {
        audiobuf.push_back(p[i]);
      }
    }
    return;
  }
  uint32_t fr_width = tag1 & 0xffffu;
  uint32_t fr_height = (tag1 >> 16u) & 0x1fffu;
  uint32_t fr_last = (tag1 & 0x80000000u);
  uint32_t fr_skipline = ((tag1 & 0x40000000u) != 0) ? 1u : 0u;
  uint32_t fr_yuv422 = (tag1 & 0x20000000u);
  if (fr_height >= 8192 || fr_width >= 4096 ||
    fr_height == 0 || fr_width == 0) {
    log(0, "corrupted data tag0=%x tag1=%x w=%u h=%u\n", (unsigned)tag0,
      (unsigned)tag1, (unsigned)fr_width, (unsigned)fr_height);
    pix.set_wrpos(pix.begin());
    close_socket();
    return;
  }
  if (fr_width != pix.get_width() || fr_height != pix.get_height()) {
    uint32_t const fdiff = stat_videoframe_count
      - last_resize_videoframe_count;
    video_signal_stable = (fdiff >= 60 || pix.get_width() == 0);
      // 短い時間に2回解像度が変化したときはvideo_signal_stableをfalseにする
    last_resize_videoframe_count = stat_videoframe_count;
    pix.set_yuv422(fr_yuv422 != 0);
    pix.resize_if(fr_width, fr_height);
    log(2, "fdiff=%u stable=%d fr_yuv422=%x\n", (unsigned)fdiff,
      (int)video_signal_stable, (unsigned)fr_yuv422);
  } else if (!video_signal_stable) {
    uint32_t const fdiff = stat_videoframe_count
      - last_resize_videoframe_count;
    if (fdiff >= 60) {
      video_signal_stable = true;
    }
  }
  uint32_t rboffset = 0;
  while (rboffset < datalen) {
    uint32_t const line_nbytes = fr_width * pix.get_bytes_per_pixel();
    size_t dst_offset = line_nbytes * fr_skipline;
    size_t src_offset = rboffset;
    size_t cpy_size = line_nbytes;
    if (dst_offset + cpy_size > pix.wrsize()) {
      // skip memcpy
    } else if (src_offset + cpy_size > datalen) {
      // skip memcpy
    } else if (incomplete_videoframe) {
      // skip memcpy
    } else {
      memcpy(pix.wrpos() + dst_offset, data + src_offset, cpy_size);
    }
    rboffset += (uint32_t)cpy_size;
    pix.set_wrpos(pix.wrpos() + dst_offset + cpy_size);
  }
  if (fr_last != 0) {
    pix.set_wrpos(pix.begin());
    videoframe_done_r = true;
    incomplete_videoframe = false;
    log(8, "videoframe_done\n");
  }
}


// 受信TCPフレームのフォーマット
// +0 tag0[7:0], frlen[23:0]
// +4 tag1[31:0]
// +8 payload...

size_t
vcnet_conn::parse_recv_buffer_tcp(vcnet_pixels& pix, bool& videoframe_done_r,
  std::vector<uint32_t>& audiobuf)
{
  unsigned char *rbpos = &recv_buffer[0];
  unsigned char *const rbpos_end = &recv_buffer[recv_buffer_offset];
  log(8, "parse_recv_buffer_tcp (%p,%p)\n", rbpos, rbpos_end);
  while (true) {
    if (rbpos + 8 > rbpos_end) {
      break;
    }
    uint32_t tag0_frlen = ((uint32_t const *)rbpos)[0];
    uint32_t tag1 = ((uint32_t const *)rbpos)[1];
    uint32_t tag0 = tag0_frlen >> 24u;
    uint32_t fr_nbytes = tag0_frlen & 0x00ffffffu;
    if (rbpos + fr_nbytes + 8 > rbpos_end) {
      if (rbpos == &recv_buffer[0] &&
        recv_buffer_offset == sizeof(recv_buffer)) {
        log(0, "netframe too large %x\n", (unsigned)fr_nbytes);
        pix.set_wrpos(pix.begin());
        close_socket();
        return 0;
      }
      break;
    }
    // consumes fr_nbytes + 8 bytes
    parse_netframe(tag0, tag1, rbpos + 8, fr_nbytes, pix, videoframe_done_r,
      audiobuf);
    rbpos += fr_nbytes + 8;
    assert(rbpos - &recv_buffer[0] <= (long)recv_buffer_offset);
    assert(rbpos <= rbpos_end);
    if (videoframe_done_r) {
      break;
    }
  }
  assert(rbpos - &recv_buffer[0] <= (long)recv_buffer_offset);
  return rbpos - &recv_buffer[0];
}

// 受信UDPパケットのフォーマット
// (1024バイト以上のとき)
//   +0 line_number[15:0], framelen[15:0]
//   +4 type[7:0], 24'h0
//   +8 payload...
// (1024バイト未満のとき)
//   送信したUDPパケットがそのままechoされて返ってくる

size_t
vcnet_conn::parse_recv_buffer_udp(vcnet_pixels& pix, bool& videoframe_done_r,
  std::vector<uint32_t>& audiobuf)
{
  size_t const len = recv_buffer_offset;
  unsigned char *rbpos = &recv_buffer[0];
  unsigned char *const rbpos_end = &recv_buffer[recv_buffer_offset];
  log(8, "parse_recv_buffer_udp (%p,%p)\n", rbpos, rbpos_end);
  if (len < 1024) {
    // エコーバックされてきたパケットがあれば接続はつながっている
    last_server_stat_time = std::chrono::system_clock::now();
    server_stat = 0x0c;
    return len;
  }
  uint64_t const hdr = ((uint64_t const *)rbpos)[0];
  uint8_t const typ = hdr >> 56u;
  uint32_t const payload_len = hdr & 0xffffu;
  if (len != payload_len) {
    log(1, "parse_recv_buffer_udp invalid len v=%llx\n",
      (unsigned long long)hdr);
    return len;
  }
  if (typ == 0) {
    // log(0, "parse_recv_buffer_udp v=%llx\n", (unsigned long long)hdr);
    uint32_t fr_width = (pix.get_yuv422())
      ? (payload_len - 8) / 2
      : (payload_len - 8) / 3;
    uint32_t fr_height = (hdr >> 32u) & 0xffffu;
    uint32_t line_number = (hdr >> 16u) & 0xffffu;
    // log(0, "parse_recv_buffer_udp line=%u\n", (unsigned)line_number);
    if (line_number >= fr_height) {
      // log(0, "parse_recv_buffer_udp line=%u\n", (unsigned)line_number);
      return len;
    }
    uint32_t fr_last = (line_number == fr_height - 1) ? 0x80000000u : 0u;
    uint32_t interlaced = (hdr >> 54) & 1;
    uint32_t odd_frame = (hdr >> 55) & 1;
    if (interlaced) {
      fr_height *= 2;
      line_number = line_number * 2 + odd_frame;
    }
    server_flags = (server_flags & ~0x01) | interlaced;
    uint32_t tag0 = 1u;
    uint32_t tag1 = fr_width | (fr_height << 16) | fr_last;
    {
      unsigned char *const curpos = pix.wrpos();
      unsigned char *const pos = pix.begin() +
        line_number * pix.get_bytes_per_pixel() * fr_width;
      if (pos != curpos) {
        if (pos > curpos) {
          size_t diff = pos - curpos;
          if (!interlaced || diff != fr_width * pix.get_bytes_per_pixel()) {
            log(2, "drop %zu lines\n",
              diff / (fr_width / pix.get_bytes_per_pixel()));
          }
        } else {
          log(2, "drop frame\n");
        }
      }
      pix.set_wrpos(pos);
    }
    // consumes fr_nbytes + 8 bytes
    parse_netframe(tag0, tag1, rbpos + 8, len - 8, pix, videoframe_done_r,
      audiobuf);
    if (videoframe_done_r) {
      return len;
    }
  } else {
    /* (i2s_axis.v) s_tdata <= { data[31:8], lr[0:0], 1'b0, nbits[5:0] } */
    size_t const datalen = len - 8;
    unsigned char *const data = rbpos + 8;
    log(5, "got audio %zu\n", datalen);
    uint32_t const *const p = (uint32_t *)data;
    size_t plen = datalen / 4;
    for (size_t i = 0; i < plen; ++i) {
      uint32_t v = p[i];
      if ((v & 0x80) == 0) {
        prev_audio_sample = v;
      } else {
        audiobuf.push_back((prev_audio_sample >> 16) | (v & 0xffff0000));
      }
    }
  }
  return len;
}

void
vcnet_conn::recv_data(vcnet_pixels& pix, bool& videoframe_done_r,
  bool& has_net_event_r, std::vector<uint32_t>& audiobuf)
{
  has_net_event_r = false;
  videoframe_done_r = false;
  // static std::chrono::system_clock::time_point prev_t =
  //   std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::now();
  // if (duration_ms(t, prev_t) > 5) {
  //   log(0, "recvdata duration %ld\n", duration_ms(t, prev_t));
  // }
  // prev_t = t;
  if (sd == INVALID_SOCKET) {
    log(8, "recv_data: %ld %ld\n", duration_ms(t, last_open_time),
      duration_ms(t, last_user_input_time));
    if (duration_ms(t, last_open_time) > 3000 &&
      duration_ms(t, last_user_input_time) < 60000) {
      open_socket();
    }
  } else {
    auto t = std::chrono::system_clock::now();
    if (duration_ms(t, last_read_time) > 3000) {
      log(0, "recv_data: timeout\n");
      close_socket();
      return;
    }
    send_heartbeat_if();
  }
  short rev = wait_events(1);
  if ((rev & POLLOUT) != 0)
  {
    size_t wcnt = write_socket();
    if (wcnt > 0) {
      has_net_event_r = true;
    }
  }
  if ((rev & POLLIN) != 0) {
    if (socktype == SOCK_STREAM) {
      while (true) {
        if (recv_buffer_offset < sizeof(recv_buffer)) {
          size_t rlen = read_socket(recv_buffer + recv_buffer_offset,
            sizeof(recv_buffer) - recv_buffer_offset);
          if (rlen == 0) {
            break;
          }
          last_read_time = std::chrono::system_clock::now();
          recv_buffer_offset += rlen;
          has_net_event_r = true;
          assert(recv_buffer_offset <= sizeof(recv_buffer));
        }
        if (recv_buffer_offset < 8) {
          break;
        }
        size_t const consumed_size = parse_recv_buffer_tcp(pix,
          videoframe_done_r, audiobuf);
        assert(consumed_size <= recv_buffer_offset);
        if (consumed_size != 0) {
          size_t rem_size = recv_buffer_offset - consumed_size;
          if (rem_size != 0) {
            memmove(&recv_buffer[0], &recv_buffer[0] + consumed_size,
              rem_size);
          }
          recv_buffer_offset = rem_size;
          assert(recv_buffer_offset < sizeof(recv_buffer));
        }
        if (videoframe_done_r) {
          break;
        }
      }
    } else if (socktype == SOCK_DGRAM) {
      while (true) {
        size_t rlen = read_socket(recv_buffer, sizeof(recv_buffer));
        if (rlen == 0) {
          break;
        }
        last_read_time = std::chrono::system_clock::now();
        recv_buffer_offset = rlen;
        has_net_event_r = true;
        parse_recv_buffer_udp(pix, videoframe_done_r, audiobuf);
        recv_buffer_offset = 0;
        if (videoframe_done_r) {
          break;
        }
      }
    }
  }
  return;
}

uint32_t
vcnet_conn::get_server_stat() const
{
  auto now = std::chrono::system_clock::now();
  if (duration_ms(now, last_server_stat_time) > 5000) {
    return 0;
  }
  return server_stat;
}

uint32_t
vcnet_conn::get_fps() const
{
  return fps;
}

uint32_t
vcnet_conn::get_audio_freq() const
{
  return audio_freq;
}

bool
vcnet_conn::is_udp() const
{
  return socktype == SOCK_DGRAM;
}

bool
vcnet_conn::get_video_signal_stable() const
{
  return video_signal_stable;
}

//////////////////////////////////////////////////////////////

struct vcnet_hid {
private:
  bool has_event = false;
  bool mouse_moved = false;
  uint8_t mbutton = 0;
  uint8_t mouse_seq = 0;
  std::array<int, 4> mouse_delta = { };
  std::array<uint16_t, 2> absmouse = { };
  uint8_t absmbutton = 0;
  std::set<uint8_t> keys;
  uint8_t key_mod = 0;
  uint16_t gcbutton = 0;
  std::array<int16_t, 6> gcaxes = { };
  void push(std::vector<uint8_t>& buf_a, uint8_t word_offset, uint8_t v0,
    uint8_t v1, uint8_t v2, uint8_t v3);
public:
  void mouse_button(uint8_t v, bool press);
  void mouse_move(int x, int y, int wx, int wy);
  void absmouse_button(uint8_t v, bool press);
  void absmouse_move(uint16_t x, uint16_t y);
  void key_down(uint8_t k, uint8_t m);
  void key_up(uint8_t k, uint8_t m);
  void key_up_all();
  void gc_buttons(uint16_t v);
  void gc_axis(uint8_t axis, int16_t v);
  void get_spi_buffer(uint8_t spi_index, std::vector<uint8_t>& buf_a,
    bool& has_event_r);
  void up_all();
};

void
vcnet_hid::mouse_button(uint8_t v, bool press)
{
  if (press) {
    mbutton |= (1 << v);
  } else {
    mbutton &= ~(1 << v);
  }
  log(9, "mouse_button %u\n", (unsigned)mbutton);
}

void
vcnet_hid::mouse_move(int x, int y, int wx, int wy)
{
  if (x != 0 || y != 0 || wx != 0 || wy != 0) {
    if (!mouse_moved) {
      mouse_delta[0] = 0;
      mouse_delta[1] = 0;
      mouse_delta[2] = 0;
      mouse_delta[3] = 0;
    }
    mouse_delta[0] += x;
    mouse_delta[1] += y;
    mouse_delta[2] += wx;
    mouse_delta[3] += wy;
    mouse_moved = true;
    has_event = true;
  }
}

void
vcnet_hid::absmouse_button(uint8_t v, bool press)
{
  if (press) {
    absmbutton |= (1 << v);
  } else {
    absmbutton &= ~(1 << v);
  }
  has_event = true;
}

void
vcnet_hid::absmouse_move(uint16_t x, uint16_t y)
{
  absmouse = { x, y };
  has_event = true;
}

void
vcnet_hid::key_down(uint8_t k, uint8_t m)
{
  keys.insert(k);
  key_mod = m;
  has_event = true;
  log(10, "key_down %u %u\n", (unsigned)k, (unsigned)m);
}

void
vcnet_hid::key_up(uint8_t k, uint8_t m)
{
  keys.erase(k);
  if (keys.empty()) {
    key_mod = 0;
  }
  has_event = true;
  log(10, "key_up %u %u\n", (unsigned)k, (unsigned)m);
}

void
vcnet_hid::key_up_all()
{
  keys.clear();
  key_mod = 0;
  has_event = true;
}

void
vcnet_hid::up_all()
{
  log(0, "up_all\n");
  keys.clear();
  key_mod = 0;
  mbutton = 0;
  absmbutton = 0;
  gcbutton = 0;
  has_event = true;
}

void
vcnet_hid::gc_buttons(uint16_t v)
{
  gcbutton = v;
  has_event = true;
}

void
vcnet_hid::gc_axis(uint8_t axis, int16_t v)
{
  if (axis < gcaxes.size()) {
    gcaxes[axis] = v;
    has_event = true;
  }
}

void
vcnet_hid::push(std::vector<uint8_t>& buf_a, uint8_t word_offset, uint8_t v0,
  uint8_t v1, uint8_t v2, uint8_t v3)
{
  buf_a.push_back(0x06);
  buf_a.push_back(0x06);
  buf_a.push_back(word_offset); // 4byte単位
  buf_a.push_back(0);
  buf_a.push_back(v0);
  buf_a.push_back(v1);
  buf_a.push_back(v2);
  buf_a.push_back(v3);
  log(11, "hid push (%x) %x %x %x %x\n", (unsigned)word_offset,
    (unsigned)v0,
    (unsigned)v1,
    (unsigned)v2,
    (unsigned)v3);
}

void
vcnet_hid::get_spi_buffer(uint8_t spi_index, std::vector<uint8_t>& buf_a,
  bool& has_event_r)
{
  auto const clamp_int8 = [](int v) -> int {
    return (v <= -128) ? -128 : (v >= 127) ? 127 : v;
  };
  if (mouse_moved) {
    ++mouse_seq;
    mouse_moved = false;
  }
  push(buf_a, 0, spi_index, mouse_seq,
    clamp_int8(mouse_delta[0]), clamp_int8(mouse_delta[1]));
  push(buf_a, 1, clamp_int8(mouse_delta[2]), clamp_int8(mouse_delta[3]),
    (uint8_t)absmouse[0], absmouse[0] >> 8);
  uint8_t mb = (absmbutton != 0) ? (absmbutton | 0x80) : (mbutton & 0x7f);
  push(buf_a, 2, (uint8_t)absmouse[1], absmouse[1] >> 8, mb, key_mod);
  std::array<uint8_t, 6> ka = { };
  size_t i = 0;
  for (auto j = keys.begin(); j != keys.end(); ++j) {
    if (i < ka.size()) {
      ka[i++] = *j;
    }
  }
  push(buf_a, 3, ka[0], ka[1], ka[2], ka[3]);
  push(buf_a, 4, ka[4], ka[5], (uint8_t)gcbutton,
    (uint8_t)(gcbutton >> 8));
  push(buf_a, 5, (uint8_t)(gcaxes[0]), (uint8_t)(gcaxes[0] >> 8),
    (uint8_t)gcaxes[1], (uint8_t)(gcaxes[1] >> 8));
  push(buf_a, 6, (uint8_t)(gcaxes[2]), (uint8_t)(gcaxes[2] >> 8),
    (uint8_t)gcaxes[3], (uint8_t)(gcaxes[3] >> 8));
  push(buf_a, 7, (uint8_t)(gcaxes[4]), (uint8_t)(gcaxes[4] >> 8),
    (uint8_t)gcaxes[5], (uint8_t)(gcaxes[5] >> 8));
  has_event_r = has_event;
  has_event = false;
}


//////////////////////////////////////////////////////////////

enum joy_type_e {
  joy_type_e_none = 0,
  joy_type_e_button = 1,
  joy_type_e_hat = 2,
  joy_type_e_axis = 3,
};

typedef std::pair<joy_type_e, uint32_t> joy_entry;
typedef std::map<joy_entry, std::vector<std::pair<joy_entry, int>>>
  joymap_type;

struct vcnet_joystick {
private:
  SDL_Joystick *inst = nullptr;
  std::array<uint8_t, 4> buttons = { };
  std::array<uint8_t, 2> hats = { };
  std::array<int16_t, 7> axes = { };
  vcnet_joystick(vcnet_joystick const&) = delete;
  vcnet_joystick& operator =(vcnet_joystick const&) = delete;
public:
  vcnet_joystick();
  ~vcnet_joystick();
  void close();
  void open(int device_index);
  void update(vcnet_hid& hid, joymap_type const& joymap);
};

vcnet_joystick::vcnet_joystick()
{
  close();
}

vcnet_joystick::~vcnet_joystick()
{
  close();
}

void
vcnet_joystick::close()
{
  if (inst != nullptr) {
    SDL_JoystickClose(inst);
    inst = nullptr;
  }
  buttons = { };
  for (size_t i = 0; i < hats.size(); ++i) {
    hats[i] = 8;
  }
  for (size_t i = 0; i < axes.size(); ++i) {
    axes[i] = -32768;
  }
}

void
vcnet_joystick::open(int device_index)
{
  close();
  inst = SDL_JoystickOpen(device_index);
}

void
vcnet_joystick::update(vcnet_hid& hid, joymap_type const& joymap)
{
  if (inst == nullptr) {
    return;
  }
  int num_buttons = SDL_JoystickNumButtons(inst);
  num_buttons = (num_buttons < 12) ? num_buttons : 12;
  int num_axes = SDL_JoystickNumAxes(inst);
  num_axes = (num_axes < 6) ? num_axes : 6;
  int num_hats = SDL_JoystickNumHats(inst);
  num_hats = (num_hats < 1) ? num_hats : 1;
  std::vector<bool> vbutton(num_buttons);
  std::vector<bool> vhat(num_hats * 4);
  std::vector<int16_t> vaxis(num_axes);
  for (int i = 0; i < num_axes; ++i) {
    int16_t v = SDL_JoystickGetAxis(inst, i);
    vaxis[i] = v;
  }
  for (int i = 0; i < num_hats; ++i) {
    uint16_t v = SDL_JoystickGetHat(inst, i);
    for (int j = 0; j < 4; ++j) {
      vhat[i * 4 + j] = ((v & (1 << j)) != 0);
    }
  }
  for (int i = 0; i < num_buttons; ++i) {
    uint8_t v = SDL_JoystickGetButton(inst, i) & 0x01;
    vbutton[i] = (v != 0);
  }
  auto get_bool = [&](std::pair<joy_entry, int> const& ep) -> bool {
    auto ty = ep.first.first;
    auto idx = ep.first.second;
    auto param = ep.second;
    switch (ty) {
    case joy_type_e_button:
      return idx < vbutton.size() ? vbutton[idx] : false;
    case joy_type_e_hat:
      return idx < vhat.size() ? vhat[idx] : false;
    case joy_type_e_axis:
      {
        auto v = idx < vaxis.size() ? vaxis[idx] : 0;
        if (param > 0) {
          return v >= param;
        } else if (param < 0) {
          return v < param;
        }
      }
      return false;
    case joy_type_e_none:
      return param != 0;
    default: return false;
    }
  };
  auto get_int = [&](std::pair<joy_entry, int> const& ep) -> int32_t {
    auto ty = ep.first.first;
    auto idx = ep.first.second;
    auto param = ep.second;
    switch (ty) {
    case joy_type_e_button:
      {
        auto const v = idx < vbutton.size() ? vbutton[idx] : false;
        return v ? param : 0;
      }
    case joy_type_e_hat:
      {
        auto const v = idx < vhat.size() ? vhat[idx] : false;
        return v ? param : 0;
      }
    case joy_type_e_axis:
      return idx < vaxis.size() ? vaxis[idx] : 0;
    case joy_type_e_none:
      return param;
    default:
      return 0;
    }
  };
  log(21, "joystick ");
  uint16_t buttons_val = 0;
  // buttons
  for (size_t i = 0; i < vbutton.size(); ++i) {
    bool val = false;
    auto iter = joymap.find(std::make_pair(joy_type_e_button, (uint32_t)i));
    if (iter != joymap.end()) {
      for (auto const& e: iter->second) {
        val |= get_bool(e);
      }
    } else {
      val = vbutton[i];
    }
    if (val) {
      buttons_val |= (1 << i);
    }
    log(21, "%d", val ? 1 : 0);
  }
  log(21, " ");
  // hats
  for (size_t i = 0; i < vhat.size(); ++i) {
    bool val = false;
    auto iter = joymap.find(std::make_pair(joy_type_e_hat, (uint32_t)i));
    if (iter != joymap.end()) {
      for (auto const& e: iter->second) {
        val |= get_bool(e);
      }
    } else {
      val = vhat[i];
    }
    if (val) {
      buttons_val |= (1 << (i + 12));
    }
    log(21, "%d", val ? 1 : 0);
  }
  hid.gc_buttons(buttons_val);
  // axes
  for (size_t i = 0; i < vaxis.size(); ++i) {
    int32_t val = 0;
    auto iter = joymap.find(std::make_pair(joy_type_e_axis, (uint32_t)i));
    if (iter != joymap.end()) {
      for (auto const& e: iter->second) {
        val += get_int(e);
      }
    } else {
      // log(0, "vaxis[%u] = %d\n", (unsigned)i, (int)vaxis[i]);
      val = vaxis[i];
    }
    // 16bit符号付整数の範囲に丸める
    if (val >= 32767) {
      val = 32767;
    } else if (val < -32768) {
      val = -32768;
    }
    log(21, " axis%zu %d", i, (int)val);
    hid.gc_axis((uint8_t)i, (int16_t)val);
  }
  log(21, "\n");
}

//////////////////////////////////////////////////////////////

#define CHECK_GL(expr) \
  expr; \
  if (get_logmask(2)) { \
    GLenum e = glGetError(); \
    if (e != 0) { log(2, #expr ": %x\n", (unsigned)e); } \
  }

static void wrap_glDeleteBuffer(GLuint b)
{
  CHECK_GL(glDeleteBuffers(1, &b));
}

static void wrap_glDeleteTexture(GLuint t)
{
  CHECK_GL(glDeleteTextures(1, &t));
}

template <typename T> struct auto_res {
  T value { };
  void (*func)(T v) = nullptr;
  auto_res() { }
  auto_res(T value0, void (*func0)(T)) : value(value0), func(func0) { }
  ~auto_res() { reset(); }
  operator T() const { return value; }
  T get() const { return value; }
  void reset(T value0, void (*func0)(T)) {
    reset();
    value = value0;
    func = func0;
  }
  void reset() {
    if (value != T() && func != nullptr) {
      (*func)(value);
    }
    value = T();
    func = nullptr;
  }
private:
  auto_res(auto_res const&) = delete;
  auto_res& operator =(auto_res const&) = delete;
};

struct vcnet_window {
private:
  vcnet_config conf;
  unsigned window_width = 640;
  unsigned window_height = 480;
  unsigned texwidth = 0;
  unsigned texheight = 0;
  bool use_gl = true;
  unsigned vsync_mode = 0;
  auto_res<SDL_Window *> window;
  auto_res<SDL_GLContext> glcontext;
  auto_res<SDL_Renderer *> rend;
  auto_res<SDL_Texture *> sdltex;
  auto_res<SDL_Surface *> textsurf;
  auto_res<SDL_Texture *> texttex;
  auto_res<TTF_Font *> font;
  auto_res<GLuint> prog;
  GLint attr_coord = -1;
  auto_res<GLuint> vbo_coord;
  auto_res<GLuint> tex_video;
  auto_res<GLuint> tex_message;
  GLint loc_tex_video = -1;
  GLint loc_video_size = -1;
  GLint loc_tex_message = -1;
  GLint loc_message_size = -1;
  GLint loc_bilinear = -1;
  GLint loc_video_format = -1;
  GLint loc_scale = -1;
  Uint32 fullscreen = 0; // SDL_WINDOW_FULLSCREEN_DESKTOP;
  std::string message;
  SDL_Rect cur_draw_rect = { };
public:
  vcnet_window(vcnet_config const& conf0);
  vcnet_window(vcnet_window const&) = delete;
  vcnet_window& operator =(vcnet_window const&) = delete;
  void update_texture_sdl(vcnet_pixels const& pix);
  void resize_texture_if(unsigned w, unsigned h);
  void resize_window_if();
  void draw_window_sdl(vcnet_pixels const& pix, bool video_stable);
  void draw_window_gl(vcnet_pixels const& pix, bool video_stable);
  void toggle_fullscreen();
  void disable_ime();
  void set_message(std::string const& s, uint32_t c);
  unsigned get_width() const;
  unsigned get_height() const;
  SDL_Rect get_cur_draw_rect() const;
  bool get_use_gl() const;
  void set_title(std::string const& s);
};

vcnet_window::vcnet_window(vcnet_config const& conf0)
  : conf(conf0)
{
  vsync_mode = (unsigned)conf.get_uint("vsync", 0);
  use_gl = (conf.get_uint("use_gl", 1) != 0);
  log(0, "use_gl=%d\n", (int)use_gl);
  fullscreen = 0; // SDL_WINDOW_FULLSCREEN_DESKTOP;
  Uint32 flags = fullscreen;
  flags |= SDL_WINDOW_RESIZABLE;
  flags |= SDL_WINDOW_OPENGL;
  window.reset(
    SDL_CreateWindow("vcnet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      1280, 720, flags),
    SDL_DestroyWindow);
  {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    #if 0
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    #endif
    glcontext.reset(SDL_GL_CreateContext(window), SDL_GL_DeleteContext);
    if (glcontext == nullptr) {
      log(0, "SDL_GL_CreateContext: %s\n", SDL_GetError());
    }
    #ifdef _MSC_VER
    if (glewInit() != 0) {
      log(0, "glewInit failed\n");
    }
    #endif
    if (vsync_mode == 0) {
      log(0, "novsync\n");
      SDL_GL_SetSwapInterval(0);
    } else if (vsync_mode == 1) {
      log(0, "vsync\n");
      SDL_GL_SetSwapInterval(1);
    } else if (vsync_mode == 2) {
      if (SDL_GL_SetSwapInterval(-1) < 0) {
        log(0, "vsync\n");
        SDL_GL_SetSwapInterval(1);
      } else {
        log(0, "adaptive vsync\n");
      }
    }
    auto_res<GLuint> vs;
    auto_res<GLuint> fs;
    {
      vs.reset(glCreateShader(GL_VERTEX_SHADER), glDeleteShader);
      const char *vssrc =
        "#version 120\n"
        "uniform vec2 video_size;\n" // viewのピクセル数
        "uniform vec2 message_size;\n" // メッセージテクスチャのピクセル数
        "uniform vec2 scale;\n" // 参照するテクスチャの座標を
        "attribute vec2 coord;\n" // [0,1]範囲のフラグメント座標
        "varying vec2 v_video_coord;\n" // ピクセル単位のvideo座標
        "varying vec2 v_video_size_inv;\n"
        "varying vec2 v_message_size_inv;\n"
        "void main(void) {\n"
        "  vec2 coord_s = coord * scale;\n"
        "  v_video_coord = vec2(1.0 + coord_s.x, 1.0 - coord_s.y)\n"
        "    * 0.5 * video_size;\n"
        "  v_video_size_inv = 1.0 / video_size;\n"
        "  v_message_size_inv = 1.0 / message_size;\n"
        "  gl_Position = vec4(coord, 0.0, 1.0);\n"
        "}\n";
      CHECK_GL(glShaderSource(vs, 1, &vssrc, nullptr));
      CHECK_GL(glCompileShader(vs));
      GLint v = 0;
      CHECK_GL(glGetShaderiv(vs, GL_COMPILE_STATUS, &v));
      std::vector<char> buf(1024);
      GLsizei sz = 0;
      CHECK_GL(glGetShaderInfoLog(vs, (GLsizei)buf.size(), &sz, &buf[0]));
      if (v == 0) {
        log(0, "failed to compile vs: %s\n", &buf[0]);
      } else {
        log(0, "vertex shader %u %s\n", vs.get(), &buf[0]);
      }
    }
    {
      fs.reset(glCreateShader(GL_FRAGMENT_SHADER), glDeleteShader);
      const char *fssrc =
        "#version 120\n"
        "uniform vec2 video_size;\n" // viewのピクセル数
        "uniform sampler2D tex_video;\n" // ビデオ画像テクスチャ
        "uniform sampler2D tex_message;\n" // メッセージテクスチャ
        "uniform vec2 message_size;\n" // メッセージテクスチャのピクセル数
        "uniform float bilinear;\n" // bilinear補間をするかどうか
        "uniform float video_format;\n" // YUV422かRGBか
        "varying vec2 v_video_coord;\n" // ピクセル単位のvideo座標
        "varying vec2 v_video_size_inv;\n"
        "varying vec2 v_message_size_inv;\n"
        "const float inv65536 = 1.0 / 65536.0;\n"
        "vec2 compatTexelFetch(vec2 p) {\n"
        "  return texture2D(tex_video, (p + 0.5) * v_video_size_inv).xy;\n"
        "}\n"
        "vec4 texelFetchYUV(vec2 pc) {\n"
        "  if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "    pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "    return vec4(0.0, 0.0, 0.0, 1.0);\n"
        "  }\n"
        "  if (pc.x < message_size.x && pc.y < message_size.y) {\n"
        "    float a = texture2D(tex_message,\n"
        "      (pc + 0.5) * v_message_size_inv).a;\n"
        "    return vec4(a, a, a, 1.0);\n"
        "  }\n"
        "  float pcx2 = pc.x * 0.5;\n"
        "  float pcx2_i = floor(pcx2);\n"
        "  float pcx2_f = pcx2 - pcx2_i;\n"
        "  float pcx_0 = pcx2_i * 2.0;\n"
        "  float pcx_1 = pcx_0 + 1.0;\n"
        "  vec2 t0 = compatTexelFetch(vec2(pcx_0, pc.y)) * 255.0;\n"
        "  vec2 t1 = compatTexelFetch(vec2(pcx_1, pc.y)) * 255.0;\n"
        "  float y = ((pcx2_f < 0.5) ? t0.y : t1.y) - 16.0;\n"
        "  float u = t0.x - 128.0;\n"
        "  float v = t1.x - 128.0;\n"
        "  float r = (298.0 * y + 409.0 * v + 128.0) * inv65536;\n"
        "  float g = (298.0 * y - 100.0 * u - 208.0 * v + 128.0) * inv65536;\n"
        "  float b = (298.0 * y + 516.0 * u + 128.0) * inv65536;\n"
        "  return clamp(vec4(r, g, b, 1.0), 0.0, 1.0);\n"
        "}\n"
        "void main(void) {\n"
        "  if (video_format == 0.0) {\n"
        "    vec2 pc = v_video_coord;\n"
        "    if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "      pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "      gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "    } else if (pc.x < message_size.x && pc.y < message_size.y) {\n"
        "      float a = texture2D(tex_message, pc * v_message_size_inv).a;\n"
        "      gl_FragColor = vec4(a, a, a, 1.0);\n"
        "    } else {\n"
        "      gl_FragColor = texture2D(tex_video, pc * v_video_size_inv);\n"
        "    }\n"
        "  } else if (bilinear == 0.0) {\n"
        "    gl_FragColor = texelFetchYUV(floor(v_video_coord));\n"
        "  } else {\n"
        "    vec2 vp00 = v_video_coord - 0.5;\n"
        "    vec2 vpf = floor(vp00);\n"
        "    vec4 c00 = texelFetchYUV(vec2(vpf.x + 0.0, vpf.y + 0.0));\n"
        "    vec4 c10 = texelFetchYUV(vec2(vpf.x + 1.0, vpf.y + 0.0));\n"
        "    vec4 c01 = texelFetchYUV(vec2(vpf.x + 0.0, vpf.y + 1.0));\n"
        "    vec4 c11 = texelFetchYUV(vec2(vpf.x + 1.0, vpf.y + 1.0));\n"
        "    vec2 a = vp00 - vpf;\n"
        "    vec4 c0 = mix(c00, c10, a.x);\n"
        "    vec4 c1 = mix(c01, c11, a.x);\n"
        "    gl_FragColor = mix(c0, c1, a.y);\n"
        "  }\n"
        "}\n";
      CHECK_GL(glShaderSource(fs, 1, &fssrc, nullptr));
      CHECK_GL(glCompileShader(fs));
      GLint v = 0;
      CHECK_GL(glGetShaderiv(fs, GL_COMPILE_STATUS, &v));
      std::vector<char> buf(1024);
      GLsizei sz = 0;
      CHECK_GL(glGetShaderInfoLog(fs, (GLsizei)buf.size(), &sz, &buf[0]));
      if (v == 0) {
        log(0, "failed to compile fs: %s\n", &buf[0]);
      } else {
        log(0, "fragment shader %u %s\n", fs.get(), &buf[0]);
      }
    }
    {
      prog.reset(glCreateProgram(), glDeleteProgram);
      CHECK_GL(glAttachShader(prog, vs));
      CHECK_GL(glAttachShader(prog, fs));
      CHECK_GL(glLinkProgram(prog));
      GLint v = 0;
      CHECK_GL(glGetProgramiv(prog, GL_LINK_STATUS, &v));
      std::vector<char> buf(1024);
      GLsizei sz = 0;
      CHECK_GL(glGetProgramInfoLog(prog, (GLsizei)buf.size(), &sz, &buf[0]));
      if (v == 0) {
        log(0, "failed to link prog: %s\n", &buf[0]);
      } else {
        log(0, "prog %u %s\n", prog.get(), &buf[0]);
      }
    }
    {
      attr_coord = glGetAttribLocation(prog, "coord");
      if (attr_coord == -1) {
        log(0, "failed to get location: attr_coord\n");
      } else {
        log(0, "attr_coord %d\n", attr_coord);
      }
    }
    {
      GLfloat coord2_quad[] = {
         1.0,  1.0,
        -1.0,  1.0,
        -1.0, -1.0,
         1.0, -1.0,
      };
      GLuint b;
      CHECK_GL(glGenBuffers(1, &b));
      vbo_coord.reset(b, wrap_glDeleteBuffer);
      CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, b));
      CHECK_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(coord2_quad),
        &coord2_quad, GL_STATIC_DRAW));
      CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }
    {
      GLuint t;
      CHECK_GL(glGenTextures(1, &t));
      tex_video.reset(t, wrap_glDeleteTexture);
      CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_video));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_NEAREST));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        GL_NEAREST));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_BORDER));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_BORDER));
    }
    {
      GLuint t;
      CHECK_GL(glGenTextures(1, &t));
      tex_message.reset(t, wrap_glDeleteTexture);
      CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_message));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        GL_LINEAR));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_BORDER));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_BORDER));
    }
    {
      loc_tex_video = glGetUniformLocation(prog, "tex_video");
      loc_tex_message = glGetUniformLocation(prog, "tex_message");
      loc_video_size = glGetUniformLocation(prog, "video_size");
      loc_message_size = glGetUniformLocation(prog, "message_size");
      loc_bilinear = glGetUniformLocation(prog, "bilinear");
      loc_video_format = glGetUniformLocation(prog, "video_format");
      loc_scale = glGetUniformLocation(prog, "scale");
    }
  }
  if (!use_gl) {
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    if (vsync_mode != 0) {
      render_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    rend.reset(SDL_CreateRenderer(window, -1, render_flags),
      SDL_DestroyRenderer);
  }
  #ifdef _MSC_VER
  std::string fn = "/Windows/Fonts/courbd.ttf";
  #elif defined(__APPLE__)
  std::string fn = "/System/Library/Fonts/Menlo.ttc";
  #else
  std::string fn = "/usr/share/fonts/truetype/mplus/mplus-1m-bold.ttf";
  #endif
  fn = conf.get_str("font", fn);
  auto fnsz = conf.get_uint("font_size", 24);
  font.reset(TTF_OpenFont(fn.c_str(), (int)fnsz), TTF_CloseFont);
  if (!font) {
    log(0, "failed to open font: %s\n", fn.c_str());
  } else {
    log(1, "font: %s\n", fn.c_str());
  }
}

void
vcnet_window::update_texture_sdl(vcnet_pixels const& pix)
{
  if (pix.get_width() < 2 || pix.get_height() < 2) {
    return;
  }
  resize_texture_if(pix.get_width(), pix.get_height());
  size_t spos = 0;
  void *pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(sdltex, nullptr, &pixels, &pitch) != 0) {
    log(0, "update_texture_sdl: SDL_LockTexture failed: %s\n",
      SDL_GetError());
    return;
  }
  std::unique_ptr<SDL_Texture, void (*)(SDL_Texture *)> sdltex_unlock(
    sdltex, &SDL_UnlockTexture);
  char *pline = (char *)pixels;
  const unsigned char *p3 = pix.begin();
  for (unsigned h = 0; h < texheight; ++h) {
    char *p = pline;
    if (!pix.get_yuv422()) {
      for (unsigned w = 0; w < texwidth; ++w) {
        p[0] = p3[spos + 0];
        p[1] = p3[spos + 1];
        p[2] = p3[spos + 2];
        p[3] = 0;
        p += 4;
        spos += 3;
      }
    } else {
      for (unsigned w = 0; w + 1 < texwidth; w += 2) {
        auto to_u8 = [] (int32_t v) {
          int32_t cv = v < 0 ? 0 : v > 65535 ? 65535 : v;
          return uint8_t(cv >> 8);
        };
        int16_t u = p3[spos + 0];
        int16_t y0 = p3[spos + 1];
        int16_t v = p3[spos + 2];
        int16_t y1 = p3[spos + 3];
        int32_t c0 = y0 - 16;
        int32_t c1 = y1 - 16;
        int32_t d = u - 128;
        int32_t e = v - 128;
        uint8_t r0 = to_u8(298 * c0 + 409 * e + 128);
        uint8_t g0 = to_u8(298 * c0 - 100 * d - 208 * e + 128);
        uint8_t b0 = to_u8(298 * c0 + 516 * d + 128);
        uint8_t r1 = to_u8(298 * c1 + 409 * e + 128);
        uint8_t g1 = to_u8(298 * c1 - 100 * d - 208 * e + 128);
        uint8_t b1 = to_u8(298 * c1 + 516 * d + 128);
        // ARGB8888
        p[0] = b0;
        p[1] = g0;
        p[2] = r0;
        p[3] = 0;
        p[4] = b1;
        p[5] = g1;
        p[6] = r1;
        p[7] = 0;
        p += 8;
        spos += 4;
      }
    }
    pline += pitch;
  }
}

void
vcnet_window::resize_texture_if(unsigned w, unsigned h)
{
  if (w == texwidth && h == texheight) {
    return;
  }
  if (w < 2 || h < 2) {
    return;
  }
  texwidth = w;
  texheight = h;
  log(1, "resize_texture (%u %u) (%u %u)\n", w, h, texwidth, texheight);
  if (rend != nullptr) {
    sdltex.reset(SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING, texwidth, texheight),
      SDL_DestroyTexture);
    if (sdltex.get() == nullptr) {
      log(0, "resize_texture_if: SDL_CreateTexture: %s\n", SDL_GetError());
    }
  }
}

void
vcnet_window::resize_window_if()
{
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(window, &w, &h);
  if ((unsigned)w != window_width || (unsigned)h != window_height) {
    window_width = w;
    window_height = h;
    log(1, "resize_window window %d %d\n", w, h);
    CHECK_GL(glViewport(0, 0, w, h));
    cur_draw_rect.w = window_width;
    cur_draw_rect.h = window_height;
  }
}

void
vcnet_window::draw_window_gl(vcnet_pixels const& pix, bool video_stable)
{
  // log(0, "prog=%d\n", (int)prog.get());
  CHECK_GL(glUseProgram(prog));
  CHECK_GL(glClearColor(0.0, 1.0, 1.0, 1.0));
  CHECK_GL(glClear(GL_COLOR_BUFFER_BIT));
  float const bilinear = (pix.get_width() == window_width &&
    pix.get_height() == window_height)
    ? 0.0f : 1.0f;
  {
    const unsigned char *p = pix.begin();
    CHECK_GL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_video));
    if (pix.get_yuv422() || bilinear == 0.0f) {
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_NEAREST));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        GL_NEAREST));
    } else {
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        GL_LINEAR));
    }
    if (pix.get_yuv422()) {
      CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, pix.get_width(),
        pix.get_height(), 0, GL_RG, GL_UNSIGNED_BYTE, p));
    } else {
      CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pix.get_width(),
        pix.get_height(), 0, GL_BGR, GL_UNSIGNED_BYTE, p));
    }
  }
  float msgtexw = 0;
  float msgtexh = 0;
  if (textsurf.get() != nullptr) {
    msgtexw = (float)textsurf.get()->w;
    msgtexh = (float)textsurf.get()->h;
    CHECK_GL(glActiveTexture(GL_TEXTURE1));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_message));
  }
  float scale_x = 1.0;
  float scale_y = 1.0;
  {
    // アスペクト比を保って拡大縮小するために、上下または左右に余白を入れる。
    unsigned hw = window_height * texwidth;
    unsigned wh = window_width * texheight;
    SDL_Rect rect = { };
      // absoluteモードでのカーソル位置を計算するためにwindow中の描画領域を
      // 計算し、cur_draw_rectにセットする。
    rect.w = window_width;
    rect.h = window_height;
    if (hw > wh) {
      scale_y = (float)hw / (float)wh;
      rect.w = window_width;
      rect.h = wh / texwidth;
      rect.x = 0;
      rect.y = (window_height - rect.h) / 2;
    } else if (wh > hw) {
      scale_x = (float)wh / (float)hw;
      rect.w = hw / texheight;
      rect.h = window_height;
      rect.x = (window_width - rect.w) / 2;
      rect.y = 0;
    }
    cur_draw_rect = rect;
  }
  CHECK_GL(glUniform1i(loc_tex_video, 0));
  CHECK_GL(glUniform2f(loc_video_size, (float)pix.get_width(),
    (float)pix.get_height()));
  CHECK_GL(glUniform1i(loc_tex_message, 1));
  CHECK_GL(glUniform2f(loc_message_size, msgtexw, msgtexh));
  CHECK_GL(glUniform2f(loc_scale, scale_x, scale_y));
  CHECK_GL(glUniform1f(loc_bilinear, bilinear));
  CHECK_GL(glUniform1f(loc_video_format, pix.get_yuv422() ? 1.0f : 0.0f));
  CHECK_GL(glEnableVertexAttribArray(attr_coord));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_coord));
  CHECK_GL(glVertexAttribPointer(attr_coord, 2, GL_FLOAT, GL_FALSE, 0, 0));
  CHECK_GL(glDrawArrays(GL_QUADS, 0, 4));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  CHECK_GL(glDisableVertexAttribArray(attr_coord));
  SDL_GL_SwapWindow(window);
}

void
vcnet_window::draw_window_sdl(vcnet_pixels const& pix, bool video_stable)
{
  unsigned hw = window_height * texwidth;
  unsigned wh = window_width * texheight;
  if (hw == wh) {
    /* 縦横比が信号とウインドウで一致する。このときはvideo_stableがfalse
     * であっても描画する。*/
    SDL_RenderCopy(rend, sdltex, nullptr, nullptr);
    SDL_Rect rect = { };
    rect.w = window_width;
    rect.h = window_height;
    cur_draw_rect = rect;
  } else if (!video_stable) {
    /* 縦横比が信号とウインドウで一致せず、video_stableがfalseならばおか
     * しなデータである可能性が高いので描画しない。*/
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderClear(rend);
  } else {
    /* 縦横比が信号とウインドウで一致しない。左右または上下に余白を入れて
     * 描画する。*/
    SDL_Rect rect = { };
    if (hw > wh) {
      rect.w = window_width;
      rect.h = wh / texwidth;
      rect.x = 0;
      rect.y = (window_height - rect.h) / 2;
    } else {
      rect.w = hw / texheight;
      rect.h = window_height;
      rect.x = (window_width - rect.w) / 2;
      rect.y = 0;
    }
    log(12, "window %u %u tex %u %u\n", window_width, window_height,
      texwidth, texheight);
    log(12, "whxy=%d %d %d %d\n", rect.w, rect.h, rect.x, rect.y);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderClear(rend);
    SDL_Rect src_rect = { };
    src_rect.w = texwidth;
    src_rect.h = texheight;
    src_rect.x = 0;
    src_rect.y = 0;
    SDL_RenderCopy(rend, sdltex, &src_rect, &rect);
    cur_draw_rect = rect;
  }
  if (textsurf) {
    SDL_Rect rect;
    SDL_GetClipRect(textsurf, &rect);
    rect.x += 20;
    rect.y += 20;
    SDL_RenderCopy(rend, texttex, nullptr, &rect);
  }
  SDL_RenderPresent(rend);
}

void
vcnet_window::toggle_fullscreen()
{
  fullscreen = fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
  // fullscreen = fullscreen ? 0 : SDL_WINDOW_FULLSCREEN;
  log(1, "toggle fullscreen %d\n", (int)fullscreen);
  SDL_SetWindowFullscreen(window, fullscreen);
}

void
vcnet_window::disable_ime()
{
  #ifdef _MSC_VER
  HWND hwnd = nullptr;
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(window, &info)) {
    hwnd = info.info.win.window;
  }
  if (hwnd != nullptr) {
    ImmAssociateContext(hwnd, (HIMC)nullptr);
  }
  #endif
}

void
vcnet_window::set_message(std::string const& s, uint32_t c)
{
  if (message == s) {
    return;
  }
  message = s;
  if (!font) {
    return;
  }
  log(2, "message: %s\n", s.c_str());
  if (s.empty()) {
    textsurf.reset();
    texttex.reset();
    return;
  }
  SDL_Color col = { };
  col.r = c >> 16;
  col.g = c >> 8;
  col.b = c >> 0;
  textsurf.reset(
    TTF_RenderUTF8_Blended(font, s.c_str(), col),
    SDL_FreeSurface);
  if (use_gl) {
    if (SDL_LockSurface(textsurf) != 0) {
      log(0, "set_message: SDL_LockSurface: %s\n", SDL_GetError());
      log(0, "str: [%s]\n", s.c_str());
      return;
    }
    std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)> textsurf_unlock(
      textsurf, &SDL_UnlockSurface);
    void const *const pixels = textsurf.get()->pixels;
    // int const w = textsurf.get()->w;
    int const w = textsurf.get()->pitch / 4;
    int const h = textsurf.get()->h;
    log(2, "w=%d,h=%d,pitch=%d\n", w, h, textsurf.get()->pitch);
    CHECK_GL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_message));
    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, pixels));
  } else {
    texttex.reset(
      SDL_CreateTextureFromSurface(rend, textsurf),
      SDL_DestroyTexture);
  }
}

unsigned
vcnet_window::get_width() const
{
  return window_width;
}

unsigned
vcnet_window::get_height() const
{
  return window_height;
}

SDL_Rect
vcnet_window::get_cur_draw_rect() const
{
  return cur_draw_rect;
}

bool
vcnet_window::get_use_gl() const
{
  return use_gl;
}

void
vcnet_window::set_title(std::string const& s)
{
  SDL_SetWindowTitle(window, s.c_str());
  log(0, "title %s\n", s.c_str());
}

//////////////////////////////////////////////////////////////

struct vcnet_keystate {
  struct entry {
    uint8_t mod;
    std::chrono::system_clock::time_point time;
  };
private:
  std::map<uint32_t, entry> keys;
public:
  bool set_pressed(uint32_t code, uint8_t mod);
  bool set_released(uint32_t code);
  bool is_pressed(uint32_t code, uint8_t *mod_r = nullptr);
  std::map<uint32_t, entry> const& get() const;
  void erase(uint32_t code);
  void clear();
};

bool
vcnet_keystate::set_pressed(uint32_t code, uint8_t mod)
{
  auto now = std::chrono::system_clock::now();
  auto iter = keys.find(code);
  if (iter == keys.end()) {
    keys[code] = vcnet_keystate::entry{mod, now};
    return true;
  }
  return false;
}

bool
vcnet_keystate::set_released(uint32_t code)
{
  auto iter = keys.find(code);
  if (iter != keys.end()) {
    keys.erase(code);
    return true;
  }
  return false;
}

bool
vcnet_keystate::is_pressed(uint32_t code, uint8_t *mod_r)
{
  auto iter = keys.find(code);
  if (iter != keys.end()) {
    if (mod_r != nullptr) {
      *mod_r = iter->second.mod;
    }
    return true;
  }
  return false;
}

std::map<uint32_t, vcnet_keystate::entry> const&
vcnet_keystate::get() const
{
  return keys;
}

void
vcnet_keystate::erase(uint32_t code)
{
  keys.erase(code);
}

void
vcnet_keystate::clear()
{
  keys.clear();
}

//////////////////////////////////////////////////////////////

struct vcnet_audio {
private:
  SDL_AudioDeviceID audiodev = 0;
  enum { USE_AUDIO_CALLBACK = 1 };
  enum { BUF_SIZE = 32768 };
  std::array<unsigned char, BUF_SIZE> buffer { };
  static void audio_cb(void *userdata, Uint8 *stream, int len);
  std::atomic_uint offset_read;
  std::atomic_uint offset_write;
  uint32_t freq = 48000;
public:
  vcnet_audio();
  vcnet_audio(vcnet_audio const&) = delete;
  vcnet_audio& operator =(vcnet_audio const&) = delete;
  ~vcnet_audio();
  void open_device_if(uint32_t audio_freq);
  void close_device();
  void queue_audio(std::vector<uint32_t>& audiobuf);
  uint32_t get_freq() const;
  unsigned get_buffered_size() const;
};

vcnet_audio::vcnet_audio()
{
}

void
vcnet_audio::open_device_if(uint32_t audio_freq)
{
  if (freq == audio_freq && audiodev != 0) {
    return;
  }
  close_device();
  freq = audio_freq;
  SDL_AudioSpec as_req { };
  SDL_AudioSpec as_got { };
  as_req.freq = freq;
  as_req.format = AUDIO_S16SYS;
  as_req.channels = 2;
  as_req.samples = 4096; // 2048以下にするとゴミが入る。要調査。
  if (USE_AUDIO_CALLBACK) {
    as_req.callback = audio_cb;
  }
  as_req.userdata = this;
  audiodev = SDL_OpenAudioDevice(nullptr, 0, &as_req, &as_got, 0);
  log(1, "audio device %u\n", (unsigned)audiodev);
  if (audiodev != 0) {
    SDL_PauseAudioDevice(audiodev, 0);
  }
  offset_read = 0;
  offset_write = 0;
}

vcnet_audio::~vcnet_audio()
{
  close_device();
}

void
vcnet_audio::close_device()
{
  if (audiodev != 0) {
    SDL_CloseAudioDevice(audiodev);
    audiodev = 0;
  }
}

static FILE *raw_audio = nullptr;
static void
dump_raw_audio(void *data, size_t len)
{
  if (raw_audio == nullptr) {
    raw_audio = fopen("rawaudio.raw", "wb");
  }
  if (raw_audio != nullptr) {
    fwrite(data, len, len, raw_audio);
  }
}

unsigned
vcnet_audio::get_buffered_size() const
{
  if (!USE_AUDIO_CALLBACK) {
    Uint32 sz = SDL_GetQueuedAudioSize(audiodev);
    return (unsigned)sz;
  } else {
    unsigned const ord = offset_read.load();
    unsigned const owr = offset_write.load();
    unsigned const bufsz = (owr - ord) % BUF_SIZE;
    return bufsz;
  }
}

void
vcnet_audio::audio_cb(void *userdata, Uint8 *stream, int len)
{
  vcnet_audio *self = (vcnet_audio *)userdata;
  unsigned const ulen = len;
  unsigned const ord = self->offset_read.load();
  unsigned const owr = self->offset_write.load();
  unsigned const bufsz = (owr - ord) % BUF_SIZE;
  log(13, "audio_cb bufsz=%u len=%u\n", bufsz, ulen);
  if (bufsz > ulen) {
    unsigned const ord_end = (ord + ulen) % BUF_SIZE;
    if (ord_end > ord) {
      memcpy(stream, &self->buffer[ord], ulen);
    } else {
      memcpy(stream, &self->buffer[ord], (BUF_SIZE - ord));
      memcpy(stream + (BUF_SIZE - ord), &self->buffer[0], ord_end);
    }
    self->offset_read.store(ord_end);
  } else {
    memset(stream, 0, ulen);
    log(3, "audio_cb: no data\n");
  }
}

void
vcnet_audio::queue_audio(std::vector<uint32_t>& audiobuf)
{
  if (audiodev != 0 && !audiobuf.empty()) {
    size_t const audiobuf_nbytes = audiobuf.size() * sizeof(uint32_t);
    unsigned char* audiobuf_p = (unsigned char*)&audiobuf[0];
    if (get_logmask(14)) {
      dump_raw_audio(&audiobuf[0], audiobuf.size() * 4u);
    }
    if (!USE_AUDIO_CALLBACK) {
      Uint32 sz = SDL_GetQueuedAudioSize(audiodev);
      if (sz > 16384) {
        log(1, "drop: queued audio size %u %zu\n", (unsigned)sz,
          audiobuf.size());
      } else {
        SDL_QueueAudio(audiodev, &audiobuf[0], (Uint32)audiobuf.size() * 4u);
        log(15, "queue audio\n");
      }
    } else {
      unsigned owr = offset_write.load();
      unsigned ord = offset_read.load();
      unsigned space = (ord == owr) ? BUF_SIZE : (ord - owr) % BUF_SIZE;
      log(15, "queue space=%u\n", space);
      if (space > audiobuf_nbytes + 1) {
        unsigned owr_end = (owr + audiobuf_nbytes) % BUF_SIZE;
        if (owr_end > owr) {
          memcpy(&buffer[owr], audiobuf_p, audiobuf_nbytes);
        } else {
          memcpy(&buffer[owr], audiobuf_p, BUF_SIZE - owr);
          memcpy(&buffer[0], audiobuf_p + (BUF_SIZE - owr), owr_end);
        }
        offset_write.store(owr_end);
      } else {
        log(1, "queue_audio: no space\n");
      }
    }
  }
  audiobuf.clear();
}

uint32_t
vcnet_audio::get_freq() const
{
  return freq;
}

//////////////////////////////////////////////////////////////

struct vcnet_control {
private:
  vcnet_config conf;
  vcnet_conn conn;
  vcnet_pixels pix;
  vcnet_audio audio;
  std::vector<uint32_t> audiobuf;
  vcnet_hid hid;
  vcnet_window wnd;
  vcnet_keystate keystate;
  bool grab_mouse = false;
  struct keycmd {
    std::chrono::system_clock::time_point time;
    uint8_t code = 0;
    uint8_t mod = 0;
    uint8_t cmd = 0;
  };
  std::deque<keycmd> delayed_keycmd;
  std::array<uint16_t, 128> ch2usb { };
  std::map<uint32_t, uint32_t> keymap;
  std::map<uint8_t, uint8_t> keymodmap;
  joymap_type joymap;
  std::chrono::system_clock::time_point last_draw_time { };
  std::chrono::system_clock::time_point last_msg_time { };
  std::chrono::system_clock::time_point last_hid_time { };
  std::chrono::system_clock::time_point last_mouse_send_time { };
  std::chrono::system_clock::time_point last_joystick_send_time { };
  std::chrono::system_clock::time_point last_suppress_frames_time { };
  bool show_info = false;
  uint32_t msg_color = 0xffffff;
  std::deque<std::array<uint8_t, 3>> adv7611_cmds;
  vcnet_joystick joy;
  int num_joysticks_saved = 0;
  std::string joystick_name;
private:
  void send_text(std::string const& text);
  void convert_scancode(uint8_t c, uint32_t s, uint8_t m, uint8_t& c_r,
    uint8_t& m_r, bool& downup_r);
  uint8_t keymod_sdl2usb(uint16_t mod);
  void handle_keyboard(SDL_KeyboardEvent const& kev, bool down);
  bool send_delayed_keycmd();
  void clear_delayed_keycmd();
public:
  vcnet_control(std::vector<std::string> const& conf_files);
  void handle_event(SDL_Event const& ev, bool& done_r);
  void mainloop();
};

vcnet_control::vcnet_control(std::vector<std::string> const& conf_files)
  : conf(conf_files), conn(conf), wnd(conf)
{
  for (size_t i = 0; i < conf_files.size(); ++i) {
    log(1, "config '%s'\n", conf_files[i].c_str());
  }
  {
    auto s = conf.get_str("logmask", "1");
    auto v = strtoull(s.c_str(), nullptr, 0);
    log(0, "logmask=%llu\n", v);
    set_logmask(v);
  }
  last_draw_time = last_msg_time = last_hid_time = last_mouse_send_time =
    last_joystick_send_time = std::chrono::system_clock::now();
  for (unsigned i = 0; i < 8; ++i) {
    auto s = conf.get_str("ch2usb@" + std::to_string(i) + "0", std::string());
    std::string tok;
    std::stringstream sstr(s);
    unsigned j = i * 16;
    while (std::getline(sstr, tok, ',')) {
      unsigned long v = strtoul(tok.c_str(), nullptr, 16);
      if (j < 127) {
        ch2usb[j] = (uint16_t)v;
        log(1, "ch2usb[%x] = %x\n", j, (unsigned)v);
        ++j;
      }
    }
  }
  {
    auto s = conf.get_str("keymap", "");
    std::string tok;
    std::stringstream sstr(s);
    while (std::getline(sstr, tok, ',')) {
      auto p = tok.find(':');
      if (p != tok.npos) {
        auto sk = tok.substr(0, p);
        auto sv = tok.substr(p + 1);
        uint32_t k = (uint32_t)strtoul(sk.c_str(), nullptr, 16);
        uint32_t v = (uint32_t)strtoul(sv.c_str(), nullptr, 16);
        log(1, "keymap[%x] = %x\n", (unsigned)k, (unsigned)v);
        keymap[k] = v;
      }
    }
  }
  {
    auto s = conf.get_str("keymodmap", "");
    std::string tok;
    std::stringstream sstr(s);
    while (std::getline(sstr, tok, ',')) {
      auto p = tok.find(':');
      if (p != tok.npos) {
        auto sk = tok.substr(0, p);
        auto sv = tok.substr(p + 1);
        uint8_t k = (uint32_t)strtoul(sk.c_str(), nullptr, 16);
        uint8_t v = (uint32_t)strtoul(sv.c_str(), nullptr, 16);
        log(1, "keymodmap[%x] = %x\n", (unsigned)k, (unsigned)v);
        keymodmap[k] = v;
      }
    }
  }
  {
    auto s = conf.get_str("joymap", "");
    std::string tok;
    std::stringstream sstr(s);
    while (std::getline(sstr, tok, ',')) {
      std::string e;
      std::stringstream stok(tok);
      joy_entry ek { };
      std::vector<std::pair<joy_entry, int>> em;
      size_t k = 0;
      while (std::getline(stok, e, ':')) {
        auto to_joy_entry = [] (std::string const& e) {
          char const ch = e[0];
          auto n = (uint32_t)strtoul(e.c_str() + 1, nullptr, 0);
          joy_type_e t { };
          switch (ch) {
          case 'n': t = joy_type_e_none; break;
          case 'b': t = joy_type_e_button; break;
          case 'h': t = joy_type_e_hat; break;
          case 'a': t = joy_type_e_axis; break;
          default: break;
          }
          return std::make_pair(t, n);
        };
        if (k == 0) {
          ek = to_joy_entry(e);
        } else if ((k % 2) == 1) {
          em.push_back(std::make_pair(to_joy_entry(e), 0));
        } else {
          em.back().second = strtol(e.c_str(), nullptr, 0);
        }
        ++k;
      }
      joymap[ek] = em;
    }
  }
  if (!conf_files.empty()) {
    auto s = conf_files[conf_files.size() - 1];
    auto p = s.rfind('/');
    if (p != s.npos) {
      s = s.substr(p + 1);
    }
    p = s.rfind('.');
    s = s.substr(0, p);
    wnd.set_title(s + " - vcnet");
  }
}

void
vcnet_control::send_text(std::string const& text)
{
  // テキスト文字列の内容をUSBキーボード操作によってターゲットへペーストする。
  // ASCII文字にしか対応しない。キーボード配置によって送信すべきキーが変わる
  // ので、設定を"ch2usb@00"などにセットする必要がある。
  log(16, "paste '%s'\n", text.c_str());
  auto now = std::chrono::system_clock::now();
  auto const interval_ms = 10; // TODO: 5msくらいまでは短くしても動く
  size_t delay = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch < 0 || ch > 0x7e) {
      log(16, "send_text: skip %x\n", (unsigned)ch);
      continue;
    }
    uint16_t code_mod = ch2usb[ch];
    if (code_mod == 0) {
      log(16, "send_text: skip %x %x\n", (unsigned)ch, (unsigned)code_mod);
      continue;
    }
    keycmd kc { };
    kc.code = code_mod & 0xffu;
    kc.mod = code_mod >> 8u;
    kc.cmd = 0x04; // down
    kc.time = now + std::chrono::milliseconds(interval_ms) * delay;
    ++delay;
    delayed_keycmd.push_back(kc);
    kc.cmd = 0x05; // up
    kc.time = now + std::chrono::milliseconds(interval_ms) * delay;
    if (i + 1 < text.size()) {
      uint16_t code_mod_next = ch2usb[text[i + 1]];
      if ((code_mod_next & 0xffu) == (code_mod & 0xffu)) {
        // 同じキーを連続で送信するときはupしてからdownまでの間隔を開ける
        // 必要がある。そうでない場合は前のキーのupと次のキーのdownを同時
        // におこなってよい。
        ++delay;
      }
    }
    delayed_keycmd.push_back(kc);
  }
  log(16, "send_text delayed_keycmd %u\n",
    (unsigned)delayed_keycmd.size());
}

void
vcnet_control::convert_scancode(uint8_t c, uint32_t s, uint8_t m0,
  uint8_t& c_r, uint8_t& m_r, bool& downup_r)
{
  uint8_t m = m0;
  {
    // modifierがkeymodmapに登録されていれば変換する。この変換は
    // keymapの変換より先に適用される。
    m = 0;
    for (uint32_t i = 0; i < 8; ++i) {
      uint32_t v = (1 << i);
      if ((v & m0) != 0) {
        auto const iter = keymodmap.find(i);
        if (iter != keymodmap.end()) {
          v = 1 << iter->second;
        }
        m |= v;
      }
    }
  }
  {
    // usage idとmodifierの組み合わせがkeymapに登録されていれば
    // そのコードのdown/upを送る
    uint32_t k = (uint32_t)c | ((uint32_t) m) << 8;
    auto const iter = keymap.find(k);
    if (iter != keymap.end()) {
      uint32_t v = iter->second;
      // [7:0] がコード、[15:8] がmodifier、[16:16] がdownup
      c_r = v & 0xff;
      m_r = (v >> 8) & 0xff;
      downup_r = (v & 0x10000) != 0;
      return;
    }
    if ((m & 0xf0) != 0) {
      // 右modifierを左modifierに置き換えてkeymapを引く
      m = (m >> 4) | (m & 0x0f);
      k = (uint32_t)c | ((uint32_t) m) << 8;
      auto const iter = keymap.find(k);
      if (iter != keymap.end()) {
        uint32_t v = iter->second;
        c_r = v & 0xff;
        m_r = (v >> 8) & 0xff;
        downup_r = (v & 0x10000) != 0;
        return;
      }
    }
  }
  // keymapに登録されていないときここにくる。最低限の変換。
  c_r = c;
  m_r = m;
  downup_r = false;
  switch (c) {
  case 0x64: // backslash, underline, hiragana ro
    c_r = 0x87;
    break;
  case 0x35: // zenkaku hankaku
  case 0x91: // mac eisu
    c_r = 0x35;
    downup_r = true;
    break;
  default:
    break;
  }
}

uint8_t
vcnet_control::keymod_sdl2usb(uint16_t mod)
{
  // SDL2のmodifierをUSBのmodifier値に変換する
  uint8_t r = 0;
  if ((mod & KMOD_LCTRL) != 0) { r |= 0x01; }
  if ((mod & KMOD_LSHIFT) != 0) { r |= 0x02; }
  if ((mod & KMOD_LALT) != 0) { r |= 0x04; }
  if ((mod & KMOD_LGUI) != 0) { r |= 0x08; }
  if ((mod & KMOD_RCTRL) != 0) { r |= 0x10; }
  if ((mod & KMOD_RSHIFT) != 0) { r |= 0x20; }
  if ((mod & KMOD_RALT) != 0) { r |= 0x40; }
  if ((mod & KMOD_RGUI) != 0) { r |= 0x80; }
  return r;
}

void
vcnet_control::handle_keyboard(SDL_KeyboardEvent const& kev, bool down)
{
  uint8_t mod = keymod_sdl2usb(kev.keysym.mod);
  if (down) {
    bool ins = keystate.set_pressed(kev.keysym.scancode, mod);
    if (!ins) {
      return;
    }
  } else {
    bool era = keystate.set_released(kev.keysym.scancode);
    if (!era) {
      return;
    }
  }
  uint8_t conv_c = 0; // 変換先キーコード(USBのusage id)
  uint8_t conv_m = 0; // 変換先modifier
  bool downup = false;
    // これがtrueのときdownイベントに対してdownとupを発生させ、upイベント
    // は無視する。全角/半角のようにupイベントが送られてこない特殊なキー
    // をターゲットに送信するのに使う。
  convert_scancode(kev.keysym.scancode, kev.keysym.sym, mod, conv_c, conv_m,
    downup);
  log(17, "keyboard %s code %u(%x) sym %x mod %x -> code %u(%x) mod %x %s\n",
    down ? "down" : "up",
    (unsigned)(kev.keysym.scancode),
    (unsigned)(kev.keysym.scancode),
    (unsigned)(kev.keysym.sym),
    (unsigned)mod,
    (unsigned)conv_c,
    (unsigned)conv_c,
    (unsigned)conv_m,
    downup ? "downup" : "-");
  if (conv_c != 0) {
    if (downup) {
      if (down) {
        hid.key_down(conv_c, conv_m);
        /* 一定時間後にkey_upを実行する */
        keycmd kc { };
        kc.code = conv_c;
        kc.mod = conv_m;
        kc.cmd = 0x05; // up
        auto now = std::chrono::system_clock::now();
        kc.time = now + std::chrono::milliseconds(100);
          // 100ms後に実行する。あまり短いとリモートデバイスのOSによっては
          // 認識されないことがある。
        delayed_keycmd.push_back(kc);
      } else {
        /* ignore */
      }
    } else {
      if (down) {
        hid.key_down(conv_c, conv_m);
      } else {
        hid.key_up(conv_c, conv_m);
      }
    }
  }
}

bool
vcnet_control::send_delayed_keycmd()
{
  // クリップボードのペーストによって積まれたキーイベントを送信する。
  bool r = false;
  auto now = std::chrono::system_clock::now();
  while (!delayed_keycmd.empty()) {
    auto const& kc = *delayed_keycmd.begin();
    if (now < kc.time) {
      #if 0
      log(18, "send_delayed_keycmd delayed_keycmd %u %d\n",
        (unsigned)delayed_keycmd.size(), (int)duration_ms(now, kc.time));
      #endif
      break;
    }
    log(18, "send delayed %u %u %d\n", (unsigned)kc.code, (unsigned)kc.mod,
      (int)kc.cmd);
    hid.key_up_all(); // altなどを押している状態であればクリア
    if (kc.cmd == 4) {
      hid.key_down(kc.code, kc.mod);
    } else {
      hid.key_up(kc.code, kc.mod);
    }
    delayed_keycmd.pop_front();
    r = true;
  }
  return r;
}

void
vcnet_control::clear_delayed_keycmd()
{
  delayed_keycmd.clear();
}

void
vcnet_control::handle_event(SDL_Event const& ev, bool& done_r)
{
  log(19, "handle event %d\n", (int)ev.type);
  done_r = false;
  switch (ev.type) {
  case SDL_QUIT:
    log(1, "SDL_QUIT\n");
    done_r = true;
    return;
  case SDL_KEYDOWN:
  case SDL_KEYUP:
    log(19, "sdl key%s scancode=%u(%x) keycode=%u(%x) mod=%x\n",
      (ev.type == SDL_KEYDOWN) ? "down" : "up",
      ev.key.keysym.scancode, ev.key.keysym.scancode,
      ev.key.keysym.sym, ev.key.keysym.sym,
      ev.key.keysym.mod);
    switch (ev.key.keysym.scancode) {
    case SDL_SCANCODE_RETURN:
      // 右ALT+RETURN: フルスクリーンにする・戻す。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          wnd.toggle_fullscreen();
        }
        return;
      }
      break;
    case SDL_SCANCODE_T:
      // 右ALT+T: test_adv7611.txtの内容をadv7611にセット。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          std::ifstream f("test_adv7611.txt");
          std::string line;
          while (std::getline(f, line)) {
            std::string x, y, z;
            std::istringstream iss(line);
            std::getline(iss, x, ' ');
            std::getline(iss, y, ' ');
            std::getline(iss, z, ' ');
            auto is_hex1b = [](std::string const& x) -> bool {
              if (x.size() != 2) {
                return false;
              }
              for (size_t i = 0; i < 2; ++i) {
                auto ch = x[i];
                if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
                  (ch >= 'A' && ch <= 'F')) {
                  continue;
                }
                return false;
              }
              return true;
            };
            if (!is_hex1b(x) || !is_hex1b(y) || !is_hex1b(z)) {
              continue;
            }
            std::array<uint8_t, 3> v { };
            v[0] = (uint8_t)strtoul(x.c_str(), nullptr, 16);
            v[1] = (uint8_t)strtoul(y.c_str(), nullptr, 16);
            v[2] = (uint8_t)strtoul(z.c_str(), nullptr, 16);
            adv7611_cmds.push_back(std::move(v));
            // std::cout << "x=" << x << " y=" << y << " z=" << z << std::endl;
          }
        }
        return;
      }
      break;
    case SDL_SCANCODE_DELETE:
      // 右ALT+DELETE: Ctrl+Alt+Delを送る。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          log(19, "send ctrl-alt-del\n");
          // TODO:
          // conn.send_keyboard(0x4c, 0x05, 0x06);
        }
        return;
      }
      break;
    case SDL_SCANCODE_V:
      // 右ALT+V: クリップボードの内容をUSBキーボードを使ってペーストする。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          log(19, "paste clipboard\n");
          char *txt = SDL_GetClipboardText();
          std::unique_ptr<char, void (*)(void *)> txt_del(txt,
            [](void *p) { SDL_free(p); });
          std::string stext(txt, strlen(txt));
          send_text(stext);
        }
        return;
      }
      break;
    case SDL_SCANCODE_I:
      // 右ALT+I: サーバ側でインタレース送信するかどうか切り替える。
      // TCPサーバのときのみ有効。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          uint32_t v = conn.get_server_flags();
          v ^= 0x01;
          log(19, "server_flags=%x\n", (unsigned)v);
          conn.send_server_flags(v);
        }
        return;
      }
      break;
    case SDL_SCANCODE_S:
      // 右ALT+S: 左上の詳細情報を表示・非表示にする。何回も押すと色が変わる。
      if ((ev.key.keysym.mod & (KMOD_RALT | KMOD_RGUI)) != 0) {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
          show_info = !show_info;
          if (show_info) {
            uint32_t c = msg_color;
            c = ((c & 0x010000) >> 14) | ((c & 0x000100) >> 7) | (c & 0x01);
            c = (c + 1) & 0x07;
            msg_color = 0;
            msg_color |= (c & 04) ? 0xff0000 : 0;
            msg_color |= (c & 02) ? 0x00ff00 : 0;
            msg_color |= (c & 01) ? 0x0000ff : 0;
          }
        }
        return;
      }
      break;
    case SDL_SCANCODE_RALT:
    case SDL_SCANCODE_RGUI:
      // 右ALT: マウス捕獲を解除する。
      if (grab_mouse) {
        if (ev.type == SDL_KEYDOWN) {
          log(19, "ungrab mouse\n");
          SDL_SetRelativeMouseMode(SDL_FALSE);
          grab_mouse = false;
        }
        return;
      }
      break;
    default:
      break;
    }
    handle_keyboard(ev.key, (ev.type == SDL_KEYDOWN));
    break;
  case SDL_WINDOWEVENT:
    {
      if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        wnd.disable_ime();
        last_suppress_frames_time = std::chrono::system_clock::now();
      } else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        if (grab_mouse) {
          SDL_SetRelativeMouseMode(SDL_FALSE);
          grab_mouse = false;
        }
        hid.up_all();
        clear_delayed_keycmd();
      }
      wnd.resize_window_if();
    }
    break;
  case SDL_MOUSEMOTION:
    if (conn.get_absmouse()) {
      uint32_t const mouse_x = (uint32_t)ev.motion.x;
      uint32_t const mouse_y = (uint32_t)ev.motion.y;
      SDL_Rect rect = wnd.get_cur_draw_rect();
      log(19, "absmouse rect=(%u %u %u %u) mouse=(%u %u)\n",
        rect.x, rect.y, rect.w, rect.h, mouse_x, mouse_y);
      if (rect.x <= (int)mouse_x && rect.x + rect.w >= (int)mouse_x &&
        rect.y <= (int)mouse_y && rect.y + rect.h >= (int)mouse_y) {
        uint32_t rx = mouse_x - rect.x;
        uint32_t ry = mouse_y - rect.y;
        uint32_t const x = rx * 32767u / rect.w;
        uint32_t const y = ry * 32767u / rect.h;
        hid.absmouse_move(x, y);
        log(19, "absmouse %u %u\n", x, y);
      }
    } else if (grab_mouse) {
      auto const& mmev = ev.motion;
      Sint32 xrel = mmev.xrel;
      Sint32 yrel = mmev.yrel;
      hid.mouse_move(xrel, yrel, 0, 0);
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    {
      auto const& mb = ev.button;
      Uint32 const btns = SDL_GetMouseState(nullptr, nullptr);
      log(19, "mousedown button=%d\n", mb.button);
      if (!grab_mouse && !conn.get_absmouse()) {
        if (ev.type == SDL_MOUSEBUTTONDOWN &&
          mb.button == SDL_BUTTON_LEFT) {
          // 左クリック: マウスが絶対値モードではないときは、マウスを捕獲する。
          log(19, "grab mouse\n");
          SDL_SetRelativeMouseMode(SDL_TRUE);
          grab_mouse = true;
        }
      } else if (ev.type == SDL_MOUSEBUTTONDOWN &&
        mb.button == SDL_BUTTON_X2) {
        // マウスボタン5クリック: マウス捕獲を解除する。
        SDL_SetRelativeMouseMode(SDL_FALSE);
        grab_mouse = false;
      } else if (ev.type == SDL_MOUSEBUTTONDOWN &&
        mb.button == SDL_BUTTON_X1) {
        // マウス2,3,4クリック: システムリセットを送信。
        // (現在は動作しない)
        if ((btns & SDL_BUTTON(3)) != 0 && (btns & SDL_BUTTON(2)) != 0) {
          conn.send_system_reset();
        }
      } else {
        uint8_t b = 0;
        if (mb.button == SDL_BUTTON_LEFT) {
          b = 0;
        }
        if (mb.button == SDL_BUTTON_RIGHT) {
          b = 1;
        }
        if (mb.button == SDL_BUTTON_MIDDLE) {
          b = 2;
        }
        if (conn.get_absmousebutton()) {
          hid.absmouse_button(b, (ev.type == SDL_MOUSEBUTTONDOWN));
        } else {
          hid.mouse_button(b, (ev.type == SDL_MOUSEBUTTONDOWN));
        }
      }
    }
    break;
  case SDL_MOUSEWHEEL:
    // if (grab_mouse)
    {
      auto const& mwh = ev.wheel;
      Sint32 x = -mwh.x;
      Sint32 y = mwh.y;
      if (conn.get_wheel_dir()) {
        y = -y;
      }
      hid.mouse_move(0, 0, x, y);
    }
    break;
  default:
    break;
  }
}

void
vcnet_control::mainloop()
{
  wnd.resize_texture_if(1920, 1080);
  bool done = false;
  while (!done) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0 && !done) {
      handle_event(ev, done);
    }
    if (send_delayed_keycmd()) {
      std::vector<uint8_t> buf;
      bool has_event = false;
      hid.get_spi_buffer(conn.get_spi_index(), buf, has_event);
      conn.send_raw(buf.data(), buf.size(), has_event);
    }
    bool videoframe_done = false;
    bool has_net_event = false;
    conn.recv_data(pix, videoframe_done, has_net_event, audiobuf);
    auto now = std::chrono::system_clock::now();
    if (duration_ms(now, last_draw_time) > 1000) {
      videoframe_done = true; // timeout
    }
    if (duration_ms(now, last_mouse_send_time) > 3) {
      // 3ms間隔でHID mouse等の状態を送信する。
      last_mouse_send_time = now;
      if (!adv7611_cmds.empty()) {
        auto& v = adv7611_cmds.front();
        conn.send_adv7611_i2c(v[0] >> 1u, v[1], v[2]);
        log(20, "send adv7611 %02x %02x %02x\n", (unsigned)v[0],
          (unsigned)v[1], (unsigned)v[2]);
        adv7611_cmds.pop_front();
      }
      {
        int n = SDL_NumJoysticks();
        if (n != num_joysticks_saved) {
          if (n != 0) {
            joy.open(0);
            joystick_name = std::string(SDL_JoystickNameForIndex(0));
          } else {
            joy.close();
            joystick_name = "";
          }
        }
        num_joysticks_saved = n;
        if (n != 0) {
          joy.update(hid, joymap);
        }
      }
      {
        std::vector<uint8_t> buf;
        bool has_event = false;
        hid.get_spi_buffer(conn.get_spi_index(), buf, has_event);
        conn.send_raw(buf.data(), buf.size(), has_event);
      }
    }
    if (videoframe_done) {
      std::string msg;
      {
        auto svrstat = conn.get_server_stat();
        auto video_locked = svrstat & 0x04;
        auto has_signal = svrstat & 0x08;
        if (svrstat == 0) {
          msg = "Disconnected";
          last_msg_time = now;
        } else if (!video_locked) {
          msg = "No Video Signal";
          last_msg_time = now;
        } else if (!conn.get_video_signal_stable()) {
          msg = "Unstable Video Signal";
          last_msg_time = now;
        } else if (!has_signal) {
          msg = "Video Signal Detected";
          last_msg_time = now;
        } else if (duration_ms(now, last_msg_time) < 3000) {
          msg = "H:" + std::to_string(pix.get_width()) + " V:" +
            std::to_string(pix.get_height());
        } else if (show_info) {
          msg = std::to_string(conn.get_fps())
             + ((conn.get_server_flags() & 0x01) ? "i" : "p")
             + " H:" + std::to_string(pix.get_width())
             + " V:" + std::to_string(pix.get_height())
             + " " + std::to_string(conn.get_audio_freq()) + "Hz" +
             + " " + to_hexstr(svrstat)
             + " " + to_hexstr(conn.get_server_flags())
             + " (" + joystick_name
             + ")";
        }
      }
      wnd.set_message(msg, msg_color);
      conn.increment_videoframe_count();
      if (wnd.get_use_gl()) {
        wnd.draw_window_gl(pix, conn.get_video_signal_stable());
      } else {
        wnd.update_texture_sdl(pix);
        wnd.draw_window_sdl(pix, conn.get_video_signal_stable());
      }
      last_draw_time = now;
      {
        auto audio_freq = conn.get_audio_freq();
        if (audio_freq != 0) {
          uint32_t const values[] = { 11025, 22050, 24000, 44100, 48000 };
          size_t const n = sizeof(values) / sizeof(values[0]);
          double const f = (double)audio_freq;
          uint32_t devfreq = 0;
          for (size_t i = 0; i < n; ++i) {
            double const v = f / (double)values[i];
            if (v > 0.97 && v < 1.03) {
              devfreq = values[i];
              break;
            }
          }
          if (devfreq != 0) {
            audio.open_device_if(devfreq);
          }
        }
      }
    } else {
    }
    // log(0, "audiobuf size=%zu\n", audiobuf.size());
    conn.increment_audio_sample_count((uint32_t)audiobuf.size());
    if (duration_ms(now, last_suppress_frames_time) > 1000) {
      audio.queue_audio(audiobuf);
    }
  }
  log(1, "quit\n");
}

}; // namespace vcnet

//////////////////////////////////////////////////////////////

#ifdef _MSC_VER
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  PWSTR pCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
  using namespace vcnet;
  std::string cmdname;
  std::vector<std::string> conf_files = { std::string("vcnet.conf") };
  #ifdef _MSC_VER
  WSAData wsadata;
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
      log(0, "failed init winsock2\n");
      return 1;
  }
  {
    int argc = 0;
    LPWSTR* argvw = CommandLineToArgvW(GetCommandLine(), &argc);
    std::unique_ptr<LPWSTR, void(*)(LPWSTR *)> argvw_del(argvw,
      [](LPWSTR *p) { LocalFree(p); });
    if (argc >= 1) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
      cmdname = conv.to_bytes(std::wstring(argvw[0]));
    }
    if (argc >= 2) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
      for (int i = 1; i < argc; ++i) {
        conf_files.push_back(conv.to_bytes(std::wstring(argvw[i])));
      }
    }
  }
  {
    #if 0
    // TODO: 意味あるのか？
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    #endif
  }
  #else
  signal(SIGPIPE, SIG_IGN);
  cmdname = std::string(argv[0]);
  if (argc >= 2) {
    for (int i = 1; i < argc; ++i) {
      conf_files.push_back(std::string(argv[1]));
    }
  }
  #endif
  if (!cmdname.empty()) {
    log(0, "cmdname=%s\n", cmdname.c_str());
    #ifdef _MSC_VER
    std::replace(cmdname.begin(), cmdname.end(), '\\', '_');
    HANDLE hnd_lock = CreateMutexA(nullptr, FALSE, cmdname.c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      log(0, "CreateMutex %s\n", cmdname.c_str());
      return 1;
    }
    #endif
  }
  {
    Uint32 f = 0;
    f |= SDL_INIT_VIDEO;
    f |= SDL_INIT_AUDIO;
    f |= SDL_INIT_JOYSTICK;
    if (SDL_Init(f) != 0) {
      log(0, "error initializing SDL: %s\n", SDL_GetError());
    }
    if (TTF_Init() != 0) {
      log(0, "error initializing TTF: %s\n", TTF_GetError());
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
  }
  vcnet_control ctrl(conf_files);
  ctrl.mainloop();
  return 0;
}

