/*
	Author:		Verloka Vadim
	Date:		20.04.2018
	Version:	18.05

	Chip type:					ATmega32
	AVR Core Clock frequency:	16 MHz
*/

#include <io.h>
#include <delay.h>
#include <alcd.h>
#include <stdio.h>

#define F_CPU						16000000									//Рабочая частота контроллера Гц
#define BAUD						56000L										//Максимальная Скорость обмена данными

#define UBRRL_VALUE					(F_CPU/(BAUD*16))-1							//Значение для регистра UBRR
#define ADC_VREF_TYPE				((1<<REFS1) | (1<<REFS0) | (0<<ADLAR))		//Voltage Reference: Int., cap. on AREF

#define SOURCE_VOLTAGE_CHANEL		0											//Канал напряжения питания ДХ
#define HALL_SENSOR_CHANEL			6											//Канал напряжения ДХ

#define TOTAL_DELAY_MS				100											//Общая задержка
#define FAST_MEASUREMENT_COUNT		800										    //Количество измерений при максимальном быстродействии

const char signature[] = "Verloka Vadim, 2018, https://verloka.github.io";

char lcdBuffer[16];																//Буфер ЖКИ
unsigned int fastMode[FAST_MEASUREMENT_COUNT];									//Буфер для режима быстрого измерения

/*
	Режимы работы:
	0 - Установка нуля
	1 - Измерение и отображение (с выводом на экран и COM)
	2 - Как и режим 1 только без конвертации кода
	3 - Измерение и отображение (Измеряет указаное число измерений без отображения на ЖК и отправки в СОМ,
								 после измерения отправляет в СОМ масив измеряных точек)
	4 - Отображение напряжения
	5 - Отображение кода напряжения
*/
int mode = 1;

/*
	Переменные для хранения нажатых, пльзоватлем, кнопок
	code - код
	pressedKey - числовое значение
*/
unsigned int startValue = 0;
char signValue = 0; // 1 - + 0 - - 

unsigned int Ux = 0, Bx = 0;                  //Переменные для хранения измерений

void INIT_USART();                            //Инициализация УАПП
void INIT_ADC(void);                          //Инициализация АЦП
unsigned int READ_ADC(unsigned char);         //Опрос АЦП с указанием канала
void SEND_UART(char);                         //Передача байта (текст)
char KEYPAD_GET_CODE(void);                   //Опрос клавиатуры                
int GET_NUMBER_BY_CODE(char);                 //Получить цифру по коду опроса клавиатуры (-1 - *, -2 - #)
unsigned int CONVERT_BX(unsigned int);        //Преобразование кода индукции

void START_MSG();                             //Стартовое сообщение

void Mode_Measurement(int);                   //Опрос и вывод на ЖК (1 - конвертация, 0 без)
void Mode_Speed_Measurement(void);            //Измерение максимального количества точек и вывод в СОМ
void Mode_Zero_Setup(void);                   //Установка нуля
void Mode_Show_Voltage_Power(int);		      //Показывать напряжение питания

void main(void)
{
	DDRB = 0x0F;		//Порт клавиатуры 7654 разряды на прием,  3210  разряды на выдачу 
	DDRA = 0;           //Порт А на прием
	DDRC = 0xFF;        //Порт С на выдачу (ЖКИ)
	DDRD = 0xFF;        //Порт D на выдачу 

	INIT_ADC();         //Инициализация АЦП
	INIT_USART();       //Инициализация УАПП
	lcd_init(8);        //Инициализация ЖКИ (8 символов)

	START_MSG();        //Стартовое сообщение

	startValue = READ_ADC(HALL_SENSOR_CHANEL);				//Запоминаем значение кода индукции при нуле
															//для дальнейшего обнуления
	while (1)
	{
		//В соответсвии с нажато кнопкой на клавиатуре
		//устанавливаем режим работы
		switch (GET_NUMBER_BY_CODE(KEYPAD_GET_CODE()))
		{
		case  1: mode = 1; break;
		case  2: mode = 2; break;
		case  3: mode = 3; break;
		case  4: mode = 4; break;
		case  5: mode = 5; break;
		case  0: mode = 0; break;
		default: mode = 1; break;
		}

		//Запускаем установленный режим
		switch (mode)
		{
		case 0: Mode_Zero_Setup(); break;                    //Установка нуля
		default:
		case 1: Mode_Measurement(1); break;                  //Обычный режим
		case 2: Mode_Measurement(0); break;                  //Обычный режим (без конвертации)
		case 3: Mode_Speed_Measurement(); break;             //Режим максимального быстродействия
		case 4: Mode_Show_Voltage_Power(1); break;           //Отображение напряжения
		case 5: Mode_Show_Voltage_Power(0); break;           //Отображение кода напряжения
		}
	}
}

