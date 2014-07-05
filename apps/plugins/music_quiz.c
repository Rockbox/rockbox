/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Richard Burke
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Includes */

#include "plugin.h"
#include "lib/helper.h"
#include "lib/highscore.h"

/* Macros */

#define MAX_QUESTION_OPTIONS 10
#define MIN_QUESTION_OPTIONS 2
#define MIN_QUESTION_TIME 5
#define MAX_QUESTION_TIME 20
/* The number of questions generated at a time */
#define QUESTION_BLOCK_SIZE 20
#define MAX_GEN_QUIZ_ITERS 3
#define MAX_TITLE_LENGTH 51
#define MAX_QUESTION_SCORE 1000
#define WAIT_TIME_TICKS (int)(HZ / 10.0)
#define TRACK_OFFSET 20000
#define NUM_SCORES 5
#define RESUME_FILE PLUGIN_GAMES_DATA_DIR "/music_quiz.resume"
#define SCORE_FILE PLUGIN_GAMES_DATA_DIR "/music_quiz.score"
/* Users change config in plugin rather than editing file,
   so we can store config in binary format */
#define CONFIG_FILE PLUGIN_GAMES_DATA_DIR "/music_quiz.cfg.bin"

/* Structs */

/* All user changeable config stored here */
struct music_quiz_config {
    int question_option_number;
    int question_time_allowed;
    bool hide_options;
};

/* Stores track names for each question and correct answer audio file path.
   The correct answer track name is stored in tracks[0] */
struct question {
    char tracks[MAX_QUESTION_OPTIONS][MAX_TITLE_LENGTH];
    char track_file[MAX_PATH];
};

/* Stores all config and data for quiz instance */
struct music_quiz {
    struct music_quiz_config config;
    struct question questions[QUESTION_BLOCK_SIZE];
    /* Tracks which options are hidden */
    bool hidden_options[MAX_QUESTION_OPTIONS - 1];
    /* Indexes questions */
    int current_question;
    /* This is used to randomise the order question options are displayed */
    int correct_answer_offset;
    int questions_answered;
    int correct_answers;
    int score;
    /* Stores where we played up to in track. Used when resuming quiz */
    int current_track_offset;
};

/* Global vars */

static struct screen *display;
/* Viewport for progressbar, score, etc */
static struct viewport vp_info;
/* Viewport for displaying questions options */
static struct viewport vp_options;
/* List which shows question options */
static struct gui_synclist lists;
static struct music_quiz quiz;
static struct music_quiz_config config;
static struct tagcache_search tcs;
static struct highscore highscores[NUM_SCORES];
/* Used to randomly place tracks in questions struct when generating quiz */
static bool option_tracker[QUESTION_BLOCK_SIZE * MAX_QUESTION_OPTIONS];
static bool resume_file_exists = false;
static bool usb_connected = false;
static bool config_changed = false;

/* Function declarations */

static const char *get_option_name(int, void *, char *, size_t);
static void init_quiz_screen(void);
static bool to_next_question(bool, double *, int *, int *, int, int);
static bool save_quiz(void);
static bool load_quiz(void);
static bool music_quiz_in_game_menu(void);
static void skip_empty_options(int, int);
static void do_music_quiz(void);
static bool create_quiz_playlist(int);
static bool generate_quiz(int, bool);
static bool init_quiz(void);
static int music_quiz_menu_cb(int, const struct menu_item_ex *);
static void quiz_end(void);
static bool save_config(void);
static bool load_config(void);
static void init_music_config(void);
static void configure_music_quiz_menu(void);
static void setup_and_do_quiz(bool);
static enum plugin_status music_quiz_main_menu(void);
static bool database_initialized(void);
enum plugin_status plugin_start(const void *);

/* Function definitions */

/* quiz.correct_answer_offset is used to randomise the index the
   correct answer is show at for each question. We use it to convert
   the selected_item into a track index */
static const char *get_option_name(int selected_item, void *data,
                                   char *buffer, size_t buffer_len)
{
    (void)data;

    struct question qtion = quiz.questions[quiz.current_question];
    int track_name_index = (selected_item + quiz.correct_answer_offset)
                           % quiz.config.question_option_number;

    if (quiz.config.hide_options && track_name_index != 0
        && quiz.hidden_options[track_name_index - 1]) {
        rb->strcpy(buffer, "");
    } else {
        rb->strlcpy(buffer, qtion.tracks[track_name_index], buffer_len);
    }

    return buffer;
}

