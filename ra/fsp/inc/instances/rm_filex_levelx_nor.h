/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef RM_FILEX_LEVELX_NOR_H
#define RM_FILEX_LEVELX_NOR_H

/*******************************************************************************************************************//**
 * @addtogroup RM_FILEX_LEVELX_NOR
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 ***********************************************************************************************************************/
#include "bsp_api.h"
#include "fx_api.h"
#include "lx_api.h"
#include "rm_levelx_nor_spi.h"

/** Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
#ifndef VOID
 #define VOID    void
typedef char           CHAR;
typedef char           BOOL;
typedef unsigned char  UCHAR;
typedef int            INT;
typedef unsigned int   UINT;
typedef long           LONG;
typedef unsigned long  ULONG;
typedef short          SHORT;
typedef unsigned short USHORT;
#endif

/** Options for the callback events. */
typedef enum e_rm_filex_levelx_nor_event
{
    RM_FILEX_LEVELX_NOR_EVENT_BUSY,    ///< Pending operation, user can define their own wait functionality.
} rm_filex_levelx_nor_event_t;

/** Callback function parameter data */
typedef struct st_rm_filex_levelx_nor_callback_args
{
    rm_filex_levelx_nor_event_t event;     ///< The event can be used to identify what caused the callback.
    void const                * p_context; ///< Placeholder for user data
} rm_filex_levelx_nor_callback_args_t;

/** FileX LevelX configuration */
typedef struct st_rm_filex_levelx_nor_cfg
{
    UINT (* nor_driver_initialize)(LX_NOR_FLASH *);                    ///< Pointer to the initialization function
    LX_NOR_FLASH * p_nor_flash;                                        ///< NOR Flash instance
    CHAR         * p_nor_flash_name;                                   ///< NOR Flash instance name
    fsp_err_t (* close)();                                             ///< Pointer to underlying driver close
    void (* p_callback)(rm_filex_levelx_nor_callback_args_t * p_args); ///< Pointer to callback function
    void const * p_context;                                            ///< Placeholder for user data
} rm_filex_levelx_nor_cfg_t;

/** FileX block media private control block. DO NOT MODIFY.  Initialization occurs when RM_FILEX_LEVELX_NOR_Open is called. */
typedef struct
{
    rm_filex_levelx_nor_cfg_t const * p_cfg; ///< Pointer to instance configuration.
} rm_filex_levelx_nor_instance_ctrl_t;

/** This structure encompasses everything that is needed to use an instance of this interface. */
typedef struct st_rm_filex_levelx_nor_instance
{
    rm_filex_levelx_nor_instance_ctrl_t * p_ctrl; ///< Pointer to the control structure for this instance
    rm_filex_levelx_nor_cfg_t const     * p_cfg;  ///< Pointer to the configuration structure for this instance
} rm_filex_levelx_nor_instance_t;

/**********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/
void RM_FILEX_LEVELX_NOR_DeviceDriver(FX_MEDIA * p_fx_media);

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/** Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER

/*******************************************************************************************************************//**
 * @} (end addtogroup RM_FILEX_LEVELX_NOR)
 **********************************************************************************************************************/

#endif
