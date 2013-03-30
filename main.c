// В настройках проекта обязательно правильно укажите свою тактовую частоту
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "n3310.h"
#include "picture.h"

int main()
{

    LcdInit();

    for (;;)
    {
        LcdClear();
            LcdImage(Picture);
        LcdUpdate();


        _delay_ms(5000);


        LcdClear();
            LcdSingleBar(0, 3, 4, 5, PIXEL_ON);
            LcdSingleBar(79, 3, 4, 5, PIXEL_ON);
            LcdSingleBar(0, 47, 4, 5, PIXEL_ON);
            LcdSingleBar(79, 47, 4, 5, PIXEL_ON);

            LcdGotoXYFont(0,2);
            LcdFStr(FONT_2X,(unsigned char*)PSTR("3310LCD"));

            LcdGotoXYFont(0,3);
            LcdFStr(FONT_1X,(unsigned char*)PSTR("Hello World :)"));
           
            byte bars[5] = {1, 2, 3, 4, 5};
            LcdBars(bars, 5, 3, 2);
        LcdUpdate();


        _delay_ms(5000);


        LcdClear();
            LcdGotoXYFont(0,2);
            LcdFStr(FONT_2X,(unsigned char*)PSTR("МОГУЧИЙ"));

            LcdGotoXYFont(0,3);
            LcdFStr(FONT_1X,(unsigned char*)PSTR("язык полностьюподдерживается:)  :)  :)  :)"));
        LcdUpdate();


        _delay_ms(5000);


        LcdClear();            
            LcdRect(0,0,83,47,PIXEL_ON);
            
            LcdCircle(41,23,20,PIXEL_ON);
            LcdCircle(33,18,3,PIXEL_ON);
            LcdCircle(49,18,3,PIXEL_ON);
            LcdPixel(33,18,PIXEL_ON);
            LcdPixel(49,18,PIXEL_ON);
            
            LcdLine(35,34,46,34,PIXEL_ON);
            LcdLine(30,31,35,34,PIXEL_ON);
            LcdLine(51,31,46,34,PIXEL_ON);
        LcdUpdate();

        
        _delay_ms(5000);
    }

    return 0;
}
