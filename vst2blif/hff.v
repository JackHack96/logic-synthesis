entity hff is
port (
        d       :in bit ;
        ck      :in bit ;
        cln     :in bit ;
        prn     :in bit ;
        q       :out bit ;
        qn      :out bit) ;
end hff;

architecture structural of hff is

signal dn,a,an,b,bn,iq,iqn,qt,bt,bnt,iqd,c1:bit;

component nand2
port (a,b:in bit; o:out bit);
end component;

component inv1x
port (a:in bit; o:out bit);
end component;

component dlatch
port (CLOCK,D:in bit; Q:out bit);
end component;

component one
port (O:out bit);
end component;

begin
        u0:     inv1x
                port map (a=>d,o=>dn);
        u1:     nand2
                port map (a=>ck,b=>d,o=>a);
        u2:     nand2
                port map (a=>ck,b=>dn,o=>an);
        u3:     nand2
                port map (a=>a,b=>prn,o=>bt);
        u4:     nand2
                port map (a=>an,b=>cln,o=>bnt);
        u5:     nand2
                port map (a=>b,b=>iqn,o=>iq);
        u6:     nand2
                port map (a=>bn,b=>iqd,o=>iqn);
        u7:     inv1x
                port map (a=>iq,o=>qt);
        u8:     inv1x
                port map (a=>qt,o=>q);
        u9:     inv1x
                port map (a=>iqn,o=>qn);
        u10:    inv1x
                port map (a=>bt,o=>b);
        u11:    inv1x
                port map (a=>bnt,o=>bn);
        u12:    dlatch
                port map (CLOCK=>c1,Q=>iqd,D=>iq);
        u13:    one
                port map (O=>c1);
end structural;
