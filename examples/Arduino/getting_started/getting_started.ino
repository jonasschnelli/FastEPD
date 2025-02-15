//
// Getting started with FastEPD
// an introduction to parallel eink displays and how to control them
// written by Larry Bank
//
#include <FastEPD.h>
#include "Roboto_Black_40.h" // a TrueType font converted to 40pt with the fontconvert tool
FASTEPD epaper; // a static instance of the FastEPD class
// This class includes all the needed functions for control of the PCB+display and
// the graphics drawing functions in two versions (1-bit and 4-bit)
// The default mode is 1-bit per pixel (pure black and white). You can dynamically
// change the mode at any time with the setMode() method.
//
// All drawing destined for the eink panel must be 'staged' in memory first. In other
// words, the drawing functions will not make any visible changes on the display until
// you explicitly tell the library to update the display. This is necessary due to the
// native of eink. Display updates are relatively slow compared to other display
// technologies such as LCDs. But, the image is retained when the power is removed.
// It would be impractical to update the display after every small change because of the
// physics of eink. The color granules must be 'pushed' through a series of passes towards
// white or black. Parallel eink can be relatively fast compared to the slower SPI type
// of displays, but it still takes considerable time and effort to change the pixels.
//
// Updates:
// The library maintains 2 full image buffers for the display - current and previous
// This allows it to know which pixels have changed so that it can do partial updates
// which means that only the changed pixels will more toward the new color. Partial
// updates are fast (typically < 200ms), but over time, will accumulate 'ghosting' due to
// imperfect pushing of the color granules. The other type of update is called a full
// update. Full in this case means that the pixels are all affected and sent to both color
// extremes to increase their mobility and ensure that they are fully seated in their
// maximum contrast positions. This type of update typically takes around 1 second for
// 1-bit more and 2 seconds for grayscale mode.
//
// Eink Power:
// There is a relatively high voltage (15-22v) needed to create a strong enough electric
// field to push the pixels to move within the transparent oil of the display. This is
// generated by a DC/DC boost circuit and is controlled seaparately from the data interface
// of the display. The default behavior for partial and full updates is to turn on the power
// (which can take up to 40ms), do the update, then turn off the power. If you're going
// to do a series of updates without pausing, then it's best to leave the eink power
// turned on until you're finished. The einkPower() method can accomplish this.
//
void setup()
{
  Serial.begin(115200);
  delay(3000); // wait for CDC-Serial to start
} /* setup() */

void loop()
{
  BBEPRECT rect;
  int i, j;
  // initialize the I/O and memory
//  epaper.initPanel(BB_PANEL_T5EPAPERS3);
//    epaper.initPanel(BB_PANEL_V7_RAW);
//    epaper.setPanelSize(1280, 720, BB_PANEL_FLAG_MIRROR_Y, 20);
//    epaper.initPanel(BB_PANEL_INKPLATE5V2);
//    epaper.initPanel(BB_PANEL_EPDIY_V7_16);
//    epaper.setPanelSize(2760, 2070, 0);
    epaper.initPanel(BB_PANEL_EPDIY_V7);
    epaper.setPanelSize(1024, 758);
    // The default drawing mode is 1-bit per pixel
    epaper.clearWhite(true); // fill the current image buffer and eink panel with white
    epaper.setFont(Roboto_Black_40);
    epaper.setTextColor(BBEP_BLACK);
    epaper.setCursor(0, 80); // TTF means Y==0-> baseline of characters, not the top
    for (i=0; i<8; i++) { // show how fast partial updates are
        epaper.getStringBox("Hello FastEPD!", &rect);
        // Tell FastEPD to only update the lines which changed (faster)
        epaper.println("Hello FastEPD!");
        epaper.partialUpdate(true, rect.y, rect.y + rect.h - 1);
    }
    delay(2000);
    // now switch to 4-bpp grayscale mode
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(15); // fill with 4-bit white (0=black, 15=white)
    // Display a grayscale spectrum
    for (i=0, j=0; i<epaper.height(); i+=epaper.height()/16, j++) {
      epaper.fillRect(0, i, epaper.width(), epaper.height()/16, j);
    }
    epaper.fullUpdate(true);
    delay(2000);
    epaper.setMode(BB_MODE_1BPP); // now switch back to 1-bpp mode and do a regional update
    epaper.fillRect(100, 100, 200, 200, BBEP_WHITE);
    epaper.drawRect(100, 100, 200, 200, BBEP_BLACK);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK);
    epaper.drawString("1-bpp", 170, 160);
    epaper.drawString("Regional Update", 110, 192);
    rect.x = rect.y = 100;
    rect.h = rect.w = 200;
    epaper.fullUpdate(true, false, &rect); // update only the rectangle
    delay(2000);
    // Now update another rectangle in grayscale mode
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillRect(600, 100, 200, 200, 0x8);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(0xe);
    epaper.drawString("4-bpp", 670, 160);
    epaper.drawString("Regional Update", 610, 192);
    rect.x = 600;
    rect.y = 100;
    rect.h = rect.w = 200;
    epaper.fullUpdate(true, false, &rect); // update only the rectangle
    while (1) {};
} /* loop() */