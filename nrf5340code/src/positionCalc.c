#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <nrfx_timer.h>
#include "../include/service.h"
#include "../include/spi.h"

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/spi.h>

/*
* C file for reading and modifying the values from the BMI270
* Initiates the device, reads gyro and accelormeter, then converts it to one of 9
* D-pad direction represented by numbers between 0-10
* Code built upon https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/sensor/bmi270
*/

#define LOG_MODULE_NAME BMI270_things
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define Gravity_const 9.81f

#define Math_Pi 3.14159265358979323846f

#define COMP_FILT_ALPHA 0.05f

#define Sampletime 10.0f

#define MIN_TILT 2.0f

#define MAX_TILT 0.0f

#define MIN_ANGLE 0.2f

#define MAX_ANGLE 1.3f

float phiHat = 0.0f;
float thetaHat = 0.0f;

float tiltValue = MIN_TILT;

bool nanFlag = false;

const struct device *const dev = DEVICE_DT_GET_ONE(bosch_bmi270);
struct sensor_value acc[3], gyr[3];
struct sensor_value full_scale, sampling_freq, oversampling;

//standard arduino map
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


//convert area of sixe 4 to single uint_8t
int SearchArray(float array[]){
        uint8_t pos=0;
        for(int i = 0; i < 4; i++)
        {
                if(array[i] !=0)
                {
                        pos |= (1 << i);
                        tiltValue = map(array[i],MIN_ANGLE,MAX_ANGLE,MIN_TILT,MAX_TILT);
                }
        }
        return pos;
}

//Converts the phi and theta values calculated to an array representing the
// 4 D-pad directions on the GB
int ConvertToNESW(float phi,float theta)
{
        if (phi > MAX_ANGLE)
        {
                phi = MAX_ANGLE;
        }
        if (phi < -MAX_ANGLE)
        {
                phi = -MAX_ANGLE;
        }
        if (theta > MAX_ANGLE)
        {
                theta = MAX_ANGLE;
        }
        if (theta < -MAX_ANGLE)
        {
                theta = -MAX_ANGLE;
        }

        float N;
        float E;
        float S;
        float W;

        float array[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        if (phi > MIN_ANGLE && fabs(theta) < MIN_ANGLE)
        {
                N = fabs(phi);
                S = 0.0f;
                E = 0.0f;
                W = 0.0f;
        }
        else if(phi < -MIN_ANGLE && fabs(theta) < MIN_ANGLE)
        {
                N=0.0f;
                S = fabs(phi);
                E = 0.0f;
                W = 0.0f;
        }
        else if (theta > MIN_ANGLE && fabs(phi) < MIN_ANGLE)
        {
                N = 0.0f;
                S = 0.0f;
                E = fabs(theta);
                W = 0.0f;
        }
        else if (theta < -MIN_ANGLE && fabs(phi) < MIN_ANGLE)
        {
                N = 0.0f;
                S = 0.0f;
                E = 0.0f;
                W = fabs(theta);
        }


        else if (theta < -MIN_ANGLE && phi < -MIN_ANGLE)
        {
                N = 0.0f;
                S = fabs(phi);
                E = 0.0f;
                W = fabs(theta);
        }
        else if (theta < -MIN_ANGLE && phi > MIN_ANGLE)
        {
                N = fabs(phi);
                S = 0.0f;
                E = 0.0f;
                W = fabs(theta);
        }
        else if (theta > MIN_ANGLE && phi < -MIN_ANGLE)
        {
                N = 0.0f;
                S = fabs(phi);
                E = fabs(theta);
                W = 0.0f;
        }
        else if (theta > MIN_ANGLE && phi > MIN_ANGLE)
        {
                N = fabs(phi);
                S = 0.0f;
                E = fabs(theta);
                W = 0.0f;
        }
        else
        {
                N = 0.0f;
                S = 0.0f;
                E = 0.0f;
                W = 0.0f;
        }

        array[0] = W;
        array[1] = E;
        array[2] = S;
        array[3] = N; 

        return SearchArray(array);
}

//Initiates the BMI270
int SetSensorParam(void)
{
	if (!device_is_ready(dev)) {
		LOG_INF("Device %s is not ready\n", dev->name);
		return 0;
	}

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);


	/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);
}

/7Reads data fro the BMI270 and converts using a complementary filter
int ObtainPosition(void)
{
        sensor_sample_fetch(dev);

        sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, acc);
        sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyr);

        //conversion formula from sensor.h definition
        float ax = (acc[0].val1+acc[0].val2*1e-6);
        float ay = (acc[1].val1+acc[1].val2*1e-6);
        float az = (acc[2].val1+acc[2].val2*1e-6);

        //to ensure no Nan's being produced by atanf and asinf
        if(ax <= -9.80)
        {
                ax = -9.80;
        }
        if(ax >= 9.80)
        {
                ax = 9.80;
        }

        if(az == 0)
        {
                az = 0.01;
        }
        //Estimation of the angle using the accelerometer values
        //Theoretical proof and code from https://www.youtube.com/watch?v=BUW2OdAtzBw

        float phiHat_acc_g = atanf(ay/az);
        float thetaHat_acc_g = asinf(ax/Gravity_const);

	 //to ensure no Nan's being produced by tanf
        if(thetaHat_acc_g >= Math_Pi/2)
        {
                thetaHat_acc_g = Math_Pi/2-0.007f;
        }

        if(thetaHat_acc_g <= -Math_Pi/2)
        {
                thetaHat_acc_g = -Math_Pi/2+0.007f;
        }

        //Gyroscope values
        float gx = (gyr[0].val1+gyr[0].val2*1e-6);
        float gy = (gyr[1].val1+gyr[1].val2*1e-6);
        float gz = (gyr[2].val1+gyr[2].val2*1e-6);


        float phiDot_rps = gx + tanf(thetaHat_acc_g) * (sinf(phiHat_acc_g) * gy + cosf(phiHat_acc_g) * gz);
        float thetaDot_rps = cosf(phiHat_acc_g) * gy - sinf(phiHat_acc_g) * gz;

        //Complementary filter
        phiHat = COMP_FILT_ALPHA * phiHat_acc_g + (1.0f - COMP_FILT_ALPHA) * (phiHat + (Sampletime / 1000.0f) * phiDot_rps);
        thetaHat = COMP_FILT_ALPHA * thetaHat_acc_g + (1.0f - COMP_FILT_ALPHA) * (thetaHat + (Sampletime/1000.0f) * thetaDot_rps);

        if (isnan(phiHat) || isnan(thetaHat))
        {
                nanFlag = true;
        }
        return ConvertToNESW(phiHat,thetaHat);
                           
}
