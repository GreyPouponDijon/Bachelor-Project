#include <zephyr/kernel.h>


#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

#define DIR 0x014
#define OUTSET volatile 0x008
#define OUTCLR 0x00C
#define IN 0x010
#define LED1 28


int setup(void)
{
        // set GPIO28-31 (LED1-4) to output
        // set GPIO23,24,08,09 to input
}


int bluetooth_pair(void)
{
        //pair with specific bluetooth device (dependent on MAC-address, device name)
}



//ved knappetrykk
int send_package(void)
{
        //upon button press read ports and construct package(s) ? one package for each button? identifier + high/low
        // send package?
}

int recieve_package(void)
{
        //read the correct bits/packages return one byte
}

int main(void)
{
        setup();
        while(CHECK_BIT(IN, 8)){
                blink_led();
        }

        
}
