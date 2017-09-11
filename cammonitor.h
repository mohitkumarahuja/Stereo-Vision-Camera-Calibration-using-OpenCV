#ifndef CAMMONITOR_H
#define CAMMONITOR_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
//#include <pam.h>
#include <inttypes.h>
#include <iostream>

#include "camwire/camwirebus.h"
#include "camwire/camwire.h"
#include "display.h"
#include "display-codes.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>


#define COMMAND_PROMPT		"> "
#define FRAMENUMBER_TOKEN	"%d"
#define DEFAULT_IMAGE_FILENAME	"cammonitor" FRAMENUMBER_TOKEN
#define DEFAULT_DISPLAY_TYPE	DISPLAY_XV
#define DEFAULT_SAVE_NUM_IMAGES	1
#define DEFAULT_SAVE_DELAY	0.0
#define SAFE_FRAME_PERIOD	1.5
#define MAX_FILENAME		1000
#define MAX_MESSAGE		1000
#define MAX_IMAGE_HEADER	200
#define MAX_TIMESTAMP		50
#define MAX_KEY_INPUT		100
#define BUF_LOW_MARK		0.1
#define BUF_HIGH_MARK		0.9
#define MAX_SETTINGS_LINE	500
#define MAX_PIXEL_CODING_STRING	10
#define MAX_PIXEL_TILING_STRING	10
#define WHITESPACE		" \t\r\n"
#define SETTINGS_COMMENT	'#'

typedef struct
{
    int shadowlevel;
    enum {stopped, running} activity;
    enum {continuous, single} acqtype;
    enum {internal, external} trigger;
    enum {falling, rising} polarity;
    double shutter;
    double framerate;
    int width;
    int height;
    Camwire_pixel coding;
    unsigned long maxval;
    Camwire_tiling tiling;
    int left;
    int top;
    double gain;
    double brightness;
    double white_bal[2];
    enum {gamma_off, gamma_on} gamma;
    enum {unchanged, linearized} response_curve;
    enum {colour_corr_off, colour_corr_on} colour_corr;
    double colour_coef[9];
    int num_buffers;
    long int framenumber;
    int save_num_images;
    double save_delay;
    char imagefilename[MAX_FILENAME+1];
}
Settings_t;

