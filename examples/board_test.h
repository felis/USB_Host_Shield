/* USB Host Shield board test sketch header */
#ifndef _BOARD_TEST_H_
#define _BOARD_TEST_H_

/* PGMSPACE */
#include <inttypes.h>
#include <avr/pgmspace.h>

/* Start banner */
const char startBanner  [] PROGMEM = "\r\nCircuits At Home 2010"
                                      "\r\nUSB Host Shield QC test routine\r\n";
                                      
const char anykey_msg [] PROGMEM =  "\r\nPress any key to continue...";
                                      
/* Test messages */

const char reset_msg [] PROGMEM = "\r\nPress RESET to restart test";

const char testpassed_msg [] PROGMEM = "\r\nTest PASSED";
const char testfailed_msg [] PROGMEM = "\r\nTest FAILED!!!";

const char revregcheck_begin_msg [] PROGMEM = "\r\nReading REVISION register...Die revision ";
const char spitest_begin_msg [] PROGMEM = "\r\nSPI test. Each  '.' indicates 64K transferred. Stops after transferring 1MB (16 dots)\r\n";
const char osctest_begin_msg [] PROGMEM = "\r\nOscillator start/stop test.";
const char osctest_oscstate_msg [] PROGMEM = " Oscillator state is ";
const char osctest_reset_msg [] PROGMEM = "\r\nSetting CHIP RESET.";
const char osctest_release_msg [] PROGMEM = "\r\nClearing CHIP RESET. ";
const char osctest_PLLstable_msg [] PROGMEM = "PLL is stable. Time to stabilize - ";
const char gpiocheck_begin_msg [] PROGMEM = "\r\nChecking GPIO lines. Install GPIO loopback adapter and press any key to continue...";

const char usbtest_begin_msg [] PROGMEM = "\r\nUSB Connectivity test. Waiting for device connection... ";
const char usbtest_device_reset_msg [] PROGMEM ="\r\nDevice connected. Resetting";
const char usbtest_waitsof_msg [] PROGMEM = "\r\nReset complete. Waiting for the first SOF...";
const char usbtest_enu_msg [] PROGMEM = "\r\nSOF generation started. Enumerating device.";
const char usbtest_addr_msg [] PROGMEM  = "\r\nSetting device address";
const char usbtest_getdevdescr_msg [] PROGMEM = "\r\nGetting device descriptor";

/* Get device descriptor strings */
//const char Gen_Error_str[] PROGMEM = "\r\nRequest error. Error code:\t";
const char Dev_Header_str[] PROGMEM ="\r\nDevice descriptor: ";
const char Dev_Length_str[] PROGMEM ="\r\nDescriptor Length:\t";
const char Dev_Type_str[] PROGMEM ="\r\nDescriptor type:\t";
const char Dev_Version_str[] PROGMEM ="\r\nUSB version:\t";
const char Dev_Class_str[] PROGMEM ="\r\nDevice class:\t";
const char Dev_Subclass_str[] PROGMEM ="\r\nDevice Subclass:\t";
const char Dev_Protocol_str[] PROGMEM ="\r\nDevice Protocol:\t";
const char Dev_Pktsize_str[] PROGMEM ="\r\nMax.packet size:\t";
const char Dev_Vendor_str[] PROGMEM ="\r\nVendor ID:\t";
const char Dev_Product_str[] PROGMEM ="\r\nProduct ID:\t";
const char Dev_Revision_str[] PROGMEM ="\r\nRevision ID:\t";
const char Dev_Mfg_str[] PROGMEM ="\r\nMfg.string index:\t";
const char Dev_Prod_str[] PROGMEM ="\r\nProd.string index:\t";
const char Dev_Serial_str[] PROGMEM ="\r\nSerial number index:\t";
const char Dev_Nconf_str[] PROGMEM ="\r\nNumber of conf.:\t";

/* General fail messages */

const char test_halted_msg [] PROGMEM = "\r\nTest Halted."
                                        "\r\n0x55 pattern is being transmitted via SPI to aid in troubleshooting";
                                        /* Test fail messages */
const char revregcheck_fail_msg [] PROGMEM = "\r\nError reading REVISION register, return value: ";
const char revregcheck_revision_invalid [] PROGMEM = "invalid. Value returned: ";
const char spitest_fail_msg [] PROGMEM = "\r\nSPI transmit/receive mismatch"
                                          "\r\nValue written: ";
const char spitest_fail_msg_2 [] PROGMEM = "Value read: ";                                          
const char usbtest_err_getdevdescr_msg [] PROGMEM = "\r\nError reading device descriptor. Error code ";
const char usbtest_error_state_msg [] PROGMEM = "\r\nUSB state machine reached error state";



#endif
