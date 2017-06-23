#include "mbed.h"
#include "AUDIO_GRBoard.h"
#include "dec_wav.h"
#include "SdUsbConnect.h"

#include "common.h"

#include <string>

#define AUDIO_WRITE_BUFF_SIZE (4096)
#define AUDIO_WRITE_BUFF_NUM (9)
#define FILE_NAME_LEN (64)
#define TEXT_SIZE (64 + 1) //null-terminated

//32 bytes aligned! No cache memory
#if defined(__ICCARM__)
#pragma data_alignment = 32
static uint8_t audio_write_buff[AUDIO_WRITE_BUFF_NUM][AUDIO_WRITE_BUFF_SIZE] @".mirrorram";
#else
static uint8_t audio_write_buff[AUDIO_WRITE_BUFF_NUM][AUDIO_WRITE_BUFF_SIZE]
    __attribute((section("NC_BSS"), aligned(32)));
#endif
//Tag buffer
static uint8_t title_buf[TEXT_SIZE];
static uint8_t artist_buf[TEXT_SIZE];
static uint8_t album_buf[TEXT_SIZE];

AUDIO_GRBoard audio(0x80, (AUDIO_WRITE_BUFF_NUM - 1), 0);

static void callback_audio_write_end(void *p_data, int32_t result, void *p_app_data)
{
  if (result < 0)
  {
    printf("audio write callback error %ld\r\n", result);
  }
}

bool playSound(const char *filename)
{
  rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};

  printf("playSound %s\r\n", filename);

  FILE *fp = NULL;

  int buff_index = 0;
  size_t audio_data_size;
  dec_wav wav_file;

  audio.power();
  audio.outputVolume(1.0, 1.0);

  fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("playSound File Not Found\r\n");
    return false;
  }

  if (wav_file.AnalyzeHeder(title_buf, artist_buf, album_buf,
                            TEXT_SIZE, fp) == false)
  {
    fclose(fp);
    printf("playSound AnalyzeHeder == false\r\n");
    fp = NULL;
  }
  else if ((wav_file.GetChannel() != 2) || (audio.format(wav_file.GetBlockSize()) == false) || (audio.frequency(wav_file.GetSamplingRate()) == false))
  {
    printf("playSound Error\r\n");
    printf("Error File  :%s\r\n", filename);
    printf("Audio Info  :%dch, %dbit, %dHz\r\n", wav_file.GetChannel(),
           wav_file.GetBlockSize(), (int)wav_file.GetSamplingRate());
    printf("\r\n");
    fclose(fp);
    fp = NULL;
  }
  else
  {
    printf("File        :%s\r\n", filename);
    printf("Audio Info  :%dch, %dbit, %dHz\r\n", wav_file.GetChannel(),
           wav_file.GetBlockSize(), (int)wav_file.GetSamplingRate());
    printf("Title       :%s\r\n", title_buf);
    printf("Artist      :%s\r\n", artist_buf);
    printf("Album       :%s\r\n", album_buf);
    printf("\r\n");
  }

  while (1)
  {
    // file read
    uint8_t *p_buf = audio_write_buff[buff_index];

    audio_data_size = wav_file.GetNextData(p_buf, AUDIO_WRITE_BUFF_SIZE);
    if (audio_data_size > 0)
    {
      audio.write(p_buf, audio_data_size, &audio_write_async_ctl);
      buff_index++;
      if (buff_index >= AUDIO_WRITE_BUFF_NUM)
      {
        buff_index = 0;
      }
    }
    else
    {
      break;
    }

    // file close
    if ((audio_data_size < AUDIO_WRITE_BUFF_SIZE))
    {
      fclose(fp);
      fp = NULL;
      Thread::wait(500);
      break;
    }
  }

  // close check
  if (fp != NULL)
  {
    fclose(fp);
    fp = NULL;
  }

  printf("playSound Success%s\r\n", filename);

  return true;
}

bool playSoundFromData(char *data, int len)
{
  rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};

  audio.power();
  audio.outputVolume(1.0, 1.0);

  audio.write(data, len, &audio_write_async_ctl);

  return true;
}