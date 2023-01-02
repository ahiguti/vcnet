
module axis_wr_ring_buffer(
CLK, RESETN, BASE_ADDR,
I_TREADY, I_TVALID, I_TDATA,
BLK_TREADY, BLK_TVALID, BLK_ADDRESS, BLK_COUNT_M1, BLK_MODE_WRITE,
WR_TREADY, WR_TVALID, WR_TDATA,
RD_TREADY, RD_TVALID, RD_TDATA,
CUR_ADDR,
DEBUG_CUR_BLOCK, DEBUG_CUR_WORD
);

parameter WIDTH_ADDR = 32;
parameter WIDTH_DATA = 32;
parameter BUFFER_NUM_BLOCKS_L2 = 10;
parameter BLOCK_NUM_WORDS_L2 = 8;
localparam BUFFER_NUM_BLOCKS = 1 << BUFFER_NUM_BLOCKS_L2;
localparam BLOCK_NUM_WORDS = 1 << BLOCK_NUM_WORDS_L2;
localparam WORD_NUM_BYTES_L2 = $clog2(WIDTH_DATA / 8);

input CLK;
input RESETN;
input [WIDTH_ADDR-1:0] BASE_ADDR;
output I_TREADY;
input I_TVALID;
input [WIDTH_DATA-1:0] I_TDATA;
input BLK_TREADY;
output BLK_TVALID;
output [WIDTH_ADDR-1:0] BLK_ADDRESS;
output [WIDTH_ADDR-1:0] BLK_COUNT_M1;
output BLK_MODE_WRITE;
input WR_TREADY;
output WR_TVALID;
output [WIDTH_DATA-1:0] WR_TDATA;
output RD_TREADY;
input RD_TVALID;
input [WIDTH_DATA-1:0] RD_TDATA;
output [WIDTH_ADDR-1:0] CUR_ADDR;
output [BUFFER_NUM_BLOCKS_L2-1:0] DEBUG_CUR_BLOCK;
output [BLOCK_NUM_WORDS_L2-1:0] DEBUG_CUR_WORD;

reg blk_tvalid;
reg [BUFFER_NUM_BLOCKS_L2-1:0] cur_block;
reg wr_tvalid;
reg [BLOCK_NUM_WORDS_L2:0] cur_word;

assign I_TREADY = wr_tvalid;
assign BLK_TVALID = blk_tvalid;
assign BLK_ADDRESS = BASE_ADDR + (cur_block << (BLOCK_NUM_WORDS_L2 + WORD_NUM_BYTES_L2));
assign BLK_COUNT_M1 = BLOCK_NUM_WORDS - 1;
assign BLK_MODE_WRITE = 1;
assign WR_TVALID = I_TVALID && wr_tvalid;
assign WR_TDATA = I_TDATA;
assign RD_TREADY = 1;
assign CUR_ADDR = BASE_ADDR + (cur_block << (BLOCK_NUM_WORDS_L2 + WORD_NUM_BYTES_L2));
assign DEBUG_CUR_BLOCK = cur_block;
assign DEBUG_CUR_WORD = cur_word;

always @(posedge CLK) begin
    if (!RESETN) begin
        blk_tvalid <= 0;
        cur_block <= 0;
        wr_tvalid <= 0;
        cur_word <= 0;
    end else begin
        if (blk_tvalid == 0 && wr_tvalid == 0 && BASE_ADDR != 0) begin
            blk_tvalid <= 1;
            wr_tvalid <= 1;
        end
        if (BLK_TVALID && BLK_TREADY) begin
            blk_tvalid <= 0;
        end
        if (WR_TVALID && WR_TREADY) begin
            if (cur_word + 1 == BLOCK_NUM_WORDS) begin
                cur_block <= cur_block + 1;
                cur_word <= 0;
                wr_tvalid <= 0;
            end else begin
                cur_word <= cur_word + 1;
            end
        end
    end
end

endmodule
