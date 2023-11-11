// vim: sw=2 ts=8 ai

#ifdef __APPLE__
  #include <TargetConditionals.h>
#endif

#if TARGET_OS_IPHONE || defined(__ANDROID__)
  #define VCNET_MOBILE
#endif

#ifdef _MSC_VER
  // windows
  #define _CRT_SECURE_NO_WARNINGS 1 // fopen
  #define NOMINMAX
  #include <WinSock2.h>
  #include <ws2tcpip.h>
  #include <Windows.h>
  // #include <mstcpip.h>
  #include <GL/glew.h>
  #include <SDL.h>
  #include <SDL_syswm.h>
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
#else
  // unix
  #include <sys/types.h>
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <poll.h>
  #include <SDL.h>
  #define GL_GLEXT_PROTOTYPES
  #ifdef __APPLE__
    #if TARGET_OS_IPHONE
      // iOS
      #define GLES_SILENCE_DEPRECATION
      #include <OpenGLES/ES2/gl.h>
      #include <OpenGLES/ES2/glext.h>
      #define VCNET_GLES
    #else
      // macOS
      #define GL_SILENCE_DEPRECATION
      #include <OpenGL/gl3.h>
      #include <OpenGL/gl3ext.h>
    #endif
  #elif defined(__ANDROID__)
    // android
    #include <android/log.h>
    #include <SDL_opengl.h>
    #define VCNET_GLES
  #else
    // linux
    #include <SDL_opengl.h>
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
#include <stdlib.h>
#include <limits.h>
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
#include <iomanip>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#ifdef _MSC_VER
#include "mediafoundation_player.h"
#endif

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
static uint64_t logmask = 0x03;

