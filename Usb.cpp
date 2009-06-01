/* USB functions */

#include "Usb.h"

static byte usb_error = 0;
static byte usb_task_state;
DEV_RECORD devtable[ USB_NUMDEVICES + 1 ];
EP_RECORD dev0ep;         //Endpoint data structure used during enumeration for uninitialized device


/* constructor */

USB::USB () {
    usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;  //set up state machine
    init(); 
}
void USB::init()
{
  byte i;
    for( i = 0; i < ( USB_NUMDEVICES + 1 ); i++ ) {
        devtable[ i ].epinfo = NULL;       //clear device table
        devtable[ i ].devclass = 0;
    }
    devtable[ 0 ].epinfo = &dev0ep; //set single ep for uninitialized device  
    dev0ep.MaxPktSize = 8;          //minimum possible                        	
    dev0ep.sndToggle = bmSNDTOG0;   //set DATA0/1 toggles to 0
    dev0ep.rcvToggle = bmRCVTOG0;
}
byte USB::getUsbTaskState( void )
{
    return( usb_task_state );
}
EP_RECORD* USB::getDevTableEntry( byte addr, byte ep )
{
  EP_RECORD* ptr;
    ptr = devtable[ addr ].epinfo;
    ptr += ep;
    return( ptr );
}
/* Control transfer. Sets address, endpoint, fills control packet with necessary data, dispatches control packet, and initiates bulk IN transfer,   */
/* depending on request. Actual requests are defined as macros                                                                                      */
/* return codes:                */
/* 00       =   success         */
/* 01-0f    =   non-zero HRSLT  */
byte USB::ctrlReq( byte addr, byte ep, byte bmReqType, byte bRequest, byte wValLo, byte wValHi, unsigned int wInd, unsigned int nbytes, char* dataptr )
{
 boolean direction = false;     //request direction, IN or OUT
 // byte datastage = 1;            //request data stage present or absent
 byte rcode;   
 SETUP_PKT setup_pkt;
 
    // debug
//    Serial.print("addr: ");
//    Serial.println( addr, HEX );
//    Serial.print("Endpoint: ");
//    Serial.println(ep, HEX );
//    Serial.print("ReqType: ");
//    Serial.println(bmReqType, HEX );
//    Serial.print("Request: ");
//    Serial.println(bRequest, HEX );
//    Serial.print("ValLo: ");
//    Serial.println(wValLo, HEX );
//    Serial.print("ValHi: ");
//    Serial.println(wValHi, HEX );
//    Serial.print("Ind: ");
//    Serial.println(wInd, HEX );
//    Serial.print("nbytes: :");
//    Serial.println(nbytes, HEX );
    //debug
 
//    if( dataptr == NULL ) {
//        datastage = 0;
//    }
    regWr( rPERADDR, addr );                    //set peripheral address
    /* fill in setup packet */
    if( bmReqType & 0x80 ) {
        direction = true;                       //determine request direction
    }
    /* fill in setup packet */
    setup_pkt.ReqType_u.bmRequestType = bmReqType;
    setup_pkt.bRequest = bRequest;
    setup_pkt.wVal_u.wValueLo = wValLo;
    setup_pkt.wVal_u.wValueHi = wValHi;
    setup_pkt.wIndex = wInd;
    setup_pkt.wLength = nbytes;
    bytesWr( rSUDFIFO, 8, ( char *)&setup_pkt );    //transfer to setup packet FIFO
    rcode = dispatchPkt( tokSETUP, ep );            //dispatch packet
    if( rcode ) {                                   //return HRSLT if not zero
        return( rcode );
    }
    //Serial.println( direction, HEX ); 
    if( dataptr != NULL ) {                         //data stage, if present
        rcode = ctrlData( addr, ep, nbytes, dataptr, direction );
    }
    if( rcode ) {   //return error
        return( rcode );
    }
    rcode = ctrlStatus( ep, direction );                //status stage
    return( rcode );
}
/* Control transfer with status stage and no data stage */
/* Assumed peripheral address is already set */
byte USB::ctrlStatus( byte ep, boolean direction )
{
  byte rcode;
    if( direction ) { //GET
        rcode = dispatchPkt( tokOUTHS, ep );
    }
    else {
        rcode = dispatchPkt( tokINHS, ep );
    }
    return( rcode );
}
/* Control transfer with data stage. Stages 2 and 3 of control transfer. Assumes preipheral address is set and setup packet has been sent */
byte USB::ctrlData( byte addr, byte ep, unsigned int nbytes, char* dataptr, boolean direction )
{
  byte rcode;
    
    if( direction ) {                      //IN transfer
        devtable[ addr ].epinfo[ ep ].rcvToggle = bmRCVTOG1;
        //Serial.print("CtrlData toggle check: ");
        //Serial.println( dev0ep.rcvToggle, HEX );
        rcode = inTransfer( addr, ep, nbytes, dataptr, devtable[ addr ].epinfo[ ep ].MaxPktSize );
        //Serial.print("CtrlData Check:" );
        //Serial.println( devtable[ addr ].epinfo[ ep ].MaxPktSize, HEX );
        return( rcode );
    }
    else {              //OUT not implemented
        return( 0xff );
    }    
}
/* IN transfer to arbitrary endpoint. Assumes PERADDR is set. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Keep sending INs and writes data to memory area pointed by 'data'                                                           */
/* rcode 0 if no errors. rcode 01-0f is relayed from prvXferDispatchPkt(). Rcode f0 means RCVDAVIRQ error,
            fe USB xfer timeout */
