/***********************************************************************

    Copyright (c) Industrial Research Limited $Date: 2011/04/12 22:13:21 $

    This file is part of Camwire, a generic camera interface.

    Camwire is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of the
    License, or (at your option) any later version.

    Camwire is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Camwire; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    USA

    Title: Camera monitor

    Description:
    Provides basic access to digital camera functions via a simple
    terminal and display interface.  Uses the Camwire API.

    To do:
    Re-design the whole thing.  It grew organically as Camwire was
    developed and is a real mess.

***********************************************************************/


#include "cammonitor.h"
using namespace std;


/*
  ----------------------------------------------------------------------
  Initializes the camera bus, gets the user to select a camera if there
  is more than one, initializes it, and sets up the display interface.
  Then enters an endless loop which displays images and reponds to user
  commands.
*/
/* Almost every camera function call returns a success/failure flag
   which should be checked, resulting in quite messy-looking code.  To
   keep things clean and simple, almost all errors therefore result in a
   call to errorexit() which prints a message, cleans up allocated
   memory, and exits with an error status. */

/*
  ----------------------------------------------------------------------
  Returns the camera to work with, which is the camera number with
  matching chip identifier string.  Displays the camera's identifier
  data and creates it as well.
*/
  int camera::get_named_cam(const Camwire_handle *handle_array,
             const int num_cameras,
             const char *chip_id_string)
{
    int c, current_cam;
    Camwire_id camid;

    current_cam = -1;;
    for (c = 0; c < num_cameras; ++c)
    {
    if (camwire_get_identifier(handle_array[c], &camid) !=
        CAMWIRE_SUCCESS)
    {
        errorexit(NULL, c, "Could not get the identifier.");
    }
    if (strncmp(camid.chip, chip_id_string, CAMWIRE_ID_MAX_CHARS) ==
        0)
    {
        current_cam = c;
        break;
    }
    }

    if (current_cam < 0)
    {
    printf("Could not find camera with ID %s.\n", chip_id_string);
    fflush(stdout);
    errorexit(NULL, current_cam,
          "Try running without specifying an ID.");
    }

    printf("Vendor name:       %s\n", camid.vendor);
    printf("Model name:        %s\n", camid.model);
    printf("Vendor & chip ID:  %s\n", camid.chip);
    fflush(stdout);
    return(current_cam);
}

/*
  ----------------------------------------------------------------------
  Reads the camera's parameters and default values into the settings
  structure.
*/
  void camera::get_camera_settings(const Camwire_handle c_handle,
                   Settings_t *set)
{
    int settingsval;

    camwire_get_stateshadow(c_handle, &set->shadowlevel);

    camwire_get_run_stop(c_handle, &settingsval);
    if (settingsval)  set->activity = set->running;
    else              set->activity = set->stopped;

    camwire_get_single_shot(c_handle, &settingsval);
    if (settingsval)  set->acqtype = set->single;
    else              set->acqtype = set->continuous;

    camwire_get_colour_coefficients(c_handle, set->colour_coef);
    camwire_get_colour_correction(c_handle, (int *)&set->colour_corr);

    camwire_get_gamma(c_handle, &settingsval);
    if (settingsval)  set->gamma = set->gamma_on;
    else              set->gamma = set->gamma_off;

    camwire_get_white_balance(c_handle, set->white_bal);

    camwire_get_brightness(c_handle, &set->brightness);

    camwire_get_gain(c_handle, &set->gain);

    camwire_get_trigger_source(c_handle, &settingsval);
    if (settingsval)  set->trigger = set->external;
    else              set->trigger = set->internal;

    camwire_get_trigger_polarity(c_handle, &settingsval);
    if (settingsval)  set->polarity = set->rising;
    else              set->polarity = set->falling;

    camwire_get_shutter(c_handle, &set->shutter);

    camwire_get_framerate(c_handle, &set->framerate);

    camwire_get_frame_size(c_handle, &set->width, &set->height);

    camwire_get_pixel_coding(c_handle, &set->coding);

    camwire_get_pixel_tiling(c_handle, &set->tiling);

    camwire_get_frame_offset(c_handle, &set->left, &set->top);

    camwire_get_num_framebuffers(c_handle, &set->num_buffers);

    camwire_get_framenumber(c_handle, &set->framenumber);
}

/*
  ----------------------------------------------------------------------
  Sets default values to the non-camera paramenters in the settings
  structure.
*/
  void camera::default_noncamera_settings(Settings_t *set)
{
    set->maxval = 0;  /* 0 means uninitialized.*/
    change_response_curve(set, set->unchanged);
    set->save_delay = DEFAULT_SAVE_DELAY;
    set->save_num_images = DEFAULT_SAVE_NUM_IMAGES;
    strncpy(set->imagefilename, DEFAULT_IMAGE_FILENAME, MAX_FILENAME);
    set->imagefilename[MAX_FILENAME] = '\0';
}

/*
  ----------------------------------------------------------------------
  Set the response_curve member of the given settings to newvalue, while
  maintaining the allocated linbuffer for linearized images.  Assumes
  currently valid values in set->coding, set->width and set->height.
*/
  void camera::change_response_curve(Settings_t *set,  int newvalue)
{
    int depth;

    if (linbuffer == NULL && newvalue == set->linearized)
    {
    camwire_pixel_depth(set->coding, &depth);
    if (depth != 8 && depth != 24)
    {
        fprintf(stderr, "Only 8-bit pixel components can be linearized.\n"
            "Did not change the response curve.\n");
        return;
    }
    linbuffer = malloc((size_t)set->width*set->height*depth/4);
    /* Divide by 4, not 8, for a buffer twice the size.*/
    if (linbuffer == NULL)
    {
        fprintf(stderr, "Could not allocate linear image buffer.\n"
            "Did not change the response curve.\n");
        return;
    }
    }
    if (linbuffer != NULL && newvalue != set->linearized)
    {
    free(linbuffer);
    linbuffer = NULL;
    }
    if (newvalue==0){
        set->response_curve = set->unchanged;//newvalue;
    }else if(newvalue==1)
    {
        set->response_curve = set->linearized;//newvalue;
    }
}

/*
  ----------------------------------------------------------------------
  Prints the menu of available commands as well as the current
  settings.
*/
  void camera::show_menu(const Settings_t *set)
{
    char *c1, *c2, *c3, *c4;
      char leftcurr[] = "[", rightcurr[] = "]", nothing[] = "";
    unsigned long maxval;
    char namebuffer[MAX_FILENAME+1];

    printf("\nCommands:  "
       "[H]otkey         "
       "%spossible_settings%s %scurrent_setting%s\n",
       nothing, nothing, leftcurr, rightcurr);

    /* Activity:*/
    if (set->activity == set->stopped)
    {
    c1 = leftcurr;  c2 = rightcurr;
    c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;
    c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[R]un/stop camera           %sstopped%s %srunning%s\n",
       c1, c2, c3, c4);

    /* Acquisition type:*/
    if (set->acqtype == set->continuous)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[A]quisition                "
       "%scontinuous%s %ssingle-shot%s\n",
       c1, c2, c3, c4);

    /* Trigger:*/
    if (set->trigger == set->internal)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[T]rigger source            %sinternal%s %sexternal%s\n",
       c1, c2, c3, c4);

    /* Polarity:*/
    if (set->polarity == set->falling)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("trigger [P]olarity          %sfalling%s %srising%s\n",
       c1, c2, c3, c4);

    /* Integration time:*/
    printf("[I]ntegration time          %s%g s%s\n",
       leftcurr, set->shutter, rightcurr);

    /* Frame rate:*/
    printf("[F]rame rate                %s%g fps%s\n",
       leftcurr, set->framerate, rightcurr);

    /* Image size:*/
    printf("frame si[Z]e         "
       "       %sw %d pixels%s %sh %d pixels%s\n",
       leftcurr, set->width, rightcurr,
       leftcurr, set->height, rightcurr);

    /* Pixel Coding:*/
    printf("pixel [C]oding       "
       "       %s%s%s\n",
       leftcurr, pixelcoding2string(set->coding), rightcurr);

    /* Pixel Maxval:*/
    maxval = set->maxval;
    if (set->maxval == 0)
    {
    maxval = (1 << component_depth(set->coding)) - 1;
    }
    printf("[9] pixel maxval     "
       "       %s%lu%s\n",
       leftcurr, maxval, rightcurr);

    /* Pixel Tiling:*/
    printf("pixel tilin[G]       "
       "       %s%s%s\n",
       leftcurr, pixeltiling2string(set->tiling), rightcurr);

    /* Image offset:*/
    printf("frame [O]ffset       "
       "       %sleft %d pixels%s %stop %d pixels%s\n",
       leftcurr, set->left, rightcurr,
       leftcurr, set->top, rightcurr);

    /* Gain:*/
    printf("[U] gain [0, 1]             %s%g%s\n",
       leftcurr, set->gain, rightcurr);

    /* Brightness:*/
    printf("[V] brightness [-1, +1]     %s%g%s\n",
       leftcurr, set->brightness, rightcurr);

    /* White balance:*/
    printf("[W]hite balance [0, 1]      %sblue %g%s %sred %g%s\n",
       leftcurr, set->white_bal[0], rightcurr,
       leftcurr, set->white_bal[1], rightcurr);

    /* Gamma:*/
    if (set->gamma == set->gamma_off)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[Y] gamma                   "
       "%soff%s %son%s\n",
       c1, c2, c3, c4);

    /* Response curve:*/
    if (set->response_curve == set->unchanged)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[X] response curve          "
       "%sunchanged%s %slinearized%s\n",
       c1, c2, c3, c4);

    /* Colour correction:*/
    if (set->colour_corr == set->colour_corr_off)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[J] colour correction       %soff%s %son%s\n", c1, c2, c3, c4);

    /* Colour coefficients:*/
    printf("[K] colour coefficients     [%g %g %g]\n",
       set->colour_coef[0], set->colour_coef[1], set->colour_coef[2]);
    printf("                            [%g %g %g]\n",
       set->colour_coef[3], set->colour_coef[4], set->colour_coef[5]);
    printf("                            [%g %g %g]\n",
       set->colour_coef[6], set->colour_coef[7], set->colour_coef[8]);

    /* Frame Buffers:*/
    printf("frame [B]uffers             %s%d%s\n",
       leftcurr, set->num_buffers, rightcurr);

    /* Shadow camera:*/
    if (!set->shadowlevel)
    {
    c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
    c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("sha[D]ow camera             %soff%s %son%s\n",
       c1, c2, c3, c4);

    /* Image filename:*/
    filename_framenumber(namebuffer, set->imagefilename, 0,
             set->framenumber + 1);
    printf("file[N]ame for save         %s%s%s\n",
       leftcurr, namebuffer, rightcurr);

    /* Delay before saving:*/
    printf("de[L]ay before saving       %s%g s%s\n",
       leftcurr, set->save_delay, rightcurr);

    /* Number of images to save:*/
    printf("[M]ultiple images to save   %s%d%s\n",
       leftcurr, set->save_num_images, rightcurr);

    /* Save image:*/
    if (set->save_num_images == 1)
    {
    printf("[S]ave image to file\n");
    }
    else if (set->save_num_images > 1)
    {
    printf("[S]ave images to files\n");
    }
    /* else do not print save item.*/

    /* Settings save or load:*/
    printf("s[E]ttings save or load\n");

    /* Show Camwire data:*/
    printf("s[H]ow camwire data\n");

    /* Quit:*/
    printf("[Q]uit camera monitor\n");
    fflush(stdout);
}

