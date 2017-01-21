#include "Menu.h"

#define MAX_ENTRIES 50
#define MAX_CHOICES 10

#define SOUND_ENABLED 1
#define MUSIC_ENABLED 2
#define FULLSCREEN_ENABLED 4
#define DRAW_BACKGROUND 5
#define ALPHA_BLEND 6
#define EFFECTS_ENABLED 7
#define GAME_DIFFICOULTY 9

typedef struct _settingEntry {
  char title[80];
  int choice_count;
  char choices[MAX_CHOICES][20];
  int selected;
} SettingEntry;
SettingEntry entries[] = {
  {"audio", 0, {""}, 0},
  {"sound", 2, {"on", "off"}, 0},
  {"music", 2, {"on", "off"}, 0},
  {"video", 0, {""}, 0},
  {"full screen", 2, {"on", "off"}, 0},
  {"background", 2, {"on", "off"}, 0},
  {"alpha blend", 2, {"on", "off"}, 0},
  {"effects", 2, {"on", "off"}, 0},
  {"game", 0, {""}, 0},
  {"difficulty", 3, {"easy", "normal", "hard"}, 0},
  {"", 0, {""}, 0}
};
int entry_count;
SDL_Surface *back;

// menu cursor
int m_menu_y = 1;
#define BULLET_FACE_COUNT 4
int m_face = 0, m_face_mod = 4;
int bg_x = 0, bg_y = 0;

int
getSelectionCharIndex(int n)
{
  int t;
  int sum = 0;
  for(t = 0; t < entries[n].choice_count; t++) {
    if(t > 0)
      sum++;
    if(entries[n].selected == t)
      return sum;
    sum += strlen(entries[n].choices[t]);
  }
  return -1;
}

int
getNumberOfCharWidthOfSelection(int n)
{
  return strlen(entries[n].choices[entries[n].selected]);
}

void
drawSettings()
{
  int i, t, start, end;
  SDL_Rect pos;
  char s[200];
  int y = 2 * (FONT_HEIGHT + 5);

  for(i = 0; i < MAX_ENTRIES; i++) {
    if(!strlen(entries[i].title)) {
      entry_count = i;
      break;
    }

    if(entries[i].choice_count > 0) {
      drawString(screen, 35, y, entries[i].title);
      // concat choices
      strcpy(s, "");
      for(t = 0; t < entries[i].choice_count; t++) {
        if(t > 0)
          strcat(s, " ");
        strcat(s, entries[i].choices[t]);
      }

      start = getSelectionCharIndex(i);
      end = start + getNumberOfCharWidthOfSelection(i);

      // highlight the selected item
      pos.x = 250 + getFontPixelWidth(s, 0, start) - 2;
      pos.y = y;
      pos.w = getFontPixelWidth(s, start, end) + 4;
      pos.h = FONT_HEIGHT;
      SDL_FillRect(screen, &pos,
                   SDL_MapRGBA(screen->format, 0xf0, 0xf0, 0x00, 0x00));

      // draw the choices
      drawString(screen, 250, y, s);
    } else {
      drawString(screen, 50, y, entries[i].title);
      pos.x = 35;
      pos.y = y + FONT_WIDTH;
      pos.w = screen->w - 70;
      pos.h = 2;
      SDL_FillRect(screen, &pos,
                   SDL_MapRGBA(screen->format, 0x80, 0x50, 0x00, 0x00));
    }
    y += (FONT_HEIGHT + 5);
  }
}

int
isEnabled(int n)
{
  if(n == SOUND_ENABLED || n == MUSIC_ENABLED)
    return sound_loaded;
  else
    return 0;
}

void
saveSettings()
{
  int old_screen, old_music;

  old_screen = mainstruct.full_screen;
  old_music = music_enabled;

  sound_enabled = !(entries[SOUND_ENABLED].selected);
  music_enabled = !(entries[MUSIC_ENABLED].selected);
  if(old_music != music_enabled) {
    if(music_enabled) {
      playIntroMusic();
    } else {
      stopMusic();
    }
  }
  mainstruct.full_screen = !(entries[FULLSCREEN_ENABLED].selected);
  if(old_screen != mainstruct.full_screen) {
    SDL_WM_ToggleFullScreen(screen);
  }
  game.difficoulty = entries[GAME_DIFFICOULTY].selected;
  mainstruct.drawBackground = !(entries[DRAW_BACKGROUND].selected);
  mainstruct.alphaBlend = !(entries[ALPHA_BLEND].selected);
  mainstruct.effects_enabled = !(entries[EFFECTS_ENABLED].selected);
  setAlphaBlends();
}

