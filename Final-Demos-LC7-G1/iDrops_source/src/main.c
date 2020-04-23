/* most of these includes are needed for nuklear (it doesnt pull its
   down dependencies) */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <portaudio.h>
#include <sndfile.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "config.h"
#include "nuklear.h"

#ifdef OPENGL4
#include "nuklear_glfw_gl4.h"
#endif

#ifdef OPENGL2
#include "nuklear_glfw_gl2.h"
#endif

#define INCLUDE_STYLE
#ifdef INCLUDE_STYLE
#include "style.c"
#endif

#include "sounds.h"
#include "files.h"

/* BEGINNING OF PROGRAM */

int BufferedCallback(const void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo *timeInfo,
                     PaStreamCallbackFlags statusFlags, void *inputData) {
  struct pa_callback_data *cbd = (struct pa_callback_data *)inputData;
  //struct sample *samples = (struct sample *)inputData;
  float *out = (float *)outputBuffer;
  
  (void)inputBuffer;
  (void)timeInfo;
  (void)statusFlags;
  
  if (*(cbd->mute) == 1) {
    for (unsigned long i = 0; i < framesPerBuffer; i++)
      *out++ = 0;
  }
  *(cbd->location) = write_audio_array(out, cbd, framesPerBuffer);
  return paContinue;
}

static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}


long unsigned int global_gain = 80;

int main(void)
{
  int i;
  PaStream *stream;
  PaError error;
  
  int location = 0, current_step = 0, mute = 0;
  float sample_len = 60.0f / DEFAULT_BPM;
  struct sample samples[INSTS];
  struct pa_callback_data callback_data;
  callback_data.samples = samples;
  callback_data.current_step = &current_step;
  callback_data.location = &location;
  callback_data.sample_len = &sample_len;
  callback_data.mute = &mute;
  
  struct dir_list dl = getconfigs();
  
  if (dl.list[0] == NULL) {
    fprintf(stderr, "No config files found. Please add one.\n");
    return 1;
  }
  if(dl.status != DIR_OK) {
    fprintf(stderr, "Failed to open config files\n");
    return 1;
  }
  
  /* @FIX: we never verify config files ahead of time. Need to check
     if samples actually exist */
  size_t dl_loc = 0;
  FILE* cfg = fopen(dl.list[dl_loc], "r");
  struct config_info cfg_info = parsecfg(cfg, INSTS);
  fclose(cfg);
  
  for (i = 0; i < (int) cfg_info.length; i++) {

    samples[i].echo = 0;
    samples[i].gain = 1.0;
    samples[i].mute = 0;

    for (int k = 0; k < TICKS; k++) {
      samples[i].active[k] = 0;
    }
  
    if (cfg_info.status == CFG_OK || cfg_info.status == CFG_TOO_BIG) {
    samples[i].snddata.filename = cfg_info.fp.names[i];
    samples[i].snddata.name = cfg_info.fp.files[i];
    }
  }
    
  if (write_samples(samples, cfg_info) == SND_ERROR) {
    fprintf(stderr, "Invalid config file(s).\n");
    return 1;
  }
  
  error = Pa_Initialize();
  if (error != paNoError) {
    fprintf(stderr, "Failed to initialize portaudio \n");
    return 1;
  }
  
  error = Pa_OpenDefaultStream(&stream, 0, samples[0].snddata.info.channels,
                               paFloat32, samples[0].snddata.info.samplerate,
                               FRAMES_PER_BUFFER, BufferedCallback, &callback_data);
  
  error = Pa_StartStream(stream);
  if (error != paNoError) {
    fprintf(stderr, "Failed to start stream\n");
  }
  
  
  /* Platform */
  static GLFWwindow *win;
  int width = 0, height = 0;
  struct nk_context *ctx;
  struct nk_colorf bg;
  struct nk_image img;
  
  /* GLFW */
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    fprintf(stdout, "[GFLW] failed to init!\n");
    exit(1);
  }
  
