// Huy Mai
// Fred

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wave.h"

int readChunkHeader(FILE *fp, W_CHUNK *pChunk){
    int x;

    // size = 1, count = 8 bytes
    x = (int) fread(pChunk, 1, 8, fp);
    if(x != 8) return(FAILURE);

    return(SUCCESS);
} // readChunkHeader

// reads in the data portion of a chunk
unsigned char *readChunkData(FILE *fp, int size){
    unsigned char *ptr;
    int tmp, x;

    tmp = size%2;	// size MUST be WORD aligned
    if(tmp != 0) size = size + 1;

    ptr = (unsigned char *) malloc(size);
    if(ptr == NULL){
        printf("\n\nError, could not allocate %d bytes of memory!\n\n", size);
	    return(NULL);
    }

    x = (int) fread(ptr, 1, size, fp);
    if(x != size){
	    printf("\n\nError reading chunkd data!\n\n");
	    return(NULL);
    }
  
    return(ptr);	
} // readChunkData


// prints out wave format info
void printFormat(W_FORMAT fmt){
	printf("\n\nWAVE FORMAT INFO\n\n");
	printf("Compression Code:		%d\n", fmt.compCode);
	printf("Number of Channels:		%d\n", fmt.numChannels);
	printf("Sample Rate: 			%d\n", fmt.sampleRate);
	printf("Average Bytes/Second:		%d\n", fmt.avgBytesPerSec);
	printf("Block Align: 			%d\n", fmt.blockAlign);
	printf("Bits per Sample: 		%d\n", fmt.bitsPerSample);
	return;
} // printFormat

char *parseArgv(char *argv[], int argc, int *nlsb, int *nSample){
    char *ptr;
    if(argc == 6){
        *nlsb = strtol(argv[3], &ptr, 0);
        *nSample = strtol(argv[4], &ptr, 0);
    } else {
        *nlsb = strtol(argv[2], &ptr, 0);
        *nSample = strtol(argv[3], &ptr, 0);
    }
    return ptr;
} // parseArgv

int power(int base, int exp){
    int i;
    int ans = 1;
    for(i=0; i < exp; i++)
        ans *= base;
        
    return ans;
} // power

void writeStegoFile(FILE *fp, unsigned char *pData, int size){
    FILE *stegoFile = fopen("Stego.wav", "wb");
    if(!stegoFile){
        perror("Could not open new stego file\n\n");
       	return;
    }

    unsigned char *buffer = (unsigned char*)malloc(sizeof(char)*44);
    if(!buffer){
        printf("\n\nCould not allocate %d bytes of memory!\n\n", size);
        return;
    }

    fseek(fp, 0 , SEEK_SET);
    fread(buffer, sizeof(char)*44, 1, fp);
    fwrite(buffer, sizeof(char)*44, 1, stegoFile);
    fwrite(pData, size, 1, stegoFile);
    return;
} // writeStegoFile

char avgLsb_8_bit(unsigned char *data, int nlsb, int nSample){
    int i, sum = 0;
    for(i=0; i<nSample; i++)
        sum += data[i];

    return (sum / nSample) & (power(2, nlsb) - 1);
} // avgLsb_8_bit

int avgLsb_16_bit(int16_t *data, int nlsb, int nSample){
    int i;
    long sum = 0;
    for(i=0; i<nSample; i++)
        sum += data[i];

    return (sum / nSample) & (power(2, nlsb) - 1);
} // avgLsb_16_bit

