/*
 * ftest_main.h
 *
 *  Created on: Jul 27, 2022
 *      Author: maibui
 */

#ifndef MOD_FATFS_CHAN_H_
#define MOD_FATFS_CHAN_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "em_chip.h"
#include "em_vdac.h"
#include "em_device.h"
#include "em_timer.h"
#include "em_cmu.h"
//#include "bsp.h"
//#include "bsp_trace.h"
//#include "retargetserial.h"
#include "ff.h"
#include "sdio.h"
#include "diskio.h"

// Command buffer and read data buffer
#define CBUFSIZE    80
static char buffer[CBUFSIZE];
static UINT bufRead;
static char commandLine[CBUFSIZE];
static int commandIndex = 0;
extern volatile uint32_t msTicks; // counts 1ms timeTicks

void BSP_SLSTK3701A_SDIO_HWInit(void);
void Delay(uint32_t dlyTicks);

int fs_bsp_init();
// adding - mai bui - 27July2022
int ftest_main(char* cmmd);
int fs_write_tst_f(char* fn, char* str);
int fs_mount_f();
int fs_unmount_f();
//FRESULT scan_files_1 (char* path);
FRESULT scan_files(char* path,bool loption);
int fs_ls_cmd_f();
int fs_cat_f(char* fn);
FRESULT fs_rm_cmd_f(char* fn);
FRESULT fs_mkdir_cmd_f (char* dirname);
FRESULT fs_mv_cmd_f (char* old_fn, char* new_fn);

extern char* w_dir;

#endif /* MOD_FATFS_CHAN_H_ */
