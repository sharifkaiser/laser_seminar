//
//
// After installing bcm2835, you can build this
// with something like:
// gcc -o pwm_Kreis pwm_Kreis.c -l bcm2835 -l m				/* output in pwm_Kreis file, using library bcm2835 and m for math */
// sudo ./pwm_Kreis
// link for gcc arguments https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html#Overall-Options
// run or to see output: ./pwm_Kreis

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <bcm2835.h>
#include <unistd.h>

#define RANGE 1024		// 10bit

#define RANGE_PIC 600
#define RANGE_PIC_START 0	 // 412
#define RANGE_PIC_END   1024 // 612


// Tested:
// 2019-12-08    x=[0285 ... 1000]  y=[0250 ... 1000]
// selected:
//
// x-axis
#define RANGE_PWM0_START	400
#define RANGE_PWM0_END		900

// y-axis
#define RANGE_PWM1_START	400
#define RANGE_PWM1_END		900


/*
			 pi@raspberrypi:~ $ gpio readall
			 +-----+-----+---------+------+---+---Pi 3B+-+---+------+---------+-----+-----+
			 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
			 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
			 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
			 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5v      |     |     |
			 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
			 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 0 | IN   | TxD     | 15  | 14  |
			 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
			 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 1 | IN   | GPIO. 1 | 1   | 18  |<--- pwm0 (x-Axis)
			 |  27 |   2 | GPIO. 2 |   IN | 1 | 13 || 14 |   |      | 0v      |     |     |
			 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |<--- Laser ON/OFF
			 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
			 |  10 |  12 |    MOSI | ALT0 | 0 | 19 || 20 |   |      | 0v      |     |     |
			 |   9 |  13 |    MISO | ALT0 | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
			 |  11 |  14 |    SCLK | ALT0 | 0 | 23 || 24 | 1 | OUT  | CE0     | 10  | 8   |
			 |     |     |      0v |      |   | 25 || 26 | 1 | OUT  | CE1     | 11  | 7   |
			 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
			 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
			 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  | (<--- pwm0)
(pwm 1  -->) |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
   pwm 1  -->|  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
  (y-axis)   |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
			 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
			 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
			 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
			 +-----+-----+---------+------+---+---Pi 3B+-+---+------+---------+-----+-----+
*/

int pwm_val;


// to exit the program ctrl-c need to be pushed
// this is the ctrl-c handler to clean up and 
// close the bcm2835 library
//
void Ctrl_c_handler(int signo)
{
    //System Exit
    printf("\r\nEntering Ctrl_c_handler() ...\r\n");

	printf("1. Turn Laser OFF\n");
	bcm2835_gpio_clr(23);
	printf("2. Close  bcm2835 \n");
	bcm2835_close();
    exit(0);
}



void my_bcm2835_pwm_set_data(uint8_t channel, uint32_t data)
{
	bcm2835_pwm_set_data(channel, data);
}


int main(int argc, char** argv)
{
	// route ctrl-c handling to our 
	// Ctrl_c_handler()
	//
    signal(SIGINT, Ctrl_c_handler);

	// startup the bcm2835 library
	// check: file:///d:/daten/10_Projekte_Idee/Lasergravur/src/bcm2835-1.59/doc/html/index.html
	//
	if (!bcm2835_init()) return 1;
	//printf("bcm init= %d", bcm2835_init());


	printf("argc=%i\n",argc);
	// printf("argv=%s\n",argv[1]);	// 0th argument ./pwm_Kreis

	if(argc>1)
	{
		// the first parameter is for the start value of the
		// second PWM1 signal 0...1024
		//
		pwm_val = atoi(argv[1]);
		printf("valstr=%s\n",argv[1]);
		printf("val   =%d\n",pwm_val);
	}
	else
	{
		pwm_val=300;
	}

	// turn laser on
	//
	printf("Turn Laser ON\n");
	bcm2835_gpio_fsel(23,BCM2835_GPIO_FSEL_OUTP);	//BCM23==phy 16
	usleep(100);
	bcm2835_gpio_set(23);

	// *****************************************************************
	// PWM configuration
	// see: file:///D:/daten/10_Projekte_Idee/Lasergravur/src/bcm2835-1.59/doc/html/group__pwm.html
	// *****************************************************************
	
	// === configure PWM0 on port BCM 18 (see fig above) == phy 12
	bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_INPT);
	usleep(100);
	bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_ALT5);
	// ====================================================

	// === configure PWM1 on port BCM 19 (see fig above) == phy 35
	bcm2835_gpio_fsel(19,BCM2835_GPIO_FSEL_INPT);
	usleep(100);
	bcm2835_gpio_fsel(19,BCM2835_GPIO_FSEL_ALT5);
	// ====================================================


	// check: file:///D:/daten/10_Projekte_Idee/Lasergravur/src/bcm2835-1.59/doc/html/group__pwm.html#ga4487f4e26e57ea3697a57cf52b8de35b
	// see: bcm2835.h
	// possible value range from 1 ... 2048
	// measured:
	//  BCM2835_PWM_CLOCK_DIVIDER_2    9.375kHz  --> PWM_Clock = 18.75kHz
	//  BCM2835_PWM_CLOCK_DIVIDER_4	   4.688kHz  --> PWM_Clock = 18.75kHz
	//  BCM2835_PWM_CLOCK_DIVIDER_8	   2.344kHz  --> PWM_Clock = 18.75kHz  <== selected!
	// BCM2835_PWM_CLOCK_DIVIDER_16	   1.171kHz  --> PWM_Clock = 18.75kHz
	// BCM2835_PWM_CLOCK_DIVIDER_32      586 Hz  --> PWM_Clock = 18.75kHz
	//
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_8);


	bcm2835_pwm_set_mode(0, 1, 1);
	bcm2835_pwm_set_range(0,RANGE);
	bcm2835_pwm_set_data(0,512);

	bcm2835_pwm_set_mode(1, 1, 1);
	bcm2835_pwm_set_range(1,RANGE);
	bcm2835_pwm_set_data(1,512);
	// *****************************************************************
	// *****************************************************************


	int direction_0=1, direction_1 = 1; // 1 is increase, -1 is decrease
	// int data_0=128, data_1 = pwm_val;
	int x_range = RANGE_PWM0_END - RANGE_PWM0_START;
	int y_range = RANGE_PWM1_END - RANGE_PWM1_START;

	double counter = 0.0;
	double counter_end = 2*M_PI;
	double pwm0, pwm1;
	
	// execute 10 full sine waves
	//
	int sin_count=10;
	
	//while (sin_count>0)
	while(1)
	{
		pwm0 = (RANGE_PWM0_START + (x_range/2)) + ((x_range/2) * sin(counter));
		pwm1 = (RANGE_PWM1_START + (y_range/2)) + ((y_range/2) * cos(counter));


		//
		// PWM 0
		//
		my_bcm2835_pwm_set_data(0, pwm0);

		//
		// PWM 1
		//
		my_bcm2835_pwm_set_data(1, pwm1);


		counter += 0.00628;
		
		if(counter > counter_end)
		{
			counter=0;
			sin_count--;
		}

		// suspend for x microseconds  1 microsec = 10^-6 -> 0,000001 sec
		// 1000 -> 1ms
		usleep(1000); 
	}


	bcm2835_gpio_clr(23);
	bcm2835_close();
	return (EXIT_SUCCESS);
}