static void init_quiz_screen(void)
{
    display = rb->screens[0];
    rb->viewport_set_defaults(&vp_info, 0);
    rb->viewport_set_defaults(&vp_options, 0);

    vp_info.height = (SYSFONT_HEIGHT * 3) + 8;
    vp_options.height = LCD_HEIGHT - (vp_info.height + 1);
    vp_options.y = vp_info.height + 1;

    rb->lcd_set_viewport(&vp_info);
    rb->lcd_clear_display();
    rb->gui_synclist_init(&lists, get_option_name, NULL, true, 1, &vp_options);
    rb->gui_synclist_set_nb_items(&lists, quiz.config.question_option_number);
    rb->gui_synclist_draw(&lists);
}

static bool to_next_question(bool is_correct, double *question_score,
                             int *ticks, int *next_hide_score,
                             int hide_score_interval, int correct_index)
{
    rb->audio_stop();

    int text_width, text_height;
    char *msg;

    if (is_correct) {
        msg = "Correct";
        quiz.correct_answers++;
        quiz.score += (int)*question_score;
    } else {
        msg = "Wrong";
        rb->lcd_set_viewport(&vp_options);
        rb->gui_synclist_select_item(&lists, correct_index);
        rb->gui_synclist_draw(&lists);
        rb->lcd_update();
    }

    rb->lcd_set_viewport(&vp_info);
    rb->lcd_getstringsize(msg, &text_width, &text_height);
    rb->lcd_putsxy((LCD_WIDTH - text_width) / 2, text_height, msg);
    rb->lcd_update();
    rb->sleep(HZ);
    rb->lcd_clear_display();

    /* Generate more questions if we've run out */
    if (++quiz.current_question == QUESTION_BLOCK_SIZE) {
        if (!(generate_quiz(QUESTION_BLOCK_SIZE, false) && !usb_connected)) {
            return false;
        }

        if (!create_quiz_playlist(QUESTION_BLOCK_SIZE)) {
            return false;
        }

        init_quiz_screen();
    }

    rb->memset(quiz.hidden_options, 0, sizeof(quiz.hidden_options));
    quiz.questions_answered++;
    quiz.correct_answer_offset = rb->rand() % quiz.config.question_option_number;
    rb->lcd_set_viewport(&vp_info);
    *question_score = MAX_QUESTION_SCORE;
    *ticks = 0;
    *next_hide_score = MAX_QUESTION_SCORE - hide_score_interval;

    rb->lcd_set_viewport(&vp_options);
    rb->gui_synclist_select_item(&lists, 0);
    rb->gui_synclist_draw(&lists);
    rb->lcd_update();

    rb->playlist_start(quiz.current_question, TRACK_OFFSET, 0);

    return true;
}

static bool save_quiz(void)
{
    int fd = rb->open(RESUME_FILE, O_WRONLY|O_CREAT, 0666);

    if (fd < 0) {
        return false;
    }

    bool save_success = rb->write(fd, &quiz, sizeof(quiz)) > 0;

    rb->close(fd);

    if (!save_success) {
        rb->remove(RESUME_FILE);
    }

    return save_success;
}

static bool load_quiz(void)
{
    int fd = rb->open(RESUME_FILE, O_RDONLY);

    if (fd < 0) {
        return true;
    }

    bool load_success = rb->read(fd, &quiz, sizeof(quiz)) == sizeof(quiz);

    rb->close(fd);

    return load_success;
}

static bool music_quiz_in_game_menu(void)
{
    MENUITEM_STRINGLIST(in_game_menu, "Music Quiz Menu", NULL,
                        "Resume Quiz", "Save And Stop Quiz", "End Quiz");

    int selection = 0;
    bool end_quiz = false;

    switch (rb->do_menu(&in_game_menu, &selection, NULL, false)) {
        case 0:
            break;
        case 1:
            if (!save_quiz()) {
                rb->splash(HZ * 2, "Failed to save quiz");
            } else {
                resume_file_exists = true;
            }

            end_quiz = true;
            break;
        case 2:
            quiz_end();
            end_quiz = true;
            break;
        case MENU_ATTACHED_USB:
            usb_connected = end_quiz = true;
            break;
        default:
            break;
    }

    return end_quiz;
}

