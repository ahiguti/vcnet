
module gmii_to_rgmii(
input CLK,
input RESET,
input [7:0] TX_D_IN,
input TX_CTRL_IN,
input TX_ERR_IN,
output [3:0] TX_D_OUT,
output TX_CTRL_OUT
);

//reg [7:0] d_reg;
//reg c_reg;
//reg e_reg;
//reg [3:0] d_po;
//reg [3:0] d_ne;
//reg c_po;
//reg c_ne;

//assign TX_D_OUT = d_po ^ d_ne;
//assign TX_CTRL_OUT = c_po ^ c_ne;

//always @(posedge CLK) begin
//    if (RESET) begin
//        d_reg <= 0;
//        c_reg <= 0;
//        e_reg <= 0;
//        d_po <= 0;
//        c_po <= 0;
//    end else begin
//        d_reg <= TX_D_IN;
//        c_reg <= TX_CTRL_IN;
//        e_reg <= TX_ERR_IN;
//        d_po <= d_ne ^ TX_D_IN[3:0];
//        c_po <= TX_CTRL_IN;
//    end
//end

//always @(negedge CLK) begin
//    if (RESET) begin
//        d_ne <= 0;
//        c_ne <= 0;
//    end else begin
//        d_ne <= d_po ^ d_reg[7:4];
//        c_ne <= 0;
//    end
//end

ODDRE1 oddr0(.Q(TX_D_OUT[0]), .C(CLK), .SR(RESET), .D1(TX_D_IN[0]), .D2(TX_D_IN[4]));
ODDRE1 oddr1(.Q(TX_D_OUT[1]), .C(CLK), .SR(RESET), .D1(TX_D_IN[1]), .D2(TX_D_IN[5]));
ODDRE1 oddr2(.Q(TX_D_OUT[2]), .C(CLK), .SR(RESET), .D1(TX_D_IN[2]), .D2(TX_D_IN[6]));
ODDRE1 oddr3(.Q(TX_D_OUT[3]), .C(CLK), .SR(RESET), .D1(TX_D_IN[3]), .D2(TX_D_IN[7]));

//assign TX_CTRL_OUT = TX_CTRL_IN;

ODDRE1 oddrc(.Q(TX_CTRL_OUT), .C(CLK), .SR(RESET), .D1(TX_CTRL_IN), .D2(TX_CTRL_IN));

endmodule
