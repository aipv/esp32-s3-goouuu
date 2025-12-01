#ifndef WAV_AUDIO_H
#define WAV_AUDIO_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma pack(1)

// RIFF 块 (12 bytes)
typedef struct {
    char chunkId[4];         // "RIFF"
    uint32_t chunkSize;      // 文件总大小 - 8
    char format[4];          // "WAVE"
} RiffChunk;

// fmt 块 (24 bytes)
typedef struct {
    char subchunk1Id[4];     // "fmt "
    uint32_t subchunk1Size;  // 16 for PCM
    uint16_t audioFormat;    // 1 for PCM
    uint16_t numChannels;    // 2 (双声道)
    uint32_t sampleRate;     // 16000 Hz
    uint32_t byteRate;       // SampleRate * NumChannels * (BitsPerSample/8)
    uint16_t blockAlign;     // NumChannels * (BitsPerSample/8)
    uint16_t bitsPerSample;  // 16 bit
} FmtChunk;

// data 块 (8 bytes + data)
typedef struct {
    char subchunk2Id[4];     // "data"
    uint32_t subchunk2Size;  // 音频数据总字节数
} DataChunk;
#pragma pack() // 恢复默认的字节对齐

#define WAV_AUDIO_HEADER_SIZE           44
#define WAV_AUDIO_CHUNK_SIZE            36
#define WAV_AUDIO_SAMPLE_RATE 			16000
#define WAV_AUDIO_DEFAULT_SAMPLE        32768
#define WAV_AUDIO_DEFAULT_DATA_SIZE     65536
#define WAV_AUDIO_DEFAULT_FILE_SIZE     65580
#define WAV_AUDIO_DEFAULT_TRUNK_SIZE    65572
#define WAV_AUDIO_NUM_OF_CHANNELS 		1
#define WAV_PCM16_BITS_PER_SAMPLE       16
#define WAV_PCM16_BYTE_PER_SMAPLE       2
#define WAV_PCM16_BLOCK_ALIGN           2
#define WAV_PCM16_BYTE_RATE             32000

esp_err_t wav_audio_init(void);
esp_err_t wav_audio_default_header(char *buffer);
esp_err_t wav_audio_data_process(char *input, char *output);

#endif // WAV_AUDIO_H