/* If question options are hidden then we don't want the user to be
   able to select them. This function changes the select option to the next
   non-hidden option in the list when the currently selected option is hidden */
static void skip_empty_options(int selected_index, int direction)
{
    int track_name_index = (selected_index + quiz.correct_answer_offset)
                           % quiz.config.question_option_number;
    bool change = false;

    /* The correct answer will never be hidden, so while the index is not that
       of the correct answer and the current option is hidden find the next
       option we can try */
    while (track_name_index != 0 && quiz.hidden_options[track_name_index - 1]) {
        selected_index = (selected_index + direction)
                         % quiz.config.question_option_number;
        change = true;

        if (selected_index < 0) {
            selected_index = quiz.config.question_option_number - 1;
        }

        track_name_index = (selected_index + quiz.correct_answer_offset)
                           % quiz.config.question_option_number;
    }

    if (change) {
        rb->lcd_set_viewport(&vp_options);
        rb->gui_synclist_select_item(&lists, selected_index);
        rb->gui_synclist_draw(&lists);
        rb->lcd_update();
    }
}

static void do_music_quiz(void)
{
    int button;
    /* quiz.current_track_offset will be 0 unless resuming a quiz */
    int ticks_passed = quiz.current_track_offset;
    /* The amount we reduce the score by each iteration */
    double score_reduction = MAX_QUESTION_SCORE /
                             ((double)(quiz.config.question_time_allowed * HZ)
                             / WAIT_TIME_TICKS);
    /* question_score is equal to MAX_QUESTION_SCORE unless we're resuming */
    double question_score = MAX_QUESTION_SCORE -
                            ((ticks_passed / WAIT_TIME_TICKS) * score_reduction);
    int hide_score_interval = (int)((double)MAX_QUESTION_SCORE /
                              (quiz.config.question_option_number - 1));
    /* Score should reduce to <= next_hide_score before we hide another option */
    int next_hide_score = question_score - hide_score_interval;
    int selected_index = 0;
    char score[18];

    int question_score_width, total_score_width, text_height;
    display->set_viewport(&vp_info);
    rb->lcd_getstringsize("1000", &question_score_width, &text_height);

    int track_start_time = TRACK_OFFSET +
                           (int)(((double)quiz.current_track_offset / HZ) * 1000);
    rb->playlist_start(quiz.current_question, track_start_time, 0);

    while (true) {
        rb->yield();
        button = rb->get_action(CONTEXT_TREE, HZ / WAIT_TIME_TICKS);
        rb->gui_synclist_do_button(&lists, &button, LIST_WRAP_UNLESS_HELD);

        switch (button) {
            case ACTION_STD_OK:
                selected_index = rb->gui_synclist_get_sel_pos(&lists);
                int correct_index = (quiz.config.question_option_number -
                                     quiz.correct_answer_offset)
                                    % quiz.config.question_option_number;

                if (!to_next_question(selected_index == correct_index,
                                      &question_score, &ticks_passed,
                                      &next_hide_score, hide_score_interval,
                                      correct_index)) {
                    return;
                }

                break;
            case ACTION_TREE_STOP:
            case ACTION_STD_CANCEL:
                rb->audio_pause();
                quiz.current_track_offset = ticks_passed;
                bool stop = music_quiz_in_game_menu();

                if (stop) {
                    rb->audio_stop();
                    rb->playlist_remove_all_tracks(NULL);
                    return;
                } else {
                    init_quiz_screen();
                    rb->audio_resume();
                }

                break;
            case ACTION_STD_PREV:
                /* Change selected option and skip over hidden options */
                selected_index = rb->gui_synclist_get_sel_pos(&lists);
                skip_empty_options(selected_index, -1);
                break;
            case ACTION_STD_NEXT:
                /* Change selected option and skip over hidden options */
                selected_index = rb->gui_synclist_get_sel_pos(&lists);
                skip_empty_options(selected_index, 1);
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    usb_connected = true;
                    return;
                }

                break;
        }

        ticks_passed += WAIT_TIME_TICKS;
        question_score -= score_reduction;

        /* Update screen */
        display->set_viewport(&vp_info);
        rb->lcd_putsf(1, 0, (question_score < 100 ? " %2d " : "%3d"),
                      (int)question_score);

        if (quiz.questions_answered > 0) {
            rb->lcd_putsf(1, 1, "%d/%d", quiz.correct_answers,
                          quiz.questions_answered);
        }

        rb->snprintf(score, sizeof(score), "%d", quiz.score);
        rb->lcd_getstringsize(score, &total_score_width, &text_height);
        rb->lcd_putsxy(LCD_WIDTH - total_score_width - 5, text_height, score);

        int bar_height = SYSFONT_HEIGHT - 2;
        int bar_y = (text_height - bar_height) / 2;

        rb->gui_scrollbar_draw(display, 5 + question_score_width, bar_y,
                vp_info.width - (11 + question_score_width), SYSFONT_HEIGHT - 2,
                MAX_QUESTION_SCORE, 0, (int)question_score, 1);
        display->update_viewport_rect(5 + question_score_width, bar_y,
                                      vp_info.width - 6, SYSFONT_HEIGHT * 2);

        rb->lcd_update();

        /* Check if we need to hide another option */
        if (quiz.config.hide_options && question_score > 0
            && question_score <= next_hide_score) {
            /* Randomly decide which option to hide */
            int hide_option_index = rb->rand()
                                    % (quiz.config.question_option_number - 1);

            while (quiz.hidden_options[hide_option_index]) {
                hide_option_index = (hide_option_index + 1)
                                    % (quiz.config.question_option_number - 1);
            }

            quiz.hidden_options[hide_option_index] = true;
            rb->lcd_set_viewport(&vp_options);
            rb->gui_synclist_draw(&lists);
            selected_index = rb->gui_synclist_get_sel_pos(&lists);
            skip_empty_options(selected_index, 1);
            next_hide_score = question_score - hide_score_interval;
        } else if (ticks_passed >= (quiz.config.question_time_allowed * HZ)) {
            /* The user has run out of time for this question */
            int correct_index = quiz.config.question_option_number -
                                quiz.correct_answer_offset;

            if (!to_next_question(false, &question_score, &ticks_passed,
                                  &next_hide_score, hide_score_interval,
                                  correct_index)) {
                return;
            }
        }

        if (usb_connected) {
            return;
        }
    }
}

