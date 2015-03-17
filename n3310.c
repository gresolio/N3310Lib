/*
 * ���          :  n3310.c
 *
 * ��������     :  ��� ������� ��� ������������ LCD �� Nokia 3310, � ����� ��� ��������� ������.
 *                 ���������� �� ���� ��������� ���������� Sylvain Bissonnette � Fandi Gunawan:
 *                 http://www.microsyl.com/index.php/2010/03/24/nokia-lcd-library/
 *                 http://fandigunawan.wordpress.com/2008/06/18/lcd-nokia-3310-pcd8544-driver-in-winavravr-gcc/
 *                 �������� ������� ����� ���������� � ������ ������ ������� � ������ �� Aheir:
 *                 http://radiokot.ru/articles/29/
 *
 * �����        :  Xander Gresolio <xugres@gmail.com>
 * ���-�������� :  http://we.easyelectronics.ru/profile/XANDER/
 *
 * ��������     :  GPL v3.0
 *
 * ����������   :  WinAVR, GCC for AVR platform
 *
 * �������      :
 * ������ 1.0 (06.08.2011)
 * + ������ ������
 * + ��������� ��������� ��������� ������ LCD Nokia 3310
 * + ������ ������� ������������ � ��������� ���� ��������
 * + ������� �������� �������� ��������� ���������� (���������� Windows-1251)
 * + ��������� ������� ��������� ����������� LcdCircle
 * - ���������� ������ � �������� ������������ ��������� ��� ������ ������� ���������
 * - ���������� ������ � ������� LcdSingleBar (�������� ��������� �� y)
 */

#include <avr/io.h>
#include <string.h>
#include "n3310.h"

// ��������� ��������� ������� ��������

static void LcdSend    ( byte data, LcdCmdData cd );
static void Delay      ( void );

// ���������� ����������

// ��� � ��� 84*48 ��� ��� 504 �����
static byte  LcdCache [ LCD_CACHE_SIZE ];

// ����� �� ��������� ���� �������, � ���� �� ����� ��� ����������,
// ����� �������� ��� ������� ���� ��� ��������� ���������. �����
// ����� ���������� ��� ����� ���� ����� ��������� � ��� �������.
static int   LoWaterMark;   // ������ �������
static int   HiWaterMark;   // ������� �������

// ��������� ��� ������ � LcdCache[]
static int   LcdCacheIdx;

// ���� ��������� ����
static byte  UpdateLcd;



/*
 * ���                   :  LcdInit
 * ��������              :  ���������� ������������� ����� �� � ����������� LCD
 * ��������(�)           :  ���
 * ������������ �������� :  ���
 */
void LcdInit ( void )
{
    // Pull-up �� ����� ������������ � reset �������
    LCD_PORT |= _BV ( LCD_RST_PIN );

    // ������������� ������ ���� ����� �� �����
    LCD_DDR |= _BV( LCD_RST_PIN ) | _BV( LCD_DC_PIN ) | _BV( LCD_CE_PIN ) | _BV(LCD_DATA_PIN) | _BV(LCD_CLK_PIN);

    // ��������������� ��������
    Delay();

    // ������� reset
    LCD_PORT &= ~( _BV( LCD_RST_PIN ) );
    Delay();
    LCD_PORT |= _BV ( LCD_RST_PIN );

    // ��������� LCD ���������� - ������� ������� �� SCE
    LCD_PORT |= _BV( LCD_CE_PIN );

    // ���������� ������� �������
    LcdSend( 0x21, LCD_CMD ); // �������� ����������� ����� ������ (LCD Extended Commands)
    LcdSend( 0xC8, LCD_CMD ); // ��������� ������������� (LCD Vop)
    LcdSend( 0x06, LCD_CMD ); // ��������� �������������� ������������ (Temp coefficent)
    LcdSend( 0x13, LCD_CMD ); // ��������� ������� (LCD bias mode 1:48)
    LcdSend( 0x20, LCD_CMD ); // �������� ����������� ����� ������ � �������������� ��������� (LCD Standard Commands,Horizontal addressing mode)
    LcdSend( 0x0C, LCD_CMD ); // ���������� ����� (LCD in normal mode)

    // ��������� ������� �������
    LcdClear();
    LcdUpdate();
}



/*
 * ���                   :  LcdClear
 * ��������              :  ������� �������. ����� ���������� ��������� LcdUpdate
 * ��������(�)           :  ���
 * ������������ �������� :  ���
 */
