/* USB Host Shield Board test routine. Runs after assembly to check board functionality */

/* USB related */
#include <Spi.h>
#include <Max3421e.h>
#include <Max3421e_constants.h>
#include <Usb.h>

#include "board_test.h" /* Board test messages */

void setup();
void loop();

MAX3421E Max;
USB Usb;

void setup()
{
  Serial.begin( 9600 );
  Max.powerOn();
  printProgStr( startBanner );
  printProgStr( anykey_msg );
}

void loop()
{
  while( Serial.available() == 0 );  //wait for input
  Serial.read();                     //empty input buffer
  /* start tests */
  /* SPI short test */
  if (!revregcheck()) test_halted();
  /* GPIO test */
  if (!gpiocheck()) Serial.print("\r\nGPIO check failed. Make sure GPIO loopback adapter is installed");
  /* SPI long test */
  if (!spitest()) test_halted();      //test SPI for transmission errors
  if (!osctest()) Serial.print("OSCOK test failed. Check the oscillator");
  if (!usbtest()) Serial.print("USB connection test failed. Check traces from USB connector to MAX3421E, as well as VBUS");  //never gets here
    /* All tests passed */
  printProgStr( anykey_msg );
}

/* SPI short test. Checks connectivity to MAX3421E by reading REVISION register. */
/* Die rev.1 returns 0x01, rev.2 0x12, rev.3 0x13. Any other value is considered communication error */
bool revregcheck()
{
  byte tmpbyte;
  printProgStr( revregcheck_begin_msg );
  tmpbyte = Max.regRd( rREVISION );
  switch( tmpbyte ) {
    case( 0x01 ):  //rev.01
      Serial.print("01");
      break;  
    case( 0x12 ):  //rev.02
      Serial.print("02");
      break;
    case( 0x13 ):  //rev.03
      Serial.print("03");
      break;
    default:
      printProgStr( revregcheck_revision_invalid );
      print_hex( tmpbyte, 8 );
      printProgStr( testfailed_msg );
      return( false );
      break;
  }//switch( tmpbyte )...
    
  printProgStr( testpassed_msg );
  return( true );
}
/* SPI long test */
bool spitest()
{
  byte l = 0;
  byte k = 0;
  byte gpinpol_copy = Max.regRd( rGPINPOL );
  printProgStr( spitest_begin_msg );
  /**/
  for( byte j = 0; j < 16; j++ ) {
    for( word i = 0; i < 65535; i++ ) {
      Max.regWr( rGPINPOL, k );
      l = Max.regRd( rGPINPOL);
      if( l != k ) {
        printProgStr( spitest_fail_msg );
        print_hex( k, 8);
        printProgStr( spitest_fail_msg_2 );
        print_hex( l, 8 );
        return( false );                  //test failed
      }
      k++;
    }//for( i = 0; i < 65535; i++
    Serial.print(".");
  }//for j = 0; j < 16...
  Max.regWr( rGPINPOL, gpinpol_copy );
  printProgStr(testpassed_msg);
  return( true );
}
/* Oscillator test */
bool osctest()
{
  printProgStr( osctest_begin_msg );
  printProgStr( osctest_oscstate_msg );
  check_OSCOKIRQ();                          //print OSCOK state
  printProgStr( osctest_reset_msg );
  Max.regWr( rUSBCTL, bmCHIPRES );              //Chip reset. This stops the oscillator
  printProgStr( osctest_oscstate_msg );
  check_OSCOKIRQ();  //print OSCOK state
  printProgStr( osctest_release_msg );
  Max.regWr( rUSBCTL, 0x00 );                //Chip reset release
  for( word i = 0; i < 65535; i++) {
    if( Max.regRd( rUSBIRQ ) & bmOSCOKIRQ ) {
      printProgStr( osctest_PLLstable_msg );
      Serial.print( i, DEC );
      Serial.print(" cycles");
      printProgStr(testpassed_msg);
      return( true );
    }
  }//for i = 
  return(false);
}
/* Stop/start oscillator */
void check_OSCOKIRQ()
{
  if( Max.regRd( rUSBIRQ ) & bmOSCOKIRQ ) {  //checking oscillator state
    Serial.print("ON");
  }
  else {
    Serial.print("OFF");
  }
}
/* Test USB connectivity */
bool usbtest()
{
  byte rcode;
  byte usbstate;
    Max.powerOn();
    delay( 200 );
    printProgStr( usbtest_begin_msg );
    while( 1 ) {
      delay( 100 );
      Max.Task();
      Usb.Task();
      usbstate = Usb.getUsbTaskState();
      switch( usbstate ) {
        case( USB_ATTACHED_SUBSTATE_RESET_DEVICE ):
          printProgStr( usbtest_device_reset_msg );
          break;
        case( USB_ATTACHED_SUBSTATE_WAIT_SOF ): 
          printProgStr( usbtest_waitsof_msg );
          break;  
        case( USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE ):
          printProgStr( usbtest_enu_msg );
          break;
        case( USB_STATE_ADDRESSING ):
          printProgStr( usbtest_addr_msg );
        case( USB_STATE_CONFIGURING ):
          printProgStr( usbtest_getdevdescr_msg);
          rcode = getdevdescr( 1 );
            if( rcode ) {
              printProgStr( usbtest_err_getdevdescr_msg );
              print_hex( rcode, 8 );
            }
            else {
              Serial.print("\r\n\nAll tests passed. Press RESET to restart test"); 
              while(1);
            }
          break;
        case( USB_STATE_ERROR ):
          printProgStr( usbtest_error_state_msg ); 
        default:
          break;
    }//switch
  }//while(1)
}
/* Get device descriptor */
byte getdevdescr( byte addr )
{
  USB_DEVICE_DESCRIPTOR buf;
  byte rcode;
  rcode = Usb.getDevDescr( addr, 0, 0x12, ( char *)&buf );
  if( rcode ) {
    return( rcode );
  }
  printProgStr(Dev_Header_str);
  printProgStr(Dev_Length_str);
  print_hex( buf.bLength, 8 );
  printProgStr(Dev_Type_str);
  print_hex( buf.bDescriptorType, 8 );
  printProgStr(Dev_Version_str);
  print_hex( buf.bcdUSB, 16 );
  printProgStr(Dev_Class_str);
  print_hex( buf.bDeviceClass, 8 );
  printProgStr(Dev_Subclass_str);
  print_hex( buf.bDeviceSubClass, 8 );
  printProgStr(Dev_Protocol_str);
  print_hex( buf.bDeviceProtocol, 8 );
  printProgStr(Dev_Pktsize_str);
  print_hex( buf.bMaxPacketSize0, 8 );
  printProgStr(Dev_Vendor_str);
  print_hex( buf.idVendor, 16 );
  printProgStr(Dev_Product_str);
  print_hex( buf.idProduct, 16 );
  printProgStr(Dev_Revision_str);
  print_hex( buf.bcdDevice, 16 );
  printProgStr(Dev_Mfg_str);
  print_hex( buf.iManufacturer, 8 );
  printProgStr(Dev_Prod_str);
  print_hex( buf.iProduct, 8 );
  printProgStr(Dev_Serial_str);
  print_hex( buf.iSerialNumber, 8 );
  printProgStr(Dev_Nconf_str);
  print_hex( buf.bNumConfigurations, 8 );
  return( 0 );
}

