/***************************************************************************//**
 * @file
 * @brief FAT example using FatFS for access to the MicroSD card on the SLWSTK.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

/*
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
*/
//#define AM_LFN    0x0F  /* LFN entry */
#include <mod_fatfs_chan.h>
#include "sl_iostream.h"
#include "sl_iostream_init_instances.h"
#include "sl_iostream_handles.h"/* File system */
FATFS Fatfs;
FIL fh;
TCHAR path[100];
volatile uint32_t msTicks;
#define SZ_STR 100
extern char Curr_Addr[100] = "/";
/* Command buffer and read data buffer */
/*
#define CBUFSIZE    80
static char buffer[CBUFSIZE];
static UINT bufRead;
static char commandLine[CBUFSIZE];
static int commandIndex = 0;
volatile uint32_t msTicks; // counts 1ms timeTicks

void BSP_SLSTK3701A_SDIO_HWInit(void);
void Delay(uint32_t dlyTicks);
*/
#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824

/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++; /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks);
}

/***************************************************************************//**
 * @brief
 *   This function is required by the FAT file system in order to provide
 *   timestamps for created files. Since we do not have a reliable clock we
 *   hardcode a value here.
 *
 *   Refer to drivers/fatfs/doc/en/fattime.html for the format of this DWORD.
 * @return
 *    A DWORD containing the current time and date as a packed datastructure.
 ******************************************************************************/
//MAG make switch to get gg11 system time if available
DWORD get_fattime(void)
{
  return (28 << 25) | (2 << 21) | (1 << 16);
}

/***************************************************************************//**
 * @brief Output an array of characters
 * @param buf Pointer to string buffer
 * @param length Number of characters to output
 ******************************************************************************/
void PrintBuf(char *buf, int length)
{
  while (length--)
    putchar(*buf++);
}

/***************************************************************************//**
 * @brief Development board related HW configuration
 ******************************************************************************/
void BSP_SLSTK3701A_SDIO_HWInit(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Soldered sdCard slot
  GPIO_PinModeSet(gpioPortE, 7u, gpioModePushPull, 1);
  GPIO_PinOutSet(gpioPortE, 7u);

  GPIO_PinModeSet(gpioPortB, 10, gpioModeInput, 0);             // SDIO_CD
  GPIO_PinModeSet(gpioPortE, 15, gpioModePushPullAlternate, 0); // SDIO_CMD
  GPIO_PinModeSet(gpioPortE, 14, gpioModePushPullAlternate, 1); // SDIO_CLK
  GPIO_PinModeSet(gpioPortA, 0, gpioModePushPullAlternate, 1);  // SDIO_DAT0
  GPIO_PinModeSet(gpioPortA, 1, gpioModePushPullAlternate, 1);  // SDIO_DAT1
  GPIO_PinModeSet(gpioPortA, 2, gpioModePushPullAlternate, 1);  // SDIO_DAT2
  GPIO_PinModeSet(gpioPortA, 3, gpioModePushPullAlternate, 1);  // SDIO_DAT3
  GPIO_PinModeSet(gpioPortB, 9, gpioModePushPullAlternate, 0);  // WP
}
/***************************************************************************//**
* @brief Convert File size
* @param path to traverse
******************************************************************************/

void convert_size(long long size) {
    if (size < KILOBYTE)
        printf("%d b\n", (uint32_t)size);
    else if (size < MEGABYTE)
        printf("%.2f Kb\n", size / (double)KILOBYTE);
    else if (size < GIGABYTE){
        volatile double temp = size / (double)MEGABYTE;
        printf("%.2f Mb\n", temp );
    }

    else{
        printf("%.2f Gb\n", size / (double)GIGABYTE);
    }
}
/***************************************************************************//**
 * @brief scan_files from FatFS documentation
 * @param path to traverse
 ******************************************************************************/
