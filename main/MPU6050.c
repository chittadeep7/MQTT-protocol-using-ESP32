#include <stdio.h>
#include "MPU6050.h"
#include "driver/i2c.h"

uint8_t buf[14];
int16_t AX, AY, AZ, TEMP, GX, GY, GZ;
struct data* map;

void MPU6050_init(int dev_addr)                                   //writes 0x01 to power management register 1 
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, pwr, true);
  i2c_master_write_byte(cmd, 0x01, true);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2000));
  i2c_cmd_link_delete(cmd);
}

//0x68 should be returned (slave address of MPU6050)
void MPU_sleep(int dev_addr)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, pwr, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, 0x40, true);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2000));
  i2c_cmd_link_delete(cmd);

}
void MPU6050_read(int dev_addr)                                                  //Reads the data from address 0x3B in a buffer
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, ACC, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
  for(uint8_t i = 0; i < 13; i++){
    i2c_master_read_byte(cmd, &buf[i], ACK);
  }
  i2c_master_read_byte(cmd, &buf[13], NACK);
 
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2000));
  i2c_cmd_link_delete(cmd);
  AX = ((buf[0] << 8) | buf[1]);
  AY = ((buf[2] << 8) | buf[3]);
  AZ = ((buf[4] << 8) | buf[5]);
  TEMP = ((buf[6] << 8) | buf[7]);
  GX = ((buf[8] << 8) | buf[9]);
  GY = ((buf[10] << 8) | buf[11]);
  GZ = ((buf[12] << 8) | buf[13]);
  
}

float acc(int16_t Acc_value)                               //accelerometer raw value / acc sensitivity factor
{
  return Acc_value / 16384.0;

}
float Temp(int16_t Temp_value)                              //Conversion to celcius
{
  return (Temp_value / 340.0) + 36.53;

}
float Gyro(int16_t Gyro_value)                              //gyroscope conversion
{
  return Gyro_value / 131.0;

}


void MPU(void)
{
    
    map = (struct data*) malloc(sizeof(struct data));                 //assigning memory to struct pointer
    MPU6050_init(MPU6050_address);                                    //setting PWR1 register to 0x01
    MPU6050_read(MPU6050_address);                                    //reading from the MPU6050
    //storing the values in the variables
    map->accx = acc(AX);              
    map->accy = acc(AY);
    map->accz = acc(AZ);
    map->tem = Temp(TEMP);
    map->gyx = Gyro(GX);
    map->gyy = Gyro(GY);
    map->gyz = Gyro(GZ);
}