static bool create_quiz_playlist(int question_num)
{
    if (!((rb->playlist_amount() == 0 || rb->playlist_remove_all_tracks(NULL) == 0)
          && rb->playlist_create(NULL, NULL) == 0)) {
        return false;
    }

    for (int i = 0; i < question_num; i++) {
        struct question q = quiz.questions[i];
        if (rb->playlist_insert_track(NULL, q.track_file,
                    PLAYLIST_INSERT_LAST, false, false) < 0) {
            return false;
        }

        rb->yield();
    }

    rb->playlist_sync(NULL);

    return true;
}

static bool generate_quiz(int question_num, bool first_call)
{
    char *generate_msg;

    if (first_call) {
        generate_msg = "Generating Quiz...";
    } else {
        generate_msg = "Generating Next Questions...";
        rb->viewport_set_fullscreen(&vp_info, SCREEN_MAIN);
    }

    rb->lcd_clear_display();
    int text_width, text_height;
    rb->lcd_getstringsize(generate_msg, &text_width, &text_height);
    rb->lcd_putsxy((LCD_WIDTH - text_width) / 2, (LCD_HEIGHT - text_height) / 2,
                   generate_msg);
    rb->lcd_update();

    rb->memset(option_tracker, 0, sizeof(option_tracker));
    int track_num = question_num * quiz.config.question_option_number;
    int tracks_done = 0;
    int min_track_length = (quiz.config.question_time_allowed + 1) * 1000;
    int current_track;
    int next_track_index;
    int range;
    int action;

    /* Unless a user has very few entries in their DB, the tracks
       in their DB are all very short or title attributes aren't set,
       this loop should only require one iteration */
    for (int passes = 0; passes < MAX_GEN_QUIZ_ITERS
                         && tracks_done < track_num; passes++) {
        if (!rb->tagcache_search(&tcs, tag_title)) {
            rb->splash(HZ * 2, "Unable to search Database");
            return false;
        }

        if (tcs.entry_count == 0 ||
                track_num > (tcs.entry_count * MAX_GEN_QUIZ_ITERS)) {
            rb->splash(HZ * 2, "Not enough entries in DB."
                               " Reduce question option number.");
            return false;
        }

        current_track = 0;
        range = tcs.entry_count / track_num;
        next_track_index = rb->rand() % range;

        while (rb->tagcache_get_next(&tcs) && tracks_done < track_num) {
            if (current_track != next_track_index) {
                current_track++;
                continue;
            }

            /* Check if we need to stop */
            action = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);

            if (action == ACTION_STD_CANCEL) {
                return false;
            } else if (rb->default_event_handler(action) == SYS_USB_CONNECTED) {
                usb_connected = true;
                return false;
            }

            /* Find random place to put our track */
            int rand = rb->rand();
            int option_index = rand % track_num;

            while (option_tracker[option_index]) {
                option_index = (option_index + 1) % track_num;
            }

            /* Convert this into a place in our questions struct */
            int question_index = option_index / quiz.config.question_option_number;
            int track_index = option_index % quiz.config.question_option_number;

            if (rb->strlen(tcs.result) == 0 ||
                rb->strcmp(tcs.result, "<Untagged>") == 0) {
                next_track_index = current_track + 1;
                continue;
            }

            /* If this track happens to go at index 0 in tracks then
               it will be a correct answer and we need to fetch other details
               for this track */
            if (track_index == 0) {
                long track_length = rb->tagcache_get_numeric(&tcs, tag_length);

                /* If this tracks not long enough then skip it */
                if (track_length < min_track_length) {
                    next_track_index = current_track + 1;
                    continue;
                }

                /* Can't retrieve the audio file path then skip this track */
                if (!rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_filename,
                         quiz.questions[question_index].track_file, MAX_PATH)) {
                    continue;
                }
            }

            /* Copy track name into tracks and mark this spot as used */
            rb->strlcpy(quiz.questions[question_index].tracks[track_index],
                        tcs.result, MAX_TITLE_LENGTH - 1);
            option_tracker[option_index] = true;

            next_track_index = current_track +
                               (range - (current_track % range)) + (rand % range);
            tracks_done++;

            rb->yield();
        }

        rb->tagcache_search_finish(&tcs);
    }

    if (tracks_done < track_num) {
        rb->splash(HZ * 2, "Unable to generate quiz.");
        return false;
    }

    quiz.current_question = 0;

    return true;
}

