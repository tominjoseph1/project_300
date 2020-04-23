#ifndef SOUNDS
#define SOUNDS

#include <stdio.h>
#include <portaudio.h>
#include <sndfile.h>
#include <stdlib.h>
#include "config.h"
#include "files.h"

enum snddata_state{SND_NO_ERROR, SND_ERROR};

struct snd {
  char *name;
  char *filename;
  SNDFILE *file;
  SF_INFO info;
};

struct sample {
  int active[TICKS];
  float *data;     /* pointer to buffered PCM (modified) */
  float gain;
  int mute;
  int echo;
  float echo_amount;
  struct snd snddata;
};

struct pa_callback_data {
  struct sample *samples;
  int *current_step;
  int *location;
  int *mute;
  float *sample_len;
};

int openSNDData(struct snd *data, int mode) {
  data->file = sf_open(data->filename, mode, &data->info);
  if (sf_error(data->file) != SF_ERR_NO_ERROR) {
    fprintf(stderr, "%s\n", sf_strerror(data->file));
    fprintf(stderr, "File: %s\n", data->filename);
    return SND_ERROR;
  }
  return SND_NO_ERROR;
}

/* @HACK */
extern long unsigned int global_gain;

int write_samples(struct sample *samples, struct config_info cfg_info)
{
for (int i = 0; i < cfg_info.length; i++) {
    if (openSNDData(&samples[i].snddata, SFM_READ) == SND_ERROR)
      return SND_NO_ERROR;
    
    int tmp = samples[i].snddata.info.channels *
      samples[i].snddata.info.samplerate * MAX_SAMPLE_LEN;
    samples[i].data = malloc(sizeof(float) * tmp);

    for(int k = 0; k < tmp; k++)
      samples[i].data[k] = 0.0f;

    sf_read_float(samples[i].snddata.file, samples[i].data, tmp);
    sf_close(samples[i].snddata.file);
 }
 return SND_NO_ERROR;
}


int write_audio_array(float *out, struct pa_callback_data *cbd,
                       size_t framesPerBuffer)
{

  struct sample *samples = cbd->samples;
  unsigned int i = 0;
  int j;
  float tmp;
  int location = *cbd->location;

  /* @HACK: all of this is very bad... */
  for (i = 0; i < framesPerBuffer; i++, location++) {
    tmp = 0;
    for (j = 0; j < INSTS; ++j) {
      if (samples[j].data == NULL)
        break;
      if (samples[j].active[*cbd->current_step] && !samples[j].mute) {
        tmp += samples[j].gain * samples[j].data[location];
        if (location < SAMPLE_RATE * (*cbd->sample_len) - cbd->samples[j].echo &&
            0 <= SAMPLE_RATE * (*cbd->sample_len) - cbd->samples[j].echo) {
          for (int k = 1; k < cbd->samples[j].echo; k++) {
            tmp += samples[j].gain * samples[j].echo_amount * (1 / (float) k) *
              samples[j].data[location + cbd->samples[j].echo +k];
          }
        }
      }
    }
    
    *out++ = ((float) global_gain / 100) * (tmp / INSTS);
    
    if (location >=
        SAMPLE_RATE * (*cbd->sample_len) - 1) {
      location = 0;
      *cbd->current_step = (*cbd->current_step + 1) % TICKS;
    }
  }

  return location;
}

#endif // SOUNDS
