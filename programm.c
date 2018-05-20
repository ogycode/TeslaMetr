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

#define F_CPU						16000000									//������� ������� ����������� ��
#define BAUD						56000L										//������������ �������� ������ �������

#define UBRRL_VALUE					(F_CPU/(BAUD*16))-1							//�������� ��� �������� UBRR
#define ADC_VREF_TYPE				((1<<REFS1) | (1<<REFS0) | (0<<ADLAR))		//Voltage Reference: Int., cap. on AREF

#define SOURCE_VOLTAGE_CHANEL		0											//����� ���������� ������� ��
#define HALL_SENSOR_CHANEL			6											//����� ���������� ��

#define TOTAL_DELAY_MS				100											//����� ��������
#define FAST_MEASUREMENT_COUNT		800										    //���������� ��������� ��� ������������ ��������������

const char signature[] = "Verloka Vadim, 2018, https://verloka.github.io";

char lcdBuffer[16];																//����� ���
unsigned int fastMode[FAST_MEASUREMENT_COUNT];									//����� ��� ������ �������� ���������

/*
	������ ������:
	0 - ��������� ����
	1 - ��������� � ����������� (� ������� �� ����� � COM)
	2 - ��� � ����� 1 ������ ��� ����������� ����
	3 - ��������� � ����������� (�������� �������� ����� ��������� ��� ����������� �� �� � �������� � ���,
								 ����� ��������� ���������� � ��� ����� ��������� �����)
	4 - ����������� ����������
	5 - ����������� ���� ����������
*/
int mode = 1;

/*
	���������� ��� �������� �������, �����������, ������
	code - ���
	pressedKey - �������� ��������
*/
unsigned int startValue = 0;
char signValue = 0; // 1 - + 0 - - 

unsigned int Ux = 0, Bx = 0;                  //���������� ��� �������� ���������

void INIT_USART();                            //������������� ����
void INIT_ADC(void);                          //������������� ���
unsigned int READ_ADC(unsigned char);         //����� ��� � ��������� ������
void SEND_UART(char);                         //�������� ����� (�����)
char KEYPAD_GET_CODE(void);                   //����� ����������                
int GET_NUMBER_BY_CODE(char);                 //�������� ����� �� ���� ������ ���������� (-1 - *, -2 - #)
unsigned int CONVERT_BX(unsigned int);        //�������������� ���� ��������

void START_MSG();                             //��������� ���������

void Mode_Measurement(int);                   //����� � ����� �� �� (1 - �����������, 0 ���)
void Mode_Speed_Measurement(void);            //��������� ������������� ���������� ����� � ����� � ���
void Mode_Zero_Setup(void);                   //��������� ����
void Mode_Show_Voltage_Power(int);		      //���������� ���������� �������

void main(void)
{
	DDRB = 0x0F;		//���� ���������� 7654 ������� �� �����,  3210  ������� �� ������ 
	DDRA = 0;           //���� � �� �����
	DDRC = 0xFF;        //���� � �� ������ (���)
	DDRD = 0xFF;        //���� D �� ������ 

	INIT_ADC();         //������������� ���
	INIT_USART();       //������������� ����
	lcd_init(8);        //������������� ��� (8 ��������)

	START_MSG();        //��������� ���������

	startValue = READ_ADC(HALL_SENSOR_CHANEL);				//���������� �������� ���� �������� ��� ����
															//��� ����������� ���������
	while (1)
	{
		//� ����������� � ������ ������� �� ����������
		//������������� ����� ������
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

		//��������� ������������� �����
		switch (mode)
		{
		case 0: Mode_Zero_Setup(); break;                    //��������� ����
		default:
		case 1: Mode_Measurement(1); break;                  //������� �����
		case 2: Mode_Measurement(0); break;                  //������� ����� (��� �����������)
		case 3: Mode_Speed_Measurement(); break;             //����� ������������� ��������������
		case 4: Mode_Show_Voltage_Power(1); break;           //����������� ����������
		case 5: Mode_Show_Voltage_Power(0); break;           //����������� ���� ����������
		}
	}
}

