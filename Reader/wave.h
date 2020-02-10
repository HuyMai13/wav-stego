// wave.h
// 
// contains wave structures
//

// built-in windows types
//typedef unsigned char		BYTE;
//typedef unsigned short	WORD;
//typedef unsigned int		DWORD;

#define SUCCESS 0
#define FAILURE -1
#define MAX_CHUNKS 8

typedef struct{
    unsigned int	chunkID;
    unsigned int	chunkSize;
} W_CHUNK;

typedef struct{
    unsigned short	compCode;				
    unsigned short   numChannels;         
    unsigned int   sampleRate;    
    unsigned int   avgBytesPerSec;   //  avgBytesPerSec = sampleRate * blockAlign 
    unsigned short    blockAlign;       //  blockAlign = bitsPerSample / 8 * numChannels
    unsigned short    bitsPerSample;    
} W_FORMAT;

//typedef struct{
//} W_DATA;


