/***************************************************************************//**
 * @file
 * @brief cli micrium os kernel examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "cli.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"
#include "os.h"
#include "btl_interface.h"
#include "sdio.h"
#include "mod_fatfs_chan.h"
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define CLI_DEMO_TASK_STACK_SIZE     256
#define CLI_DEMO_TASK_PRIO            15
FIL superfile;
uint32_t bytes_read;
//uint8_t* buffer_t;
volatile uint8_t buffer_t[52100];
#define BTL_PARSER_CTX_SZ  0x200
static uint8_t parserContext[BTL_PARSER_CTX_SZ];
static BootloaderParserCallbacks_t parserCallbacks;
uint32_t total_bytes = 0;
OS_TMR  App_Timer;
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void echo_str(sl_cli_command_arg_t *arguments);
void echo_int(sl_cli_command_arg_t *arguments);
void led_cmd(sl_cli_command_arg_t *arguments);
void ls_cmd(sl_cli_command_arg_t *arguments);
void mk_dir(sl_cli_command_arg_t *arguments);
void rm_cmd(sl_cli_command_arg_t *arguments);
void mv_cmd(sl_cli_command_arg_t *arguments);
void enable_sd();
void exit_sd();
void cd_cmd(sl_cli_command_arg_t *arguments);
void pwd_cmd();
/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

static const sl_cli_command_info_t cmd__echostr = \
  SL_CLI_COMMAND(echo_str,
                 "echoes string arguments to the output",
                 "No argument",
                 {  SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__echoint = \
  SL_CLI_COMMAND(echo_int,
                 "echoes integer arguments to the output",
                 "No argument",
                 {  SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__led = \
  SL_CLI_COMMAND(led_cmd,
                 "Change an led status",
                 "led number: 0 or 1"SL_CLI_UNIT_SEPARATOR "instruction: on, off, or toggle",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__ls = \
  SL_CLI_COMMAND(ls_cmd,
                 "Print everything under current directory",
                 "Optional: flag"SL_CLI_UNIT_SEPARATOR "Optional: file path",
                 {  SL_CLI_ARG_WILDCARD,SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__mkdir = \
  SL_CLI_COMMAND(mk_dir,
                 "Create a new Directory",
                 "Directory Name",
                 {  SL_CLI_ARG_WILDCARD,SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__rm = \
  SL_CLI_COMMAND(rm_cmd,
                 "delete a Directory/File",
                 "Directory/File Name",
                 {  SL_CLI_ARG_WILDCARD,SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__mv = \
  SL_CLI_COMMAND(mv_cmd,
                 "Rename a File",
                 "old File Name"SL_CLI_UNIT_SEPARATOR"new File Name/directory",
                 {SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__sd = \
  SL_CLI_COMMAND(enable_sd,
                 "Enable SD CLI Commands",
                 "None",
                 { SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__exit = \
  SL_CLI_COMMAND(exit_sd,
                 "Disable SD CLI Commands",
                 "None",
                 { SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__cd = \
  SL_CLI_COMMAND(cd_cmd,
                 "Change Directory",
                 "Directory to change to",
                 { SL_CLI_ARG_STRINGOPT,SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__pwd = \
  SL_CLI_COMMAND(pwd_cmd,
                 "Print out Current Working Directory",
                 "Directory to change to",
                 { SL_CLI_ARG_END, });
static sl_cli_command_entry_t a_table[] = {
  { "bootload_uart", &cmd__echostr,false },
  { "bootload_sd", &cmd__echoint, false },
  { "led", &cmd__led, false },
  { "sd", &cmd__sd,false},
  { NULL, NULL, false },
};
static sl_cli_command_entry_t b_table[] = {
  {"cd",&cmd__cd,false},
  {"ls", &cmd__ls,false},
  {"mkdir", &cmd__mkdir,false},
  {"rm", &cmd__rm,false},
  {"mv", &cmd__mv, false},
  {"exit", &cmd__exit,false},
  {"pwd", &cmd__pwd,false},
  { NULL, NULL, false },
};
static sl_cli_command_group_t a_group = {
  { NULL },
  false,
  a_table
};
static sl_cli_command_group_t b_group = {
  { NULL },
  false,
  b_table
};
/*******************************************************************************
 *************************  EXPORTED VARIABLES   *******************************
 ******************************************************************************/