static bool init_quiz(void)
{
    quiz.config = config;
    quiz.correct_answers = 0;
    quiz.questions_answered = 0;
    quiz.score = 0;
    quiz.correct_answer_offset = rb->rand() % quiz.config.question_option_number;
    quiz.current_track_offset = 0;
    rb->memset(quiz.hidden_options, 0, sizeof(quiz.hidden_options));

    return generate_quiz(QUESTION_BLOCK_SIZE, true);
}

static void quiz_end(void)
{
    int pos = highscore_update(quiz.score, 0, "", highscores, NUM_SCORES);

    if (pos != -1) {
        if (pos == 0) {
            rb->splashf(HZ, "New High Score: %d", quiz.score);
        }

        highscore_show(-1, highscores, NUM_SCORES, false);
    }
}

static bool save_config(void)
{
    int fd = rb->open(CONFIG_FILE, O_WRONLY|O_CREAT, 0666);

    if (fd < 0) {
        return false;
    }

    bool save_success = rb->write(fd, &config, sizeof(config)) > 0;

    rb->close(fd);

    if (!save_success) {
        rb->remove(CONFIG_FILE);
    }

    return save_success;
}

static bool load_config(void)
{
    int fd = rb->open(CONFIG_FILE, O_RDONLY);

    if (fd < 0) {
        return true;
    }

    bool load_success = rb->read(fd, &config, sizeof(config)) == sizeof(config);

    rb->close(fd);

    return load_success;
}

static void init_music_config(void)
{
    if (rb->file_exists(CONFIG_FILE) && load_config()) {
        return;
    }

    /* Default config values */
    config.question_option_number = 5;
    config.question_time_allowed = 10;
    config.hide_options = true;
}