int hide_8_bit(unsigned char *pData, char *msgFileName, int dataSize, int nlsb, int nSample){
    uint32_t size;
    
    FILE *msgFile = fopen(msgFileName, "rb");
    // get msg file size
    if(msgFile){
        fseek(msgFile, 0 , SEEK_END);
        size = ftell(msgFile);
        fseek(msgFile, 0 , SEEK_SET);
    } else {
        printf("Could not open file '%s'\n\n", msgFileName);
       	return 0;
    }

    int estCapacity = dataSize/nSample*nlsb;
    if(size*8 >= estCapacity){
        printf("Estimated capacity is too low for hiding\n");
        return 0;
    }

    // read msg file to buffer
    unsigned char *msg = (unsigned char*)malloc(sizeof(char)*size*8);
    fread(msg, sizeof(char), size*8, msgFile);
    memset(pData, 0, 4);

    // write hidden msg size in the first 4 byte
    // NOTE : SIZE STORE IN LITTLE EDIAN
    // (modify extract func to get big edian)
    pData[0] = size & 0x000000ff;
    pData[1] = (size & 0x0000ff00) >> 8u;
    pData[2] = (size & 0x00ff0000) >> 16u;
    pData[3] = (size & 0xff000000) >> 24u;
    printf("Message size: %d byte\n", size);

    int i, b;
    long idx = 4;    // Start from sample 4
    unsigned char avg, bit;
    unsigned char buf = 0;
    int tmp = nlsb - 1;
    int hide = 0;
    
    for(i=0; i < size; i++){   // go through msg 1 byte at a time
        for(b=7; b >= 0; b--){  // go from msb to lsb
            // get 1 bit and set buffer
            bit = (msg[i] & (1 << b)) >> b;
            if(bit)
                buf |= (1 << tmp);
            
            if(tmp <= 0){   // check if buffer full
                avg = avgLsb_8_bit(&(pData[idx]), nlsb, nSample);

                if(pData[idx] <= 128){   // prevent overflow
                    while(avg != buf){
                        pData[idx] += 1;
                        avg = avgLsb_8_bit(&(pData[idx]), nlsb, nSample);
                    }
                } else {
                    while(avg != buf){
                        pData[idx] -= 1;
                        avg = avgLsb_8_bit(&(pData[idx]), nlsb, nSample);
                    }
                }
                
                buf = 0;
                tmp = nlsb;
                idx += nSample;
            }
            tmp--;
            hide++;
        }   
    }
    return hide;
} // hide_8_bit

int hide_16_bit(unsigned char *pChar, char *msgFileName, int dataSize, int nlsb, int nSample){
    uint32_t size;
    
    FILE *msgFile = fopen(msgFileName, "rb");
    // get msg file size
    if(msgFile){
        fseek(msgFile, 0 , SEEK_END);
        size = ftell(msgFile);
        fseek(msgFile, 0 , SEEK_SET);
    } else {
        printf("Could not open file '%s'\n\n", msgFileName);
       	return 0;
    }

    int estCapacity = dataSize/(nSample*2)*nlsb;
    if(size*8 >= estCapacity){
        printf("Estimated capacity is too low for hiding\n");
        return 0;
    }

    // read msg file to buffer
    unsigned char *msg = (unsigned char*)malloc(sizeof(char)*size*8);
    fread(msg, sizeof(char), size*8, msgFile);
    memset(pChar, 0, 4);

    // write hidden msg size in the first 4 byte
    // NOTE : SIZE STORE IN LITTLE EDIAN
    // (modify extract func to get big edian)
    pChar[0] = size & 0x000000ff;
    pChar[1] = (size & 0x0000ff00) >> 8u;
    pChar[2] = (size & 0x00ff0000) >> 16u;
    pChar[3] = (size & 0xff000000) >> 24u;
    printf("Message size: %d byte\n", size);

    int16_t *pData = (int16_t *)pChar;

    int i, b;
    long idx = 4;    // First index after size
    unsigned char bit;
    int16_t avg;
    int16_t buf = 0;
    int tmp = nlsb - 1;
    int hide = 0;
    
    for(i=0; i < size; i++){   // go through msg 1 byte at a time
        for(b=7; b >= 0; b--){  // go from msb to lsb
            // get 1 bit and set buffer
            bit = (msg[i] & (1 << b)) >> b;
            if(bit)
                buf |= (1 << tmp);
            
            if(tmp <= 0){   // check if buffer full
                avg = avgLsb_16_bit(&(pData[idx]), nlsb, nSample);

                if(pData[idx] <= 0){   // prevent overflow
                    while(avg != buf){
                        pData[idx] += 1;
                        avg = avgLsb_16_bit(&(pData[idx]), nlsb, nSample);
                    }
                } else {
                    while(avg != buf){
                        pData[idx] -= 1;
                        avg = avgLsb_16_bit(&(pData[idx]), nlsb, nSample);
                    }
                }
                
                buf = 0;
                tmp = nlsb;
                idx += nSample;
            }
            tmp--;
            hide++;
        }   
    }
    return hide;
} // hide_8_bit

