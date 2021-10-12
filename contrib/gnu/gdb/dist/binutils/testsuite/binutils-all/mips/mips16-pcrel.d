#PROG: objcopy
#objdump: -d --prefix-addresses --show-raw-insn
#name: MIPS16 PC-relative instruction disassembly

# Verify delay-slot adjustment for PC-relative operations.

.*: +file format .*mips.*

Disassembly of section \.text:
00000000 <[^>]*> 6500      	nop
00000002 <[^>]*> 6500      	nop
00000004 <[^>]*> 0aff      	la	v0,00000400 <foo0\+0x400>
00000006 <[^>]*> 6500      	nop
00000008 <[^>]*> 6500      	nop
0000000a <[^>]*> 6500      	nop
0000000c <[^>]*> b3ff      	lw	v1,00000408 <foo0\+0x408>
0000000e <[^>]*> 6500      	nop
00000010 <[^>]*> 6500      	nop
00000012 <[^>]*> 6500      	nop
00000014 <[^>]*> fe9f      	dla	a0,00000090 <foo0\+0x90>
00000016 <[^>]*> 6500      	nop
00000018 <[^>]*> 6500      	nop
0000001a <[^>]*> 6500      	nop
0000001c <[^>]*> 6500      	nop
0000001e <[^>]*> 6500      	nop
00000020 <[^>]*> fcbf      	ld	a1,00000118 <foo0\+0x118>
	\.\.\.
00001000 <[^>]*> 1800 0000 	jal	00000000 <foo0>
00001004 <[^>]*> 0aff      	la	v0,000013fc <foo1\+0x3fc>
00001006 <[^>]*> 6500      	nop
00001008 <[^>]*> 1800 0000 	jal	00000000 <foo0>
0000100c <[^>]*> b3ff      	lw	v1,00001404 <foo1\+0x404>
0000100e <[^>]*> 6500      	nop
00001010 <[^>]*> 1800 0000 	jal	00000000 <foo0>
00001014 <[^>]*> fe9f      	dla	a0,0000108c <foo1\+0x8c>
00001016 <[^>]*> 6500      	nop
00001018 <[^>]*> 6500      	nop
0000101a <[^>]*> 6500      	nop
0000101c <[^>]*> 1800 0000 	jal	00000000 <foo0>
00001020 <[^>]*> fcbf      	ld	a1,00001110 <foo1\+0x110>
	\.\.\.
00002000 <[^>]*> 1c00 0000 	jalx	00000000 <foo0>
00002004 <[^>]*> 0aff      	la	v0,000023fc <foo2\+0x3fc>
00002006 <[^>]*> 6500      	nop
00002008 <[^>]*> 1c00 0000 	jalx	00000000 <foo0>
0000200c <[^>]*> b3ff      	lw	v1,00002404 <foo2\+0x404>
0000200e <[^>]*> 6500      	nop
00002010 <[^>]*> 1c00 0000 	jalx	00000000 <foo0>
00002014 <[^>]*> fe9f      	dla	a0,0000208c <foo2\+0x8c>
00002016 <[^>]*> 6500      	nop
00002018 <[^>]*> 6500      	nop
0000201a <[^>]*> 6500      	nop
0000201c <[^>]*> 1c00 0000 	jalx	00000000 <foo0>
00002020 <[^>]*> fcbf      	ld	a1,00002110 <foo2\+0x110>
	\.\.\.
00003000 <[^>]*> 6500      	nop
00003002 <[^>]*> e800      	jr	s0
00003004 <[^>]*> 0aff      	la	v0,000033fc <foo3\+0x3fc>
00003006 <[^>]*> 6500      	nop
00003008 <[^>]*> 6500      	nop
0000300a <[^>]*> e800      	jr	s0
0000300c <[^>]*> b3ff      	lw	v1,00003404 <foo3\+0x404>
0000300e <[^>]*> 6500      	nop
00003010 <[^>]*> 6500      	nop
00003012 <[^>]*> e800      	jr	s0
00003014 <[^>]*> fe9f      	dla	a0,0000308c <foo3\+0x8c>
00003016 <[^>]*> 6500      	nop
00003018 <[^>]*> 6500      	nop
0000301a <[^>]*> 6500      	nop
0000301c <[^>]*> 6500      	nop
0000301e <[^>]*> e800      	jr	s0
00003020 <[^>]*> fcbf      	ld	a1,00003110 <foo3\+0x110>
	\.\.\.