void LcdClear ( void )
{
//    // ������� ���� �������
//    int i;
//    for ( i = 0; i < LCD_CACHE_SIZE; i++ )
//    {
//        LcdCache[i] = 0x00;
//    }

    // ����������� �� Jakub Lasinski (March 14 2009)
    memset( LcdCache, 0x00, LCD_CACHE_SIZE );
    
    // ����� ���������� ������ � ������������ ��������
    LoWaterMark = 0;
    HiWaterMark = LCD_CACHE_SIZE - 1;

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
}



/*
 * ���                   :  LcdUpdate
 * ��������              :  �������� ��� � ��� �������
 * ��������(�)           :  ���
 * ������������ �������� :  ���
 */
void LcdUpdate (void)
{
    int i;

    if ( LoWaterMark < 0 )
        LoWaterMark = 0;
    else if ( LoWaterMark >= LCD_CACHE_SIZE )
        LoWaterMark = LCD_CACHE_SIZE - 1;

    if ( HiWaterMark < 0 )
        HiWaterMark = 0;
    else if ( HiWaterMark >= LCD_CACHE_SIZE )
        HiWaterMark = LCD_CACHE_SIZE - 1;

    #ifdef CHINA_LCD  // �������� ��� ���������� �� �� ������������� ������������

        byte x,y;

        // 102 x 64 - ������ �������������� ���������� ������ ���������� ��, ��� ���
        // ������ ������ ������������ �� ������� �� ������� ����� �� 3 �������.
        // ������� ������� �������� ���� - ������� � ������ ������ y+1, � �����
        // ������� ����� (����� ���� ���� �������, �������� � ������ ������)
                
        x = LoWaterMark % LCD_X_RES;      // ������������� ��������� ����� x
        LcdSend( 0x80 | x, LCD_CMD );     // ������������ ������ ������� LoWaterMark
        
        y = LoWaterMark / LCD_X_RES + 1;  // ������������� ��������� ����� y+1
        LcdSend( 0x40 | y, LCD_CMD );     // ������������ ������ ������� LoWaterMark

        for ( i = LoWaterMark; i <= HiWaterMark; i++ )
        {
            // �������� ������ � ����� �������
            LcdSend( LcdCache[i], LCD_DATA );
            
            x++;                 // ������ ������������ ���������� x, ����� ������� ������� �� ����� ������
            if (x >= LCD_X_RES)  // ���� ����� �� ������, �� ��������� �� ��������� ������ (x=0; y++)
            {
                // ����� ������, ����� ����� ��������� ������ ����� �������������� ������,
                // �������� ���� ��������� ��������� �����, ����� ��� �������� :)
                x=0;                
                LcdSend( 0x80, LCD_CMD );
                y++;
                LcdSend( 0x40 | y, LCD_CMD );
            }
        }

        LcdSend( 0x21, LCD_CMD );    // �������� ����������� ����� ������
        LcdSend( 0x45, LCD_CMD );    // �������� �������� �� 5 �������� ����� (������������� ������� �������, �������� � ����������)
        LcdSend( 0x20, LCD_CMD );    // �������� ����������� ����� ������ � �������������� ���������

    #else  // �������� ��� ������������� �������

        // ������������� ��������� ����� � ������������ � LoWaterMark
        LcdSend( 0x80 | ( LoWaterMark % LCD_X_RES ), LCD_CMD );
        LcdSend( 0x40 | ( LoWaterMark / LCD_X_RES ), LCD_CMD );

        // ��������� ����������� ����� ������ �������
        for ( i = LoWaterMark; i <= HiWaterMark; i++ )
        {
            // ��� ������������� ������� �� ����� ������� �� ������� � ������,
            // ����� ������ ��������������� �������� ������
            LcdSend( LcdCache[i], LCD_DATA );
        }

    #endif

    // ����� ���������� ������ � �������
    LoWaterMark = LCD_CACHE_SIZE - 1;
    HiWaterMark = 0;

    // ����� ����� ��������� ����
    UpdateLcd = FALSE;
}



/*
 * ���                   :  LcdSend
 * ��������              :  ���������� ������ � ���������� �������
 * ��������(�)           :  data -> ������ ��� ��������
 *                          cd   -> ������� ��� ������ (������ enum � n3310.h)
 * ������������ �������� :  ���
 */
