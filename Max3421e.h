/* MAX3421E functions */
#ifndef _MAX3421E_H_
#define _MAX3421E_H_


#include <../Spi/Spi.h>
//#include <WProgram.h>
#include "WProgram.h"
#include "Max3421e_constants.h"

class MAX3421E : public SPI {
    // byte vbusState;
    public:
        MAX3421E( void );
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
    private:
        void init();
    friend class Max_LCD;        
};




#endif //_MAX3421E_H_
