#include "rip2_config.h"
#include "otp.h"

/*
 * Flash functions are implemented in the bootloader/common-tch.git
 * repository in file technicolor/rip/interface.c
 */

int otp_chipid_read()
{
    return otp_read_chipid();
}
