#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdint.h>
#include "proximity_sensor.hpp"
#include "limit_switch.hpp"
#include "motor_driver.hpp"
#include "encoder.hpp"
#include "PID.hpp"
#include "solenoid_valve.hpp"

HardwareSerial SerialCom(PA10, PA9);

MotorDriver mob_motor1(PA6, PA7);  // Left Pin, Right Pin
MotorDriver mob_motor2(PB0, PB1);  // Left Pin, Right Pin
MotorDriver brush_motor(PA2, PA3); // Left Pin, Right Pin

LimitSwitch limitSwitch1(PB10);
LimitSwitch limitSwitch2(PB11);
ProximitySensor proximitySensor1(PB4);
ProximitySensor proximitySensor2(PB5);

Encoder encoder1(PB6, NULL, 2, TIM3); // A 2 Hz RPMS calculation interval produced the best results.
Encoder encoder2(PB7, NULL, 2, TIM4); // A 2 Hz RPMS calculation interval produced the best results.

SolenoidValve valve1(PB3);
SolenoidValve valve2(PA15);

float mob1_speed = 0.0f;
float mob2_speed = 0.0f;
float brush_speed = 0.0f;

float delta_time = 100e-3;

PID_Controller pid_mob1(2.0f, 0.1f, 0.1f, 0.0f, 1.0f);
PID_Controller pid_mob2(2.0f, 0.1f, 0.1f, 0.0f, 1.0f);

uint32_t time_stamp = 0;

#define DISTANCE_THRESHOLD 1947.6 // mm
#define WIDTH_THRESHOLD (w / 2)

bool bidirectional_mode = true;
bool direction_movement = true; // forward = true, backward = false

void setup()
{
    SerialCom.begin(9600);
    brush_motor.set_direction(Forward); // Backward == Right
    mob_motor1.set_direction(Forward);  // Forward == Left
    mob_motor2.set_direction(Backward); // Backward == Right

    time_stamp = millis();
}

void loop()
{
    uint32_t cuurent_time = millis();

    // CHECKING PROXIMITY

    if (proximitySensor1.sense == LOW)
    {
        SerialCom.println("Warning: Proximity sensor 1 is not working. System remains stopped") return;
    }

    if (proximitySensor2.sense == LOW)
    {
        SerialCom.println("Warning: Proximity sensor 2 is not working. System remains stopped") return;
    }

    // STARTING SYSTEM'S MOVEMENT

    if ((current_time - time_stamp) > 50)
    {
        mob1_speed = pid_mob1.calculate(0.14, encoder1.velocity, delta_time);
        mob2_speed = pid_mob2.calculate(0.14, encoder2.velocity, delta_time);

        mob_motor1.set_speed(mob1_speed);
        mob_motor2.set_speed(mob2_speed);
        encoder1.reset();
        encoder2.reset();

        time_stamp = current_time;
    }

    // STARTING DISTANCE MEASUREMENT

    while (true)
    {
        float distance = (encoder1.getDistance() + encoder2.getDistance()) / 2.0;

        if (distance > WIDTH_THRESHOLD)
        {
            valve1.turnOn(); // start water
            brush_motor.set_speed(255);
        }

        if (distance >= DISTANCE_THRESHOLD)
        {
            valve1.turnOff(); // STOP water
            brush_motor.set_speed(0);
        }

        if ((limitSwitch1.sense() == HIGH) || (proximitySensor1.sense() == HIGH))
        {
            mob_motor1.set_speed(0);
            mob_motor2.set_speed(0);
            brush_motor.stop();
            break;
        }
    }

    // BI DRECTIONAL MODE CHECK

    if (bidirectional_mode)
    {
        direction_movement = !direction_movement; // After reaching the end of its path, the robot will reverse direction on the next run instead of stopping.
    }

    // REVERSE MOVEMENT
    if (proximitySensor2.sense() == LOW)
    {
        SerialCom.println("Warning: Sensor I2 not working. System remains stopped.");
        return;
    }

    mob_motor1.set_speed(mob1_speed);
    mob_motor2.set_speed(mob2_speed);
    encoder1.reset();
    encoder2.reset();

    while (true)
    {
        float distance = (encoder1.getDistance() + encoder2.getDistance()) / 2.0;

        if (distance > WIDTH_THRESHOLD)
        {
            valve2.turnOn();            // Start water
            brush_motor.set_speed(255); // Start brush
        }

        if (distance >= DISTANCE_THRESHOLD)
        {
            valve2.turnOff();
            brush_motor.set_speed(0); // Stop brush
        }

        if ((limitSwitch2.sense() == HIGH) || (proximitySensor2.sense() == HIGH))
        {
            mob_motor1.set_speed(0);
            mob_motor2.set_speed(0);
            brush_motor.set_speed(0);
            break;
        }
    }

    SerialCom.println("System shutting down.");
}

//   // limitSwitch.sense();
//   // proximitySensor.sense();

//   uint32_t current_time = millis();

//   // Check if proximity sensors are working
//   if (!proximitySensor1.isWorking())
//   {
//       SerialCom.println("Warning: Sensor I1 not working. System remains stopped.");
//       return;
//   }

//   if (!proximitySensor2.isWorking())
//   {
//       SerialCom.println("Warning: Sensor I2 not working. System remains stopped.");
//       return;
//   }

//   // Print with a delay of 500 ms.
//   if ((current_time - time_stamp) > 500)
//   {
//     SerialCom.print("PID output: ");
//     SerialCom.println(mob2_speed);

//     SerialCom.print("RPM: ");
//     SerialCom.println(encoder1.rpm);

//     SerialCom.print("Average Velocity: ");
//     SerialCom.println(encoder1.velocity);
//   }

//   if ((current_time - time_stamp) > 50)
//   {
//     mob1_speed = pid_mob1.calculate(0.14, encoder1.velocity, delta_time);
//     mob2_speed = pid_mob2.calculate(0.14, encoder2.velocity, delta_time);

//     mob_motor1.set_speed(mob1_speed);
//     mob_motor2.set_speed(mob2_speed);

//     time_stamp = current_time;
//   }

//   delay(10);
}
