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
#include "em_adc.h"
#include "mod_fatfs_chan.h"
#include "unit_test.h"
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
static uint32_t rxDataReady = 0;  // Flag indicating receiver does not have data
static volatile char rxBuffer[20]; // Software receive buffer
static char txBuffer[20]; // Software transmit buffer
char read_buffer[30];
int read_request_bytes = 30;
unsigned int read_bytes_recieved = 0;
unsigned int file_num = 0;
static sl_sleeptimer_timer_handle_t periodic_timer;
#define TIMEOUT_MS 2
#define adcFreq         16000000
uint32_t sample;
uint32_t millivolts;
void
ADC_tas ();
uint64_t tick_cnt = 0;
uint64_t timestamp;
sl_status_t stat;
int retval = 0;
int total_bytes_written;
int write_request_bytes;
int write_bytes_written;
RTOS_ERR RT_ERR;
OS_TCB ADC_TaskTCB;
OS_TASK_PTR ADC_task;
#define   ADC_TASK_STK_SIZE 256u
#define RX_BUFFER_SIZE 20
CPU_STK ADC_TaskStk[ADC_TASK_STK_SIZE];
;
FIL superfile;
uint64_t time_offset = 0;
uint32_t data_entries = 0;
uint64_t new_time;
uint64_t new_tick_cnt;
uint64_t descrep;
int reach_flag = 0;
int testing_flag =0;

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef BUFSIZE
#define BUFSIZE    80
#endif

#ifndef TERMINAL_TASK_STACK_SIZE
#define TERMINAL_TASK_STACK_SIZE      256
#endif

#ifndef TERMINAL_TASK_PRIO
#define TERMINAL_TASK_PRIO            20
#endif
#define CLI_DEMO_TASK_STACK_SIZE     256
#define CLI_DEMO_TASK_PRIO            15
FIL superfile;
uint32_t bytes_read;
char file_name[] = "ADC_Data_0000.txt";


struct adc_data
{
  uint64_t t_stamp;
  uint32_t voltage;
};

struct adc_data *cirbuff;
int front = 0;
int back = 160;
//uint8_t* buffer_t;
volatile uint8_t buffer_t[52100];
#define BTL_PARSER_CTX_SZ  0x200
static uint8_t parserContext[BTL_PARSER_CTX_SZ];
static BootloaderParserCallbacks_t parserCallbacks;
uint32_t total_bytes = 0;
OS_TMR  App_Timer;
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
static OS_TCB tcb;
static CPU_STK stack[TERMINAL_TASK_STACK_SIZE];

