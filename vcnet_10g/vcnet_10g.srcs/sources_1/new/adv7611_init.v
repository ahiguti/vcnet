
// Copyright (C) Akira Higuchi  ( https://github.com/ahiguti )
// Copyright (C) DeNA Co.,Ltd. ( https://dena.com )
// All rights reserved.
// See COPYRIGHT.txt for details


// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module adv7611_init(
input CLK,
input RESET_N,
input I2CM_READY,
output I2CM_VALID,
output [6:0] I2CM_DEVADDR,
output [7:0] I2CM_REGADDR,
output [31:0] I2CM_WDATA,
output [2:0] I2CM_WBYTES,
input [31:0] I2CM_RDATA,
output [2:0] I2CM_RBYTES,
output I2CM_SEND_NACK,
input [2:0] I2CM_ERR,
input I_TVALID,
output I_TREADY,
input [23:0] I_TDATA,
output [3:0] DEBUG_STATE,
output [8:0] DEBUG_IDX
);

reg [8:0] idx;
reg [3:0] state;
reg [31:0] delay_cnt;
reg i2cm_valid;
reg [7:0] i2cm_devaddr;
reg [7:0] i2cm_regaddr;
reg [7:0] i2cm_wdata;

assign I2CM_VALID = i2cm_valid;
assign I2CM_DEVADDR = i2cm_devaddr;
assign I2CM_REGADDR = i2cm_regaddr;
assign I2CM_WDATA = i2cm_wdata;
assign I2CM_WBYTES = 1;
assign I2CM_RBYTES = 0;
assign I2CM_SEND_NACK = 1;
assign DEBUG_STATE = state;
assign DEBUG_IDX = idx;
assign I_TREADY = ((state == 4) && I2CM_READY);

always @(posedge CLK) begin
    if (!RESET_N) begin
        state <= 0;
        delay_cnt <= 0;
        idx <= 0;
        i2cm_valid <= 0;
        i2cm_devaddr <= 0;
        i2cm_regaddr <= 0;
        i2cm_wdata <= 0;
    end else begin
        if (state == 0) begin
            if (delay_cnt < 1000_000) begin
                delay_cnt <= delay_cnt + 1;
            end else begin
                delay_cnt <= 0;
                state <= 1;
            end
        end
        if (state == 1 && I2CM_READY) begin
            i2cm_valid <= 1;
            i2cm_devaddr <= init_reg(idx) >> 17; // 7bit address
            i2cm_regaddr <= init_reg(idx) >> 8;
            i2cm_wdata <= init_reg(idx);
            state <= 2;
        end
        if (state == 2 && !I2CM_READY) begin
            i2cm_valid <= 0;
            if ((init_reg(idx + 1) >> 16) != 0) begin
                delay_cnt <= 0;
                idx <= idx + 1;
                state <= 0;
            end else begin
                state <= 3;
            end
        end
        if (state == 3) begin
            if (delay_cnt <= 1000_000) begin
                delay_cnt <= delay_cnt + 1;
            end else begin
                delay_cnt <= 0;
                state <= 4;
            end
        end
        if (state == 4 && I_TVALID && I_TREADY) begin
            i2cm_valid <= 1;
            i2cm_devaddr <= I_TDATA[6:0]; // 7bit address
            i2cm_regaddr <= I_TDATA[15:8];
            i2cm_wdata <= I_TDATA[23:16];
            state <= 5;
        end
        if (state == 5 && !I2CM_READY) begin
            i2cm_valid <= 0;
            state <= 3;
        end
    end
end

function [23:0] init_reg(input [8:0] idx);
begin
init_reg = 0;
case (idx)

