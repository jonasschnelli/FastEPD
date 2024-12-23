#include <Wire.h>
#include "bb_epdiy.h"
#include "arduino_io.inl"
#include "bb_epdiy.inl"
#include "bb_ep_gfx.inl"

#if !(defined(CONFIG_ESP32_SPIRAM_SUPPORT) || defined(CONFIG_ESP32S3_SPIRAM_SUPPORT))
#error "Please enable PSRAM support"
#endif
#pragma GCC optimize("O2")
// Display how much time each operation takes on the serial monitor
#define SHOW_TIME

int BBEPDIY::setRotation(int iAngle)
{
    iAngle %= 360;
    if (iAngle % 90 != 0) return BBEP_ERROR_BAD_PARAMETER;
    _state.rotation = iAngle;
    // set the correct fast pixel function
    switch (iAngle) {
        case 0:
            _state.width = _state.native_width;
            _state.height = _state.native_height;
            if (_state.mode == BB_MODE_1BPP) {
                _state.pfnSetPixelFast = bbepSetPixelFast2Clr;
            } else {
                _state.pfnSetPixelFast = bbepSetPixelFast16Clr;
            }
            break;
        case 90:
            _state.width = _state.native_height;
            _state.height = _state.native_width;
            if (_state.mode == BB_MODE_1BPP) {
                _state.pfnSetPixelFast = bbepSetPixelFast2Clr_90;
            } else {
                _state.pfnSetPixelFast = bbepSetPixelFast16Clr_90;
            }
            break;
        case 180:
            _state.width = _state.native_width;
            _state.height = _state.native_height;
            if (_state.mode == BB_MODE_1BPP) {
                _state.pfnSetPixelFast = bbepSetPixelFast2Clr_180;
            } else {
                _state.pfnSetPixelFast = bbepSetPixelFast16Clr_180;
            }
            break;
        case 270:
            _state.width = _state.native_height;
            _state.height = _state.native_width;
            if (_state.mode == BB_MODE_1BPP) {
                _state.pfnSetPixelFast = bbepSetPixelFast2Clr_270;
            } else {
                _state.pfnSetPixelFast = bbepSetPixelFast16Clr_270;
            }
            break;
    }
    return BBEP_SUCCESS;
}
void BBEPDIY::rowControl(int iMode)
{
    bbepRowControl(&_state, iMode);
}
void BBEPDIY::writeRow(uint8_t *pData, int iLen)
{
    bbepWriteRow(&_state, pData, iLen);
}
void BBEPDIY::drawPixel(int16_t x, int16_t y, uint8_t color)
{
    (*_state.pfnSetPixel)(&_state, x, y, color);
}
void BBEPDIY::drawPixelFast(int16_t x, int16_t y, uint8_t color)
{
    (*_state.pfnSetPixelFast)(&_state, x, y, color);
}

void BBEPDIY::drawRoundRect(int x, int y, int w, int h,
                   int r, uint8_t color)
{
    bbepRoundRect(&_state, x, y, w, h, r, color, 0);
}
void BBEPDIY::fillRoundRect(int x, int y, int w, int h,
                   int r, uint8_t color)
{
    bbepRoundRect(&_state, x, y, w, h, r, color, 1);
}

void BBEPDIY::fillRect(int x, int y, int w, int h,
                   uint8_t color)
{
    bbepRectangle(&_state, x, y, x+w-1, y+h-1, color, 1);
}

void BBEPDIY::drawLine(int x1, int y1, int x2, int y2, int iColor)
{
    bbepDrawLine(&_state, x1, y1, x2, y2, iColor);
} /* drawLine() */ 

int BBEPDIY::setMode(int iMode)
{
    int iPitch;

    if (iMode != BB_MODE_1BPP && iMode != BB_MODE_4BPP) return BBEP_ERROR_BAD_PARAMETER;
    if (iMode == BB_MODE_1BPP) {
        _state.pfnSetPixel = bbepSetPixel2Clr;
        _state.pfnSetPixelFast = bbepSetPixelFast2Clr;
    } else {
        _state.pfnSetPixel = bbepSetPixel16Clr;
        _state.pfnSetPixelFast = bbepSetPixelFast16Clr;
    }
    _state.mode = iMode;
    return BBEP_SUCCESS;
} /* setMode() */

void BBEPDIY::setFont(int iFont)
{
    _state.iFont = iFont;
    _state.pFont = NULL;
} /* setFont() */

void BBEPDIY::setFont(const void *pFont)
{
    _state.iFont = -1;
    _state.pFont = (void *)pFont;
} /* setFont() */