00004000 <[^>]*> 6500      	nop
00004002 <[^>]*> e820      	jr	ra
00004004 <[^>]*> 0aff      	la	v0,000043fc <foo4\+0x3fc>
00004006 <[^>]*> 6500      	nop
00004008 <[^>]*> 6500      	nop
0000400a <[^>]*> e820      	jr	ra
0000400c <[^>]*> b3ff      	lw	v1,00004404 <foo4\+0x404>
0000400e <[^>]*> 6500      	nop
00004010 <[^>]*> 6500      	nop
00004012 <[^>]*> e820      	jr	ra
00004014 <[^>]*> fe9f      	dla	a0,0000408c <foo4\+0x8c>
00004016 <[^>]*> 6500      	nop
00004018 <[^>]*> 6500      	nop
0000401a <[^>]*> 6500      	nop
0000401c <[^>]*> 6500      	nop
0000401e <[^>]*> e820      	jr	ra
00004020 <[^>]*> fcbf      	ld	a1,00004110 <foo4\+0x110>
	\.\.\.
00005000 <[^>]*> 6500      	nop
00005002 <[^>]*> e840      	jalr	s0
00005004 <[^>]*> 0aff      	la	v0,000053fc <foo5\+0x3fc>
00005006 <[^>]*> 6500      	nop
00005008 <[^>]*> 6500      	nop
0000500a <[^>]*> e840      	jalr	s0
0000500c <[^>]*> b3ff      	lw	v1,00005404 <foo5\+0x404>
0000500e <[^>]*> 6500      	nop
00005010 <[^>]*> 6500      	nop
00005012 <[^>]*> e840      	jalr	s0
00005014 <[^>]*> fe9f      	dla	a0,0000508c <foo5\+0x8c>
00005016 <[^>]*> 6500      	nop
00005018 <[^>]*> 6500      	nop
0000501a <[^>]*> 6500      	nop
0000501c <[^>]*> 6500      	nop
0000501e <[^>]*> e840      	jalr	s0
00005020 <[^>]*> fcbf      	ld	a1,00005110 <foo5\+0x110>
	\.\.\.
00006000 <[^>]*> 6500      	nop
00006002 <[^>]*> e860      	0xe860
00006004 <[^>]*> 0aff      	la	v0,00006400 <foo6\+0x400>
00006006 <[^>]*> 6500      	nop
00006008 <[^>]*> 6500      	nop
0000600a <[^>]*> e860      	0xe860
0000600c <[^>]*> b3ff      	lw	v1,00006408 <foo6\+0x408>
0000600e <[^>]*> 6500      	nop
00006010 <[^>]*> 6500      	nop
00006012 <[^>]*> e860      	0xe860
00006014 <[^>]*> fe9f      	dla	a0,00006090 <foo6\+0x90>
00006016 <[^>]*> 6500      	nop
00006018 <[^>]*> 6500      	nop
0000601a <[^>]*> 6500      	nop
0000601c <[^>]*> 6500      	nop
0000601e <[^>]*> e860      	0xe860
00006020 <[^>]*> fcbf      	ld	a1,00006118 <foo6\+0x118>
	\.\.\.