// adv7611_7.txt.v
000: init_reg = 24'h98_FF_80; // 98 FF 80 ; I2C reset
001: init_reg = 24'h98_F4_80; // 98 F4 80 ; CEC
002: init_reg = 24'h98_F5_7C; // 98 F5 7C ; INFOFRAME
003: init_reg = 24'h98_F8_4C; // 98 F8 4C ; DPLL
004: init_reg = 24'h98_F9_64; // 98 F9 64 ; KSV
005: init_reg = 24'h98_FA_6C; // 98 FA 6C ; EDID
006: init_reg = 24'h98_FB_68; // 98 FB 68 ; HDMI
007: init_reg = 24'h98_FD_44; // 98 FD 44 ; CP
008: init_reg = 24'h98_00_01; // 98 00 01
009: init_reg = 24'h98_01_06; // 98 01 06
010: init_reg = 24'h98_02_F2; // 98 02 F2
011: init_reg = 24'h98_03_40; // 98 03 40 ; 24 bit SDR 444 Mode 0
012: init_reg = 24'h98_05_28; // 98 05 28 ; AV Codes Off
013: init_reg = 24'h98_06_A6; // 98 06 A6 ; Invert VS,HS pins
014: init_reg = 24'h98_0B_44; // 98 0B 44 ; Power up part
015: init_reg = 24'h98_0C_42; // 98 0C 42 ; Power up part
016: init_reg = 24'h98_14_7F; // 98 14 7F ; Max Drive Strength
017: init_reg = 24'h98_15_80; // 98 15 80 ; Disable Tristate of Pins
018: init_reg = 24'h98_19_83; // 98 19 83 ; LLC DLL phase
019: init_reg = 24'h98_33_40; // 98 33 40 ; LLC DLL enable
020: init_reg = 24'h44_BA_01; // 44 BA 01 ; Set HDMI FreeRun
021: init_reg = 24'h64_40_81; // 64 40 81 ; Disable HDCP 1.1 features
022: init_reg = 24'h68_9B_03; // 68 9B 03 ; ADI recommended setting
023: init_reg = 24'h68_C1_01; // 68 C1 01 ; ADI recommended setting
024: init_reg = 24'h68_C2_01; // 68 C2 01 ; ADI recommended setting
025: init_reg = 24'h68_C3_01; // 68 C3 01 ; ADI recommended setting
026: init_reg = 24'h68_C4_01; // 68 C4 01 ; ADI recommended setting
027: init_reg = 24'h68_C5_01; // 68 C5 01 ; ADI recommended setting
028: init_reg = 24'h68_C6_01; // 68 C6 01 ; ADI recommended setting
029: init_reg = 24'h68_C7_01; // 68 C7 01 ; ADI recommended setting
030: init_reg = 24'h68_C8_01; // 68 C8 01 ; ADI recommended setting
031: init_reg = 24'h68_C9_01; // 68 C9 01 ; ADI recommended setting
032: init_reg = 24'h68_CA_01; // 68 CA 01 ; ADI recommended setting
033: init_reg = 24'h68_CB_01; // 68 CB 01 ; ADI recommended setting
034: init_reg = 24'h68_CC_01; // 68 CC 01 ; ADI recommended setting
035: init_reg = 24'h68_00_00; // 68 00 00 ; Set HDMI Input Port A
036: init_reg = 24'h68_83_FE; // 68 83 FE ; Enable clock terminator for port A
037: init_reg = 24'h68_6F_0C; // 68 6F 0C ; ADI recommended setting
038: init_reg = 24'h68_85_1F; // 68 85 1F ; ADI recommended setting
039: init_reg = 24'h68_87_70; // 68 87 70 ; ADI recommended setting
040: init_reg = 24'h68_8D_04; // 68 8D 04 ; LFG
041: init_reg = 24'h68_8E_1E; // 68 8E 1E ; HFG
042: init_reg = 24'h68_1A_8A; // 68 1A 8A ; unmute audio
043: init_reg = 24'h68_57_DA; // 68 57 DA ; ADI recommended setting
044: init_reg = 24'h68_58_01; // 68 58 01 ; ADI recommended setting
045: init_reg = 24'h68_03_98; // 68 03 98 ; DIS_I2C_ZERO_COMPR
046: init_reg = 24'h68_75_10; // 68 75 10 ; DDC drive strength
047: init_reg = 24'h72_01_00; // 72 01 00 ; Set N Value(6144)
048: init_reg = 24'h72_02_18; // 72 02 18 ; Set N Value(6144)
049: init_reg = 24'h72_03_00; // 72 03 00 ; Set N Value(6144)
050: init_reg = 24'h72_15_00; // 72 15 00 ; Input 444 (RGB or YCrCb) with Separate Syncs, 44.1kHz fs
051: init_reg = 24'h72_16_70; // 72 16 70 ; Output format 444, 24-bit input
052: init_reg = 24'h72_18_46; // 72 18 46 ; CSC disabled
053: init_reg = 24'h72_40_80; // 72 40 80 ; General Control packet enable
054: init_reg = 24'h72_41_10; // 72 41 10 ; Power down control
055: init_reg = 24'h72_48_08; // 72 48 08 ; Data right justified
056: init_reg = 24'h72_49_A8; // 72 49 A8 ; Set Dither_mode - 12-to-10 bit
057: init_reg = 24'h72_4C_00; // 72 4C 00 ; 8 bit Output
058: init_reg = 24'h72_55_40; // 72 55 40 ; Set YCrCb 444 in AVinfo Frame
059: init_reg = 24'h72_56_08; // 72 56 08 ; Set active format Aspect
060: init_reg = 24'h72_96_20; // 72 96 20 ; HPD Interrupt clear
061: init_reg = 24'h72_98_03; // 72 98 03 ; ADI Recommended Write
062: init_reg = 24'h72_99_02; // 72 99 02 ; ADI Recommended Write
063: init_reg = 24'h72_9C_30; // 72 9C 30 ; PLL Filter R1 Value
064: init_reg = 24'h72_9D_61; // 72 9D 61 ; Set clock divide
065: init_reg = 24'h72_A2_A4; // 72 A2 A4 ; ADI Recommended Write
066: init_reg = 24'h72_A3_A4; // 72 A3 A4 ; ADI Recommended Write
067: init_reg = 24'h72_A5_04; // 72 A5 04 ; ADI Recommended Write
068: init_reg = 24'h72_AB_40; // 72 AB 40 ; ADI Recommended Write
069: init_reg = 24'h72_AF_16; // 72 AF 16 ; Set HDMI Mode
070: init_reg = 24'h72_BA_60; // 72 BA 60 ; No clock delay
071: init_reg = 24'h72_D1_FF; // 72 D1 FF ; ADI Recommended Write
072: init_reg = 24'h72_DE_D8; // 72 DE D8 ; ADI Recommended Write
073: init_reg = 24'h72_E4_60; // 72 E4 60 ; VCO_Swing_Reference_Voltage
074: init_reg = 24'h72_FA_7D; // 72 FA 7D ; Nbr of times to search for good phase
075: init_reg = 24'h64_77_00; // 64 77 00 ; Disable the Internal EDID
076: init_reg = 24'h6C_00_00; // 6C 00 00
077: init_reg = 24'h6C_01_FF; // 6C 01 FF
078: init_reg = 24'h6C_02_FF; // 6C 02 FF
079: init_reg = 24'h6C_03_FF; // 6C 03 FF
080: init_reg = 24'h6C_04_FF; // 6C 04 FF
081: init_reg = 24'h6C_05_FF; // 6C 05 FF
082: init_reg = 24'h6C_06_FF; // 6C 06 FF
083: init_reg = 24'h6C_07_00; // 6C 07 00
084: init_reg = 24'h6C_08_10; // 6C 08 10
085: init_reg = 24'h6C_09_EC; // 6C 09 EC
086: init_reg = 24'h6C_0A_01; // 6C 0A 01
087: init_reg = 24'h6C_0B_00; // 6C 0B 00
088: init_reg = 24'h6C_0C_01; // 6C 0C 01
089: init_reg = 24'h6C_0D_00; // 6C 0D 00
090: init_reg = 24'h6C_0E_00; // 6C 0E 00
091: init_reg = 24'h6C_0F_00; // 6C 0F 00
092: init_reg = 24'h6C_10_24; // 6C 10 24
093: init_reg = 24'h6C_11_1B; // 6C 11 1B
094: init_reg = 24'h6C_12_01; // 6C 12 01
095: init_reg = 24'h6C_13_03; // 6C 13 03
096: init_reg = 24'h6C_14_80; // 6C 14 80
097: init_reg = 24'h6C_15_33; // 6C 15 33
098: init_reg = 24'h6C_16_1D; // 6C 16 1D
099: init_reg = 24'h6C_17_78; // 6C 17 78
100: init_reg = 24'h6C_18_0A; // 6C 18 0A
101: init_reg = 24'h6C_19_6E; // 6C 19 6E
102: init_reg = 24'h6C_1A_A5; // 6C 1A A5
103: init_reg = 24'h6C_1B_A3; // 6C 1B A3
104: init_reg = 24'h6C_1C_54; // 6C 1C 54
105: init_reg = 24'h6C_1D_4F; // 6C 1D 4F
106: init_reg = 24'h6C_1E_9F; // 6C 1E 9F
107: init_reg = 24'h6C_1F_26; // 6C 1F 26
108: init_reg = 24'h6C_20_11; // 6C 20 11
109: init_reg = 24'h6C_21_50; // 6C 21 50
110: init_reg = 24'h6C_22_54; // 6C 22 54
111: init_reg = 24'h6C_23_21; // 6C 23 21
112: init_reg = 24'h6C_24_08; // 6C 24 08
113: init_reg = 24'h6C_25_00; // 6C 25 00
114: init_reg = 24'h6C_26_01; // 6C 26 01
115: init_reg = 24'h6C_27_01; // 6C 27 01
116: init_reg = 24'h6C_28_01; // 6C 28 01
117: init_reg = 24'h6C_29_01; // 6C 29 01
118: init_reg = 24'h6C_2A_01; // 6C 2A 01
119: init_reg = 24'h6C_2B_01; // 6C 2B 01
120: init_reg = 24'h6C_2C_01; // 6C 2C 01
121: init_reg = 24'h6C_2D_01; // 6C 2D 01
122: init_reg = 24'h6C_2E_01; // 6C 2E 01
123: init_reg = 24'h6C_2F_01; // 6C 2F 01
124: init_reg = 24'h6C_30_01; // 6C 30 01
125: init_reg = 24'h6C_31_01; // 6C 31 01
126: init_reg = 24'h6C_32_01; // 6C 32 01
127: init_reg = 24'h6C_33_01; // 6C 33 01
128: init_reg = 24'h6C_34_01; // 6C 34 01
129: init_reg = 24'h6C_35_01; // 6C 35 01
130: init_reg = 24'h6C_36_02; // 6C 36 02
131: init_reg = 24'h6C_37_3A; // 6C 37 3A
132: init_reg = 24'h6C_38_80; // 6C 38 80
133: init_reg = 24'h6C_39_18; // 6C 39 18
134: init_reg = 24'h6C_3A_71; // 6C 3A 71
135: init_reg = 24'h6C_3B_38; // 6C 3B 38
136: init_reg = 24'h6C_3C_2D; // 6C 3C 2D
137: init_reg = 24'h6C_3D_40; // 6C 3D 40
138: init_reg = 24'h6C_3E_58; // 6C 3E 58
139: init_reg = 24'h6C_3F_2C; // 6C 3F 2C
140: init_reg = 24'h6C_40_45; // 6C 40 45
141: init_reg = 24'h6C_41_00; // 6C 41 00
142: init_reg = 24'h6C_42_FE; // 6C 42 FE
143: init_reg = 24'h6C_43_22; // 6C 43 22
144: init_reg = 24'h6C_44_11; // 6C 44 11
145: init_reg = 24'h6C_45_00; // 6C 45 00
146: init_reg = 24'h6C_46_00; // 6C 46 00
147: init_reg = 24'h6C_47_1E; // 6C 47 1E
148: init_reg = 24'h6C_48_01; // 6C 48 01
149: init_reg = 24'h6C_49_1D; // 6C 49 1D
150: init_reg = 24'h6C_4A_80; // 6C 4A 80
151: init_reg = 24'h6C_4B_18; // 6C 4B 18
152: init_reg = 24'h6C_4C_71; // 6C 4C 71
153: init_reg = 24'h6C_4D_1C; // 6C 4D 1C
154: init_reg = 24'h6C_4E_16; // 6C 4E 16
155: init_reg = 24'h6C_4F_20; // 6C 4F 20
156: init_reg = 24'h6C_50_58; // 6C 50 58
157: init_reg = 24'h6C_51_2C; // 6C 51 2C
158: init_reg = 24'h6C_52_25; // 6C 52 25
159: init_reg = 24'h6C_53_00; // 6C 53 00
160: init_reg = 24'h6C_54_81; // 6C 54 81
161: init_reg = 24'h6C_55_49; // 6C 55 49
162: init_reg = 24'h6C_56_00; // 6C 56 00
163: init_reg = 24'h6C_57_00; // 6C 57 00
164: init_reg = 24'h6C_58_00; // 6C 58 00
165: init_reg = 24'h6C_59_9E; // 6C 59 9E
166: init_reg = 24'h6C_5A_00; // 6C 5A 00
167: init_reg = 24'h6C_5B_00; // 6C 5B 00
168: init_reg = 24'h6C_5C_00; // 6C 5C 00
169: init_reg = 24'h6C_5D_FC; // 6C 5D FC
170: init_reg = 24'h6C_5E_00; // 6C 5E 00
171: init_reg = 24'h6C_5F_56; // 6C 5F 56
172: init_reg = 24'h6C_60_43; // 6C 60 43
173: init_reg = 24'h6C_61_4E; // 6C 61 4E
174: init_reg = 24'h6C_62_45; // 6C 62 45
175: init_reg = 24'h6C_63_54; // 6C 63 54
176: init_reg = 24'h6C_64_2D; // 6C 64 2D
177: init_reg = 24'h6C_65_31; // 6C 65 31
178: init_reg = 24'h6C_66_30; // 6C 66 30
179: init_reg = 24'h6C_67_47; // 6C 67 47
180: init_reg = 24'h6C_68_0A; // 6C 68 0A
181: init_reg = 24'h6C_69_20; // 6C 69 20
182: init_reg = 24'h6C_6A_20; // 6C 6A 20
183: init_reg = 24'h6C_6B_20; // 6C 6B 20
184: init_reg = 24'h6C_6C_00; // 6C 6C 00
185: init_reg = 24'h6C_6D_00; // 6C 6D 00
186: init_reg = 24'h6C_6E_00; // 6C 6E 00
187: init_reg = 24'h6C_6F_FD; // 6C 6F FD
188: init_reg = 24'h6C_70_00; // 6C 70 00
189: init_reg = 24'h6C_71_14; // 6C 71 14
190: init_reg = 24'h6C_72_F0; // 6C 72 F0
191: init_reg = 24'h6C_73_0F; // 6C 73 0F
192: init_reg = 24'h6C_74_7E; // 6C 74 7E
193: init_reg = 24'h6C_75_10; // 6C 75 10
194: init_reg = 24'h6C_76_00; // 6C 76 00
195: init_reg = 24'h6C_77_0A; // 6C 77 0A
196: init_reg = 24'h6C_78_20; // 6C 78 20
197: init_reg = 24'h6C_79_20; // 6C 79 20
198: init_reg = 24'h6C_7A_20; // 6C 7A 20
199: init_reg = 24'h6C_7B_20; // 6C 7B 20
200: init_reg = 24'h6C_7C_20; // 6C 7C 20
201: init_reg = 24'h6C_7D_20; // 6C 7D 20
202: init_reg = 24'h6C_7E_01; // 6C 7E 01
203: init_reg = 24'h6C_7F_B7; // 6C 7F B7
204: init_reg = 24'h6C_80_02; // 6C 80 02
205: init_reg = 24'h6C_81_03; // 6C 81 03
206: init_reg = 24'h6C_82_1F; // 6C 82 1F
207: init_reg = 24'h6C_83_42; // 6C 83 42
208: init_reg = 24'h6C_84_47; // 6C 84 47
209: init_reg = 24'h6C_85_90; // 6C 85 90
210: init_reg = 24'h6C_86_04; // 6C 86 04
211: init_reg = 24'h6C_87_02; // 6C 87 02
212: init_reg = 24'h6C_88_01; // 6C 88 01
213: init_reg = 24'h6C_89_05; // 6C 89 05
214: init_reg = 24'h6C_8A_1F; // 6C 8A 1F
215: init_reg = 24'h6C_8B_13; // 6C 8B 13
216: init_reg = 24'h6C_8C_23; // 6C 8C 23
217: init_reg = 24'h6C_8D_0F; // 6C 8D 0F
218: init_reg = 24'h6C_8E_07; // 6C 8E 07
219: init_reg = 24'h6C_8F_07; // 6C 8F 07
220: init_reg = 24'h6C_90_83; // 6C 90 83
221: init_reg = 24'h6C_91_4F; // 6C 91 4F
222: init_reg = 24'h6C_92_00; // 6C 92 00
223: init_reg = 24'h6C_93_00; // 6C 93 00
224: init_reg = 24'h6C_94_67; // 6C 94 67
225: init_reg = 24'h6C_95_03; // 6C 95 03
226: init_reg = 24'h6C_96_0C; // 6C 96 0C
227: init_reg = 24'h6C_97_00; // 6C 97 00
228: init_reg = 24'h6C_98_10; // 6C 98 10
229: init_reg = 24'h6C_99_00; // 6C 99 00
230: init_reg = 24'h6C_9A_88; // 6C 9A 88
231: init_reg = 24'h6C_9B_2D; // 6C 9B 2D
232: init_reg = 24'h6C_9C_E2; // 6C 9C E2
233: init_reg = 24'h6C_9D_00; // 6C 9D 00
234: init_reg = 24'h6C_9E_40; // 6C 9E 40
235: init_reg = 24'h6C_9F_00; // 6C 9F 00
236: init_reg = 24'h6C_A0_00; // 6C A0 00
237: init_reg = 24'h6C_A1_00; // 6C A1 00
238: init_reg = 24'h6C_A2_00; // 6C A2 00
239: init_reg = 24'h6C_A3_00; // 6C A3 00
240: init_reg = 24'h6C_A4_00; // 6C A4 00
241: init_reg = 24'h6C_A5_00; // 6C A5 00
242: init_reg = 24'h6C_A6_00; // 6C A6 00
243: init_reg = 24'h6C_A7_00; // 6C A7 00
244: init_reg = 24'h6C_A8_00; // 6C A8 00
245: init_reg = 24'h6C_A9_00; // 6C A9 00
246: init_reg = 24'h6C_AA_00; // 6C AA 00
247: init_reg = 24'h6C_AB_00; // 6C AB 00
248: init_reg = 24'h6C_AC_00; // 6C AC 00
249: init_reg = 24'h6C_AD_00; // 6C AD 00
250: init_reg = 24'h6C_AE_00; // 6C AE 00
251: init_reg = 24'h6C_AF_00; // 6C AF 00
252: init_reg = 24'h6C_B0_00; // 6C B0 00
253: init_reg = 24'h6C_B1_00; // 6C B1 00
254: init_reg = 24'h6C_B2_00; // 6C B2 00
255: init_reg = 24'h6C_B3_00; // 6C B3 00
256: init_reg = 24'h6C_B4_00; // 6C B4 00
257: init_reg = 24'h6C_B5_00; // 6C B5 00
258: init_reg = 24'h6C_B6_00; // 6C B6 00
259: init_reg = 24'h6C_B7_00; // 6C B7 00
260: init_reg = 24'h6C_B8_00; // 6C B8 00
261: init_reg = 24'h6C_B9_00; // 6C B9 00
262: init_reg = 24'h6C_BA_00; // 6C BA 00
263: init_reg = 24'h6C_BB_00; // 6C BB 00
264: init_reg = 24'h6C_BC_00; // 6C BC 00
265: init_reg = 24'h6C_BD_00; // 6C BD 00
266: init_reg = 24'h6C_BE_00; // 6C BE 00
267: init_reg = 24'h6C_BF_00; // 6C BF 00
268: init_reg = 24'h6C_C0_00; // 6C C0 00
269: init_reg = 24'h6C_C1_00; // 6C C1 00
270: init_reg = 24'h6C_C2_00; // 6C C2 00
271: init_reg = 24'h6C_C3_00; // 6C C3 00
272: init_reg = 24'h6C_C4_00; // 6C C4 00
273: init_reg = 24'h6C_C5_00; // 6C C5 00
274: init_reg = 24'h6C_C6_00; // 6C C6 00
275: init_reg = 24'h6C_C7_00; // 6C C7 00
276: init_reg = 24'h6C_C8_00; // 6C C8 00
277: init_reg = 24'h6C_C9_00; // 6C C9 00
278: init_reg = 24'h6C_CA_00; // 6C CA 00
279: init_reg = 24'h6C_CB_00; // 6C CB 00
280: init_reg = 24'h6C_CC_00; // 6C CC 00
281: init_reg = 24'h6C_CD_00; // 6C CD 00
282: init_reg = 24'h6C_CE_00; // 6C CE 00
283: init_reg = 24'h6C_CF_00; // 6C CF 00
284: init_reg = 24'h6C_D0_00; // 6C D0 00
285: init_reg = 24'h6C_D1_00; // 6C D1 00
286: init_reg = 24'h6C_D2_00; // 6C D2 00
287: init_reg = 24'h6C_D3_00; // 6C D3 00
288: init_reg = 24'h6C_D4_00; // 6C D4 00
289: init_reg = 24'h6C_D5_00; // 6C D5 00
290: init_reg = 24'h6C_D6_00; // 6C D6 00
291: init_reg = 24'h6C_D7_00; // 6C D7 00
292: init_reg = 24'h6C_D8_00; // 6C D8 00
293: init_reg = 24'h6C_D9_00; // 6C D9 00
294: init_reg = 24'h6C_DA_00; // 6C DA 00
295: init_reg = 24'h6C_DB_00; // 6C DB 00
296: init_reg = 24'h6C_DC_00; // 6C DC 00
297: init_reg = 24'h6C_DD_00; // 6C DD 00
298: init_reg = 24'h6C_DE_00; // 6C DE 00
299: init_reg = 24'h6C_DF_00; // 6C DF 00
300: init_reg = 24'h6C_E0_00; // 6C E0 00
301: init_reg = 24'h6C_E1_00; // 6C E1 00
302: init_reg = 24'h6C_E2_00; // 6C E2 00
303: init_reg = 24'h6C_E3_00; // 6C E3 00
304: init_reg = 24'h6C_E4_00; // 6C E4 00
305: init_reg = 24'h6C_E5_00; // 6C E5 00
306: init_reg = 24'h6C_E6_00; // 6C E6 00
307: init_reg = 24'h6C_E7_00; // 6C E7 00
308: init_reg = 24'h6C_E8_00; // 6C E8 00
309: init_reg = 24'h6C_E9_00; // 6C E9 00
310: init_reg = 24'h6C_EA_00; // 6C EA 00
311: init_reg = 24'h6C_EB_00; // 6C EB 00
312: init_reg = 24'h6C_EC_00; // 6C EC 00
313: init_reg = 24'h6C_ED_00; // 6C ED 00
314: init_reg = 24'h6C_EE_00; // 6C EE 00
315: init_reg = 24'h6C_EF_00; // 6C EF 00
316: init_reg = 24'h6C_F0_00; // 6C F0 00
317: init_reg = 24'h6C_F1_00; // 6C F1 00
318: init_reg = 24'h6C_F2_00; // 6C F2 00
319: init_reg = 24'h6C_F3_00; // 6C F3 00
320: init_reg = 24'h6C_F4_00; // 6C F4 00
321: init_reg = 24'h6C_F5_00; // 6C F5 00
322: init_reg = 24'h6C_F6_00; // 6C F6 00
323: init_reg = 24'h6C_F7_00; // 6C F7 00
324: init_reg = 24'h6C_F8_00; // 6C F8 00
325: init_reg = 24'h6C_F9_00; // 6C F9 00
326: init_reg = 24'h6C_FA_00; // 6C FA 00
327: init_reg = 24'h6C_FB_00; // 6C FB 00
328: init_reg = 24'h6C_FC_00; // 6C FC 00
329: init_reg = 24'h6C_FD_00; // 6C FD 00
330: init_reg = 24'h6C_FE_00; // 6C FE 00
331: init_reg = 24'h6C_FF_16; // 6C FF 16
332: init_reg = 24'h64_77_00; // 64 77 00 ; Set the Most Significant Bit of the SPA location to 0
333: init_reg = 24'h64_52_20; // 64 52 20 ; Set the SPA for port B.
334: init_reg = 24'h64_53_00; // 64 53 00 ; Set the SPA for port B.
335: init_reg = 24'h64_70_9E; // 64 70 9E ; Set the Least Significant Byte of the SPA location
336: init_reg = 24'h64_74_03; // 64 74 03 ; Enable the Internal EDID for Ports

endcase
end
endfunction

endmodule