sl_cli_command_group_t *command_group = &a_group;
sl_cli_command_group_t *sd_group = &b_group;
/*******************************************************************************
 *************************   LOCAL FUNCTIONS   *********************************
 ******************************************************************************/
void  App_TimerCallback (void  *p_tmr,
                         void  *p_arg)
{
    /* Called when timer expires:                            */
    /*   'p_tmr' is pointer to the user-allocated timer.     */
    /*   'p_arg' is argument passed when creating the timer. */
  printf("Failed to Mount. Check SD card Presence!!!!\n");
  NVIC_SystemReset();
}
/***************************************************************************//**
 * bootload_uart
 *
 * bootload from uart terminal command and xmodem file transfer
 ******************************************************************************/
void echo_str(sl_cli_command_arg_t *arguments)
{
  bootloader_init();
  bootloader_eraseStorageSlot(0); // Have to erase the slot in order to enter uart mode if slot preoccupied.
  bootloader_rebootAndInstall();
}
#define MAX_METADATA_LENGTH   512
uint8_t metadata[MAX_METADATA_LENGTH];

void metadataCallback(uint32_t address, uint8_t *data, size_t length, void *context)
{
    uint8_t i;

    for (i = 0; i < MIN(length , MAX_METADATA_LENGTH - address); i++)
    {
        metadata[address + i] = data[i];
    }
}
/***************************************************************************//**
 * bootload_sd
 *
 * Bootload from a upgrade .gbl file on sd card
 ******************************************************************************/
void echo_int(sl_cli_command_arg_t *arguments)
{
  int32_t flag; //flag for bootloader
  BootloaderStorageSlot_t info_s; // info pointer for bootloader
  FRESULT retval; //return value for FAT FS
  uint32_t remain_bytes = 51200; //Byte Upper Limit for file

  memset(buffer_t,0,51200); //initialize the buffer with 0
  char file_name[] = "new_checker.gbl";//"new_checker.gbl"; // name of the file
  retval = f_open (&superfile, file_name,
                            FA_READ); //open the file
  if(retval!=FR_OK){
      printf("ERROR!!!ERROR!!!\n"); //Error Opening the file
  }
  printf("retval: %d\n",retval);
  int i=0;
  while(remain_bytes>0){//Read the file incrementally as FAT-FS can't handle all at once
      retval = f_read(&superfile,buffer_t+64*i,64,&bytes_read);

      total_bytes+=bytes_read;
      remain_bytes-=bytes_read;
      if(bytes_read==0){
          break;
      }
      i++;
  }
  if(retval!=FR_OK){
      printf("ERROR!!!ERROR!!!\n");
  }
  printf("bytes_read: %d\n",total_bytes);//Check with size of file
  flag=bootloader_init(); //init the bootloader
  printf("Init: %d\n",flag);


  bootloader_getStorageSlotInfo(0,&info_s); //get the storage info !not important

  flag=bootloader_eraseWriteStorage(0,0,buffer_t,total_bytes); // Erase the storage and start writing all into storage
  printf("Write Storage: %d\n",flag);

  if(bootloader_storageIsBusy()){
      osDelay(10);  //make sure it is done
  }
  bootloader_getStorageSlotInfo(0,&info_s); // Get the storage info !not important

   flag = bootloader_verifyImage(0,NULL); // Verify the image in the storage
   printf("Verify Image: %d\n",flag);


   bool flag_v = bootloader_verifyApplication(0x100000); // verify app
   printf("Verify App: %d\n",flag_v);


   flag=bootloader_setImageToBootload(0); // Select slot 0 as the place to run and reboot.
   printf("Set Image: %d\n",flag);





   osDelay(10);
  bootloader_rebootAndInstall();//reboot and start the new program.
}