void INIT_USART()
{
	UBRRL = UBRRL_VALUE;                                    //Младшие 8 бит UBRRL_value
	UBRRH = UBRRL_VALUE >> 8;                                //Старшие 8 бит UBRRL_value
	UCSRB |= (1 << TXEN);                                    //Бит разрешения передачи
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);    //8 бит данных
}
void INIT_ADC()
{
	ADMUX = ADC_VREF_TYPE;
	ADCSRA = (1 << ADEN) | (0 << ADSC) | (0 << ADATE) | (0 << ADIF) | (0 << ADIE) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	SFIOR = (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
}
unsigned int READ_ADC(unsigned char chanel)
{
	ADMUX = chanel | ADC_VREF_TYPE;
	delay_us(10);                            //Задержка для стабилизации напряжения АЦП
	ADCSRA |= (1 << ADSC);                    //Запуск преобразования
	while ((ADCSRA & (1 << ADIF)) == 0);    //Ждем конца преобразования
	ADCSRA |= (1 << ADIF);
	return ADCW;
}
void SEND_UART(char value)
{
	while (!(UCSRA & (1 << UDRE)));        //Ожидаем когда очистится буфер передачи
	UDR = value;                        //Помещаем данные в буфер, начинаем передачу
}
char KEYPAD_GET_CODE()
{
	char k, i, mask = 0x01;       // начальная маска     


	for (i = 0; i < 3; i++)        // 3 цикла опроса линий
	{
		PORTB = (~mask) & 0xF7;     // выдача 1-го 0 на столбец (012)
		k = (~PINB) & 0xF0;           // опрос линий (инверсия и маскирование не линий)
		if (k)   // нажатие было - данные есть
		{
			// PORTB =0x08;          // включение бузера  (звук) 
			PORTB.3 = 1;
			delay_ms(50);           // пауза послушать гудок
									// PORTC = 0;           // выключение бузера   
			PORTB.3 = 0;
			k >>= 4;            // из старших разрядов ----> в младшие 
			return (i * 16 + k);
		}     // выход если было нажатие = код+смещение
		mask <<= 1;                 // следующий столбец  если в текущем нажатие не замечено
	}
	return (0);  /* все столбцы проверили = выход, нажатия нет */
}
int GET_NUMBER_BY_CODE(char code)
{
	switch (code)
	{
	case 0x11: return 1; case 0x21: return 2;
	case 0x01: return 3; case 0x12: return 4;
	case 0x22: return 5; case 0x02: return 6;
	case 0x14: return 7; case 0x24: return 8;
	case 0x04: return 9; case 0x28: return 0;
	case 0x18: return -1; case 0x08: return -2;
	default: return -3;
	}
}
unsigned int CONVERT_BX(unsigned int code)
{
	if (startValue == code) return 0;

	//Конвертирование кода индукции в Гауссы
	if (code > startValue)
	{
		code = ((code - startValue) * 100) / 88;
		signValue = 1;
	}
	else
	{
		code = ((startValue - code) * 100) / 89;
		signValue = 0;
	}

	return code;
}

void START_MSG()
{
	lcd_putsf("Tesla - ");
	lcd_gotoxy(0, 1);
	lcd_putsf("   metr ");
	delay_ms(TOTAL_DELAY_MS * 30);
	lcd_clear();

	lcd_putsf("Author: ");
	lcd_gotoxy(0, 1);
	lcd_putsf("Verloka ");
	delay_ms(TOTAL_DELAY_MS * 30);
	lcd_clear();

	printf("Tesla-metr\r\n");
	printf("Автор: Верлока Вадим, https://verloka.github.io\r\n\n");
	printf("Режимы работы:\r\n");
	printf("  0 - Установка нуля\r\n");
	printf("  1 - Измерение и отображение индукции на ЖКИ и отправка в СОМ\r\n");
	printf("  2 - Измерение и отображение кода индукци на ЖКИ и отправка в СОМ\r\n");
	printf("  3 - Измерение с максимальным быстродействием без отправки в СОМ и отображение на ЖКИ, после измерения ");
	printf("%d", FAST_MEASUREMENT_COUNT);
	printf(" точек массив значений будет отправлен в СОМ\r\n\n");
	printf("  4 - Измерение и отображение напряжения на ЖКИ и отправка в СОМ\r\n");
	printf("  5 - Измерение и отображение кода напряжения на ЖКИ и отправка в СОМ\r\n");
	printf("Для установки нужного режима нажать соответствующую кнопку на клавиатуре микроконтроллера.\r\n\n\n");

}

void Mode_Measurement(int convert)
{
	lcd_clear();

	if (convert)
	{
		Bx = CONVERT_BX(READ_ADC(HALL_SENSOR_CHANEL));         //Получение и конвертирование кода индукции

		lcd_putsf("Mode: 1");

		if (signValue) sprintf(lcdBuffer, "B=+%d", Bx);      //Вывод индукции на жки со знаком
		else           sprintf(lcdBuffer, "B=-%d", Bx);

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);

		if (signValue) printf("+%d \n\r", Bx);               //Вывод в СОМ со знаком 
		else           printf("-%d \n\r", Bx);
	}
	else
	{
		lcd_putsf("Mode: 2");

		Bx = READ_ADC(HALL_SENSOR_CHANEL);

		sprintf(lcdBuffer, "Nx=%d", Bx);

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);

		printf("%d \n\r", Bx);
	}

	delay_ms(TOTAL_DELAY_MS);
}
void Mode_Speed_Measurement()
{
	//Считываем с АЦП значение индукции
	for (mode = 0; mode < FAST_MEASUREMENT_COUNT; mode++)
		fastMode[mode] = READ_ADC(HALL_SENSOR_CHANEL);

	printf("\n##### START #####\r\n");

	//Переопразовываем и выводим в СОМ значение индукци
	for (mode = 0; mode < FAST_MEASUREMENT_COUNT; mode++)
	{
		if (signValue)  printf("+%d\r\n", CONVERT_BX(fastMode[mode]));              //Вывод в СОМ со знаком 
		else            printf("-%d\r\n", CONVERT_BX(fastMode[mode]));
	}


	printf("\n##### STOP #####\r\n");

	mode = 1;
}
void Mode_Zero_Setup()
{
	while (1)
	{
		startValue = READ_ADC(HALL_SENSOR_CHANEL);    //Запoминаем значение кода индукци  при нуле

		lcd_clear();
		lcd_puts("Zero - ");
		lcd_gotoxy(0, 1);
		lcd_puts("setup");

		delay_ms(TOTAL_DELAY_MS);

		if (KEYPAD_GET_CODE() == 0x28)
		{
			lcd_clear();
			lcd_puts("Zero end");
			lcd_gotoxy(0, 1);
			lcd_puts("setup");

			mode = 1;

			delay_ms(TOTAL_DELAY_MS);
			return;
		}
	}
}
void Mode_Show_Voltage_Power(int convert)
{
	lcd_clear();

	if (convert)
	{
		lcd_putsf("Mode: 4");

		Ux = READ_ADC(SOURCE_VOLTAGE_CHANEL);                  //Код напряжения питания + КОНВЕРТИРОВАНИЕ

		sprintf(lcdBuffer, "U=%d", Ux);                       //Вывод кода напряжения

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);
	}
	else
	{
		lcd_putsf("Mode: 5");

		Ux = READ_ADC(SOURCE_VOLTAGE_CHANEL);                  //Код напряжения питания

		sprintf(lcdBuffer, "Nx=%d", Ux);                       //Вывод кода напряжения

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);
	}

	delay_ms(TOTAL_DELAY_MS);
}