static void
log_valist(const char *fmt, va_list *ap_p)
{
  #ifdef __ANDROID__
  {
    va_list ap;
    va_copy(ap, *ap_p);
    __android_log_vprint(ANDROID_LOG_INFO, "vcnet", fmt, ap);
    va_end(ap);
  }
  #else
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
  #endif
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

void
fatal_error(const char *msg)
{
  log(0, "fatal_error: %s\n", msg);
  exit(1);
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

std::string
get_pref_path()
{
  char *buf = SDL_GetPrefPath("vcnet", "vcnet");
  std::unique_ptr<void, void(*)(void *)> buf_del(buf,
    [](void *p) { SDL_free(p); });
  std::string r;
  if (buf != nullptr) {
    r = std::string(buf);
    if (!r.empty() && r[r.size() - 1] != '\\' && r[r.size() - 1] != '/') {
      r += "/";
    }
  }
  return r;
}

std::string
load_file(std::string const& fname, bool *loaded_pref_r = nullptr)
{
  if (loaded_pref_r != nullptr) {
    *loaded_pref_r = false;
  }
  void *buf = nullptr;
  // pref pathから読んでみて、なかったらfnameそのままのファイル名を読む
  auto const pref_path = get_pref_path();
  auto const pref_fn = pref_path + fname;
  size_t sz = 0;
  bool load_pref = false;
  buf = SDL_LoadFile(pref_fn.c_str(), &sz);
  if (buf == nullptr) {
    buf = SDL_LoadFile(fname.c_str(), &sz);
  } else {
    load_pref = true;
  }
  std::unique_ptr<void, void(*)(void *)> buf_del(buf,
    [](void *p) { SDL_free(p); });
  if (buf == nullptr) {
    log(0, "failed to load file: %s\n", fname.c_str());
  } else {
    log(0, "loaded file %s size=%zu\n",
      load_pref ? pref_fn.c_str() : fname.c_str(), sz);
  }
  if (loaded_pref_r != nullptr) {
    *loaded_pref_r = load_pref;
  }
  std::string r((char *)buf, sz);
  return r;
}

struct auto_rwops {
  SDL_RWops *rwops = nullptr;
  auto_rwops() { }
  auto_rwops(auto_rwops const&) = delete;
  auto_rwops& operator =(auto_rwops const&) = delete;
  ~auto_rwops() {
    if (rwops != nullptr) {
      SDL_RWclose(rwops);
    }
  }
};

bool
save_file(std::string const& fname, std::string const& data)
{
  auto_rwops arwops;
  arwops.rwops = SDL_RWFromFile(fname.c_str(), "wb");
  if (arwops.rwops == nullptr) {
    return false;
  }
  if (SDL_RWwrite(arwops.rwops, data.data(), data.size(), 1) != 1) {
    return false;
  }
  int e = SDL_RWclose(arwops.rwops);
  arwops.rwops = nullptr;
  return e == 0;
}

template <typename T> uint64_t
time_us(T t)
{
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
    t.time_since_epoch()).count();
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
to_hexstr(unsigned long long v, int precision = 0)
{
  std::stringstream ss;
  if (precision != 0) {
    ss << std::setw(precision) << std::setfill('0');
  }
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

static uint32_t
compat_inet_addr(const char *s)
{
  uint32_t addr = 0;
  #ifdef _MSC_VER
  InetPtonA(AF_INET, s, &addr);
  #else
  addr = inet_addr(s);
  #endif
  return addr;
}

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
vcnet_send_wol_magic_packet(const char *macaddr_str, const char *ipaddr_str)
{
  std::array<uint8_t, 6> macaddr { };
  if (!parse_macaddr(macaddr_str, macaddr)) {
    log(0, "failed to parse mac address: '%s'\n", macaddr_str);
    return;
  }
  auto_socket sock(AF_INET, SOCK_DGRAM, 0);
  if (!sock) {
    log(0, "wol socket: %d\n", get_last_error());
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
  addr.sin_addr.s_addr = compat_inet_addr(ipaddr_str);
  log(1, "sending wol magic packet: macaddr=%s broadcast_ipaddr=%s\n",
    macaddr_str, ipaddr_str);
  auto e = sendto(sock.get(), cast_win32(const char *)&pkt[0],
    cast_win32(int)pkt.size(),
    0, (sockaddr const *)&addr, sizeof(addr));
  if (e == SOCKET_ERROR) {
    log(0, "wol sendto: %d\n", get_last_error());
    return;
  }
}

//////////////////////////////////////////////////////////////

struct vcnet_config {
  std::shared_ptr<std::map<std::string, std::string>> mapp;
  std::string conf_text;
  std::vector<std::string> filenames;
  bool user_pref = false;
  vcnet_config();
  vcnet_config(std::string const& s);
  vcnet_config(std::vector<std::string> const& fns);
  void conf_set_logmask();
  std::string get_str(std::string const& key, std::string const& defval)
    const;
  unsigned long long get_uint(std::string const& key,
    unsigned long long defval) const;
  long long get_int(std::string const& key, long long defval) const;
  double get_double(std::string const& key, double defval) const;
  std::vector<std::string> get_keys(std::string const& prefix) const;
  std::string get_conf_text() const { return conf_text; }
  std::vector<std::string> get_filenames() const { return filenames; }
  bool get_user_pref() const { return user_pref; }
};

vcnet_config::vcnet_config()
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
  // 引数の文字列リストの個々をファイル名とみなしてその内容を全て読み込む。
  // ただし文字列が"key=value"の形だったときは設定項目keyに値valueをセット。
  filenames = fns;
  for (size_t i = 0; i < fns.size(); ++i) {
    auto const& fn = fns[i];
    auto const eqpos = fn.find('=');
    if (eqpos != fn.npos) {
      // fnに'='を含むとき、左辺の設定項目の設定値を右辺にセットする
      auto const k = fn.substr(0, eqpos);
      auto const v = fn.substr(eqpos + 1);
      (*mapp)[k] = v;
      conf_text += k + ": " + v + "\n";
    } else {
      // fnに'='を含まないとき、その名前のファイルを開いてその内容をよみこむ
      bool loaded_pref = false;
      auto s = load_file(fn, &loaded_pref);
      auto m = parse_map(std::istringstream(s), '\n', ':');
      for (auto iter = m.begin(); iter != m.end(); ++iter) {
        (*mapp)[iter->first] = iter->second;
      }
      conf_text += s;
      if (conf_text.size() > 0 && conf_text[conf_text.size() - 1] != '\n') {
        conf_text += "\n";
      }
      if (loaded_pref) {
        user_pref = true;
      }
    }
  }
  conf_set_logmask();
}

void
vcnet_config::conf_set_logmask()
{
  auto s = this->get_str("logmask", "3");
  auto v = strtoull(s.c_str(), nullptr, 0);
  log(0, "logmask=%llu\n", v);
  set_logmask(v);
}

std::vector<std::string>
vcnet_config::get_keys(std::string const& prefix) const
{
  std::vector<std::string> r;
  auto iter = mapp->lower_bound(prefix);
  while (iter != mapp->end()) {
    auto const& k = iter->first;
    if (k.size() < prefix.size() ||
      !std::equal(prefix.begin(), prefix.end(), k.begin())) {
      break;
    }
    r.push_back(k);
    ++iter;
  }
  return r;
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

long long
vcnet_config::get_int(std::string const& key, long long defval) const
{
  if (mapp == nullptr) {
    return defval;
  }
  auto const i = mapp->find(key);
  if (i == mapp->end()) {
    return defval;
  }
  return strtoll(i->second.c_str(), nullptr, 0);
}

double
vcnet_config::get_double(std::string const& key, double defval) const
{
  if (mapp == nullptr) {
    return defval;
  }
  auto const i = mapp->find(key);
  if (i == mapp->end()) {
    return defval;
  }
  return strtod(i->second.c_str(), nullptr);
}

//////////////////////////////////////////////////////////////

enum vcnet_video_format_e {
  vcnet_video_format_e_rgb888 = 0,
  vcnet_video_format_e_yuv411 = 1,
  vcnet_video_format_e_yuv422 = 2,
  vcnet_video_format_e_rgba8888 = 3,
  vcnet_video_format_e_nv12 = 4,
  vcnet_video_format_e_yuy2 = 5,
};

const char *vcnet_video_format_str(vcnet_video_format_e v)
{
  switch (v) {
  case vcnet_video_format_e_rgb888:
    return "RGB888";
  case vcnet_video_format_e_yuv411:
    return "YUV411";
  case vcnet_video_format_e_yuv422:
    return "YUV422";
  case vcnet_video_format_e_rgba8888:
    return "RGBA8888";
  case vcnet_video_format_e_nv12:
    return "NV12";
  case vcnet_video_format_e_yuy2:
    return "YUY2";
  default:
    return "Unknown";
  }
}

//////////////////////////////////////////////////////////////

struct vcnet_pixels {
private:
  std::vector<unsigned char> pixels3;
  size_t pixels3_offset = 0;
  unsigned width = 0;
  unsigned height = 0;
  vcnet_video_format_e format = vcnet_video_format_e_rgb888;
  void recreate_buffer();
public:
  void set_format(vcnet_video_format_e v);
  vcnet_video_format_e get_format() const { return format; }
  unsigned get_bits_per_pixel() const {
    return (format == 0) ? 24 : (format == 1) ? 12 : 16;
  } 
  unsigned char *begin() { return pixels3.data(); }
  const unsigned char *begin() const { return pixels3.data(); }
  unsigned char *end() { return begin() + size(); }
  const unsigned char *end() const { return begin() + size(); }
  unsigned char *wrpos() { return begin() + pixels3_offset; }
  size_t size() const { return pixels3.size(); }
  size_t wrsize() const { return size() - pixels3_offset; }
  size_t get_offset() const { return pixels3_offset; }
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
vcnet_pixels::set_format(vcnet_video_format_e v)
{
  if (format != v) {
    format = v;
    recreate_buffer();
  }
}

void
vcnet_pixels::resize_if(unsigned w, unsigned h)
{
  if (w != width || h != height) {
    if ((width % 4) != 0) {
      width = 0;
      height = 0;
    } else {
      width = w;
      height = h;
    }
    recreate_buffer();
  }
}

void
vcnet_pixels::recreate_buffer()
{
  log(1, "pixels3 recreate %u %u %u\n", width, height,
    this->get_bits_per_pixel());
  pixels3.resize(width * height * get_bits_per_pixel() / 8);
  pixels3_offset = 0;
}

//////////////////////////////////////////////////////////////

struct vcnet_conn {
public:
  struct device_info {
    vcnet_config device_conf;
    std::string title;
    int spi_index = -1;
    int gpio_index = -1;
    bool enable_video = 1;
    bool enable_audio = 1;
    std::string wol;
    std::string wol_br;
    std::vector<std::string> ircmd;
    bool absmouse = false;
    bool absmousebutton = false;
    bool wheel_dir = false;
    uint32_t cropx_val = 0;
    std::string socktype_str;
    std::string addr_str;
    int socktype = 0;
    uint32_t addr = 0;
    uint16_t port = 0;
  };
private:
  uint32_t server_index = 0;
  uint32_t num_devices = 0;
  uint32_t cur_device = 0;
  vcnet_config conf;
  device_info cur;
  uint32_t ir_sent = 0;
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
  uint32_t stat_audiobuf_min = (uint32_t)-1;
  uint32_t stat_audiobuf_max = 0;
  uint32_t stat_drop_count = 0;
  uint32_t total_videoframe_count = 0;
  uint32_t last_resize_videoframe_count = 0; // 信号不良の判定に使う
  bool video_signal_stable = true;
  uint32_t fps = 0;
  std::deque<uint32_t> audio_freq_me;
  uint32_t audio_freq = 0;
  uint32_t prev_audio_sample = 0;
  uint32_t audiobuf_min = 0;
  uint32_t audiobuf_max = 0;
  uint32_t drop_count = 0;
  bool incomplete_videoframe = true;
  uint32_t server_stat = 0;
  std::array<uint32_t, 2> server_flags { 0x1, 0x0 }; // サーバ側の設定値
  #ifdef VCNET_USE_OVERLAPPED_IO
  struct ovl_buffer {
    WSAOVERLAPPED ovl { };
    char buffer[6000] { };
  };
  std::deque<std::shared_ptr<ovl_buffer>> ovl_reads;
  #endif
  bool use_poll = false;
    // すくなくともwindowsではpoll使わないほうがCPU食わない
  uint32_t video_count_seq = 0;
  uint32_t video_count_eq_saved = 0;
  uint32_t video_count_ne_saved = 0;
  uint32_t video_count_eq_diff = 0;
  uint32_t video_count_ne_diff = 0;
  uint64_t net_roundtrip_time = 0;
private:
  void open_socket();
  void close_socket();
  size_t write_socket();
  size_t read_socket(void *buf, size_t buflen);
  void parse_netframe(uint8_t tag0, uint32_t tag1, unsigned char *data,
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
  vcnet_conn(uint32_t server_index0, uint32_t num_devices0,
    vcnet_config const& conf, bool& found_couf_r);
  vcnet_conn(vcnet_conn const&) = delete;
  vcnet_conn& operator =(vcnet_conn const&) = delete;
  uint32_t get_num_devices() const { return num_devices; }
  uint32_t get_cur_device() const { return cur_device; }
  bool get_device_info(uint32_t idx, device_info& d_r) const;
  bool set_cur_device(uint32_t idx);
  std::string get_title() const { return cur.title; }
  int get_spi_index() const { return cur.spi_index; }
  bool get_enable_video() const { return cur.enable_video; }
  bool get_enable_audio() const { return cur.enable_audio; }
  bool get_absmouse() const { return cur.absmouse; }
  void set_absmouse(bool v) { cur.absmouse = v; }
  bool get_absmousebutton() const { return cur.absmousebutton; }
  bool get_wheel_dir() const { return cur.wheel_dir; }
  uint32_t get_cropx_val() const { return cur.cropx_val; }
  void send_wol();
  void send_netframe(uint8_t typ, const void *data, uint8_t data_len,
    bool is_user_input = true);
  void send_raw_spi(const void *data, size_t data_len, bool has_event);
  void send_heartbeat_if();
  void send_ir();
  void send_system_reset();
  void send_server_flags();
  void send_adv7611_i2c(uint8_t i2c_addr7, uint8_t reg, uint8_t val);
  uint32_t get_server_flags(size_t idx) const;
  uint32_t get_server_flags_mask(size_t idx, uint32_t bit_offset,
    uint32_t nbits) const;
  void set_server_flags(size_t idx, uint32_t v);
  void set_server_flags_mask(size_t idx, uint32_t bit_offset, uint32_t nbits,
    uint32_t v);
  void recv_data(vcnet_pixels& pix, bool& videoframe_done_r, 
    bool& has_net_event_r, std::vector<uint32_t>& audiobuf);
  void increment_videoframe_count();
  void increment_audio_sample_count(uint32_t v);
  void stat_audiobuf(uint32_t v);
  uint32_t get_server_stat() const;
  uint32_t get_fps() const { return fps; }
  uint32_t get_audio_freq() const { return audio_freq; }
  uint32_t get_audiobuf_min() const { return audiobuf_min; }
  uint32_t get_audiobuf_max() const { return audiobuf_max; }
  uint32_t get_drop_count() const { return drop_count; }
  bool get_video_signal_stable() const { return video_signal_stable; }
  std::chrono::system_clock::time_point get_last_open_time() const {
    return last_open_time;
  };
  std::chrono::system_clock::time_point get_last_read_time() const {
    return last_read_time;
  };
  std::pair<uint32_t, uint32_t> get_video_count_diff() const {
    return std::make_pair(video_count_eq_diff, video_count_ne_diff);
  }
  uint64_t get_net_roundtrip_time() const {
    return net_roundtrip_time;
  }
  void set_gpio_out(bool v);
};

vcnet_conn::~vcnet_conn()
{
  close_socket();
}

vcnet_conn::vcnet_conn(uint32_t server_index0, uint32_t num_devices0,
  vcnet_config const& conf0, bool& found_conf_r)
  : server_index(server_index0), num_devices(num_devices0), conf(conf0)
{
  last_open_time = last_user_input_time = std::chrono::system_clock::now();
  uint32_t n = 0;
  auto const key_prefix = "device" + std::to_string(server_index) + ".";
  auto devices = conf.get_keys(key_prefix);
  for (auto const& e: devices) {
    assert(e.size() >= 6);
    const char *const p = e.c_str() + key_prefix.size();
    unsigned long const v = strtoul(p, nullptr, 10);
    if (v == ULONG_MAX) {
      continue;
    }
    n = std::max(n, (uint32_t)(v + 1));
  }
  if (server_index == 0) {
    num_devices = n;
  } else {
    num_devices = num_devices0;
  }
  found_conf_r = (n != 0);
  cur_device = (uint32_t)conf.get_uint("device", 0);
  set_cur_device(cur_device);
}

bool
vcnet_conn::get_device_info(uint32_t idx, device_info& d_r) const
{
  d_r = { };
  auto s = conf.get_str("device" + std::to_string(server_index) + "."
    + std::to_string(idx), std::string());
  if (s.empty()) {
    return false;
  }
  auto device_conf = vcnet_config(s);
  d_r.device_conf = device_conf;
  d_r.spi_index = (int)device_conf.get_int("spi", -1);
  d_r.gpio_index = (int)device_conf.get_int("gpio", -1);
  d_r.enable_video = (bool)device_conf.get_uint("enable_video", 1);
  d_r.enable_audio = (bool)device_conf.get_uint("enable_audio", 1);
  d_r.wol = device_conf.get_str("wol", "");
  d_r.wol_br = device_conf.get_str("wol_br", "255.255.255.255");
  {
    auto v = device_conf.get_str("ir0", device_conf.get_str("ir", ""));
    for (uint32_t i = 1; !v.empty(); ++i) {
      d_r.ircmd.push_back(v);
      v = device_conf.get_str("ir" + std::to_string(i), "");
    }
  }
  d_r.absmouse = (bool)device_conf.get_uint("absmouse", 0);
  d_r.absmousebutton = (bool)device_conf.get_uint("absmousebutton", 0);
  d_r.wheel_dir = (bool)device_conf.get_uint("wheel_dir", 0);
  d_r.cropx_val = ((uint32_t)device_conf.get_uint("cropw", 0) << 16u) |
    ((uint32_t)device_conf.get_uint("cropx", 0) & 0xffffu);
  d_r.title = device_conf.get_str("title",
    "device0." + std::to_string(idx));
  auto st = device_conf.get_str("socktype", "tcp");
  d_r.socktype_str = st;
  d_r.socktype = 0;
  if (st == "tcp") {
    d_r.socktype = SOCK_STREAM;
  } else if (st == "udp") {
    d_r.socktype = SOCK_DGRAM;
  }
  d_r.port = (uint16_t)device_conf.get_uint("port", 5001);
  auto addr_str = device_conf.get_str("ip", "192.168.250.250");
  d_r.addr_str = addr_str;
  d_r.addr = compat_inet_addr(addr_str.c_str());
  return true;
}

bool
vcnet_conn::set_cur_device(uint32_t idx)
{
  device_info di { };
  if (!get_device_info(idx, di)) {
    return false;
  }
  bool need_reconnect = false;
  if (cur.addr != di.addr || cur.port != di.port ||
    cur.socktype != di.socktype) {
    need_reconnect = true;
  }
  if (need_reconnect) {
    close_socket();
  }
  cur_device = idx;
  cur = di;
  log(0, "vcnet_conn device=%u spi=%u wol=%s wol_br=%s ir=%s absmouse=%u "
    "absmb=%u wheel=%u\n",
    (unsigned)cur_device,
    cur.spi_index, cur.wol.c_str(), cur.wol_br.c_str(),
    cur.ircmd.empty() ? "" : cur.ircmd.front().c_str(),
    (unsigned)cur.absmouse, (unsigned)cur.absmousebutton,
    (unsigned)cur.wheel_dir);
  if (need_reconnect) {
    open_socket();
  }
  send_wol();
  ir_sent = 0;
  video_count_seq = 0;
  video_count_eq_diff = 0;
  video_count_ne_diff = 0;
  return true;
}

void
vcnet_conn::send_wol()
{
  if (!cur.wol.empty()) {
    vcnet_send_wol_magic_packet(cur.wol.c_str(), cur.wol_br.c_str());
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
  log(2, "close_socket %s\n", cur.socktype_str.c_str());
  sd = INVALID_SOCKET;
}

void
vcnet_conn::open_socket()
{
  if (cur.addr == 0) {
    return;
  }
  log(2, "open_socket\n");
  last_open_time = last_read_time = last_write_time
    = last_hb_time = std::chrono::system_clock::now();
  recv_buffer_offset = 0;
  send_buffer.clear();
  stat_videoframe_count = 0;
  stat_audio_sample_count = 0;
  stat_audiobuf_min = (uint32_t)-1;
  stat_audiobuf_max = 0;
  stat_drop_count = 0;
  total_videoframe_count = 0;
  last_resize_videoframe_count = 0;
  video_signal_stable = true;
  fps = 0;
  audio_freq = 0;
  incomplete_videoframe = true;
  sd = socket(AF_INET, cur.socktype, 0);
  if (sd == INVALID_SOCKET) {
    log(0, "open_socket: socket: %d\n", get_last_error());
    close_socket();
    return;
  }
  sockaddr_in addr = { };
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cur.port);
  addr.sin_addr.s_addr = cur.addr;
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
    if (cur.socktype == SOCK_STREAM) {
      int v = 1 * 1024 * 1024;
      if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
      v = 1 * 1024 * 1024;
      if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
      v = 1;
      if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
      #if 0
      #ifdef _MSC_VER
      v = 1;
      DWORD rbytes = 0;
      if (WSAIoctl(sd, SIO_TCP_SET_ACK_FREQUENCY, &v, sizeof(v), nullptr, 0,
        &rbytes, nullptr, nullptr) != 0) {
        log(0, "WSAIoctl: %d\n", get_last_error());
      }
      #endif
      #endif
    } else {
      int v = 10 * 1024 * 1024;
      if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
      v = 1 * 1024 * 1024;
      if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, cast_win32(const char *)&v,
        (socklen_t)sizeof(v)) != 0) {
        log(0, "setsockopt: %d\n", get_last_error());
      }
    }
    {
      int v = 0;
      socklen_t len = (socklen_t)sizeof(v);
      if (getsockopt(sd, SOL_SOCKET, SO_RCVBUF, cast_win32(char *)&v,
        &len) != 0) {
        log(0, "getsockopt: %d\n", get_last_error());
      }
      log(1, "rcvbuf %d\n", v);
    }
  }
  if (connect(sd, (const sockaddr *)&addr, (socklen_t)sizeof(addr)) != 0) {
    int en = get_last_error();
    if (!is_retryable_error(en)) {
      log(0, "failed to connect %x %u: e=%d\r\n", (unsigned)cur.addr,
        (unsigned)cur.port, en);
      close_socket();
      return;
    }
  }
  log(2, "connected\n");
}

// 送信TCP/UDPフレームのフォーマット
//   (受信UDPフレームについても1024バイト以下のときは下記と同じフォーマット)
//   +0 payload_len[7:0] (ヘッダの2byteは含まない長さ)
//   +1 type[7:0]
//   +2 payload...
// typeの意味 (vcnet_artyz7/vcnet/src/vcnet_main.c:vcnet_recv_cb()参照)
//   0: stat送信要求
//   1: マイコンへのi2cコマンド(USBマウス等)(廃止)
//   2: サーバのリセット要求
//   3: irコマンド
//   4: サーバフラグの書き換え
//   5: サーバ内i2cコマンド(adv7611等)
//   6: マイコンへspiで渡すデータ
//   7: (UDPサーバからクライアントへ)フレームカウンタ
//   8: irコマンド新フォーマット
//   9: GPIO(電源ボタン操作)
// payloadのフォーマット
//   type=0のとき:
//     +0 time_us[63:0] マイクロ秒単位の時刻。UDPのRTT測定に使用する。
//   type=3のとき:
//     +0 ir_cmd[15:0] ...   16bitのコマンドの列
//   type=4のとき:
//     +0 { 8'b0, fps_limit[7:0], format[7:0], 5'b0, disable_audio[0:0],
//          disable_video[0:0], interlaced[0:0] }
//     +4 { 16'b0, cropx[15:0] }
//   type=6のとき:
//     +0 word_offset[7:0]
//     +1 8'b0
//     +2 word_value[31:0]
//     SPIで渡すレジスタ配列のword_offset位置へword_valueを書き込む。
//     レジスタ配列のフォーマット:
//     { mouse_dy[7:0], mouse_dx[7:0], mouse_seq[7:0], spi_index[7:0] }
//     { absmouse_x[15:0], mouse_wy[7:0], mouse_wx[7:0] }
//     { key_mod[7:0], absmbutton[0:0], mouse_button[7:0], absmouse_y[31:15] }
//     { key3[7:0], key2[7:0], key1[7:0], key0[7:0] }
//     { gcbutton[15:0], key5[7:0], key4[7:0] }
//     { gcaxes1[15:0], gcaxes0[15:0] }
//     { gcaxes3[15:0], gcaxes2[15:0] }
//     { gcaxes5[15:0], gcaxes4[15:0] }
//   type=7のとき:
//     ()
//   type=8のとき:
//     +0 ir_index[15:0]
//     +2 ir_cmd[15:0] ... 16bitのコマンドの列
//   type=9のとき:
//     +0 value[15:0]
//     +2 mask[15:0]

void
vcnet_conn::send_netframe(uint8_t typ, const void *data, uint8_t data_len,
  bool is_user_input)
{
  #if 0
  {
    unsigned long long v = 0;
    memcpy(&v, data, data_len < 8 ? data_len : 8);
    log(0, "send_netframe %llx\n", v);
  }
  #endif
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
vcnet_conn::send_raw_spi(const void *data, size_t data_len, bool has_event)
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
    if (d < 2000) {
      // 2秒未満なら正確に前回の1秒後に設定
      last_hb_time += std::chrono::milliseconds(1000);
    } else {
      // 2秒以上なら現時刻を設定。時計が大幅に進んだ場合の対策。
      last_hb_time = t;
    }
    // heartbeatメッセージをサーバに送る。ペイロードには時刻をセットする。
    // UDPではこれをRTT測定に使う。TDPでは使用していない。
    auto const t_hr = std::chrono::high_resolution_clock::now();
    uint64_t const now_us = (uint64_t)
      std::chrono::duration_cast<std::chrono::microseconds>(
        t_hr.time_since_epoch()).count();
    log(23, "ts=%llx\n", (unsigned long long)now_us);
    send_netframe(0, &now_us, sizeof(now_us), false); // heartbeat
    if (ir_sent < 5) {
      send_server_flags();
      send_ir();
      send_wol();
      ++ir_sent;
    }
    log(2, "%u fps %u hz %u\n", stat_videoframe_count, stat_audio_sample_count,
      (unsigned)d);
    fps = d < 2000 ? stat_videoframe_count : 0;
    stat_videoframe_count = 0;
    uint32_t fr = d < 2000 ? stat_audio_sample_count : 0;
    auto round_freq = [](uint32_t v) -> uint32_t {
      uint32_t const values[] = { 11025, 22050, 24000, 44100, 48000 };
      size_t const n = sizeof(values) / sizeof(values[0]);
      double const f = (double)v;
      uint32_t r = v;
      for (size_t i = 0; i < n; ++i) {
        double const rat = f / (double)values[i];
        if (rat > 0.98 && rat < 1.02) {
          r = values[i];
          break;
        }
      }
      return r;
    };
    fr = round_freq(fr);
    audio_freq_me.push_front(fr);
    audio_freq_me.resize(3);
    // log(0, "fr %u %u %u\n", fr, audio_freq_me[1], audio_freq_me[2]);
    if (audio_freq_me[1] == fr && audio_freq_me[2] == fr) {
      audio_freq = fr;
    }
    stat_audio_sample_count = 0;
    audiobuf_min = stat_audiobuf_min;
    audiobuf_max = stat_audiobuf_max;
    stat_audiobuf_min = (uint32_t)-1;
    stat_audiobuf_max = 0;
    drop_count = stat_drop_count;
    stat_drop_count = 0;
  }
}

void
vcnet_conn::send_system_reset()
{
  log(6, "sending system reset\n");
  send_netframe(2, nullptr, 0);
}

uint32_t
vcnet_conn::get_server_flags(size_t idx) const
{
  if (idx < server_flags.size()) {
    return server_flags[idx];
  }
  return 0;
}

uint32_t
vcnet_conn::get_server_flags_mask(size_t idx, uint32_t bit_offset,
  uint32_t nbits) const
{
  if (idx < server_flags.size()) {
    uint32_t flags = server_flags[idx];
    flags >>= bit_offset;
    flags &= (1 << nbits) - 1;
    return flags;
  }
  return 0;
}

void
vcnet_conn::set_server_flags(size_t idx, uint32_t v)
{
  if (idx < server_flags.size()) {
    log(19, "set_server_flags %zu %u\n", idx, (unsigned)v);
    server_flags[idx] = v;
  }
}

void
vcnet_conn::set_server_flags_mask(size_t idx, uint32_t bit_offset,
  uint32_t nbits, uint32_t v)
{
  if (idx < server_flags.size()) {
    log(19, "set_server_flags_mask %zu %u %u %u\n", idx, (unsigned)bit_offset,
      (unsigned)nbits, (unsigned)v);
    v &= (1 << nbits) - 1;
    v <<= bit_offset;
    uint32_t flags = server_flags[idx];
    uint32_t const nbits_mask = (1 << nbits) - 1;
    flags &= ~(nbits_mask << bit_offset);
    flags |= v;
    server_flags[idx] = flags;
    #if 0
    log(0, "set_server_flags %s %zu %x\n", cur.socktype_str.c_str(), idx,
      flags);
    #endif
  }
}

void
vcnet_conn::send_server_flags()
{
  char buf[sizeof(server_flags)];
  memcpy(buf, &server_flags[0], sizeof(server_flags));
  send_netframe(4, buf, sizeof(buf));
  #if 0
  log(0, "send_server_flags %s %x %x\n", cur.socktype_str.c_str(),
    (unsigned)server_flags[0], (unsigned)server_flags[1]);
  #endif
  log(19, "send_server_flags %x %x\n",
    (unsigned)server_flags[0], (unsigned)server_flags[1]);
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

void
vcnet_conn::send_ir()
{
  bool const old_format = (cur.ircmd.size() < 2);
    // 古いサーバとの互換性のため
  for (size_t i = 0; i < cur.ircmd.size(); ++i) {
    std::string tok;
    std::vector<uint16_t> vs;
    if (!old_format) {
      vs.push_back((uint16_t)i);
    }
    std::stringstream sstr(cur.ircmd[i]);
    while (std::getline(sstr, tok, ':')) {
      unsigned long v = strtoul(tok.c_str(), nullptr, 10);
      if (v != 0) {
        vs.push_back((uint16_t)v);
      }
    }
    if (vs.size() > 120) {
      log(0, "ir command too long\n");
    } else {
      send_netframe(old_format ? 3 : 8, vs.data(), (uint8_t)(vs.size() * 2),
        false);
      log(1, "sent ir[%zu] command %u\n", i, (unsigned)vs.size());
    }
  }
}

void
vcnet_conn::increment_videoframe_count()
{
  ++stat_videoframe_count;
  ++total_videoframe_count;
}

void
vcnet_conn::increment_audio_sample_count(uint32_t v)
{
  stat_audio_sample_count += v;
}

void
vcnet_conn::stat_audiobuf(uint32_t v)
{
  stat_audiobuf_min = std::min(v, stat_audiobuf_min);
  stat_audiobuf_max = std::max(v, stat_audiobuf_max);
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

// tag0の値:
//   0: server statistics (tag1とpayloadの両方にデータが入っている)
//     tag1 := server_stat
//     server_stat := { 8'b0, reboot_status[7:0], 12'b0, video_status[0:0],
//       video_locked[0:0], connected[0:0], svrtime_ms_and_32[0:0] }
//       reboot_statusはZynqのREBOOT_STATUSレジスタ(0xf8000258)の[23:16]の値
//         REBOOT_STATUS[22] Power-On Reset
//         REBOOT_STATUS[21] System Reset
//         REBOOT_STATUS[20] Debug Reset
//         REBOOT_STATUS[19] SLC soft Reset
//         REBOOT_STATUS[18] CPU 1 Watchdog Reset
//         REBOOT_STATUS[17] CPU 0 Watchdog Reset
//         REBOOT_STATUS[16] System Watchdog Reset
//     payload := { cnt_eq[15:0], cnt_ne[15:0] }
//   1: video frame
//     tag1 := { fr_last[0:0], fr_skipline[0:0], fr_fmt[1:0], fr_height[11:0],
//       fr_width[15:0] }
//     payload := video_frame_data
//   2: audio frame
//     tag1 := { 32'b0 }
//     payload := audio_frame_data
//
void
vcnet_conn::parse_netframe(uint8_t tag0, uint32_t tag1,
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
      if (datalen >= 4) {
        uint32_t hdmi_in_cnts = 0;
        memcpy(&hdmi_in_cnts, data, 4);
        uint32_t const cnt_eq = (hdmi_in_cnts >> 16) & 0xffffu;
        uint32_t const cnt_ne = (hdmi_in_cnts >>  0) & 0xffffu;
        if (video_count_seq < 5) {
          ++video_count_seq;
          video_count_eq_diff = 0;
          video_count_ne_diff = 0;
        } else {
          video_count_eq_diff = (cnt_eq - video_count_eq_saved) & 0xffffu;
          video_count_ne_diff = (cnt_ne - video_count_ne_saved) & 0xffffu;
        }
        video_count_eq_saved = cnt_eq;
        video_count_ne_saved = cnt_ne;
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
  // tag0 == 0x01, video frame
  uint32_t fr_width = tag1 & 0xffffu;
  uint32_t fr_height = (tag1 >> 16u) & 0x0fffu;
  uint32_t fr_last = (tag1 & 0x80000000u);
  uint32_t fr_skipline = ((tag1 & 0x40000000u) != 0) ? 1u : 0u;
  uint8_t fr_fmt = (tag1 &  0x30000000u) >> 28u;
    // 0: rgb888, 1: yuv411, 2: yuv422
    // svr_flagsに指定するformatとbit逆順なので注意。
  if (fr_height >= 8192 || fr_width >= 4096 /* ||
    fr_height == 0 || fr_width == 0 */) {
    log(0, "corrupted data tag0=%x tag1=%x w=%u h=%u\n", (unsigned)tag0,
      (unsigned)tag1, (unsigned)fr_width, (unsigned)fr_height);
    pix.set_wrpos(pix.begin());
    close_socket();
    return;
  }
  #if 0
  log(0, "fr w=%u h=%u\r\n", fr_width, fr_height);
  #endif
  if (fr_width != pix.get_width() || fr_height != pix.get_height() ||
    fr_fmt != pix.get_format()) {
    uint32_t const fdiff = total_videoframe_count
      - last_resize_videoframe_count;
    video_signal_stable = (fdiff >= 60 || pix.get_width() == 0);
      // 短い時間に2回解像度が変化したときはvideo_signal_stableをfalseにする
    last_resize_videoframe_count = total_videoframe_count;
    pix.set_format((vcnet_video_format_e)fr_fmt);
    pix.resize_if(fr_width, fr_height);
    log(1, "fdiff=%u stable=%d fr_fmt=%x %u %u\n", (unsigned)fdiff,
      (int)video_signal_stable, (unsigned)fr_fmt, (unsigned)fr_width,
      (unsigned)fr_height);
  } else if (!video_signal_stable) {
    uint32_t const fdiff = total_videoframe_count
      - last_resize_videoframe_count;
    if (fdiff >= 60) {
      video_signal_stable = true;
    }
  }
  uint32_t rboffset = 0;
  while (rboffset < datalen) {
    uint32_t const line_nbytes = fr_width * pix.get_bits_per_pixel() / 8;
    size_t dst_offset = line_nbytes * fr_skipline;
    size_t src_offset = rboffset;
    size_t cpy_size = line_nbytes;
    // FIXME? width == 0
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
// +0 tag0[7:0], frlen[23:0] (ヘッダ8bytesを含まないpayloadだけの長さ)
// +4 tag1[31:0]
// +8 payload...
// tag0の値:
//   0: server statistics (tag1とpayloadの両方にデータが入っている)
//   1: video frame
//   2: audio frame
// tag1 := { fr_last[0:0], fr_skipline[0:0], fr_fmt[1:0], fr_height[11:0],
//   fr_width[15:0] }
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
    uint8_t tag0 = tag0_frlen >> 24u;
    uint32_t fr_nbytes = tag0_frlen & 0x00ffffffu;
    if (rbpos + fr_nbytes + 8 > rbpos_end) {
      if (rbpos == &recv_buffer[0] &&
        recv_buffer_offset == sizeof(recv_buffer)) {
        log(0, "netframe too large frlen=%x tag0=%x tag1=%x\n",
          (unsigned)fr_nbytes, (unsigned)tag0, (unsigned)tag1);
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

// 受信UDPフレームのフォーマット
// (1024バイト未満のとき)
//   送信UDPフレームのフォーマットと同じ。送信されたUDPフレームはそのまま
//   エコーされて返ってくる。エコーされたもの以外のものもある。
// (1024バイト以上のとき、typeの値が0か1、vcnet_10gフォーマット)
//   (rgb_to_axis.v 参照)
//   +0 { type[7:0], i_odd_frame, i_interlaced, frame_count[5:0],
//        i_disp_height[15:0], line_count[15:0], framelen[15:0] }
//      framelenはヘッダ8byteを含む長さ (= i_disp_width[15:0] * 16'd3 + 16'd8)
//   +8 payload...
//     typeの値:
//       0: vcnet_10g video frame
//       1: vcnet_10g audio frame
// (1024バイト以上のとき、typeの値が2、vcnet_1gフォーマット)
//   +0 { type[7:0], 8'b0, fr_last[0:0], fr_skipline[0:0], fr_fmt[1:0],
//        fr_height[11:0], line_count[15:0], framelen[15:0] }
//      framelenはヘッダ8byteを含む長さ。
//   +8 payload...
//     typeの値:
//       2: vcnet_1g video frame
size_t
vcnet_conn::parse_recv_buffer_udp(vcnet_pixels& pix, bool& videoframe_done_r,
  std::vector<uint32_t>& audiobuf)
{
  size_t const len = recv_buffer_offset;
  unsigned char *rbpos = &recv_buffer[0];
  unsigned char *const rbpos_end = &recv_buffer[recv_buffer_offset];
  log(8, "parse_recv_buffer_udp (%p,%p)\n", rbpos, rbpos_end);
  if (len < 1024) {
    // 短いフレーム。送信したUDPフレームのエコーまたは応答。
    #if 0
    log(0, "parse_recv_buffer_udp short fmt %u\n", (unsigned)len); // FIXME
    #endif
    last_server_stat_time = std::chrono::system_clock::now();
    server_stat = 0x0c;
    unsigned long long hdr = 0;
    memcpy(&hdr, rbpos, len < 8 ? len : 8);
    uint8_t const typ = (uint8_t)(hdr >> 8);
    if (typ == 0 && len >= 10) {
      // send_netframeでheartbeat(type 0)をUDPで送ったエコー。ペイロードに
      // 送信時刻がセットされているので、これを使ってRTTを測る。RTTが大きいと
      // UDP受信バッファに溜まっている可能性が高いので、そのときは一時的に
      // vsync待ちを無効化するようにしている(suppress_vsync_wait)。
      uint64_t resp_ts = 0;
      memcpy(&resp_ts, rbpos + 2, 8);
      auto const t_hr = std::chrono::high_resolution_clock::now();
      uint64_t const now_us = (uint64_t)
        std::chrono::duration_cast<std::chrono::microseconds>(
          t_hr.time_since_epoch()).count();
      uint64_t const tdiff_us = now_us - resp_ts;
      log(23, "resp_ts=%llx now_us=%llx, tdiff_us=%llu\n",
        (unsigned long long)resp_ts,
        (unsigned long long)now_us,
        (unsigned long long)tdiff_us);
      net_roundtrip_time = tdiff_us;
    } else if (typ == 7) {
      uint32_t const cnt_eq = (hdr >> 40) & 0xffffffu;
      uint32_t const cnt_ne = (hdr >> 16) & 0xffffffu;
      if (video_count_seq < 5) {
        ++video_count_seq;
        video_count_eq_diff = 0;
        video_count_ne_diff = 0;
      } else {
        video_count_eq_diff = (cnt_eq - video_count_eq_saved) & 0xffffffu;
        video_count_ne_diff = (cnt_ne - video_count_ne_saved) & 0xffffffu;
      }
      video_count_eq_saved = cnt_eq;
      video_count_ne_saved = cnt_ne;
      #if 0
      log(0, "eq=%u ne=%u\n", (unsigned)video_count_eq_diff,
        (unsigned)video_count_ne_diff);
      log(0, "parse_recv_buffer_udp %llx, %zu\n", hdr, len);
      #endif
    }
    #if 0
    {
      unsigned long long hdr = 0;
      memcpy(&hdr, rbpos, len < 8 ? len : 8);
      if ((hdr & 0xff00) != 0x0600) {
        log(0, "parse_recv_buffer_udp %llx, %zu\n", hdr, len);
      }
    }
    #endif
    return len;
  }
  // long udp frame
  uint64_t const hdr = ((uint64_t const *)rbpos)[0];
  uint8_t const typ = hdr >> 56u;
  uint32_t const frame_len = hdr & 0xffffu;
  if (len != frame_len) {
    log(1, "parse_recv_buffer_udp invalid len v=%llx\n",
      (unsigned long long)hdr);
    return len;
  }
  #if 0
  log(0, "parse_recv_buffer_udp long fmt len=%u typ=%u\n", (unsigned)len,
    (unsigned)typ);
  #endif
  if (typ == 0 || typ == 2) {
    uint8_t tag0 = 0;
    uint32_t tag1 = 0;
    uint32_t interlaced = 0;
    uint32_t line_number = 0;
    uint8_t const fmt = pix.get_format();
    uint32_t const fr_width =
      (fmt == 0) ? (frame_len - 8) / 3
      : (fmt == 1) ? (frame_len - 8) * 2 / 3
      : (frame_len - 8) / 2;
    if (typ == 0) {
      /* typ==0はvcnet10gのudpビデオのフォーマット */
      // log(0, "parse_recv_buffer_udp v=%llx\n", (unsigned long long)hdr);
      uint32_t fr_height = (hdr >> 32u) & 0xffffu;
      line_number = (hdr >> 16u) & 0xffffu;
      #if 0
      log(0, "parse_recv_buffer_udp long fmt len=%u "
        "typ=%u frw=%u frh=%u ln=%u\n", (unsigned)len, (unsigned)typ,
        fr_width, fr_height, line_number);
      #endif
      if (line_number >= fr_height) {
        #if 0
        log(0, "parse_recv_buffer_udp line=%u\n", (unsigned)line_number);
        #endif
        return len;
      }
      uint32_t fr_last = (line_number == fr_height - 1) ? 0x80000000u : 0u;
      uint32_t odd_frame = (hdr >> 55) & 1; // unused
      if (interlaced) {
        fr_height *= 2;
        line_number = line_number * 2 + odd_frame;
      }
      server_flags[0] = (server_flags[0] & ~0x01) | interlaced;
      tag0 = 1u;
      tag1 = fr_width | (fr_height << 16) | fr_last;
      interlaced = (hdr >> 54) & 1;
    } else if (typ == 2) {
      /* typ==2はvcnet1gのudpビデオのフォーマット */
      line_number = (hdr >> 16u) & 0xffffu;
      tag0 = 1u;
      tag1 = ((hdr >> 16) & 0xffff0000) | (fr_width &  0x0000ffff);
      interlaced = 0;
      #if 0
      log(0, "type=2 fr tag1=%x\r\n", tag1);
      #endif
    }
    {
      unsigned char *const curpos = pix.wrpos();
      unsigned char *const pos = pix.begin() +
        line_number * pix.get_bits_per_pixel() * fr_width / 8;
      if (pos != curpos) {
        if (pos > curpos) {
          size_t diff = pos - curpos;
          // FIXME: vcnet1gのときはどうすればいいのか？
          if (!interlaced ||
            diff != fr_width * pix.get_bits_per_pixel() / 8) {
            #if 0
            log(2, "drop %zu lines\n",
              fr_width != 0
                ? diff / (fr_width / pix.get_bits_per_pixel() / 8) : 0);
            #endif
            ++stat_drop_count;
          }
        } else {
          log(2, "drop frame\n");
          ++stat_drop_count;
        }
      }
      pix.set_wrpos(pos);
    }
    // consumes fr_nbytes + 8 bytes
    #if 0
    log(0, "parse_recv_buffer_udp parse_netframe\r\n");
    #endif
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
  auto t = std::chrono::system_clock::now();
  #if 0
  static std::chrono::system_clock::time_point prev_t =
    std::chrono::system_clock::now();
  if (duration_ms(t, prev_t) > 5) {
    log(0, "recvdata duration %ld\n", duration_ms(t, prev_t));
  }
  prev_t = t;
  #endif
  if (sd == INVALID_SOCKET) {
    log(8, "recv_data: %ld %ld\n", duration_ms(t, last_open_time),
      duration_ms(t, last_user_input_time));
    if (duration_ms(t, last_open_time) > 3000 &&
      duration_ms(t, last_user_input_time) < 60000) {
      open_socket();
      send_server_flags();
        // サーバは接続が閉じられるとsvr_flags_tcp_disable_videoなどのフラグ
        // を既定の1にリセットするので、現在の設定を反映させるために
        // send_server_flagsを呼ぶ。
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
  short rev = POLLIN | POLLOUT;
  int timeout_ms = 0;
  if (use_poll) {
    rev = wait_events(timeout_ms);
  }
  size_t wcnt = 0;
  size_t rcnt = 0;
  if ((rev & POLLOUT) != 0)
  {
    wcnt = write_socket();
    if (wcnt > 0) {
      has_net_event_r = true;
    }
  }
  if ((rev & POLLIN) != 0) {
    if (cur.socktype == SOCK_STREAM) {
      while (true) {
        if (recv_buffer_offset < sizeof(recv_buffer)) {
          size_t rlen = read_socket(recv_buffer + recv_buffer_offset,
            sizeof(recv_buffer) - recv_buffer_offset);
          if (rlen == 0) {
            break;
          }
          ++rcnt;
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
        if (sd == INVALID_SOCKET) {
          break;
        }
      }
    } else if (cur.socktype == SOCK_DGRAM) {
      while (true) {
        size_t rlen = read_socket(recv_buffer, sizeof(recv_buffer));
        if (rlen == 0) {
          break;
        }
        ++rcnt;
        last_read_time = std::chrono::system_clock::now();
        recv_buffer_offset = rlen;
        has_net_event_r = true;
        parse_recv_buffer_udp(pix, videoframe_done_r, audiobuf);
        recv_buffer_offset = 0;
        if (videoframe_done_r) {
          break;
        }
        if (sd == INVALID_SOCKET) {
          break;
        }
      }
    }
  }
  #if 0
  if (!use_poll && wcnt == 0 && rcnt == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
  }
  #endif
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

void
vcnet_conn::set_gpio_out(bool v)
{
  if (cur.gpio_index < 0) {
    return;
  }
  uint16_t val = v;
  val <<= cur.gpio_index;
  uint16_t mask = 1 << cur.gpio_index;
  uint16_t buf[2] = { val, mask };
  send_netframe(9, buf, sizeof(buf));
}

//////////////////////////////////////////////////////////////

struct vcnet_conn_delegate {
private:
  std::vector<std::shared_ptr<vcnet_conn>> conns;
  vcnet_pixels dummy_pix;
  uint32_t svridx_spi = 0;
  uint32_t svridx_video = 0;
  uint32_t svridx_audio = 0;
  std::chrono::system_clock::time_point start_time { };
private:
  void update_server_index() {
    auto const iter_spi = std::find_if(conns.begin(), conns.end(),
      [](std::shared_ptr<vcnet_conn> const& conn) {
        return conn->get_spi_index() >= 0;
      }
    );
    auto const iter_video = std::find_if(conns.begin(), conns.end(),
      [](std::shared_ptr<vcnet_conn> const& conn) {
        return conn->get_enable_video();
      }
    );
    auto const iter_audio = std::find_if(conns.begin(), conns.end(),
      [](std::shared_ptr<vcnet_conn> const& conn) {
        return conn->get_enable_audio();
      }
    );
    svridx_spi = static_cast<uint32_t>(
      (iter_spi != conns.end()) ? (iter_spi - conns.begin()) : 0);
    svridx_video = static_cast<uint32_t>(
      (iter_video != conns.end()) ? (iter_video - conns.begin()) : 0);
    svridx_audio = static_cast<uint32_t>(
      (iter_audio != conns.end()) ? (iter_audio - conns.begin()) : 0);
    log(0, "idxspi=%u idxvid=%u idxaud=%u\n", (unsigned)svridx_spi,
      (unsigned)svridx_video, (unsigned)svridx_audio);
    // svridx_videoとsvridx_audio以外のserverについてはvideo, audioを
    // 送信しないようにserver_flagsをセットする。
    for (size_t i = 0; i < conns.size(); ++i) {
      uint32_t const disable_v =
        (i == svridx_video && conns[i]->get_enable_video()) ? 0 : 1;
      uint32_t const disable_a =
        (i == svridx_audio && conns[i]->get_enable_audio()) ? 0 : 1;
      conns[i]->set_server_flags_mask(0, 1, 1, disable_v);
      conns[i]->set_server_flags_mask(0, 2, 1, disable_a);
      log(0, "svridx_s=%u svridx_v=%u svridx_a=%u disable_v=%u disable_a=%u\n",
        (unsigned)svridx_spi, (unsigned)svridx_video, (unsigned)svridx_audio,
        (unsigned)disable_v, (unsigned)disable_a);
    }
  }
public:
  vcnet_conn_delegate(vcnet_config const& conf) {
    start_time = std::chrono::system_clock::now();
    uint32_t server_index = 0;
    uint32_t num_devices = 0;
    while (true) {
      bool found_conf = false;
      auto conn = std::make_shared<vcnet_conn>(server_index, num_devices,
        conf, found_conf);
      if (!found_conf && server_index > 0) {
        break;
      }
      if (server_index == 0) {
        num_devices = conn->get_num_devices();
      }
      conns.push_back(conn);
      ++server_index;
    }
    assert(conns.size() >= 1);
    update_server_index();
  }
  vcnet_conn_delegate(vcnet_conn_delegate const&) = delete;
  vcnet_conn_delegate& operator =(vcnet_conn_delegate const&) = delete;
  uint32_t get_num_concurrent_servers() const {
    return static_cast<uint32_t>(conns.size());
  }
  uint32_t get_svridx_spi() const { return svridx_spi; }
  uint32_t get_svridx_video() const { return svridx_video; }
  uint32_t get_svridx_audio() const { return svridx_audio; }
  uint32_t get_num_devices() const { return conns[0]->get_num_devices(); }
  uint32_t get_cur_device() const { return conns[0]->get_cur_device(); }
  bool set_cur_device(uint32_t idx) {
    bool r = false;
    for (size_t i = 0; i < conns.size(); ++i) {
      bool v = conns[i]->set_cur_device(idx);
      if (i == 0) {
        r = v;
      }
    }
    update_server_index();
    return r;
  }
  std::string get_title() const { return conns[0]->get_title(); }
  bool get_device_info(uint32_t svridx, uint32_t devidx,
    vcnet_conn::device_info& d_r) const {
    if (svridx >= get_num_concurrent_servers()) {
      return false;
    }
    return conns[svridx]->get_device_info(devidx, d_r);
  }
  int get_spi_index() const {
    return conns[svridx_spi]->get_spi_index();
  }
  bool get_absmouse() const {
    return conns[svridx_spi]->get_absmouse();
  }
  void set_absmouse(bool v) {
    conns[svridx_spi]->set_absmouse(v);
  }
  bool get_absmousebutton() const {
    return conns[svridx_spi]->get_absmousebutton();
  }
  bool get_wheel_dir() const {
    return conns[svridx_spi]->get_wheel_dir();
  }
  uint32_t get_cropx_val() const {
    return conns[svridx_video]->get_cropx_val();
  }
  void send_wol() {
    for (auto const& conn: conns) {
      conn->send_wol();
    }
  }
  void send_netframe(uint8_t typ, const void *data, uint8_t data_len,
    bool is_user_input = true) {
    for (auto const& conn: conns) {
      conn->send_netframe(typ, data, data_len, is_user_input);
    }
  }
  void send_raw_spi(const void *data, size_t data_len, bool has_event) {
    conns[svridx_spi]->send_raw_spi(data, data_len, has_event);
  }
  void send_heartbeat_if() {
    for (auto const& conn: conns) {
      conn->send_heartbeat_if();
    }
  }
  void send_ir() {
    for (auto const& conn: conns) {
      conn->send_ir();
    }
  }
  void send_system_reset() {
    for (auto const& conn: conns) {
      conn->send_system_reset();
    }
  }
  void send_server_flags() {
    for (auto const& conn: conns) {
      conn->send_server_flags();
    }
  }
  void send_adv7611_i2c(uint8_t i2c_addr7, uint8_t reg, uint8_t val) {
    for (auto const& conn: conns) {
      conn->send_adv7611_i2c(i2c_addr7, reg, val);
    }
  }
  uint32_t get_server_flags(size_t idx) const {
    return conns[0]->get_server_flags(idx);
  }
  uint32_t get_server_flags_mask(size_t idx, uint32_t bit_offset,
    uint32_t nbits) const {
    return conns[0]->get_server_flags_mask(idx, bit_offset, nbits);
  }
  void set_server_flags(size_t idx, uint32_t v) {
    for (auto const& conn: conns) {
      conn->set_server_flags(idx, v);
    }
  }
  void set_server_flags_mask(size_t idx, uint32_t bit_offset, uint32_t nbits,
    uint32_t v) {
    for (auto const& conn: conns) {
      conn->set_server_flags_mask(idx, bit_offset, nbits, v);
    }
  }
  void recv_data(vcnet_pixels& pix, bool& videoframe_done_r, 
    bool& has_net_event_r, std::vector<uint32_t>& audiobuf) {
    videoframe_done_r = false;
    has_net_event_r = false;
    for (size_t i = 0; i < conns.size(); ++i) {
      auto& conn = conns[i];
      bool vdr = false;
      bool hner = false;
      auto audiobuf_size_saved = audiobuf.size();
      conn->recv_data((i == svridx_video) ? pix : dummy_pix, vdr, hner,
        audiobuf);
      if (i == svridx_video) {
        videoframe_done_r = vdr;
      }
      has_net_event_r |= hner;
      if (i != svridx_audio) {
        audiobuf.resize(audiobuf_size_saved);
      }
    }
  }
  void increment_videoframe_count() {
    conns[svridx_video]->increment_videoframe_count();
  }
  void increment_audio_sample_count(uint32_t v) {
    conns[svridx_audio]->increment_audio_sample_count(v);
  }
  void stat_audiobuf(uint32_t v) {
    conns[svridx_audio]->stat_audiobuf(v);
  }
  uint32_t get_server_stat() const {
    return conns[svridx_video]->get_server_stat();
  }
  uint32_t get_fps() const { return conns[svridx_video]->get_fps(); }
  uint32_t get_audio_freq() const {
    return conns[svridx_audio]->get_audio_freq();
  }
  uint32_t get_audiobuf_min() const {
    return conns[svridx_audio]->get_audiobuf_min();
  }
  uint32_t get_audiobuf_max() const {
    return conns[svridx_audio]->get_audiobuf_max();
  }
  uint32_t get_drop_count() const {
    return conns[svridx_video]->get_drop_count();
  }
  bool get_video_signal_stable() const {
    return conns[svridx_video]->get_video_signal_stable();
  }
  std::chrono::system_clock::time_point get_last_open_time(uint32_t svridx)
    const {
    if (svridx >= get_num_concurrent_servers()) {
      return start_time;
    }
    return conns[svridx]->get_last_open_time();
  };
  std::chrono::system_clock::time_point get_last_read_time(uint32_t svridx)
    const {
    if (svridx >= get_num_concurrent_servers()) {
      return start_time;
    }
    return conns[svridx]->get_last_read_time();
  };
  std::pair<uint32_t, uint32_t> get_video_count_diff() const {
    return conns[svridx_video]->get_video_count_diff();
  }
  uint64_t get_net_roundtrip_time() const {
    return conns[svridx_video]->get_net_roundtrip_time();
  }
  void set_gpio_out(bool v) {
    for (auto const& conn: conns) {
      conn->set_gpio_out(v);
    }
  }
};

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
  bool is_complex_conv = false;
  uint16_t gcbutton = 0;
  std::array<int16_t, 6> gcaxes = { };
  void push(std::vector<uint8_t>& buf_a, uint8_t word_offset, uint8_t v0,
    uint8_t v1, uint8_t v2, uint8_t v3);
public:
  void mouse_button(uint8_t v, bool press);
  void mouse_move(int x, int y, int wx, int wy);
  void absmouse_button(uint8_t v, bool press);
  void absmouse_move(uint16_t x, uint16_t y);
  void key_down(uint8_t k, uint8_t m, bool is_complex);
  void key_up(uint8_t k, uint8_t m);
  void key_up_all();
  void gc_buttons(uint16_t v);
  void gc_axis(uint8_t axis, int16_t v);
  void get_spi_buffer(int spi_index, std::vector<uint8_t>& buf_a,
    bool& has_event_r);
  void up_all();
  std::string get_state_str() const;
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
vcnet_hid::key_down(uint8_t k, uint8_t m, bool is_complex)
{
  keys.insert(k);
  key_mod = m;
  is_complex_conv = is_complex;
    // これがtrueのとき、このキーはmodifierを含んだ変換の結果なので、今後
    // key_upの際は全てのkeyをupする。
  has_event = true;
  log(10, "key_down %u %u %u\n", (unsigned)k, (unsigned)m,
    (unsigned)is_complex);
}

void
vcnet_hid::key_up(uint8_t k, uint8_t m)
{
  if (is_complex_conv) {
    keys.clear();
  } else {
    keys.erase(k);
  }
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
  log(10, "up_all\n");
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
  buf_a.push_back(0x06); // tag=6
  buf_a.push_back(0x06); // data_length=6
  buf_a.push_back(word_offset); // SPIバッファ中オフセット、4byte単位
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
vcnet_hid::get_spi_buffer(int spi_index, std::vector<uint8_t>& buf_a,
  bool& has_event_r)
{
  if (spi_index < 0 || spi_index > 255) {
    return;
  }
  auto const clamp_int8 = [](int v) -> int {
    return (v <= -128) ? -128 : (v >= 127) ? 127 : v;
  };
  if (mouse_moved) {
    ++mouse_seq;
    mouse_moved = false;
  }
  push(buf_a, 0, (uint8_t)spi_index, mouse_seq,
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

std::string
vcnet_hid::get_state_str() const
{
  std::string r;
  if ((key_mod & 0x11) != 0) {
    r += "C";
  }
  if ((key_mod & 0x22) != 0) {
    r += "S";
  }
  if ((key_mod & 0x44) != 0) {
    r += "A";
  }
  if ((key_mod & 0x88) != 0) {
    r += "G";
  }
  for (auto j = keys.begin(); j != keys.end(); ++j) {
    r += to_hexstr(*j, 2);
  }
  return r;
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
  auto joy_get_int = [&](std::pair<joy_entry, int> const& ep) -> int32_t {
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
        val += joy_get_int(e);
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

struct vcnet_audio {
private:
  SDL_AudioDeviceID audiodev = 0;
  #ifdef __ANDROID__
  enum { DEFAULT_BUF_SIZE_LOG2 = 17 };
  #else
  enum { DEFAULT_BUF_SIZE_LOG2 = 15 };
  #endif
  std::vector<unsigned char> buffer { };
  static void audio_cb(void *userdata, Uint8 *stream, int len);
  std::atomic_uint offset_read;
  std::atomic_uint offset_write;
  uint32_t freq = 48000;
  uint32_t channels = 2;
  bool use_audio_callback = false;
  bool dump_audio = false;
  unsigned buf_size_log2 = DEFAULT_BUF_SIZE_LOG2;
  unsigned buf_size = 0; // (1 << bus_size_log2)
  unsigned buf_size_mask = 0; // (1 << bus_size_log2) - 1
  unsigned audio_device_samples = 12;
public:
  vcnet_audio(vcnet_config const& conf0);
  vcnet_audio(vcnet_audio const&) = delete;
  vcnet_audio& operator =(vcnet_audio const&) = delete;
  ~vcnet_audio();
  void open_device_if(uint32_t audio_freq, uint32_t audio_channels);
  void close_device();
  size_t queue_audio(unsigned char const *audiobuf, size_t audiobuf_size);
  uint32_t get_freq() const;
  unsigned get_buffered_size() const;
};

vcnet_audio::vcnet_audio(vcnet_config const& conf0)
{
  use_audio_callback = (bool)conf0.get_uint("use_audio_callback", 0);
  dump_audio = (bool)conf0.get_uint("dump_audio", 0);
  buf_size_log2 = (unsigned)conf0.get_uint("audio_buffer",
    DEFAULT_BUF_SIZE_LOG2);
  audio_device_samples = (unsigned)conf0.get_uint("audio_device_samples",
    12);
  log(1, "audio_buffer(log2)=%u audio_device_samples(log2)=%u\n",
    buf_size_log2, audio_device_samples);
  buf_size = (1 << buf_size_log2);
  buf_size_mask = (1 << buf_size_log2);
  buffer.resize(buf_size);
}

void
vcnet_audio::open_device_if(uint32_t audio_freq, uint32_t audio_channels)
{
  if (freq == audio_freq && channels == audio_channels && audiodev != 0) {
    return;
  }
  close_device();
  freq = audio_freq;
  channels = audio_channels;
  SDL_AudioSpec as_req { };
  SDL_AudioSpec as_got { };
  as_req.freq = freq;
  as_req.format = AUDIO_S16SYS;
  as_req.channels = channels;
  as_req.samples = (1 << audio_device_samples);
    // 2048以下にするとゴミが入る。要調査。
  if (use_audio_callback) {
    as_req.callback = audio_cb;
  }
  as_req.userdata = this;
  audiodev = SDL_OpenAudioDevice(nullptr, 0, &as_req, &as_got, 0);
  log(1, "audio device %u freq=%u ch=%u s=%u buf=%u cb=%u\n",
    (unsigned)audiodev, (unsigned)audio_freq, (unsigned)audio_channels,
    audio_device_samples, buf_size_log2, (unsigned)use_audio_callback);
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
dump_raw_audio(void const *data, size_t len)
{
  // 16bit signed PCM, little-endian, stereo
  if (raw_audio == nullptr) {
    raw_audio = fopen("rawaudio.raw", "wb");
  }
  if (raw_audio != nullptr) {
    fwrite(data, len, 1, raw_audio);
  }
}

unsigned
vcnet_audio::get_buffered_size() const
{
  if (!use_audio_callback) {
    Uint32 sz = SDL_GetQueuedAudioSize(audiodev);
    return (unsigned)sz;
  } else {
    unsigned const ord = offset_read.load();
    unsigned const owr = offset_write.load();
    unsigned const bufsz = (owr - ord) % buf_size_mask;
    return bufsz;
  }
}

void
vcnet_audio::audio_cb(void *userdata, Uint8 *stream, int len)
{
  static bool cb_init = false;
  if (!cb_init) {
    #ifdef _MSC_VER
    log(0, "audio thread: %llx\n", (unsigned long long)GetCurrentThread());
    #endif
    cb_init = true;
  }
  vcnet_audio *self = (vcnet_audio *)userdata;
  unsigned const ulen = len;
  unsigned const ord = self->offset_read.load();
  unsigned const owr = self->offset_write.load();
  unsigned const bufsz = (owr - ord) % self->buf_size_mask;
  log(13, "audio_cb bufsz=%u len=%u\n", bufsz, ulen);
  if (bufsz > ulen) {
    unsigned const ord_end = (ord + ulen) & self->buf_size_mask;
    if (ord_end > ord) {
      memcpy(stream, &self->buffer[ord], ulen);
    } else {
      memcpy(stream, &self->buffer[ord], (self->buf_size - ord));
      memcpy(stream + (self->buf_size - ord), &self->buffer[0], ord_end);
    }
    self->offset_read.store(ord_end);
  } else {
    memset(stream, 0, ulen);
    log(3, "audio_cb: no data\n");
  }
}

size_t
vcnet_audio::queue_audio(unsigned char const *audiobuf, size_t audiobuf_nbytes)
{
  size_t r = 0;
  if (audiodev != 0 && audiobuf_nbytes != 0) {
    if (dump_audio) {
      dump_raw_audio(audiobuf, audiobuf_nbytes);
    }
    if (!use_audio_callback) {
      Uint32 sz = SDL_GetQueuedAudioSize(audiodev);
      if (sz > buf_size) {
        log(15, "drop: queued audio size %u %zu\n", (unsigned)sz,
          audiobuf_nbytes);
      } else {
        SDL_QueueAudio(audiodev, audiobuf,
          static_cast<Uint32>(audiobuf_nbytes));
        log(15, "queue audio\n");
      }
      r = sz;
    } else {
      unsigned owr = offset_write.load();
      unsigned ord = offset_read.load();
      unsigned space = (ord == owr) ? buf_size : (ord - owr) & buf_size_mask;
      log(15, "queue space=%u\n", space);
      if (space > audiobuf_nbytes + 1) {
        unsigned owr_end = (owr + audiobuf_nbytes) & buf_size_mask;
        if (owr_end > owr) {
          memcpy(&buffer[owr], audiobuf, audiobuf_nbytes);
        } else {
          memcpy(&buffer[owr], audiobuf, buf_size - owr);
          memcpy(&buffer[0], audiobuf + (buf_size - owr), owr_end);
        }
        offset_write.store(owr_end);
      } else {
        log(1, "queue_audio: no space\n");
      }
      r = (buf_size - space);
    }
  }
  return r;
}

uint32_t
vcnet_audio::get_freq() const
{
  return freq;
}

//////////////////////////////////////////////////////////////

#define CHECK_GL(expr) \
  (expr); \
  if (get_logmask(2)) { \
    GLenum e = glGetError(); \
    if (e != 0) { log(2, #expr ": %x\n", (unsigned)e); } \
  }

#define CHECK_SDL(expr) \
  { \
    int st = (expr); \
    if (st != 0) { \
      log(1, #expr ": %s\n", SDL_GetError()); \
    } \
  }

static void wrap_glDeleteBuffer(GLuint b)
{
  CHECK_GL(glDeleteBuffers(1, &b));
}

static void wrap_glDeleteTexture(GLuint t)
{
  CHECK_GL(glDeleteTextures(1, &t));
}

#ifdef VCNET_OPENGL3
static void wrap_glDeleteVertexArray(GLuint a)
{
  CHECK_GL(glDeleteVertexArrays(1, &a));
}
#endif

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

#if 0
LRESULT CALLBACK wndproc_player_window(HWND hwnd, UINT message, WPARAM wp,
  LPARAM lp)
{
  return DefWindowProc(hwnd, message, wp, lp);
}

HWND create_player_window(HINSTANCE hinst, HWND parent, unsigned width,
  unsigned height)
{
  static const PCWSTR klass_name = L"vcnet_mfplayer";
  WNDCLASSEX wcex = { };
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = wndproc_player_window;
  wcex.hInstance = hinst;
  wcex.lpszClassName = klass_name;
  throw_failure(RegisterClassEx(&wcex));
  HWND hwnd = CreateWindow(klass_name, L"", WS_CHILD, 0, 0, width, height,
    parent, nullptr, hinst, nullptr);
  return hwnd;
}
#endif

struct vcnet_window {
private:
  vcnet_config conf;
  unsigned window_width = 640;
  unsigned window_height = 480;
  unsigned texwidth = 0;
  unsigned texheight = 0;
  bool use_gl = true;
  bool real_fullscreen = false;
  unsigned vsync_mode = 0;
  unsigned vsync_mode_cur = 0;
  bool y_interpolation = true;
  auto_res<SDL_Window *> window;
  auto_res<SDL_GLContext> glcontext;
  auto_res<SDL_Renderer *> rend;
  auto_res<SDL_Texture *> sdltex;
  auto_res<SDL_Texture *> texttex;
  auto_res<GLuint> prog_video;
  auto_res<GLuint> prog_cursor;
  auto_res<GLuint> prog_lagtest;
  GLint attr_coord = -1;
  GLint attr_cursor_coord = -1;
  GLint attr_lagtest_coord = -1;
  #ifdef VCNET_OPENGL3
  auto_res<GLuint> vao;
  #endif
  auto_res<GLuint> vbo_coord;
  auto_res<GLuint> tex_video;
  GLint loc_tex_video = -1;
  GLint loc_video_size = -1;
  GLint loc_bilinear = -1;
  GLint loc_video_format = -1;
  GLint loc_y_interpolation = -1;
  GLint loc_scale = -1;
  GLint loc_cursor_pos = -1;
  GLint loc_cursor_scale = -1;
  GLint loc_lagtest_color = -1;
  Uint32 fullscreen = 0;
  std::string message;
  SDL_Rect cur_draw_rect = { };
  std::shared_ptr<std::string> ttf_image;
  bool suppress_vsync_wait_saved = false;
  vcnet_video_format_e cur_video_format = { };
  bool show_mf_window = false;
  bool video_use_sample_reader = false;
  bool audio_use_sample_reader = false;
  uint32_t mfplayer_audio_sps = 0;
  uint32_t mfplayer_audio_channels = 0;
  #ifdef _MSC_VER
  typedef mediafoundation_player::media_attribute media_attribute;
  std::vector<media_attribute> video_devices;
  std::vector<media_attribute> audio_devices;
  media_attribute video_filter;
  media_attribute video_attr;
  media_attribute audio_filter;
  media_attribute audio_attr;
  com_ptr<mediafoundation_player> mfplayer_video;
  com_ptr<mediafoundation_player> mfplayer_audio;
  vcnet_video_format_e capt_video_format = vcnet_video_format_e_rgba8888;
  com_ptr<IMFMediaBuffer> cur_video_sample;
  #else
  void *const mfplayer_video = nullptr;
  void *const mfplayer_audio = nullptr;
  #endif
public:
  vcnet_window(vcnet_config const& conf0);
  vcnet_window(vcnet_window const&) = delete;
  ~vcnet_window();
  vcnet_window& operator =(vcnet_window const&) = delete;
  void update_texture_sdl(vcnet_pixels const& pix);
  void resize_texture_if(unsigned w, unsigned h);
  void resize_window_if();
  void recalc_draw_rect(unsigned texwidth, unsigned texheight);
  void draw_window_sdl(vcnet_pixels const& pix, bool video_stable);
  void draw_window_gl(vcnet_pixels const& pix, bool video_stable,
    bool imgui_draw, bool cursor_draw, bool suppress_vsync_wait);
  void draw_window_lagtest(uint32_t mode, bool color1, bool show_gui);
  bool capture_video_read_sample();
  uint32_t queue_audio_sample(vcnet_audio& aud);
public:
  bool get_real_fullscreen() const;
  void set_real_fullscreen(bool v);
  bool get_fullscreen() const;
  void set_fullscreen(bool v);
  void disable_ime();
  unsigned get_width() const { return window_width; }
  unsigned get_height() const { return window_height; }
  unsigned get_texwidth() const { return texwidth; }
  unsigned get_texheight() const { return texheight; }
  SDL_Rect get_cur_draw_rect() const { return cur_draw_rect; }
  bool get_use_gl() const { return use_gl; }
  void set_title(std::string const& s);
  void warp_mouse(int x, int y);
  bool is_screen_keyboard_shown();
  void set_vsync_mode(unsigned v);
  unsigned get_vsync_mode() const { return vsync_mode; }
  void set_y_interpolation(bool v) { y_interpolation = v; }
  bool get_y_interpolation() const { return y_interpolation; }
  bool get_suppress_vsync_wait_saved() const {
    return suppress_vsync_wait_saved;
  }
  void set_show_mf_window(bool v);
  bool has_video_capture() const { return mfplayer_video != nullptr; }
  bool has_audio_capture() const { return mfplayer_audio != nullptr; }
  bool has_mf_window() const {
    return mfplayer_video != nullptr && !video_use_sample_reader;
  }
  bool get_video_use_sample_reader() const { return video_use_sample_reader; }
  bool get_audio_use_sample_reader() const { return audio_use_sample_reader; }
  vcnet_video_format_e get_cur_video_format() const {
    return cur_video_format;
  }
  uint32_t get_audio_sps() const { return mfplayer_audio_sps; }
  uint32_t get_audio_channels() const { return mfplayer_audio_channels; }
  #ifdef _MSC_VER
  std::vector<media_attribute> get_video_devices() const {
    return video_devices;
  }
  std::vector<media_attribute> get_audio_devices() const {
    return audio_devices;
  }
  media_attribute get_video_attr() const { return video_attr; }
  media_attribute get_audio_attr() const { return audio_attr; }
  void set_capt_video_filter(media_attribute const& vf) {
    video_filter = vf;
  }
  void set_capt_audio_filter(media_attribute const& af) {
    audio_filter = af;
  }
  void open_capture_device();
  #endif
private:
  void compile_shader(const char *vssrc, const char *fssrc,
    auto_res<GLuint>& prog);
};

void
vcnet_window::set_vsync_mode(unsigned v)
{
  if (v == 0) {
    log(0, "novsync\n");
    SDL_GL_SetSwapInterval(0);
    vsync_mode = 0;
  } else if (v == 1) {
    log(0, "vsync\n");
    SDL_GL_SetSwapInterval(1);
    vsync_mode = 1;
  } else if (v == 2) {
    if (SDL_GL_SetSwapInterval(-1) < 0) {
      log(0, "vsync\n");
      SDL_GL_SetSwapInterval(1);
      vsync_mode = 1;
    } else {
      log(0, "adaptive vsync\n");
      vsync_mode = 2;
    }
  }
  vsync_mode_cur = vsync_mode;
  suppress_vsync_wait_saved = false;
}

#ifdef _MSC_VER
void
vcnet_window::open_capture_device()
{
  SDL_SysWMinfo wminfo = { };
  SDL_VERSION(&wminfo.version);
  SDL_GetWindowWMInfo(window.get(), &wminfo);
  HINSTANCE hinst = wminfo.info.win.hinstance;
  HWND hwnd = wminfo.info.win.window;
    // media foundationのプレビュー表示に使用するウインドウ。sample reader
    // を使うときはこのウインドウは使わない。
  log(0, "HWND %p\n", hwnd);
  mfplayer_video = mediafoundation_player::create(hwnd);
  mfplayer_audio = mediafoundation_player::create(hwnd);
  // キャプチャデバイスを一覧しvideo_devices, audio_devicesにセット
  bool enumerate_capt =
    (conf.get_uint("enumerate_capture_devices", 1) != 0);
  if (enumerate_capt) {
    media_attribute filter_attr;
    filter_attr.majortype = "vids";
    auto const devices_v = mediafoundation_player::enumerate_capture_devices(
      filter_attr);
    filter_attr.majortype = "auds";
    auto const devices_a = mediafoundation_player::enumerate_capture_devices(
      filter_attr);
    log(0, "enumerate capture devices:\n");
    for (auto const d: devices_v) {
      log(0, "  video '%s' fmt='%s' %ux%u %u/%u\n", d.name.c_str(),
        d.subtype.c_str(), d.width, d.height, d.fps_numerator,
        d.fps_denominator);
    }
    for (auto const d: devices_a) {
      log(0, "  audio '%s' fmt='%s'\n", d.name.c_str(), d.subtype.c_str());
    }
    video_devices = devices_v;
    audio_devices = devices_a;
  }
  try {
    video_attr = mfplayer_video->open_capture_device(video_filter,
      video_use_sample_reader);
    if (video_attr.subtype == "YUY2") {
      capt_video_format = vcnet_video_format_e_yuy2;
    } else if (video_attr.subtype == "NV12") {
      capt_video_format = vcnet_video_format_e_nv12;
    } else if (video_attr.subtype == "16000000") {
      capt_video_format = vcnet_video_format_e_rgba8888;
    }
  } catch (hresult_exception const& hr) {
    log(0, "open_capture_device(video): %x\n", (unsigned)hr.get());
  }
  log(0, "open device:\n");
  {
    auto const& d = video_attr;
    log(0, "  video '%s' fmt='%s' %ux%u %u/%u\n", d.name.c_str(),
      d.subtype.c_str(), d.width, d.height, d.fps_numerator,
      d.fps_denominator);
  }
  try {
    audio_attr = mfplayer_audio->open_capture_device(audio_filter,
      audio_use_sample_reader);
  } catch (hresult_exception const& hr) {
    log(0, "open_capture_device(audio): %x\n", (unsigned)hr.get());
  }
  {
    auto const& d = audio_attr;
    log(0, "  audio '%s' fmt='%s'\n", d.name.c_str(), d.subtype.c_str());
  }
  log(0, "capture_audio_use_sample_reader %d\n", audio_use_sample_reader);
  set_show_mf_window(!video_use_sample_reader);
}
#endif

vcnet_window::vcnet_window(vcnet_config const& conf0)
  : conf(conf0)
{
  use_gl = (conf.get_uint("use_gl", 1) != 0);
    // 通常はuse_gl=1にする。use_gl=0はデバッグ用途でのみ使用する。
  log(0, "use_gl=%d\n", (int)use_gl);
  {
    #ifdef VCNET_GLES
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
      SDL_GL_CONTEXT_PROFILE_ES));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
    #else
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0));
    #if 0
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
      SDL_GL_CONTEXT_PROFILE_CORE));
    #endif
    #endif
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0));
  }
  real_fullscreen = (conf.get_uint("real_fullscreen", 0) != 0);
  fullscreen = 0;
  bool const fsc = (conf.get_uint("fullscreen", 0) != 0);
  set_fullscreen(fsc);
  int const w = (int)conf.get_uint("width", 1280);
  int const h = (int)conf.get_uint("height", 720);
  Uint32 flags = fullscreen;
  flags |= SDL_WINDOW_RESIZABLE;
  flags |= SDL_WINDOW_OPENGL;
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
  if (conf.get_uint("minimized", 0) != 0) {
    flags |= SDL_WINDOW_MINIMIZED;
  }
  window.reset(
    SDL_CreateWindow("vcnet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      w, h, flags),
      SDL_DestroyWindow);
  window_width = w;
  window_height = h;
  auto const capture_v = conf.get_str("capture_video", "");
  auto const capture_a = conf.get_str("capture_audio", "");
  if (!capture_v.empty() || !capture_a.empty()) {
    // UVC/UAC などのキャプチャデバイスを開く。Windowsのみ。
    #ifdef _MSC_VER
    auto const capture_a_fmt = conf.get_str("capture_audio_format", "");
    auto const capture_v_fmt = conf.get_str("capture_video_format", "");
    auto const capture_v_fps_num = conf.get_uint(
      "capture_video_fps_numerator", 0);
    auto const capture_v_fps_den = conf.get_uint(
      "capture_video_fps_denominator", 0);
    auto const capture_v_width = conf.get_uint(
      "capture_video_width", 0);
    auto const capture_v_height = conf.get_uint(
      "capture_video_height", 0);
    // キャプチャデバイスを開きプレイヤーに接続する。
    video_use_sample_reader =
      (conf.get_uint("capture_video_use_sample_reader", 1) != 0);
      // 有効のとき、ビデオをmedia sessionで勝手に再生するのではなく、
      // sample readerを使ってサンプルを取得しOpenGLで描画する。
      // 前者はフレーム飛びが酷いので後者を既定にする。
    audio_use_sample_reader =
      (conf.get_uint("capture_audio_use_sample_reader", 1) != 0);
      // 有効のとき、音声をmedia sessionで勝手に再生するのではなく、
      // sample readerを使ってサンプルを取得しSDL2で再生する。
      // 後者のほうが遅延が小さいので既定では後者。
    auto& va = video_filter; // 開くvideoデバイスが満たすべき条件
    va = { };
    va.name = capture_v;
    va.majortype = "vids";
    va.subtype = capture_v_fmt;
    va.fps_numerator = static_cast<uint32_t>(capture_v_fps_num);
    va.fps_denominator = static_cast<uint32_t>(capture_v_fps_den);
    va.width = static_cast<uint32_t>(capture_v_width);
    va.height = static_cast<uint32_t>(capture_v_height);
    auto& aa = audio_filter; // 開くaudioデバイスが満たすべき条件
    aa = { };
    aa.name = capture_a;
    aa.majortype = "auds";
    aa.subtype = capture_a_fmt;
    // video_filterとaudio_filterの条件を満たすキャプチャデバイスを開く
    open_capture_device();
    #endif
  }
  {
    glcontext.reset(SDL_GL_CreateContext(window), SDL_GL_DeleteContext);
    if (glcontext == nullptr) {
      log(0, "SDL_GL_CreateContext: %s\n", SDL_GetError());
    }
    CHECK_SDL(SDL_GL_MakeCurrent(window, glcontext));
    #ifdef _MSC_VER
    if (glewInit() != 0) {
      log(0, "glewInit failed\n");
    }
    #endif
    set_vsync_mode((unsigned)conf.get_uint("vsync", 0));
    {
      #ifdef VCNET_OPENGL3
      GLuint a = 0;
      CHECK_GL(glGenVertexArrays(1, &a));
      vao.reset(a, wrap_glDeleteVertexArray);
      log(0, "vao %u\n", (unsigned)vao.get());
      #endif
    }
    #ifdef VCNET_GLES
    #define highp "highp "
    #define lowp "mediump "
    #else
    #define highp ""
    #define lowp ""
    #endif
    {
      const char *vssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "uniform " highp "vec2 video_size;\n" // viewのピクセル数
        "uniform " highp "vec2 scale;\n" // アスペクト比を合わせるための係数
        "uniform " lowp "float video_format;\n" // 0: RGB, 1: YUV411
        "attribute " highp "vec2 coord;\n" // [-1,1]範囲のフラグメント座標
        "varying " highp "vec2 v_video_coord;\n" // ピクセル単位のvideo座標
        "varying " highp "vec2 v_video_size_inv;\n"
        "void main(void) {\n"
        "  " highp "vec2 coord_s = coord * scale;\n"
        "  if (video_format == 3.0) { coord_s.y *= -1.0; }\n"
          // RGBAのときは上下反転
        "  v_video_coord = vec2(1.0 + coord_s.x, 1.0 - coord_s.y)\n"
        "    * 0.5 * video_size;\n"
        "  v_video_size_inv = 1.0 / video_size;\n"
        "  gl_Position = vec4(coord, 0.0, 1.0);\n"
        "}\n";
      const char *fssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "uniform " highp "vec2 video_size;\n" // viewのピクセル数
        "uniform sampler2D tex_video;\n" // ビデオ画像テクスチャ
        "uniform sampler2D tex_message;\n" // メッセージテクスチャ
        "uniform " lowp "float bilinear;\n" // bilinear補間をするかどうか
        "uniform " lowp "float video_format;\n" // 0: RGB, 1: YUV411
        "uniform " lowp "float y_interpolation;\n" // 補間処理方式
        "varying " highp "vec2 v_video_coord;\n" // ピクセル単位のvideo座標
        "varying " highp "vec2 v_video_size_inv;\n"
        "const " highp "float inv65536 = 1.0 / 65536.0;\n"
        lowp "vec4 compatTexelFetch(" highp "vec2 p) {\n"
        "  " highp "vec2 c = (p + 0.5) * v_video_size_inv;\n"
        "  if (c.x >= 0.0 && c.y >= 0.0 && c.x < 1.0 && c.y < 1.0) {\n"
        "    return texture2D(tex_video, (p + 0.5) * v_video_size_inv);\n"
        "  }\n"
        "  return vec4(0.5, 0.0, 0.0, 1.0);\n"
        "}\n"
        lowp "float interpolateUV(" lowp "float ya, " lowp "float yb,\n"
        "  " lowp "float yc, " lowp "float w, " lowp "float uva,\n"
        "  " lowp "float uvb) {\n"
        "  if (y_interpolation != 0.0) {\n"
        "    if (ya < yb) {\n"
        "      w = smoothstep(ya, yb, yc);\n"
        "    } else if (ya > yb) {\n"
        "      w = 1.0 - smoothstep(yb, ya, yc);\n"
        "    }\n"
        "  }\n"
        "  return mix(uva, uvb, w);\n"
        "}\n"
        lowp "vec4 texelFetchYUV411(" highp "vec2 pc) {\n"
        "  if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "    pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "    return vec4(0.0, 0.0, 0.0, 1.0);\n"
        "  }\n"
        "  " highp "float pcx4 = pc.x * 0.25;\n"
        "  " highp "float pcx4_i = floor(pcx4);\n"
        "  " highp "float pcx4_f = pcx4 - pcx4_i;\n"
        "  " highp "float pcx2 = pc.x * 0.5;\n"
        "  " highp "float pcx2_i = floor(pcx2);\n"
        "  " highp "float pcx2_f = pcx2 - pcx2_i;\n"
        "  " highp "float pcx2_0 = pcx2_i * 2.0;\n"
        "  " lowp "vec3 t0 = compatTexelFetch(vec2(pcx2_0 - 2.0, pc.y)).xyz;\n"
        "  " lowp "vec3 t1 = compatTexelFetch(vec2(pcx2_0, pc.y)).xyz;\n"
        "  " lowp "vec3 t2 = compatTexelFetch(vec2(pcx2_0 + 2.0, pc.y)).xyz;\n"
        "  " lowp "vec3 t3 = compatTexelFetch(vec2(pcx2_0 + 4.0, pc.y)).xyz;\n"
        "  " lowp "float y, u, v;\n"
        "  if (pcx4_f < 0.25) {\n"
        "    y = t1.y;\n"
        "    u = t1.x;\n"
        "    v = interpolateUV(t0.y, t2.y, y, 0.5, t0.x, t2.x);\n"
        "  } else if (pcx4_f < 0.5) {\n"
        "    y = t1.z;\n"
        "    u = interpolateUV(t1.y, t3.y, y, 0.25, t1.x, t3.x);\n"
        "    v = interpolateUV(t0.y, t2.y, y, 0.75, t0.x, t2.x);\n"
        "  } else if (pcx4_f < 0.75) {\n"
        "    y = t1.y;\n"
        "    u = interpolateUV(t0.y, t2.y, y, 0.5, t0.x, t2.x);\n"
        "    v = t1.x;\n"
        "  } else {\n"
        "    y = t1.z;\n"
        "    u = interpolateUV(t0.y, t2.y, y, 0.75, t0.x, t2.x);\n"
        "    v = interpolateUV(t1.y, t3.y, y, 0.25, t1.x, t3.x);\n"
        "  }\n"
        "  u = u - (128.0/255.0);\n"
        "  v = v - (128.0/255.0);\n"
        "  " lowp "float r = 0.996074463 * y + 0.00493749408 * u\n"
        "    + 1.40562660 * v;\n"
        "  " lowp "float g = 0.997438770 * y - 0.344325245 * u\n"
        "    - 0.721328904 * v;\n"
        "  " lowp "float b = 0.989187958 * y + 1.76788275 * u\n"
        "    - 0.00116940649 * v;\n"
        "  return clamp(vec4(r, g, b, 1.0), 0.0, 1.0);\n"
        "}\n"
        lowp "vec2 texelFetchNV12_UV(" highp "vec2 puv) {\n"
          // puvはxy座標の半分
        "  puv = clamp(puv, vec2(0.0), video_size * 0.5 - 1.0);\n"
        "  " highp "float puv_x0 = puv.x * 2.0;\n"
        "  " highp "float puv_x1 = puv_x0 + 1.0;\n"
        "  " highp "float puv_y = (puv.y + video_size.y) / 1.5;\n"
        "  " lowp "float u = compatTexelFetch(vec2(puv_x0, puv_y)).x * 255.0;\n"
        "  " lowp "float v = compatTexelFetch(vec2(puv_x1, puv_y)).x * 255.0;\n"
        "  return vec2(u, v);\n"
        "}\n"
        lowp "vec4 texelFetchNV12(" highp "vec2 pc) {\n"
        "  if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "    pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "    return vec4(0.0, 0.0, 0.0, 1.0);\n"
        "  }\n"
        "  " lowp "float t0 = compatTexelFetch(vec2(pc.x, pc.y / 1.5)).x\n"
        "    * 255.0;\n"
        "  " lowp "float y = t0 - 16.0;\n"
        "  " lowp "vec2 uv0, uv1, uv2, uv3;\n"
        "  " highp "vec2 pc2 = pc * 0.5;\n"
        "  " highp "vec2 pc2_i = floor(pc2);\n"
        "  " highp "vec2 pc2_f = pc2 - pc2_i;\n"
        "  if (pc2_f.x < 0.5) {\n"
        "    if (pc2_f.y < 0.5) {\n" // 左上
        "      uv0 = texelFetchNV12_UV(pc2_i);\n"
        "      uv1 = texelFetchNV12_UV(pc2_i + vec2(0.0, -1.0));\n"
        "      uv2 = uv0;\n"
        "      uv3 = uv1;\n"
        "    } else {\n"             // 左下
        "      uv0 = texelFetchNV12_UV(pc2_i);\n"
        "      uv1 = texelFetchNV12_UV(pc2_i + vec2(0.0, 1.0));\n"
        "      uv2 = uv0;\n"
        "      uv3 = uv1;\n"
        "    }\n"
        "  } else {\n"
        "    if (pc2_f.y < 0.5) {\n" // 右上
        "      uv0 = texelFetchNV12_UV(pc2_i);\n"
        "      uv1 = texelFetchNV12_UV(pc2_i + vec2(0.0, -1.0));\n"
        "      uv2 = texelFetchNV12_UV(pc2_i + vec2(1.0, 0.0));\n"
        "      uv3 = texelFetchNV12_UV(pc2_i + vec2(1.0, -1.0));\n"
        "    } else {\n"             // 右下
        "      uv0 = texelFetchNV12_UV(pc2_i);\n"
        "      uv1 = texelFetchNV12_UV(pc2_i + vec2(0.0, 1.0));\n"
        "      uv2 = texelFetchNV12_UV(pc2_i + vec2(1.0, 0.0));\n"
        "      uv3 = texelFetchNV12_UV(pc2_i + vec2(1.0, 1.0));\n"
        "    }\n"
        "  }\n"
        "  " lowp "vec2 uv = (uv0 + uv1 + uv2 + uv3) * 0.25 - 128.0;\n"
        "  " lowp "float u = uv.x;\n"
        "  " lowp "float v = uv.y;\n"
        "  " lowp "float r = (298.0 * y + 409.0 * v + 128.0) * inv65536;\n"
        "  " lowp "float g = (298.0 * y - 100.0 * u - 208.0 * v + 128.0)\n"
        "    * inv65536;\n"
        "  " lowp "float b = (298.0 * y + 516.0 * u + 128.0) * inv65536;\n"
        "  return vec4(clamp(vec3(r, g, b), 0.0, 1.0), 1.0);\n"
        "}\n"
        lowp "vec4 texelFetchYUV422(" highp "vec2 pc) {\n"
        "  if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "    pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "    return vec4(0.0, 0.0, 0.0, 1.0);\n"
        "  }\n"
        "  " highp "float pcx2 = pc.x * 0.5;\n"
        "  " highp "float pcx2_i = floor(pcx2);\n"
        "  " highp "float pcx2_f = pcx2 - pcx2_i;\n"
        "  " highp "float pcx_0 = pcx2_i * 2.0;\n"
        "  " highp "float pcx_1 = pcx_0 + 1.0;\n"
        "  " lowp "vec2 t0 = compatTexelFetch(vec2(pcx_0, pc.y)).xy * 255.0;\n"
        "  " lowp "vec2 t1 = compatTexelFetch(vec2(pcx_1, pc.y)).xy * 255.0;\n"
        "  " lowp "float y = ((pcx2_f < 0.5) ? t0.y : t1.y) - 16.0;\n"
        "  " lowp "float u = t0.x - 128.0;\n"
        "  " lowp "float v = t1.x - 128.0;\n"
        "  " lowp "float r = (298.0 * y + 409.0 * v + 128.0) * inv65536;\n"
        "  " lowp "float g = (298.0 * y - 100.0 * u - 208.0 * v + 128.0)\n"
        "    * inv65536;\n"
        "  " lowp "float b = (298.0 * y + 516.0 * u + 128.0) * inv65536;\n"
        "  return vec4(clamp(vec3(r, g, b), 0.0, 1.0), 1.0);\n"
        "}\n"
        lowp "vec4 texelFetchYUY2(" highp "vec2 pc) {\n"
        "  pc = clamp(pc, vec2(0.0), video_size - vec2(0.5));\n"
        "  " highp "float pcx2 = pc.x * 0.5;\n"
        "  " highp "float pcx2_i = floor(pcx2);\n"
        "  " highp "float pcx2_f = pcx2 - pcx2_i;\n"
        "  " highp "float pcx_0 = pcx2_i * 2.0;\n"
        "  " highp "float pcx_1 = pcx_0 + 1.0;\n"
        "  " lowp "vec2 t0 = compatTexelFetch(vec2(pcx_0, pc.y)).yx * 255.0;\n"
        "  " lowp "vec2 t1 = compatTexelFetch(vec2(pcx_1, pc.y)).yx * 255.0;\n"
        "  " lowp "float y, u, v;\n"
        "  if (pcx2_f < 0.5) {\n"
        "    y = t0.y;\n"
        "    u = t0.x - 128.0;\n"
        "    v = t1.x - 128.0;\n"
        "  } else {\n"
        "    y = t1.y;\n"
        "    " highp "float pcx_2 = pcx_0 + 2.0;\n"
        "    if (pcx_2 >= video_size.x) { pcx_2 = pcx_0; }\n"
        "    " highp "float pcx_3 = pcx_2 + 1.0;\n"
        "    " lowp "vec2 t2 = compatTexelFetch(vec2(pcx_2, pc.y)).yx\n"
        "      * 255.0;\n"
        "    " lowp "vec2 t3 = compatTexelFetch(vec2(pcx_3, pc.y)).yx\n"
        "      * 255.0;\n"
        "    u = (t0.x + t2.x) * 0.5 - 128.0;\n"
        "    v = (t1.x + t3.x) * 0.5 - 128.0;\n"
        "  }\n"
        "  " lowp "float r = (298.0 * y + 409.0 * v + 128.0) * inv65536;\n"
        "  " lowp "float g = (298.0 * y - 100.0 * u - 208.0 * v + 128.0)\n"
        "    * inv65536;\n"
        "  " lowp "float b = (298.0 * y + 516.0 * u + 128.0) * inv65536;\n"
        "  return vec4(clamp(vec3(r, g, b), 0.0, 1.0), 1.0);\n"
        "}\n"
        "void main(void) {\n"
        "  if (video_format == 0.0 || video_format == 3.0) {\n"
          // RGB888またはRGBA8888。それぞれvcnet_10g、UVCキャプチャで使用。
        "    " highp "vec2 pc = v_video_coord;\n"
        "    if (pc.x < 0.0 || pc.x >= video_size.x ||\n"
        "      pc.y < 0.0 || pc.y >= video_size.y) {\n"
        "      gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "    } else {\n"
        "      gl_FragColor =\n"
        "        texture2D(tex_video, pc * v_video_size_inv).bgra;\n"
        "    }\n"
        "  } else if (video_format == 1.0) {\n"
          // YUV411。vcnet_1gで使用。
        "    if (bilinear == 0.0) {\n"
        "      gl_FragColor = texelFetchYUV411(floor(v_video_coord));\n"
        "    } else {\n"
        "      " highp "vec2 vp00 = v_video_coord - 0.5;\n"
        "      " highp "vec2 vpf = floor(vp00);\n"
        "      " lowp "vec4 c00 = texelFetchYUV411(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c10 = texelFetchYUV411(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c01 = texelFetchYUV411(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec4 c11 = texelFetchYUV411(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec2 a = vp00 - vpf;\n"
        "      " lowp "vec4 c0 = mix(c00, c10, a.x);\n"
        "      " lowp "vec4 c1 = mix(c01, c11, a.x);\n"
        "      gl_FragColor = mix(c0, c1, a.y);\n"
        "    }\n"
        "  } else if (video_format == 4.0) {\n"
          // NV12 12bpp。UVCキャプチャで使用。8bppで高さ1.5倍のテクスチャ。
        "    if (bilinear == 0.0) {\n"
        "      gl_FragColor = texelFetchNV12(floor(v_video_coord));\n"
        "    } else {\n"
        "      " highp "vec2 vp00 = v_video_coord - 0.5;\n"
        "      " highp "vec2 vpf = floor(vp00);\n"
        "      " lowp "vec4 c00 = texelFetchNV12(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c10 = texelFetchNV12(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c01 = texelFetchNV12(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec4 c11 = texelFetchNV12(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec2 a = vp00 - vpf;\n"
        "      " lowp "vec4 c0 = mix(c00, c10, a.x);\n"
        "      " lowp "vec4 c1 = mix(c01, c11, a.x);\n"
        "      gl_FragColor = mix(c0, c1, a.y);\n"
        "    }\n"
        "  } else if (video_format == 2.0) {\n"
          // YUV422 使っていない。TODO: 右ピクセルのChromaは補間するように。
        "    if (bilinear == 0.0) {\n"
        "      gl_FragColor = texelFetchYUV422(floor(v_video_coord));\n"
        "    } else {\n"
        "      " highp "vec2 vp00 = v_video_coord - 0.5;\n"
        "      " highp "vec2 vpf = floor(vp00);\n"
        "      " lowp "vec4 c00 = texelFetchYUV422(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c10 = texelFetchYUV422(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c01 = texelFetchYUV422(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec4 c11 = texelFetchYUV422(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec2 a = vp00 - vpf;\n"
        "      " lowp "vec4 c0 = mix(c00, c10, a.x);\n"
        "      " lowp "vec4 c1 = mix(c01, c11, a.x);\n"
        "      gl_FragColor = mix(c0, c1, a.y);\n"
        "    }\n"
        "  } else if (video_format == 5.0) {\n"
          // YUY2。UVCキャプチャで使用。
        "    if (bilinear == 0.0) {\n"
        "      gl_FragColor = texelFetchYUY2(floor(v_video_coord));\n"
        "    } else {\n"
        "      " highp "vec2 vp00 = v_video_coord - 0.5;\n"
        "      " highp "vec2 vpf = floor(vp00);\n"
        "      " lowp "vec4 c00 = texelFetchYUY2(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c10 = texelFetchYUY2(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 0.0));\n"
        "      " lowp "vec4 c01 = texelFetchYUY2(vec2(vpf.x + 0.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec4 c11 = texelFetchYUY2(vec2(vpf.x + 1.0,\n"
        "        vpf.y + 1.0));\n"
        "      " lowp "vec2 a = vp00 - vpf;\n"
        "      " lowp "vec4 c0 = mix(c00, c10, a.x);\n"
        "      " lowp "vec4 c1 = mix(c01, c11, a.x);\n"
        "      gl_FragColor = mix(c0, c1, a.y);\n"
        "    }\n"
        "  }\n"
        "}\n";
      compile_shader(vssrc, fssrc, prog_video);
    }
    {
      const char *vssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "uniform " highp "vec2 cursor_pos;\n"
        "uniform " highp "vec2 cursor_scale;\n"
        "attribute " highp "vec2 cursor_coord;\n" // [-1,1]範囲
        "varying " highp "vec2 v_coord;\n" // [-1,1]範囲
        "void main(void) {\n"
        "  " highp "vec2 pos = cursor_pos + cursor_coord * cursor_scale;\n"
        "  v_coord = cursor_coord;\n"
        "  gl_Position = vec4(pos, 0.0, 1.0);\n"
        "}\n";
      const char *fssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "varying " highp "vec2 v_coord;\n" // [-1,1]範囲
        "void main(void) {\n"
        "  " highp "float dist = dot(v_coord, v_coord);\n"
        "  " lowp "float c = dist < 1.0 ? 1.0 : 0.0;\n"
        "  if (c < 1.0) { discard; }\n"
        "  gl_FragColor = vec4(0.3, 0.3, 0.3, 0.5);\n"
        "}\n";
      compile_shader(vssrc, fssrc, prog_cursor);
    }
    {
      const char *vssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "uniform " lowp "vec4 color;\n"
        "attribute " highp "vec2 coord;\n" // [-1,1]範囲
        "varying " lowp "vec4 v_color;\n"
        "void main(void) {\n"
        "  v_color = color;\n"
        "  gl_Position = vec4(coord, 0.0, 1.0);\n"
        "}\n";
      const char *fssrc =
        #ifdef VCNET_GLES
        "#version 100\n"
        #else
        "#version 110\n"
        #endif
        "varying " lowp "vec4 v_color;\n" // [-1,1]範囲
        "void main(void) {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";
      compile_shader(vssrc, fssrc, prog_lagtest);
    }
    #undef highp
    #undef lowp
    {
      attr_coord = glGetAttribLocation(prog_video, "coord");
      if (attr_coord == -1) {
        log(0, "failed to get location: attr_coord\n");
        fatal_error("failed to get location");
      } else {
        log(0, "attr_coord %d\n", attr_coord);
      }
      loc_tex_video = glGetUniformLocation(prog_video, "tex_video");
      loc_video_size = glGetUniformLocation(prog_video, "video_size");
      loc_bilinear = glGetUniformLocation(prog_video, "bilinear");
      loc_video_format = glGetUniformLocation(prog_video, "video_format");
      loc_y_interpolation = glGetUniformLocation(prog_video,
        "y_interpolation");
      loc_scale = glGetUniformLocation(prog_video, "scale");
    }
    {
      attr_cursor_coord = glGetAttribLocation(prog_cursor, "cursor_coord");
      if (attr_cursor_coord == -1) {
        log(0, "failed to get location: attr_cursor_coord\n");
        fatal_error("failed to get location");
      } else {
        log(0, "attr_cursor_coord %d\n", attr_cursor_coord);
      }
      loc_cursor_pos = glGetUniformLocation(prog_cursor, "cursor_pos");
      loc_cursor_scale = glGetUniformLocation(prog_cursor, "cursor_scale");
    }
    {
      attr_lagtest_coord = glGetAttribLocation(prog_lagtest, "coord");
      if (attr_lagtest_coord == -1) {
        log(0, "failed to get location: attr_lagtest_coord\n");
        fatal_error("failed to get location");
      } else {
        log(0, "attr_lagtest_coord %d\n", attr_lagtest_coord);
      }
      loc_lagtest_color = glGetUniformLocation(prog_lagtest, "color");
    }
    {
      GLfloat coord2_triangles[] = {
         1.0,  1.0,
        -1.0,  1.0,
        -1.0, -1.0,
        -1.0, -1.0,
         1.0, -1.0,
         1.0,  1.0,
      };
      GLuint b;
      CHECK_GL(glGenBuffers(1, &b));
      vbo_coord.reset(b, wrap_glDeleteBuffer);
      CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, b));
      CHECK_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(coord2_triangles),
        &coord2_triangles, GL_STATIC_DRAW));
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
      #ifdef VCNET_GLES
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE));
      #else
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_BORDER));
      CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_BORDER));
      #endif
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
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    std::string fn = "Cousine-Regular.ttf";
    fn = conf.get_str("font", fn);
    if (!fn.empty()) {
      auto fnsz = conf.get_uint("font_size", 24);
      ttf_image = std::make_shared<std::string>(load_file(fn));
        // ttfのイメージのアドレスが変わってはいけないので、まちがえない
        // ようにshared_ptrで保持する。
      if (ttf_image->empty()) {
        fatal_error("failed to load font file");
      }
      ImFontConfig font_cfg { };
      font_cfg.FontDataOwnedByAtlas = false;
        // このようにしないとttfのイメージはImGuiがfreeする。
      io.Fonts->AddFontFromMemoryTTF(&(*ttf_image)[0], (int)ttf_image->size(),
        (float)fnsz, &font_cfg); 
    }
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
    #ifdef VCNET_GLES
    const char *glsl_version = "#version 100";
    #else
    const char *glsl_version = "#version 110";
    #endif
    ImGui_ImplOpenGL3_Init(glsl_version);
  }
}

vcnet_window::~vcnet_window()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void
vcnet_window::compile_shader(const char *vssrc, const char *fssrc,
  auto_res<GLuint>& prog)
{
  auto_res<GLuint> vs;
  auto_res<GLuint> fs;
  {
    vs.reset(glCreateShader(GL_VERTEX_SHADER), glDeleteShader);
    CHECK_GL(glShaderSource(vs, 1, &vssrc, nullptr));
    CHECK_GL(glCompileShader(vs));
    GLint v = 0;
    CHECK_GL(glGetShaderiv(vs, GL_COMPILE_STATUS, &v));
    std::vector<char> buf(1024);
    GLsizei sz = 0;
    CHECK_GL(glGetShaderInfoLog(vs, (GLsizei)buf.size(), &sz, &buf[0]));
    if (v == 0) {
      log(0, "failed to compile vs: %s\n", &buf[0]);
      fatal_error("failed to compile vs");
    } else {
      log(0, "vertex shader %u %s\n", vs.get(), &buf[0]);
    }
  }
  {
    fs.reset(glCreateShader(GL_FRAGMENT_SHADER), glDeleteShader);
    CHECK_GL(glShaderSource(fs, 1, &fssrc, nullptr));
    CHECK_GL(glCompileShader(fs));
    GLint v = 0;
    CHECK_GL(glGetShaderiv(fs, GL_COMPILE_STATUS, &v));
    std::vector<char> buf(1024);
    GLsizei sz = 0;
    CHECK_GL(glGetShaderInfoLog(fs, (GLsizei)buf.size(), &sz, &buf[0]));
    if (v == 0) {
      log(0, "failed to compile fs: %s\n", &buf[0]);
      fatal_error("failed to compile fs");
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
      fatal_error("failed to link prog");
    } else {
      log(0, "prog %u %s\n", prog.get(), &buf[0]);
    }
  }
}

void
vcnet_window::update_texture_sdl(vcnet_pixels const& pix)
{
  // 通常この関数は使用しない。デバッグのためだけに残している。
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
    uint8_t const fmt = pix.get_format();
    if (fmt == 0) {
      // RGB
      for (unsigned w = 0; w < texwidth; ++w) {
        p[0] = p3[spos + 0];
        p[1] = p3[spos + 1];
        p[2] = p3[spos + 2];
        p[3] = 0;
        p += 4;
        spos += 3;
      }
    } else if (fmt == 1) {
      // YUV411: ただしこの処理は不正確。GL版のほうが正確。
      for (unsigned w = 0; w + 3 < texwidth; w += 4) {
        auto to_u8 = [] (int32_t v) {
          int32_t cv = v < 0 ? 0 : v > 65535 ? 65535 : v;
          return uint8_t(cv >> 8);
        };
        int16_t u = p3[spos + 0];
        int16_t y0 = p3[spos + 1];
        int16_t y1 = p3[spos + 2];
        int16_t v = p3[spos + 3];
        int16_t y2 = p3[spos + 4];
        int16_t y3 = p3[spos + 5];
        int32_t c0 = y0 - 16;
        int32_t c1 = y1 - 16;
        int32_t c2 = y2 - 16;
        int32_t c3 = y3 - 16;
        int32_t d = u - 128;
        int32_t e = v - 128;
        uint8_t r0 = to_u8(298 * c0 + 409 * e + 128);
        uint8_t g0 = to_u8(298 * c0 - 100 * d - 208 * e + 128);
        uint8_t b0 = to_u8(298 * c0 + 516 * d + 128);
        uint8_t r1 = to_u8(298 * c1 + 409 * e + 128);
        uint8_t g1 = to_u8(298 * c1 - 100 * d - 208 * e + 128);
        uint8_t b1 = to_u8(298 * c1 + 516 * d + 128);
        uint8_t r2 = to_u8(298 * c2 + 409 * e + 128);
        uint8_t g2 = to_u8(298 * c2 - 100 * d - 208 * e + 128);
        uint8_t b2 = to_u8(298 * c2 + 516 * d + 128);
        uint8_t r3 = to_u8(298 * c3 + 409 * e + 128);
        uint8_t g3 = to_u8(298 * c3 - 100 * d - 208 * e + 128);
        uint8_t b3 = to_u8(298 * c3 + 516 * d + 128);
        p[0] = b0;
        p[1] = g0;
        p[2] = r0;
        p[3] = 0;
        p[4] = b1;
        p[5] = g1;
        p[6] = r1;
        p[7] = 0;
        p[8] = b2;
        p[9] = g2;
        p[10] = r2;
        p[11] = 0;
        p[12] = b3;
        p[13] = g3;
        p[14] = r3;
        p[15] = 0;
        p += 16;
        spos += 6;
      }
    } else if (fmt == 2) {
      // YUV422: ただしこの処理は不正確。GL版のほうが正確。
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
  if (use_gl && !has_mf_window()) {
    // FIXME?
    CHECK_SDL(SDL_GL_MakeCurrent(window, glcontext));
    SDL_GL_GetDrawableSize(window, &w, &h);
  } else {
    SDL_GetWindowSize(window, &w, &h);
  }
  if ((unsigned)w != window_width || (unsigned)h != window_height) {
    window_width = w;
    window_height = h;
    log(1, "resize_window window %d %d\n", w, h);
    CHECK_GL(glViewport(0, 0, w, h));
    cur_draw_rect.w = window_width;
    cur_draw_rect.h = window_height;
  }
  #ifdef _MSC_VER
  if (mfplayer_video) {
    if (!video_use_sample_reader) {
      mfplayer_video->resize_video_window(window_width, window_height);
    } else {
    }
    recalc_draw_rect(video_attr.width, video_attr.height);
  }
  #endif
}

void
vcnet_window::recalc_draw_rect(unsigned texwidth, unsigned texheight)
{
  // cur_draw_rectを計算する。これはabsmouseの座標計算に使われる。
  // windowと映像のアスペクト比が異なる場合、映像の上下または左右に
  // 余白が入る。その余白を除いたwindowの範囲をcur_draw_rectにセットする。
  log(12, "window %u %u tex %u %u\n", window_width, window_height,
    texwidth, texheight);
  if (texwidth == 0 || texheight == 0 || window_height == 0
    || window_width == 0) {
    return;
  }
  unsigned hw = window_height * texwidth;
  unsigned wh = window_width * texheight;
  SDL_Rect rect = { };
  if (hw == wh) {
    rect.w = window_width;
    rect.h = window_height;
  } else {
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
  }
  cur_draw_rect = rect;
  log(12, "draw rect x,y=%u,%u w,h=%u,%u\n", rect.x, rect.y, rect.w, rect.h);
}

void
vcnet_window::draw_window_gl(vcnet_pixels const& pix, bool video_stable,
  bool imgui_draw, bool cursor_draw, bool suppress_vsync_wait)
{
  texwidth = pix.get_width();
  texheight = pix.get_height();
  #if _MSC_VER
  if (mfplayer_video && !video_use_sample_reader && !imgui_draw) {
    // mfのプレビューウインドウがビデオを描画する
    return;
  }
  if (mfplayer_video && video_use_sample_reader) {
    if (cur_video_sample.get() == nullptr) {
        return;
    }
    texwidth = video_attr.width;
    texheight = video_attr.height;
    suppress_vsync_wait = false;
      // videoにsample_reader使用のときは一時的vsync無効化やらない
  }
  #endif
  // log(0, "prog=%d\n", (int)prog.get());
  CHECK_GL(glDisable(GL_DEPTH_TEST)); // FIXME?
  CHECK_GL(glClearColor(0.5, 0.5, 0.5, 1.0));
  CHECK_GL(glClear(
    GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
  // ビデオフレーム描画
  CHECK_GL(glUseProgram(prog_video));
  float const bilinear = (texwidth == window_width &&
    texheight == window_height)
    ? 0.0f : 1.0f;
  vcnet_video_format_e fmt = pix.get_format();
  {
    const unsigned char *p = nullptr;
    p = pix.begin();
    size_t psz = pix.size();
    #ifdef _MSC_VER
    if (video_use_sample_reader) {
      assert(cur_video_sample.get());
      unsigned char *data = nullptr;
      DWORD datasz = 0;
      (void)cur_video_sample->Lock(&data, nullptr, &datasz);
      p = data;
      psz = datasz;
      log(24, "data %p sz %u\n", data, datasz);
      fmt = capt_video_format;
    }
    std::unique_ptr<IMFMediaBuffer, void(*)(IMFMediaBuffer *)>
      data_unlock(cur_video_sample.get(),
        [](IMFMediaBuffer *p) { if (p) { p->Unlock(); } });
      // ブロックを抜けたときにcur_video_sample->Unlock()を呼ぶ
    #endif
    cur_video_format = fmt;
    CHECK_GL((void)0);
    CHECK_GL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, tex_video));
    // TODO: 毎回やらなくてもいいのでは
    if ((fmt != vcnet_video_format_e_rgb888 &&
      fmt != vcnet_video_format_e_rgba8888)
      || bilinear == 0.0f) {
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
    bool skip_draw = true;
      // キャプチャデバイスから取得したサンプルは、フォーマット変更直後は
      // テクスチャの大きさが変更前のもののことがある。そのときはテクスチャ
      // 転送できないので描画をスキップする。
    switch (fmt) {
    case vcnet_video_format_e_rgb888:
      // RGB 24bpp
      if (psz == texwidth * texheight * 3) {
        skip_draw = false;
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth,
          texheight, 0, GL_RGB, GL_UNSIGNED_BYTE, p));
      }
      break;
    case vcnet_video_format_e_rgba8888:
      // RGBA 32bpp
      if (psz == texwidth * texheight * 4) {
        skip_draw = false;
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texwidth,
          texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, p));
      }
      break;
    case vcnet_video_format_e_yuv411:
      // YUV411 12bpp
      // 便宜上RGB24bpp形式テクスチャ(幅半分)として転送する
      if (psz == texwidth * texheight * 3 / 2) {
        skip_draw = false;
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth / 2,
          texheight, 0, GL_RGB, GL_UNSIGNED_BYTE, p));
      }
      break;
    case vcnet_video_format_e_nv12:
      // NV12 12bpp
      // 便宜上8bpp形式テクスチャ(高さ1.5倍)として転送する
      if (psz == texwidth * texheight * 3 / 2) {
        skip_draw = false;
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texwidth,
          texheight + (texheight >> 1), 0, GL_RED, GL_UNSIGNED_BYTE, p));
      }
      break;
    case vcnet_video_format_e_yuv422:
    case vcnet_video_format_e_yuy2:
      // YUV422/YUY2 16bpp
      if (psz == texwidth * texheight * 2) {
        skip_draw = false;
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, texwidth,
          texheight, 0, GL_RG, GL_UNSIGNED_BYTE, p));
      }
      break;
    }
    if (skip_draw) {
      return;
    }
  }
  float scale_x = 1.0;
  float scale_y = 1.0;
  if (window_height != 0 && window_width != 0 &&
    texwidth != 0 && texheight != 0) {
    recalc_draw_rect(texwidth, texheight);
    unsigned hw = window_height * texwidth;
    unsigned wh = window_width * texheight;
    if (hw > wh) {
      scale_y = (float)hw / (float)wh;
    } else if (wh > hw) {
      scale_x = (float)wh / (float)hw;
    }
  }
  // log(0, "scale_x=%f scale_y=%f\n", scale_x, scale_y);
  CHECK_GL(glDisable(GL_BLEND));
  CHECK_GL(glUniform1i(loc_tex_video, 0));
  CHECK_GL(glUniform2f(loc_video_size, (float)texwidth, (float)texheight));
  CHECK_GL(glUniform2f(loc_scale, scale_x, scale_y));
  CHECK_GL(glUniform1f(loc_bilinear, bilinear));
  CHECK_GL(glUniform1f(loc_video_format, (float)fmt));
  CHECK_GL(glUniform1f(loc_y_interpolation, (float)y_interpolation));
  CHECK_GL(glEnableVertexAttribArray(attr_coord));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_coord));
  CHECK_GL(glVertexAttribPointer(attr_coord, 2, GL_FLOAT, GL_FALSE, 0, 0));
  CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  CHECK_GL(glDisableVertexAttribArray(attr_coord));
  // GUI描画
  if (imgui_draw) {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  // フルスクリーン時のカーソル描画
  if (cursor_draw) {
    CHECK_GL(glUseProgram(prog_cursor));
    double cursor_scale_x = 16.0 / (double)window_width;
    double cursor_scale_y = 16.0 / (double)window_height;
    int x = 0;
    int y = 0;
    // FIXME: SDL_GetDrawableSizeとSDL_GetWindowSizeが異なるときは？
    SDL_GetMouseState(&x, &y);
    double cursor_pos_x = ((double)x / (double)window_width) * 2.0 - 1.0;
    double cursor_pos_y = ((double)y / (double)window_height) * 2.0 - 1.0;
    CHECK_GL(glEnable(GL_BLEND));
    CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    CHECK_GL(glUniform2f(loc_cursor_pos, (float)cursor_pos_x,
      (float)(-cursor_pos_y)));
    CHECK_GL(glUniform2f(loc_cursor_scale, (float)cursor_scale_x,
      (float)cursor_scale_y));
    CHECK_GL(glEnableVertexAttribArray(attr_cursor_coord));
    CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_coord));
    CHECK_GL(glVertexAttribPointer(attr_cursor_coord, 2, GL_FLOAT, GL_FALSE,
      0, 0));
    CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
    CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECK_GL(glDisableVertexAttribArray(attr_cursor_coord));
  }
  if (vsync_mode != 0) {
    if (suppress_vsync_wait_saved != suppress_vsync_wait) {
      if (suppress_vsync_wait) {
        SDL_GL_SetSwapInterval(0);
        log(23, "suppress vsync wait\n");
      } else {
        SDL_GL_SetSwapInterval(vsync_mode == 1 ? 1 : -1);
        log(23, "no suppress vsync wait\n");
      }
    }
    suppress_vsync_wait_saved = suppress_vsync_wait;
  }
  SDL_GL_SwapWindow(window);
}