00007000 <[^>]*> 6500      	nop
00007002 <[^>]*> e880      	jrc	s0
00007004 <[^>]*> 0aff      	la	v0,00007400 <foo7\+0x400>
00007006 <[^>]*> 6500      	nop
00007008 <[^>]*> 6500      	nop
0000700a <[^>]*> e880      	jrc	s0
0000700c <[^>]*> b3ff      	lw	v1,00007408 <foo7\+0x408>
0000700e <[^>]*> 6500      	nop
00007010 <[^>]*> 6500      	nop
00007012 <[^>]*> e880      	jrc	s0
00007014 <[^>]*> fe9f      	dla	a0,00007090 <foo7\+0x90>
00007016 <[^>]*> 6500      	nop
00007018 <[^>]*> 6500      	nop
0000701a <[^>]*> 6500      	nop
0000701c <[^>]*> 6500      	nop
0000701e <[^>]*> e880      	jrc	s0
00007020 <[^>]*> fcbf      	ld	a1,00007118 <foo7\+0x118>
	\.\.\.
00008000 <[^>]*> 6500      	nop
00008002 <[^>]*> e8a0      	jrc	ra
00008004 <[^>]*> 0aff      	la	v0,00008400 <foo8\+0x400>
00008006 <[^>]*> 6500      	nop
00008008 <[^>]*> 6500      	nop
0000800a <[^>]*> e8a0      	jrc	ra
0000800c <[^>]*> b3ff      	lw	v1,00008408 <foo8\+0x408>
0000800e <[^>]*> 6500      	nop
00008010 <[^>]*> 6500      	nop
00008012 <[^>]*> e8a0      	jrc	ra
00008014 <[^>]*> fe9f      	dla	a0,00008090 <foo8\+0x90>
00008016 <[^>]*> 6500      	nop
00008018 <[^>]*> 6500      	nop
0000801a <[^>]*> 6500      	nop
0000801c <[^>]*> 6500      	nop
0000801e <[^>]*> e8a0      	jrc	ra
00008020 <[^>]*> fcbf      	ld	a1,00008118 <foo8\+0x118>
	\.\.\.
00009000 <[^>]*> 6500      	nop
00009002 <[^>]*> e8c0      	jalrc	s0
00009004 <[^>]*> 0aff      	la	v0,00009400 <foo9\+0x400>
00009006 <[^>]*> 6500      	nop
00009008 <[^>]*> 6500      	nop
0000900a <[^>]*> e8c0      	jalrc	s0
0000900c <[^>]*> b3ff      	lw	v1,00009408 <foo9\+0x408>
0000900e <[^>]*> 6500      	nop
00009010 <[^>]*> 6500      	nop
00009012 <[^>]*> e8c0      	jalrc	s0
00009014 <[^>]*> fe9f      	dla	a0,00009090 <foo9\+0x90>
00009016 <[^>]*> 6500      	nop
00009018 <[^>]*> 6500      	nop
0000901a <[^>]*> 6500      	nop
0000901c <[^>]*> 6500      	nop
0000901e <[^>]*> e8c0      	jalrc	s0
00009020 <[^>]*> fcbf      	ld	a1,00009118 <foo9\+0x118>
	\.\.\.
0000a000 <[^>]*> 6500      	nop
0000a002 <[^>]*> e960      	0xe960
0000a004 <[^>]*> 0aff      	la	v0,0000a400 <fooa\+0x400>
0000a006 <[^>]*> 6500      	nop
0000a008 <[^>]*> 6500      	nop
0000a00a <[^>]*> e960      	0xe960
0000a00c <[^>]*> b3ff      	lw	v1,0000a408 <fooa\+0x408>
0000a00e <[^>]*> 6500      	nop
0000a010 <[^>]*> 6500      	nop
0000a012 <[^>]*> e960      	0xe960
0000a014 <[^>]*> fe9f      	dla	a0,0000a090 <fooa\+0x90>
0000a016 <[^>]*> 6500      	nop
0000a018 <[^>]*> 6500      	nop
0000a01a <[^>]*> 6500      	nop
0000a01c <[^>]*> 6500      	nop
0000a01e <[^>]*> e960      	0xe960
0000a020 <[^>]*> fcbf      	ld	a1,0000a118 <fooa\+0x118>
	\.\.\.
