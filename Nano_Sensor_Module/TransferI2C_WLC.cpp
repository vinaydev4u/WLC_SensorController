#include <TransferI2C_WLC.h>

//Captures address and size of struct
void TransferI2C_WLC::begin(uint8_t * ptr, uint8_t length, TwoWire *theSerial){
	
	address = ptr;
	size = length;
	_serial = theSerial;
	//dynamic creation of rx parsing buffer in RAM
	rx_buffer = (uint8_t*) malloc(size);
	tx_buffer = (uint8_t*) malloc(((uint8_t)(size+3)));
	flagSendData = false;
	rx_len = 0;
}

void TransferI2C_WLC::flagSlaveSend() {
	flagSendData = true;
}

//Sends out struct in binary, with header, length info and checksum
void TransferI2C_WLC::sendData(uint8_t i2c_address){
	if(i2c_address == 0) {
		//if the default i2c_address of "0" is detected than this must be a SLAVE sendData function call.
	
	
	}
	else {
		//if an i2c_address is specified in the function call, then this must be a MASTER sending data to a slave
		flagSendData = true;
		_serial->beginTransmission(i2c_address);
	
	}
  
	if(flagSendData) {
		
		tx_array_inx = 0;
		
		uint8_t CS = size;
		
		tx_buffer[tx_array_inx++] = 0x06;
		tx_buffer[tx_array_inx++] = 0x85;
		tx_buffer[tx_array_inx++] = size;
		
		for(int i = 0; i<size; i++) {
			CS^=*(address+i);
			tx_buffer[tx_array_inx++] = *(address+i);
		}
		tx_buffer[tx_array_inx++] = CS;
		//Serial.print("tx_buffer size=");
		//Serial.println(tx_array_inx);
		#if ARDUINO >= 100
			_serial->write(tx_buffer,tx_array_inx);
		#else
			_serial->send(tx_buffer,tx_array_inx);
		#endif

		if(i2c_address == 0) {
			//slave specific code
		}
		else {
			_serial->endTransmission(true);
		}
		
	}
	flagSendData = false;
}
	
boolean TransferI2C_WLC::receiveData(uint8_t i2c_address){
	
	uint8_t received_char;
	
	uint8_t total_size;
	

	if(i2c_address == 0) {
		//this is a Slave call for receiving data
	}
	else {
		total_size = size + 4;
		//this is a master call for receving data
		_serial->requestFrom(i2c_address, total_size);		// This initiates the communication to the slave device.
														// Note that the second parameter is the number of bytes requested from the slave device
														// The reason for the +3 is so that the slave can transmit the check bytes 0x06 and 0x85 and the
														// data structure size (for comparisons and error checking)
			
	} 
	//start off by looking for the header bytes. If they were already found in a previous call, skip it.
	if(rx_len == 0){
		//this size check may be redundant due to the size check below, but for now I'll leave it the way it is.
		if(_serial->available() >= 3){
		//this will block until a 0x06 is found or buffer size becomes less then 3.
			//Serial.println("Receiving Init Data");
			#if ARDUINO >= 100
			while((received_char = _serial->read()) != 0x06) {
				//Serial.print("Byte Rcvd: ");
				//Serial.println(received_char);
			#else
			while(_serial->receive() != 0x06) {
			#endif
				//This will trash any preamble junk in the serial buffer
				//but we need to make sure there is enough in the buffer to process while we trash the rest
				//if the buffer becomes too empty, we will escape and try again on the next call
				if(_serial->available() < 3) {
					//Serial.println("Failed < 3 bytes");
					return false;					
				}
				else {
					//Serial.println("Success! >3 bytes");
				}
			}
			#if ARDUINO >= 100
			if (_serial->read() == 0x85){
				rx_len = _serial->read();
			#else
			if (_serial->receive() == 0x85){
				rx_len = _serial->receive();
			#endif
				//Serial.println("Received SIZE Byte!");
		//make sure the binary structs on both Arduinos are the same size.
				if(rx_len != size){
					rx_len = 0;
					//Serial.println("Failed rx_len != size");
					return false;
				}
			}
		}
	}
  
	//we get here if we already found the header bytes, the struct size matched what we know, and now we are byte aligned.
	if(rx_len != 0){
		//Serial.println("Receive STRUCT Data");
		while(_serial->available() && rx_array_inx <= rx_len){
			#if ARDUINO >= 100
			rx_buffer[rx_array_inx++] = _serial->read();
			#else
			rx_buffer[rx_array_inx++] = _serial->receive();
			#endif
		}
    
		if(rx_len == (rx_array_inx-1)){
			//seem to have got whole message
			//last uint8_t is CS
			calc_CS = rx_len;
			for (int i = 0; i<rx_len; i++){
				calc_CS^=rx_buffer[i];
			} 
      
			if(calc_CS == rx_buffer[rx_array_inx-1]){//CS good
				memcpy(address,rx_buffer,size);
				rx_len = 0;
				rx_array_inx = 0;
			return true;
			}
		
			else{
				//failed checksum, need to clear this out anyway
				rx_len = 0;
				rx_array_inx = 0;
				//Serial.println("Failed Checksum");
				return false;
			}
        
		}
	}
	return false;
}