void INIT_USART()
{
	UBRRL = UBRRL_VALUE;                                    //������� 8 ��� UBRRL_value
	UBRRH = UBRRL_VALUE >> 8;                                //������� 8 ��� UBRRL_value
	UCSRB |= (1 << TXEN);                                    //��� ���������� ��������
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);    //8 ��� ������
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
	delay_us(10);                            //�������� ��� ������������ ���������� ���
	ADCSRA |= (1 << ADSC);                    //������ ��������������
	while ((ADCSRA & (1 << ADIF)) == 0);    //���� ����� ��������������
	ADCSRA |= (1 << ADIF);
	return ADCW;
}
void SEND_UART(char value)
{
	while (!(UCSRA & (1 << UDRE)));        //������� ����� ��������� ����� ��������
	UDR = value;                        //�������� ������ � �����, �������� ��������
}
char KEYPAD_GET_CODE()
{
	char k, i, mask = 0x01;       // ��������� �����     


	for (i = 0; i < 3; i++)        // 3 ����� ������ �����
	{
		PORTB = (~mask) & 0xF7;     // ������ 1-�� 0 �� ������� (012)
		k = (~PINB) & 0xF0;           // ����� ����� (�������� � ������������ �� �����)
		if (k)   // ������� ���� - ������ ����
		{
			// PORTB =0x08;          // ��������� ������  (����) 
			PORTB.3 = 1;
			delay_ms(50);           // ����� ��������� �����
									// PORTC = 0;           // ���������� ������   
			PORTB.3 = 0;
			k >>= 4;            // �� ������� �������� ----> � ������� 
			return (i * 16 + k);
		}     // ����� ���� ���� ������� = ���+��������
		mask <<= 1;                 // ��������� �������  ���� � ������� ������� �� ��������
	}
	return (0);  /* ��� ������� ��������� = �����, ������� ��� */
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

	//��������������� ���� �������� � ������
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
	printf("�����: ������� �����, https://verloka.github.io\r\n\n");
	printf("������ ������:\r\n");
	printf("  0 - ��������� ����\r\n");
	printf("  1 - ��������� � ����������� �������� �� ��� � �������� � ���\r\n");
	printf("  2 - ��������� � ����������� ���� ������� �� ��� � �������� � ���\r\n");
	printf("  3 - ��������� � ������������ ��������������� ��� �������� � ��� � ����������� �� ���, ����� ��������� ");
	printf("%d", FAST_MEASUREMENT_COUNT);
	printf(" ����� ������ �������� ����� ��������� � ���\r\n\n");
	printf("  4 - ��������� � ����������� ���������� �� ��� � �������� � ���\r\n");
	printf("  5 - ��������� � ����������� ���� ���������� �� ��� � �������� � ���\r\n");
	printf("��� ��������� ������� ������ ������ ��������������� ������ �� ���������� ����������������.\r\n\n\n");

}

void Mode_Measurement(int convert)
{
	lcd_clear();

	if (convert)
	{
		Bx = CONVERT_BX(READ_ADC(HALL_SENSOR_CHANEL));         //��������� � ��������������� ���� ��������

		lcd_putsf("Mode: 1");

		if (signValue) sprintf(lcdBuffer, "B=+%d", Bx);      //����� �������� �� ��� �� ������
		else           sprintf(lcdBuffer, "B=-%d", Bx);

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);

		if (signValue) printf("+%d \n\r", Bx);               //����� � ��� �� ������ 
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
	//��������� � ��� �������� ��������
	for (mode = 0; mode < FAST_MEASUREMENT_COUNT; mode++)
		fastMode[mode] = READ_ADC(HALL_SENSOR_CHANEL);

	printf("\n##### START #####\r\n");

	//���������������� � ������� � ��� �������� �������
	for (mode = 0; mode < FAST_MEASUREMENT_COUNT; mode++)
	{
		if (signValue)  printf("+%d\r\n", CONVERT_BX(fastMode[mode]));              //����� � ��� �� ������ 
		else            printf("-%d\r\n", CONVERT_BX(fastMode[mode]));
	}


	printf("\n##### STOP #####\r\n");

	mode = 1;
}
void Mode_Zero_Setup()
{
	while (1)
	{
		startValue = READ_ADC(HALL_SENSOR_CHANEL);    //���o������ �������� ���� �������  ��� ����

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

		Ux = READ_ADC(SOURCE_VOLTAGE_CHANEL);                  //��� ���������� ������� + ���������������

		sprintf(lcdBuffer, "U=%d", Ux);                       //����� ���� ����������

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);
	}
	else
	{
		lcd_putsf("Mode: 5");

		Ux = READ_ADC(SOURCE_VOLTAGE_CHANEL);                  //��� ���������� �������

		sprintf(lcdBuffer, "Nx=%d", Ux);                       //����� ���� ����������

		lcd_gotoxy(0, 1);
		lcd_puts(lcdBuffer);
	}

	delay_ms(TOTAL_DELAY_MS);
}