static void LcdSend ( byte data, LcdCmdData cd )
{
    // �������� ���������� ������� (������ ������� ��������)
    LCD_PORT &= ~( _BV( LCD_CE_PIN ) );

    byte i;

    if (cd == LCD_DATA)
        LCD_PORT |= _BV(LCD_DC_PIN);
    else
        LCD_PORT &= ~_BV(LCD_DC_PIN);

    for (i = 0; i < 8; i++) {

        if ((data >> (7 - i)) & 1) {
            LCD_PORT |= _BV(LCD_DATA_PIN);
        } else {
            LCD_PORT &= ~_BV(LCD_DATA_PIN);
        }

        LCD_PORT |= _BV(LCD_CLK_PIN);
        LCD_PORT &= ~_BV(LCD_CLK_PIN);

    }
    LCD_PORT |= _BV(LCD_DATA_PIN);
    LCD_PORT |= _BV(LCD_DC_PIN);

    // ��������� ���������� �������
    LCD_PORT |= _BV( LCD_CE_PIN );
}



/*
 * ���                   :  LcdContrast
 * ��������              :  ������������� ������������� �������
 * ��������(�)           :  �������� -> �������� �� 0x00 � 0x7F
 * ������������ �������� :  ���
 */
void LcdContrast ( byte contrast )
{
    LcdSend( 0x21, LCD_CMD );              // ����������� ����� ������
    LcdSend( 0x80 | contrast, LCD_CMD );   // ��������� ������ �������������
    LcdSend( 0x20, LCD_CMD );              // ����������� ����� ������, �������������� ���������
}



/*
 * ���                   :  Delay
 * ��������              :  ��������������� �������� ��� ��������� ������������� LCD
 * ��������(�)           :  ���
 * ������������ �������� :  ���
 */
static void Delay ( void )
{
    int i;

    for ( i = -32000; i < 32000; i++ );
}



/*
 * ���                   :  LcdGotoXYFont
 * ��������              :  ������������� ������ � ������� x,y ������������ ������������ ������� ������
 * ��������(�)           :  x,y -> ���������� ����� ������� �������. ��������: 0,0 .. 13,5
 * ������������ �������� :  ������ ������������ �������� � n3310.h
 */
byte LcdGotoXYFont ( byte x, byte y )
{
    // �������� ������
    if( x > 13 || y > 5 ) return OUT_OF_BORDER;

    //  ���������� ���������. ��������� ��� ����� � �������� 504 ����
    LcdCacheIdx = x * 6 + y * 84;
    return OK;
}