/*
  ----------------------------------------------------------------------
  Prints the command line prompt.
*/
  void camera::show_prompt(void)
{
    printf(COMMAND_PROMPT);
    fflush(stdout);
}

/*
  ----------------------------------------------------------------------
  Does something in response to the user input.
*/
  int camera::user_input(const Camwire_handle c_handle, const int key,
              Settings_t *set)
{
    int continue_flag;
    int oldwidth, oldheight;
    Camwire_pixel oldcoding;
    unsigned long oldmaxval;

    continue_flag = 1;
    switch(tolower(key))
    {
    case 'r': 	/* Run-stop toggle:*/
        if(set_run_stop(c_handle))
            set->activity = set->running;
        else
            set->activity = set->stopped;
        break;
    case 'a': 	/* Acquisition type toggle:*/
        if(set_acq_type(c_handle))
            set->acqtype = set->single;
        else
            set->acqtype = set->continuous;
        break;
    case 't': 	/* Trigger source toggle:*/
        if(set_external_trigger(c_handle))
            set->trigger = set->external;
        else
            set->trigger = set->internal;
        break;
    case 'p': 	/* Trigger polarity toggle:*/
        if(set_trigger_polarity(c_handle))
            set->polarity = set->rising;
        else
            set->polarity = set->falling;
        break;
    case 'i': 	/* Integration time:*/
        set->shutter = set_shutter(c_handle);
        break;
    case 'f': 	/* Frame rate:*/
        set->framerate = set_framerate(c_handle);
        break;
    case 'z': 	/* Frame siZe:*/
        oldwidth = set->width;
        oldheight = set->height;
        set_framesize(c_handle, &set->width, &set->height);
        if (display_initialized)
        {
        if (display_resize(d_handle, set->width, set->height) !=
            DISPLAY_SUCCESS)
        { 	/* Can't display it for some reason.*/
            set->width = oldwidth;
            set->height = oldheight;
            camwire_set_frame_size(c_handle, set->width,
                       set->height);
        }
        }
        break;
    case 'c': 	/* Pixel Coding:*/
        oldcoding = set->coding;
        set_pixel_coding(c_handle, &set->coding);
        if (display_initialized)
        {
        if (display_coding(d_handle, set->coding) !=
            DISPLAY_SUCCESS)
        { 	/* Can't display it.*/
            fprintf(stderr, "Cannot display this pixel coding.\n");
            set->coding = oldcoding;
            camwire_set_pixel_coding(c_handle, set->coding);
        }
        }
        break;
    case '9': 	/* Pixel maxval:*/
        oldmaxval = set->maxval;
        set_pixel_maxval(&set->maxval);
        if (display_initialized)
        {
        if (display_maxval(d_handle, set->maxval) != DISPLAY_SUCCESS)
        {
            fprintf(stderr, "Could not set pixel maxval %lu in display.\n",
                set->maxval);
            set->maxval = oldmaxval;
        }
        }
        break;
    case 'g': 	/* Pixel Tiling:*/
        set_pixel_tiling(c_handle, &set->tiling);
        break;
    case 'o': 	/* Frame Offset:*/
        set_offset(c_handle, &set->left, &set->top);
        break;
    case 'u': 	/* Gain:*/
        set_gain(c_handle, &set->gain);
        break;
    case 'v': 	/* Brightness:*/
        set_brightness(c_handle, &set->brightness);
        break;
    case 'w': 	/* White balance:*/
        set_white_bal(c_handle, set->white_bal);
        break;
    case 'y': 	/* Gamma:*/
        if (set_gamma(c_handle))
            set->gamma = set->gamma_on;
        else
            set->gamma = set->gamma_off;
        break;
    case 'x': 	/* Response curve:*/
        change_response_curve(set, set_response_curve(c_handle));
        break;
    case 'j': 	/* Colour correction:*/
        if(set_colour_corr(c_handle))
            set->colour_corr = set->colour_corr_on;
        else
            set->colour_corr = set->colour_corr_off;
        break;
    case 'k': 	/* Colour coefficients:*/
        set_colour_coef(c_handle, set->colour_coef);
        break;
    case 'b': 	/* Frame Buffers:*/
        set_buffers(c_handle, &set->num_buffers);
        break;
    case 'd': 	/* ShaDow camera:*/
        set->shadowlevel = set_shadow(c_handle);
        break;
    case 'n': 	/* Change image filename:*/
        set_image_filename(set);
        break;
    case 'l': 	/* Delay before saving:*/
        set_save_delay(set);
        break;
    case 'm': 	/* Number of images to save:*/
        set_save_num_images(set);
        break;
    case 's': 	/* Save images to file:*/
        continue_flag = save_images(c_handle, set);
        break;
    case 'e':
        settings_save_load(c_handle, set);
        break;
    case 'h':
        show_camwire_data(c_handle);
        break;
    case 'q': 	/* Quit:*/
        continue_flag = 0;
        break;
    default:
        break;
    } /*switch*/

    return(continue_flag);
}