void extract_16_bit(unsigned char *pChar, int nlsb, int nSample){
    int16_t *pData = (int16_t *)pChar;

    uint32_t size = *((uint32_t *)pData);
    printf("Hidden data: %u byte\n", size);

    unsigned char *msg = (unsigned char*)malloc(sizeof(char)*size);
    memset(msg, 0, size);

    int i, b, lsb;
    int tmp = -1;
    int idx = 4;
    char bit;

    for(i=0; i < size; i++){    // loop through msg 1 byte at a time
        for(b=7; b >= 0; b--){  // loop from lsb to msb
            if(tmp < 0){    // get new average lsb when done writing
                tmp = nlsb - 1;
                lsb = avgLsb_16_bit(&(pData[idx]), nlsb, nSample);
                idx += nSample;
            }

            bit = (lsb & (1 << tmp)) >> tmp;
            if(bit)
                msg[i] |= (1 << b);
            
            tmp--;
        }
    }
    
    FILE *hiddenFile = fopen("hidden", "wb");
    if(!hiddenFile){
        perror("Could not open new hidden file\n\n");
       	return;
    }

    fwrite(msg, size, 1, hiddenFile);
    return;
} // extract_16_bit

void extract_8_bit(unsigned char *pData, int nlsb, int nSample){
    uint32_t size = *((uint32_t *)pData);
    printf("Hidden data: %u byte\n", size);

    unsigned char *msg = (unsigned char*)malloc(sizeof(char)*size);
    memset(msg, 0, size);

    int i, b, lsb;
    int tmp = -1;
    int idx = 4;    // First index after size
    char bit;

    for(i=0; i < size; i++){    // loop through msg 1 byte at a time
        for(b=7; b >= 0; b--){  // loop from lsb to msb
            if(tmp < 0) {   // get new average lsb when done writing
                tmp = nlsb - 1;
                lsb = avgLsb_8_bit(&(pData[idx]), nlsb, nSample);
                idx += nSample;
            }

            bit = (lsb & (1 << tmp)) >> tmp;
            if(bit)
                msg[i] |= (1 << b);

            tmp--;
        }
    }
    
    FILE *hiddenFile = fopen("hidden", "wb");
    if(!hiddenFile){
        perror("Could not open new hidden file\n\n");
       	return;
    }

    fwrite(msg, size, 1, hiddenFile);
    return;
} // extract_8_bit

