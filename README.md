# wav-stego

Wav-stego is a steganography program that can embed any data or message file into another WAV file using average variable LSB method. Works on 8 bit and 16 bit WAV file.

Average variable LSB is a steganography technique that hide data in the LSB of the sample average. It will compute the average of a set of sample and compare the LSB of the result to the message data. If it is not equal, the program will add or subtract from the sample and compare the average again until the result match the message data. If it is equal, move on to the next set of sample. Repeat until the all the data is embeded in the cover file

Extraction is the reverse of embedding data using the above algorithm. Note that you need the same parameter on both embedding and extracting to get the correct data.

# Usage
Hiding msg.txt in cover.wav:

	cover.wav msg.txt numLsb numSample -h
  
Extract from stego.wav:

	stego.wav numLsb numSample -e
  
numLsb - number of lsb use to hide data

numSample - number of sample to use for average calculation
