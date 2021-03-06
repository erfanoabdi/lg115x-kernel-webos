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

const PE_REG_PARAM_T sre_l_lut_sd_default_ai_m14b0[] =
{
	/* pe1 sre_lut : 0x48B0~0x48B4 */
	/* sra Filter Coef. :  range of address 0 ~ 31 */
	{0x48B0, 0x00001000},
	/* Class 0 */
	{0x48B4, 0x84bc9ec7},
	{0x48B4, 0xcf00a6ff},
	{0x48B4, 0x5c110d2f},
	{0x48B4, 0xb5db50a1},
	{0x48B4, 0x247ba07b},
	{0x48B4, 0x830eac1e},
	{0x48B4, 0x12c443fa},
	{0x48B4, 0x802af93f},
	{0x48B4, 0xee26cbd0},
	{0x48B4, 0x32c1b05f},
	{0x48B4, 0xbfe1212c},
	{0x48B4, 0xd6185dfd},
	{0x48B4, 0x900be602},
	{0x48B4, 0xd6544fd0},
	{0x48B4, 0x22497e46},
	{0x48B4, 0x000004e9},
	/* Class 1 */
	{0x48B4, 0x49f21bf0},
	{0x48B4, 0x3093e819},
	{0x48B4, 0x1362dbbb},
	{0x48B4, 0x8205c28f},
	{0x48B4, 0x08bb6e37},
	{0x48B4, 0xcbef581f},
	{0x48B4, 0xe0e3d52b},
	{0x48B4, 0x35a828b9},
	{0x48B4, 0xe3fb3781},
	{0x48B4, 0xeac9b2f7},
	{0x48B4, 0x77c07805},
	{0x48B4, 0x209fb8dd},
	{0x48B4, 0x335ef045},
	{0x48B4, 0xc60ffece},
	{0x48B4, 0x1a8c81cf},
	{0x48B4, 0x00000f99},
	/* Class 2 */
	{0x48B4, 0x046fe7ef},
	{0x48B4, 0xe0c7f603},
	{0x48B4, 0xcfc2e87a},
	{0x48B4, 0x01fc80d0},
	{0x48B4, 0xf87c601c},
	{0x48B4, 0x10c02c1b},
	{0x48B4, 0xe01f6c18},
	{0x48B4, 0x59721f7a},
	{0x48B4, 0x890c7b60},
	{0x48B4, 0x014dc20b},
	{0x48B4, 0xb8ffd027},
	{0x48B4, 0xff943402},
	{0x48B4, 0x47a50442},
	{0x48B4, 0xf8fafece},
	{0x48B4, 0xf5fee0fb},
	{0x48B4, 0x00000c0d},
	/* Class 3 */
	{0x48B4, 0x04b04814},
	{0x48B4, 0x7f53f010},
	{0x48B4, 0xcb88e381},
	{0x48B4, 0xf700be20},
	{0x48B4, 0x0a3caebb},
	{0x48B4, 0x093ec857},
	{0x48B4, 0xc0a7a122},
	{0x48B4, 0x0532fefb},
	{0x48B4, 0xa1073bd0},
	{0x48B4, 0xed4a9207},
	{0x48B4, 0xfcf08051},
	{0x48B4, 0x8037d7ea},
	{0x48B4, 0xf38a0cbf},
	{0x48B4, 0xedf4ffed},
	{0x48B4, 0x0b8530a7},
	{0x48B4, 0x00001019},
	/* Class 4 */
	{0x48B4, 0xbfd0001d},
	{0x48B4, 0x8044171c},
	{0x48B4, 0xb776f07f},
	{0x48B4, 0x47e5bd91},
	{0x48B4, 0x073a6f8c},
	{0x48B4, 0x908e805d},
	{0x48B4, 0x011f9514},
	{0x48B4, 0x055f047c},
	{0x48B4, 0xa7113d40},
	{0x48B4, 0xe1cd70cf},
	{0x48B4, 0xb741ac6d},
	{0x48B4, 0x6ed806fa},
	{0x48B4, 0xdf5b183c},
	{0x48B4, 0x23fcbc0e},
	{0x48B4, 0x0dc060f0},
	{0x48B4, 0x00000c29},
	/* Class 5 */
	{0x48B4, 0x02b08fef},
	{0x48B4, 0xffb3df08},
	{0x48B4, 0x07ace981},
	{0x48B4, 0xc408bf51},
	{0x48B4, 0x04bcaedf},
	{0x48B4, 0xcb1eec2c},
	{0x48B4, 0xa097b32a},
	{0x48B4, 0x7d31f6fd},
	{0x48B4, 0xc5033def},
	{0x48B4, 0xf0c430f3},
	{0x48B4, 0x7e506011},
	{0x48B4, 0x705febf1},
	{0x48B4, 0xc3e50aff},
	{0x48B4, 0xf5fe01cf},
	{0x48B4, 0x00c19fff},
	{0x48B4, 0x000013f7},
	/* Class 6 */
	{0x48B4, 0xc0d0840e},
	{0x48B4, 0x8e03c926},
	{0x48B4, 0x17cbebfe},
	{0x48B4, 0x204780be},
	{0x48B4, 0x30448d30},
	{0x48B4, 0x89c9fbda},
	{0x48B4, 0x303a6c23},
	{0x48B4, 0x44fb324b},
	{0x48B4, 0x3ef545a3},
	{0x48B4, 0xa789430a},
	{0x48B4, 0x0bd43fc6},
	{0x48B4, 0xd49c7cb7},
	{0x48B4, 0x5ffac4bc},
	{0x48B4, 0x46ea395e},
	{0x48B4, 0x1382b140},
	{0x48B4, 0x000013e9},
	/* Class 7 */
	{0x48B4, 0xc2b0b00f},
	{0x48B4, 0x1d24100f},
	{0x48B4, 0x0b92f151},
	{0x48B4, 0xbc09f8c5},
	{0x48B4, 0x21cf9c98},
	{0x48B4, 0x439edc91},
	{0x48B4, 0x90fbfa31},
	{0x48B4, 0xb9eccdbc},
	{0x48B4, 0x0920f71d},
	{0x48B4, 0x99c7f403},
	{0x48B4, 0x884f1d34},
	{0x48B4, 0x00eb65f9},
	{0x48B4, 0xd38940f5},
	{0x48B4, 0x6fd0d1fb},
	{0x48B4, 0x2dabf0ec},
	{0x48B4, 0x00000834},
	/* Class 8 */
	{0x48B4, 0x479023e3},
	{0x48B4, 0xffeffd06},
	{0x48B4, 0xbb60e543},
	{0x48B4, 0x1ffc3da0},
	{0x48B4, 0x1082adc4},
	{0x48B4, 0x8a2db84c},
	{0x48B4, 0xf04b6a1f},
	{0x48B4, 0x3d62fdfd},
	{0x48B4, 0x6e09ffc0},
	{0x48B4, 0xd207c21b},
	{0x48B4, 0x4110e84f},
	{0x48B4, 0x1fd43ee4},
	{0x48B4, 0x573f0fbe},
	{0x48B4, 0xf6fe024e},
	{0x48B4, 0x00c7c047},
	{0x48B4, 0x000013f8},
	/* Class 9 */
	{0x48B4, 0xc0d0c7fa},
	{0x48B4, 0x8f6c0210},
	{0x48B4, 0xb7d8db84},
	{0x48B4, 0xaefc41ff},
	{0x48B4, 0xef3e6f57},
	{0x48B4, 0x07ae384d},
	{0x48B4, 0x018f4d3f},
	{0x48B4, 0xf5d9063c},
	{0x48B4, 0x6e133880},
	{0x48B4, 0xdc8d13a3},
	{0x48B4, 0xff6f507b},
	{0x48B4, 0x603f9700},
	{0x48B4, 0x63f1f6c1},
	{0x48B4, 0x16f0434d},
	{0x48B4, 0x06c0e064},
	{0x48B4, 0x00000c27},
	/* Class 10 */
	{0x48B4, 0xc4c01c38},
	{0x48B4, 0xe0e42b11},
	{0x48B4, 0x97ece5f9},
	{0x48B4, 0x74f3c310},
	{0x48B4, 0x0ac0d02f},
	{0x48B4, 0xcb5f0461},
	{0x48B4, 0x2163cf32},
	{0x48B4, 0xeda31e02},
	{0x48B4, 0xcc17bc00},
	{0x48B4, 0xe1cb72d7},
	{0x48B4, 0xf74f444e},
	{0x48B4, 0x3f379b01},
	{0x48B4, 0xc396e203},
	{0x48B4, 0x270afa5e},
	{0x48B4, 0x04860044},
	{0x48B4, 0x00000832},
	/* Class 11 */
	{0x48B4, 0xc8516cf3},
	{0x48B4, 0xece40113},
	{0x48B4, 0xa348d1f6},
	{0x48B4, 0x87fbc28e},
	{0x48B4, 0x07bae05f},
	{0x48B4, 0x078ef0ed},
	{0x48B4, 0x1067cc3c},
	{0x48B4, 0x3595ed41},
	{0x48B4, 0xe1fb3b81},
	{0x48B4, 0xe30ca357},
	{0x48B4, 0xbfdfe442},
	{0x48B4, 0xef9f8605},
	{0x48B4, 0xa33dea3b},
	{0x48B4, 0x9f10bced},
	{0x48B4, 0x14874f78},
	{0x48B4, 0x00000c17},
	/* Class 12 */
	{0x48B4, 0x80c04c08},
	{0x48B4, 0x00a01602},
	{0x48B4, 0xfbe1fb7d},
	{0x48B4, 0xdef4015f},
	{0x48B4, 0xfe3da0bb},
	{0x48B4, 0xc90ed83d},
	{0x48B4, 0xb067b51b},
	{0x48B4, 0x01280cfe},
	{0x48B4, 0xb90d7d70},
	{0x48B4, 0xe60832a7},
	{0x48B4, 0x3d2f8037},
	{0x48B4, 0xcf8bae13},
	{0x48B4, 0x57e6f381},
	{0x48B4, 0x2d087c9f},
	{0x48B4, 0xfc803044},
	{0x48B4, 0x00000c1c},
	/* Class 13 */
	{0x48B4, 0xc62fdfcb},
	{0x48B4, 0x2f9ff917},
	{0x48B4, 0xff64e9c9},
	{0x48B4, 0xc4107ffe},
	{0x48B4, 0x0c814e37},
	{0x48B4, 0xc3ed8822},
	{0x48B4, 0x704faa38},
	{0x48B4, 0xe577017d},
	{0x48B4, 0xa3107d6f},
	{0x48B4, 0xe1815337},
	{0x48B4, 0x02fff423},
	{0x48B4, 0x50cbcfe2},
	{0x48B4, 0x8b6af8ff},
	{0x48B4, 0xfdf7480e},
	{0x48B4, 0xfe86f0df},
	{0x48B4, 0x000013e4},
	/* Class 14 */
	{0x48B4, 0x3a70d43c},
	{0x48B4, 0x20307017},
	{0x48B4, 0x6b5cee3f},
	{0x48B4, 0x45ec82c3},
	{0x48B4, 0x2efd2f88},
	{0x48B4, 0x0c8e9c95},
	{0x48B4, 0xd2239b0d},
	{0x48B4, 0xc92bec7c},
	{0x48B4, 0x331e344f},
	{0x48B4, 0xc514f04f},
	{0x48B4, 0xb7e04468},
	{0x48B4, 0x3df0181e},
	{0x48B4, 0x17c51bfe},
	{0x48B4, 0x441dff1f},
	{0x48B4, 0xf5ba81ec},
	{0x48B4, 0x000008bd},
	/* Class 15 */
	{0x48B4, 0x83d1403e},
	{0x48B4, 0xb0107a1e},
	{0x48B4, 0x4013f804},
	{0x48B4, 0x1f0a0663},
	{0x48B4, 0x2609eff0},
	{0x48B4, 0x88600467},
	{0x48B4, 0x0064302d},
	{0x48B4, 0x5d221181},
	{0x48B4, 0xf70bf9d0},
	{0x48B4, 0xda43722b},
	{0x48B4, 0x407fdbf0},
	{0x48B4, 0x6f3f97ee},
	{0x48B4, 0x9bb7087f},
	{0x48B4, 0x350b80be},
	{0x48B4, 0x1481d0c4},
	{0x48B4, 0x00000459},
	/* Class 16 */
	{0x48B4, 0x4320ec02},
	{0x48B4, 0xaf401309},
	{0x48B4, 0x4f9ee401},
	{0x48B4, 0xd10bc01f},
	{0x48B4, 0x02810f67},
	{0x48B4, 0x449e684b},
	{0x48B4, 0xd093da29},
	{0x48B4, 0x490c043a},
	{0x48B4, 0xd90c7b50},
	{0x48B4, 0xe304b2b7},
	{0x48B4, 0x812f7430},
	{0x48B4, 0xc0ebbff1},
	{0x48B4, 0x579ff8bf},
	{0x48B4, 0x0ff8039e},
	{0x48B4, 0x0f42a07c},
	{0x48B4, 0x000013ed},
	/* Class 17 */
	{0x48B4, 0x4d712381},
	{0x48B4, 0x706ba9f0},
	{0x48B4, 0xc3c1e685},
	{0x48B4, 0xcf24c532},
	{0x48B4, 0x36094c53},
	{0x48B4, 0x425f3409},
	{0x48B4, 0xbf1bf316},
	{0x48B4, 0xdd3b0635},
	{0x48B4, 0xedf2b16e},
	{0x48B4, 0xe7820177},
	{0x48B4, 0xc171bbff},
	{0x48B4, 0x928c17c9},
	{0x48B4, 0xbb9f1582},
	{0x48B4, 0xc505487e},
	{0x48B4, 0x028a001b},
	{0x48B4, 0x00000fa9},
	/* Class 18 */
	{0x48B4, 0x42f033fd},
	{0x48B4, 0x407bd20e},
	{0x48B4, 0x8391edc1},
	{0x48B4, 0x12eb82c0},
	{0x48B4, 0xf1802048},
	{0x48B4, 0xc2903832},
	{0x48B4, 0x309c14fb},
	{0x48B4, 0xc505e43b},
	{0x48B4, 0x2cfab8f0},
	{0x48B4, 0x0740308c},
	{0x48B4, 0xbdcf884f},
	{0x48B4, 0x2e8c2bfd},
	{0x48B4, 0xebe00c02},
	{0x48B4, 0xdf08c0dd},
	{0x48B4, 0x0c046fe3},
	{0x48B4, 0x000017ef},
	/* Class 19 */
	{0x48B4, 0xc2f0d415},
	{0x48B4, 0x60340417},
	{0x48B4, 0xf39cdfbe},
	{0x48B4, 0xb9f77fff},
	{0x48B4, 0xfdfb3f7b},
	{0x48B4, 0x0cfe505d},
	{0x48B4, 0xd1079531},
	{0x48B4, 0xe5e8137a},
	{0x48B4, 0x920fbc30},
	{0x48B4, 0xe40c531f},
	{0x48B4, 0xfa6f9848},
	{0x48B4, 0x8fa39bf3},
	{0x48B4, 0x0f75fe00},
	{0x48B4, 0x15007ede},
	{0x48B4, 0x1142c0dc},
	{0x48B4, 0x00000c11},
	/* Class 20 */
	{0x48B4, 0x44a09bb5},
	{0x48B4, 0xc117e5ff},
	{0x48B4, 0xa3540583},
	{0x48B4, 0x04f641b0},
	{0x48B4, 0xed0ddd20},
	{0x48B4, 0xb0428373},
	{0x48B4, 0xd0becc63},
	{0x48B4, 0x5defb8cd},
	{0x48B4, 0x9813cd4b},
	{0x48B4, 0x352cf70e},
	{0x48B4, 0x113e5f88},
	{0x48B4, 0xfdec5ebd},
	{0x48B4, 0x3f1616c0},
	{0x48B4, 0xdb1abfe1},
	{0x48B4, 0x08854017},
	{0x48B4, 0x000017c2},
	/* Class 21 */
	{0x48B4, 0x3ff11831},
	{0x48B4, 0x6fa7e012},
	{0x48B4, 0xcfc6f539},
	{0x48B4, 0xbdfa4470},
	{0x48B4, 0xfa7d1073},
	{0x48B4, 0x4bae8ca8},
	{0x48B4, 0x7fbf1a25},
	{0x48B4, 0x0590f8bb},
	{0x48B4, 0x39fb3731},
	{0x48B4, 0xff1272bb},
	{0x48B4, 0x7d8f74ab},
	{0x48B4, 0xe0701600},
	{0x48B4, 0x2ba80386},
	{0x48B4, 0xc0ea3a0c},
	{0x48B4, 0x1b005203},
	{0x48B4, 0x00000c23},
	/* Class 22 */
	{0x48B4, 0x3da297b9},
	{0x48B4, 0xd43fbae9},
	{0x48B4, 0xc8300c3e},
	{0x48B4, 0x2ceac2b2},
	{0x48B4, 0x12c5a18b},
	{0x48B4, 0xc18a446e},
	{0x48B4, 0x608bd139},
	{0x48B4, 0x559af13c},
	{0x48B4, 0x1a28bb71},
	{0x48B4, 0xfa0bd350},
	{0x48B4, 0x3f917050},
	{0x48B4, 0xc06791f1},
	{0x48B4, 0x4efed73b},
	{0x48B4, 0x741a7d4c},
	{0x48B4, 0x10070fb8},
	{0x48B4, 0x00000872},
	/* Class 23 */
	{0x48B4, 0x83fe8404},
	{0x48B4, 0x6fac1a09},
	{0x48B4, 0x23ba1e41},
	{0x48B4, 0x490cff4f},
	{0x48B4, 0x02bf9c90},
	{0x48B4, 0xb8af2002},
	{0x48B4, 0xd04c192b},
	{0x48B4, 0x4d3af03d},
	{0x48B4, 0xef083b90},
	{0x48B4, 0xddff12c3},
	{0x48B4, 0xc1509022},
	{0x48B4, 0x712bdfdb},
	{0x48B4, 0xf7b2ed7f},
	{0x48B4, 0x06f8c55f},
	{0x48B4, 0xfd82d0e8},
	{0x48B4, 0x000017e8},
	/* Class 24 */
	{0x48B4, 0x46924802},
	{0x48B4, 0xbf5420ff},
	{0x48B4, 0x185ccdc4},
	{0x48B4, 0x4b0f7b33},
	{0x48B4, 0x0593add3},
	{0x48B4, 0xc68e3c37},
	{0x48B4, 0xa1a8130d},
	{0x48B4, 0xad361df3},
	{0x48B4, 0x790045eb},
	{0x48B4, 0xf6c411f7},
	{0x48B4, 0xb4f1445f},
	{0x48B4, 0xaf653fe0},
	{0x48B4, 0x5adb203f},
	{0x48B4, 0x07d64e91},
	{0x48B4, 0xfd416190},
	{0x48B4, 0x0000084d},
	/* Class 25 */
	{0x48B4, 0xbed023c6},
	{0x48B4, 0xd0b80af6},
	{0x48B4, 0xbbe0ffbc},
	{0x48B4, 0x0dee07ff},
	{0x48B4, 0xf1807124},
	{0x48B4, 0xcc1e9808},
	{0x48B4, 0x30077532},
	{0x48B4, 0x09451738},
	{0x48B4, 0xf709b401},
	{0x48B4, 0xea8d22c3},
	{0x48B4, 0x7f1fe82a},
	{0x48B4, 0xbf2bed05},
	{0x48B4, 0x3fa7f684},
	{0x48B4, 0xf8fe7b4f},
	{0x48B4, 0x0682808f},
	{0x48B4, 0x00000c1b},
	/* Class 26 */
	{0x48B4, 0x04bff3f8},
	{0x48B4, 0x0f8c0207},
	{0x48B4, 0xe39de7c4},
	{0x48B4, 0xdd08ff3f},
	{0x48B4, 0x0285ce87},
	{0x48B4, 0x095e5828},
	{0x48B4, 0xe0838a28},
	{0x48B4, 0xe55cf77e},
	{0x48B4, 0xad06012e},
	{0x48B4, 0xf280023b},
	{0x48B4, 0x3fd01407},
	{0x48B4, 0x807829d6},
	{0x48B4, 0xdbc2093f},
	{0x48B4, 0xfcfac07f},
	{0x48B4, 0xfd044ff3},
	{0x48B4, 0x00001008},
	/* Class 27 */
	{0x48B4, 0xc570d42a},
	{0x48B4, 0xe08c3205},
	{0x48B4, 0x1bd3f0f9},
	{0x48B4, 0xb1f2c230},
	{0x48B4, 0xf6bcd063},
	{0x48B4, 0x0ccfa03b},
	{0x48B4, 0x8007981b},
	{0x48B4, 0x85531cc0},
	{0x48B4, 0xa20e3d20},
	{0x48B4, 0xfa49c297},
	{0x48B4, 0x380ec023},
	{0x48B4, 0x3f73bff9},
	{0x48B4, 0xc7a2efc3},
	{0x48B4, 0x2e053bad},
	{0x48B4, 0x03c370e4},
	{0x48B4, 0x00000c3b},
	/* Class 28 */
	{0x48B4, 0x54cef4ff},
	{0x48B4, 0x8c8a9214},
	{0x48B4, 0x2cbcd218},
	{0x48B4, 0xfb4d347f},
	{0x48B4, 0xf8085b64},
	{0x48B4, 0x7644949a},
	{0x48B4, 0xc0b7b8d7},
	{0x48B4, 0xb41df387},
	{0x48B4, 0xd370b00e},
	{0x48B4, 0x80935186},
	{0x48B4, 0x75de71f3},
	{0x48B4, 0x1b5fd45a},
	{0x48B4, 0xec78f1b8},
	{0x48B4, 0xbc3c3be4},
	{0x48B4, 0x63850008},
	{0x48B4, 0x000004fe},
	/* Class 29 */
	{0x48B4, 0x891fd8e5},
	{0x48B4, 0xfe887d11},
	{0x48B4, 0x3f8ae0b3},
	{0x48B4, 0x82f6fc7f},
	{0x48B4, 0xf5b43f8b},
	{0x48B4, 0x4bb0785a},
	{0x48B4, 0x405fa415},
	{0x48B4, 0x01851144},
	{0x48B4, 0xa0f48361},
	{0x48B4, 0x040d62af},
	{0x48B4, 0xb69ff0a4},
	{0x48B4, 0x3fd39305},
	{0x48B4, 0xf793ed86},
	{0x48B4, 0x4aef352d},
	{0x48B4, 0x08ca0f80},
	{0x48B4, 0x00000c62},
	/* Class 30 */
	{0x48B4, 0x3fd0c023},
	{0x48B4, 0x1fa02217},
	{0x48B4, 0x3bbfe53f},
	{0x48B4, 0xbcfbbf30},
	{0x48B4, 0xfd3f8ffb},
	{0x48B4, 0x490edc4c},
	{0x48B4, 0x20b7cc2f},
	{0x48B4, 0x095a077e},
	{0x48B4, 0x9d0f7c41},
	{0x48B4, 0xe949c20b},
	{0x48B4, 0x3b0f7c40},
	{0x48B4, 0x3f87bbff},
	{0x48B4, 0x97a7fa00},
	{0x48B4, 0x2b02be4e},
	{0x48B4, 0x0a3f50a4},
	{0x48B4, 0x00000c2f},
	/* Class 31 */
	{0x48B4, 0x3c00441d},
	{0x48B4, 0x303c5c26},
	{0x48B4, 0xe022e9c1},
	{0x48B4, 0xbf03bfe2},
	{0x48B4, 0x0e47d013},
	{0x48B4, 0x042e688e},
	{0x48B4, 0x7127f424},
	{0x48B4, 0x9102f27f},
	{0x48B4, 0x771e3f6f},
	{0x48B4, 0xdd8531e7},
	{0x48B4, 0xbb2fa49b},
	{0x48B4, 0xe00b910a},
	{0x48B4, 0xc7c7fb01},
	{0x48B4, 0x420b3e3e},
	{0x48B4, 0x06bc7084},
	{0x48B4, 0x0000082a},

	/* srb Filter Coef. :  range of address 0 ~ 3 */
	{0x48B0, 0x00001200},
	/* Class 0 */
	{0x48B4, 0xc5001bfd},
	{0x48B4, 0x8f8fed02},
	{0x48B4, 0xffb0e8c3},
	{0x48B4, 0xe2063ee0},
	{0x48B4, 0x08fd1e6b},
	{0x48B4, 0x88fe4c2f},
	{0x48B4, 0x4053a11d},
	{0x48B4, 0xc968fc7f},
	{0x48B4, 0x9f03ff5f},
	{0x48B4, 0xe408e1d3},
	{0x48B4, 0x7d20942b},
	{0x48B4, 0x2067e3e6},
	{0x48B4, 0xa3ae10bf},
	{0x48B4, 0xeaf9836e},
	{0x48B4, 0x0244d02f},
	{0x48B4, 0x000013fa},
	/* Class 1 */
	{0x48B4, 0xc85f9002},
	{0x48B4, 0x5f83e5ff},
	{0x48B4, 0xfb8be604},
	{0x48B4, 0x4bfebff0},
	{0x48B4, 0x03430c90},
	{0x48B4, 0x48ddc83f},
	{0x48B4, 0x60278d29},
	{0x48B4, 0x65bce6c1},
	{0x48B4, 0x8c01c16e},
	{0x48B4, 0xdc88e293},
	{0x48B4, 0x02d0443f},
	{0x48B4, 0xdff449c9},
	{0x48B4, 0x6b8d0fbf},
	{0x48B4, 0xe7f7c43e},
	{0x48B4, 0xf9086ff3},
	{0x48B4, 0x00001002},
	/* Class 2 */
	{0x48B4, 0x89fefc28},
	{0x48B4, 0x1f07d600},
	{0x48B4, 0x1765ee83},
	{0x48B4, 0x7af88161},
	{0x48B4, 0x0583fbf4},
	{0x48B4, 0x05be0466},
	{0x48B4, 0x9f8f7a36},
	{0x48B4, 0xadffd982},
	{0x48B4, 0x83f7c28d},
	{0x48B4, 0xe1058367},
	{0x48B4, 0x83d04464},
	{0x48B4, 0x9f847ac0},
	{0x48B4, 0x03561241},
	{0x48B4, 0xd2f2429f},
	{0x48B4, 0xf1c99033},
	{0x48B4, 0x00001022},
	/* Class 3*/
	{0x48B4, 0xc4aee428},
	{0x48B4, 0xdf2bdc0c},
	{0x48B4, 0xa7a4ff82},
	{0x48B4, 0x6af5c25f},
	{0x48B4, 0x08412d68},
	{0x48B4, 0x3f8e9065},
	{0x48B4, 0xcf2ff225},
	{0x48B4, 0x9532ecbf},
	{0x48B4, 0xfaf2408e},
	{0x48B4, 0xe6c021e3},
	{0x48B4, 0xc0809c5e},
	{0x48B4, 0xaf3471d7},
	{0x48B4, 0x27a5fe41},
	{0x48B4, 0xe2f542e0},
	{0x48B4, 0xef0450a3},
	{0x48B4, 0x00001421},

	/* sra Index Table  :  range of address 0 ~ 194 */
	{0x48B0, 0x00001400},
	{0x48B4, 0x348420aa},
	{0x48B4, 0x08f293cf},
	{0x48B4, 0x1ef7bf5a},
	{0x48B4, 0x3482b49e},
	{0x48B4, 0x2057edef},
	{0x48B4, 0x227d2348},
	{0x48B4, 0x0af1a8a4},
	{0x48B4, 0x0b3fffef},
	{0x48B4, 0x35ad68a9},
	{0x48B4, 0x0a929610},
	{0x48B4, 0x0e739f43},
	{0x48B4, 0x06340c7e},
	{0x48B4, 0x08e7bdef},
	{0x48B4, 0x1efd68a8},
	{0x48B4, 0x1a82959e},
	{0x48B4, 0x22f7be31},
	{0x48B4, 0x345421ac},
	{0x48B4, 0x11e9abca},
	{0x48B4, 0x3cf3ff4d},
	{0x48B4, 0x1b76b46d},
	{0x48B4, 0x3de7bce7},
	{0x48B4, 0x22741503},
	{0x48B4, 0x3cf23ca4},
	{0x48B4, 0x07e7bdef},
	{0x48B4, 0x3486b77e},
	{0x48B4, 0x1df6a273},
	{0x48B4, 0x3df7bda5},
	{0x48B4, 0x1adf7e6f},
	{0x48B4, 0x1a44154f},
	{0x48B4, 0x1ef6b757},
	{0x48B4, 0x18cf7da5},
	{0x48B4, 0x20d39ce7},
	{0x48B4, 0x1032a4be},
	{0x48B4, 0x1bf2914f},
	{0x48B4, 0x15f7bd03},
	{0x48B4, 0x0a52d433},
	{0x48B4, 0x2737bd5f},
	{0x48B4, 0x1ef6a5b0},
	{0x48B4, 0x20f4be04},
	{0x48B4, 0x0aaf1def},
	{0x48B4, 0x1ad6b67e},
	{0x48B4, 0x27e9a59e},
	{0x48B4, 0x0e779e6a},
	{0x48B4, 0x16a9a8aa},
	{0x48B4, 0x36f5bd4f},
	{0x48B4, 0x3af9cd73},
	{0x48B4, 0x1299f94a},
	{0x48B4, 0x02157d4f},
	{0x48B4, 0x26c6cd2a},
	{0x48B4, 0x11edbcaa},
	{0x48B4, 0x3cf9fd33},
	{0x48B4, 0x1ad4d465},
	{0x48B4, 0x37b9cfff},
	{0x48B4, 0x26a9b02c},
	{0x48B4, 0x34344d4f},
	{0x48B4, 0x02f9fe6f},
	{0x48B4, 0x2735ccc6},
	{0x48B4, 0x0687a831},
	{0x48B4, 0x129dc4ac},
	{0x48B4, 0x1a5d4d05},
	{0x48B4, 0x3ce2a8ae},
	{0x48B4, 0x073695ad},
	{0x48B4, 0x34d423cc},
	{0x48B4, 0x0219a4b0},
	{0x48B4, 0x26a82a7f},
	{0x48B4, 0x20f9be0a},
	{0x48B4, 0x13f5fe6c},
	{0x48B4, 0x2104a576},
	{0x48B4, 0x3ca08529},
	{0x48B4, 0x3ca829b3},
	{0x48B4, 0x27f47a0f},
	{0x48B4, 0x1b34fc7f},
	{0x48B4, 0x12c6b529},
	{0x48B4, 0x1b09b210},
	{0x48B4, 0x1292a4aa},
	{0x48B4, 0x2196a47e},
	{0x48B4, 0x26a0fdb3},
	{0x48B4, 0x06a86e70},
	{0x48B4, 0x1b01c18c},
	{0x48B4, 0x021d40b0},
	{0x48B4, 0x1ac695a9},
	{0x48B4, 0x105811b3},
	{0x48B4, 0x1a9441a9},
	{0x48B4, 0x1ad6a50d},
	{0x48B4, 0x12c841a9},
	{0x48B4, 0x1ad4235a},
	{0x48B4, 0x205295ef},
	{0x48B4, 0x1efe3def},
	{0x48B4, 0x35ad3609},
	{0x48B4, 0x10dd9273},
	{0x48B4, 0x0e77be05},
	{0x48B4, 0x20d4a61e},
	{0x48B4, 0x3c4da929},
	{0x48B4, 0x1ef69517},
	{0x48B4, 0x27e19724},
	{0x48B4, 0x20539fc7},
	{0x48B4, 0x2657bf5a},
	{0x48B4, 0x18a76def},
	{0x48B4, 0x0a877b5a},
	{0x48B4, 0x145d94ef},
	{0x48B4, 0x11a73f45},
	{0x48B4, 0x1087bfef},
	{0x48B4, 0x348fab5a},
	{0x48B4, 0x108ff9c7},
	{0x48B4, 0x18577cba},
	{0x48B4, 0x3ff9c3ef},
	{0x48B4, 0x26d24d08},
	{0x48B4, 0x3fe9a0ff},
	{0x48B4, 0x0a523ca5},
	{0x48B4, 0x06363d8f},
	{0x48B4, 0x0a824d08},
	{0x48B4, 0x128f4ce4},
	{0x48B4, 0x1a8294a5},
	{0x48B4, 0x27e80f7b},
	{0x48B4, 0x12c7bda5},
	{0x48B4, 0x1052b06a},
	{0x48B4, 0x0a42ce6f},
	{0x48B4, 0x26f6b5ad},
	{0x48B4, 0x069194a5},
	{0x48B4, 0x2058295f},
	{0x48B4, 0x1b06c1b3},
	{0x48B4, 0x1092fa1f},
	{0x48B4, 0x1a781db4},
	{0x48B4, 0x1b76b5ad},
	{0x48B4, 0x21084210},
	{0x48B4, 0x210bb6f7},
	{0x48B4, 0x1292a5ad},
	{0x48B4, 0x2ed82610},
	{0x48B4, 0x0a3991ef},
	{0x48B4, 0x11e0bdb3},
	{0x48B4, 0x20584746},
	{0x48B4, 0x2314de05},
	{0x48B4, 0x3e5bfeff},
	{0x48B4, 0x297bb5ad},
	{0x48B4, 0x28dbb46a},
	{0x48B4, 0x06fd3d1f},
	{0x48B4, 0x06e1bc6f},
	{0x48B4, 0x06f188aa},
	{0x48B4, 0x2422fabb},
	{0x48B4, 0x02aefebf},
	{0x48B4, 0x34247d09},
	{0x48B4, 0x11e190b8},
	{0x48B4, 0x10e1e062},
	{0x48B4, 0x21ed0f53},
	{0x48B4, 0x2a21447e},
	{0x48B4, 0x07883c6f},
	{0x48B4, 0x24c92a4f},
	{0x48B4, 0x3af5aa7f},
	{0x48B4, 0x25094c21},
	{0x48B4, 0x16f2e10a},
	{0x48B4, 0x34cd1438},
	{0x48B4, 0x0b89bd13},
	{0x48B4, 0x065d2031},
	{0x48B4, 0x0316bd1f},
	{0x48B4, 0x0aa2bc6f},
	{0x48B4, 0x06f2a8bf},
	{0x48B4, 0x0aa2ba6f},
	{0x48B4, 0x27f6fd1e},
	{0x48B4, 0x10f47878},
	{0x48B4, 0x1188791e},
	{0x48B4, 0x10244fd8},
	{0x48B4, 0x07fd2105},
	{0x48B4, 0x06a23cb3},
	{0x48B4, 0x14f71ce7},
	{0x48B4, 0x07013f48},
	{0x48B4, 0x0637bdf1},
	{0x48B4, 0x0b377f43},
	{0x48B4, 0x3df9bdc7},
	{0x48B4, 0x0b313cba},
	{0x48B4, 0x1ef415ef},
	{0x48B4, 0x0b37bf45},
	{0x48B4, 0x06ff01e7},
	{0x48B4, 0x2082bdfa},
	{0x48B4, 0x3fe7fb45},
	{0x48B4, 0x1f1d21fa},
	{0x48B4, 0x3d3d3dfa},
	{0x48B4, 0x1e37ecb3},
	{0x48B4, 0x1ef44de7},
	{0x48B4, 0x37b11595},
	{0x48B4, 0x265dcc25},
	{0x48B4, 0x1939cd7b},
	{0x48B4, 0x3d55f7a6},
	{0x48B4, 0x021eed25},
	{0x48B4, 0x2654cfc9},
	{0x48B4, 0x06818d4c},
	{0x48B4, 0x18354d8c},
	{0x48B4, 0x368faa05},
	{0x48B4, 0x2659cd08},
	{0x48B4, 0x321297bd},
	{0x48B4, 0x3aae8583},
	{0x48B4, 0x15b0f601},
	{0x48B4, 0x3a60ce6a},
	{0x48B4, 0x0be1fbe8},
	{0x48B4, 0x26529503},
	{0x48B4, 0x1052a000},

	/* set to normal */
	{0x48B0, 0x00008000}
};