/*
  ----------------------------------------------------------------------
  Returns the camera shadow flag obtained from the user, after setting
  the camera accordingly.
*/
  int camera::set_shadow(const Camwire_handle c_handle)
{
    int shadowlevel;

    shadowlevel =
    get_user_char_setting(c_handle, camwire_get_stateshadow, "oFf/oN", 'n', 'f');

    if (shadowlevel)
    {
    if (camwire_set_stateshadow(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set camera state shadow flag.\n");
    }
    }
    else
    {
    if (camwire_set_stateshadow(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not unset camera state shadow flag.\n");
    }
    }

    if (camwire_get_stateshadow(c_handle, &shadowlevel) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not get camera shadow status.\n");
    return(shadowlevel);
}

/*
  ----------------------------------------------------------------------
  Returns the activity status obtained from the user, after setting the
  camera accordingly.
*/
  int camera::set_run_stop(const Camwire_handle c_handle)
{
    int run;
    int num_buffers;

//    run = get_user_char_setting(c_handle, camwire_get_run_stop,
//                "Run/Stop camera", 'r', 's');

    run =1;

    if (run)
    {
    if (camwire_set_run_stop(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr, "Could not set camera to running.\n");
    }
    }
    else
    {
    if (camwire_set_run_stop(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr, "Could not set camera to stopped.\n");
    }
    //wait_frametime(c_handle, SAFE_FRAME_PERIOD);
    camwire_get_num_framebuffers(c_handle, &num_buffers);
    camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
    }


    /* Note that we do not bother to check the camera run/stop status
       again here because it auto-clears when single-shot is set:*/

    if (run)  return(1);
    else      return(0);
}
/*
----------------------------------------------------------------------
Returns true if the camera was set to stop, based on set_run_stop function.
*/

  int camera::set_stop(const Camwire_handle c_handle)
{
    int run;
    int num_buffers;

//    run = get_user_char_setting(c_handle, camwire_get_run_stop,
//                "Run/Stop camera", 'r', 's');

    run =0;

    if (run)
    {
        if (camwire_set_run_stop(c_handle, 1) != CAMWIRE_SUCCESS)
        {
            fprintf(stderr, "Could not set camera to running.\n");
        }
    }
    else
    {
        if (camwire_set_run_stop(c_handle, 0) != CAMWIRE_SUCCESS)
        {
            fprintf(stderr, "Could not set camera to stopped.\n");
        }
        //wait_frametime(c_handle, SAFE_FRAME_PERIOD);
        camwire_get_num_framebuffers(c_handle, &num_buffers);
        camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
        camwire_destroy(c_handle);
    }


    /* Note that we do not bother to check the camera run/stop status
       again here because it auto-clears when single-shot is set:*/

    if (!run)  return(1);
    else      return(0);
}


/*
  ----------------------------------------------------------------------
  Returns the acquisition type obtained from the user, after setting
  the camera accordingly.
*/
  int camera::set_acq_type(const Camwire_handle c_handle)
{
    int singleshot;
    int num_buffers;

    singleshot =
    get_user_char_setting(c_handle, camwire_get_single_shot,
                  "Continuous/Single-shot acquisition",
                  's', 'c');

    if (singleshot)
    {
    camwire_get_num_framebuffers(c_handle, &num_buffers);
    camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
    if (camwire_set_single_shot(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set camera to single-shot acquisition.\n");
    }
    }
    else
    {
    if (camwire_set_single_shot(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set camera to continuous acquisition.\n");
    }
    }

    if (camwire_get_single_shot(c_handle, &singleshot) !=
    CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get single-shot status.\n");
    }
    if (singleshot)  return(1);//single
    else             return(0);//continuos
}

  /*
    ----------------------------------------------------------------------
    Returns the acquisition type obtained from the user, after setting
    the camera accordingly.
  */
    int camera::set_acq_type_jaymi(const Camwire_handle c_handle)
  {
      int singleshot;
      int num_buffers;

      singleshot =1;

      if (singleshot)
      {
          camwire_get_num_framebuffers(c_handle, &num_buffers);
          camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
          if (camwire_set_single_shot(c_handle, 1) != CAMWIRE_SUCCESS)
          {
              fprintf(stderr,
              "Could not set camera to single-shot acquisition.\n");
          }
      }
      else
      {
          if (camwire_set_single_shot(c_handle, 0) != CAMWIRE_SUCCESS)
          {
              fprintf(stderr,
              "Could not set camera to continuous acquisition.\n");
          }
      }

      if (camwire_get_single_shot(c_handle, &singleshot) !=
      CAMWIRE_SUCCESS)
      {
      fprintf(stderr, "Could not get single-shot status.\n");
      }
      if (singleshot)  return(1);//single
      else             return(0);//continuos
  }


/*
  ----------------------------------------------------------------------
  Returns a shutter speed (exposure time) obtained from the user, after
  setting the camera accordingly.
*/
  double camera::set_shutter(const Camwire_handle c_handle)
{
    int gotinput;
    double shutter;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Integration time (seconds): ");
    fflush(stdout);
    if (scanf("%lf", &shutter) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid floating point number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_shutter(c_handle, shutter) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera shutter.\n");
    if (camwire_get_shutter(c_handle, &shutter) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera shutter.\n");
    return(shutter);
}

/*
  ----------------------------------------------------------------------
  Returns the trigger source obtained from the user, after setting the
  camera accordingly.
*/
  int camera::set_external_trigger(const Camwire_handle c_handle)
{
    int exttrig;

    exttrig = get_user_char_setting(c_handle,
                    camwire_get_trigger_source,
                    "Internal/External trigger",
                    'e', 'i');

    if (exttrig)
    {
    if (camwire_set_trigger_source(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
            "Could not set trigger source to external.\n");
    }
    }
    else
    {
    if (camwire_set_trigger_source(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set trigger source to internal.\n");
    }
    }

    if (camwire_get_trigger_source(c_handle, &exttrig) !=
    CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get trigger source.\n");
    }
    if (exttrig)  return(1);//external
    else	  return(0);//internal
}

/*
  ----------------------------------------------------------------------
  Returns the trigger polarity obtained from the user, after setting
  the camera accordingly.
*/
  int camera::set_trigger_polarity(const Camwire_handle c_handle)
{
    int risingpol;

    risingpol =
    get_user_char_setting(c_handle, camwire_get_trigger_polarity,
                  "Rising/Falling trigger polarity",
                  'r', 'f');

    if (risingpol)
    {
    if (camwire_set_trigger_polarity(c_handle, 1) !=
        CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
            "Could not set trigger polarity to rising edge.\n");
    }
    }
    else
    {
    if (camwire_set_trigger_polarity(c_handle, 0) !=
        CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set trigger polarity to falling edge.\n");
    }
    }

    if (camwire_get_trigger_polarity(c_handle, &risingpol) !=
    CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get trigger polarity.\n");
    }
    if (risingpol)  return(1);//rising
    else	    return(0);//falling
}

/*
  ----------------------------------------------------------------------
  Returns a frame rate (frames per second) obtained from the user,
  after setting the camera accordingly.
*/
  double camera::set_framerate(const Camwire_handle c_handle)
{
    int gotinput;
    double framerate;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Frame rate (frames per second): ");
    fflush(stdout);
    if (scanf("%lf", &framerate) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid floating point number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_framerate(c_handle, framerate) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera framerate.\n");
    if (camwire_get_framerate(c_handle, &framerate) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera framerate.\n");
    return(framerate);
}

  double camera::set_framerate_jaymi(const Camwire_handle c_handle,double framerate)
{

    if (camwire_set_framerate(c_handle, framerate) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera framerate.\n");
    if (camwire_get_framerate(c_handle, &framerate) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera framerate.\n");
    return(framerate);
}



/*
  ----------------------------------------------------------------------
  Returns the frame size obtained from the user, after setting the
  camera accordingly.
*/
  void camera::set_framesize(const Camwire_handle c_handle, int *width,
               int *height)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Frame size (width height): ");
    fflush(stdout);
    if (scanf("%d %d", width, height) == 2)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_frame_size(c_handle, *width, *height) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera frame size.\n");
    if (camwire_get_frame_size(c_handle, width, height) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr,
        "Could not confirm the camera frame size.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the pixel coding obtained from the user, after setting
  the camera accordingly.
*/
  void camera::set_pixel_coding(const Camwire_handle c_handle,
                 Camwire_pixel *coding)
{
    int gotinput;
    char pix_coding[MAX_PIXEL_CODING_STRING+1];
    char formatstring[20];

    /* Limit the input width: */
    sprintf(formatstring, "%%%ds", MAX_PIXEL_CODING_STRING);

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Pixel coding.  Type one of:\n"
           "MONO8\n"
           "YUV411\n"
           "YUV422\n"
           "YUV444\n"
           "RGB8\n"
           "MONO16\n"
           "RGB16\n"
           "MONO16S\n"
           "RGB16S\n"
           "RAW8\n"
           "RAW16\n"
           ": ");
    fflush(stdout);
    if (scanf(formatstring, pix_coding) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid coding string.\n");
        clear_stdin();
    }
    }

    *coding = string2pixelcoding(pix_coding);
    if (camwire_set_pixel_coding(c_handle, *coding) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera's pixel coding.\n");
    if (camwire_get_pixel_coding(c_handle, coding) != CAMWIRE_SUCCESS)
    fprintf(stderr,
        "Could not confirm the camera's pixel coding.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the maxval obtained from the user.
*/
  void camera::set_pixel_maxval(unsigned long *maxval)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("maxval: ");
    fflush(stdout);
    if (scanf("%lu", maxval) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
}

/*
  ----------------------------------------------------------------------
  Returns the pixel tiling obtained from the user, after setting
  the camera accordingly.
*/
  void camera::set_pixel_tiling(const Camwire_handle c_handle,
                 Camwire_tiling *tiling)
{
    int gotinput;
    char pix_tiling[MAX_PIXEL_TILING_STRING+1];
    char formatstring[20];

    /* Limit the input width: */
    sprintf(formatstring, "%%%ds", MAX_PIXEL_TILING_STRING);

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Pixel tiling.  Type one of:\n"
           "RGGB\n"
           "GBRG\n"
           "GRBG\n"
           "BGGR\n"
           "UYVY\n"
           "YUYV\n"
           ": ");
    fflush(stdout);
    if (scanf(formatstring, pix_tiling) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid tiling string.\n");
        clear_stdin();
    }
    }

    *tiling = string2pixeltiling(pix_tiling);
/*     if (camwire_set_pixel_tiling(c_handle, *tiling) != CAMWIRE_SUCCESS) */
    fprintf(stderr, "Could not set the camera's pixel tiling.\n");
    if (camwire_get_pixel_tiling(c_handle, tiling) != CAMWIRE_SUCCESS)
    fprintf(stderr,
        "Could not confirm the camera's pixel tiling.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the frame offset obtained from the user, after setting
  the camera accordingly.
*/
  void camera::set_offset(const Camwire_handle c_handle, int *left,
               int *top)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Frame offset (left top): ");
    fflush(stdout);
    if (scanf("%d %d", left, top) == 2)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_frame_offset(c_handle, *left, *top) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera frame offset.\n");
    if (camwire_get_frame_offset(c_handle, left, top) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera frame offset.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the gain obtained from the user, after setting the camera
  accordingly.
*/
  void camera::set_gain(const Camwire_handle c_handle, double *gain)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("gain: ");
    fflush(stdout);
    if (scanf("%lf", gain) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_gain(c_handle, *gain) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera gain.\n");
    if (camwire_get_gain(c_handle, gain) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera gain.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the brightness obtained from the user, after setting the
  camera accordingly.
*/
  void camera::set_brightness(const Camwire_handle c_handle, double *brightness)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("brightness: ");
    fflush(stdout);
    if (scanf("%lf", brightness) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_brightness(c_handle, *brightness) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera brightness.\n");
    if (camwire_get_brightness(c_handle, brightness) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera brightness.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the white balance obtained from the user, after setting
  the camera accordingly.
*/
  void camera::set_white_bal(const Camwire_handle c_handle, double bal[2])
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("White balance (blue red): ");
    fflush(stdout);
    if (scanf("%lf %lf", &bal[0], &bal[1]) == 2)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_white_balance(c_handle, bal) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the camera white balance.\n");
    if (camwire_get_white_balance(c_handle, bal) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the camera white balance.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the gamma obtained from the user, after setting the camera
  accordingly.
*/
  int camera::set_gamma(const Camwire_handle c_handle)
{
    int gamma_val;

    gamma_val = get_user_char_setting(c_handle, camwire_get_gamma, "oFf/oN", 'n', 'f');

    if (gamma_val)
    {
    if (camwire_set_gamma(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not set camera gamma.\n");
    }
    }
    else
    {
    if (camwire_set_gamma(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr,
        "Could not unset camera gamma.\n");
    }
    }

    if (camwire_get_gamma(c_handle, &gamma_val) != CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get gamma status.\n");
    }
    if (gamma_val)  return(1);//gamma_on
    else            return(0);//gamma_off
}

/*
  ----------------------------------------------------------------------
  Returns the response curve obtained from the user.  Nothing on the
  camera changes.
*/
  int camera::set_response_curve(const Camwire_handle c_handle)
{
    char true_char, false_char;
    int curve_val, gamma_val, gotinput;
    char formatstring[20];
    char userinput[MAX_KEY_INPUT+1];
    char key;

    true_char  = 'L';
    false_char = 'U';
    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT);  /* Limit input width.*/

    gotinput = 0;
    curve_val = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Unchanged/Linearized: ");
    fflush(stdout);
    scanf(formatstring, userinput); 	/* Safe input.*/
    key = toupper(userinput[0]);
    if (key == true_char)
    {
        curve_val = 1;
        gotinput = 1;
    }
    else if (key == false_char)
    {
        curve_val = 0;
        gotinput = 1;
    }
    else
    {
        printf("Type either %c or %c.\n", true_char, false_char);
        fflush(stdout);
        clear_stdin();
    }
    }

    if (camwire_get_gamma(c_handle, &gamma_val) != CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get gamma status.\n");
    }
    if (curve_val && !gamma_val)
    {
    fprintf(stderr, "Relinearizing an already linear response: gamma "
        "should be set.\n");
    }

    if (curve_val)  return(1);//linearized
    else            return(0);//unchanged
}

/*
  ----------------------------------------------------------------------
  Returns the colour correction obtained from the user, after setting
  the camera accordingly.
*/
  int camera::set_colour_corr(const Camwire_handle c_handle)
{
    int corr_val;

    corr_val = get_user_char_setting(c_handle, camwire_get_colour_correction, "oFf/oN",
                     'n', 'f');

    if (corr_val)
    {
    if (camwire_set_colour_correction(c_handle, 1) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr, "Could not set camera colour correction.\n");
    }
    }
    else
    {
    if (camwire_set_colour_correction(c_handle, 0) != CAMWIRE_SUCCESS)
    {
        fprintf(stderr, "Could not unset camera colour correction.\n");
    }
    }

    if (camwire_get_colour_correction(c_handle, &corr_val) != CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get colour correction status.\n");
    }
    if (corr_val)  return(1);//colour_corr_on
    else           return(0);//colour_corr_off
}

/*
  ----------------------------------------------------------------------
  Returns the colour correction coefficients obtained from the user,
  after setting the camera accordingly.
*/
  void camera::set_colour_coef(const Camwire_handle c_handle, double coef[])
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("9 colour correction coefficients: ");
    fflush(stdout);
    if (scanf("%lf %lf %lf %lf %lf %lf %lf %lf %lf", &coef[0], &coef[1], &coef[2],
          &coef[3], &coef[4], &coef[5], &coef[6], &coef[7], &coef[8]) == 9)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number or not enough input.\n");
        clear_stdin();
    }
    }
    if (camwire_set_colour_coefficients(c_handle, coef) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the colour correction coefficients.\n");
    if (camwire_get_colour_coefficients(c_handle, coef) != CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not confirm the colour correction coefficients.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the number of frame buffers obtained from the user, after
  setting the camera accordingly.
*/
  void camera::set_buffers(const Camwire_handle c_handle, int *num_bufs)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Number of frame buffers: ");
    fflush(stdout);
    if (scanf("%d", num_bufs) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
    if (camwire_set_num_framebuffers(c_handle, *num_bufs) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr, "Could not set the number of frame buffers.\n");
    if (camwire_get_num_framebuffers(c_handle, num_bufs) !=
    CAMWIRE_SUCCESS)
    fprintf(stderr,
        "Could not confirm the number of frame buffers.\n");
}

  /*
    ----------------------------------------------------------------------
    Returns the number of frame buffers obtained from the user, after
    setting the camera accordingly.
  */
    void camera::set_buffers_jaymi(const Camwire_handle c_handle, int *num_bufs)
  {

      if (camwire_set_num_framebuffers(c_handle, *num_bufs) !=CAMWIRE_SUCCESS)
      fprintf(stderr, "Could not set the number of frame buffers.\n");
//      if (camwire_get_num_framebuffers(c_handle, *num_bufs) !=
//      CAMWIRE_SUCCESS)
//      fprintf(stderr,
//          "Could not confirm the number of frame buffers.\n");
  }

/*
  ----------------------------------------------------------------------
  Changes the name of the root image filename.
*/
  void camera::set_image_filename(Settings_t *set)
{
    char formatstring[20];

    sprintf(formatstring, "%%%ds", MAX_FILENAME); 	/* Limit the
                             * input field
                             * width.*/
    printf("\nImage root filename (currently \"%s\"): ",
       set->imagefilename);
    fflush(stdout);
    scanf(formatstring, set->imagefilename);
}

/*
  ----------------------------------------------------------------------
  Sets the delay before image saving starts.
*/
  void camera::set_save_delay(Settings_t *set)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Delay in seconds before saving (was %g): ",
           set->save_delay);
    fflush(stdout);
    if (scanf("%lf", &set->save_delay) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
}

/*
  ----------------------------------------------------------------------
  Changes the number of frames saved at a time.
*/
  void camera::set_save_num_images(Settings_t *set)
{
    int gotinput;

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Number of frames to save at a time (was %d): ",
           set->save_num_images);
    fflush(stdout);
    if (scanf("%d", &set->save_num_images) == 1)
    {
        gotinput = 1;
    }
    else
    {
        fprintf(stderr, "Invalid number.\n");
        clear_stdin();
    }
    }
}

/*
  ----------------------------------------------------------------------
  Returns a setting by prompting the user for a single character until
  one of the wanted characters is input.  The user's terminating newline
  is left in the stdin buffer.
*/
  int camera::get_user_char_setting(const Camwire_handle c_handle,
                 int (*get_function)(const Camwire_handle, int *),
                 char prompt_message[],
                 char true_char,
                 char false_char)
{
    int setting, gotinput;
    char formatstring[20];
    char userinput[MAX_KEY_INPUT+1];
    char key;

    if (get_function(c_handle, &setting) != CAMWIRE_SUCCESS)
    {
    fprintf(stderr, "Could not get current camera setting.\n");
    }
    true_char = toupper(true_char);
    false_char = toupper(false_char);
    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
                             * input field
                             * width.*/

    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("%s (currently %c): ", prompt_message,
           setting ? true_char : false_char);
    fflush(stdout);
    scanf(formatstring, userinput); 	/* Safe input.*/
    key = toupper(userinput[0]);
    if (key == true_char)
    {
        setting = 1;
        gotinput = 1;
    }
    else if (key == false_char)
    {
        setting = 0;
        gotinput = 1;
    }
    else
    {
        printf("Type either %c or %c.\n", true_char, false_char);
        fflush(stdout);
        clear_stdin();
    }
    }
    return(setting);
}

/*
  ----------------------------------------------------------------------
  Captures, displays and saves the next frame(s) in image file(s), with
  optional delay.  Writes frame number, time stamp and buffer level in a
  log file for each frame.  Prints an error message if a file cannot be
  opened.  Tries to ensure that the frame buffers does not overflow by
  flushing frames when the buffer gets too full.
*/
  int camera::save_images(const Camwire_handle c_handle, Settings_t *set)
{
    int continue_flag;
    int depth;
    int now_running, now_single;
    double framerate, frameperiod, waitperiod;
    struct timespec nap;
    FILE *logfile;
    char logfile_name[MAX_FILENAME+1];
    char errormessage[MAX_MESSAGE+1];
    char *nameptr;
    int num_buffers, count;
    int image_type;
    void *fb;
    int display_return;
    int display_saved_frames;

    continue_flag = 1;

    /* Skip unsupported pixel codings: */
    if (set->coding == CAMWIRE_PIXEL_YUV411)
    {
    fprintf(stderr, "Don't know how to save YUV411.\n");
    return(continue_flag);
    }
    camwire_pixel_depth(set->coding, &depth);

    /* Find out the current status and update our record: */
    camwire_get_run_stop(c_handle, &now_running);
    if (now_running)  set->activity = set->running;
    else              set->activity = set->stopped;
    camwire_get_single_shot(c_handle, &now_single);
    if (now_single)  set->acqtype = set->single;
    else             set->acqtype = set->continuous;
    camwire_get_framerate(c_handle, &framerate);
    camwire_get_num_framebuffers(c_handle, &num_buffers);

    /* If camera is stopped, or if delay is required, stop, wait,
       flush, and restart: */
    frameperiod = 1.0/framerate;
    if (!now_running || set->save_delay > frameperiod)
    {
    if (now_running)
    {
        now_running = 0;
        camwire_set_run_stop(c_handle, now_running);
    }
    if (set->save_delay > frameperiod)
    {
        waitperiod = set->save_delay - frameperiod;
        nap.tv_sec = waitperiod;
        nap.tv_nsec = (waitperiod - nap.tv_sec)*1e9;
        nanosleep(&nap, NULL);
    }
    wait_frametime(c_handle, SAFE_FRAME_PERIOD);
    camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
    if (set->activity == set->running)
    {
        now_running = 1;
        camwire_set_run_stop(c_handle, now_running);
    }
    /* Entry status is now restored. */
    }

    /* Return now if no images are needed: */
    if (set->save_num_images < 1)  return(continue_flag);

    /* If the camera was already running, grab images on the fly.  If
       the camera is stopped, use single-shot for single images and
       continuous running for multiple images: */
    if (set->activity == set->stopped)
    {
    if (set->save_num_images == 1 && set->acqtype == set->continuous)
    {
        now_single = 1;
        camwire_set_single_shot(c_handle, now_single);
    }
    else if (set->save_num_images > 1 && set->acqtype == set->single)
    {
        now_single = 0;
        camwire_set_single_shot(c_handle, now_single);
    }
    now_running = 1;
    camwire_set_run_stop(c_handle, now_running);
    }
    /* Camera is now running. */

    /* Decide on type of image to save: */
    switch (depth)
    {
    case 8:
    case 16:
        image_type = 5; 	/* PGM.*/
        break;
    case 24:
    case 48:
        image_type = 6; 	/* PPM.*/
        break;
    case 12:
    default:
        image_type = 0; 	/* YUV411?*/
        break;
    }

    /* Set up image saving: */
    if (set->save_num_images == 1)
    {
    printf("Saving a %s image.\n",
           (image_type == 6 ? "PPM" : "PGM"));
    }
    else
    {
    printf("Saving %d %s images at %g fps.\n",
           set->save_num_images,
           (image_type == 6 ? "PPM" : "PGM"),
           framerate);
    }
    fflush(stdout);

    display_saved_frames = 0;
    if (set->activity == set->running && display_initialized)
    {
    fb = malloc((size_t) set->width*set->height*depth/8);
    if (fb != NULL)
    {
        display_saved_frames = 1;
    }
    else
    {
        fprintf(stderr, "Could not allocate display buffer.\n"
            "Display disabled while saving.\n");
    }
    }

    filename_framenumber(logfile_name, set->imagefilename, 1, 0);
    nameptr = &logfile_name[strlen(logfile_name)-4];
    if (strncmp(nameptr, ".ppm", 4) == 0 ||
    strncmp(nameptr, ".pgm", 4) == 0)
    {
    *nameptr = '\0'; 	/* Truncate ".ppm" or ".pgm".*/
    }
    strcat(logfile_name, ".log");
    logfile = fopen(logfile_name, "w");
    if (logfile == NULL)
    {
    snprintf(errormessage, MAX_MESSAGE, "Can't create '%s'",
         logfile_name);
    perror(errormessage);
    }

    /* Grab, display and save images: */
    for (count = 1; count <= set->save_num_images; ++count)
    {
    if (display_saved_frames)
    {
        if (camwire_copy_next_frame(c_handle, fb, NULL) != CAMWIRE_SUCCESS)
        {
        fprintf(stderr, "Could not copy the next frame to save.");
        break;
        }

        display_return = display_frame(d_handle, fb);
        if (display_return == DISPLAY_FAILURE)
        {
        fprintf(stderr, "Display error while saving.\n");
        break;
        }
        else if (display_return == DISPLAY_QUIT_REQ)
        {
        continue_flag = 0;
        break;
        }
        save_numbered_image(c_handle, fb, image_type, set, logfile);
    }
    else
    {
        if (camwire_point_next_frame(c_handle, &fb, NULL) !=
        CAMWIRE_SUCCESS)
        {
        fprintf(stderr, "Could not point to the next frame to save.");
        break;
        }

        save_numbered_image(c_handle, fb, image_type, set, logfile);
        camwire_unpoint_frame(c_handle);
    }
    manage_buffer_level(c_handle, logfile);
    }

    fclose(logfile);
    if (display_saved_frames)  free(fb);

    /* Single-shot acquisition automatically stops the camera: */
    if (now_single)
    {
    camwire_get_run_stop(c_handle, &now_running);
    if (now_running != 0)
    { 	/* Double check.*/
        errorexit(c_handle, -1,
              "Camera should have stopped after single-shot.");
    }
    }

    /* Restore entry settings: */
    if (set->activity == set->stopped)
    {
    if (set->save_num_images == 1 && set->acqtype == set->continuous)
    {
        now_single = 0;
        camwire_set_single_shot(c_handle, now_single);
    }
    else if (set->save_num_images > 1)
    {
        now_running = 0;
        camwire_set_run_stop(c_handle, now_running);
        wait_frametime(c_handle, SAFE_FRAME_PERIOD);
        camwire_flush_framebuffers(c_handle, num_buffers, NULL,
                       NULL);
        if (set->acqtype == set->single)
        {
        now_single = 1;
        camwire_set_single_shot(c_handle, now_single);
        }
    }
    }
    return(continue_flag);
}

/*
  ----------------------------------------------------------------------
  The name says it all really.
*/
  void camera::save_numbered_image(const Camwire_handle c_handle,
                const void *fb,
                const int image_type,
                Settings_t *set,
                FILE *logfile)
{
    long framenumber;
    char numbered_name[MAX_FILENAME+1];
    char *nameptr;
    int depth;
    unsigned long max_pixel;
    FILE *imagefile;
    char errormessage[MAX_MESSAGE+1];
    char fileheader[MAX_IMAGE_HEADER+1];
    struct timespec timestamp;
    char timestamp_str[MAX_TIMESTAMP+1];
    ssize_t byteswritten;
    size_t numel;
    int buffer_lag;

    /* Generate a filename: */
    camwire_get_framenumber(c_handle, &framenumber);
    filename_framenumber(numbered_name, set->imagefilename, 0,
             framenumber);
    nameptr = &numbered_name[strlen(numbered_name)-4];
    if (strncmp(nameptr, ".ppm", 4) != 0 &&
    strncmp(nameptr, ".pgm", 4) != 0)
    {
    if (image_type == 5)       strcat(numbered_name, ".pgm");
    else if (image_type == 6)  strcat(numbered_name, ".ppm");
    }

    /* Decide on max pixel size to save: */
    max_pixel = set->maxval;  /* Use user setting unless uninitialized.*/
    if (set->maxval == 0)  max_pixel = (1 << component_depth(set->coding)) - 1;

    /* Write the frame to an image file:*/
    imagefile = fopen(numbered_name, "w");
    if (imagefile == NULL)
    {
    snprintf(errormessage, MAX_MESSAGE, "Can't create '%s'",
         numbered_name);
    perror(errormessage);
    return;
    }
    camwire_get_timestamp(c_handle, &timestamp);
    snprintf(timestamp_str, MAX_TIMESTAMP, "%.6lf",
         timestamp.tv_sec + 1.0e-9*timestamp.tv_nsec);
    snprintf(fileheader, MAX_IMAGE_HEADER,
         "P%d\n%c frame %ld, \ttime %s\n%d %d\n%lu\n",
         image_type, SETTINGS_COMMENT, framenumber, timestamp_str,
         set->width, set->height, max_pixel);

    byteswritten = fprintf(imagefile, "%s", fileheader);
    numel = (size_t)set->width*set->height;
    if (byteswritten > 0)
    {
    camwire_pixel_depth(set->coding, &depth);
#if 0
    byteswritten = write_pam_image_data(fb, set->width, set->height,
                        depth, max_pixel, imagefile);
#else
    byteswritten = fwrite(fb, 1, numel*depth/8, imagefile);
#endif
    }
    if (use_fsync)  fsync(fileno(imagefile));
    fclose(imagefile);

    if (byteswritten > 0)
    {
    camwire_get_framebuffer_lag(c_handle, &buffer_lag);
    fprintf(logfile, "%11ld \t%s \t%4d\n", framenumber,
        timestamp_str, buffer_lag);
    fflush(logfile);
    }
    else
    {
    snprintf(errormessage, MAX_MESSAGE, "Could not write to '%s'",
         numbered_name);
    perror(errormessage);
    }
}

/*
  ----------------------------------------------------------------------
  If strip is 0, expands the given filename by inserting an
  11-decimal-digit frame number at the first `%d' (or not if it does not
  include a `%d').  If strip is not 0, strips the given filename of any
  `%d' token.  Returns the expanded or stripped filename in new_name.
*/
  void camera::filename_framenumber(char *new_name,
                 const char *filename,
                 const int strip,
                 const long frameno)
{
    size_t namelen, tokenindex;
    const char *token;

    namelen = strlen(filename);
    token = strstr(filename, FRAMENUMBER_TOKEN);
    if (token == NULL)  tokenindex = namelen;
    else                tokenindex = token - filename;
    strncpy(new_name, filename, tokenindex);
    new_name[tokenindex] = '\0';
    if (token != NULL)
    {
    if (!strip)  sprintf(&new_name[tokenindex], "%011ld", frameno);
    tokenindex += strlen(FRAMENUMBER_TOKEN);
    }
    strcat(new_name, &filename[tokenindex]);
}

#if 0
/*
  ----------------------------------------------------------------------
  Uses Netpbm calls to write the image properly to a netpbm file.
*/

  size_t camera::write_pam_image_data(const void *fb,
                   const int width,
                   const int height,
                   const int depth,
                   const unsigned long max_pixel,
                   FILE *imagefile)
{
    struct pam info;
    int bpp;
    uint8_t *pixels_1;   /* For 1-byte elements.*/
    uint16_t *pixels_2;  /* For 2-byte elements.*/
    tuple *tuplerow;
    int row, col, rowpos, plane;
    size_t byteswritten;

    /* Fill in PAM data: */
    info.size = info.len = sizeof(struct pam);
    info.file = imagefile;
    info.plainformat = 0;
    info.height = height;
    info.width = width;
    info.maxval = max_pixel;
    bpp = depth/8;
    if (bpp < 3)
    {
    info.format = RPGM_FORMAT;
    info.depth = 1;
    strncpy(info.tuple_type, "GRAYSCALE", 255);
    info.tuple_type[255] = '\0';
    }
    else
    {
    info.format = RPPM_FORMAT;
    info.depth = 3;
    strncpy(info.tuple_type, "RGB", 255);
    info.tuple_type[255] = '\0';
    }
    if (bpp == 1 || bpp == 3)  info.bytes_per_sample = 1;
    else                       info.bytes_per_sample = 2;

    /* Set up pointers and allocate memory: */
    pixels_1 = (uint8_t *) fb;
    pixels_2 = (uint16_t *) fb;
    tuplerow = pnm_allocpamrow(&info);

    /* Write the image data row by row: */
    byteswritten = 0;
    for (row = 0; row < info.height; ++row)
    {
    rowpos = row*info.width;
        for (col = 0; col < info.width; ++col)
        for (plane = 0; plane < (int) info.depth; ++plane)
        {
        if (info.bytes_per_sample == 1)
            tuplerow[col][plane] = pixels_1[rowpos+col];
        else
            tuplerow[col][plane] = pixels_2[rowpos+col];
        /* Note that (for 16-bit images) pixels_2 points to
           big-endian values as received from an IIDC-complying
           camera.  NetPBM also uses network byte order (most
           significant byte first) so we can copy the
           framebuffer samples directly into tuplerow and not
           have endianness problems. */
        }
        pnm_writepamrow(&info, tuplerow);
    byteswritten += info.bytes_per_sample*info.depth*info.width;
    }

    /* Tidy up: */
    pnm_freepamrow(tuplerow);
    return byteswritten;
}
#endif

/*
  ----------------------------------------------------------------------
  Checks the number of pending filled buffers and flushes some of them
  if there are too many.  This helps to ensure that frame numbers are
  accurately updated when frames are dropped.  Otherwise buffer
  overflows may result in the user not knowing if frames have been lost.
*/
  void camera::manage_buffer_level(const Camwire_handle c_handle,
                FILE *logfile)
{
    int total_frames, current_level, num_to_flush;

    camwire_get_num_framebuffers(c_handle, &total_frames);
    if (total_frames < 3)  return;
    camwire_get_framebuffer_lag(c_handle, &current_level);
    ++current_level; 	/* Buffer lag does not count current frame.*/

    /* It seems that the DMA buffers sometimes do not fill up
       completely, hence the extra -1 in the if expression below: */
    if (current_level >= total_frames - 1)
    { 	/* Hit the ceiling.*/
    num_to_flush = total_frames;
    if (camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL) !=
        CAMWIRE_SUCCESS)
        fprintf(stderr,
            "Could not flush all buffers in manage_buffer_level().\n");
    if (logfile != NULL)
    {
        fprintf(logfile, "Frame buffers overflowed.  "
            "Frame numbers may no longer be in synch.\n");
        fflush(logfile);
    }
    }
    else if (current_level + 0.5 >= BUF_HIGH_MARK*total_frames)
    {
    num_to_flush = current_level - BUF_LOW_MARK*total_frames;
    if (camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL) !=
        CAMWIRE_SUCCESS)
        fprintf(stderr,
            "Could not flush %d buffers in manage_buffer_level().\n",
            num_to_flush);
    }
    /* else don't flush. */
}

/*
  ----------------------------------------------------------------------
  Returns a pointer to a pixel colour coding string corresponding to
  the Camwire pixel coding.
*/
  char * camera::pixelcoding2string(const Camwire_pixel coding)
{
    char *str;

    switch (coding)
    {
    case CAMWIRE_PIXEL_MONO8:
        str = "MONO8";
        break;
    case CAMWIRE_PIXEL_YUV411:
        str = "YUV411";
        break;
    case CAMWIRE_PIXEL_YUV422:
        str = "YUV422";
        break;
    case CAMWIRE_PIXEL_YUV444:
        str = "YUV444";
        break;
    case CAMWIRE_PIXEL_RGB8:
        str = "RGB8";
        break;
    case CAMWIRE_PIXEL_MONO16:
        str = "MONO16";
        break;
    case CAMWIRE_PIXEL_RGB16:
        str = "RGB16";
        break;
    case CAMWIRE_PIXEL_MONO16S:
        str = "MONO16S";
        break;
    case CAMWIRE_PIXEL_RGB16S:
        str = "RGB16S";
        break;
    case CAMWIRE_PIXEL_RAW8:
        str = "RAW8";
        break;
    case CAMWIRE_PIXEL_RAW16:
        str = "RAW16";
        break;
    case CAMWIRE_PIXEL_INVALID:
    default:
        str = "INVALID";
        break;
    }
    return(str);
}

/*
  ----------------------------------------------------------------------
  Returns a pointer to a pixel colour tiling string corresponding to
  the Camwire pixel tiling.
*/
  char * camera::pixeltiling2string(const Camwire_tiling tiling)
{
    char *str;

    switch (tiling)
    {
    case CAMWIRE_TILING_RGGB:
        str = "RGGB";
        break;
    case CAMWIRE_TILING_GBRG:
        str = "GBRG";
        break;
    case CAMWIRE_TILING_GRBG:
        str = "GRBG";
        break;
    case CAMWIRE_TILING_BGGR:
        str = "BGGR";
        break;
    case CAMWIRE_TILING_UYVY:
        str = "UYVY";
        break;
    case CAMWIRE_TILING_YUYV:
        str = "YUYV";
        break;
    case CAMWIRE_TILING_INVALID:
    default:
        str = "INVALID";
        break;
    }
    return(str);
}

/*
  ----------------------------------------------------------------------
  Returns the Camwire pixel coding corresponding to a pixel colour
  coding string.  Case-insensitive.
*/
  Camwire_pixel camera::string2pixelcoding(const char *str)
{
    if      (!stricmp(str, "MONO8"))  	return(CAMWIRE_PIXEL_MONO8);
    else if (!stricmp(str, "YUV411"))  	return(CAMWIRE_PIXEL_YUV411);
    else if (!stricmp(str, "YUV422"))  	return(CAMWIRE_PIXEL_YUV422);
    else if (!stricmp(str, "YUV444"))  	return(CAMWIRE_PIXEL_YUV444);
    else if (!stricmp(str, "RGB8"))  	return(CAMWIRE_PIXEL_RGB8);
    else if (!stricmp(str, "MONO16"))  	return(CAMWIRE_PIXEL_MONO16);
    else if (!stricmp(str, "RGB16"))  	return(CAMWIRE_PIXEL_RGB16);
    else if (!stricmp(str, "MONO16S")) 	return(CAMWIRE_PIXEL_MONO16S);
    else if (!stricmp(str, "RGB16S"))  	return(CAMWIRE_PIXEL_RGB16S);
    else if (!stricmp(str, "RAW8"))  	return(CAMWIRE_PIXEL_RAW8);
    else if (!stricmp(str, "RAW16"))  	return(CAMWIRE_PIXEL_RAW16);
    else  				return(CAMWIRE_PIXEL_INVALID);
}

/*
  ----------------------------------------------------------------------
  Returns the Camwire pixel tiling corresponding to a pixel colour
  tiling string.  Case-insensitive.
*/
  Camwire_tiling camera::string2pixeltiling(const char *str)
{
    if      (!stricmp(str, "RGGB"))  	return(CAMWIRE_TILING_RGGB);
    else if (!stricmp(str, "GBRG"))  	return(CAMWIRE_TILING_GBRG);
    else if (!stricmp(str, "GRBG"))  	return(CAMWIRE_TILING_GRBG);
    else if (!stricmp(str, "BGGR"))  	return(CAMWIRE_TILING_BGGR);
    else if (!stricmp(str, "UYVY"))  	return(CAMWIRE_TILING_UYVY);
    else if (!stricmp(str, "YUYV"))  	return(CAMWIRE_TILING_YUYV);
    else  				return(CAMWIRE_TILING_INVALID);
}

/*
  -----------------------------------------------------------------------------
  Returns the number of bits per component in the given pixel coding.
*/
  int camera::component_depth(const Camwire_pixel coding)
{
    switch (coding)
    {
    case CAMWIRE_PIXEL_MONO8:
    case CAMWIRE_PIXEL_YUV411:
    case CAMWIRE_PIXEL_YUV422:
    case CAMWIRE_PIXEL_YUV444:
    case CAMWIRE_PIXEL_RGB8:
    case CAMWIRE_PIXEL_RAW8:
        return 8;
    case CAMWIRE_PIXEL_MONO16:
    case CAMWIRE_PIXEL_RGB16:
    case CAMWIRE_PIXEL_MONO16S:
    case CAMWIRE_PIXEL_RGB16S:
    case CAMWIRE_PIXEL_RAW16:
        return 16;
    default:
        return 0;
    }
} /* component_depth() */

/*
  ----------------------------------------------------------------------
  Saves or loads the current monitor settings to or from file.
*/
  void camera::settings_save_load(const Camwire_handle c_handle,
                   Settings_t *set)
{
    int save, gotinput;
    char userinput[MAX_KEY_INPUT+1];
    char key;
    char formatstring[20];
    char settings_filename[MAX_FILENAME+1];

    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
                             * input field
                             * width.*/
    save = 0;
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Save/Load settings: ");
    fflush(stdout);
    scanf(formatstring, userinput); 	/* Safe input.*/
    key = toupper(userinput[0]);
    if (key == 'S')
    {
        save = 1;
        gotinput = 1;
    }
    else if (key == 'L')
    {
        save = 0;
        gotinput = 1;
    }
    else
    {
        printf("Type either S or L.\n");
        fflush(stdout);
        clear_stdin();
    }
    }

    sprintf(formatstring, "%%%ds", MAX_FILENAME); 	/* Limit the
                             * input field
                             * width.*/
    printf("\nSettings filename: ");
    fflush(stdout);
    scanf(formatstring, settings_filename);

    if (save)  settings_save(set, settings_filename);
    else       settings_load(c_handle, set, settings_filename);
}

/*
  ----------------------------------------------------------------------
  Saves the current monitor settings to file.
*/
  void camera::settings_save(Settings_t *set, const char *settings_filename)
{
    FILE *settingsfile;

    settingsfile = fopen(settings_filename, "w");
    if (settingsfile != NULL)
    {
    fprintf(settingsfile, "%c Cammonitor run-time settings\n\n",
        SETTINGS_COMMENT);
    fprintf(settingsfile, settings_format,
        set->shadowlevel,
        set->activity,
        set->acqtype,
        set->trigger,
        set->polarity,
        set->shutter,
        set->framerate,
        set->width,
        set->height,
        (int) set->coding,
        set->maxval,
        (int) set->tiling,
        set->left,
        set->top,
        set->gain,
        set->brightness,
        set->white_bal[0],
        set->white_bal[1],
        set->gamma,
        set->response_curve,
        set->colour_corr,
        set->colour_coef[0],
        set->colour_coef[1],
        set->colour_coef[2],
        set->colour_coef[3],
        set->colour_coef[4],
        set->colour_coef[5],
        set->colour_coef[6],
        set->colour_coef[7],
        set->colour_coef[8],
        set->num_buffers,
        set->framenumber,
        set->save_num_images,
        set->save_delay,
        set->imagefilename);

    fclose(settingsfile);
    printf("Settings saved to \"%s\".\n", settings_filename);
    fflush(stdout);
    }
    else
    {
    fprintf(stderr, "Could not open the settings file \"%s\" "
        "for writing.\n", settings_filename);
    }
}

/*
  ----------------------------------------------------------------------
  Loads the current monitor settings from file and updates the
  camera.
*/
  void camera::settings_load(const Camwire_handle c_handle,
              Settings_t *set,
              const char *settings_filename)
{
    FILE *settingsfile;
    Settings_t new_settings;
    int gotinput;
    int oldwidth, oldheight;
    Camwire_pixel oldcoding;

    settingsfile = fopen(settings_filename, "r");
    if (settingsfile != NULL)
    {
    /* Read any new settings with old settings as default: */
    new_settings = *set;
    gotinput = update_settings(settingsfile, &new_settings);
    fclose(settingsfile);
    if (gotinput)
    {
        printf("Settings loaded from \"%s\".\n", settings_filename);

        /* Remember old settings that affect display: */
        oldwidth = set->width;
        oldheight = set->height;
        oldcoding = set->coding;

        /* Write any changes to camera and read back actual: */
        update_camera(c_handle, &new_settings, set);
        get_camera_settings(c_handle, set);

        /* Resize the display if necessary: */
        if (display_initialized)
        {
        if (set->width != oldwidth || set->height != oldheight)
        {
            if (display_resize(d_handle, set->width, set->height) !=
            DISPLAY_SUCCESS)
            { 	/* Can't display it for some reason.*/
            fprintf(stderr, "Cannot display frame size setting.\n");
            set->width = oldwidth;
            set->height = oldheight;
            camwire_set_frame_size(c_handle, set->width,
                           set->height);
            }
        }
        if (set->coding != oldcoding)
        {
            if (display_coding(d_handle, set->coding) != DISPLAY_SUCCESS)
            { 	/* Can't display it.*/
            fprintf(stderr, "Cannot display pixel coding setting.\n");
            set->coding = oldcoding;
            camwire_set_pixel_coding(c_handle, set->coding);
            }
        }
        }

        /* Copy internal monitor settings. response_curve must be
           set after display initialization to ensure valid image
           size and coding: */
        set->maxval = new_settings.maxval;
        change_response_curve(set, new_settings.response_curve);
        set->save_num_images = new_settings.save_num_images;
        set->save_delay = new_settings.save_delay;
        strncpy(set->imagefilename, new_settings.imagefilename,
            MAX_FILENAME);
    }
    else
    {
        fprintf(stderr, "No settings read from %s.\n",
            settings_filename);
    }
    }
    else
    {
    fprintf(stderr, "Could not open the settings file \"%s\" "
        "for reading.\n", settings_filename);
    }
}

/*
  ----------------------------------------------------------------------
  Reads the opened settings file, changing only the fields present in
  the file.  The new settings are assumed already to be initialized with
  old settings.
*/
  int camera::update_settings(FILE *settingsfile, Settings_t *_new)
{
    int gotinput, empty, comment;
    char linebuffer[MAX_SETTINGS_LINE+1];
    char *lineptr;
    char *new_tag_str, *value_str;
    const char *ref_tag_str, *format_str;

    gotinput = 0;
    while(!feof(settingsfile))
    {
    /* Read lines until not empty and not a comment: */
    empty = 1;
    comment = 1;
    while (empty || comment)
    {
        lineptr =
        fgets(linebuffer, MAX_SETTINGS_LINE, settingsfile);
        if (lineptr == NULL)  break;
        empty = (strlen(skip_whitespace(linebuffer)) == 0);
        comment = (linebuffer[0] == SETTINGS_COMMENT);
    }
    if (lineptr == NULL)  break;

    /* Separate input tag string and value string: */
    new_tag_str = skip_whitespace(linebuffer);
    lineptr = skip_non_whitespace(new_tag_str);
    *lineptr = '\0';
    ++lineptr;
    value_str = skip_whitespace(lineptr);
    if (strlen(value_str) >= 2)  /* Including a newline char.*/
    {
        /* Find corresponding tag and conversion in the format: */
        ref_tag_str = strstr(settings_format, new_tag_str);
        if (ref_tag_str != NULL)
        {
        lineptr = skip_non_whitespace(ref_tag_str);
        format_str = skip_whitespace(lineptr);

        if (strcmp(new_tag_str, "shadowlevel") == 0)
        {
            check(1, sscanf(value_str, format_str, &(_new)->shadowlevel), new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "activity") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->activity),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "acqtype") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->acqtype),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "trigger") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->trigger),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "polarity") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->polarity),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "shutter") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->shutter),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "framerate") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->framerate),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "width") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->width),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "height") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->height),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "coding") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->coding),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "maxval") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->maxval),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "tiling") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->tiling),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "left") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->left),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "top") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->top),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "gain") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->gain),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "brightness") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->brightness),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "white_bal") == 0)
        {
            check(2, sscanf(value_str, format_str,  & _new->white_bal[0],
                     & _new->white_bal[1]),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "gamma") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->gamma),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "response_curve") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->response_curve),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "colour_corr") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->colour_corr),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "colour_coef") == 0)
        {
            check(9, sscanf(value_str, format_str,  & _new->colour_coef[0],
                     & _new->colour_coef[1],  & _new->colour_coef[2],
                     & _new->colour_coef[3],  & _new->colour_coef[4],
                     & _new->colour_coef[5],  & _new->colour_coef[6],
                     & _new->colour_coef[7],  & _new->colour_coef[8]),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "num_buffers") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->num_buffers),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "framenumber") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->framenumber),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "save_num_images") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->save_num_images),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "save_delay") == 0)
        {
            check(1, sscanf(value_str, format_str,  & _new->save_delay),
              new_tag_str, &gotinput);
        }
        else if (strcmp(new_tag_str, "imagefilename") == 0)
        {
            check(1, sscanf(value_str, format_str,  _new->imagefilename),
              new_tag_str, &gotinput);
        }
                else
        {
            fprintf(stderr, "Cammonitor internal error:  Don't "
                "know how to read tag `%s' in the settings "
                "format string.\n", new_tag_str);
        }
        }
        else
        {
        fprintf(stderr,
            "Unrecognized tag `%s' in settings file.\n",
            new_tag_str);
        }
    }
    else
    {
        fprintf(stderr, "No value for tag `%s' in settings file.\n",
            new_tag_str);
    }
    }  /* while */

    return(gotinput);
}