void
vcnet_window::draw_window_lagtest(uint32_t mode, bool color1, bool show_gui)
{
  CHECK_GL(glDisable(GL_DEPTH_TEST)); // FIXME?
  CHECK_GL(glClearColor(0.5, 0.5, 0.5, 1.0));
  CHECK_GL(glClear(
    GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
  CHECK_GL(glDisable(GL_BLEND));
  CHECK_GL(glUseProgram(prog_lagtest));
  float col_r = 0.0f;
  float col_b = 0.0f;
  float col_g = 0.0f;
  if (mode == 1) {
    // mode 1: green/blue
    col_g = color1 ? 1.0f : 0.0f;
    col_b = color1 ? 0.0f : 1.0f;
  } else if (mode == 2) {
    // mode 2: white/black
    col_r = col_g = col_b = (color1 ? 1.0f : 0.0f);
  }
  CHECK_GL(glUniform4f(loc_lagtest_color, col_r, col_g, col_b, 1.0f));
  CHECK_GL(glEnableVertexAttribArray(attr_lagtest_coord));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_coord));
  CHECK_GL(glVertexAttribPointer(attr_lagtest_coord, 2, GL_FLOAT, GL_FALSE,
    0, 0));
  CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
  CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  CHECK_GL(glDisableVertexAttribArray(attr_lagtest_coord));
  if (show_gui) {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  SDL_GL_SetSwapInterval(vsync_mode == 1 ? 1 : 0);
  SDL_GL_SwapWindow(window);
}

void
vcnet_window::draw_window_sdl(vcnet_pixels const& pix, bool video_stable)
{
  // 通常この関数は使用しない。デバッグのためだけに残している。
  recalc_draw_rect(texwidth, texheight);
  unsigned hw = window_height * texwidth;
  unsigned wh = window_width * texheight;
  if (hw == wh) {
    /* 縦横比が信号とウインドウで一致する。このときはvideo_stableがfalse
     * であっても描画する。*/
    SDL_RenderCopy(rend, sdltex, nullptr, nullptr);
  } else if (!video_stable) {
    /* 縦横比が信号とウインドウで一致せず、video_stableがfalseならばおか
     * しなデータである可能性が高いので描画しない。*/
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderClear(rend);
  } else {
    /* 縦横比が信号とウインドウで一致しない。左右または上下に余白を入れて
     * 描画する。*/
    if (window_height != 0 && window_width != 0 &&
      texwidth != 0 && texheight != 0) {
      SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
      SDL_RenderClear(rend);
      SDL_Rect src_rect = { };
      src_rect.w = texwidth;
      src_rect.h = texheight;
      src_rect.x = 0;
      src_rect.y = 0;
      SDL_RenderCopy(rend, sdltex, &src_rect, &cur_draw_rect);
    }
  }
  SDL_RenderPresent(rend);
}

bool
vcnet_window::get_real_fullscreen() const
{
  return real_fullscreen != 0;
}

void
vcnet_window::set_real_fullscreen(bool v)
{
  real_fullscreen = v;
  this->set_fullscreen(fullscreen);
}

bool
vcnet_window::get_fullscreen() const
{
  return fullscreen != 0;
}

void
vcnet_window::set_fullscreen(bool v)
{
  if (v && real_fullscreen) {
    fullscreen = SDL_WINDOW_FULLSCREEN;
  } else if (v) {
    fullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
  } else {
    fullscreen = 0;
  }
  log(1, "set fullscreen %d\n", (int)fullscreen);
  if (window.get() != nullptr) {
    SDL_SetWindowFullscreen(window, fullscreen);
    if (fullscreen != 0) {
      // SDL_SetWindowGrab(window, SDL_FALSE);
      // SDL_SetRelativeMouseMode(SDL_FALSE);
      // SDL_ShowCursor(SDL_ENABLE);
    }
  }
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
vcnet_window::set_title(std::string const& s)
{
  SDL_SetWindowTitle(window, s.c_str());
  log(0, "title %s\n", s.c_str());
}

void
vcnet_window::warp_mouse(int x, int y)
{
  SDL_WarpMouseInWindow(window, x, y);
}

bool
vcnet_window::is_screen_keyboard_shown()
{
  return SDL_IsScreenKeyboardShown(window);
}

void vcnet_window::set_show_mf_window(bool v)
{
  if (!has_mf_window()) {
    return;
  }
  show_mf_window = v;
  #ifdef _MSC_VER
  try {
    MFVideoNormalizedRect src_rect = { };
    RECT dest_rect = { };
    if (!v) {
      log(0, "hide video\n");
      mfplayer_video->set_video_position(NULL, &dest_rect);
    } else {
      log(0, "show video\n");
      dest_rect.left = 0;
      dest_rect.top = 0;
      dest_rect.right = window_width;
      dest_rect.bottom = window_height;
      mfplayer_video->set_video_position(NULL, &dest_rect);
      log(0, "set video position %u %u\n", window_width, window_height);
      if (fullscreen) {
        // fullscreenのときにshow_guiをfalseにするとcapture videoが表示
        // されなくなる(原因不明)のを回避するために、一度hide/showする。
        // ただしこれによって一度focusが失われ、右ALTを押している状態が
        // 解除される。
        SDL_HideWindow(window.get());
        SDL_ShowWindow(window.get());
      }
    }
  } catch (hresult_exception const& ex) {
    log(0, "exception %s\n", ex.what());
  }
  #endif
}

bool
vcnet_window::capture_video_read_sample()
{
  #ifdef _MSC_VER
  log(24, "capture_video_read_sample\n");
  try {
    if (!mfplayer_video || !video_use_sample_reader) {
      return false;
    }
    com_ptr<IMFSample> sample = mfplayer_video->read_sample();
    log(24, "capture_video_read_sample sample %p\n", sample.get());
    if (!sample) {
      return false;
    }
    com_ptr<IMFMediaBuffer> buf;
    throw_failure(sample->ConvertToContiguousBuffer(buf.put()));
    cur_video_sample = buf;
    return true;
  } catch (hresult_exception const& ex) {
    log(0, "capture_video_read_sample: hresult %x\n", (unsigned)ex.get());
  }
  #endif
  return false;
}

uint32_t
vcnet_window::queue_audio_sample(vcnet_audio& aud)
{
  #ifdef _MSC_VER
  // 再生バッファにたまったサイズを返す　
  uint32_t r = 0;
  try {
    if (!mfplayer_audio) {
      return 0;
    }
    com_ptr<IMFSample> sample = mfplayer_audio->read_sample();
    log(25, "capture_audio_read_sample sample %p\n", sample.get());
    if (!sample) {
      return 0;
    }
    if (mfplayer_audio_sps == 0) {
      mfplayer_audio_sps = mfplayer_audio->get_audio_sps();
      mfplayer_audio_channels = mfplayer_audio->get_audio_channels();
      log(0, "sps=%u ch=%u\n", mfplayer_audio_sps, mfplayer_audio_channels);
      aud.open_device_if(mfplayer_audio_sps, mfplayer_audio_channels);
    }
    // TODO: 連続にする必要ない?
    com_ptr<IMFMediaBuffer> buf;
    throw_failure(sample->ConvertToContiguousBuffer(buf.put()));
    BYTE *data = nullptr;
    DWORD datasz = 0;
    throw_failure(buf->Lock(&data, nullptr, &datasz));
    r = (uint32_t)aud.queue_audio(data, datasz);
    throw_failure(buf->Unlock());
    return r;
  } catch (hresult_exception const& ex) {
    log(0, "queue_audio_sample: hresult %x\n", (unsigned)ex.get());
  }
  #endif
  return 0;
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

struct vcnet_timeprof {
public:
  typedef std::chrono::high_resolution_clock clock;
  enum { num_counters = 6 };
  std::array<uint32_t, num_counters> counter;
  uint32_t time_prev = 0;
public:
  vcnet_timeprof() {
    time_prev = static_cast<uint32_t>(time_us(clock::now()));
    reset();
  }
  void mark(size_t p, clock::time_point t = clock::now()) {
    auto const t_us = static_cast<uint32_t>(time_us(t));
    if (p < num_counters) {
      counter[p] += t_us - time_prev;
    }
    time_prev = t_us;
  }
  void reset() {
    counter = { };
  }
  uint32_t sum() const {
    uint32_t r = 0;
    for (auto const& e: counter) {
      r += e;
    }
    return r;
  }
  void operator +=(vcnet_timeprof const& x) {
    for (size_t i = 0; i < num_counters; ++i) {
      counter[i] += x.counter[i];
    }
  }
  void operator /=(uint32_t x) {
    for (size_t i = 0; i < num_counters; ++i) {
      counter[i] /= x;
    }
  }
  std::string to_string() const {
    std::string r;
    r += "(" + std::to_string(sum()) + ") ";
    for (size_t i = 0; i < num_counters; ++i) {
      r += " " + std::to_string(counter[i]);
    }
    return r;
  }
};

//////////////////////////////////////////////////////////////

struct vcnet_control {
private:
  vcnet_config conf;
  vcnet_conn_delegate conn;
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
    bool is_complex = false;
  };
  std::deque<keycmd> delayed_keycmd;
  std::array<uint16_t, 128> ch2usb { };
  std::map<uint32_t, uint32_t> keymap;
  std::map<uint8_t, uint8_t> keymodmap;
  joymap_type joymap;
  std::chrono::system_clock::time_point start_time { };
  std::chrono::system_clock::time_point last_draw_time { };
  std::chrono::system_clock::time_point last_msg_time { };
  std::chrono::system_clock::time_point last_hid_time { };
  std::chrono::system_clock::time_point last_mouse_send_time { };
  std::chrono::system_clock::time_point last_joystick_send_time { };
  std::chrono::system_clock::time_point last_suppress_frames_time { };
  std::chrono::system_clock::time_point last_vfd_time { };
  uint32_t stat_vfd_min = (uint32_t)-1;
  uint32_t stat_vfd_max = 0;
  uint32_t stat_vfd_min_saved = 0;
  uint32_t stat_vfd_max_saved = 0;
  uint32_t stat_vfd_count = 0;
  bool show_info = false;
  uint32_t msg_color = 0xffffff;
  std::deque<std::array<uint8_t, 3>> adv7611_cmds;
  vcnet_joystick joy;
  int num_joysticks_saved = 0;
  std::string joystick_name;
  uint32_t hid_interval = 1;
  bool crop_mode = false;
  uint32_t special_keyaction = 0;
  uint32_t autoclose = 0;
  uint32_t special_kmod = 0x0a80; // KMOD_RCTRL | KMOD_RALT | KMOD_RGUI
    /* SDL_keycode.h
      KMOD_LSHIFT = 0x0001,
      KMOD_RSHIFT = 0x0002,
      KMOD_LCTRL = 0x0040,
      KMOD_RCTRL = 0x0080,
      KMOD_LALT = 0x0100,
      KMOD_RALT = 0x0200,
      KMOD_LGUI = 0x0400,
      KMOD_RGUI = 0x0800,
      KMOD_NUM = 0x1000,
      KMOD_CAPS = 0x2000,
      KMOD_MODE = 0x4000,
      KMOD_SCROLL = 0x8000,
     */
  uint32_t special_button = 4;
  bool special_button_pressed = false;
  std::set<SDL_FingerID> cur_fingers;
  #ifdef VCNET_MOBILE
  int last_fingerdown = -1;
  int last_fingermotion = -1;
  uint32_t last_fingerdown_timestamp = 0;
  #endif
  std::string cur_msg;
  uint32_t cur_msg_highlight = 0;
  bool show_gui = false;
  bool show_gui_demo = false;
  float gui_alpha = 1.0f;
  struct gui_values {
    bool mod_lctrl = false;
    bool mod_lshift = false;
    bool mod_lalt = false;
    bool mod_lgui = false;
    bool mod_rctrl = false;
    bool mod_rshift = false;
    bool mod_ralt = false;
    bool mod_rgui = false;
    unsigned keycode_idx = 0;
    int device_idx = -1;
    std::string conf_text;
    bool conf_saved = false;
  };
  gui_values gui { };
  #ifdef _MSC_VER
  UINT_PTR const timer_id = 1;
  bool timer_running = false;
  #endif
  uint64_t rtt_thr = 3000;
  struct timeprof_record {
    vcnet_timeprof cur, mi, mx, sum;
    vcnet_timeprof mi_saved, mx_saved, sum_saved;
    uint32_t count = 0;
    void reset() {
      cur = mi = mx = sum = mi_saved = mx_saved = sum_saved = { };
      count = 0;
    }
  };
  timeprof_record prof;
  uint32_t cur_joystick = 0;
  uint32_t lagtest_mode = 0;
  bool lagtest_mode_color1 = false;
private:
  void send_text(std::string const& text);
  void send_mouse_button_delayed(uint8_t btn, bool press);
  void convert_scancode(uint8_t c, uint32_t s, uint8_t m, uint8_t& c_r,
    uint8_t& m_r, bool& downup_r, bool& is_complex_conv_r);
  uint8_t keymod_sdl2usb(uint16_t mod);
  void handle_keyboard(SDL_KeyboardEvent const& kev, bool down);
  bool send_delayed_keycmd();
  void clear_delayed_keycmd();
  void set_show_gui(bool value);
  bool is_special_kmod_scancode(uint32_t scancode) const;
  void handle_event(SDL_Event const& ev, bool& done_r);
  void draw_gui();
  void paste_clipboard_text();
  void send_hid_state();
  void exec_step();
  void exec_step_lagtest_mode();
  void set_grab_mouse(bool value);
public:
  vcnet_control(vcnet_config const& conf0);
  bool mainloop();
  bool mainloop_lagtest_mode();
};

vcnet_control::vcnet_control(vcnet_config const& conf0)
  : conf(conf0), conn(conf), audio(conf), wnd(conf)
{
  start_time = std::chrono::system_clock::now();
  last_draw_time = last_msg_time = last_hid_time = last_mouse_send_time =
    last_joystick_send_time = last_vfd_time = start_time;
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
          em.back().second = (int)strtol(e.c_str(), nullptr, 0);
        }
        ++k;
      }
      joymap[ek] = em;
    }
  }
  {
    hid_interval = (uint32_t)conf.get_uint("hid_interval", hid_interval);
    log(1, "hid_interval = %u\n", (unsigned)hid_interval);
    crop_mode = (conf.get_uint("crop", 0) != 0);
    uint32_t fmt = (uint32_t)conf.get_uint("video_format", 0);
    uint32_t fpslim = (uint32_t)conf.get_uint("fps_limit", 0);
    uint32_t f0 = 0;
    f0 |= (fmt & 0xff);
    f0 |= (fpslim & 0xff) << 8;
    conn.set_server_flags_mask(0, 8, 16, f0);
    conn.set_server_flags(1, crop_mode ? conn.get_cropx_val() : 0);
    conn.send_server_flags();
  }
  {
    bool v = (conf.get_uint("y_interpolation", 1) != 0);
    wnd.set_y_interpolation(v);
    show_gui = (conf.get_uint("show_gui", 0) != 0);
    show_gui_demo = (conf.get_uint("show_gui_demo", 0) != 0);
  }
  {
    auto const s = conn.get_title();
    wnd.set_title(s + " - vcnet");
  }
  pix.resize_if(wnd.get_width(), wnd.get_height());
  autoclose = (uint32_t)conf.get_uint("autoclose", 0);
  special_kmod = (uint32_t)conf.get_uint("special_kmod", special_kmod);
  log(0, "special_kmod=%x\n", (unsigned)special_kmod);
  special_button = (uint32_t)conf.get_uint("special_button", special_button);
  log(0, "special_button=%x\n", (unsigned)special_button);
  show_info = conf.get_uint("show_info", 0) != 0;
  gui_alpha = (float)conf.get_double("gui_alpha", 0.7);
  rtt_thr = (uint64_t)conf.get_uint("rtt_thr", 3000);
  cur_joystick = (uint32_t)conf.get_uint("joystick", 0);
  lagtest_mode = (uint32_t)conf.get_uint("lagtest_mode", 0);
  if (lagtest_mode != 0) {
    log(0, "lagtest_mode %u\n", (unsigned)lagtest_mode);
  }
}

