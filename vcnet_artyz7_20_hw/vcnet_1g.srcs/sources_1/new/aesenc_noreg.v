
module aesenc_noreg(
    input CLK,
    input [127:0] IN_DATA,
    output [127:0] OUT_DATA
);

wire [127:0] subst;
generate
    genvar i;
    for (i = 0; i < 16; i = i + 1) begin
        rijndael_sbox sbox(.CLK(CLK), .IN_DATA(IN_DATA[i*8+7:i*8]),
            .OUT_DATA(subst[i*8+7:i*8]));
    end
endgenerate

function [127:0] shift_rows;
input [127:0] x;
integer i;
reg [7:0] s[0:15];
reg [7:0] r[0:15];
begin
    for (i = 0; i < 16; i = i + 1) begin
        s[i] = x[i*8 +: 8];
    end
    for (i = 0; i < 4; i = i + 1) begin
        r[i*4] = s[i*4];
        r[i*4+1] = s[((i+1)%4)*4+1];
        r[i*4+2] = s[((i+2)%4)*4+2];
        r[i*4+3] = s[((i+3)%4)*4+3];
    end
    for (i = 0; i < 16; i = i + 1) begin
        shift_rows[i*8 +: 8] = r[i];
    end
end
endfunction

wire [127:0] sr = shift_rows(subst);

function [7:0] lshift_mod;
input [7:0] x;
begin
    if (x[7]) begin
        lshift_mod = { x[6:0], 1'b0 } ^ 8'h1b;
    end else begin
        lshift_mod = { x[6:0], 1'b0 };
    end
end
endfunction

function [127:0] mix_columns;
input [127:0] x;
begin: blk0
    integer i;
    for (i = 0; i < 4; i = i + 1) begin: blk1
        integer j;
        reg [7:0] s[0:3];
        reg [7:0] ss[0:3];
        for (j = 0; j < 4; j = j + 1) begin
            s[j] = x[i*32+j*8 +: 8];
            ss[j] = lshift_mod(s[j]);
        end
        mix_columns[i*32    +: 8] = ss[0] ^ s[1] ^ ss[1] ^ s[2] ^ s[3];
        mix_columns[i*32+8  +: 8] = s[0] ^ ss[1] ^ s[2] ^ ss[2] ^ s[3];
        mix_columns[i*32+16 +: 8] = s[0] ^ s[1] ^ ss[2] ^ s[3] ^ ss[3];
        mix_columns[i*32+24 +: 8] = s[0] ^ ss[0] ^ s[1] ^ s[2] ^ ss[3];
    end
end
endfunction

assign OUT_DATA = mix_columns(sr);

endmodule