/*
  ----------------------------------------------------------------------
  Sets gotinput if scanresult is 1, else prints error message.
*/
  void camera::check(const int scanexpect, const int scanresult, const char *tag,
          int *gotinput)
{
    if (scanresult == scanexpect)
    {
    *gotinput = 1;
    }
    else
    {
    fprintf(stderr,
        "Error reading value of `%s' in settings file.\n", tag);
    }
}

/*
  ----------------------------------------------------------------------
  Return a pointer to the 1st non-whitespace character in null-delimited
  string.  If the string contains only whitespace, returns a pointer to
  an empty string (pointer to the null character at the end of the
  string).
*/
  char * camera::skip_whitespace(const char *string)
{
    return((char *) &string[strspn(string, WHITESPACE)]);
}

/*
  ----------------------------------------------------------------------
  Return a pointer to the 1st whitespace character in null-delimited
  string.  If the string contains no whitespace, returns a pointer to an
  empty string (pointer to the null character at the end of the string).
*/
  char * camera::skip_non_whitespace(const char *string)
{
    return((char *) &string[strcspn(string, WHITESPACE)]);
}

/*
  ----------------------------------------------------------------------
  Update the camera with new settings.  Note that the order of register
  writes may be significant for some cameras after power-up or
  reset/initilize, hence the elaborate cascade.
*/
  void camera::update_camera(const Camwire_handle c_handle,
              Settings_t *_new,
              const Settings_t *set)
{
    int dotherest;

    dotherest = 0;
    if ( _new->shadowlevel != (set->shadowlevel))
    {
    camwire_set_stateshadow(c_handle,  _new->shadowlevel);
    }
    if (dotherest ||
     _new->num_buffers != set->num_buffers)
    {
    camwire_set_num_framebuffers(c_handle,  _new->num_buffers);
    dotherest = 1;
    }
    if (dotherest ||
     _new->left != set->left ||  _new->top != set->top)
    {
    camwire_set_frame_offset(c_handle,  _new->left,  _new->top);
    dotherest = 1;
    }
    if (dotherest ||
     _new->width != set->width ||  _new->height != set->height)
    {
    camwire_set_frame_size(c_handle,  _new->width,  _new->height);
    dotherest = 1;
    }
    /* Do offsets a second time in case they were not allowed by frame
       size the first time: */
    if (dotherest ||
     _new->left != set->left ||  _new->top != set->top)
    {
    camwire_set_frame_offset(c_handle,  _new->left,  _new->top);
    dotherest = 1;
    }
    if (dotherest ||
     _new->coding != set->coding)
    {
    camwire_set_pixel_coding(c_handle,  _new->coding);
    dotherest = 1;
    }
    if (dotherest ||
    fabs( _new->framerate - set->framerate) > 1e-6)
    {
    camwire_set_framerate(c_handle,  _new->framerate);
    dotherest = 1;
    }
    if (dotherest ||
     _new->trigger != set->trigger)
    {
    camwire_set_trigger_source(c_handle,
                    _new->trigger);
    dotherest = 1;
    }
    if (dotherest ||
     _new->polarity != set->polarity)
    {
    camwire_set_trigger_polarity(c_handle,
                      _new->polarity);
    dotherest = 1;
    }
    if (dotherest ||
     _new->shutter != set->shutter)
    {
    camwire_set_shutter(c_handle,  _new->shutter);
    dotherest = 1;
    }
    if (memcmp( _new->colour_coef, set->colour_coef, 9*sizeof(double)) != 0)
    {
    camwire_set_colour_coefficients(c_handle,  _new->colour_coef);
    }
    if ( _new->colour_corr != set->colour_corr)
    {
    camwire_set_colour_correction(c_handle,
                      ( _new->colour_corr==_new->colour_corr_on ? 1 : 0));
    }
    if ( _new->gamma != set->gamma)
    {
    camwire_set_gamma(c_handle, ( _new->gamma==_new->gamma_on ? 1 : 0));
    }
    if ( _new->white_bal[0] != set->white_bal[0] ||
     _new->white_bal[1] != set->white_bal[1])
    {
    camwire_set_white_balance(c_handle,  _new->white_bal);
    }
    if ( _new->brightness != set->brightness)
    {
    camwire_set_brightness(c_handle,  _new->brightness);
    }
    if ( _new->gain != set->gain)
    {
    camwire_set_gain(c_handle,  _new->gain);
    }
    if (dotherest ||
     _new->acqtype != set->acqtype)
    {
    camwire_set_single_shot(c_handle,  _new->acqtype);
    dotherest = 1;
    }
    if (dotherest ||
     _new->activity != set->activity)
    {
    camwire_set_run_stop(c_handle,  _new->activity);
    dotherest = 1;
    }
}

