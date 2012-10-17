/* place holder code for log_read API */

int bufPos = 0;
char logTestBuf[] = "Good evening gentlemen.\n";

void log_read_start() {
  bufPos = 0;
}

int log_read_block(char *buf) {
  int i;
  if( bufPos == 0 ) {
    for( i = 0; logTestBuf[i] != '\0'; i++ ) {
      buf[i] = logTestBuf[i];
    }
    bufPos = i;
    return(bufPos - 1);  // note how the trailing null character was not copied into the string to hash
  }
  
  return 0;
}