void BBEPDIY::setTextColor(int iFG, int iBG)
{
    _state.iFG = iFG;
    _state.iBG = (iBG == -1) ? iFG : iBG;
} /* setTextColor() */

void BBEPDIY::drawString(const char *pText, int x, int y)
{
    if (_state.pFont) {
        bbepWriteStringCustom(&_state, (BB_FONT *)_state.pFont, x, y, (char *)pText, _state.iFG);
    } else if (_state.iFont >= FONT_6x8 && _state.iFont < FONT_COUNT) {
        bbepWriteString(&_state, x, y, (char *)pText, _state.iFont, _state.iFG);
    }
} /* drawString() */

size_t BBEPDIY::write(uint8_t c) {
char szTemp[2]; // used to draw 1 character at a time to the C methods
int w=8, h=8;

  szTemp[0] = c; szTemp[1] = 0;
   if (_state.pFont == NULL) { // use built-in fonts
      if (_state.iFont == FONT_8x8 || _state.iFont == FONT_6x8) {
        h = 8;
        w = (_state.iFont == FONT_8x8) ? 8 : 6;
      } else if (_state.iFont == FONT_12x16 || _state.iFont == FONT_16x16) {
        h = 16;
        w = (_state.iFont == FONT_12x16) ? 12:16;
      }

    if (c == '\n') {              // Newline?
      _state.iCursorX = 0;          // Reset x to zero,
      _state.iCursorY += h; // advance y one line
    } else if (c != '\r') {       // Ignore carriage returns
        if (_state.wrap && ((_state.iCursorX + w) > _state.width)) { // Off right?
            _state.iCursorX = 0;               // Reset x to zero,
            _state.iCursorY += h; // advance y one line
        }
        bbepWriteString(&_state, -1, -1, szTemp, _state.iFont, _state.iFG);
    }
  } else { // Custom font
      BB_FONT *pBBF = (BB_FONT *)_state.pFont;
    if (c == '\n') {
      _state.iCursorX = 0;
      _state.iCursorY += pBBF->height;
    } else if (c != '\r') {
      if (c >= pBBF->first && c <= pBBF->last) {
          BB_GLYPH *pBBG = &pBBF->glyphs[c - pBBF->first];
        w = pBBG->width;
        h = pBBG->height;
        if (w > 0 && h > 0) { // Is there an associated bitmap?
          w += pBBG->xOffset;
          if (_state.wrap && (_state.iCursorX + w) > _state.width) {
            _state.iCursorX = 0;
            _state.iCursorY += h;
          }
          bbepWriteStringCustom(&_state, (BB_FONT *)_state.pFont, -1, -1, szTemp, _state.iFG);
        }
      }
    }
  }
  return 1;
} /* write() */

int BBEPDIY::initCustomPanel(BBPANELDEF *pPanel)
{
    _state.iPanelType = BB_PANEL_CUSTOM;
    memcpy(&_state.panelDef, pPanel, sizeof(BBPANELDEF));
    return bbepIOInit(&_state);
} /* setPanelType() */

int BBEPDIY::setPanelSize(int width, int height) {
    _state.width = _state.native_width = width;
    _state.height = _state.native_height = height;
    _state.pCurrent = (uint8_t *)ps_malloc(_state.width * _state.height / 2); // current pixels
    _state.pPrevious = (uint8_t *)ps_malloc(_state.width * _state.height / 2); // comparison with previous buffer
    _state.pTemp = (uint8_t *)ps_malloc(_state.width * _state.height / 4); // LUT data
    return BBEP_SUCCESS;
} /* setPanelSize() */

int BBEPDIY::initPanel(int iPanel)
{
    int rc;
    if (iPanel > 0 && iPanel < BB_PANEL_COUNT) {
        _state.iPanelType = iPanel;
        _state.width = panelDefs[iPanel].width;
        _state.height = panelDefs[iPanel].height;
        memcpy(&_state.panelDef, &panelDefs[iPanel], sizeof(BBPANELDEF));
        rc = bbepIOInit(&_state);
        _state.mode = BB_MODE_1BPP; // start in 1-bit mode
        if (rc == BBEP_SUCCESS) {
            // allocate memory for the buffers
            if (_state.width) { // if size is defined
                _state.pCurrent = (uint8_t *)ps_malloc(_state.width * _state.height / 8); // current pixels
                _state.pPrevious = (uint8_t *)ps_malloc(_state.width * _state.height / 8); // comparison with previous buffer
                _state.pTemp = (uint8_t *)ps_malloc(_state.width * _state.height / 4); // LUT data
            }
        }
        GLUT = (uint32_t *)malloc(256 * 9 * sizeof(uint32_t));
        GLUT2 = (uint32_t *)malloc(256 * 9 * sizeof(uint32_t));
        // Prepare grayscale lookup tables
        for (int j = 0; j < 9; j++) {
            for (int i = 0; i < 256; i++) {
                GLUT[j * 256 + i] = (waveform3Bit[i & 0x07][j] << 2) | (waveform3Bit[(i >> 4) & 0x07][j]);
                GLUT2[j * 256 + i] = ((waveform3Bit[i & 0x07][j] << 2) | (waveform3Bit[(i >> 4) & 0x07][j])) << 4;
            }
        }
        _state.pfnSetPixel = bbepSetPixel2Clr;
        _state.pfnSetPixelFast = bbepSetPixelFast2Clr;
        return rc;
    }
    return BBEP_ERROR_BAD_PARAMETER;
} /* initIO() */