/* GPIO lines check. A loopback adapter connecting GPIN to GPOUT is assumed */
bool gpiocheck()
{
 byte tmpbyte = 0;
  printProgStr( gpiocheck_begin_msg );
  while( Serial.available() == 0 );  //wait for input
  Serial.read();                     //empty input buffer  
    for( byte i = 0; i < 255; i++ ) {
      Max.gpioWr( i );
      tmpbyte = Max.gpioRd();
      if( tmpbyte != i ) {
        Serial.print("GPIO read/write mismatch. Write: ");
        Serial.print(i, HEX);
        Serial.print(" Read: ");
        Serial.println( tmpbyte, HEX );
        return( false );
      }//if( tmpbyte != i )
    }//for( i= 0...
    printProgStr( testpassed_msg );
    return( true );
}
/* Test halted state. Generates 0x55 to aid in SPI troubleshooting */    
void test_halted()
{
  printProgStr( test_halted_msg );
  printProgStr( reset_msg );
  while(1)  {            //System Stop. Generating pattern to keep SCLK, MISO, MOSI, SS busy
    Select_MAX3421E;
    //Max.regRd( rREVISION );
    Spi.transfer( 0x55 ); 
    Deselect_MAX3421E;
  }
}
/* given a PROGMEM string, use Serial.print() to send it out */
void printProgStr(const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    Serial.print(c,BYTE);
}
/* prints hex numbers with leading zeroes */
// copyright, Peter H Anderson, Baltimore, MD, Nov, '07
// source: http://www.phanderson.com/arduino/arduino_display.html
void print_hex(int v, int num_places)
{
  int mask=0, n, num_nibbles, digit;

  for (n=1; n<=num_places; n++)
  {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask; // truncate v to specified number of places

  num_nibbles = num_places / 4;
  if ((num_places % 4) != 0)
  {
    ++num_nibbles;
  }

  do
  {
    digit = ((v >> (num_nibbles-1) * 4)) & 0x0f;
    Serial.print(digit, HEX);
  } 
  while(--num_nibbles);
}
