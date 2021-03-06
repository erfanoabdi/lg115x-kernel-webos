/*
	SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
	Copyright(c) 2013 by LG Electronics Inc.

	This program is free software; you can redistribute it and/or 
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
	GNU General Public License for more details.
*/ 


static const CVD_REG_PARAM_T cvd_av_ntsc_m_l_default_h14a0[] = 
{
    {0x42B4, 0x00000200},	
    {0x42B8, 0x00020010},	
    {0x42EC, 0x00016FC1},
    {0x42C0, 0x00000020},
    {0x4650, 0x00000000},   // CTA    
    {0x46A0, 0x00000000},   // SECAM DCR
    {0x46A4, 0x00000000},
    {0x46A8, 0x00000000},
    {0x46AC, 0x00000000},
    {0x4768, 0x08000003},   // Cordic
    {0x476C, 0x08000003},   // Cordic
    {0x4770, 0x08000003},   // Cordic
    {0x4678, 0x00000000},
    {0x4654, 0x00000000},
    {0x45CC, 0x45470000},
    {0x4414, 0x00000011},
    {0x46D4, 0x000F4040}, // H13B 0x00004F40 E2D_ST_dly : reg_cdct_ifcomp_ctrl_0[30]  -> 0x47D8,reg_cdct_ifcomp_ctrl_1[30]
    {0x4620, 0x00000006},
    {0x4624, 0x00000206}, // CTI
    {0x45C8, 0x00033028}, //0x00032008},
    {0x45B0, 0x0010000F},
    {0x469C, 0x00000000},
    {0x4570, 0x00004D61},
    {0x4588, 0x0000FF00},
    {0x4560, 0x00000003},
    {0x4544, 0xF2100300}, //0xF2100000},
    {0x45E8, 0x00000020}, //0x00000008},
    {0x456C, 0x00100000}, //0x00000000},
    {0x45B8, 0x30300409}, //0x3030040B},
    {0x4598, 0x86003FFF},
    {0x4574, 0x140E0F00}, //0x100A0F00},	//0B0A0F00
    {0x4578, 0x00801C20},
    {0x457C, 0x30200000},
    {0x4580, 0x8002A300},
    {0x4584, 0x08200120},
    {0x458C, 0x08200820},
    {0x4630, 0x00001200},
    {0x4718, 0x00040081},
    {0x4734, 0x40800400},
    {0x471C, 0x00200020},
    {0x4720, 0x8040F040},
    {0x4724, 0x00302040},
    {0x4728, 0xC060FFFF}, //0xC040FF60},
    {0x47C8, 0x00000005},
    {0x47BC, 0x08400F30},
    {0x47CC, 0x0F300F30},
    {0x47C0, 0x00301040},
    {0x47C4, 0xC040F060},
    {0x45F4, 0x102007F7},
    {0x45F8, 0xA0FF0800}, //0x90FF0800},
    {0x46D0, 0x00FFA040},
    {0x45FC, 0x01000000},
    {0x4600, 0x00000000},
    {0x4604, 0x00000000},
    {0x4608, 0x0018002F}, //0x00190030 0x0016002C //0x00280046
    {0x4634, 0x002F002E}, //0x0030002E 0x002C002B //0x00340020
    {0x4638, 0x002E002E}, //0x002E002B 0x00290027 //0x00100006
    {0x4250, 0x00000000}, //reg_blue_mode[9:8]
    {0x455C, 0x00050000}, //0x00000000},
    {0x4548, 0x00080425}, //H13B : 0x00080324
    {0x46D8, 0x03302211}, //0x03302210, 0x03302211},
    {0x454C, 0x00201300}, //0x00202300},
    {0x4554, 0x30202000}, //0x30202800},
    {0x46DC, 0x00000000},
    {0x4550, 0x00080010},
    {0x4558, 0x00000000},
    {0x4568, 0x00000001},
    {0x45B4, 0x40000000},
    {0x46EC, 0x00000014},
    {0x46E8, 0x020030C0}, //0x200040C0},
    {0x46F0, 0x80FFC0FF},
    {0x45D8, 0x30204010},
    {0x45DC, 0xB0808060},
    {0x45E0, 0x20004010},
    {0x45E4, 0xB0808040},
    {0x45C0, 0x0D000060},
    {0x45C4, 0x000008FF},
    {0x4640, 0x80FFFFFF},
    {0x4644, 0x00FF80FF},
    {0x46BC, 0xA0E0FFE0},
    {0x4740, 0x40116002}, //H13B 0x40112002
    {0x4744, 0x18000800}, //0x17E10800},
    {0x4748, 0x00201051},
    {0x474C, 0x1C4F1000}, //0x1F4F1000},
    {0x4764, 0x404C0200}, //0x404C1500},
    {0x4750, 0x1C1C1C1C}, //0x18171716},
    {0x4788, 0x25211914},
    {0x4784, 0x2D000000},
    {0x4758, 0x70003510},
    {0x475C, 0x00044120}, //0x060440A0}, //0x360840A0},
    {0x4760, 0x04000010}, //0x30003F0B},
    {0x45BC, 0x08002021},
    {0x46F8, 0x5001F181},
    {0x46FC, 0x00210200},
    {0x470C, 0x523D1901},
    {0x4704, 0x00003027},
    {0x4700, 0x1F140A04},
    {0x4708, 0x00008048},
    {0x45EC, 0x081020FF}, // 0x008008FF}, // 0x08FF20FF Ref4 peaking unblance
    {0x45F0, 0x80FFFFFF}, // 0x10FF20FF},
    {0x4590, 0x0C002438},
    {0x4594, 0x48FF99FF},
    {0x459C, 0x00100013}, // 0x0010001B}, // ST-Y
    {0x472C, 0x0C082408}, // 0x0C002440},
    {0x4730, 0x48089908}, // 0x48009900},
    {0x45A0, 0x00000000},
    {0x45A4, 0x00000000},
    {0x45AC, 0x00000000},
    {0x4738, 0x0C082408},
    {0x473C, 0x48089908},
    {0x45A8, 0x86003FFF},
    {0x4418, 0x00001110},
    {0x4410, 0x00000000},
    {0x441C, 0x00010000},
    {0x4420, 0xFC1EF9CE},
    {0x4424, 0x00001098},
    {0x4428, 0x23862BEC},
    {0x439C, 0x0011038A},
    {0x43A0, 0x01031F8F},
    {0x43A4, 0x1FD00010},
    {0x43A8, 0x00000003},
    {0x43AC, 0x00010380},
    {0x43B0, 0x00EE1FA9},
    {0x43B4, 0x1FE60003},
    {0x43B8, 0x00000000},
    {0x43BC, 0x00010100},
    {0x43C0, 0x0077E77C},
    {0x43C4, 0x0000077C},
    {0x43C8, 0x0077C000},
    {0x43CC, 0x0077C77E},
    {0x43D0, 0x00020020},
    {0x43D4, 0x00002000},
    {0x43D8, 0x00020000}, //{0x43D8, 0x00030000},
    {0x460C, 0x007FF00C},
    {0x4610, 0x007C0135},
    {0x4614, 0x001357C0},
    {0x4618, 0x0000C7FF},
    {0x43DC, 0x0F7C003F},   // NTSC-J OADJ
    {0x43E0, 0x00000EEC},
    {0x43E4, 0x00600202},
    {0x43E8, 0x10001000},
    {0x43EC, 0x06002020}
    //{0x43DC, 0x0F7C003C},   // NTSC-M OADJ
    //{0x43E0, 0x00000EF8},
    //{0x43E4, 0x00600202},
    //{0x43E8, 0x10001000},
    //{0x43EC, 0x06002020}
};