byte USB::inTransfer( byte addr, byte ep, unsigned int nbytes, char* data, byte maxpktsize )
{
 byte rcode;
 byte pktsize;
 unsigned int xfrlen = 0;
    //toggle( BPNT_0 );
    regWr( rHCTL, devtable[ addr ].epinfo[ ep ].rcvToggle );    //set toggle value
    while( 1 ) { // use a 'return' to exit this loop
        rcode = dispatchPkt( tokIN, ep );           //IN packet to EP-'endpoint'. Function takes care of NAKS.
        if( rcode ) {
            return( rcode );                            //should be 0, indicating ACK. Else return error code.
        }
        /* check for RCVDAVIRQ and generate error if not present */ 
        /* the only case when absense of RCVDAVIRQ makes sense is when toggle error occured. Need to add handling for that */
        if(( regRd( rHIRQ ) & bmRCVDAVIRQ ) == 0 ) {
            return ( 0xf0 );                            //receive error
        }
        pktsize = regRd( rRCVBC );                      //number of received bytes
        data = bytesRd( rRCVFIFO, pktsize, data );
        regWr( rHIRQ, bmRCVDAVIRQ );                    // Clear the IRQ & free the buffer
        xfrlen += pktsize;                              // add this packet's byte count to total transfer length
        /* The transfer is complete under two conditions:           */
        /* 1. The device sent a short packet (L.T. maxPacketSize)   */
        /* 2. 'nbytes' have been transferred.                       */
        if (( pktsize < maxpktsize ) || (xfrlen >= nbytes )) {      // have we transferred 'nbytes' bytes?
            if( regRd( rHRSL ) & bmRCVTOGRD ) {                     //save toggle value
                devtable[ addr ].epinfo[ ep ].rcvToggle = bmRCVTOG1;
            }
            else {
                devtable[ addr ].epinfo[ ep ].rcvToggle = bmRCVTOG0;
            }
            return( 0 );
        }
  }//while( 1 )
}
/* dispatch usb packet. Assumes peripheral address is set and relevant buffer is loaded/empty       */
/* If NAK, tries to re-send up to USB_NAK_LIMIT times                                               */
/* If bus timeout, re-sends up to USB_RETRY_LIMIT times                                             */
/* return codes 0x00-0x0f are HRSLT( 0x00 being success ), 0xff means timeout                       */
byte USB::dispatchPkt( byte token, byte ep )
{
  unsigned long timeout = millis() + USB_XFER_TIMEOUT;;
  byte tmpdata;   
  byte rcode;
  byte nak_count = 0;
  char retry_count = 0;

    while( 1 ) {
        regWr( rHXFR, ( token|ep ));            //launch the transfer
        rcode = 0xff;   
        while( millis() < timeout ) {           //wait for transfer completion
            tmpdata = regRd( rHIRQ );
            if( tmpdata & bmHXFRDNIRQ ) {
                regWr( rHIRQ, bmHXFRDNIRQ );    //clear the interrupt
                rcode = 0x00;
                break;
            }
        }//while ( millis() < timeout
        if( rcode != 0x00 ) {               //exit if timeout
            return( rcode );
        }
        rcode = ( regRd( rHRSL ) & 0x0f );
        if( rcode == hrNAK ) {
            nak_count++;
            if( nak_count == USB_NAK_LIMIT ) {
                break;
            }
            else {
                continue;
            }
        }
        if( rcode == hrTIMEOUT ) {
            retry_count++;
            if( retry_count == USB_RETRY_LIMIT ) {
                break;
            }
            else {
                continue;
            }
        }
        else break;
    }//while( 1 )
    return( rcode );
}