#define TPS_REG_ENABLE 0x01
#define TPS_REG_PG 0x0F
int BBEPDIY::einkPower(int bOn)
{
#ifdef SHOW_TIME
    long l = millis();
#endif
    if (bOn == _state.pwr_on) return BBEP_SUCCESS;
    if (bOn) {
        if (_state.panelDef.flags & BB_PANEL_FLAG_TPS65185) {
            uint8_t ucTemp[4];
            uint8_t u8Value = 0; // I/O bits for the PCA9535
          //  u8Value |= 4; // STV on DEBUG - not sure why it's not used
            u8Value |= 1; // OE on
            u8Value |= 0x20; // WAKEUP on
            bbepPCA9535Write(1, u8Value);
            u8Value |= 8; // PWRUP on
            bbepPCA9535Write(1, u8Value);
            u8Value |= 0x10; // VCOM CTRL on
            bbepPCA9535Write(1, u8Value);
            delay(10); // allow time to power up
            while (!(bbepPCA9535Read(1) & 0x40 /*CFG_PIN_PWRGOOD*/)) { }
            ucTemp[0] = TPS_REG_ENABLE;
            ucTemp[1] = 0x3f; // enable output
            bbepI2CWrite(0x68, ucTemp, 2);
            // set VCOM to 1.6v (1600)
            ucTemp[0] = 3; // vcom voltage register 3+4 = L + H
            ucTemp[1] = (uint8_t)(160);
            ucTemp[2] = (uint8_t)(160 >> 8);
            bbepI2CWrite(0x68, ucTemp, 3);

            int iTimeout = 0;
            u8Value = 0;
            while (iTimeout < 400 && ((u8Value & 0xfa) != 0xfa)) {
                bbepI2CReadRegister(0x68, TPS_REG_PG, &u8Value, 1); // read power good
                iTimeout++;
                delay(5);
            }
            if (iTimeout >= 400) {
                Serial.println("The power_good signal never arrived!");
                return BBEP_IO_ERROR;
            }
        } else { // single MOSFET
            gpio_set_level((gpio_num_t)(_state.panelDef.ioOE & 0xff), 1);
            delayMicroseconds(100);
            gpio_set_level((gpio_num_t)(_state.panelDef.ioPWR & 0xff), 1);
            delayMicroseconds(100);
            gpio_set_level((gpio_num_t)(_state.panelDef.ioSPV & 0xff), 1);
            gpio_set_level((gpio_num_t)(_state.panelDef.ioSPH & 0xff), 1);
        }
        _state.pwr_on = 1;
    } else { // turn off the power
        if (_state.panelDef.flags & BB_PANEL_FLAG_TPS65185) {
            bbepPCA9535Write(1, 0x20); // only leave WAKEUP on
            delay(10);
            bbepPCA9535Write(1, 0); // now turn everything off
        } else { // single MOSFET
            gpio_set_level((gpio_num_t)(_state.panelDef.ioPWR & 0xff), 0);
            delayMicroseconds(10);
            gpio_set_level((gpio_num_t)(_state.panelDef.ioOE & 0xff), 0);
            delayMicroseconds(100);
            gpio_set_level((gpio_num_t)(_state.panelDef.ioSPV & 0xff), 0);
        }
        _state.pwr_on = 0;
    }
#ifdef SHOW_TIME
    l = millis() - l;
    Serial.printf("eink power %s time = %d ms\n", (bOn) ? "on": "off", (int)l);
#endif
    return BBEP_SUCCESS;
} /* einkPower() */