static void configure_music_quiz_menu(void)
{
    MENUITEM_STRINGLIST(config_menu, "Music Quiz Configuration Menu", NULL,
                        "Question Options", "Question Time",
                        "Hide Question Options");

    int selection = 0;
    bool exit = false;
    int old_ivalue;
    bool old_bvalue;

    while (!exit) {
        switch (rb->do_menu(&config_menu, &selection, NULL, false)) {
            case 0:
                old_ivalue = config.question_option_number;
                rb->set_int("Question Options", "", UNIT_INT,
                            &config.question_option_number, NULL, 1,
                            MIN_QUESTION_OPTIONS, MAX_QUESTION_OPTIONS, NULL);
                config_changed = config_changed ||
                                (old_ivalue != config.question_option_number);
                break;
            case 1:
                old_ivalue = config.question_time_allowed;
                rb->set_int("Question Time", "sec", UNIT_INT,
                            &config.question_time_allowed, NULL, 1,
                            MIN_QUESTION_TIME, MAX_QUESTION_TIME, NULL);
                config_changed = config_changed ||
                                 (old_ivalue != config.question_time_allowed);
                break;
            case 2:
                old_bvalue = config.hide_options;
                rb->set_bool("Hide Question Options", &config.hide_options);
                config_changed = config_changed ||
                                 (old_bvalue != config.hide_options);
                break;
            case MENU_ATTACHED_USB:
                usb_connected = exit = true;
                break;
            default:
                exit = true;
                break;
        }
    }
}

static void setup_and_do_quiz(bool is_resume)
{
    rb->srand(*rb->current_tick);

    if (is_resume) {
        if (!load_quiz()) {
            rb->splash(HZ * 2, "Unable to load quiz");
            return;
        }
    } else if (!init_quiz()) {
        return;
    }

    if (!create_quiz_playlist(QUESTION_BLOCK_SIZE)) {
        rb->splash(HZ * 2, "Unable create playlist");
        return;
    }

    if (is_resume) {
        rb->remove(RESUME_FILE);
        resume_file_exists = false;
    }

    init_quiz_screen();
    do_music_quiz();
}

static int music_quiz_menu_cb(int action, const struct menu_item_ex *this_item)
{
    intptr_t item = (intptr_t)this_item;

    /* Hide resume option if no resume file exists */
    if (action == ACTION_REQUEST_MENUITEM && !resume_file_exists && item == 0) {
        return ACTION_EXIT_MENUITEM;
    }

    return action;
}

static enum plugin_status music_quiz_main_menu(void)
{
    MENUITEM_STRINGLIST(main_menu, "Music Quiz Menu", music_quiz_menu_cb,
                        "Resume Quiz", "Start Quiz", "Configure Quiz",
                        "High Scores", "Quit");

    enum plugin_status status = PLUGIN_OK;
    int selection = 0;
    bool exit = false;

    init_music_config();
    highscore_load(SCORE_FILE, highscores, NUM_SCORES);
    resume_file_exists = rb->file_exists(RESUME_FILE);

    while (!exit && !usb_connected) {
        switch (rb->do_menu(&main_menu, &selection, NULL, false)) {
            case 0:
                setup_and_do_quiz(true);
                break;
            case 1:
                setup_and_do_quiz(false);
                break;
            case 2:
                configure_music_quiz_menu();
                break;
            case 3:
                highscore_show(-1, highscores, NUM_SCORES, false);
                break;
            case 4:
                exit = true;
                break;
            case MENU_ATTACHED_USB:
                usb_connected = true;
                break;
            default:
                break;
        }

        if (usb_connected) {
            status = PLUGIN_USB_CONNECTED;
        }
    }

    save_config();
    highscore_save(SCORE_FILE, highscores, NUM_SCORES);

    return status;
}

static bool database_initialized(void)
{
    return rb->file_exists(TAGCACHE_FILE_MASTER);
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    backlight_ignore_timeout();

    enum plugin_status status = PLUGIN_OK;

    if (database_initialized()) {
        status = music_quiz_main_menu();
    } else {
        rb->splash(HZ * 4, "The Database must be initialized to run music_quiz");
    }

    backlight_use_settings();

    return status;
}