void
vcnet_control::send_mouse_button_delayed(uint8_t btn, bool press)
{
  // 一定時間後にマウスボタンを操作する。
  auto now = std::chrono::system_clock::now();
  auto const interval_ms = 10; // TODO: 5msくらいまでは短くしても動く
  keycmd kc { };
  kc.code = btn;
  kc.mod = 0;
  kc.cmd = press ? 0 : 1;
  kc.time = now + std::chrono::milliseconds(interval_ms);
  delayed_keycmd.push_back(kc);
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
  uint8_t& c_r, uint8_t& m_r, bool& downup_r, bool& is_complex_conv_r)
{
  c_r = 0;
  m_r = 0;
  downup_r = false;
  is_complex_conv_r = false;
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
      is_complex_conv_r = (m_r != 0);
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
        is_complex_conv_r = (m_r != 0);
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
  bool is_complex_conv = false;
    // これがtrueのとき、keymapによる変換先にmodifierが含まれている。そのよう
    // なキーがdownされた後、何らかのキーがupされたときは、全キーをupされた
    // ように動作する。これは、複数キーのdownとupで順序がFILOになっていない
    // ときにも確実に全キーupされるようにするため。
  convert_scancode(kev.keysym.scancode, kev.keysym.sym, mod, conv_c, conv_m,
    downup, is_complex_conv);
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
        hid.key_down(conv_c, conv_m, is_complex_conv);
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
        hid.key_down(conv_c, conv_m, is_complex_conv);
      } else {
        #ifdef VCNET_MOBILE
        // androidでスクリーンキーボードによってEnterなどが押された場合、
        // downとupがほぼ同時にきてしまう。リモートに認識させるために
        // upの前に遅延を入れる。
        keycmd kc { };
        kc.code = conv_c;
        kc.mod = conv_m;
        kc.cmd = 0x05; // up
        auto now = std::chrono::system_clock::now();
        kc.time = now + std::chrono::milliseconds(20);
        delayed_keycmd.push_back(kc);
        #else
        hid.key_up(conv_c, conv_m);
        #endif
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
      hid.key_down(kc.code, kc.mod, kc.is_complex);
    } else if (kc.cmd == 5) {
      hid.key_up(kc.code, kc.mod);
    } else if (kc.cmd == 0) {
      hid.mouse_button(kc.code, true);
    } else if (kc.cmd == 1) {
      hid.mouse_button(kc.code, false);
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
vcnet_control::set_grab_mouse(bool value)
{
  if (show_gui || conn.get_absmouse() || conn.get_spi_index() < 0) {
    value = false;
  }
  if (grab_mouse != value) {
    if (value) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
      grab_mouse = true;
    } else {
      SDL_SetRelativeMouseMode(SDL_FALSE);
      grab_mouse = false;
    }
  }
  grab_mouse = value;
}

