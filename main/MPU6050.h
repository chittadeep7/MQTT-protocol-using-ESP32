#define MPU6050_address 0x68
#define who_am_i 0x75
#define pwr 0x6B
#define ACC 0x3B
#define ACK 0x00
#define NACK 0x01

extern uint8_t buf[14];
extern int16_t AX, AY, AZ, TEMP, GX, GY, GZ;

struct data {
    float accx;
    float accy;
    float accz;
    float tem;
    float gyx;
    float gyz;
    float gyy;
};
extern struct data* map;
extern uint8_t buf[14];
extern int16_t AX, AY, AZ, TEMP, GX, GY, GZ;

void MPU6050_init(int dev_addr);
void MPU_sleep(int dev_addr);
void MPU6050_read(int dev_addr);
float acc(int16_t Acc_value);
float Temp(int16_t Temp_value);
float Gyro(int16_t Gyro_value);
void MPU(void);