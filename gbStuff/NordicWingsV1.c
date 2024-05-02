#include <stdio.h>
#include <gb/gb.h>
#include <gb/drawing.h>
#include "Splashscreen.h"
#include "Rings.h"

uint8_t i;  // Used as a counter in the fade functions

// OK BROTHER HERE IS WHAT YOU DO (note to self)
// cd Documents\VSCstuff\Projects\GB\Examples\"03. NordicWings"
// .\compile.bat

/* WELCOME TO THE NORDICWINGS BACKROOMS



Info:
LUTs with pre-calculated values are used instead of doing the calculations in the code as trigonometry is not practically possible in standard C.
They also have the added benefit of being efficient and low cost compared to continuous calculation.

The BGP_REG register is used to alter the indexes of the Gameboy color palette. In this code, it is used to fade in and out images on screen.


Author: Jesper Reksten
Co-authors: Fredrik Kihl, Halvor Arnesen
Version: 1.0
Last updated: 02.05.24



*/

void ClearScreen()  // To be called to completely clear a screen. Not efficient, call only when nessecary
{
    color(WHITE, WHITE, SOLID); // Sets fore- and background color
    box(0, 0, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, M_FILL); // Fills box from 0,0 to 159,143 with selected color
    color(BLACK, WHITE, SOLID);
}

void safedelay(uint8_t numloops)    // Custom delay function using vertical blank instead of delay() to not max out cpu. "numloops" = 60 each second
{
    uint8_t n;
    for(n = 0; n < numloops; n++)
    {
        vsync();
    }
}

void fadeo(uint8_t frames)    // Fade out current image
{
    for (i = 0; i < 4; i++)
    {
        switch (i)
        {
            case 0:
                BGP_REG = 0xE4;
                break;
            case 1:
                BGP_REG = 0x90;
                break;
            case 2:
                BGP_REG = 0x40;
                break;
            case 3:
                BGP_REG = 0x00;
                break;
        }
        safedelay(frames);  // 60 frames = 1 second
    }
}

void fadei(uint8_t frames)    // Fade in current image
{
    for (i = 0; i < 4; i++)
    {
        switch (i)
        {
            case 0:
                BGP_REG = 0x00;
                break;
            case 1:
                BGP_REG = 0x40;
                break;
            case 2:
                BGP_REG = 0x90;
                break;
            case 3:
                BGP_REG = 0xE4;
                break;
        }
        safedelay(frames);
    }
}

const uint8_t squareLUT[10][4] = {   // Values for corners of rings calculated with python
    {80, 72, 80, 72},   // {x1, y1, x2, y2} (1: top left, 2, bottom right)
    {76, 68, 84, 76},
    {72, 64, 88, 80},
    {68, 60, 92, 84},
    {60, 56, 100, 88},
    {52, 48, 108, 96},
    {40, 36, 120, 108},
    {28, 24, 132, 120},
    {16, 16, 144, 128},
    {0, 0, 159, 143}
};

uint8_t tmpLUT[10][4] = { // Only contains center values, to be changed later in code to remove rings
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72},
    {80, 72, 80, 72}
};

#pragma region Animation Stuff

uint8_t animationIndex = 0;
uint16_t frameCount = 0;    // Both used for a delay-less animation
const uint16_t framesPerStep = 20;  // Number of frames before changing frame (20 = 3 times per second)

#pragma endregion

void updateRings(uint8_t shiftx, uint8_t shifty)
{
    if (frameCount == framesPerStep)    // If the frame is currently an animation frame
    {
        if (animationIndex < 10)
        {
            color(WHITE, WHITE, SOLID);
            box(tmpLUT[animationIndex-1][0], tmpLUT[animationIndex-1][1], tmpLUT[animationIndex-1][2], tmpLUT[animationIndex-1][3], M_NOFILL);

            color(BLACK, WHITE, SOLID);
            box(squareLUT[animationIndex][0] + shiftx, squareLUT[animationIndex][1] + shifty, squareLUT[animationIndex][2] + shiftx, squareLUT[animationIndex][3] + shifty, M_NOFILL);

            // Saves last ring frame to be deleted next frame
            tmpLUT[animationIndex][0] = squareLUT[animationIndex][0] + shiftx;
            tmpLUT[animationIndex][1] = squareLUT[animationIndex][1] + shifty;
            tmpLUT[animationIndex][2] = squareLUT[animationIndex][2] + shiftx;
            tmpLUT[animationIndex][3] = squareLUT[animationIndex][3] + shifty;

            animationIndex ++;
        }
        else
        {   // If animation finished, remove rings
            animationIndex = 0; // Loop animation
            ClearScreen();
        }

        frameCount = 0; // Reset frame counter
    }

    frameCount ++;  // Advance frame counter
    
}


const uint8_t angleLUT[24][2] = {
    {140, 72},
    {136, 57},
    {132, 42},
    {121, 31},
    {110, 20},
    {95, 16}, 
    {80, 12}, 
    {65, 16}, 
    {50, 20}, 
    {35, 31}, 
    {28, 42}, 
    {24, 57}, 
    {20, 72}, 
    {24, 87}, 
    {28, 102},
    {35, 113},
    {50, 124},
    {65, 128},
    {80, 132},
    {95, 128},
    {110, 124},
    {121, 113},
    {132, 102},
    {136, 87}  
};

void Control()
{
    size_t count = sizeof(angleLUT) / sizeof(angleLUT[0]);
    size_t i = 0;           // Initialize angle index for LUT
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
        
        size_t oppositeIndex = (i + count / 2) % count; // Finds the opposite index in angleLUT to make a line through the center

        color(BLACK, WHITE, SOLID);
        int y1 = angleLUT[i][1] + yOffset;
        int y2 = angleLUT[oppositeIndex][1] + yOffset;

        // Ensure y1 and y2 remain within screen bounds
        // condition ? if_true : if_false ("simplified" if statement)
        y1 = (y1 < 0) ? 0 : (y1 > 143 ? 143 : y1);
        y2 = (y2 < 0) ? 0 : (y2 > 143 ? 143 : y2);

        line(angleLUT[i][0], y1, angleLUT[oppositeIndex][0], y2);   // Draws line between two opposing points

        while(joypad_state == joypad());    // Wait until new press is detected

        color(WHITE, WHITE, SOLID);
        line(angleLUT[i][0], y1, angleLUT[oppositeIndex][0], y2);
    }
}




void main()
{

    set_bkg_data(0, Splashscreen_tileset_size, Splashscreen_tileset);
    set_bkg_tiles(0, 0, 20, 18, Splashscreen_tilemap);

    SHOW_BKG;
    DISPLAY_ON;

    fadei(10);
    waitpad(J_START);
    fadeo(10);

    uint8_t butt = joypad();
    uint8_t last = 0;
    uint8_t shiftx = 0;
    uint8_t shifty = 0;
    uint8_t inc = 5;    // Increment in x and y direction for rings

    while (1)
    {/*
        butt = joypad();

        // Left and right can't be concurrent, moves rings horizontally
        if (butt == J_RIGHT && butt != last)
        {
            shiftx += inc;
        }
        else if (butt == J_LEFT && butt != last)
        {
            shiftx -= inc;
        }

        // Up and down can't be concurrent, moves rings vertically
        if (butt == J_UP && butt != last)
        {
            shifty += inc;
        }
        else if (butt == J_DOWN && butt != last)
        {
            shifty -= inc;
        }
        

        last = butt;    // Prevents multiple triggers per press
*/
        //Control();
        //updateRings(shiftx, shifty);
        vsync();
    }
}