/*
  ----------------------------------------------------------------------
  Displays Camwire hardware configuration or current settings.
*/
  void camera::show_camwire_data(const Camwire_handle c_handle)
{
    int getconfig, gotinput;
    char userinput[MAX_KEY_INPUT+1];
    char key;
    char formatstring[20];
    Camwire_conf config;
    Camwire_state settings;

    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
                             * input field
                             * width.*/
    getconfig = 0;
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
    printf("Display Config/Settings: ");
    fflush(stdout);
    scanf(formatstring, userinput); 	/* Safe input.*/
    key = toupper(userinput[0]);
    if (key == 'C')
    {
        getconfig = 1;
        gotinput = 1;
    }
    else if (key == 'S')
    {
        getconfig = 0;
        gotinput = 1;
    }
    else
    {
        printf("Type either C or S.\n");
        fflush(stdout);
        clear_stdin();
    }
    }

    if (getconfig)
    {
    camwire_get_config(c_handle, &config);
    camwire_write_config_to_file(stdout, &config);
    }
    else
    {
    camwire_get_state(c_handle, &settings);
    camwire_write_state_to_file(stdout, &settings);
    }
}

/*
  ----------------------------------------------------------------------
  Sleeps for multiple times one frame period, where multiple is a double.
*/
  void camera::wait_frametime(const Camwire_handle c_handle,
               const double multiple)
{
    double framerate, frameperiod;
    struct timespec nap;

    camwire_get_framerate(c_handle, &framerate);
    frameperiod = multiple/framerate;
    nap.tv_sec = frameperiod;
    nap.tv_nsec = (frameperiod - nap.tv_sec)*1e9;
    nanosleep(&nap, NULL);
}