FRESULT get_file_size(char* path, DWORD* size) {
    FRESULT fr;               /* FatFs function common result code */
    FILINFO fno;              /* File information object */

    fr = f_stat(path, &fno);
    if (fr) return fr;

    *size = fno.fsize;

    return FR_OK;
}
//MAG from CHAN org example added line to print directory
FRESULT scan_files (
    TCHAR* path,
    bool loption,/* Start node to be scanned (***also used as work area***) */
    uint32_t tag
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                 /* Read a directory item */
            if(tag==0){
                if (res != FR_OK || fno.fname[0] == 0) break;
                printf("%-30s\n" ,fno.fname);
                continue;
            }

            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                printf("%-30s|/%-30s| ", path, fno.fname);
                sprintf(&path[i], "/%s ", fno.fname);
                unsigned int year = ((fno.fdate >> 9) & 0x7F) + 1980;
                unsigned int month = (fno.fdate >> 5) & 0x0F;
                unsigned int day = fno.fdate & 0x1F;
                printf("%04u/%02u/%02u  ", year, month, day);

                unsigned int hour = (fno.ftime >> 11) & 0x1F;
                unsigned int minute = (fno.ftime >> 5) & 0x3F;
                unsigned int second = (fno.ftime & 0x1F) * 2;  // multiply by 2 because seconds are stored divided by 2.
                printf("%02u:%02u:%02u\n", hour, minute, second);
                /**char temp [100] = {0};

                if(strcmp(Curr_Addr,"/")){
                    strcat(temp, Curr_Addr);
                    strcat(temp, path+1);
                }**/
                res = scan_files(path,0,tag);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                printf("%-30s|/%-30s| ", path, fno.fname);
                //printf("/%s/ ", fno.fname);
                unsigned int year = ((fno.fdate >> 9) & 0x7F) + 1980;
                unsigned int month = (fno.fdate >> 5) & 0x0F;
                unsigned int day = fno.fdate & 0x1F;
                printf("%04u/%02u/%02u  ", year, month, day);

                unsigned int hour = (fno.ftime >> 11) & 0x1F;
                unsigned int minute = (fno.ftime >> 5) & 0x3F;
                unsigned int second = (fno.ftime & 0x1F) * 2;  // multiply by 2 because seconds are stored divided by 2.
                printf("%02u:%02u:%02u  ", hour, minute, second);
                if(tag== 1){
                    printf("%lu\n",(uint32_t)fno.fsize);
                }else if (tag ==2){
                    convert_size(fno.fsize);
                }
            }
        }
        f_closedir(&dir);
    }else{
        printf("File/Directory Does not Exist!!! Res = %d\n" ,res);
    }

    return res;
}


int fs_mount_f()
{
  int retval = 0;
  retval = f_mount(&Fatfs,"", 0);
    if (retval != FR_OK)
    {
      printf("FAT-mount failed: %d\n", retval);
    }

  return retval;
}

int fs_unmount_f()
{
  int retval = 0;
  retval = f_unmount("");

  return retval;

}
/*
FRESULT scan_files_1 (
    char* path        // Start node to be scanned (***also used as work area***)/
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       // Open the directory /
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   // Read a directory item /
            if (res != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir/
            if (fno.fattrib & AM_DIR) {                    // It is a directory /
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files_1(path);                    // Enter the directory
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       // It is a file.
                printf("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}
*/
int fs_bsp_init()
{
  int retval = 0;
  int fs_init_flag=0;
    CMU_ClockEnable(cmuClock_HFPER, true);

    BSP_SLSTK3701A_SDIO_HWInit();

    /* Initialize filesystem */
    //  res = f_mount(0, &Fatfs,1);
    retval = f_mount(&Fatfs,"", 1);
    if (retval != FR_OK)
    {
      printf("FAT-mount failed: %d\n",retval );
      fs_init_flag = 0;
      return retval;
    }
    else
    {
      printf("FAT-mount successful\n");
    }

    fs_init_flag = 1;

    return retval;
}

int fs_ls_cmd_f(uint32_t tag, TCHAR* PATH_T)
{
  int retval = 0;

  bool loption = false;
  char *option = strtok(NULL, " ");


        if (option != NULL && !strcmp(option, "-l"))
        {
            loption = true;
        }
        strcpy(path, PATH_T);
      scan_files(path, loption, tag);
//        scan_files_1(path);

  return retval;

}

int fs_cat_f(char* fn)
{
  int res = 0;

  /* Get first argument */
  char *fileName = fn;

  if (fileName == (char *) NULL)
  {
      res = 1;
    printf("cat: Missing argument\n");
    printf("usage: cat <file>");
    return res;
  }

  res = f_open(&fh, fileName, FA_READ);
  if (res == FR_OK)
  {
    printf("Content of file %s\n", fileName);
    printf("-----------------------------------------\n");
    f_rewind(&fh);
    res = f_read(&fh, buffer, CBUFSIZE, &bufRead);
    if (res == FR_OK)
    {
      PrintBuf(buffer, bufRead);
    } else
    {
      printf("Read Failure: %d\n", res);
    }
    f_close(&fh);
  } else
  {
    printf("Failed to open %s, error %u\n", fileName, res);
  }
  return res;
}

FRESULT fs_rm_cmd_f(char* fn)
{
  FRESULT retval;
  char *fileName = fn;
  if (fileName == NULL)
  {
    printf("rm: Missing file name\n");
    printf("usage: rm <file>");
    return retval;
  }
  retval = f_unlink(fileName);
  if (retval != FR_OK)
  {
    printf("rm %s failed, error %u\n", fileName, retval);
  }
  return retval;
}

