#include "I2C_slave.h"
#include "Wire.h"

#define END_OF_I2C_MESSAGE              0x00
#define END_OF_I2C_COMMAND              0x0D      // carriage return in ASCII
#define END_OF_I2C_PARTIAL_MESSAGE      0x0A      // line feed in ASCII


enum {
    NOT_SYNCHRONIZED = 0,
    RECEIVE_COMMAND,
    RECEIVE_DATA
};

int16_t _messageID = 0;
char _message[MAX_LENGTH_MESSAGE];
bool _message_received = false;


void onReceiveI2C(int received_bytes);

I2C_slave::I2C_slave() {}

void I2C_slave::init(uint8_t i2cAdr, uint32_t clock) {
    _i2cAdr = i2cAdr;
    // setup i2C receive callback
    Wire.onReceive(onReceiveI2C);

    // init I2C busses
    Wire.begin(_i2cAdr);
    if (clock != 400000)
        clock = 100000;
    Wire.setClock(clock);
}

bool I2C_slave::message_available() {
    return _message_received;
}

int16_t I2C_slave::getMessage(char* message) {
    strncpy(message, _message, MAX_LENGTH_MESSAGE);
    _message_received = false;
    return _messageID;
}

// callback function for I2C receive
void onReceiveI2C(int received_bytes) {

  static uint8_t byte_counter = 0;
  static uint8_t state = RECEIVE_COMMAND;
  char buffer[7] = {0};                                    // range is -32768 ... 32767 -> max. 6 character plus terminating NULL

  for (uint8_t i = 0; i < received_bytes; i++) {
    switch (state) {
    case NOT_SYNCHRONIZED:
      if (Wire.read() == END_OF_I2C_MESSAGE) {              // wait for end of message to get synchronized
        state = RECEIVE_COMMAND;
        byte_counter = 0;
      }
      break;

    case RECEIVE_COMMAND:
      buffer[i] = Wire.read();
      if (buffer[i] == END_OF_I2C_COMMAND) {
        buffer[i] = 0x00;                                   // terminate string
        _messageID = atoi(buffer);
        state = RECEIVE_DATA;                               // next bytes are Data Bytes
      }
      if (i >= 6) {                                         // buffer overflow for messageID
        state == NOT_SYNCHRONIZED;                          // something went wrong, get a new synchronization
        byte_counter = 0;
        return;
      }
      break;

    case RECEIVE_DATA:
      _message[byte_counter] = Wire.read();
      if (_message[byte_counter] == END_OF_I2C_MESSAGE) {    // end of message detected, prepare for receiving next messageID
        byte_counter = 0;
        state = RECEIVE_COMMAND;
        _message_received = true;
        return;
      } else if(_message[byte_counter] == END_OF_I2C_PARTIAL_MESSAGE) {   // end of partial message detected, next transmission will be rest of message  
        return;                                             // keep receiving data
      } else {
        byte_counter++;                                     // get the next byte
      }
      break;

    default:
      break;
    }
  }
  state = NOT_SYNCHRONIZED;                                 // We shouldn't come here, something went wrong
}