/*
  ----------------------------------------------------------------------
  Reads and discards a line from stdin.
*/
  void camera::clear_stdin(void)
{
    char throwaway[MAX_KEY_INPUT+1];

    fgets(throwaway, MAX_KEY_INPUT, stdin);
}

/*
  ----------------------------------------------------------------------
  Prints a usage message to stderr.
*/
  void camera::usage_message(char * const argv[])
{
    fprintf(stderr,
        "\n"
        "Usage:  %s [-s] [-d display_type] [-e settings_filename] "
            "[-c chip_id_string]\n"
        "where   -s disables image file sync to disk\n"
        "        -d selects type of screen display "
        "(NONE | SDL | XV), default is SDL\n"
        "        -e autoloads the given settings file\n"
        "        -c specifies a camera by its unique chip ID\n",
        argv[0]);
}

/*
  ----------------------------------------------------------------------
  Case-insensitive string comparison.
*/

  int camera::stricmp(const char *str1, const char *str2)
{
    int i;

    i = 0;
    while (str1[i] != '\0' && str2[i] != '\0')
    {
    if (toupper(str1[i]) != toupper(str2[i]))  break;
    ++i;
    }
    return((int) toupper(str1[i]) - (int) toupper(str2[i]));
}

/*
  ----------------------------------------------------------------------
  Prints the given error message (including the camera number if it is
  non-negative) to stderr, cleans up, and exits with a return status of
  1.
*/
  void camera::errorexit(const Camwire_handle c_handle, const int cam,
              const char *msg)
{
    fflush(stdout);  /* libdc1394 writes error messages to stdout.*/
    if (cam < 0)  fprintf(stderr, "\n%s\n", msg);
    else          fprintf(stderr, "\nCamera %d: %s\n", cam + 1, msg);
    cleanup(c_handle);
    exit(EXIT_FAILURE);
}

