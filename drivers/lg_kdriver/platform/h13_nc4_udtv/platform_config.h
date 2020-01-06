#include "../h13_columbus/platform_config.h"

#if 0

#ifdef  CONFIG_LX_BOARD_FHD
#undef  CONFIG_LX_BOARD_FHD
#endif

#ifdef  CONFIG_LX_BOARD_UD
#undef  CONFIG_LX_BOARD_UD
#endif

#define CONFIG_LX_BOARD_FHD 0  /* disable BOARD_FHD */
#define CONFIG_LX_BOARD_UD  1  /* enable BOARD_UD   */

#endif
