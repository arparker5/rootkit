// read_from_deivce.cpp
// Alex & Andrew
// 2019-10-24
// read string from device

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256          ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

int main(){
  int ret, fd;
  char stringToSend[BUFFER_LENGTH];
  printf("Opening device....\n");
  fd = open("/dev/ttyR0", O_RDWR);             // Open the device with read/write access
  if (fd < 0){
     perror("Failed to open the device...");
     return errno;
  }

  //printf("Press ENTER to read back from the device...\n");
  //getchar();

  printf("Reading from the device...\n");
  ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
  if (ret < 0){
     perror("Failed to read the message from the device.");
     printf("ret: %d\terrno: %d", ret, errno);
     return errno;
  }
  printf("The received message is: [%s]\n", receive);
  printf("End of the message\n");

  close(fd);

  return 0;
}
