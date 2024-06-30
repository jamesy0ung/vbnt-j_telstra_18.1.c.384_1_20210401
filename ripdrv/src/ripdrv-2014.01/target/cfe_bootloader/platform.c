#include "rip2_config.h"
#include "flash_api.h"
#include "otp.h"

/*
 * Flash functions are implemented in the CFE bootloader
 * repository in file technicolor/rip/interface.c
 */

int otp_chipid_read()
{
    return otp_read_chipid();
}

int rip2_flash_get_size()
{
    return flash_get_sector_size(0);
}