/*
  ----------------------------------------------------------------------
  Resets the console input to default settings, stops the camera, closes
  the display window, frees the frame buffer, and destroys the camera
  object and its bus.  Should be safe to call with null arguments.
*/
  void camera::cleanup(const Camwire_handle c_handle)
{
    if (c_handle != NULL)
    {
    camwire_set_run_stop(c_handle, 0);
    //wait_frametime(c_handle, SAFE_FRAME_PERIOD);
    }
    free(linbuffer);
    linbuffer = NULL;
    if (display_initialized)  display_destroy(d_handle);
    display_initialized = 0;
    camwire_destroy(c_handle);
    camwire_bus_destroy();
    putchar('\n');
    fflush(stdout);
}

/*
  ----------------------------------------------------------------------
  Returns the camera to work with, which is 0 if there is only one
  camera or the camera number chosen interactively by the user if there
  is more than one camera.  Displays the camera's identifier data and
  creates it as well.
*/
  int camera::get_user_cam(const Camwire_handle *handle_array,
            const int num_cameras)
{
    int c, current_cam;
    int got_one;
    Camwire_id camid;

    for (c = 0; c < num_cameras; ++c)
    {
    if (camwire_get_identifier(handle_array[c], &camid) !=
        CAMWIRE_SUCCESS)
    {
        errorexit(NULL, c, "Could not get the identifier.");
    }
    if (num_cameras > 1)  printf("\nCamera %d:\n", c + 1);
    printf("Vendor name:       %s\n", camid.vendor);
    printf("Model name:        %s\n", camid.model);
    printf("Vendor & chip ID:  %s\n", camid.chip);
    fflush(stdout);
    }

    if (num_cameras == 1)
    {
    current_cam = 0;
    }
    else
    {
    got_one = 0;
    while (!got_one)
    {
        current_cam = -1;
        while (current_cam < 0 || current_cam > num_cameras)
        {
        printf("\nSelect a camera number "
               "(from 1 to %d, 0 quits): ", num_cameras);
        fflush(stdout);
        scanf("%d", &current_cam);
        clear_stdin();
        }
        --current_cam;
        if (current_cam < 0)
        {
        camwire_bus_destroy();
        putchar('\n');
        fflush(stdout);
        exit(EXIT_SUCCESS); 	/* Normal exit.*/
        }
        else
        {
        got_one = 1;
        }
    }
    }
    return(current_cam);
}
void camera::cameraMain(int cameraNumber)
//void camera::cameraMain()
{

//    int more_options;
//    int option;
//    char settings_filename[MAX_FILENAME+1] = "\0";
//    char chip_id_string[CAMWIRE_ID_MAX_CHARS+1] = "\0";
//    Camwire_handle *handle_array = NULL;
//    Camwire_handle c_handle = NULL;
//    int retry;
//    int bad_bus, over_run;
//    int num_cameras, current_cam;
//    void *capturebuffer = NULL;
//    int display_return;
//    int key;
//    //Settings_t settings;
//    int runsts;
//    struct timespec timestamp, nap;
//    fd_set rfds;
//    struct timeval tv;

    /* Check command line options: */
    opterr = 0; 	/* Suppress error messages in getopt().*/
    use_display = DEFAULT_DISPLAY_TYPE;
    more_options = 0;

    /* Initialize the camera bus:*/
    printf("\nCamera monitor\n\n");

    //fflush(stdout);
    //camwire_bus_reset();


    handle_array = camwire_bus_create(&num_cameras);
    if (num_cameras < 0 || handle_array == NULL)
    { 	/* Could be the known kernel 1394 cycle master bug. */
    printf("Error initializing the bus: trying reset");
    retry = 1;
    bad_bus = 1;
    while (retry <= 5 && bad_bus)
    {
        printf(" %d", retry);
        fflush(stdout);
        camwire_bus_reset();
        sleep(1);
        handle_array = camwire_bus_create(&num_cameras);
        if (num_cameras >= 0 && handle_array != NULL)  bad_bus = 0;
        ++retry;
    }
    if (bad_bus)
    {
        errorexit(NULL, -1,
              "Could not initialize the camera bus.  "
              "There may not be a camera connected.");
    }
    else
    {
        printf("\n\n");
        fflush(stdout);
    }
    }
    if (num_cameras == 0)
    {
    errorexit(NULL, -1, "Could not find a camera.");
    }
    if (num_cameras == 1)  printf("Found one camera.\n");
    else                   printf("Found %d cameras.\n\n", num_cameras);
    fflush(stdout);

    /* Identify the camera to use: */
    if (chip_id_string[0] != '\0')
    {
    /* Find the camera named on the command line: */
    current_cam =
        get_named_cam(handle_array, num_cameras, chip_id_string);
    }
    else
    {
    /* Ask user which camera to use: 0->camera 1, 1->camera 2*/
    current_cam = cameraNumber;//get_user_cam(handle_array, num_cameras);
    }

    /* Initialize the camera and our local copy of its settings: */
    c_handle = handle_array[current_cam];
    if (camwire_create(c_handle) != CAMWIRE_SUCCESS)
    {
    errorexit(NULL, current_cam, "Could not initialize camera.");
    }

    get_camera_settings(c_handle, &settings);
    default_noncamera_settings(&settings);

    /* Auto-load settings if a command line filename was given: */
    if (settings_filename[0] != '\0')
    {
    settings_load(c_handle, &settings, settings_filename);
    }

    /* Set up display interface:*/
    if (use_display != DISPLAY_NONE)
    {
    d_handle = display_create(use_display, settings.width, settings.height,
                  settings.coding, settings.maxval);
    if (d_handle == NULL)
    {
        errorexit(c_handle, -1, "Could not set up display.");
    }
    display_initialized = 1;
    }

    /* Main loop, which gets and displays frames, and responds to user
       input:*/
    //show_menu(&settings);
    //show_prompt();




}
cv::Mat camera::cameraImage()
{
    //YUV422 is 2 channels
    img = cv::Mat::zeros(settings.height, settings.width, CV_8UC2);
    imgBGR= cv::Mat::zeros(settings.height, settings.width, CV_8UC3);

//    for (;;)
//    {

        //cout<<"Frame-for"<<endl;

        /* Get and display the next frame:*/
    if (settings.activity == settings.running)//running
    {
        /* Avoid getting blocked if not running.*/
        if (camwire_point_next_frame(c_handle, &capturebuffer,NULL) != CAMWIRE_SUCCESS)
        errorexit(c_handle, current_cam,"Could not point to the next frame.");

        /* Display:*/

        buffer=(uchar *)capturebuffer;
//        cout<<bufferint[ii]<<endl;
//        cout<< "width :" << settings.width <<endl;
//        cout<< "height :" << settings.height <<endl;
//        cout<< "sizeofBuffer :" << sizeof(buffer)<<endl;

       // cv::imdecode(buffer,CV_LOAD_IMAGE_COLOR);

        /*/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1*/
        int y=0;

        for (int j = 0; j < settings.height; j++) {
            for (int i = 0; i < settings.width; i++) {
                //YUV422 is 2 channels
                img.at<cv::Vec2b>(j,i)[0]= buffer[y];
                img.at<cv::Vec2b>(j,i)[1]= buffer[y+1];
                y=y+2;
            }
        }

        /*/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
        //converting from YUV422 to BGR-OpenCV
        cv::cvtColor(img, imgBGR, CV_YUV2BGR_Y422,3);

        //need this delay in order to give time to CPU to refresh the image.
        //cv::waitKey(250/settings.framerate);

//***********************************************************************
//        if (display_initialized)
//        {
//            display_return = display_frame(d_handle, capturebuffer);
//            if (display_return == DISPLAY_FAILURE)
//            {
//                fprintf(stderr, "Display error.\n");
//                cleanup(c_handle);
//                //return(EXIT_FAILURE);
//            }
//            else if (display_return == DISPLAY_QUIT_REQ)
//            {
//                cleanup(c_handle);
//                //return(EXIT_SUCCESS);
//            }
//        }
//***********************************************************************
        camwire_unpoint_frame(c_handle);
        manage_buffer_level(c_handle, NULL);

        retry = 0;
        over_run = 1;
        while (over_run && retry < 10)
        {
        if (camwire_get_run_stop(c_handle, &runsts) !=
            CAMWIRE_SUCCESS)
        {
            fprintf(stderr, "Could not get activity status.\n");
        }
        if (runsts != 0)  settings.activity = settings.running;
        else              settings.activity = settings.stopped;
        over_run =
            (settings.acqtype == settings.single && settings.activity == settings.running);
        if (over_run)
        {  /* Wait for camera to stop after single-shot.*/
            nap.tv_sec = 0;
            nap.tv_nsec = 1000000;  /* 1 ms.*/
            nanosleep(&nap, NULL);
        }
        ++retry;
        }

        if (over_run)
        fprintf(stderr, "Single-shot should have stopped the camera!\n");

        if (settings.acqtype == settings.single)
        {
        printf("\nCaptured a single shot.\n");
        fflush(stdout);
        if(camwire_get_timestamp(c_handle, &timestamp) != CAMWIRE_SUCCESS)
            fprintf(stderr, "Could not get a timestamp.\n");
        printf("Timestamp: %f\n", timestamp.tv_sec + 1.0e-9*timestamp.tv_nsec);
        fflush(stdout);

        if (camwire_get_framenumber(c_handle, &settings.framenumber)
            != CAMWIRE_SUCCESS)
            fprintf(stderr, "Could not get a frame number.\n");
//        show_menu(&settings);
//        show_prompt();
        }
      }
