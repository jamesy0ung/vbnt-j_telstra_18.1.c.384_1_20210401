/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2017-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/** \file
 * Multimedia Switch.
 *
 * \version v1.0
 *
 *************************************************************************/

#ifndef _MMSWITCH_H_
#define _MMSWITCH_H_

#include <linux/workqueue.h>
#include "mmcommon.h"

/**
 * Initialize a work queue
 *
 * This function is similar to the kernel INIT_WORK() function with the exception that the work
 * will be executed by a dedicated thread and not the kworker thread.
 *
 * \since v1.0
 *
 * \param [in] work The work to be executed.
 * \param [in] func The function that will execute the work.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The work has been initialized successfully.
 * \retval MMPBX_ERROR_INVALIDPARAMS One of the input params is invalid.
 */
MmPbxError mmSwitchInitWork(struct work_struct  *work,
                            work_func_t         func);

/**
 * Put an already initialized work in the workqueue of the mmswitch dedicated thread ("mmswitchsock_wq")
 *
 * This function is similar to the kernel schedule_work() function with the exception that the work
 * will be executed by a dedicated thread and not the kworker thread.
 *
 * \since v1.0
 *
 * \param [in] work The work to be executed.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The work has been initialized successfully.
 * \retval MMPBX_ERROR_INVALIDPARAMS One of the input params is invalid.
 */
MmPbxError mmSwitchScheduleWork(struct work_struct *work);
#endif /* _MMSWITCH_H_ */