#ifdef OPENGL4
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#endif
  
  win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "iDrops", NULL, NULL);
  glfwMakeContextCurrent(win);
  glfwGetWindowSize(win, &width, &height);
  
  /* OpenGL */
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to setup GLEW\n");
    exit(1);
  }
  
  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
  
#ifdef OPENGL4
  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
#endif
  
#ifdef OPENGL2
  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
#endif
  {struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "./Roboto-Regular.ttf", 14, 0);
    nk_glfw3_font_stash_end();
    nk_style_set_font(ctx, &roboto->handle);}
  
#ifdef INCLUDE_STYLE
  set_style(ctx, THEME_WHITE);
  /*set_style(ctx, THEME_RED);*/
  /*set_style(ctx, THEME_BLUE);*/
  /*set_style(ctx, THEME_DARK);*/
#endif
  
#ifdef OPENGL4
  /* Some stuff from the example code
     that is needed for some reason */
  { int tex_index = 0;
    enum {tex_width = 256, tex_height = 256};
    char pixels[tex_width * tex_height * 4];
    memset(pixels, 128, sizeof(pixels));
    tex_index = nk_glfw3_create_texture(pixels, tex_width, tex_height);
    img = nk_image_id(tex_index);}
#endif
  
  bg.r = 0.20f, bg.g = 0.20f, bg.b = 0.20f, bg.a = 1.0f;
  
  while (!glfwWindowShouldClose(win))
    {
      /* Input */
      glfwPollEvents();
      nk_glfw3_new_frame();
      
      if (nk_begin(ctx, "options", nk_rect(1375, 0, WINDOW_WIDTH - 1375, WINDOW_HEIGHT), WINDOW_OPTIONS)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
        {
          
          nk_layout_row_push(ctx, 0.20f);
          nk_labelf(ctx, NK_TEXT_LEFT, "Volume (%zu):" , global_gain);
          nk_layout_row_push(ctx, 0.80f);
          nk_progress(ctx, &global_gain, MAX_GLOBAL_GAIN, NK_MODIFIABLE);
        }
        nk_layout_row_end(ctx);
        
        
        static int popup_active;
        
        nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 2);

        nk_layout_row_push(ctx, 0.2f);
        nk_label(ctx, "Drum set:", NK_LEFT);

        nk_layout_row_push(ctx, 0.8f);
        int dl_loc_old = dl_loc;
        dl_loc = nk_combo(ctx, (const char**) dl.list, dl.length, dl_loc, 25, nk_vec2(275,200));
        if (dl_loc != dl_loc_old) {
          mute = 1;
          /* Reload samples */

          cfg = fopen(dl.list[dl_loc], "r");
          cfg_info = parsecfg(cfg, INSTS);
          fclose(cfg);

          for (i = 0; i < cfg_info.length; i++) {
            free(samples[i].data);
          }

          for (i = 0; i < (int) cfg_info.length; i++) {
            if (cfg_info.status == CFG_OK || cfg_info.status == CFG_TOO_BIG) {
              samples[i].snddata.filename = cfg_info.fp.names[i];
              samples[i].snddata.name = cfg_info.fp.files[i];
            }
          }
          
          write_samples(samples, cfg_info);
          
          mute = 0;
        }
  
        nk_layout_row_end(ctx);

        nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 1);
        nk_layout_row_push(ctx, 1.0f);
        if(nk_button_label(ctx, "Export loop to file"))
          popup_active = 1;
        nk_layout_row_end(ctx);
        
        if (popup_active)
          {
            static struct nk_rect s = {20, 75, 325, 100};
            if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Export to file", 0, s))
              {
                
                static char text[64];
                static int text_len;
                static int active = 0;
                
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_label(ctx, "Filename:", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 25, 1);
                active = nk_edit_string(ctx, NK_EDIT_FIELD|NK_EDIT_SIG_ENTER, text, &text_len, 64,  nk_filter_default);
                
                nk_layout_row_dynamic(ctx, 25, 2);
                if (nk_button_label(ctx, "Save")) {
                  popup_active = 0;
                  
                  struct pa_callback_data callback_tmp;
                  int current_step_tmp = current_step;
                  int location_tmp = 0;
                  callback_tmp.current_step = &current_step_tmp;
                  callback_tmp.location = &location_tmp;
                  callback_tmp.sample_len = callback_data.sample_len;
                  callback_tmp.samples = callback_data.samples;
                  callback_tmp.mute = 0;
                  /* @HACK evil malloc */
                  SNDFILE* output_snd = sf_open(text, SFM_WRITE, &samples[0].snddata.info);
                  float *mixbuffer = malloc(sizeof(float) * samples[0].snddata.info.channels *
                                            samples[0].snddata.info.samplerate * MAX_SAMPLE_LEN * TICKS);
                  
                  if (mixbuffer == NULL) {
                    fprintf(stderr,"Failed to export wav file.\n");
                  } else {
                    write_audio_array(mixbuffer, &callback_tmp, samples[0].snddata.info.channels *
                                      samples[0].snddata.info.samplerate * sample_len * TICKS);
                    
                    sf_write_float(output_snd, mixbuffer, SAMPLE_RATE * sample_len * TICKS);
                  }
                  
                  free(mixbuffer);
                  sf_write_sync(output_snd);
                  sf_close(output_snd);
                  
                  //SNDFILE* output_snd = sf_open(text, SFM_WRITE, &samples[0].snddata.info);
                  //float *mixbuffer = malloc(sizeof(float) * samples[0].snddata.info.channels *
                  //                          samples[0].snddata.info.samplerate * MAX_SAMPLE_LEN);
                  //
                  //for (int i = 0; i < TICKS; i++) {
                  //  for (int k = 0; k < SAMPLE_RATE * sample_len; k++)
                  //    mixbuffer[k] = 0;
                  
                  //  for (int j = 0; j < INSTS; j++) {
                  //    if (samples[j].active[i]) {
                  //      int loc = 0;
                  //      for (int k = 0; k < SAMPLE_RATE * sample_len; k++, loc++) {
                  //        mixbuffer[k] += samples[j].data[k];
                  //      }
                  //    }
                  //    
                  //  }
                  
                  //  for (int k = 0; k < SAMPLE_RATE * sample_len; k++)
                  //    mixbuffer[k] /= INSTS;
                  
                  //  sf_write_float(output_snd, mixbuffer, SAMPLE_RATE * sample_len);
                  //}
                  //
                  //free(mixbuffer);
                  
                  
                  nk_popup_close(ctx);
                }
                if (nk_button_label(ctx, "Cancel")) {
                  popup_active = 0;
                  nk_popup_close(ctx);
                }
                nk_popup_end(ctx);
              } else popup_active = nk_false;
          }
        
        
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
        {
          static float bpm = DEFAULT_BPM;
          nk_layout_row_push(ctx, 0.20f);
          nk_labelf(ctx, NK_TEXT_LEFT, "BPM (%3.1f):" , bpm);
          //nk_label(ctx, buffer, NK_TEXT_LEFT);
          nk_layout_row_push(ctx, 0.80f);
          if(nk_slider_float(ctx, MIN_BPM, &bpm, 300.0f, 1.0f)) {
            sample_len = 60.0 /  bpm;
          }
          
        }
        nk_layout_row_end(ctx);
        
        
        char buffer[128];
        for (int j = 0; j < INSTS; j++) {
          sprintf(buffer, "%s %s", samples[j].snddata.name, "effects");
          if(nk_tree_push_id(ctx, NK_TREE_TAB, buffer, NK_MINIMIZED, j))
            {
              nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 1);
              {
                nk_layout_row_push(ctx, 1.0f);
                if (nk_button_label(ctx, "Clear ticks"))
                  memset(samples[j].active, 0, TICKS * sizeof(int));
                nk_layout_row_push(ctx, 1.0f);
                if (samples[j].mute) sprintf(buffer, "Unmute");
                else sprintf(buffer, "Mute");
                if (nk_button_label(ctx, buffer))
                  samples[j].mute = !samples[j].mute;
              }
              nk_layout_row_end(ctx);
              nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 2);
              {
                nk_layout_row_push(ctx, 0.2f);
                if(nk_button_label(ctx, "Gain:")) {
                  samples[j].gain = 1;
                }
                nk_layout_row_push(ctx, 0.8f);
                nk_slider_float(ctx, 0, &samples[j].gain, MAX_LOCAL_GAIN, GAIN_CHANGE_AMOUNT);  
              }
              nk_layout_row_end(ctx);
              nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 2);
              {
                nk_layout_row_push(ctx, 0.2f);
                if(nk_button_label(ctx, "Echo length:")) {
                  samples[j].echo = 0;
                }
                nk_layout_row_push(ctx, 0.8f);
                nk_slider_int(ctx, -MAX_ECHO, &samples[j].echo, MAX_ECHO, 1);
              }
              
              nk_layout_row_end(ctx);
              nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 2);
              {
                nk_layout_row_push(ctx, 0.2f);
                if(nk_button_label(ctx, "Echo amount:")) {
                  samples[j].echo_amount = 0;
                }
                nk_layout_row_push(ctx, 0.8f);
                nk_slider_float(ctx, -2.0f, &samples[j].echo_amount, 2.0f, 0.1f);
              }
              
              nk_layout_row_end(ctx);
              nk_layout_row_end(ctx);
              nk_tree_pop(ctx);
            }
        }
        
      }
      nk_end(ctx);
      
      if (nk_begin(ctx, "Sequencer", nk_rect(0, 0, TICKS * 80 + 100, WINDOW_HEIGHT), WINDOW_OPTIONS)) {
        for (int j = 0; j < INSTS; j++) {
          if(nk_tree_push_id(ctx, NK_TREE_TAB, samples[j].snddata.name, NK_MAXIMIZED, j))
            {
              int i;
              nk_layout_row_static(ctx, 50, 80, TICKS);
              for (i = 0; i < TICKS; ++i) {
                nk_selectable_label(ctx, "   ", NK_TEXT_CENTERED, &samples[j].active[i]);
              }
              
              nk_layout_row_begin(ctx, NK_STATIC, 20, TICKS + 1);
              
              
              
              nk_layout_row_push(ctx, 30);
              nk_label(ctx, " ", NK_TEXT_LEFT);
              for (i = 0; i < TICKS; ++i) {
                nk_layout_row_push(ctx, 80);
                nk_option_label(ctx, "", (current_step == i));
                //nk_label(ctx, "1", NK_TEXT_CENTERED);
              }
              
              nk_layout_row_end(ctx);
              nk_tree_pop(ctx);
            }
        }
      }
      
      nk_end(ctx);
      /* Draw */
      glfwGetWindowSize(win, &width, &height);
      glViewport(0, 0, width, height);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(bg.r, bg.g, bg.b, bg.a);
      nk_glfw3_render(NK_ANTI_ALIASING_ON);
      glfwSwapBuffers(win);
    }
  
  error = Pa_StopStream(stream);
  if (error != paNoError) {
    fprintf(stderr, "Failed to stop stream\n");
    return 1;
  }
  
  error = Pa_CloseStream(stream);
  if (error != paNoError) {
    fprintf(stderr, "Failed to close steam\n");
    return 1;
  }
  
  for (i = 0; i < INSTS; i++) {
    free(samples[i].data);
  }
  
  /* @HACK */
  for (i = 0; i < (int) dl.length; i++) {
    free(dl.list[i]);
  }
  free(dl.list);
  
  nk_glfw3_shutdown();
  glfwTerminate();
  return 0;
}