void USB::Task( void )      //USB state machine
{
  byte i;   
  byte rcode;
  static byte tmpaddr; 
  byte tmpdata;
  static unsigned long delay = 0;
  USB_DEVICE_DESCRIPTOR buf;
    tmpdata = getVbusState();
    /* modify USB task state if Vbus changed */

    switch( tmpdata ) {
        case SE1:   //illegal state
            usb_task_state = USB_DETACHED_SUBSTATE_ILLEGAL;
            break;
        case SE0:   //disconnected
            if(( usb_task_state & USB_STATE_MASK ) != USB_STATE_DETACHED ) {
                usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
            }
            break;
        case FSHOST:    //attached
        case LSHOST:
            if(( usb_task_state & USB_STATE_MASK ) == USB_STATE_DETACHED ) {
                delay = millis() + USB_SETTLE_DELAY;
                usb_task_state = USB_ATTACHED_SUBSTATE_SETTLE;
            }
            break;
        }// switch( tmpdata
    //Serial.print("USB task state: ");
    //Serial.println( usb_task_state, HEX );
    switch( usb_task_state ) {
        case USB_DETACHED_SUBSTATE_INITIALIZE:
            init();
            usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
            break;
        case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:     //just sit here
            break;
        case USB_DETACHED_SUBSTATE_ILLEGAL:             //just sit here
            break;
        case USB_ATTACHED_SUBSTATE_SETTLE:              //setlle time for just attached device                  
            if( delay < millis() ) {
                usb_task_state = USB_ATTACHED_SUBSTATE_RESET_DEVICE;
            }
            break;
        case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
            // regWr( rHIRQ, bmBUSEVENTIRQ );              //clear bus event IRQ
            regWr( rHCTL, bmBUSRST );                   //issue bus reset
            usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE;
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
            if(( regRd( rHCTL ) & bmBUSRST ) == 0 ) {
                tmpdata = regRd( rMODE ) | bmSOFKAENAB;                 //start SOF generation
                regWr( rMODE, tmpdata );
                usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_SOF;
            }
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_SOF:
            if( regRd( rHIRQ ) & bmFRAMEIRQ ) {                         //when first SOF received we can continue
                usb_task_state = USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE;
            }
            break;
        case USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE:
            // toggle( BPNT_0 );
            rcode = getDevDescr( 0, 0, 8, ( char* )&buf );
            if( rcode == 0 ) {
                devtable[ 0 ].epinfo->MaxPktSize = buf.bMaxPacketSize0;
                usb_task_state = USB_STATE_ADDRESSING;
            }
            else {
                usb_error = USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE;
                usb_task_state = USB_STATE_ERROR;
            }
            break;
        case USB_STATE_ADDRESSING:
            for( i = 1; i < USB_NUMDEVICES; i++ ) {
                if( devtable[ i ].epinfo == NULL ) {
                    devtable[ i ].epinfo = devtable[ 0 ].epinfo;        //set correct MaxPktSize
                    rcode = setAddr( 0, 0, i );
                    if( rcode == 0 ) {
                        tmpaddr = i;
                        usb_task_state = USB_STATE_CONFIGURING;
                    }
                    else {
                        usb_error = USB_STATE_ADDRESSING;          //set address error
                        usb_task_state = USB_STATE_ERROR;
                    }
                    break;  //break if address assigned or error occured during address assignment attempt                      
                }
            }//for( i = 1; i < USB_NUMDEVICES; i++
            if( usb_task_state == USB_STATE_ADDRESSING ) {     //no vacant place in devtable
                usb_error = 0xfe;
                usb_task_state = USB_STATE_ERROR;
            }
            break;
        case USB_STATE_CONFIGURING:
            break;
        case USB_STATE_RUNNING:
            break;
        case USB_STATE_ERROR:
            break;
    }// switch( usb_task_state
}    
  