void
vcnet_control::set_show_gui(bool value)
{
  if (value == show_gui) {
    return;
  }
  #if 0
  if (wnd.has_mf_window() && wnd.get_fullscreen()) {
    // capture video有効でfullscreenのときguiをoffにすると表示が更新され
    // なくなる(原因不明)ので回避する。
    return;
  }
  #endif
  show_gui = value;
  set_grab_mouse(grab_mouse);
  wnd.set_show_mf_window(!value);
}

bool
vcnet_control::is_special_kmod_scancode(uint32_t scancode) const
{
  uint32_t m = 0;
  switch (scancode) {
  case SDL_SCANCODE_LSHIFT: m = KMOD_LSHIFT; break;
  case SDL_SCANCODE_LCTRL: m = KMOD_LCTRL; break;
  case SDL_SCANCODE_LALT: m = KMOD_LALT; break;
  case SDL_SCANCODE_LGUI: m = KMOD_LGUI; break;
  case SDL_SCANCODE_RSHIFT: m = KMOD_RSHIFT; break;
  case SDL_SCANCODE_RCTRL: m = KMOD_RCTRL; break;
  case SDL_SCANCODE_RALT: m = KMOD_RALT; break;
  case SDL_SCANCODE_RGUI: m = KMOD_RGUI; break;
  default: break;
  }
  return (m & special_kmod) != 0;
}

