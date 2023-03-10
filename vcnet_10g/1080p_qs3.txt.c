{ 0x98, 0xFF, 0x80 }, // 98 FF 80 ; I2C reset
{ 0x98, 0xF4, 0x80 }, // 98 F4 80 ; CEC
{ 0x98, 0xF5, 0x7C }, // 98 F5 7C ; INFOFRAME
{ 0x98, 0xF8, 0x4C }, // 98 F8 4C ; DPLL
{ 0x98, 0xF9, 0x64 }, // 98 F9 64 ; KSV
{ 0x98, 0xFA, 0x6C }, // 98 FA 6C ; EDID
{ 0x98, 0xFB, 0x68 }, // 98 FB 68 ; HDMI
{ 0x98, 0xFD, 0x44 }, // 98 FD 44 ; CP
{ 0x98, 0x03, 0x40 }, // 98 03 40 ; 24 bit SDR 444 Mode 0
{ 0x98, 0x05, 0x28 }, // 98 05 28 ; AV Codes Off
{ 0x98, 0x06, 0xA6 }, // 98 06 A6 ; Invert VS,HS pins
{ 0x98, 0x0B, 0x44 }, // 98 0B 44 ; Power up part
{ 0x98, 0x0C, 0x42 }, // 98 0C 42 ; Power up part
{ 0x98, 0x14, 0x7F }, // 98 14 7F ; Max Drive Strength
{ 0x98, 0x15, 0x80 }, // 98 15 80 ; Disable Tristate of Pins
{ 0x98, 0x19, 0x83 }, // 98 19 83 ; LLC DLL phase
{ 0x98, 0x33, 0x40 }, // 98 33 40 ; LLC DLL enable
{ 0x44, 0xBA, 0x01 }, // 44 BA 01 ; Set HDMI FreeRun
{ 0x64, 0x40, 0x81 }, // 64 40 81 ; Disable HDCP 1.1 features
{ 0x68, 0x9B, 0x03 }, // 68 9B 03 ; ADI recommended setting
{ 0x68, 0xC1, 0x01 }, // 68 C1 01 ; ADI recommended setting
{ 0x68, 0xC2, 0x01 }, // 68 C2 01 ; ADI recommended setting
{ 0x68, 0xC3, 0x01 }, // 68 C3 01 ; ADI recommended setting
{ 0x68, 0xC4, 0x01 }, // 68 C4 01 ; ADI recommended setting
{ 0x68, 0xC5, 0x01 }, // 68 C5 01 ; ADI recommended setting
{ 0x68, 0xC6, 0x01 }, // 68 C6 01 ; ADI recommended setting
{ 0x68, 0xC7, 0x01 }, // 68 C7 01 ; ADI recommended setting
{ 0x68, 0xC8, 0x01 }, // 68 C8 01 ; ADI recommended setting
{ 0x68, 0xC9, 0x01 }, // 68 C9 01 ; ADI recommended setting
{ 0x68, 0xCA, 0x01 }, // 68 CA 01 ; ADI recommended setting
{ 0x68, 0xCB, 0x01 }, // 68 CB 01 ; ADI recommended setting
{ 0x68, 0xCC, 0x01 }, // 68 CC 01 ; ADI recommended setting
{ 0x68, 0x00, 0x00 }, // 68 00 00 ; Set HDMI Input Port A
{ 0x68, 0x83, 0xFE }, // 68 83 FE ; Enable clock terminator for port A
{ 0x68, 0x6F, 0x0C }, // 68 6F 0C ; ADI recommended setting
{ 0x68, 0x85, 0x1F }, // 68 85 1F ; ADI recommended setting
{ 0x68, 0x87, 0x70 }, // 68 87 70 ; ADI recommended setting
{ 0x68, 0x8D, 0x04 }, // 68 8D 04 ; LFG
{ 0x68, 0x8E, 0x1E }, // 68 8E 1E ; HFG
{ 0x68, 0x1A, 0x8A }, // 68 1A 8A ; unmute audio
{ 0x68, 0x57, 0xDA }, // 68 57 DA ; ADI recommended setting
{ 0x68, 0x58, 0x01 }, // 68 58 01 ; ADI recommended setting
{ 0x68, 0x03, 0x98 }, // 68 03 98 ; DIS_I2C_ZERO_COMPR
{ 0x68, 0x75, 0x10 }, // 68 75 10 ; DDC drive strength
{ 0x72, 0x01, 0x00 }, // 72 01 00 ; Set N Value(6144)
{ 0x72, 0x02, 0x18 }, // 72 02 18 ; Set N Value(6144)
{ 0x72, 0x03, 0x00 }, // 72 03 00 ; Set N Value(6144)
{ 0x72, 0x15, 0x00 }, // 72 15 00 ; Input 444 (RGB or YCrCb) with Separate Syncs, 44.1kHz fs
{ 0x72, 0x16, 0x70 }, // 72 16 70 ; Output format 444, 24-bit input
{ 0x72, 0x18, 0x46 }, // 72 18 46 ; CSC disabled
{ 0x72, 0x40, 0x80 }, // 72 40 80 ; General Control packet enable
{ 0x72, 0x41, 0x10 }, // 72 41 10 ; Power down control
{ 0x72, 0x48, 0x08 }, // 72 48 08 ; Data right justified
{ 0x72, 0x49, 0xA8 }, // 72 49 A8 ; Set Dither_mode - 12-to-10 bit
{ 0x72, 0x4C, 0x00 }, // 72 4C 00 ; 8 bit Output
{ 0x72, 0x55, 0x40 }, // 72 55 40 ; Set YCrCb 444 in AVinfo Frame
{ 0x72, 0x56, 0x08 }, // 72 56 08 ; Set active format Aspect
{ 0x72, 0x96, 0x20 }, // 72 96 20 ; HPD Interrupt clear
{ 0x72, 0x98, 0x03 }, // 72 98 03 ; ADI Recommended Write
{ 0x72, 0x99, 0x02 }, // 72 99 02 ; ADI Recommended Write
{ 0x72, 0x9C, 0x30 }, // 72 9C 30 ; PLL Filter R1 Value
{ 0x72, 0x9D, 0x61 }, // 72 9D 61 ; Set clock divide
{ 0x72, 0xA2, 0xA4 }, // 72 A2 A4 ; ADI Recommended Write
{ 0x72, 0xA3, 0xA4 }, // 72 A3 A4 ; ADI Recommended Write
{ 0x72, 0xA5, 0x04 }, // 72 A5 04 ; ADI Recommended Write
{ 0x72, 0xAB, 0x40 }, // 72 AB 40 ; ADI Recommended Write
{ 0x72, 0xAF, 0x16 }, // 72 AF 16 ; Set HDMI Mode
{ 0x72, 0xBA, 0x60 }, // 72 BA 60 ; No clock delay
{ 0x72, 0xD1, 0xFF }, // 72 D1 FF ; ADI Recommended Write
{ 0x72, 0xDE, 0xD8 }, // 72 DE D8 ; ADI Recommended Write
{ 0x72, 0xE4, 0x60 }, // 72 E4 60 ; VCO_Swing_Reference_Voltage
{ 0x72, 0xFA, 0x7D }, // 72 FA 7D ; Nbr of times to search for good phase
{ 0x64, 0x77, 0x00 }, // 64 77 00 ; Disable the Internal EDID
{ 0x64, 0x77, 0x00 }, // 64 77 00 ; Set the Most Significant Bit of the SPA location to 0
{ 0x64, 0x52, 0x20 }, // 64 52 20 ; Set the SPA for port B.
{ 0x64, 0x53, 0x00 }, // 64 53 00 ; Set the SPA for port B.
{ 0x64, 0x70, 0x9E }, // 64 70 9E ; Set the Least Significant Byte of the SPA location
{ 0x64, 0x74, 0x03 }, // 64 74 03 ; Enable the Internal EDID for Ports