class camera {


/* Global variables needed in various places: */
  int use_fsync = 1;
  Display_type use_display = DEFAULT_DISPLAY_TYPE;
  int display_initialized = 0;
  Display_handle d_handle = NULL;
  void *linbuffer = NULL;
  const char *settings_format =
        "shadowlevel     %d\n"
    "activity        %d\n"
    "acqtype         %d\n"
    "trigger         %d\n"
    "polarity        %d\n"
    "shutter         %lg\n"
    "framerate       %lg\n"
    "width           %d\n"
    "height          %d\n"
    "coding          %d\n"
    "maxval          %lu\n"
    "tiling          %d\n"
    "left            %d\n"
    "top             %d\n"
        "gain            %lg\n"
        "brightness      %lg\n"
        "white_bal       %lg %lg\n"
    "gamma           %d\n"
    "response_curve  %d\n"
    "colour_corr     %d\n"
    "colour_coef     %lg %lg %lg %lg %lg %lg %lg %lg %lg\n"
    "num_buffers     %d\n"
    "framenumber     %ld\n"
    "save_num_images %d\n"
    "save_delay      %lg\n"
    "imagefilename   %s\n";
/* This is dreadful design, because the number and order of the fields
   in the format string above are important.  There must be exactly one
   conversion for each member of the Settings_t structure above.  It is
   used in settings_save() and update_settings(). */


/* Private prototypes: */
  int get_user_cam(const Camwire_handle *handle_array,
            const int num_cameras);
  int get_named_cam(const Camwire_handle *handle_array,
             const int num_cameras,
             const char *chip_id_string);
  void get_camera_settings(const Camwire_handle c_handle,
                   Settings_t *set);
  void default_noncamera_settings(Settings_t *set);
  void change_response_curve(Settings_t *set, const int newvalue);
  void show_menu(const Settings_t *set);
  void show_prompt(void);
  int user_input(const Camwire_handle c_handle, const int key,
              Settings_t *set);
  int set_shadow(const Camwire_handle c_handle);
  int set_run_stop(const Camwire_handle c_handle);
  int set_acq_type(const Camwire_handle c_handle);
  double set_shutter(const Camwire_handle c_handle);
  int set_external_trigger(const Camwire_handle c_handle);
  int set_trigger_polarity(const Camwire_handle c_handle);
  double set_framerate(const Camwire_handle c_handle);
  void set_framesize(const Camwire_handle c_handle, int *width,
               int *height);
  void set_pixel_coding(const Camwire_handle c_handle,
                 Camwire_pixel *coding);
  void set_pixel_maxval(unsigned long *maxval);
  void set_pixel_tiling(const Camwire_handle c_handle,
                 Camwire_tiling *tiling);
  void set_offset(const Camwire_handle c_handle, int *left,
               int *top);
  void set_gain(const Camwire_handle c_handle, double *gain);
  void set_brightness(const Camwire_handle c_handle, double *brightness);
  void set_white_bal(const Camwire_handle c_handle, double bal[2]);
  int set_gamma(const Camwire_handle c_handle);
  int set_response_curve(const Camwire_handle c_handle);
  int set_colour_corr(const Camwire_handle c_handle);
  void set_colour_coef(const Camwire_handle c_handle, double coef[9]);
  void set_buffers(const Camwire_handle c_handle, int *num_bufs);
  void set_image_filename(Settings_t *set);
  void set_save_delay(Settings_t *set);
  void set_save_num_images(Settings_t *set);
  int get_user_char_setting(const Camwire_handle c_handle,
                 int (*get_function)(
                     const Camwire_handle, int *),
                 char prompt_message[],
                 char true_char,
                 char false_char);
  int save_images(const Camwire_handle c_handle, Settings_t *set);
  void save_numbered_image(const Camwire_handle c_handle,
                const void *fb,
                const int image_type,
                Settings_t *set,
                FILE *logfile);
  void filename_framenumber(char *namebuffer,
                 const char *filename,
                 const int strip,
                 const long frameno);

#if 0
  size_t write_pam_image_data(const void *fb,
                   const int width,
                   const int height,
                   const int depth,
                   const unsigned long max_pixel,
                   FILE *imagefile);
#endif
  void manage_buffer_level(const Camwire_handle c_handle,
                FILE *logfile);
  char * pixelcoding2string(const Camwire_pixel coding);
  char * pixeltiling2string(const Camwire_tiling tiling);
  Camwire_pixel string2pixelcoding(const char *str);
  Camwire_tiling string2pixeltiling(const char *str);
  int component_depth(const Camwire_pixel coding);
  void settings_save_load(const Camwire_handle c_handle,
                   Settings_t *set);
  void settings_save(Settings_t *set,
              const char *settings_filename);
  void settings_load(const Camwire_handle c_handle,
              Settings_t *set,
              const char *settings_filename);
  int update_settings(FILE *settingsfile, Settings_t *_new);
  void check(const int scanexpect, const int scanresult, const char *tag,
          int *gotinput);
  char * skip_whitespace(const char *string);
  char * skip_non_whitespace(const char *string);
  void update_camera(const Camwire_handle c_handle,
              Settings_t *_new,
              const Settings_t *set);
  void show_camwire_data(const Camwire_handle c_handle);
  void wait_frametime(const Camwire_handle c_handle,
               const double multiple);
  void clear_stdin(void);
  void usage_message(char * const argv[]);
  int stricmp(const char *str1, const char *str2);
  void errorexit(const Camwire_handle c_handle, const int cam,
              const char *msg);
  void cleanup(const Camwire_handle c_handle);

public:
  //void cameraMain(char * const argv[]);

  int more_options;
  int option;
  char settings_filename[MAX_FILENAME+1] = "\0";
  char chip_id_string[CAMWIRE_ID_MAX_CHARS+1] = "\0";
  Camwire_handle *handle_array = NULL;
  Camwire_handle c_handle = NULL;
  int retry;
  int bad_bus, over_run;
  int num_cameras, current_cam;
  void *capturebuffer = NULL;
  int display_return;
  int key;
  Settings_t settings;
  int runsts;
  struct timespec timestamp, nap;
  fd_set rfds;
  struct timeval tv;

  //Initializing Mat imag from OpenCV
  //int ii=0;
  uint *bufferint;
  uchar *buffer;
  cv::Mat img;
  cv::Mat imgBGR;

  void cameraMain(int cameraNumber);
  cv::Mat cameraImage();
  int set_stop(const Camwire_handle c_handle);
  double set_framerate_jaymi(const Camwire_handle c_handle,double framerate);
  int set_acq_type_jaymi(const Camwire_handle c_handle);
  void set_buffers_jaymi(const Camwire_handle c_handle, int *num_bufs);
};

#endif // CAMMONITOR_H