void
vcnet_control::handle_event(SDL_Event const& ev, bool& done_r)
{
  log(19, "handle event %d\n", (int)ev.type);
  bool ev_process_imgui = show_gui;
  if ((ev.type == SDL_MOUSEBUTTONUP || ev.type == SDL_MOUSEBUTTONDOWN) &&
    special_button_pressed) {
    ev_process_imgui = false;
  }
  if (ev_process_imgui && ImGui_ImplSDL2_ProcessEvent(&ev)) {
    if (show_gui) {
      switch (ev.type) {
      case SDL_MOUSEMOTION:
      case SDL_MOUSEWHEEL:
        return;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        switch (ev.button.button) {
        case SDL_BUTTON_LEFT:
        case SDL_BUTTON_RIGHT:
        case SDL_BUTTON_MIDDLE:
          return;
        default:
          break;
        }
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if ((ev.key.keysym.mod & special_kmod) == 0) {
          return;
        }
        break;
      default:
        break;
      }
    }
  }
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
    if (is_special_kmod_scancode(ev.key.keysym.scancode)) {
      // 右ALT: マウス捕獲を解除する。
      if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
        // 右ALTが押された。
        special_keyaction = 0;
        log(19, "special down\n");
      } else if (ev.type == SDL_KEYUP) {
        log(19, "special up %u\n", (unsigned)special_keyaction);
        // 右ALTが押されてから何も押されずに右ALTが離されたならマウスの
        // grabを解除する
        if (special_keyaction == 0 && grab_mouse) {
          log(19, "ungrab mouse\n");
          set_grab_mouse(false);
        }
      }
    } else if ((ev.key.keysym.mod & special_kmod) != 0) {
      if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
        // 右Altを押しながら何らかのキーが押された
        special_keyaction = ev.key.keysym.scancode;
          // 右ALTを押しながら別のキーが押されたことを記録する
        log(19, "special keyaction %u\n", (unsigned)special_keyaction);
        switch (ev.key.keysym.scancode) {
        case SDL_SCANCODE_RETURN:
          // 右ALT+RETURN: フルスクリーンにする・戻す。
          wnd.set_fullscreen(!wnd.get_fullscreen());
          break;
        case SDL_SCANCODE_F:
          // 右ALT+F: フルスクリーンのモードを切り替える
          {
            wnd.set_real_fullscreen(!wnd.get_real_fullscreen());
          }
          break;
        case SDL_SCANCODE_T:
          // 右ALT+T: test_adv7611.txtの内容をadv7611にセット。
          {
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
            }
          }
          break;
        case SDL_SCANCODE_DELETE:
          // 右ALT+DELETE: Ctrl+Alt+Delを送る。
          {
            log(19, "send ctrl-alt-del\n");
            // TODO:
            // conn.send_keyboard(0x4c, 0x05, 0x06);
          }
          break;
        case SDL_SCANCODE_V:
          // 右ALT+V: クリップボードの内容をUSBキーボードを使ってペーストする。
          paste_clipboard_text();
          break;
        case SDL_SCANCODE_I:
          // 右ALT+I: サーバ側でインタレース送信するかどうか切り替える。
          // TCPサーバのときのみ有効。
          {
            uint32_t v = conn.get_server_flags_mask(0, 0, 1);
            log(19, "server_flags0=%x\n", (unsigned)v);
            v ^= 0x01;
            conn.set_server_flags_mask(0, 0, 1, v);
            conn.send_server_flags();
          }
          break;
        case SDL_SCANCODE_Y:
          // 右ALT+F: 伝送フォーマットを切り替える(RGB24, YUV411)
          if (wnd.has_video_capture()) {
            #ifdef _MSC_VER
            auto const vdefs = wnd.get_video_devices();
            std::vector<std::string> subtypes;
            for (auto const& e: vdefs) {
              auto i = std::find(subtypes.begin(), subtypes.end(), e.subtype);
              if (i == subtypes.end()) {
                subtypes.push_back(e.subtype);
              }
            }
            auto const cur_vdev = wnd.get_video_attr();
            auto const cur_subtype = cur_vdev.subtype;
            auto i = std::find(subtypes.begin(), subtypes.end(), cur_subtype);
            if (i != subtypes.end()) {
              ++i;
            }
            if (i == subtypes.end()) {
              i = subtypes.begin();
            }
            if (i != subtypes.end()) {
              auto const next_subtype = *i;
              auto const j = std::find_if(vdefs.begin(), vdefs.end(),
                [&](mediafoundation_player::media_attribute const& x) {
                return x.subtype == next_subtype;
              });
              if (j != vdefs.end()) {
                wnd.set_capt_video_filter(*j);
                wnd.open_capture_device();
              }
            }
            #endif
          } else {
            uint32_t v = conn.get_server_flags_mask(0, 8, 8);
            log(19, "server_flags0=%x\n", (unsigned)v);
            v ^= 0x02;
            conn.set_server_flags_mask(0, 8, 8, v);
            conn.send_server_flags();
          }
          break;
        #if 1
        case SDL_SCANCODE_P:
          // 右ALT+P: 補間方式の切り替え
          {
            wnd.set_y_interpolation(!wnd.get_y_interpolation());
          }
          break;
        #endif
        case SDL_SCANCODE_J:
          {
            cur_joystick += 1;
            if (static_cast<int>(cur_joystick) >= SDL_NumJoysticks()) {
              cur_joystick = 0;
            }
            if (static_cast<int>(cur_joystick) < SDL_NumJoysticks()) {
              joy.open(cur_joystick);
              joystick_name = std::string(SDL_JoystickNameForIndex(
                cur_joystick));
            } else {
              joy.close();
            }
          }
          break;
        case SDL_SCANCODE_S:
          // 右ALT+S: 左上の詳細情報を表示・非表示にする。何回も押すと色が
          // 変わる。
          {
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
          break;
        case SDL_SCANCODE_C:
          // cropするかどうか切り替える
          {
            crop_mode = !crop_mode;
            uint32_t cropx_val = crop_mode ? conn.get_cropx_val() : 0;
            conn.set_server_flags(1, cropx_val);
            conn.send_server_flags();
          }
          break;
        case SDL_SCANCODE_G:
          set_show_gui(!show_gui);
          break;
        case SDL_SCANCODE_A:
          conn.set_absmouse(!conn.get_absmouse());
          break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_1:
        case SDL_SCANCODE_2:
        case SDL_SCANCODE_3:
        case SDL_SCANCODE_4:
        case SDL_SCANCODE_5:
          // 右ALT+RIGHT, 右ALT+LEFT: デバイスを切り替える
          // 右ALT+UP: 現在のデバイスへ再接続する
          // 右ALT+1から5: 0番から4番のデバイスへ切り替える
          {
            hid.up_all();
            clear_delayed_keycmd();
            auto const num = conn.get_num_devices();
            auto idx = conn.get_cur_device();
            auto const scancode = ev.key.keysym.scancode;
            if (scancode == SDL_SCANCODE_RIGHT) {
              idx = (idx + 1 < num) ? (idx + 1) : 0;
            } else if (scancode == SDL_SCANCODE_LEFT) {
              idx = (idx > 0) ? (idx - 1) : (num - 1);
            } else if (scancode >= SDL_SCANCODE_1 &&
              scancode <= SDL_SCANCODE_5) {
              idx = (scancode - SDL_SCANCODE_1) % num;
            }
            conn.set_cur_device(idx);
            auto const s = conn.get_title();
            wnd.set_title(s + " - vcnet");
            conn.set_server_flags(1, crop_mode ? conn.get_cropx_val() : 0);
            conn.send_server_flags();
          }
          break;
        case SDL_SCANCODE_F12:
          conn.set_gpio_out(0);
          break;
        default:
          break;
        }
      } else if (ev.type == SDL_KEYUP) {
        switch (ev.key.keysym.scancode) {
        case SDL_SCANCODE_F12:
          conn.set_gpio_out(1);
          break;
        default:
          break;
        }
      }
      return;
    } else {
      #if 0
      #ifdef __ANDROID__
      if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0 &&
        ev.key.keysym.scancode == 0x10e) {
        log(0, "toggletextinput keydown\n");
        if (wnd.is_screen_keyboard_shown()) {
          SDL_StopTextInput();
          log(0, "stoptextinput\n");
        } else {
          SDL_StartTextInput();
          log(0, "starttextinput\n");
        }
        return;
      }
      #endif
      #endif
      handle_keyboard(ev.key, (ev.type == SDL_KEYDOWN));
    }
    break;
  case SDL_WINDOWEVENT:
    {
      if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        wnd.disable_ime();
        last_suppress_frames_time = std::chrono::system_clock::now();
        #if 0
        if (!conn.get_absmouse() && conn.get_spi_index() >= 0 && !show_gui) {
          set_grab_mouse(true);
        }
        #endif
        special_keyaction = 0;
        prof.reset();
      } else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        set_grab_mouse(false);
        hid.up_all();
        clear_delayed_keycmd();
        special_keyaction = 0;
        special_button_pressed = false;
      }
      wnd.resize_window_if();
    }
    break;
  #ifndef VCNET_MOBILE
  case SDL_MOUSEMOTION:
    if (conn.get_absmouse()) {
      uint32_t const mouse_x = (uint32_t)ev.motion.x;
      uint32_t const mouse_y = (uint32_t)ev.motion.y;
      SDL_Rect rect = wnd.get_cur_draw_rect();
      log(19, "absmouse rect=(%u %u %u %u) mouse=(%u %u)\n",
        rect.x, rect.y, rect.w, rect.h, mouse_x, mouse_y);
      if (rect.w > 0 && rect.h > 0 &&
        rect.x <= (int)mouse_x && rect.x + rect.w >= (int)mouse_x &&
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
      #if 0
      wnd.warp_mouse(0, 0);
      #endif
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    {
      auto const& mb = ev.button;
      log(19, "mousedown button=%d\n", mb.button);
      uint32_t const button_state = SDL_GetMouseState(nullptr, nullptr);
      if (!grab_mouse && !conn.get_absmouse() && conn.get_spi_index() >= 0) {
        if (ev.type == SDL_MOUSEBUTTONDOWN &&
          mb.button == SDL_BUTTON_LEFT) {
          // 左クリック: マウスが絶対値モードではないときは、マウスを捕獲する。
          log(19, "grab mouse\n");
          set_grab_mouse(true);
        }
      } else if (mb.button == special_button) {
        if (ev.type == SDL_MOUSEBUTTONDOWN) {
          // special_buttonのdown。
          special_button_pressed = true;
        } else if (special_button_pressed) {
          // special_button単独クリック: マウス捕獲を解除する。
          set_grab_mouse(false);
        }
      } else if (special_button_pressed &&
        (button_state & SDL_BUTTON(special_button)) != 0) {
        // special_buttonを押しながら別のマウスボタンを押したときの動作。
        // マウスがwindowを離れてもspecial_button_pressedはtrueのままに
        // なるので、button_stateも見てspecial_buttonが押されていること
        // を確認している。
        if (mb.button == SDL_BUTTON_LEFT && ev.type == SDL_MOUSEBUTTONUP) {
          set_show_gui(!show_gui);
          special_button_pressed = false;
        }
        if (mb.button == SDL_BUTTON_RIGHT && ev.type == SDL_MOUSEBUTTONUP) {
          show_info = !show_info;
          special_button_pressed = false;
        }
      } else {
        uint8_t b = (mb.button - SDL_BUTTON_LEFT);
        b = (b == 1) ? 2 : (b == 2) ? 1 : b; // swap 1 and 2
        if (b < 8) {
          if (conn.get_absmousebutton()) {
            hid.absmouse_button(b, (ev.type == SDL_MOUSEBUTTONDOWN));
          } else {
            hid.mouse_button(b, (ev.type == SDL_MOUSEBUTTONDOWN));
          }
        }
      }
    }
    break;
  case SDL_MOUSEWHEEL:
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
  #endif // !VCNET_MOBILE
  #ifdef VCNET_MOBILE
  case SDL_FINGERDOWN:
    {
      log(20, "fingerdown fingerid=%d x=%f y=%f\n", (int)ev.tfinger.fingerId,
        (double)ev.tfinger.x, (double)ev.tfinger.y);
      cur_fingers.insert(ev.tfinger.fingerId);
      last_fingerdown = (int)cur_fingers.size();
      last_fingerdown_timestamp = ev.tfinger.timestamp;
      last_fingermotion = -1;
    }
    break;
  case SDL_FINGERUP:
    {
      log(20, "fingerup fingerid=%d x=%f y=%f last=%d m=%d\n",
        (int)ev.tfinger.fingerId,
        (double)ev.tfinger.x, (double)ev.tfinger.y, last_fingerdown,
        last_fingermotion);
      int finger = (int)cur_fingers.size();
      cur_fingers.clear();
      if (last_fingermotion == -1) {
        if (finger == 3) {
          log(20, "toggletextinput keydown\n");
          if (wnd.is_screen_keyboard_shown()) {
            SDL_StopTextInput();
            log(20, "stoptextinput\n");
          } else {
            SDL_StartTextInput();
            log(20, "starttextinput\n");
          }
        } else if (finger == 4) {
          log(20, "finger: toggle gui\n");
          set_show_gui(!show_gui);
        } else if (finger == 1 || finger == 2) {
          if (!show_gui) {
            log(20, "finger: mousebutton click %d\n", finger - 1);
            // 0または1のマウスボタンクリック
            hid.mouse_button((uint8_t)(finger - 1), true);
            send_mouse_button_delayed((uint8_t)(finger - 1), false);
          }
        }
      } else if (last_fingermotion == 2) {
        // finger == 2 のmotionによりbutton 0が押されていることがある。
        hid.mouse_button(0, false);
      }
    }
    break;
  case SDL_FINGERMOTION:
    {
      int finger = (int)cur_fingers.size();
      int dx = round(ev.tfinger.dx * (double)wnd.get_width());
      int dy = round(ev.tfinger.dy * (double)wnd.get_height());
      log(20, "fingermotion %f %f %d %d id=%d(%d) f=%d\n",
        (double)ev.tfinger.dx, (double)ev.tfinger.dy, dx, dy,
        (int)ev.tfinger.fingerId, last_fingerdown, finger);
      if ((dx != 0 || dy != 0) && last_fingerdown != -1) {
        uint32_t tdiff = ev.tfinger.timestamp - last_fingerdown_timestamp;
        log(20, "tdiff %u\n", (unsigned)tdiff);
        if (std::abs(dx) >= 20 || std::abs(dy) >= 20 || tdiff > 300) {
          if (finger == 1) {
            if (!show_gui) {
              hid.mouse_move(dx, dy, 0, 0);
            }
            last_fingermotion = 1;
          } else if (finger == 2) {
            if (!show_gui) {
              hid.mouse_button(0, true);
              hid.mouse_button(1, false);
              hid.mouse_button(2, false);
              hid.mouse_move(dx, dy, 0, 0);
            }
            last_fingermotion = 2;
          } else if (finger == 3) {
            if (!show_gui) {
              hid.mouse_button(0, false);
              hid.mouse_button(1, false);
              hid.mouse_button(2, false);
              hid.mouse_move(0, 0, dx, -dy); // TODO: clamp?
            }
            last_fingermotion = 3;
          }
        }
      }
    }
    break;
  case SDL_TEXTINPUT:
    {
      std::string text(ev.text.text);
      log(21, "textinput %s\n", text.c_str());
      if (!show_gui) {
        send_text(text);
      }
      return;
    }
    break;
  #endif // VCNET_MOBILE
  default:
    break;
  }
}

