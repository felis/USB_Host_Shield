/* MAX3421E USB host controller support */

#include "Max3421e.h"
// #include "Max3421e_constants.h"

static byte vbusState;

/* Functions    */

/* Constructor */
MAX3421E::MAX3421E()
{
    //Serial.begin( 9600 );
    init();
    //powerOn();
}
byte MAX3421E::getVbusState( void )
{ 
    return( vbusState );
}
/* initialization */
void MAX3421E::init()
{
    /* setup pins */
    pinMode( MAX_INT, INPUT);
    pinMode( MAX_GPX, INPUT );
    pinMode( MAX_SS, OUTPUT );
    pinMode( BPNT_0, OUTPUT );
    pinMode( BPNT_1, OUTPUT );
    digitalWrite( BPNT_0, LOW );
    digitalWrite( BPNT_1, LOW );
    Deselect_MAX3421E;              
    pinMode( MAX_RESET, OUTPUT );
    digitalWrite( MAX_RESET, HIGH );  //release MAX3421E from reset
}
//byte MAX3421E::getVbusState( void )
//{
//    return( vbusState );
//}
void MAX3421E::toggle( byte pin )
{
    digitalWrite( pin, HIGH );
    digitalWrite( pin, LOW );
}
/* Single host register write   */
void MAX3421E::regWr( byte reg, byte val)
{
      Select_MAX3421E;
      Spi.transfer( reg + 2 ); //set WR bit and send register number
      Spi.transfer( val );
      Deselect_MAX3421E;
}
/* multiple-byte write */
/* returns a pointer to a memory position after last written */
char * MAX3421E::bytesWr( byte reg, byte nbytes, char * data )
{
    Select_MAX3421E;            //assert SS
    Spi.transfer ( reg + 2 );   //set W/R bit and select register   
    while( nbytes ) {                
        Spi.transfer( *data );  // send the next data byte
        data++;                 // advance the pointer
        nbytes--;
    }
    Deselect_MAX3421E;          //deassert SS
    return( data );
}
/* Single host register read        */
byte MAX3421E::regRd( byte reg )    
{
  byte tmp;
    Select_MAX3421E;
    Spi.transfer ( reg );         //send register number
    tmp = Spi.transfer ( 0x00 );  //send empty byte, read register contents
    Deselect_MAX3421E; 
    return (tmp);
}
/* multiple-bytes register read                             */
/* returns a pointer to a memory position after last read   */
char * MAX3421E::bytesRd ( byte reg, byte nbytes, char  * data )
{
    Select_MAX3421E;    //assert SS
    Spi.transfer ( reg );     //send register number
    while( nbytes ) {
        *data = Spi.transfer ( 0x00 );    //send empty byte, read register contents
        data++;
        nbytes--;
    }
    Deselect_MAX3421E;  //deassert SS
    return( data );   
}
/* reset MAX3421E using chip reset bit. SPI configuration is not affected   */
boolean MAX3421E::reset()
{
  byte tmp = 0;
    regWr( rUSBCTL, bmCHIPRES );                        //Chip reset. This stops the oscillator
    regWr( rUSBCTL, 0x00 );                             //Remove the reset
    while(!(regRd( rUSBIRQ ) & bmOSCOKIRQ )) {          //wait until the PLL is stable
        tmp++;                                          //timeout after 256 attempts
        if( tmp == 0 ) {
            return( false );
        }
    }
    return( true );
}
/* turn USB power on/off                                                */
/* ON pin of VBUS switch (MAX4793 or similar) is connected to GPOUT7    */
/* OVERLOAD pin of Vbus switch is connected to GPIN7                    */
/* OVERLOAD state low. NO OVERLOAD or VBUS OFF state high.              */
boolean MAX3421E::vbusPwr ( boolean action )
{
  byte tmp;
    tmp = regRd( rIOPINS2 );                //copy of IOPINS2
    if( action ) {                          //turn on by setting GPOUT7
        tmp |= bmGPOUT7;
    }
    else {                                  //turn off by clearing GPOUT7
        tmp &= ~bmGPOUT7;
    }
    regWr( rIOPINS2, tmp );                 //send GPOUT7
    if( action ) {
        delay( 60 );
    }
    if (( regRd( rIOPINS2 ) & bmGPIN7 ) == 0 ) {     // check if overload is present. MAX4793 /FLAG ( pin 4 ) goes low if overload
        return( false );
    }                      
    return( true );                                             // power on/off successful                       
}
/* probe bus to determine device presense and speed */
void MAX3421E::busprobe( void )
{
 byte bus_sample;
    bus_sample = regRd( rHRSL );            //Get J,K status
    bus_sample &= ( bmJSTATUS|bmKSTATUS );      //zero the rest of the byte
    switch( bus_sample ) {                          //start full-speed or low-speed host 
        case( bmJSTATUS ):
            if(( regRd( rMODE ) & bmLOWSPEED ) == 0 ) {
                regWr( rMODE, MODE_FS_HOST );       //start full-speed host
                vbusState = FSHOST;
            }
            else {
                regWr( rMODE, MODE_LS_HOST);        //start low-speed host
                vbusState = LSHOST;
            }
            break;
        case( bmKSTATUS ):
            if(( regRd( rMODE ) & bmLOWSPEED ) == 0 ) {
                regWr( rMODE, MODE_LS_HOST );       //start low-speed host
                vbusState = LSHOST;
            }
            else {
                regWr( rMODE, MODE_FS_HOST );       //start full-speed host
                vbusState = FSHOST;
            }
            break;
        case( bmSE1 ):              //illegal state
            vbusState = SE1;
            break;
        case( bmSE0 ):              //disconnected state
            vbusState = SE0;
            break;
        }//end switch( bus_sample )
}
/* MAX3421E initialization after power-on   */
void MAX3421E::powerOn()
{
    /* Configure full-duplex SPI, interrupt pulse   */
    regWr( rPINCTL,( bmFDUPSPI + bmINTLEVEL + bmGPXB ));    //Full-duplex SPI, level interrupt, GPX
    if( reset() == false ) {                                //stop/start the oscillator
        Serial.println("Error: OSCOKIRQ failed to assert");
    }
    /* configure power switch   */
    vbusPwr( OFF );                                         //turn Vbus power off
    regWr( rGPINIEN, bmGPINIEN7 );                          //enable interrupt on GPIN7 (power switch overload flag)
    if( vbusPwr( ON  ) == false ) {
        Serial.println("Error: Vbus overload");
    }
    /* configure host operation */
    regWr( rMODE, bmDPPULLDN|bmDMPULLDN|bmHOST|bmSEPIRQ );      // set pull-downs, Host, Separate GPIN IRQ on GPX
    regWr( rHIEN, bmCONDETIE|bmFRAMEIE );                                             //connection detection
    regWr(rHCTL,bmSAMPLEBUS);                                               // update the JSTATUS and KSTATUS bits
    busprobe();                                                             //check if anything is connected
    regWr( rHIRQ, bmCONDETIRQ );                                            //clear connection detect interrupt                 
    regWr( rCPUCTL, 0x01 );                                                 //enable interrupt pin
}
/* MAX3421 state change task and interrupt handler */
byte MAX3421E::Task( void )
{
 byte rcode = 0;
 byte pinvalue;
    //Serial.print("Vbus state: ");
    //Serial.println( vbusState, HEX );
    pinvalue = digitalRead( MAX_INT );    
    if( pinvalue  == LOW ) {
        rcode = IntHandler();
    }
    pinvalue = digitalRead( MAX_GPX );
    if( pinvalue == LOW ) {
        GpxHandler();
    }
//    usbSM();                                //USB state machine                            
    return( rcode );   
}   
byte MAX3421E::IntHandler()
{
 byte HIRQ;
 byte HIRQ_sendback = 0x00;
    HIRQ = regRd( rHIRQ );                  //determine interrupt source
    if( HIRQ & bmFRAMEIRQ ) {               //->1ms SOF interrupt handler
        HIRQ_sendback |= bmFRAMEIRQ;
    }//end FRAMEIRQ handling
    if( HIRQ & bmCONDETIRQ ) {
        busprobe();
        HIRQ_sendback |= bmCONDETIRQ;
    }
    /* End HIRQ interrupts handling, clear serviced IRQs    */
    regWr( rHIRQ, HIRQ_sendback );
    return( HIRQ_sendback );
}
byte MAX3421E::GpxHandler()
{
 byte GPINIRQ = regRd( rGPINIRQ );          //read GPIN IRQ register
    if( GPINIRQ & bmGPINIRQ7 ) {            //vbus overload
        vbusPwr( OFF );                     //attempt powercycle
        delay( 1000 );
        vbusPwr( ON );
        regWr( rGPINIRQ, bmGPINIRQ7 );
    }       
    return( GPINIRQ );
}

//void MAX3421E::usbSM( void )                //USB state machine
//{
//    
//
//}