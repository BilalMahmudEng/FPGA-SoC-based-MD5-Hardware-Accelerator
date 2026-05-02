library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity md5_group_data is
    port (
        avs_s0_address  : in  std_logic_vector(3 downto 0) := (others => '0'); -- s0.address
        avs_s0_read     : in  std_logic := '0';                                  -- .read
        avs_s0_write    : in  std_logic := '0';                                  -- .write
        avs_s0_readdata : out std_logic_vector(31 downto 0);                     -- .readdata
        avs_s0_writedata: in  std_logic_vector(31 downto 0) := (others => '0'); -- s0.writedata
        clk             : in  std_logic := '0';                                  -- clock.clk
        reset           : in  std_logic := '0';                                  -- reset.reset
		  md5_group_writeaddr		: OUT std_logic_vector(8 DOWNTO 0)		:= (OTHERS => '0');
		  md5_group_writedata     : OUT std_logic_vector(31 DOWNTO 0)   	:= (OTHERS => '0');                		-- .md5_data
		  md5_group_readaddr			: OUT STD_LOGIC_VECTOR(6 DOWNTO 0);
		  md5_group_readdata		: IN std_logic_vector(31 DOWNTO 0)
    );
end entity md5_group_data;

architecture rtl of md5_group_data is
    signal in1 : std_logic_vector(31 downto 0);
	 signal in2 : std_logic_vector(31 downto 0);
	 signal in3 : std_logic_vector(31 downto 0);
begin
    PROCESS(clk, reset)
    BEGIN
        IF(reset = '1')THEN
            avs_s0_readdata <= (OTHERS => '0');
            in1 <= (OTHERS => '0');
				in2 <= (OTHERS => '0');
				in3 <= (OTHERS => '0');
        ELSIF(rising_edge(clk))THEN
            IF(avs_s0_read = '1')THEN
                CASE avs_s0_address IS
                    WHEN "0000" =>
                        avs_s0_readdata <= in1; -- md5_group_writedata
						  WHEN "0001" =>
                        avs_s0_readdata <= in2; -- md5_group_writeaddr
						  WHEN "0010" =>
                        avs_s0_readdata <= in3; -- md5_group_readaddr
						  WHEN "0011" =>
								avs_s0_readdata <= md5_group_readdata;
                    WHEN OTHERS =>
                        avs_s0_readdata <= (OTHERS => '0');
                END CASE;
            ELSIF(avs_s0_write = '1')THEN
                CASE avs_s0_address IS
                    WHEN "0000" =>
                        in1 <= avs_s0_writedata;
						  WHEN "0001" =>
                        in2 <= avs_s0_writedata;
						  WHEN "0010" =>
                        in3 <= avs_s0_writedata;
						  WHEN OTHERS =>
								
                END CASE;
            END IF;
        END IF;
    END PROCESS;

    md5_group_writedata <= in1;
	 md5_group_writeaddr <= in2(8 downto 0);
	 md5_group_readaddr <= in3(6 downto 0);

end architecture rtl;