
#define VCNET_SERVER_ADDR IPAddress(192, 168, 100, 244)
#define VCNET_SERVER_PORT 5001
#define VCNET_SERIAL Serial 

//#define VCNET_MCU_ID 127
//  // 制御対象MCUを区別する7bitの値。クライアントの設定項目spi_indexの値。
//  // これが定義されているとサーバ自体もUSBコントローラとして動作する。
// TODO: UnoR4でこれを定義してUSBを使うとEthernetがおかしくなる。何故？

#define VCNET_SERVER_VERBOSE 1
#define VCNET_USB_VERBOSE 0

#include <vcnet_server.h>
#include <bitbang_sender.h>
#ifdef VCNET_MCU_ID
  #include <vcnet_usb.h>
#endif

vcnet_server vcnet_server_inst(VCNET_SERVER_ADDR, VCNET_SERVER_PORT);
bitbang_4bit_sender bb4_sender;

#ifdef VCNET_MCU_ID
  enum { REG_WORDS = 32 };
  vcnet_usb vcnet_usb_inst;
#endif

void setup()
{
  VCNET_SERIAL.begin(115200);
  VCNET_SERIAL.println("vcnet server setup");
  vcnet_server_inst.begin(VCNET_SERVER_VERBOSE);
  bb4_sender.begin(3);
  #ifdef VCNET_MCU_ID
  vcnet_usb_inst.begin(VCNET_MCU_ID, VCNET_USB_VERBOSE);
  #endif
}

void loop()
{
  vcnet_server_inst.loop();
  uint8_t const *const arr = vcnet_server_inst.get_regarray();
  uint16_t const arrsz = vcnet_server_inst.get_regarray_size();
  bb4_sender.send(arr, arrsz);
  #ifdef VCNET_MCU_ID
  vcnet_usb_inst.loop((uint16_t *)arr, arrsz / sizeof(uint16_t));
  #endif
}
