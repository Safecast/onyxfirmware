#ifndef LOGREAD_H
#define LOGREAD_H

extern "C" {

void log_read_start();
int log_read_block(char *buf);

}

#endif
