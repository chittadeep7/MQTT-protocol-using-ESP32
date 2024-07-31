# Use ESP32 to read MPU6050 sensor by using the I2C protocol and send the data to a server using WIFI and MQTT interface of ESP32
This project can be used by anyone to send sample data to a server. MPU6050 driver can be replaced by their own sensor drivers. The data is published in JSON format. 
You can send the data periodically =>>  by using a separate task and an eventgroup (commented out) to signal the mqtt start or using the light sleep mode feature of ESP32. The project has been built using ESP-IDF v5.2.2.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
|   |__ wifi.c
|   |__ mpu6050.c
└── README.md       
```

