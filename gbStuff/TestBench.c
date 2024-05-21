#include <stdio.h>
#include <gb/gb.h>
#include <gb/drawing.h>

// OK BROTHER HERE IS WHAT YOU DO
// cd Documents\VSCstuff\Projects\GB\Examples\"02. TestbedRead"
// .\compile.bat

void ClearScreen()
{
    color(WHITE, WHITE, SOLID);
    box(0, 0, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, M_FILL);
    color(BLACK, WHITE, SOLID);
}

void Lines()
{
while (1)
    {
        color(BLACK, WHITE, SOLID);
        switch (joypad())
            {
            case J_A:
                line(0,0,GRAPHICS_WIDTH,GRAPHICS_HEIGHT);
                break;
            case J_B:
                box(20,20,40,40,M_NOFILL);
                break;
            case J_UP:
                circle(GRAPHICS_WIDTH/2, GRAPHICS_HEIGHT/2, 50, M_FILL);
                break;
            case J_DOWN:
                ClearScreen();
                break;

            default:
                break;
            }
    }
}

void Buttons()
{
    //Using ifs instead of switch case to allow for multiple button inputs at once
    while (1)
    {
        uint8_t mButtons = joypad();

        if (mButtons & J_A)
        {
            printf("A ");
        }
        if (mButtons & J_B)
        {
            printf("B ");
        }
        if (mButtons & J_DOWN)
        {
            printf("V ");
        }
        if (mButtons & J_LEFT)
        {
            printf("< ");
        }
        if (mButtons & J_RIGHT)
        {
            printf("> ");
        }
        if (mButtons & J_UP)
        {
            printf("^ ");
        }
        if (mButtons & J_START)
        {
            printf("+ ");
        }
        if (mButtons & J_SELECT)
        {
            printf("- ");
        }

        printf("\n");
        while(mButtons == joypad());
    }
}

const uint8_t angleLUTx[] = {
    140, // 0 deg
    136, // 15 deg - Approximation
    132, // 30 deg
    121, // 45 deg - Approximation
    110, // 60 deg
    95,  // 75 deg - Approximation
    80,  // 90 deg
    65,  // 105 deg - Approximation
    50,  // 120 deg
    35,  // 135 deg - Approximation
    28,  // 150 deg
    24,  // 165 deg - Approximation
    20,  // 180 deg
    24,  // 195 deg - Approximation
    28,  // 210 deg
    35,  // 225 deg - Approximation
    50,  // 240 deg
    65,  // 255 deg - Approximation
    80,  // 270 deg
    95,  // 285 deg - Approximation
    110, // 300 deg
    121, // 315 deg - Approximation
    132, // 330 deg
    136  // 345 deg - Approximation
};

const uint8_t angleLUTy[] = {
    72, // 0 deg
    57, // 15 deg - Approximation
    42, // 30 deg
    31, // 45 deg - Approximation
    20, // 60 deg
    16, // 75 deg - Approximation
    12, // 90 deg
    16, // 105 deg - Approximation
    20, // 120 deg
    31, // 135 deg - Approximation
    42, // 150 deg
    57, // 165 deg - Approximation
    72, // 180 deg
    87, // 195 deg - Approximation
    102, // 210 deg
    113, // 225 deg - Approximation
    124, // 240 deg
    128, // 255 deg - Approximation
    132, // 270 deg
    128, // 285 deg - Approximation
    124, // 300 deg
    113, // 315 deg - Approximation
    102, // 330 deg
    87  // 345 deg - Approximation
};

void Rotate()
{
    while (1)
    {
        size_t count = sizeof(angleLUTx) / sizeof(angleLUTx[0]);

        for(size_t i = 0; i < count; ++i) {

            size_t oppositeIndex = (i + count / 2) % count; // Get the opposite side of the index for end points of the line

            color(BLACK, WHITE, SOLID);
            line(angleLUTx[i], angleLUTy[i], angleLUTx[oppositeIndex], angleLUTy[oppositeIndex]);
            delay(100);
            color(WHITE, WHITE, SOLID);
            line(angleLUTx[i], angleLUTy[i], angleLUTx[oppositeIndex], angleLUTy[oppositeIndex]);
        }
    }
}