void
vcnet_control::draw_gui()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  if (show_gui && show_gui_demo) {
    ImGui::ShowDemoWindow(&show_gui);
  }
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
  ImGui::SetNextWindowSize(ImVec2((float)wnd.get_width(), 0.0));
  bool open_flag = true;
  ImGuiWindowFlags window_flags = 0;
  window_flags |= ImGuiWindowFlags_NoDecoration;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, gui_alpha);
  ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 32.0f);
  ImGui::Begin("Text", &open_flag, window_flags);
  {
    if (cur_msg_highlight) {
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
    }
    ImGui::TextUnformatted(cur_msg.data(), cur_msg.data() + cur_msg.size());
    if (cur_msg_highlight) {
      ImGui::PopStyleColor(1);
    }
  }
  if (show_gui) {
    ImGui::Separator();
    if (ImGui::Button("Hide GUI (RightAlt + G)")) {
      show_gui = false;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
    ImGui::DragFloat("Alpha", &gui_alpha, 0.01f, 0.3f, 1.0f);
    ImGui::PopItemWidth();
    ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_None);
    if (ImGui::BeginTabItem("Status")) {
      std::string s;
      uint32_t video_locked = 0;
      uint32_t has_signal = 0;
      for (uint32_t i = 0; i < conn.get_num_concurrent_servers(); ++i) {
        vcnet_conn::device_info d;
        conn.get_device_info(i, conn.get_cur_device(), d);
        auto svrstat = conn.get_server_stat();
        if (i == conn.get_svridx_video()) {
          video_locked = (svrstat & 0x04) >> 2;
          has_signal = (svrstat & 0x08) >> 3;
        }
        s += "device: " + d.title + "\n";
        s += "  socktype=" + d.socktype_str + ",ip=" + d.addr_str + ",port=" +
          std::to_string(d.port) + "\n";
        s += "  spi=" + std::to_string(d.spi_index) + "\n";
        s += "  wol=" + d.wol + "\n";
        s += "  wol_br=" + d.wol_br + "\n";
        // s += "  ir=" + d.ir + "\n";
        s += "  absmouse=" + std::to_string((int)d.absmouse) +
          ",absmousebutton=" + std::to_string((int)d.absmousebutton) + "\n";
        s += "connection:\n";
        auto now = std::chrono::system_clock::now();
        s += "  last open: " +
          std::to_string(duration_ms(now, conn.get_last_open_time(i)))
          + "ms\n";
        s += "  last read: " +
          std::to_string(duration_ms(now, conn.get_last_read_time(i)))
          + "ms\n";
        if (d.socktype_str == "udp") {
          s += "  rtt: " + std::to_string(conn.get_net_roundtrip_time())
            + "us\n";
        }
      }
      if (wnd.has_video_capture()) {
        /* video capture */
        s += "video capture device:\n";
        s += "  format: " +
          std::string(vcnet_video_format_str(wnd.get_cur_video_format()))
          + "\n";
        s += "  frame rate: " + std::to_string(conn.get_fps()) + "fps\n";
      } else {
        /* video over ip */
        s += "video over ip:\n";
        s += "  locked: " + std::to_string(video_locked) + "\n";
        s += "  has signal: " + std::to_string(has_signal) + "\n";
        s += "  size: " + std::to_string(pix.get_width()) + "x" +
          std::to_string(pix.get_height()) + "\n";
        s += "  format: " +
          std::string(vcnet_video_format_str(pix.get_format())) + "\n";
        s += "  y_interpolation: " + std::to_string(wnd.get_y_interpolation())
          + "\n";
        s += "  frame rate: " + std::to_string(conn.get_fps()) + "fps ";
        s += (conn.get_server_flags(0) & 0x01)
          ? "interlaced\n" : "progressive\n";
        auto const p = conn.get_video_count_diff();
        s += "  motion: " + std::to_string(p.second) + "/"
          + std::to_string(p.first + p.second) + "\n";
        s += "  line drop: " + std::to_string(conn.get_drop_count()) + "\n";
      }
      if (wnd.has_audio_capture()) {
        s += "audio capture device:\n";
        s += "  frequency: " + std::to_string(wnd.get_audio_sps()) + "Hz\n";
        s += "  channels: " + std::to_string(wnd.get_audio_channels()) + "\n";
      } else {
        s += "audio over ip:\n";
        s += "  frequency: " + std::to_string(conn.get_audio_freq()) + "Hz\n";
        s += "  buffer: " + std::to_string((int)conn.get_audiobuf_min()) + ","
          + std::to_string((int)conn.get_audiobuf_max()) + "\n";
      }
      s += "display:\n";
      s += "  size: " + std::to_string(wnd.get_width()) + "x" +
        std::to_string(wnd.get_height()) + "\n";
      s += "  vsync mode: " + std::to_string(wnd.get_vsync_mode()) +
        (wnd.get_suppress_vsync_wait_saved() ? "(suppressed)\n" : "\n");
      s += "prof:\n";
      s += " avg: " + prof.sum_saved.to_string() + "\n";
      s += " mi:  " + prof.mi_saved.to_string() + "\n";
      s += " mx:  " + prof.mx_saved.to_string() + "\n";
      ImGui::TextUnformatted(s.c_str());
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Devices")) {
      uint32_t num = conn.get_num_devices();
      if (gui.device_idx < 0) {
        gui.device_idx = (int)conn.get_cur_device();
      }
      vcnet_conn::device_info d;
      conn.get_device_info(0, (uint32_t)gui.device_idx, d);
      if (ImGui::BeginCombo("Device", d.title.c_str(), 0)) {
        for (uint32_t i = 0; i < num; ++i) {
          if (!conn.get_device_info(0, i, d)) {
            continue;
          }
          bool const is_sel = (gui.device_idx == (int)i);
          if (ImGui::Selectable(d.title.c_str(), is_sel)) {
            gui.device_idx = (int)i;
          }
          if (is_sel) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      } else {
        if (!d.addr_str.empty()) {
          std::string s;
          s += "socktype=" + d.socktype_str + ",ip=" + d.addr_str + ",port=" +
            std::to_string(d.port) + "\n";
          s += "spi=" + std::to_string(d.spi_index) + "\n";
          s += "wol=" + d.wol + "\n";
          ImGui::TextUnformatted(s.c_str());
        }
      }
      bool const disable_btn = ((int)conn.get_cur_device() == gui.device_idx);
      if (disable_btn) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Switch")) {
        conn.set_cur_device((uint32_t)gui.device_idx);
      }
      if (disable_btn) {
        ImGui::EndDisabled();
      }
      ImGui::EndTabItem();
    }
    #ifdef _MSC_VER
    if (ImGui::BeginTabItem("Capture")) {
      typedef mediafoundation_player::media_attribute media_attribute;
      auto const to_str_v = [] (media_attribute const& x) {
        auto f = x.subtype;
        if (f == "16000000") { f = "RGB"; }
        return x.name + " " + f + " " + std::to_string(x.width) + "x"
          + std::to_string(x.height) + " (" + std::to_string(x.fps_numerator)
          + "/" + std::to_string(x.fps_denominator) + ")Hz";
      };
      auto const to_str_a = [] (media_attribute const& x) {
        auto f = x.subtype;
        if (f == "03000000") { f = "PCM"; }
        return x.name + " " + f;
      };
      auto const vdevs = wnd.get_video_devices();
      auto const adevs = wnd.get_audio_devices();
      auto const cur_vdev_str = to_str_v(wnd.get_video_attr());
      auto const cur_adev_str = to_str_a(wnd.get_audio_attr());
      long v_sel = -1;
      long a_sel = -1;
      if (ImGui::BeginCombo("Video", cur_vdev_str.c_str(), 0)) {
        for (size_t i = 0; i < vdevs.size(); ++i) {
          auto const& d = vdevs[i];
          auto const s = to_str_v(d);
          bool const is_sel = (s == cur_vdev_str);
          if (ImGui::Selectable(s.c_str(), is_sel)) {
            v_sel = static_cast<long>(i);
          }
          if (is_sel) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::BeginCombo("Audio", cur_adev_str.c_str(), 0)) {
        for (size_t i = 0; i < adevs.size(); ++i) {
          auto const& d = adevs[i];
          auto const s = to_str_a(d);
          bool const is_sel = (s == cur_vdev_str);
          if (ImGui::Selectable(s.c_str(), is_sel)) {
            a_sel = static_cast<long>(i);
          }
          if (is_sel) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (v_sel >= 0 || a_sel >= 0) {
        if (v_sel >= 0) {
          wnd.set_capt_video_filter(vdevs[v_sel]);
        }
        if (a_sel >= 0) {
          wnd.set_capt_audio_filter(adevs[a_sel]);
        }
        wnd.open_capture_device();
      }
      ImGui::EndTabItem();
    }
    #endif
    if (ImGui::BeginTabItem("Control")) {
      bool v = false;
      ImGui::Checkbox("Show status message (RightAlt + S)", &show_info);
      v = wnd.get_fullscreen();
      if (ImGui::Checkbox("Fullscreen (RightAlt + ENTER)", &v)) {
        wnd.set_fullscreen(v);
      }
      int vint0 = 0;
      int vint = 0;
      if (!wnd.has_video_capture()) {
        v = (bool)conn.get_server_flags_mask(0, 0, 1);
        if (ImGui::Checkbox("Interlaced (RightAlt + I)", &v)) {
          conn.set_server_flags_mask(0, 0, 1, (uint32_t)v);
          conn.send_server_flags();
        }
        vint0 = (int)conn.get_server_flags_mask(0, 8, 8);
        vint = vint0;
        ImGui::RadioButton("RGB", &vint, 0);
        ImGui::SameLine();
        ImGui::RadioButton("YUV411 (RightAlt + Y)", &vint, 2);
        if (vint != vint0) {
          conn.set_server_flags_mask(0, 8, 8, (uint32_t)vint);
          conn.send_server_flags();
        }
        v = wnd.get_y_interpolation();
        if (ImGui::Checkbox("Y-Interpolation (RightAlt + P)", &v)) {
          wnd.set_y_interpolation(v);
        }
      }
      vint0 = (int)wnd.get_vsync_mode();
      vint = vint0;
      ImGui::RadioButton("Nowait vsync", &vint, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Wait vsync", &vint, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Adaptive vsync", &vint, 2);
      if (vint != vint0) {
        wnd.set_vsync_mode((unsigned)vint);
      }
      if (ImGui::Button("Paste clipboard text (RightAlt + V)")) {
        paste_clipboard_text();
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Keyboard")) {
      ImGui::Checkbox("Left Ctrl ", &gui.mod_lctrl);
      ImGui::SameLine();
      ImGui::Checkbox("Left Shift ", &gui.mod_lshift);
      ImGui::SameLine();
      ImGui::Checkbox("Left Alt ", &gui.mod_lalt);
      ImGui::SameLine();
      ImGui::Checkbox("Left Gui ", &gui.mod_lgui);
      ImGui::Checkbox("Right Ctrl", &gui.mod_rctrl);
      ImGui::SameLine();
      ImGui::Checkbox("Right Shift", &gui.mod_rshift);
      ImGui::SameLine();
      ImGui::Checkbox("Right Alt", &gui.mod_ralt);
      ImGui::SameLine();
      ImGui::Checkbox("Right Gui", &gui.mod_rgui);
      static const char *const kitems[] = {
        /* https://usb.org/document-library/hid-usage-tables-14 */
        // "01 ErrorRollOver",
        // "02 POSTFail",
        // "03 ErrorUndefined",
        "04 a and A",
        "05 b and B",
        "06 c and C",
        "07 d and D",
        "08 e and E",
        "09 f and F",
        "0a g and G",
        "0b h and H",
        "0c i and I",
        "0d j and J",
        "0e k and K",
        "0f l and L",
        "10 m and M",
        "11 n and N",
        "12 o and O",
        "13 p and P",
        "14 q and Q",
        "15 r and R",
        "16 s and S",
        "17 t and T",
        "18 u and U",
        "19 v and V",
        "1a w and W",
        "1b x and X",
        "1c y and Y",
        "1d z and Z",
        "1e 1 and !",
        "1f 2 and @",
        "20 3 and #",
        "21 4 and $",
        "22 5 and %",
        "23 6 and ^",
        "24 7 and &",
        "25 8 and *",
        "26 9 and (",
        "27 0 and )",
        "28 Retern(ENTER)",
        "29 ESCAPE",
        "2a DELETE(Backspace)",
        "2b Tab",
        "2d - and (underscore)",
        "2e = and +",
        "2f [ and {",
        "30 ] and }",
        "31 \\ and |",
        "32 Non-US # and ~",
        "33 ; and :",
        "34 ' and \"",
        "35 Grave Accent and Tilde",
        "36 , and <",
        "37 . and >",
        "38 / and ?",
        "39 Caps Lock",
        "3a F1",
        "3b F2",
        "3c F3",
        "3d F4",
        "3e F5",
        "3f F6",
        "40 F7",
        "41 F8",
        "42 F9",
        "43 F10",
        "44 F11",
        "45 F12",
        "46 PrintScreen",
        "47 Scroll Lock",
        "48 Pause",
        "49 Insert",
        "4a Home",
        "4b PageUp",
        "4c Delete Forward",
        "4d End",
        "4e PageDown",
        "4f RightArrow",
        "50 LeftArrow",
        "51 DownArrow",
        "52 UpArrow",
        "53 Keypad Num Lock and Clear",
        "54 Keypad /",
        "55 Keypad *",
        "56 Keypad -",
        "57 Keypad |",
        "58 Keypad ENTER",
        "59 Keypad 1 and End",
        "5a Keypad 2 and Down Arrow",
        "5b Keypad 3 and PageDn",
        "5c Keypad 4 and Left Arrow",
        "5d Keypad 5",
        "5e Keypad 6 and Right Arrow",
        "5f Keypad 7 and Home",
        "60 Keypad 8 and Up Arrow",
        "61 Keypad 9 and PageUp",
        "62 Keypad 0 and Insert",
        "63 Keypad . and Delete",
        "64 Non-US \\ and |",
        "65 Application",
        "66 Power",
        "67 Keypad =",
        "68 F13",
        "69 F14",
        "6a F15",
        "6b F16",
        "6c F17",
        "6d F18",
        "6e F19",
        "6f F20",
        "70 F21",
        "71 F22",
        "72 F23",
        "73 F24",
        "74 Execute",
        "75 Help",
        "76 Menu",
        "77 Select",
        "78 Stop",
        "79 Again",
        "7a Undo",
        "7b Cut",
        "7c Copy",
        "7d Paste",
        "7e Find",
        "7f Mute",
        "80 Volume Up",
        "81 Volume Down",
        "82 Locking Caps Lock",
        "83 Locking Num Lock",
        "84 Locking Scroll Lock",
        "85 Comma",
        "86 Equal Sign",
        "87 International1",
        "88 International2",
        "89 International3",
        "8a International4",
        "8b International5",
        "8c International6",
        "8d International7",
        "8e International8",
        "8f International9",
        "90 LANG1",
        "91 LANG2",
        "92 LANG3",
        "93 LANG4",
        "94 LANG5",
        "95 LANG6",
        "96 LANG7",
        "97 LANG8",
        "98 LANG9",
        "99 Alternate Erase",
        "9a SysReq/Attention",
        "9b Cancel",
        "9c Clear",
        "9d Prior",
        "9e Return",
        "9f Separator",
        "a0 Out",
        "a1 Oper",
        "a2 Clear/Again",
        "a3 CrSel/Props",
        "a4 ExSel",
        "b0 Keypad 00",
        "b1 Keypad 000",
        "b2 Thousands Separator",
        "b3 Decimal Separator",
        "b4 Currency Unit",
        "b5 Currency Sub-unit",
        "b6 Keypad (",
        "b7 Keypad )",
        "b8 Keypad {",
        "b9 Keypad }",
        "ba Keypad Tab",
        "bb Keypad Backspace",
        "bc Keypad A",
        "bd Keypad B",
        "be Keypad C",
        "bf Keypad D",
        "c0 Keypad E",
        "c1 Keypad F",
        "c2 Keypad XOR",
        "c3 Keypad ^",
        "c4 Keypad %",
        "c5 Keypad <",
        "c6 Keypad >",
        "c7 Keypad &",
        "c8 Keypad &&",
        "c9 Keypad |",
        "ca Keypad ||",
        "cb Keypad :",
        "cc Keypad #",
        "cd Keypad Space",
        "ce Keypad @",
        "cf Keypad !",
        "d0 Keypad Memory Store",
        "d1 Keypad Memory Recall",
        "d2 Keypad Memory Clear",
        "d3 Keypad Memory Add",
        "d4 Keypad Memory Subtract",
        "d5 Keypad Memory Multiply",
        "d6 Keypad Memory Divide",
        "d7 Keypad +/-",
        "d8 Keypad Clear",
        "d9 Keypad Clear Entry",
        "da Keypad Binary",
        "db Keypad Octal",
        "dc Keypad Decimal",
        "dd Keypad Hexadecimal",
        "e0 LeftControl",
        "e1 LeftShift",
        "e2 LeftAlt",
        "e3 Left GUI",
        "e4 RightControl",
        "e5 RightShift",
        "e6 RightAlt",
        "e7 Right GUI",
      };
      constexpr unsigned nitems =
        (unsigned)(sizeof(kitems) / sizeof(kitems[0]));
      gui.keycode_idx %= nitems;
      if (ImGui::BeginCombo("Key", kitems[gui.keycode_idx], 0)) {
        for (unsigned i = 0; i < nitems; ++i) {
          bool is_sel = (gui.keycode_idx == i);
          if (ImGui::Selectable(kitems[i], is_sel)) {
            gui.keycode_idx = i;
          }
          if (is_sel) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::Button("Send")) {
        const char *sel = kitems[gui.keycode_idx];
        char hstr[3] = { };
        memcpy(hstr, sel, 2);
        uint8_t code = (uint8_t)strtoul(hstr, nullptr, 16);
        uint8_t mod = 0;
        if (gui.mod_lctrl) { mod |= 0x01; }
        if (gui.mod_lshift) { mod |= 0x02; }
        if (gui.mod_lalt) { mod |= 0x04; }
        if (gui.mod_lgui) { mod |= 0x08; }
        if (gui.mod_rctrl) { mod |= 0x10; }
        if (gui.mod_rshift) { mod |= 0x20; }
        if (gui.mod_ralt) { mod |= 0x40; }
        if (gui.mod_rgui) { mod |= 0x80; }
        hid.key_down(code, mod, false);
        /* 一定時間後にkey_upを実行する */
        keycmd kc { };
        kc.code = code;
        kc.mod = mod;
        kc.cmd = 0x05; // up
        auto now = std::chrono::system_clock::now();
        kc.time = now + std::chrono::milliseconds(100);
          // 100ms後に実行する。あまり短いとリモートデバイスのOSによっては
          // 認識されないことがある。
        delayed_keycmd.push_back(kc);
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Configuration")) {
      std::string const conf_text0 = conf.get_conf_text();
      if (gui.conf_text.empty()) {
        gui.conf_text = conf_text0;
      }
      bool mod = (gui.conf_text != conf_text0);
      if (!mod) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Reset")) {
        gui.conf_text = conf_text0;
      }
      ImGui::SameLine();
      int save_or_revert = 0;
      if (ImGui::Button("Save and restart")) {
        save_or_revert = 1;
      }
      if (!mod) {
        ImGui::EndDisabled();
      }
      ImGui::SameLine();
      bool is_user_pref = conf.get_user_pref();
      if (!is_user_pref) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Revert and restart")) {
        save_or_revert = 2;
      }
      if (!is_user_pref) {
        ImGui::EndDisabled();
      }
      if (save_or_revert != 0) {
        auto const fns = conf.get_filenames();
        if (!fns.empty()) {
          auto const fn = fns[fns.size() - 1];
          auto const pref_path = get_pref_path();
          auto const pref_fn = pref_path + fn;
          if (save_or_revert == 1) {
            if (save_file(pref_fn, gui.conf_text)) {
              gui.conf_saved = true;
            }
          } else if (save_or_revert == 2) {
            remove(pref_fn.c_str());
            gui.conf_saved = true;
          }
        }
      }
      ImGuiInputTextFlags flags = 0; // ImGuiInputTextFlags_ReadOnly;
      ImGui::InputTextMultiline("##source", &gui.conf_text,
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();
  ImGui::PopStyleVar(3);
  ImGui::Render();
}

void
vcnet_control::paste_clipboard_text()
{
  log(19, "paste clipboard text\n");
  char *txt = SDL_GetClipboardText();
  std::unique_ptr<char, void (*)(void *)> txt_del(txt,
    [](void *p) { SDL_free(p); });
  std::string stext(txt, strlen(txt));
  send_text(stext);
}

void
vcnet_control::send_hid_state()
{
  std::vector<uint8_t> buf;
  bool has_event = false;
  int spi_index = conn.get_spi_index();
  if (spi_index >= 0) {
    // log(0, "send_hid_state spi=%d\n", spi_index); // FIXME
    hid.get_spi_buffer(conn.get_spi_index(), buf, has_event);
    conn.send_raw_spi(buf.data(), buf.size(), has_event);
  }
}

void
vcnet_control::exec_step()
{
  if (send_delayed_keycmd()) {
    send_hid_state();
  }
  bool videoframe_done = false;
  bool has_net_event = false;
  conn.recv_data(pix, videoframe_done, has_net_event, audiobuf);
  if (wnd.capture_video_read_sample()) {
    videoframe_done = true;
      // ビデオのサンプルを受け取ったのでフレーム終了し描画する
  }
  prof.cur.mark(2);
  if (!has_net_event && !videoframe_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  prof.cur.mark(3);
  auto now = std::chrono::system_clock::now();
  if (duration_ms(now, last_draw_time) > 100 && pix.get_offset() == 0) {
    videoframe_done = true; // timeout
  }
  if (duration_ms(now, last_draw_time) > 500) {
    videoframe_done = true; // timeout
  }
  if (duration_ms(now, last_mouse_send_time) > (long)hid_interval) {
    // 既定では3ms間隔でHID mouse等の状態を送信する。
    last_mouse_send_time = now;
    if (!adv7611_cmds.empty()) {
      // adv7611のテスト用
      auto& v = adv7611_cmds.front();
      conn.send_adv7611_i2c(v[0] >> 1u, v[1], v[2]);
      log(20, "send adv7611 %02x %02x %02x\n", (unsigned)v[0],
        (unsigned)v[1], (unsigned)v[2]);
      adv7611_cmds.pop_front();
    }
    {
      int n = SDL_NumJoysticks();
      if (n != num_joysticks_saved) {
        if (n > static_cast<int>(cur_joystick)) {
          joy.open(cur_joystick);
          joystick_name = std::string(SDL_JoystickNameForIndex(cur_joystick));
        } else {
          joy.close();
          joystick_name = "";
        }
      }
      num_joysticks_saved = n;
      if (n > static_cast<int>(cur_joystick)) {
        joy.update(hid, joymap);
      }
    }
    {
      // HIDの状態を送信する
      send_hid_state();
    }
  }
  prof.cur.mark(4);
  if (videoframe_done) {
    {
      long vft = duration_ms(now, last_vfd_time);
      last_vfd_time = now;
      stat_vfd_min = std::min((uint32_t)vft, stat_vfd_min);
      stat_vfd_max = std::max((uint32_t)vft, stat_vfd_max);
      if (++stat_vfd_count > 60) {
        stat_vfd_min_saved = stat_vfd_min;
        stat_vfd_max_saved = stat_vfd_max;
        stat_vfd_min = (uint32_t)-1;
        stat_vfd_max = 0;
        stat_vfd_count = 0;
      }
    }
    std::string msg;
    {
      cur_msg_highlight = 0;
      auto svrstat = conn.get_server_stat();
      auto video_locked = svrstat & 0x04;
      auto has_signal = svrstat & 0x08;
      if (wnd.get_video_use_sample_reader()) {
        video_locked = true;
        has_signal = true;
      }
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
      } else if (duration_ms(now, last_msg_time) < 3000 && !show_info) {
        msg = std::to_string(wnd.get_texwidth()) + "x" +
          std::to_string(wnd.get_texheight()) + " " +
          vcnet_video_format_str(wnd.get_cur_video_format());
      }
      if (show_info) {
        if (!msg.empty()) {
          msg = "[" + msg + "] ";
        }
        auto const p = conn.get_video_count_diff();
        msg += std::to_string(conn.get_fps());
        if (wnd.has_video_capture()) {
          msg += "Hz";
        } else {
          msg += ((conn.get_server_flags(0) & 0x01) ? "i" : "p");
        }
        if (p.first != 0 || p.second != 0) {
          msg += " " + std::to_string(p.second) + "/"
            + std::to_string(p.first + p.second);
          if (p.first != 0) {
            cur_msg_highlight |= 1;
          }
        }
        msg += std::string(" ");
        std::string const keystatestr = hid.get_state_str();
        if (!keystatestr.empty()) {
          msg += keystatestr;
        } else {
          msg += "-";
        }
        msg += std::string(" ")
          + vcnet_video_format_str(wnd.get_cur_video_format());
        uint32_t const audio_freq = (wnd.get_audio_sps() != 0)
          ? wnd.get_audio_sps() : conn.get_audio_freq();
        msg += " " + std::to_string(wnd.get_texwidth())
           + "x" + std::to_string(wnd.get_texheight())
           + " " + std::to_string(audio_freq) + "Hz"
           + " SS:" + to_hexstr(svrstat)
           + " SF:" + to_hexstr(conn.get_server_flags(0))
           + " V:" + std::to_string(stat_vfd_min_saved)
           + "-" + std::to_string(stat_vfd_max_saved)
           + " A:" + std::to_string((int)conn.get_audiobuf_min())
           + "-" + std::to_string(conn.get_audiobuf_max())
           + " D:" + std::to_string(conn.get_drop_count())
           + " C:" + std::to_string(cur_joystick) + "(" + joystick_name + ")";
      }
      cur_msg = msg;
    }
    conn.increment_videoframe_count();
    bool suppress_vsync_wait = false;
    if (conn.get_net_roundtrip_time() >= rtt_thr) {
      // RTTが大きいとUDP受信バッファに溜まっている可能性がある。クライアント
      // がVSYNC待ち有効でディスプレイが60Hzの場合、遅延がずっと解消しないまま
      // になってしまうので、一時的にVSYNC待ちを無効化する。
      suppress_vsync_wait = true;
    }
    if (wnd.get_use_gl()) {
      bool imgui_draw = false;
      {
        if (show_gui || (!cur_msg.empty() && !wnd.has_mf_window())) {
          draw_gui();
          imgui_draw = true;
        }
      }
      wnd.draw_window_gl(pix, conn.get_video_signal_stable(), imgui_draw,
        !grab_mouse && wnd.get_fullscreen(), suppress_vsync_wait);
    } else {
      wnd.update_texture_sdl(pix);
      wnd.draw_window_sdl(pix, conn.get_video_signal_stable());
    }
    last_draw_time = now;
    {
      auto audio_freq = conn.get_audio_freq();
      if (audio_freq != 0) {
        audio.open_device_if(audio_freq, 2);
      }
    }
  } else {
  }
  // フォーカスが当たった直後はしばらくaudioフレームを再生キューに入れない
  // if (duration_ms(now, last_suppress_frames_time) > 1000) {
  if (!wnd.has_mf_window()) {
    // log(0, "audiobuf size=%zu\n", audiobuf.size());
    conn.increment_audio_sample_count((uint32_t)audiobuf.size());
    if (!audiobuf.empty()) {
      size_t abufsz = audio.queue_audio(
        (unsigned char const *)audiobuf.data(),
        audiobuf.size() * sizeof(uint32_t));
      // log(0, "abufsz %u\n", (unsigned)abufsz);
      conn.stat_audiobuf(static_cast<uint32_t>(abufsz));
      audiobuf.clear();
    }
  }
  {
    // キャプチャデバイスからsamplereaderで読んだサンプルを再生キューに入れる
    uint32_t abufsz = wnd.queue_audio_sample(audio);
      // 読み込まなかったときは0を返す
    if (abufsz != 0) {
      conn.stat_audiobuf(abufsz);
    }
  }
  prof.cur.mark(5);
  if (videoframe_done) {
    if (prof.cur.sum() > prof.mx.sum()) {
      prof.mx = prof.cur;
    }
    if (prof.mi.sum() == 0 || prof.mi.sum() > prof.cur.sum()) {
      prof.mi = prof.cur;
    }
    prof.sum += prof.cur;
    prof.cur.reset();
    if (++prof.count >= 100) {
      prof.sum /= 100;
      prof.mi_saved = prof.mi;
      prof.mx_saved = prof.mx;
      prof.sum_saved = prof.sum;
      prof.mi.reset();
      prof.mx.reset();
      prof.sum.reset();
      prof.count = 0;
    }
  }
}

bool
vcnet_control::mainloop_lagtest_mode()
{
  bool done = false;
  while (!done) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0 && !done) {
      if (ImGui_ImplSDL2_ProcessEvent(&ev)) {
        // continue;
      }
      switch (ev.type) {
      case SDL_QUIT:
        log(1, "SDL_QUIT\n");
        done = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if (ev.key.keysym.scancode == SDL_SCANCODE_LSHIFT) {
          lagtest_mode_color1 = (ev.type == SDL_KEYDOWN);
        }
        break;
      case SDL_WINDOWEVENT:
        wnd.resize_window_if();
        break;
      case SDL_MOUSEBUTTONUP:
      case SDL_FINGERUP:
        show_gui = true;
        break;
      default:
        break;
      }
    }
    if (show_gui) {
      draw_gui();
    }
    wnd.draw_window_lagtest(lagtest_mode, lagtest_mode_color1, show_gui);
  }
  return false;
}

bool
vcnet_control::mainloop()
{
  if (lagtest_mode != 0) {
    return mainloop_lagtest_mode();
  }

  prof.reset();

  bool quit_flag = true;
  wnd.resize_texture_if(1920, 1080);
  bool done = false;
  while (!done) {
    if (gui.conf_saved) {
      quit_flag = false;
      done = true;
      break;
    }
    #if 0
    {
      if (prof.cur.sum() > prof.mx.sum()) {
        prof.mx = prof.cur;
      }
      if (prof.mi.sum() == 0 || prof.mi.sum() > prof.cur.sum()) {
        prof.mi = prof.cur;
      }
      prof.sum += prof.cur;
      if (++prof.count >= 100) {
        prof.mi_saved = prof.mi;
        prof.mx_saved = prof.mx;
        prof.sum_saved = prof.sum;
        prof.mi.reset();
        prof.mx.reset();
        prof.sum.reset();
        prof.count = 0;
      }
    }
    #endif
    prof.cur.mark(0);
    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0 && !done) {
      handle_event(ev, done);
    }
    prof.cur.mark(1);
    #ifdef _MSC_VER
    if (timer_running) {
      KillTimer(GetActiveWindow(), timer_id);
      log(22, "killtimer\n");
      timer_running = false;
    }
    #endif
    exec_step();
    auto now = std::chrono::system_clock::now();
    if (autoclose != 0 && duration_ms(now, start_time) > (long)autoclose) {
      log(0, "autoclose\n");
      done = true;
    }
  }
  log(1, "quit_flag %d\n", (int)quit_flag);
  return !quit_flag;
}

}; // namespace vcnet

//////////////////////////////////////////////////////////////

#ifdef _MSC_VER
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  PWSTR pCmdLine, int nCmdShow)
#else
extern "C"
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
      conf_files.clear();
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
      for (int i = 1; i < argc; ++i) {
        conf_files.push_back(conv.to_bytes(std::wstring(argvw[i])));
      }
    }
  }
  {
    #if 1
    // TODO: 意味あるのか？
    BOOL v = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    log(0, "SetPriorityClass: %x\n", v == 0 ? (unsigned)GetLastError() : 0);
    v = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    log(0, "SetThreadPriority: %x\n", v == 0 ? (unsigned)GetLastError() : 0);
    log(0, "main thread: %llx\n", (unsigned long long)GetCurrentThread());
    #endif
  }
  #else
  signal(SIGPIPE, SIG_IGN);
  cmdname = std::string(argv[0]);
  if (argc >= 2) {
    for (int i = 1; i < argc; ++i) {
      conf_files.push_back(std::string(argv[i]));
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
  vcnet_config conf(conf_files);
  for (size_t i = 0; i < conf_files.size(); ++i) {
    log(1, "config '%s'\n", conf_files[i].c_str());
  }
  {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    if (conf.get_uint("screensaver", 0) != 0) {
      SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    }
    Uint32 f = 0;
    f |= SDL_INIT_VIDEO;
    f |= SDL_INIT_AUDIO;
    f |= SDL_INIT_JOYSTICK;
    if (SDL_Init(f) != 0) {
      log(0, "error initializing SDL: %s\n", SDL_GetError());
    }
  }
  while (true) {
    vcnet_control ctrl(conf);
    if (!ctrl.mainloop()) {
      break;
    }
  }
  return 0;
}

