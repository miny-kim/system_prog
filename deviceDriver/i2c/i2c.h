#ifndef I2C_H
#define I2C_H
#endif

void i2c_open(void);
void i2c_release(void);
void i2c_setSlave(u_int8_t addr);
int i2c_write_one(u_int8_t buf);
bool i2c_write(u_int8_t* buf, int len);
bool i2c_read(u_int8_t* buf, int len);
