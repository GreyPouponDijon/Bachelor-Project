#include <stdio.h>
#include <gb/gb.h>
#include <gb/drawing.h>
#include <gb/metasprites.h>
#include <string.h>

#include "Splashscreen.h"
#include "Rings.h"
#include "Amogus.h"
#include "Cockpit.h"
#include "Numbers.h"

uint8_t i;  // Used as a counter in the fade functions
uint8_t oldShiftx;  // Used to dynamically update rings. Needs to be initialized here to not constantly re-initialize in the function.
uint8_t oldShifty;
uint8_t score = 1;
uint8_t playMode;

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
move_metasprite_ex schedules the metasprite to be moved in the next vertical blank. The sprite does not move immediately.

While global variables are bad code practice, they are a lot more efficient in a GBDK-setting as they are not stored on the stack like local ones and don't have the
absolute addresses calculated by the compiler every time they are used. Instead, they are statically allocated in RAM which in most cases makes the code more efficient.
Source: https://gbdk.sourceforge.net/doc/html/c0204.html


Author: Jesper Reksten
Co-authors: Fredrik Kihl, Halvor Arnesen
Version: 1.2
Last updated: 08.05.24



*/

void ClearScreen()  // To be called to completely clear a screen. Not efficient, call only when nessecary
{
    color(WHITE, WHITE, SOLID); // Sets fore- and background color
    box(0, 0, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, M_FILL); // Fills box from 0,0 to 159,143 with selected color
    color(BLACK, WHITE, SOLID);
}

void safedelay(uint8_t numloops)    // Custom delay function using vertical blank instead of delay() to not max out cpu. "numloops" = 60 each second.
{                                   // Original author unknown, seen variations of this used in many different projects.
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

                if ((-10 < shiftx && shiftx < 10) && (-10 < shifty && shifty < 10)) // Is the ring ish centered?
                {
                    score++;
                }
                

                break;

            default:
                break;
        }

        frameCount = 0;
    }

    move_metasprite_ex(__current_metasprite, 0, 0, 0, 80+oldShiftx, 72+oldShifty);

    oldShiftx = shiftx;
    oldShifty = shifty;

    frameCount ++;  // Advance frame counter
    
}











/*
#pragma region TESTING NEW UPDATE RING

int8_t velX = 0, velY = 0;
uint8_t maxVelocity = 5; // Max velocity in either direction
int8_t friction = 1;     // Friction to slow down the rings

void updateVelocity(uint8_t butt) {
    if (butt & J_RIGHT) {
        velX = (velX + 1 < maxVelocity) ? velX + 1 : maxVelocity;
    }
    if (butt & J_LEFT) {
        velX = (velX - 1 > -maxVelocity) ? velX - 1 : -maxVelocity;
    }
    if (butt & J_UP) {
        velY = (velY - 1 > -maxVelocity) ? velY - 1 : -maxVelocity;
    }
    if (butt & J_DOWN) {
        velY = (velY + 1 < maxVelocity) ? velY + 1 : maxVelocity;
    }

    // Apply friction to naturally slow down
    if (velX > 0) velX -= friction;
    else if (velX < 0) velX += friction;

    if (velY > 0) velY -= friction;
    else if (velY < 0) velY += friction;
}

void updateRings(uint8_t shiftx, uint8_t shifty, uint8_t butt)
{
    updateVelocity(butt); // Update velocity based on button presses

    // Update positions based on velocity
    shiftx += velX;
    shifty += velY;

    if (frameCount == framesPerStep)    // If the frame is currently an animation frame
    {
        // Simplified switch statement: the same logic with position adjustment
        switch (animationIndex) {
            // Assuming each case updates the position of rings similarly
            case 0:
                move_metasprite_ex(ring8_metasprite, 0, 0, 0, 200, 200);    // Starts the ring off screen to "remove" the object from frame
                move_metasprite_ex(ring1_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 1;
                break;
            case 1:
                move_metasprite_ex(ring2_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 2;
                break;
            case 2:
                move_metasprite_ex(ring3_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 3;
                break;
            case 3:
                move_metasprite_ex(ring4_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 4;
                break;
            case 4:
                move_metasprite_ex(ring5_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 5;
                break;
            case 5:
                move_metasprite_ex(ring6_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 6;
                break;
            case 6:
                move_metasprite_ex(ring7_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 7;
                break;
            case 7:
                move_metasprite_ex(ring8_metasprite, 0, 0, 0, 80 + shiftx, 72 + shifty);
                animationIndex = 0;
                if ((-10 < shiftx && shiftx < 10) && (-10 < shifty && shifty < 10)) { // Is the ring centered?
                    score++;
                }
                break;
            default:
                break;
        }
        frameCount = 0;
    }

    move_metasprite_ex(__current_metasprite, 0, 0, 0, 80 + oldShiftx, 72 + oldShifty);

    oldShiftx = shiftx;
    oldShifty = shifty;

    frameCount++;  // Advance frame counter
}

#pragma endregion
*/











