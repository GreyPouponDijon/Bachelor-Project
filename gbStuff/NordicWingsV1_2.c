#include <stdio.h>
#include <gb/gb.h>
#include <gb/drawing.h>
#include <gb/metasprites.h>

#include "Splashscreen.h"
#include "Rings.h"
#include "Amogus.h"
#include "Cockpit.h"

uint8_t i;  // Used as a counter in the fade functions

// OK BROTHER HERE IS WHAT YOU DO (note to self)
// cd Documents\VSCstuff\Projects\GB\Examples\"03. NordicWings"
// .\compile.bat

/* WELCOME TO THE NORDICWINGS BACKROOMS



Info:
LUTs with pre-calculated values were used instead of doing the calculations in the code as trigonometry is not practically possible in standard C.
They also had the added benefit of being efficient and low cost compared to continuous calculation.

The BGP_REG register is used to alter the indexes of the Gameboy color palette. In this code, it is used to fade in and out images on screen.

Metasprites are great for smaller sprites, but for larger as the rings, this is a silly undertaking.
The gameboy can at max render 40 individual sprites on the sprite layer at a time, so any rings larger than ring 7 would be over that amount and would not load.
Ring 8 has cut down sides to account for this and allow for a larger defined ring.


Author: Jesper Reksten
Co-authors: Fredrik Kihl, Halvor Arnesen
Version: 1.2
Last updated: 06.05.24



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





#pragma region Animation Stuff

uint8_t animationIndex = 0;
uint16_t frameCount = 0;    // Both used for a delay-less animation
const uint16_t framesPerStep = 20;  // Number of frames before changing frame (20 = 3 times per second)

#pragma endregion





#pragma region Metasprites

const metasprite_t ring1_metasprite[] = {
    {.dy=-8, .dx=-8, .dtile=1, .props = 0x80},   // This first line dictates the pivot of the rest of the sprite
    {.dy=0, .dx=8, .dtile=2, .props = 0x80},     // .dx abd .dy are relative positions to the pivot
    {.dy=8, .dx=-8, .dtile=3, .props = 0x80},    // .props = 0x80 makes the sprite be rendered below the window
    {.dy=0, .dx=8, .dtile=4, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring2_metasprite[] = {
    {.dy=-8, .dx=-8, .dtile=5, .props = 0x80},
    {.dy=0, .dx=8, .dtile=6, .props = 0x80},
    {.dy=8, .dx=-8, .dtile=7, .props = 0x80},
    {.dy=0, .dx=8, .dtile=8, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring3_metasprite[] = {
    {.dy=-8, .dx=-8, .dtile=9, .props = 0x80},
    {.dy=0, .dx=8, .dtile=10, .props = 0x80},
    {.dy=8, .dx=-8, .dtile=11, .props = 0x80},
    {.dy=0, .dx=8, .dtile=12, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring4_metasprite[] = {
    {.dy=-16, .dx=-16, .dtile=1, .props = 0x80},
    {.dy=0, .dx=8, .dtile=13, .props = 0x80},
    {.dy=0, .dx=8, .dtile=13, .props = 0x80},
    {.dy=0, .dx=8, .dtile=2, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=14, .props = 0x80},
    {.dy=0, .dx=24, .dtile=15, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=14, .props = 0x80},
    {.dy=0, .dx=24, .dtile=15, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=3, .props = 0x80},
    {.dy=0, .dx=8, .dtile=16, .props = 0x80},
    {.dy=0, .dx=8, .dtile=16, .props = 0x80},
    {.dy=0, .dx=8, .dtile=4, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring5_metasprite[] = {
    {.dy=-16, .dx=-16, .dtile=9, .props = 0x80},
    {.dy=0, .dx=8, .dtile=16, .props = 0x80},
    {.dy=0, .dx=8, .dtile=16, .props = 0x80},
    {.dy=0, .dx=8, .dtile=10, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=15, .props = 0x80},
    {.dy=0, .dx=24, .dtile=14, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=15, .props = 0x80},
    {.dy=0, .dx=24, .dtile=14, .props = 0x80},

    {.dy=8, .dx=-24, .dtile=11, .props = 0x80},
    {.dy=0, .dx=8, .dtile=13, .props = 0x80},
    {.dy=0, .dx=8, .dtile=13, .props = 0x80},
    {.dy=0, .dx=8, .dtile=12, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring6_metasprite[] = {
    {.dy = -24, .dx = -24, .dtile = 0x09, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0A, .props = 0x80},

    {.dy =   8, .dx = -40, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   40, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -40, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   40, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -40, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   40, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -40, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   40, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -40, .dtile = 0x0B, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =    0, .dx =  8, .dtile = 0x0C, .props = 0x80},
    METASPR_TERM
};
const metasprite_t ring7_metasprite[] = {
    {.dy = -32, .dx = -32, .dtile = 0x09, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x10, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0A, .props = 0x80},

    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},
    
    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},
    
    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -56, .dtile = 0x0F, .props = 0x80},
    {.dy =   0, .dx =   56, .dtile = 0x0E, .props = 0x80},

    {.dy =   8, .dx = -56, .dtile = 0x0B, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0D, .props = 0x80},
    {.dy =   0, .dx =   8, .dtile = 0x0C, .props = 0x80},

    METASPR_TERM
};
const metasprite_t ring8_metasprite[] = {
    {.dy = -56, .dx = -56, .dtile = 0x09, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x10, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x0A, .props = 0x80},

    {.dy = 8, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 24, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 8, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 24, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 8, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 24, .dx = -104, .dtile = 0x0F, .props = 0x80},
    {.dy = 0, .dx = 104, .dtile = 0x0E, .props = 0x80},

    {.dy = 8, .dx = -104, .dtile = 0x0B, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 24, .dtile = 0x0D, .props = 0x80},
    {.dy = 0, .dx = 8, .dtile = 0x0C, .props = 0x80},
    

    METASPR_TERM
};

#pragma endregion






void updateRings(uint8_t shiftx, uint8_t shifty)
{
    if (frameCount == framesPerStep)    // If the frame is currently an animation frame
    {
        switch (animationIndex)
        {
            case 0:
                move_metasprite_ex(ring8_metasprite, 0, 0, 0, 200, 200);    // Starts the ring off screen to "remove" the object from frame
                move_metasprite_ex(ring1_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 1;
                break;
            case 1:
                move_metasprite_ex(ring2_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 2;
                break;
            case 2:
                move_metasprite_ex(ring3_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 3;
                break;
            case 3:
                move_metasprite_ex(ring4_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 4;
                break;
            case 4:
                move_metasprite_ex(ring5_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 5;
                break;
            case 5:
                move_metasprite_ex(ring6_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 6;
                break;
            case 6:
                move_metasprite_ex(ring7_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 7;
                break;
            case 7:
                move_metasprite_ex(ring8_metasprite, 0, 0, 0, 80+shiftx, 72+shifty);
                animationIndex = 0;
                break;

            default:
                break;
        }

        frameCount = 0;
    }

    frameCount ++;  // Advance frame counter
    
}






void main()
{
    set_bkg_data(0, Splashscreen_tileset_size, Splashscreen_tileset);
    set_bkg_tiles(0, 0, 20, 18, Splashscreen_tilemap);

    SHOW_BKG;
    SPRITES_8x8;
    DISPLAY_ON;

    fadei(10);
    waitpad(J_START);
    _io_out = 0x45;
    send_byte();
    fadeo(10);

    set_win_data(0, Cockpit_tileset_size, Cockpit_tileset);
    set_win_tiles(0, 0, 20, 18, Cockpit_tilemap);

    SHOW_WIN;
    SHOW_SPRITES;
    LCDC_REG |= 0x21;   // Enable window and give it priority over sprites

    fadei(10);

    

    set_sprite_data(0, Rings_tileset_size, Rings_tileset);

    
    uint8_t butt;
    uint8_t last;
    uint8_t shiftx;
    uint8_t shifty;
    uint8_t inc = 1;    // Increment in x and y direction for rings

    while (1)
    {
        receive_byte();
        butt = _io_in;

        // Left and right can't be concurrent, moves rings horizontally
        if (butt == J_RIGHT && butt != last)
        {
            shiftx -= inc;
        }
        else if (butt == J_LEFT && butt != last)
        {
            shiftx += inc;
        }

        // Up and down can't be concurrent, moves rings vertically
        if (butt == J_UP && butt != last)
        {
            shifty -= inc;
        }
        else if (butt == J_DOWN && butt != last)
        {
            shifty += inc;
        }
        

        //last = butt;    // Prevents multiple triggers per press

        //Control();
        updateRings(shiftx, shifty);
        vsync();
    }
}

// WHAT TO DO NEXT:
// Passe på at ringene oppdateres selv når det ikke er animation frame så de ikke hopper sånn 100 steg hver gang
