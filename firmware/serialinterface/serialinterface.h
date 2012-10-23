#ifndef SERIALINTERFACE_H
#define SERIALINTERFACE_H

void serial_initialise();
void serial_sendlog();
void serial_eventloop();

void serial_write_string(const char *str);
#endif