void main(int argc, char *argv[]){
    FILE *fp;
    FILE *msg;
    unsigned int fileSize;
    int x, cnt, dataFlag, formatFlag;
    W_CHUNK chunk[MAX_CHUNKS] = {0};		// assuming max of 8 chunks, should only be 3 for you
    unsigned char *pChunkData[MAX_CHUNKS];
    W_FORMAT format;		// only 1 format chunk

    if(argc == 6){
        if(strcmp(argv[5], "-h")){
            printf("\nUSAGE: cover.wav message.txt numLsb numSample -h");
            return;
        }
    } else if(argc == 5){
        if(strcmp(argv[4], "-e")){
            printf("\nUSAGE: stego.wav numLsb numSample -e\n");
	        return;
        }
    } else {
        printf("\nUSAGE: cover.wav message.txt numLsb numSample -h\n\tstego.wav numLsb numSample -e\n");
        return;
    }

    fp = fopen(argv[1], "rb");
    if(fp == NULL){
        printf("Could not file file named '%s.'\n\n", argv[1]);
       	return;
    }

    // pChunk[0] is the chunk representing the file header
    readChunkHeader(fp, &chunk[0]);

    // check to make sure it is a RIFF file
    if(memcmp( &(chunk[0].chunkID), "RIFF", 4) != 0){
        printf("\n\nError, file is NOT a RIFF file!\n\n");
	    return;
    }
    fileSize = chunk[0].chunkSize + 8;

    // check to make sure it is a wave file
    pChunkData[0] = readChunkData(fp, 4);

    if(memcmp( pChunkData[0], "WAVE", 4) != 0){
        printf("\n\nError, file is not a WAVE file!\n\n");
	    return;
    }

    // chunk[1] should be format chunk, but if not, skip
    cnt = 1;
    dataFlag = -1;
    formatFlag = -1;
    while(cnt < MAX_CHUNKS){
        readChunkHeader(fp, &chunk[cnt]);

	    // read in chunk data
        pChunkData[cnt] = readChunkData(fp, chunk[cnt].chunkSize);
        if(pChunkData[cnt] == NULL) return;

        if(memcmp( &(chunk[cnt].chunkID), "data", 4) == 0)
            dataFlag = cnt;	// if find data chunk, take note

        if(memcmp( &(chunk[cnt].chunkID), "fmt ", 4) == 0){
            formatFlag = cnt;	//	marks which chunk has format data
            break;	// found format chunk
	    }

	    cnt++;
    }
    if(cnt == MAX_CHUNKS){
        printf("\n\nError, format chunk not found after 8 tries!\n\n");
	    return;
    }

    // check format size to make sure this is not a fancy WAVE file
    if(chunk[cnt].chunkSize != 16){
        printf("\n\nError, this WAVE file is not a standard format - we will not use this one!\n\n");
	    return;
    }

    // put format chunk in our special variable
    // format chunk data already contained in pChunkData[cnt]
    memcpy(&format, pChunkData[cnt], 16);

    // make sure we are working with uncompressed PCM data
    if(format.compCode != 1){
        printf("\n\nError, this file does not contain uncompressed PCM data!\n\n");
	    return;
    }
    printFormat(format);

    if(dataFlag == -1){	// have not found data chunk yet
        cnt++;
        while(cnt < MAX_CHUNKS){
	        readChunkHeader(fp, &chunk[cnt]);
            // read in chunk data
            pChunkData[cnt] = readChunkData(fp, chunk[cnt].chunkSize);
            if(pChunkData[cnt] == NULL) return;

            if(memcmp( &(chunk[cnt].chunkID), "data", 4) == 0){
                dataFlag = cnt;	// found data chunk
                break;
            }

	        cnt++;
        }
    }

    // pChunkData[dataFlag] is a pointer to the begining of the WAVE data
    // if 8 bit, then it is unsigned	0 to 255
    // if 16 bit, then it is signed		-32768 to +32767
    // ask me any other questions
    // the web page should answer others

    int h;
    char *msgFile = argv[2];
    int *nlsb = (int *)malloc(sizeof(int));
    int *nSample = (int *)malloc(sizeof(int));

    parseArgv(argv, argc, nlsb, nSample);

    printf("\n");
    if(format.bitsPerSample == 8){
        if(argc == 6){
            h = hide_8_bit(pChunkData[dataFlag], msgFile, chunk[dataFlag].chunkSize, *nlsb, *nSample);
            printf("\nHide %d bits", h);
            writeStegoFile(fp, pChunkData[dataFlag], chunk[dataFlag].chunkSize);
        } else {
            extract_8_bit(pChunkData[dataFlag], *nlsb, *nSample);
            printf("Extraction complete");
        }
    }
    if(format.bitsPerSample == 16){
        if(argc == 6){
            int h = hide_16_bit(pChunkData[dataFlag], msgFile, chunk[dataFlag].chunkSize, *nlsb, *nSample);
            printf("\nHide %d bits", h);
            writeStegoFile(fp, pChunkData[dataFlag], chunk[dataFlag].chunkSize);
        } else {
            extract_16_bit(pChunkData[dataFlag], *nlsb, *nSample);
            printf("Extraction complete");
        }
    }

    printf("\n");
    fclose(fp);
    return;
} // main