//
// Clear the display with the given code for the given number of repetitions
// val: 0 = lighten, 1 = darken, 2 = discharge, 3 = skip
//
void BBEPDIY::clear(uint8_t val, uint8_t count)
{
    if (val == 0) val = 0xaa;
    else if (val == 1) val = 0x55;
    else if (val == 2) val = 0x00;
    else val = 0xff;
    memset(_state.dma_buf, val, _state.width / 4);

    for (int k = 0; k < count; ++k)
    {
        bbepRowControl(&_state, ROW_START);
        for (int i = 0; i < _state.height; ++i)
        {
            // Send the data using I2S DMA driver.
            bbepWriteRow(&_state, _state.dma_buf, _state.width / 4);
            delayMicroseconds(15);
            bbepRowControl(&_state, ROW_STEP);
        }
        delayMicroseconds(230);
    }
} /* clear() */

void BBEPDIY::fillScreen(uint8_t u8Color)
{
    int iPitch;
    if (_state.mode == BB_MODE_1BPP) {
        if (u8Color == BBEP_WHITE) u8Color = 0xff;
        iPitch = (_state.width + 7) / 8;
    } else {
        iPitch = (_state.width + 1) / 2;
        u8Color |= (u8Color << 4);
    }
    memset(_state.pCurrent, u8Color, iPitch * _state.height);
} /* fillScreen() */

int BBEPDIY::fullUpdate(bool bFast, bool bKeepOn)
{
    int passes;
#ifdef SHOW_TIME
    long l = millis();
#endif
    if (einkPower(1) != BBEP_SUCCESS) return BBEP_IO_ERROR;
// Fast mode ~= 600ms, normal mode ~=1000ms
    passes = (bFast) ? 5:11;
    if (!bFast) { // skip initial black phase for fast mode
        clear(0, 1);
        clear(1, passes);
        clear(2, 1);
    }
    clear(0, passes);
    clear(2, 1);
    clear(1, passes);
    clear(2, 1);
    clear(0, passes);

    if (_state.mode == BB_MODE_1BPP) {
        // Set the color in multiple passes starting from white
        // First create the 2-bit codes per pixel for the black pixels
        uint8_t *s, *d;
        for (int i = 0; i < _state.native_height; i++) {
            s = &_state.pCurrent[i * (_state.native_width/8)];
            d = &_state.pTemp[i * (_state.native_width/4)];
            memcpy(&_state.pPrevious[i * (_state.native_width/8)], s, _state.native_width / 8); // previous = current
            for (int n = 0; n < (_state.native_width / 4); n += 4) {
                uint8_t dram1 = *s++;
                uint8_t dram2 = *s++;
                *(uint16_t *)&d[n+2] = LUTB_16[dram2];
                *(uint16_t *)&d[n] = LUTB_16[dram1];
            }
        }
        // Write 5 passes of the black data to the whole display
        for (int k = 0; k < 5; ++k) {
            rowControl(ROW_START);
            for (int i = 0; i < _state.native_height; ++i) {
                s = &_state.pTemp[i * (_state.native_width / 4)];
                // Send the data for the row
                memcpy(_state.dma_buf, s, _state.native_width/ 4);
                writeRow(_state.dma_buf, (_state.native_width / 4));
                delayMicroseconds(15);
                rowControl(ROW_STEP);
            }
            delayMicroseconds(230);
        }

        for (int k = 0; k < 1; ++k) {
            uint8_t *s, *d;
            rowControl(ROW_START);
            for (int i = 0; i < _state.native_height; i++) {
                s = &_state.pCurrent[(_state.native_width/8) * i];
                d = &_state.pTemp[i * (_state.native_width/4)];
                for (int n = 0; n < (_state.native_width / 4); n += 4) {
                    uint8_t dram1 = *s++;
                    uint8_t dram2 = *s++;
                    *(uint16_t *)&d[n+2] = LUT2_16[dram2];
                    *(uint16_t *)&d[n] = LUT2_16[dram1];
                }
                // Send the data for the row
                memcpy(_state.dma_buf, d, _state.native_width/ 4);
                writeRow(d, (_state.native_width / 4));
                delayMicroseconds(15);
                rowControl(ROW_STEP);
            }
            delayMicroseconds(230);
        }

        for (int k = 0; k < 1; ++k) {
            rowControl(ROW_START);
            memset((void *)_state.dma_buf, 0, _state.native_width /4); // send 0's
            for (int i = 0; i < _state.native_height; i++) {
                // Send the data for the row
                writeRow(_state.dma_buf, (_state.native_width / 4));
                delayMicroseconds(15);
                rowControl(ROW_STEP);
            }
            delayMicroseconds(230);
        }
    } else { // must be 4BPP mode
        for (int k = 0; k < 9; ++k) { // 9 phases to make 9 gray levels
            uint8_t *s, *d = _state.dma_buf;
            rowControl(ROW_START);
            for (int i = 0; i < _state.native_height; i++) {
                s = &_state.pCurrent[i *(_state.native_width / 2)];
                for (int j = 0; j < (_state.native_width / 4); j += 4) {
                    d[j + 0] = (GLUT2[k * 256 + *s++] | GLUT[k * 256 + *s++]);
                    d[j + 1] = (GLUT2[k * 256 + *s++] | GLUT[k * 256 + *s++]);
                    d[j + 2] = (GLUT2[k * 256 + *s++] | GLUT[k * 256 + *s++]);
                    d[j + 3] = (GLUT2[k * 256 + *s++] | GLUT[k * 256 + *s++]);
                } // for j
                writeRow(_state.dma_buf, (_state.native_width / 4));
                delayMicroseconds(15);
                rowControl(ROW_STEP);
            } // for i
            delayMicroseconds(230);
        } // for k
    } // 4bpp

        // Set the drivers inside epaper panel into dischare state.
        clear(3, 1);
        if (!bKeepOn) einkPower(0);
    
#ifdef SHOW_TIME
    l = millis() - l;
    Serial.printf("fullUpdate time: %dms\n", (int)l);
#endif
    return BBEP_SUCCESS;
} /* fullUpdate() */

