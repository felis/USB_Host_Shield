/* MAX3421E functions */
#ifndef _MAX3421E_H_
#define _MAX3421E_H_


#include <Spi.h>
//#include <WProgram.h>
#include "WProgram.h"
#include "Max3421e_constants.h"

class MAX3421E : public SPI {
    // byte vbusState;
    public:
        MAX3421E( uint8_t _ss = MAX_SS, uint8_t _int = MAX_INT, uint8_t _reset = MAX_RESET );
        byte getvar( void );
        byte getVbusState( void );
        void toggle( byte pin );
        static void regWr( byte, byte );
        char * bytesWr( byte, byte, char * );
        static void gpioWr( byte );
        byte regRd( byte );
        char * bytesRd( byte, byte, char * );
        byte gpioRd( void );
        boolean reset();
        boolean vbusPwr ( boolean );
        void busprobe( void );
        void powerOn();
        byte IntHandler();
        byte GpxHandler();
        byte Task();
        //void max_select();
    private:
        void init();
        //void max_select();
    friend class Max_LCD;
    
    
};

//    int8_t _ss_pin;     //slave select
//    uint8_t _int_pin;    //interrupt
//    uint8_t _reset_pin;  //reset      

//inline void MAX3421E::max_select() { digitalWrite(_ss_pin,LOW); }

#endif //_MAX3421E_H_
