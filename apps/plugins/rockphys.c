/* My physics sim ported to Rockbox */
#include <plugin.h>
#include <stdlib.h>
#include "lib/pluginlib_actions.h"
#define KEY_QUIT PLA_CANCEL
#define KEY_UP PLA_UP
#define KEY_DOWN PLA_DOWN
#define KEY_LEFT PLA_LEFT
#define KEY_RIGHT PLA_RIGHT
#define NUM_BALLS 1
#define PLUGIN_NAME_VERSION "RockPhysics v1.2.1"
double gravity=.5, elasticity=.8, friction=.95;
int grav_int=0, elas_int=0, fric_int=0;
int manual_speed=1, ball_size=2;
bool backlight_status;
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
static void pxl_on(int x, int y)
{
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_fillrect(x, y, ball_size,ball_size);
}
static void pxl_off(int x, int y)
{
  rb->lcd_set_foreground(LCD_BLACK);
  rb->lcd_fillrect(x,y,ball_size,ball_size);
}
struct ball {
  double x, y, dx, dy, friction, elasticity;
};
struct ball balls[NUM_BALLS];
struct ball next_world[NUM_BALLS];
static void check_ball_off_screen(struct ball* b)
{
  if(b->x<0)
    b->x=0;
  else if(b->x>LCD_WIDTH-ball_size)
    b->x=LCD_WIDTH-ball_size;
  
  if(b->y<0)
    b->y=0;
  else if(b->y>LCD_HEIGHT-ball_size)
    b->y=LCD_HEIGHT-ball_size;
}
static void step(struct ball *b)
{
  int oldx=b->x, oldy=b->y;
  /* Find new position */
  b->x+=b->dx;
  b->y+=b->dy;
  /* Calculate collision with walls */
  if(b->x<=0 || b->x>=LCD_WIDTH-ball_size)
    {
      b->dx=-b->dx*b->elasticity;
      b->x+=2*b->dx;
      b->dy*=b->friction;
    }

  if(b->y<=0 || b->y>=LCD_HEIGHT-ball_size)
    {
      /* Find new velocities */
      b->dy=-b->dy*b->elasticity;
      b->y+=2*b->dy;
      /* Calculate friction on other velocity */
      b->dx*=b->friction;
    }

  if(gravity>=0)
    {
      if(b->y<=LCD_HEIGHT-ball_size-gravity)
	b->dy+=gravity;
    }
  else /* negative gravity! */
    {
      if(b->y>=0+gravity)
	b->dy+=gravity;
    }
  /* TODO: Calculate collisions with other balls!!! */
  check_ball_off_screen(b);
  pxl_off(oldx, oldy);
  pxl_on(b->x, b->y);
}
static signed int rand_range(int min, int max)
{
  return rb->rand()%(max-min)+min;
}
void run_sim(void)
{
  rb->lcd_update();
  for(int i=0;i<NUM_BALLS;++i)
    {
      balls[i].x=rand_range(0, LCD_WIDTH-ball_size);
      balls[i].y=rand_range(0, LCD_HEIGHT-ball_size-gravity);
      balls[i].dx=rand_range(-10, 10);
      balls[i].dy=rand_range(-10, 10);
      balls[i].elasticity=elasticity;
      balls[i].friction=friction;
    }
  for(;;)
    {
      int button;
      button = pluginlib_getaction(0, plugin_contexts,
				   ARRAYLEN(plugin_contexts));
      switch(button)
	{
	case 0:
	  break;
	case KEY_QUIT:
	  return;
	case KEY_UP:
	  {
	    for(int i=0;i<NUM_BALLS;++i)
	      {
		balls[i].dy-=manual_speed;
	      }
	    break;
	  }
	case KEY_LEFT:
	  {
	    for(int i=0;i<NUM_BALLS;++i)
	      {
		balls[i].dx-=manual_speed;
	      }
	    break;
	  }
	case KEY_RIGHT:
	  {
	    for(int i=0;i<NUM_BALLS;++i)
	      {
		balls[i].dx+=manual_speed;
	      }
	    break;
	  }
	case KEY_DOWN:
	  {
	    for(int i=0;i<NUM_BALLS;++i)
	      {
		balls[i].dy+=manual_speed;
	      }
	    break;
	  }
	default:  
	  if (rb->default_event_handler(button) == SYS_USB_CONNECTED) 
	    {
	      exit(PLUGIN_USB_CONNECTED);
	    }
	  break;
	}
      for(int i=0;i<NUM_BALLS;++i)
	{
	  step(&balls[i]);
	}
      rb->lcd_update();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(false);
#endif
      rb->yield();
    }
}
static const struct opt_items gravity_settings[7]={
  {"Negative", -5},
  {"Zero-G", 0},
  {"Moon", 5},
  {"Earth", 10},
  {"Jupiter", 20},
  {"Saturn", 40},
  {"Sun", 80}
};
int grav_values[7]={-5, 0, 5, 10, 20, 40, 80};
void set_real_grav(int opt)
{
  gravity=(double)grav_values[opt]/10;
}
static const struct opt_items elasticity_settings[11]={
  {"0%", 0},
  {"10%", 10},
  {"20%", 20},
  {"30%", 30},
  {"40%", 40},
  {"50%", 50},
  {"60%", 60},
  {"70%", 70},
  {"80%", 80},
  {"90%", 90},
  {"100%", 100}
};
void set_real_elas(void)
{
  elasticity=(double)elas_int/100;
}
static const struct opt_items friction_settings[10]={
  {"0% (most)", 0},
  {"25%", 25},
  {"50%", 50},
  {"75%", 75},
  {"80%", 80},
  {"85%", 85},
  {"90%", 90},
  {"95%", 95},
  {"98%", 98},
  {"100% (none)", 100}
};
void set_real_friction(void)
{
  friction=(double)fric_int/100;
}
static const struct opt_items manual_speed_settings[]={
  {"Disabled", 0},
  {"1", 1},
  {"2", 2},
  {"3", 3},
  {"4", 4},
  {"5", 5},
  {"6", 6},
  {"7", 7},
  {"8", 8},
  {"9", 9},
  {"10", 10}
};
static const struct opt_items ball_size_settings[]={
  {"1 pixel", 1},
  {"2 pixels", 2},
  {"3 pixels", 3},
  {"4 pixels", 4},
  {"5 pixels", 5},
  {"6 pixels", 6},
  {"7 pixels", 7},
  {"8 pixels", 8},
  {"9 pixels", 9},
  {"10 pixels", 10}
};
static void rockphys_menu(void)
{
  MENUITEM_STRINGLIST(menu, PLUGIN_NAME_VERSION, NULL, "Start demo", "Gravity", "Elasticity", "Friction", "Manual speed", "Ball size", "About", "Exit");
 menu_start:
  switch(rb->do_menu(&menu, NULL, NULL, false))
    {
    case 0:
      rb->lcd_clear_display();
      run_sim();
      rb->lcd_clear_display();
      goto menu_start;
    case 1:
      rb->set_option("Gravity", &grav_int, INT,
		     gravity_settings,7, &set_real_grav);
      goto menu_start;
    case 2:
      rb->set_option("Elasticity", &elas_int, INT,
		     elasticity_settings, 11, NULL);
      set_real_elas();
      goto menu_start;
    case 3:
      rb->set_option("Friction", &fric_int, INT,
		     friction_settings,10, NULL);
      set_real_friction();
      goto menu_start;
    case 4:
      rb->set_option("Manual speed", &manual_speed, INT,
		     manual_speed_settings,11, NULL);
      goto menu_start;
    case 5:
      rb->set_option("Ball size", &ball_size, INT, ball_size_settings, 10, NULL);
      goto menu_start;
    case 6:
      rb->lcd_clear_display();
      rb->lcd_putsxy(0,0,PLUGIN_NAME_VERSION);
      rb->lcd_putsxy(0,11,"(C) Franklin Wei");
      rb->lcd_putsxy(0,22,"Any key to exit...");
      rb->lcd_update();
      rb->button_get(true); /* wait for a button press */
      goto menu_start;
    case 7:
      return;
    }
}

enum plugin_status plugin_start(const void* parameter)
{
  (void)parameter;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(false); /* can be too fast to see with boost */
#endif
  backlight_status=rb->is_backlight_on(true);
  rb->srand((*rb->current_tick));
  rb->lcd_clear_display();
  rockphys_menu();
  return PLUGIN_OK; /* make GCC happy */
}