int BBEPDIY::partialUpdate(bool bKeepOn, int iStartLine, int iEndLine)
{
    long l = millis();
    if (einkPower(1) != BBEP_SUCCESS) return BBEP_IO_ERROR;
    if (iStartLine < 0) iStartLine = 0;
    if (iEndLine >= _state.native_height) iEndLine = _state.native_height-1;
    if (iEndLine < iStartLine) return BBEP_ERROR_BAD_PARAMETER;

    uint32_t _send;
    uint8_t whiteMask, *pCur, *pPrev, *d, data = 0;
    uint8_t diffw, diffb, cur, prev;
    
    for (int i = iStartLine; i <= iEndLine; ++i) {
        d = &_state.pTemp[i * (_state.native_width/4)]; // LUT temp storage
        pCur = &_state.pCurrent[i * (_state.native_width / 8)];
        pPrev = &_state.pPrevious[i * (_state.native_width / 8)];
        for (int j = 0; j < _state.native_width / 16; ++j)
        {
            cur = *pCur++; prev = *pPrev++;
            diffw = prev & ~cur;
            diffb = ~prev & cur;
            *(uint16_t *)&d[0] = LUTW_16[diffw] & LUTB_16[diffb];

            cur = *pCur++; prev = *pPrev++;
            diffw = prev & ~cur;
            diffb = ~prev & cur;
            *(uint16_t *)&d[2] = LUTW_16[diffw] & LUTB_16[diffb];
            d += 4;
        }
    }
    for (int k = 0; k < 4; ++k) { // each pass is about 32ms
        uint8_t *dp = _state.pTemp;
        int iSkipped = 0;
        rowControl(ROW_START);
        for (int i = 0; i < _state.native_height; ++i) {
            if (i >= iStartLine && i <= iEndLine) {
                memcpy((void *)_state.dma_buf, dp, _state.native_width/4);
                // Send the data using I2S DMA driver.
                writeRow(_state.dma_buf, (_state.native_width / 4));
                delayMicroseconds(10);
                iSkipped = 0;
            } else {
                if (iSkipped >= 2) {
                    gpio_set_level((gpio_num_t)_state.panelDef.ioCKV, 1); // CKV_SET;
                    delayMicroseconds(35);
                } else {
                    // write 2 floating rows
                    if (iSkipped == 0) { // clean
                       memset((void *)_state.dma_buf, 0xff, _state.native_width/4);
                    }
                    writeRow(_state.dma_buf, (_state.native_width / 4));
                    delayMicroseconds(10);
                }
                iSkipped++;
            }
            rowControl(ROW_STEP);
            dp += (_state.native_width / 4);
        }
        //delayMicroseconds(230);
    }

    if (bKeepOn) {
        clear(2, 1);
    } else {
        einkPower(0);
    }
    int offset = iStartLine * (_state.native_width/8);
    memcpy(&_state.pPrevious[offset], &_state.pCurrent[offset], (_state.native_width/8) * (iEndLine - iStartLine+1));

    l = millis() - l;
    Serial.printf("partial update time: %dms\n", (int)l);
    return BBEP_SUCCESS;
} /* partialUpdate() */