void main()
{
    set_bkg_data(0, Splashscreen_tileset_size, Splashscreen_tileset);
    set_bkg_tiles(0, 0, 20, 18, Splashscreen_tilemap);

    SHOW_BKG;
    SPRITES_8x8;
    DISPLAY_ON;

    fadei(10);
    waitpad(J_START);
    fadeo(10);

    printf("Choose game mode:\n\nSolo:.......A\n\nTwo ship:...B");
    fadei(10);
    waitpad(0x30);  // Wait for J_A or J_B to be pressed

    switch (joypad())
    {
    case J_A:
        printf("\n\nAll alone?");
        _io_out = 0xFC; // Signal for singleplayer to begin
        playMode = 1;
        break;

    case J_B:
        printf("\n\nYou have friends?");
        _io_out = 0xFE; // Signal for multiplayer to begin
        playMode = 2;
        break;

    default:
        break;
    }

    send_byte();
    while(_io_status == IO_SENDING);

    fadeo(10);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); // Clears the screen a lot simpler than the ClearScreen() function

    set_win_data(0, Cockpit_tileset_size, Cockpit_tileset);
    set_win_tiles(0, 0, 20, 18, Cockpit_tilemap);
    fadei(10);

    SHOW_WIN;
    SHOW_SPRITES;
    LCDC_REG |= 0x21;   // Enable window and give it priority over sprites

    fadei(10);

    

    set_sprite_data(0, Rings_tileset_size, Rings_tileset);
    set_sprite_data(18, Numbers_tileset_size, Numbers_tileset); // Appends more sprite data along with the rings

    
    uint8_t butt;
    uint8_t last;
    uint8_t shiftx;
    uint8_t shifty;
    uint8_t inc = 1;    // Increment in x and y direction for rings

    while (1)
    {
        receive_byte();
        
        butt = _io_in;
        //butt = joypad();

        // Left and right can't be concurrent, moves rings horizontally
        if (butt & J_RIGHT) // &'s are used instead of == as it gives us the option of checking multiple buttons at once
        {
            shiftx -= inc;
        }
        else if (butt & J_LEFT)
        {
            shiftx += inc;
        }

        // Up and down can't be concurrent, moves rings vertically
        if (butt & J_UP)
        {
            shifty -= inc;
        }
        else if (butt & J_DOWN)
        {
            shifty += inc;
        }
        
        if (joypad() == J_SELECT || score >= 0xFA)   // Send a stop bit and break out of game loop to end game
        {
            _io_out = 0xFF;
            send_byte();
            break;
        }
        

        updateRings(shiftx, shifty);
        
        if (playMode == 2)
        {
            _io_out = score;
        }
        
        send_byte();
        vsync();
    }
}

/* TO DO FOR NEXT TIME:

Make a display_score function that actually works and displays the correct score where it's supposed to be.

Get the damn velocity thing to work

*/