/*
 * ���                   :  LcdChr
 * ��������              :  ������� ������ � ������� ������� �������, ����� �������������� ��������� �������
 * ��������(�)           :  size -> ������ ������. ������ enum � n3310.h
 *                          ch   -> ������ ��� ������
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdChr ( LcdFontSize size, byte ch )
{
    byte i, c;
    byte b1, b2;
    int  tmpIdx;

    if ( LcdCacheIdx < LoWaterMark )
    {
        // ��������� ������ �������
        LoWaterMark = LcdCacheIdx;
    }

    if ( (ch >= 0x20) && (ch <= 0x7F) )
    {
        // �������� � ������� ��� �������� ASCII[0x20-0x7F]
        ch -= 32;
    }
    else if ( ch >= 0xC0 )
    {
        // �������� � ������� ��� �������� CP1251[0xC0-0xFF]
        ch -= 96;
    }
    else
    {
        // ��������� ���������� (�� ������ ��� � ������� ��� �������� ������)
        ch = 95;
    }

    if ( size == FONT_1X )
    {
        for ( i = 0; i < 5; i++ )
        {
            // �������� ��� ������� �� ������� � ���
            LcdCache[LcdCacheIdx++] = pgm_read_byte( &(FontLookup[ch][i]) ) << 1;
        }
    }
    else if ( size == FONT_2X )
    {
        tmpIdx = LcdCacheIdx - 84;

        if ( tmpIdx < LoWaterMark )
        {
            LoWaterMark = tmpIdx;
        }

        if ( tmpIdx < 0 ) return OUT_OF_BORDER;

        for ( i = 0; i < 5; i++ )
        {
            // �������� ��� ������� �� ������� � ��������� ����������
            c = pgm_read_byte(&(FontLookup[ch][i])) << 1;
            // ����������� ��������
            // ������ �����
            b1 =  (c & 0x01) * 3;
            b1 |= (c & 0x02) * 6;
            b1 |= (c & 0x04) * 12;
            b1 |= (c & 0x08) * 24;

            c >>= 4;
            // ������ �����
            b2 =  (c & 0x01) * 3;
            b2 |= (c & 0x02) * 6;
            b2 |= (c & 0x04) * 12;
            b2 |= (c & 0x08) * 24;

            // �������� ��� ����� � ���
            LcdCache[tmpIdx++] = b1;
            LcdCache[tmpIdx++] = b1;
            LcdCache[tmpIdx + 82] = b2;
            LcdCache[tmpIdx + 83] = b2;
        }

        // ��������� x ���������� �������
        LcdCacheIdx = (LcdCacheIdx + 11) % LCD_CACHE_SIZE;
    }

    if ( LcdCacheIdx > HiWaterMark )
    {
        // ��������� ������� �������
        HiWaterMark = LcdCacheIdx;
    }

    // �������������� ������ ����� ���������
    LcdCache[LcdCacheIdx] = 0x00;
    // ���� �������� ������� ��������� LCD_CACHE_SIZE - 1, ��������� � ������
    if(LcdCacheIdx == (LCD_CACHE_SIZE - 1) )
    {
        LcdCacheIdx = 0;
        return OK_WITH_WRAP;
    }
    // ����� ������ �������������� ���������
    LcdCacheIdx++;
    return OK;
}



/*
 * ���                   :  LcdStr
 * ��������              :  ��� ������� ������������� ��� ������ ������ ������� �������� � RAM
 * ��������(�)           :  size      -> ������ ������. ������ enum � n3310.h
 *                          dataArray -> ������ ���������� ������ ������� ����� ����������
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdStr ( LcdFontSize size, byte dataArray[] )
{
    byte tmpIdx=0;
    byte response;
    while( dataArray[ tmpIdx ] != '\0' )
    {
        // ������� ������
        response = LcdChr( size, dataArray[ tmpIdx ] );
        // �� ����� ����������� ���� ���������� OUT_OF_BORDER,
        // ������ ����� ���������� ������ �� ������ �������
        if( response == OUT_OF_BORDER)
            return OUT_OF_BORDER;
        // ����������� ���������
        tmpIdx++;
    }
    return OK;
}



/*
 * ���                   :  LcdFStr
 * ��������              :  ��� ������� ������������� ��� ������ ������ ������� �������� � Flash ROM
 * ��������(�)           :  size    -> ������ ������. ������ enum � n3310.h
 *                          dataPtr -> ��������� �� ������ ������� ����� ����������
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 * ������                :  LcdFStr(FONT_1X, PSTR("Hello World"));
 *                          LcdFStr(FONT_1X, &name_of_string_as_array);
 */
byte LcdFStr ( LcdFontSize size, const byte *dataPtr )
{
    byte c;
    byte response;
    for ( c = pgm_read_byte( dataPtr ); c; ++dataPtr, c = pgm_read_byte( dataPtr ) )
    {
        // ������� ������
        response = LcdChr( size, c );
        if(response == OUT_OF_BORDER)
            return OUT_OF_BORDER;
    }

    return OK;
}