/* Input buffer */
static char buffer[BUFSIZE];
void
app_iostream_terminal_task (void *arg);
/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void echo_str(sl_cli_command_arg_t *arguments);
void echo_int(sl_cli_command_arg_t *arguments);
void led_cmd();
void ls_cmd(sl_cli_command_arg_t *arguments);
void mk_dir(sl_cli_command_arg_t *arguments);
void rm_cmd(sl_cli_command_arg_t *arguments);
void mv_cmd(sl_cli_command_arg_t *arguments);
void enable_sd(sl_cli_command_arg_t *arguments);
void exit_sd();
void cd_cmd(sl_cli_command_arg_t *arguments);
void pwd_cmd();
void adc_cmd (sl_cli_command_arg_t *arguments);
void pause_cmd();
void unit_test();
void cp_cmd(sl_cli_command_arg_t *arguments);
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
                 "Nothing",
                 { SL_CLI_ARG_END, });

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
                 "Optional SD commands",
                 { SL_CLI_ARG_WILDCARD,SL_CLI_ARG_END, });
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
static const sl_cli_command_info_t cmd__adc = \
  SL_CLI_COMMAND(adc_cmd,
                 "Start ADC with Optional Time Offset",
                 "Optional Time Offset",
                 { SL_CLI_ARG_INT32OPT,SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__pause = \
    SL_CLI_COMMAND(pause_cmd,
                     "Stop ADC",
                     "None",
                     {SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__test = \
    SL_CLI_COMMAND(unit_test,
                     "Unit Testing",
                     "None",
                     {SL_CLI_ARG_END, });
static const sl_cli_command_info_t cmd__cp = \
    SL_CLI_COMMAND(cp_cmd,
                     "make a copy of file or directory",
                     "Original File"SL_CLI_UNIT_SEPARATOR"new File Name",
                     {SL_CLI_ARG_STRING,SL_CLI_ARG_STRING,SL_CLI_ARG_END, });
static sl_cli_command_entry_t a_table[] = {
  { "bootload_uart", &cmd__echostr,false },
  { "bootload_sd", &cmd__echoint, false },
  { "led", &cmd__led, false },
  { "set_time", &cmd__adc, false },
  {"pause", &cmd__pause,false},
  { "sd", &cmd__sd,false},
  {"test", &cmd__test,false},
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
  {"cp", &cmd__cp,false},
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
void led_cmd()
{
  testing_flag =1;
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

  if(testing_flag==0){
      printf("SD");
  }

}
void mk_dir(sl_cli_command_arg_t *arguments){
  char* close;
  close = sl_cli_get_argument_string(arguments,0);
  fs_mkdir_cmd_f(close);
  if(testing_flag==0){
      printf("SD");
  }

}
void rm_cmd(sl_cli_command_arg_t *arguments){
  char* close;
  close = sl_cli_get_argument_string(arguments,0);
  fs_rm_cmd_f(close);
  if(testing_flag==0){
      printf("SD");
  }

}
void mv_cmd(sl_cli_command_arg_t *arguments){
  char* n_name;
  char* o_name;
  n_name =  sl_cli_get_argument_string(arguments,1);
  o_name = sl_cli_get_argument_string(arguments,0);
  fs_mv_cmd_f(o_name,n_name);
  if(testing_flag==0){
      printf("SD");
  }

}
void enable_sd(sl_cli_command_arg_t *arguments){
  RTOS_ERR     err;
  CPU_BOOLEAN  deleted;
  CPU_BOOLEAN  stopped;
  CPU_BOOLEAN  started;
  int tmp_flag_pr =0;
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
  if(sl_cli_get_argument_string(arguments,0)!= NULL){
      tmp_flag_pr =1;

      char* tmp_cmd = sl_cli_get_argument_string(arguments,0);
      if(strcmp(tmp_cmd,"ls")==0){
          sl_cli_command_arg_t args;
          args.handle = sl_cli_inst_handle;
          if(sl_cli_get_argument_count(arguments)==2){
              args.argc = 2;
              args.argv = malloc(args.argc * sizeof(void*));
              args.argv[0] = "ls";
              args.argv[1] = sl_cli_get_argument_string(arguments,1);
              args.arg_ofs = 1;
          }else if(sl_cli_get_argument_count(arguments)==3){
              args.argc = 3;
              args.argv = malloc(args.argc * sizeof(void*));
              args.argv[0] = "ls";
              args.argv[1] = sl_cli_get_argument_string(arguments,1);
              args.argv[2] = sl_cli_get_argument_string(arguments,2);
              args.arg_ofs = 1;
          }else{
              args.argc = 1;
              args.argv = malloc(args.argc * sizeof(void*));
              args.argv[0] = "ls";
              args.arg_ofs = 1;
          }
          ls_cmd(&args);

      }else if(strcmp(tmp_cmd,"cp")==0){
          sl_cli_command_arg_t args;
          args.handle = sl_cli_inst_handle;
          args.argc = 3;
          args.argv = malloc(args.argc * sizeof(void*));
          args.argv[0] = "cp";
          args.argv[1] = sl_cli_get_argument_string(arguments,1);
          args.argv[2] = sl_cli_get_argument_string(arguments,2);
          args.arg_ofs = 1;
          cp_cmd(&args);
      }else if(strcmp(tmp_cmd,"mv")==0){
          sl_cli_command_arg_t args;
          args.handle = sl_cli_inst_handle;
          args.argc = 3;
          args.argv = malloc(args.argc * sizeof(void*));
          args.argv[0] = "mv";
          args.argv[1] = sl_cli_get_argument_string(arguments,1);
          args.argv[2] = sl_cli_get_argument_string(arguments,2);
          args.arg_ofs = 1;
          mv_cmd(&args);
      }else if(strcmp(tmp_cmd,"rm")==0){
          sl_cli_command_arg_t args;
          args.handle = sl_cli_inst_handle;
          args.argc = 2;
          args.argv = malloc(args.argc * sizeof(void*));
          args.argv[0] = "rm";
          args.argv[1] = sl_cli_get_argument_string(arguments,1);
          args.arg_ofs = 1;
          rm_cmd(&args);
      }else if(strcmp(tmp_cmd,"mkdir")==0){
          sl_cli_command_arg_t args;
          args.handle = sl_cli_inst_handle;
          args.argc = 2;
          args.argv = malloc(args.argc * sizeof(void*));
          args.argv[0] = "mkdir";
          args.argv[1] = sl_cli_get_argument_string(arguments,1);
          args.arg_ofs = 1;
          mk_dir(&args);
      }
      else{
          tmp_flag_pr =0;

      }

  }
  if(testing_flag==0&&tmp_flag_pr!=1){
      printf("SD");
  }

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
void
file_setup (void)
{
  int num_write = 0;
  unsigned int nwritten = 0;
  char header_txt[25] = "I don't know the format.\n";
  int write_request_bytes = 0;
  int read_request_bytes = 24;
  int total_bytes_written = 0;
  unsigned int write_bytes_written = 0;
  unsigned int read_bytes_recieved = 0;

  //create testfile on sd card and write a test string
  if(file_num==0){
        retval = f_open (&superfile, file_name,
          FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
  }else{
      retval = f_open (&superfile, file_name,
                       FA_OPEN_APPEND | FA_WRITE | FA_READ);
  }

  if (retval != FR_OK)
    {
      printf ("Failed to open %s, error %u\n", file_name, retval);
    }

  retval = f_close (&superfile);
  if (retval != FR_OK)
    {
      printf ("Failed to close %s, error %u\n", file_name, retval);
    }

  //closing and reopening is taxing- but we'd like to write to the file without overwriting
  //each data entry. opening in FA_OPEN_APPEND lets us write multiple lines to the file
  retval = f_open (&superfile, file_name, FA_OPEN_APPEND | FA_WRITE | FA_READ);
  if (retval != FR_OK)
    {
      printf ("Failed to open %s, error %u\n", file_name, retval);
    }

  return;
}

void
app_iostream_usart_init (void)
{
  RTOS_ERR err;

  /* Prevent buffering of output/input.*/
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf (stdout, NULL, _IONBF, 0); /*Set unbuffered mode for stdout (newlib)*/
  setvbuf (stdin, NULL, _IONBF, 0); /*Set unbuffered mode for stdin (newlib)*/
#endif
  fs_bsp_init (); //SD Card Set-up
  file_setup (); // First file initialization

  // Enable ADC0 clock
  CMU_ClockEnable (cmuClock_ADC0, true);
  CMU_ClockSelectSet (cmuClock_LFE, cmuSelect_LFRCO);
  CMU_ClockEnable (cmuClock_RTCC, true);
  // Declare init structs
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;

  // Modify init structs and initialize
  init.prescale = ADC_PrescaleCalc (adcFreq, 0); // Init to max ADC clock for Series 1

  initSingle.diff = false;        // single ended
  initSingle.reference = adcRef2V5;    // internal 2.5V reference
  initSingle.resolution = adcRes12Bit;  // 12-bit resolution
  initSingle.acqTime = adcAcqTime4; // set acquisition time to meet minimum requirement

  // Expansion Header Pin #6
  initSingle.posSel = adcPosSelAPORT4XCH11;
  init.timebase = ADC_TimebaseCalc (0);

  //Initialize ADC
  ADC_Init (ADC0, &init);
  ADC_InitSingle (ADC0, &initSingle);

  //Add task to system
  OSTaskCreate (&tcb, "iostream terminal task", app_iostream_terminal_task,
  DEF_NULL,
                TERMINAL_TASK_PRIO, &stack[0], (TERMINAL_TASK_STACK_SIZE / 10u),
                TERMINAL_TASK_STACK_SIZE,
                0u, 0u,
                DEF_NULL,
                (OS_OPT_TASK_STK_CLR ), &err);
  EFM_ASSERT((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE));
}
static void
on_periodic_timeout (sl_sleeptimer_timer_handle_t *handle, void *data)
{
  if (front == 160)
    {
      front = 0;
    }
  (void) &handle;
  (void) &data;
  reach_flag = 1;
  ADC_Start (ADC0, adcStartSingle);
  while (!(ADC0->STATUS & _ADC_STATUS_SINGLEDV_MASK))
    ;
  sample = ADC_DataSingleGet (ADC0);
  // Calculate input voltage in mV
  millivolts = (sample * 2500) / 4096;
  tick_cnt = sl_sleeptimer_get_tick_count64 ();
  stat = sl_sleeptimer_tick64_to_ms (tick_cnt, &timestamp);
  uint64_t new_t = timestamp + time_offset * 1000;
  (cirbuff + front)->t_stamp = new_t;
  (cirbuff + front)->voltage = millivolts;
  front++;


}
void
app_iostream_terminal_task (void *arg)
{

  RTOS_ERR err;
  //Set default iostream to be expansion header 12,14
  //Start the periodic timer of 200 ms. Trigger timeout callback function.
  sl_sleeptimer_start_periodic_timer_ms (
      &periodic_timer,
      TIMEOUT_MS,
      on_periodic_timeout, NULL, 0,
      0);
  char* output =  malloc(512*sizeof output);

  while (true)
    {
      uint64_t new_t;
      uint32_t millivolt;
      if (back == 160)
        {
          //check whether reached end of cirbuff
          //return to beginning if reached
          back = 0;
        }

      while (true)
        {
          if(back==front){
              OSTimeDly( 1,
                         OS_OPT_TIME_DLY,
                         &err);
          }
          else{
              //if there is no new data.
              /**OSTimeDly( 1,
               OS_OPT_TIME_DLY,
               &err);**/
              new_t = (cirbuff + back)->t_stamp;
              millivolt = (cirbuff + back)->voltage;
              //divide the 64 bit int to two to correctly print
              uint32_t first_t, second_t;
              first_t = (uint32_t) (new_t >> 32);
              second_t = (uint32_t) new_t;
              snprintf (output + ((back %16) * 32), 34, "Time: %08x%08x mV: %04d\n", first_t,
                        second_t, millivolt);
              back++;
          }

          if(back!=0&&back%16==0){
              break;
          }

        }

      //read from circular buffer

      //divide the 64 bit int to two to correctly print
      uint32_t first_t, second_t;
      first_t = (uint32_t) (new_t >> 32);
      second_t = (uint32_t) new_t;
      //write to the string to be used for SD card write
      //print to iostream
     printf ("Time: %08x%08x mV: %04d\n", first_t, second_t, millivolts);
      if (data_entries+160 >= 32000)
        {
          //if a file has 30000 data entries, close that and initialize a new one
          testing_flag=0;
          f_close (&superfile);
          file_num++;

          snprintf (file_name, 18, "ADC_Data_%04d.txt", file_num);
          retval = f_open (&superfile, file_name,
          FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
          data_entries = 0;
          retval = f_open (&superfile, file_name,
          FA_OPEN_APPEND | FA_WRITE | FA_READ);

        }
      else
        {
          data_entries+=16;
          //if there is still space, open the file to append at the  end


        }
      if (retval != FR_OK)
        {
          printf ("Failed to open %s, error %u\n", file_name, retval);
        }
      //write the string to file

      write_request_bytes = 512;

          retval = f_write (&superfile, output, write_request_bytes,
                            &write_bytes_written);



      if (retval != FR_OK)
        {
          printf ("Failed to write %s, error %u\n", file_name, retval);
        }

    }

}
void adc_cmd (sl_cli_command_arg_t *arguments){
  uint64_t argument_value;
    RTOS_ERR err;
    // Read all the arguments provided as integers and print them back

    argument_value = (uint64_t) sl_cli_get_argument_int32(arguments, 0);

    time_offset = argument_value;
    GPIO_PinModeSet (gpioPortE, 9, gpioModeInput, 0);    // RX
    //init the cirbuff, able to hold 20 struct adc_data
    cirbuff = malloc (160 * sizeof *cirbuff);
    app_iostream_usart_init ();
}
void pause_cmd(){
  RTOS_ERR err;
 // f_close (&superfile);
  OSTaskDel(&tcb,&err);
  printf("Tensionmeter Paused!\n");
}
void cp_cmd(sl_cli_command_arg_t *arguments){
  char* n_name;
  char* o_name;
  n_name =  sl_cli_get_argument_string(arguments,1);
  o_name = sl_cli_get_argument_string(arguments,0);
  fs_cp_cmd_f(o_name,n_name);
  if(testing_flag==0){
      printf("SD");
  }
}
void test_ls_cmd(void) {
  uint64_t temptime, ms_n;

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 0;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "ls";
    args.arg_ofs = 0;

    temptime = sl_sleeptimer_get_tick_count64();

    ls_cmd(&args);
    uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
        sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
        printf("Time for ls: %d\n", (uint32_t)ms_n);
    // Check the results
    // ...

}

void test_mk_dir(void) {
  uint64_t temptime, ms_n;

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 2;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "mkdir";
    args.argv[1] = "aaad";
    args.arg_ofs = 1;

    temptime = sl_sleeptimer_get_tick_count64();

    mk_dir(&args);
    uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
        sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
        printf("Time for mkdir: %d\n", (uint32_t)ms_n);
    // Check the results
    // ...

}

void test_rm_cmd(void) {
  uint64_t temptime, ms_n;

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 2;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "rm";
    args.argv[1] = "aaad";
    args.arg_ofs = 1;

    temptime = sl_sleeptimer_get_tick_count64();

    rm_cmd(&args);
    uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
        sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
        printf("Time for rm: %d\n", (uint32_t)ms_n);
    // Check the results
    // ...

}

void test_mv_cmd(void) {
  uint64_t temptime, ms_n;

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 3;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "mv";
    args.argv[1] = "aaad";
    args.argv[2] = "aas";
    args.arg_ofs = 1;

    temptime = sl_sleeptimer_get_tick_count64();

    mv_cmd(&args);
    uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
        sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
        printf("Time for mv: %d\n", (uint32_t)ms_n);
    // Check the results
    // ...

}

void test_cd_cmd(void) {
    uint64_t temptime, ms_n;

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 2;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "cd";
    args.argv[1] = "aas";
    args.arg_ofs = 1;
    args.arg_type_list = malloc((args.argc -1) * sizeof(sl_cli_argument_type_t));

    temptime = sl_sleeptimer_get_tick_count64();

    cd_cmd(&args);
    uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
    sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
    printf("Time for cd: %d\n", (uint32_t)ms_n);
    // Check the results
    // ...

}

void test_pwd_cmd(void) {
    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 1;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "pwd";
    args.arg_ofs = 0;

    pwd_cmd(&args);

    // Check the results
    // ...

}

void test_wr_cmd(void){
  uint64_t temptime, ms_n;

  uint32_t br,bw;
  FRESULT fr;
  char dst[] = "test.txt";
  fr = f_open(&superfile, dst, FA_CREATE_ALWAYS | FA_WRITE);
  char str[512] = {0};

  temptime = sl_sleeptimer_get_tick_count64();

  fr = f_write(&superfile,str,512,&bw);

  uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
  sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
  printf("Time for write: %d\n", (uint32_t)ms_n);
  f_close(&superfile);
  fr = f_open(&superfile, dst, FA_READ);

  temptime = sl_sleeptimer_get_tick_count64();

  fr = f_read(&superfile,str,512,&bw);

   mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
  sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
  f_close(&superfile);

  printf("Time for read: %d\n", (uint32_t)ms_n);
}

void clean_tests(){

    sl_cli_command_arg_t args;
    args.handle = sl_cli_inst_handle;
    args.argc = 2;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "rm";
    args.argv[1] = "test.txt";
    args.arg_ofs = 1;
    rm_cmd(&args);

      args.handle = sl_cli_inst_handle;
      args.argc = 2;
      args.argv = malloc(args.argc * sizeof(void*));
      args.argv[0] = "rm";
      args.argv[1] = "test_new.txt";
      args.arg_ofs = 1;

    rm_cmd(&args);
    args.handle = sl_cli_inst_handle;
    args.argc = 2;
    args.argv = malloc(args.argc * sizeof(void*));
    args.argv[0] = "cd";
    args.argv[1] = "/";
    args.arg_ofs = 1;
    args.arg_type_list = malloc((args.argc -1) * sizeof(sl_cli_argument_type_t));


    cd_cmd(&args);


      args.handle = sl_cli_inst_handle;
      args.argc = 2;
      args.argv = malloc(args.argc * sizeof(void*));
      args.argv[0] = "rm";
      args.argv[1] = "aaa";
      args.arg_ofs = 1;


      rm_cmd(&args);


        args.handle = sl_cli_inst_handle;
        args.argc = 2;
        args.argv = malloc(args.argc * sizeof(void*));
        args.argv[0] = "rm";
        args.argv[1] = "aas";
        args.arg_ofs = 1;


        rm_cmd(&args);

}
void test_cp_cmd (){
  sl_cli_command_arg_t args;
  uint64_t temptime, ms_n;

  args.handle = sl_cli_inst_handle;
  args.argc = 3;
  args.argv = malloc(args.argc * sizeof(void*));
  args.argv[0] = "cp";
  args.argv[1] = "test.txt";
  args.argv[2] = "test_new_new.txt";

  args.arg_ofs = 1;
  temptime = sl_sleeptimer_get_tick_count64();

  cp_cmd(&args);
  uint64_t mkdirtime = sl_sleeptimer_get_tick_count64()-temptime;
 sl_sleeptimer_tick64_to_ms(mkdirtime, &ms_n);
 printf("Time for copy: %d\n", (uint32_t)ms_n);

}
void unit_test(){
  printf("Testing All Functionalities... Avoid Name: aaad, aas, aaa to be used!\n");
  testing_flag=1;
  sl_cli_command_arg_t args;
      args.handle = sl_cli_inst_handle;
      args.argc =1;
      args.argv = malloc(args.argc * sizeof(void*));
      args.argv[0] = "sd";
      args.arg_ofs = 1;

  enable_sd(&args);
/**
  sl_cli_command_arg_t args;
      args.handle = sl_cli_inst_handle;
      args.argc = 2;
      args.argv = malloc(args.argc * sizeof(void*));
      args.argv[0] = "mkdir";
      args.argv[1] = "aaa";
      args.arg_ofs = 1;
      mk_dir(&args);

  test_mk_dir();
  test_ls_cmd();
  test_mv_cmd();
  test_ls_cmd();
  test_mk_dir();
  test_ls_cmd();
  test_rm_cmd();
  test_ls_cmd();
  test_cd_cmd();
  test_ls_cmd();
  test_wr_cmd();**/
  test_cp_cmd();
  //clean_tests();
  printf("Verify result by yourself! Break point if needed!\n");
  printf("Tests completed.\n");
  testing_flag=0;
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
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFRCO);
  CMU_ClockEnable(cmuClock_RTCC, true);
  sl_status_t  status_sl = sl_sleeptimer_init();
    if(status_sl != SL_STATUS_OK) {
      printf("Sleeptimer init error.\r\n");
    }
  printf("\r\nStarted CLI Bootload Example\r\n\r\n");
  printf("BASE");


  //printf("size of buffer: %d\n",sizeof buffer_t);


}