FRESULT fs_mkdir_cmd_f (char* dirname)
{
  char *dir_name = dirname;
  FRESULT retval = 0;

  if (dir_name == NULL)
  {
    printf("mkdir: missing name of directory\n");
    printf("usage: mkdir dirname\n");
    return 1;
  }
  retval = f_mkdir(dir_name);

  if (retval != FR_OK)
  {
    printf("mkdir %s failed, error %u\n", dir_name, retval);
  }
  return retval;
}

FRESULT fs_mv_cmd_f (char* old_fn, char* new_fn)
{
  char *old = old_fn;
  char *new = new_fn;
  FRESULT retval;

  if ((old == NULL) || (new == NULL))
  {
    printf("usage: mv <old> <new>");
    return 1;
  }
  retval = f_rename(old, new);
  if (retval != FR_OK)
  {
    printf("mv %s %s failed, error %u\n", old, new, retval);
  }
  return retval;
}

FRESULT fs_cd_cmd_f(char* path_c){
  FRESULT retval;
  retval = f_chdir(path_c);

  TCHAR curr_d [SZ_STR];

  if (retval != FR_OK)
    {
      printf("cd %s  failed!! Error %u\n", path_c, retval);

    }

  else {
      if(strcmp(path_c,"/")==0){
          strcpy(Curr_Addr,"/");
      }else if(strcmp(path_c,"..")==0){
          int index_slash =0;
          for(int i =strlen(Curr_Addr);i>=0;i--){
              if(index_slash==0&&Curr_Addr[i]=='/'){
                  index_slash++;
                  continue;
              }
              if(Curr_Addr[i]=='/'){
                  Curr_Addr[i+1] = '\0';
                  if(Curr_Addr[0] == '\0'){
                      Curr_Addr[0] = '/';
                      Curr_Addr[1] = '\0';
                  }
                  break;
              }
          }
          retval = f_chdir(Curr_Addr);
      }else{
          if(strcmp(path_c,".")==0){
              printf(". is not recognized!!!\n");
          }else{
              strcat(Curr_Addr,path_c);
              strcat(Curr_Addr,"/");
          }

      }
      return retval;
  }
}
void fs_pwd_cmd_f(){
  printf("%s\n",Curr_Addr);
  return;
}
FRESULT copy_file (char* src, char* dst) {
    FIL srcFile, dstFile;     /* File objects */
    FRESULT fr;               /* FatFs function common result code */
    UINT br, bw;              /* File read/write count */
    BYTE buffer[512];        /* File copy buffer */
    uint64_t temptime, ms_n; //testing

    /* Open source file on the drive 1 */
    fr = f_open(&srcFile, src, FA_READ);
    if (fr) return fr;

    /* Create destination file on the drive 0 */
    fr = f_open(&dstFile, dst, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr) return fr;

    /* Copy source to destination file */

    for (;;) {
        temptime = sl_sleeptimer_get_tick_count64();

        fr = f_read(&srcFile, buffer, 512, &br);  /* Read a chunk of source file */
        if (fr || br == 0) break; /* error or eof */



        fr = f_write(&dstFile, buffer, br, &bw);            /* Write it to the destination file */
        if (fr || bw < br) break; /* error or disk full */

        uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
        sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
    }
    /* Close open files */


    f_close(&srcFile);
    f_close(&dstFile);



    return fr;
}


FRESULT copy_directory(char* src, char* dst) {
    FRESULT fr;               /* FatFs function common result code */
    DIR dir;                  /* Directory object */
    FILINFO fno;              /* File information object */
    char srcPath[256], dstPath[256];

    /* Open the source directory */
    fr = f_opendir(&dir, src);
    if (fr) return fr;

    /* Create the destination directory */
    fr = f_mkdir(dst);
    if (fr) return fr;

    /* Read all files and subdirectories */
    for (;;) {
        fr = f_readdir(&dir, &fno);                   /* Read a directory item */
        if (fr || fno.fname[0] == 0) break;  /* Break on error or end of dir */

        /* Create source and destination paths */
        sprintf(srcPath, "%s/%s", src, fno.fname);
        sprintf(dstPath, "%s/%s", dst, fno.fname);

        if (fno.fattrib & AM_DIR) {                    /* It is a directory */
            /* Recursive copy */
            fr = copy_directory(srcPath, dstPath);
            if (fr) break;
        } else {                                       /* It is a file */
            /* Copy file */
            fr = copy_file(srcPath, dstPath);
            if (fr) break;
        }
    }

    f_closedir(&dir);

    return fr;
}
int is_directory(const char *path) {
    FILINFO fno;
    if (f_stat(path, &fno) != FR_OK)
        return 0;
    return (fno.fattrib & AM_DIR);
}
FRESULT fs_cp_cmd_f(char* f_name,char* n_name){

  if(is_directory(f_name)){
      copy_directory(f_name,n_name);
  }else{
      copy_file(f_name,n_name);

  }
}