void Control()
{
    size_t count = sizeof(angleLUTx) / sizeof(angleLUTx[0]);
    size_t i = 0;           // Initialize angle index for LUTs
    uint8_t yOffset = 0;    // Initialize y axis offset for multi axis movement
    uint8_t yInc = 5;       // Increment of y offset

    while(1)
    {
        vsync(); // Wait for the vertical blanking interval

        uint8_t joypad_state = joypad();

        if (joypad_state & J_RIGHT)
        {
            if(i == 0) i = count - 1;
            else i--;
        }
        else if (joypad_state & J_LEFT)
        {
            i = (i + 1) % count;
        }

        // Vertical movement
        if (joypad_state & J_UP)
        {
            yOffset += yInc;
        }
        if (joypad_state & J_DOWN)
        {
            yOffset -= yInc;
        }
        

        size_t oppositeIndex = (i + count / 2) % count;

        color(BLACK, WHITE, SOLID);
        line(angleLUTx[i], angleLUTy[i] + yOffset, angleLUTx[oppositeIndex], angleLUTy[oppositeIndex] + yOffset);

        while(joypad_state == joypad());    // Wait until new press is detected

        color(WHITE, WHITE, SOLID);
        line(angleLUTx[i], angleLUTy[i] + yOffset, angleLUTx[oppositeIndex], angleLUTy[oppositeIndex] + yOffset);
    }
}

void Link() // inspirert av github.com/mrombout/gbdk_playground/blob/master/link/link.c
{
    printf("Butt stuff?\n\n");
    uint8_t frameCnt;
    uint8_t maxFrame = 2;
    
	while (1) {
        waitpadup();                        // Wait until no buttons are pressed
        waitpad(0xff);                      // Wait until any button is pressed (Bitmask 1111 1111 or all buttons)
        _io_out = joypad();                 // Button pressed is desired output
        send_byte();                        // Send the value of the pressed button
        printf("Sending %d\n", (int)_io_out);
        while(_io_status == IO_SENDING)     // Wait while it's sending
        {
            vsync();
            frameCnt++;

            if (frameCnt >= maxFrame)
            {
                frameCnt = 0;
                break;
            }
        }

        receive_byte();
        while(_io_status == IO_RECEIVING)     // Wait while it's receiving
        {
            vsync();
            frameCnt++;

            if (frameCnt >= maxFrame)
            {
                frameCnt = 0;
                break;
            }
        }
        if (_io_status == IO_IDLE)
        {
            printf("Received %d\n", (int)_io_in);
        }
        else
        {
            printf("RX error %d\n", _io_status);
            _io_status = IO_IDLE;   // Force idle
            printf("%d\n", _io_status);
        } 
	}
}



void main()
{
    uint8_t mode = 0;

    DISPLAY_ON;
    set_bkg_data(0,0,NULL);
    set_bkg_tiles(0,0,20,18,NULL);

    printf("L - Buttons\nU - Drawing\nR - Rotate\nD - Control\n+ - Link\n");

    while (mode == 0)
    {
        if (joypad() & J_LEFT) // if buttontest
        {
            mode = 1;
            _io_out = 42;
        }
        else if (joypad() & J_UP) // If drawingtest
        {
            mode = 2;
            _io_out = 69;
        }
        else if (joypad() & J_RIGHT)
        {
            mode = 3;
            _io_out = 96;
        }
        else if (joypad() & J_DOWN)
        {
            mode = 4;
            _io_out = 77;
        }
        else if (joypad() & J_START)
        {
            mode = 5;
        }
        
        
        else{}
    }

    if (mode == 1)
    {
        Buttons();
    }
    else if (mode == 2)
    {
        Lines();
    }
    else if (mode == 3)
    {
        Rotate();
    }
    else if (mode == 4)
    {
        Control();
    }
    else if (mode == 5)
    {
        Link();
    }
    
    else{}
    send_byte();
}