/*
 * ���                   :  LcdPixel
 * ��������              :  ���������� ������� �� ���������� ����������� (x,y)
 * ��������(�)           :  x,y  -> ���������� ���������� �������
 *                          mode -> Off, On ��� Xor. ������ enum � n3310.h
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdPixel ( byte x, byte y, LcdPixelMode mode )
{
    int  index;
    byte  offset;
    byte  data;

    // ������ �� ������ �� �������
    if ( x >= LCD_X_RES || y >= LCD_Y_RES) return OUT_OF_BORDER;

    // �������� ������� � ��������
    index = ( ( y / 8 ) * 84 ) + x;
    offset  = y - ( ( y / 8 ) * 8 );

    data = LcdCache[ index ];

    // ��������� �����

    // ����� PIXEL_OFF
    if ( mode == PIXEL_OFF )
    {
        data &= ( ~( 0x01 << offset ) );
    }
    // ����� PIXEL_ON
    else if ( mode == PIXEL_ON )
    {
        data |= ( 0x01 << offset );
    }
    // ����� PIXEL_XOR
    else if ( mode  == PIXEL_XOR )
    {
        data ^= ( 0x01 << offset );
    }

    // ������������� ��������� �������� � ���
    LcdCache[ index ] = data;

    if ( index < LoWaterMark )
    {
        // ��������� ������ �������
        LoWaterMark = index;
    }

    if ( index > HiWaterMark )
    {
        // ��������� ������� �������
        HiWaterMark = index;
    }
    return OK;
}



/*
 * ���                   :  LcdLine
 * ��������              :  ������ ����� ����� ����� ������� �� ������� (�������� ����������)
 * ��������(�)           :  x1, y1  -> ���������� ���������� ������ �����
 *                          x2, y2  -> ���������� ���������� ����� �����
 *                          mode    -> Off, On ��� Xor. ������ enum � n3310.h
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdLine ( byte x1, byte y1, byte x2, byte y2, LcdPixelMode mode )
{
    int dx, dy, stepx, stepy, fraction;
    byte response;

    // dy   y2 - y1
    // -- = -------
    // dx   x2 - x1

    dy = y2 - y1;
    dx = x2 - x1;

    // dy �������������
    if ( dy < 0 )
    {
        dy    = -dy;
        stepy = -1;
    }
    else
    {
        stepy = 1;
    }

    // dx �������������
    if ( dx < 0 )
    {
        dx    = -dx;
        stepx = -1;
    }
    else
    {
        stepx = 1;
    }

    dx <<= 1;
    dy <<= 1;

    // ������ ��������� �����
    response = LcdPixel( x1, y1, mode );
    if(response)
        return response;

    // ������ ��������� ����� �� �����
    if ( dx > dy )
    {
        fraction = dy - ( dx >> 1);
        while ( x1 != x2 )
        {
            if ( fraction >= 0 )
            {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;

            response = LcdPixel( x1, y1, mode );
            if(response)
                return response;

        }
    }
    else
    {
        fraction = dx - ( dy >> 1);
        while ( y1 != y2 )
        {
            if ( fraction >= 0 )
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;

            response = LcdPixel( x1, y1, mode );
            if(response)
                return response;
        }
    }

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
    return OK;
}



/*
 * ���                   :  LcdCircle
 * ��������              :  ������ ���������� (�������� ����������)
 * ��������(�)           :  x, y   -> ���������� ���������� ������
 *                          radius -> ������ ����������
 *                          mode   -> Off, On ��� Xor. ������ enum � n3310.h
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdCircle(byte x, byte y, byte radius, LcdPixelMode mode)
{
    signed char xc = 0;
    signed char yc = 0;
    signed char p = 0;

    if ( x >= LCD_X_RES || y >= LCD_Y_RES) return OUT_OF_BORDER;

    yc = radius;
    p = 3 - (radius<<1);
    while (xc <= yc)  
    {
        LcdPixel(x + xc, y + yc, mode);
        LcdPixel(x + xc, y - yc, mode);
        LcdPixel(x - xc, y + yc, mode);
        LcdPixel(x - xc, y - yc, mode);
        LcdPixel(x + yc, y + xc, mode);
        LcdPixel(x + yc, y - xc, mode);
        LcdPixel(x - yc, y + xc, mode);
        LcdPixel(x - yc, y - xc, mode);
        if (p < 0) p += (xc++ << 2) + 6;
            else p += ((xc++ - yc--)<<2) + 10;
    }

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
    return OK;
}


/*
 * ���                   :  LcdSingleBar
 * ��������              :  ������ ���� ����������� �������������
 * ��������(�)           :  baseX  -> ���������� ���������� x (������ ����� ����)
 *                          baseY  -> ���������� ���������� y (������ ����� ����)
 *                          height -> ������ (� ��������)
 *                          width  -> ������ (� ��������)
 *                          mode   -> Off, On ��� Xor. ������ enum � n3310.h
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdSingleBar ( byte baseX, byte baseY, byte height, byte width, LcdPixelMode mode )
{
    byte tmpIdxX,tmpIdxY,tmp;

    byte response;

    // �������� ������
    if ( ( baseX >= LCD_X_RES) || ( baseY >= LCD_Y_RES) ) return OUT_OF_BORDER;

    if ( height > baseY )
        tmp = 0;
    else
        tmp = baseY - height + 1;

    // ��������� �����
    for ( tmpIdxY = tmp; tmpIdxY <= baseY; tmpIdxY++ )
    {
        for ( tmpIdxX = baseX; tmpIdxX < (baseX + width); tmpIdxX++ )
        {
            response = LcdPixel( tmpIdxX, tmpIdxY, mode );
            if(response)
                return response;

        }
    }

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
    return OK;
}



/*
 * ���                   :  LcdBars
 * ��������              :  ������ ������ ����������� ��������������� (� ������ PIXEL_ON)
 * ��������(�)           :  data[]     -> ������ ������� ����� ����������
 *                          numbBars   -> ���������� ���������������
 *                          width      -> ������ (� ��������)
 *                          multiplier -> ��������� ��� ������
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 * ����������            :  ���������� ��������� �������� EMPTY_SPACE_BARS, BAR_X, BAR_Y � n3310.h
 * ������                :  byte example[5] = {1, 2, 3, 4, 5};
 *                          LcdBars(example, 5, 3, 2);
 */