void
loadSettings()
{
  entries[SOUND_ENABLED].selected = (sound_loaded && sound_enabled ? 0 : 1);
  entries[MUSIC_ENABLED].selected = (sound_loaded && music_enabled ? 0 : 1);
  entries[FULLSCREEN_ENABLED].selected = (mainstruct.full_screen ? 0 : 1);
  entries[GAME_DIFFICOULTY].selected = game.difficoulty;
  entries[DRAW_BACKGROUND].selected = (mainstruct.drawBackground ? 0 : 1);
  entries[EFFECTS_ENABLED].selected = (mainstruct.effects_enabled ? 0 : 1);
  entries[ALPHA_BLEND].selected = (mainstruct.alphaBlend ? 0 : 1);
}

void
paintScreen()
{
  SDL_Rect pos;

  saveSettings();

  if(m_face % 2 == 0) {
    bg_x--;
    if(bg_x < -40)
      bg_x = 0;
    bg_y--;
    if(bg_y < -40)
      bg_y = 0;
  }
  pos.x = bg_x;
  pos.y = bg_y;
  pos.w = back->w;
  pos.h = back->h;
  SDL_BlitSurface(back, NULL, screen, &pos);
  drawString(screen, 0, 0, "abe!! settings");
  drawString(screen, 0, screen->h - 40, "arrows to navigate");
  drawString(screen, 0, screen->h - 20, "space to toggle");
  drawSettings();

  // draw cursor
  m_face++;
  if(m_face >= m_face_mod * BULLET_FACE_COUNT)
    m_face = 0;
  pos.x = 5;
  pos.y = (m_menu_y + 2) * (FONT_HEIGHT + 5);
  pos.w = images[img_bullet[0]]->image->w;
  pos.h = images[img_bullet[0]]->image->h;
  SDL_BlitSurface(images[img_bullet[m_face / m_face_mod]]->image, NULL,
                  screen, &pos);

  SDL_Flip(screen);
}

void
paintAboutScreen()
{
  SDL_Rect pos;
  int n = 15;

  if(m_face % 2 == 0) {
    bg_x--;
    if(bg_x < -40)
      bg_x = 0;
    bg_y--;
    if(bg_y < -40)
      bg_y = 0;
  }
  pos.x = bg_x;
  pos.y = bg_y;
  pos.w = back->w;
  pos.h = back->h;
  SDL_BlitSurface(back, NULL, screen, &pos);

  drawString(screen, 50, 10, "abes amazing adventure!!");
  pos.x = 35;
  pos.y = 10 + FONT_WIDTH;
  pos.w = screen->w - 70;
  pos.h = 2;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0x80, 0x50, 0x00, 0x00));


  drawString(screen, 50, 10 + FONT_HEIGHT * 2,
             "long years have passed since his");
  drawString(screen, 50, 10 + FONT_HEIGHT * 3, "friend disappeared");

  drawString(screen, 50, 10 + FONT_HEIGHT * 5,
             "now abe braves the deadly twisting");
  drawString(screen, 50, 10 + FONT_HEIGHT * 6,
             "passages of the great pyramid!");

  drawString(screen, 50, 10 + FONT_HEIGHT * 8,
             "he seeks to free his friend and");
  drawString(screen, 50, 10 + FONT_HEIGHT * 9,
             "uncover the mystery of the depths");

  pos.x = screen->w / 2 - (tom[0]->w / 2);
  pos.y = screen->h - 30 - tom[0]->h;
  pos.w = tom[0]->w;
  pos.h = tom[0]->h;
  if(m_face / n == 1 || m_face / n == 3) {
    SDL_BlitSurface(tom[1], NULL, screen, &pos);
  } else {
    SDL_BlitSurface(tom[m_face / n], NULL, screen, &pos);
  }
  m_face++;
  if(m_face >= 4 * n)
    m_face = 0;

  drawString(screen, screen->w / 2 - 100, screen->h - 30, "press any key");
  SDL_Flip(screen);
}

void
showSettings()
{
  SDL_Event event;

  loadSettings();
  createBack(&back);

  while(1) {
    paintScreen();

    while(SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          goto escape;
        case SDLK_DOWN:
          do {
            m_menu_y++;
            if(m_menu_y >= entry_count)
              m_menu_y = 0;
          } while(entries[m_menu_y].choice_count == 0);
          break;
        case SDLK_UP:
          do {
            m_menu_y--;
            if(m_menu_y < 0)
              m_menu_y = entry_count - 1;
          } while(entries[m_menu_y].choice_count == 0);
          break;
        case SDLK_SPACE:
          if(isEnabled(m_menu_y)) {
            entries[m_menu_y].selected++;
            if(entries[m_menu_y].selected >= entries[m_menu_y].choice_count)
              entries[m_menu_y].selected = 0;
          }
          break;
        default:
          break;
        }
        break;
      default:
        break;
      }
    }
    SDL_Delay(15);
  }
escape:
  SDL_FreeSurface(back);
}

void
showAbout()
{
  SDL_Event event;

  createBack(&back);

  while(1) {
    paintAboutScreen();
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_KEYDOWN)
        goto escape;
    }
  }

escape:
  SDL_FreeSurface(back);
}
