#ifndef I2C_H
#define I2C_H

void i2c_open();
void i2c_release();
void i2c_setSlave(u_int8_t addr);
bool i2c_write(u_int8_t* buf, int len);
bool i2c_read(u_int8_t* buf, int len);