byte LcdBars ( byte data[], byte numbBars, byte width, byte multiplier )
{
    byte b;
    byte tmpIdx = 0;
    byte response;

    for ( b = 0;  b < numbBars ; b++ )
    {
        // ������ �� ������ �� �������
        if ( tmpIdx > LCD_X_RES - 1 ) return OUT_OF_BORDER;

        // ������ �������� x
        tmpIdx = ((width + EMPTY_SPACE_BARS) * b) + BAR_X;

        // ������ ���� �������������
        response = LcdSingleBar( tmpIdx, BAR_Y, data[b] * multiplier, width, PIXEL_ON);
        if(response == OUT_OF_BORDER)
            return response;
    }

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
    return OK;

}



/*
 * ���                   :  LcdRect
 * ��������              :  ������ ������������� �������������
 * ��������(�)           :  x1    -> ���������� ���������� x ������ �������� ����
 *                          y1    -> ���������� ���������� y ������ �������� ����
 *                          x2    -> ���������� ���������� x ������� ������� ����
 *                          y2    -> ���������� ���������� y ������� ������� ����
 *                          mode  -> Off, On ��� Xor. ������ enum � n3310.h
 * ������������ �������� :  ������ ������������ �������� � n3310lcd.h
 */
byte LcdRect ( byte x1, byte y1, byte x2, byte y2, LcdPixelMode mode )
{
    byte tmpIdx;

    // �������� ������
    if ( ( x1 >= LCD_X_RES) ||  ( x2 >= LCD_X_RES) || ( y1 >= LCD_Y_RES) || ( y2 >= LCD_Y_RES) )
        return OUT_OF_BORDER;

    if ( ( x2 > x1 ) && ( y2 > y1 ) )
    {
        // ������ �������������� �����
        for ( tmpIdx = x1; tmpIdx <= x2; tmpIdx++ )
        {
            LcdPixel( tmpIdx, y1, mode );
            LcdPixel( tmpIdx, y2, mode );
        }

        // ������ ������������ �����
        for ( tmpIdx = y1; tmpIdx <= y2; tmpIdx++ )
        {
            LcdPixel( x1, tmpIdx, mode );
            LcdPixel( x2, tmpIdx, mode );
        }

        // ��������� ����� ��������� ����
        UpdateLcd = TRUE;
    }
    return OK;
}



/*
 * ���                   :  LcdImage
 * ��������              :  ������ �������� �� ������� ������������ � Flash ROM
 * ��������(�)           :  ��������� �� ������ ��������
 * ������������ �������� :  ���
 */
void LcdImage ( const byte *imageData )
{
//    // ������������� ��������� ����
//    LcdCacheIdx = 0;
//    // � �������� ����
//    for ( LcdCacheIdx = 0; LcdCacheIdx < LCD_CACHE_SIZE; LcdCacheIdx++ )
//    {
//        // �������� ������ �� ������� � ���
//        LcdCache[LcdCacheIdx] = pgm_read_byte( imageData++ );
//    }
    
    // ����������� �� Jakub Lasinski (March 14 2009)
    memcpy_P( LcdCache, imageData, LCD_CACHE_SIZE );  // ���� ����� ��� � ����, �� �������� ������ ������ � ������� �����������
    
    // ����� ���������� ������ � ������������ ��������
    LoWaterMark = 0;
    HiWaterMark = LCD_CACHE_SIZE - 1;

    // ��������� ����� ��������� ����
    UpdateLcd = TRUE;
}