//***********************************************************************
    /* Simple user interaction:*/
    if (settings.activity == settings.running &&
        settings.acqtype == settings.continuous)
    {
        /* Check stdin (fd 0) to see if it has input: */
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        tv.tv_sec = 0; 	/* Poll, don't wait.*/
        tv.tv_usec = 0;

        if (select(1, &rfds, NULL, NULL, &tv))
        {
        key = getchar();
        }
        else
        {
        key = EOF;
        }

    }
    else
    {

        key = 'R';//getchar(); 	/* Wait here for input.*/
    }

    if (key != EOF)
    {
        if (key != '\n')
        {
        if (!user_input(c_handle, key, &settings))
        { 	/* User request to quit.*/
            clear_stdin();
            cleanup(c_handle);
            //return(EXIT_SUCCESS);
        }
        //clear_stdin(); 	/* Swallow trailers and NL.*/
        if (camwire_get_framenumber(c_handle,
                        &settings.framenumber) !=
            CAMWIRE_SUCCESS)
        {
            fprintf(stderr, "Could not get a frame number.\n");
        }
        //show_menu(&settings);
        }
        //show_prompt();
    }
//***********************************************************************
//  }/*for*/        

   // camwire_destroy(c_handle);

    return imgBGR;
}

//#endif // CAMMONITOR_H