/***************************************************************************//**
 * Callback for the led
 *
 * This function is used as a callback when the led command is called
 * in the cli. The command is used to turn on, turn off and toggle leds.
 ******************************************************************************/
void led_cmd(sl_cli_command_arg_t *arguments)
{
  int retval = 0;
   char file_name[] = "sbe_sim_sample.txt";
   int num_write = 0;
   unsigned int nwritten = 0;
   char header_txt[87] = "LWG data is formatted as follows: timestamp - hex encoded CTD data - hall rpm - adc\r\n";
   int write_request_bytes = 0;
   int read_request_bytes = 24;
   int total_bytes_written = 0;
   unsigned int write_bytes_written = 0;
   unsigned int read_bytes_recieved = 0;

   //create testfile on sd card and write a test string
   retval = f_open(&superfile, file_name, FA_CREATE_ALWAYS|FA_WRITE|FA_READ);
   if (retval != FR_OK)
   {
       printf("Failed to open %s, error %u\n", file_name, retval);
   }
   write_request_bytes = 87; //length of test_entry
   retval = f_write(&superfile, header_txt, write_request_bytes, &write_bytes_written);
   if (retval != FR_OK)
     {
         printf("Failed to write %s, error %u\n", file_name, retval);
     }
   retval = f_close(&superfile);
   if (retval != FR_OK)
     {
         printf("Failed to close %s, error %u\n", file_name, retval);
     }
   retval = f_open(&superfile, file_name, FA_READ);
      if (retval != FR_OK)
      {
          printf("Failed to open %s, error %u\n", file_name, retval);
      }
      char buf_read [87];
      uint32_t rae;
      retval = f_read(&superfile, buf_read, 87, &rae);
         if (retval != FR_OK)
           {
               printf("Failed to write %s, error %u\n", file_name, retval);
           }
         printf("BASE");
}
void ls_cmd(sl_cli_command_arg_t *arguments){
  char* close = sl_cli_get_argument_string(arguments,0);
  char* patha = "";
  if(2==sl_cli_get_argument_count(arguments)){
      close = sl_cli_get_argument_string(arguments,0);
      patha = sl_cli_get_argument_string(arguments,1);
      if(0==strcmp(close,"-h")){
          fs_ls_cmd_f(2,patha);
      }else if(0==strcmp(close,"-l")){
          fs_ls_cmd_f(1,patha);
      }else if(0==strcmp(close,"-1")){
          fs_ls_cmd_f(0,patha);
      }else if( *close == '-'){
          printf("unrecognized command!!!\n");
      }
  }else if(1==sl_cli_get_argument_count(arguments)&&*close == '-'){
      close = sl_cli_get_argument_string(arguments,0);
      if(0==strcmp(close,"-h")){
          fs_ls_cmd_f(2,patha);
      }else if(0==strcmp(close,"-l")){
          fs_ls_cmd_f(1,patha);
      }else if(0==strcmp(close,"-1")){
          fs_ls_cmd_f(0,patha);
      }else if( *close == '-'){
          printf("unrecognized command!!!\n");
      }

  }else if (1==sl_cli_get_argument_count(arguments)&&*close != '-'){

      patha = sl_cli_get_argument_string(arguments,0);
      fs_ls_cmd_f(0,patha);

  }else if (0 == sl_cli_get_argument_count(arguments)){
      fs_ls_cmd_f(0,patha);
  }else{
      printf("Unrecognized Parameters!\n");
  }

  printf("SD");

}
void mk_dir(sl_cli_command_arg_t *arguments){
  char* close;
  close = sl_cli_get_argument_string(arguments,0);
  fs_mkdir_cmd_f(close);
  printf("SD");

}
void rm_cmd(sl_cli_command_arg_t *arguments){
  char* close;
  close = sl_cli_get_argument_string(arguments,0);
  fs_rm_cmd_f(close);
  printf("SD");

}
void mv_cmd(sl_cli_command_arg_t *arguments){
  char* n_name;
  char* o_name;
  n_name =  sl_cli_get_argument_string(arguments,1);
  o_name = sl_cli_get_argument_string(arguments,0);
  fs_mv_cmd_f(o_name,n_name);
  printf("SD");

}
void enable_sd(){
  RTOS_ERR     err;
  CPU_BOOLEAN  deleted;
  CPU_BOOLEAN  stopped;
  CPU_BOOLEAN  started;
  OSTmrCreate(&App_Timer,            /*   Pointer to user-allocated timer.     */
                  "App Timer",           /*   Name used for debugging.             */
                     100,                  /*     0 initial delay.                   */
                   0,                  /*   100 Timer Ticks period.              */
                   OS_OPT_TMR_ONE_SHOT ,  /*   Timer is periodic.                   */
                  &App_TimerCallback,    /*   Called when timer expires.           */
                   DEF_NULL,             /*   No arguments to callback.            */
                  &err);
  OSTimeDly( 10,              /*   Delay the task for 100 OS Ticks.         */
               OS_OPT_TIME_DLY,  /*   Delay is relative to current time.       */
              &err);
  started = OSTmrStart(&App_Timer,  /*   Pointer to user-allocated timer.      */
                       &err);
  OSTimeDly( 10,              /*   Delay the task for 100 OS Ticks.         */
               OS_OPT_TIME_DLY,  /*   Delay is relative to current time.       */
              &err);
  if (err.Code != RTOS_ERR_NONE) {
      printf("Error in Timer");
  }

  fs_bsp_init (); //SD Card Set-up
  stopped = OSTmrStop(&App_Timer,        /*   Pointer to user-allocated timer. */
                        OS_OPT_TMR_NONE,  /*   Do not execute callback.         */
                        DEF_NULL,         /*   No arguments to callback.        */
                       &err);
   if (err.Code != RTOS_ERR_NONE) {
       printf("Error in Timer");
   }
  deleted = OSTmrDel(&App_Timer,  /*   Pointer to user-allocated timer.        */
                        &err);
     if (err.Code != RTOS_ERR_NONE) {
         printf("Error in Timer");
     }
  bool status;
  status = sl_cli_command_add_command_group(sl_cli_inst_handle, sd_group);
  EFM_ASSERT(status);
  status = sl_cli_command_remove_command_group(sl_cli_inst_handle, command_group);
  EFM_ASSERT(status);
  OSTimeDly( 10,              /*   Delay the task for 100 OS Ticks.         */
               OS_OPT_TIME_DLY,  /*   Delay is relative to current time.       */
              &err);
  printf("SD");

}
void exit_sd(){
  bool status;

  status = sl_cli_command_add_command_group(sl_cli_inst_handle, command_group);
  EFM_ASSERT(status);

  status = sl_cli_command_remove_command_group(sl_cli_inst_handle, sd_group);
  EFM_ASSERT(status);
  fs_unmount_f();
  printf("BASE");

}

void cd_cmd(sl_cli_command_arg_t *arguments){

  char* pathc = sl_cli_get_argument_string(arguments,0);

  fs_cd_cmd_f(pathc);
}

void pwd_cmd(){
  fs_pwd_cmd_f();
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/*******************************************************************************
 * Initialize cli example.
 ******************************************************************************/
void cli_app_init(void)
{
  bool status;
  status = sl_cli_command_add_command_group(sl_cli_inst_handle, command_group);
  EFM_ASSERT(status);
  printf("\r\nStarted CLI Bootload Example\r\n\r\n");
  printf("BASE");


  //printf("size of buffer: %d\n",sizeof buffer_t);


}
