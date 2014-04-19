/* My physics sim ported to Sansa c200 */
#include <plugin.h>
#include <stdlib.h>
#include <button.h>
#define KEY_QUIT BUTTON_MENU
#define KEY_UP BUTTON_UP
#define KEY_DOWN BUTTON_DOWN
#define KEY_LEFT BUTTON_LEFT
#define KEY_RIGHT BUTTON_RIGHT
#define BALL_SIZE 2
#define NUM_BALLS 128
void pxl_on(int x, int y)
{
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_fillrect(x, y, BALL_SIZE,BALL_SIZE);
}
void pxl_off(int x, int y)
{
  rb->lcd_set_foreground(LCD_BLACK);
  rb->lcd_fillrect(x,y,BALL_SIZE,BALL_SIZE);
}
struct ball {
  double x, y, dx, dy, friction, elasticity;
};
struct ball balls[NUM_BALLS];
void step(struct ball *b)
{ 
  double gravity=1;
  int oldx=b->x, oldy=b->y;
  /* Apply gravity */
  b->dy+=gravity;
  /* Find new position */
  b->x+=b->dx;
  b->y+=b->dy;
  /* Calculate collision with walls */
  if(b->x<0 || b->x>=LCD_WIDTH-BALL_SIZE)
    {
      b->dx=-b->dx*b->elasticity;
      b->dy*=b->friction;
      b->x+=2*b->dx;
    }
  if(b->y<0 || b->y>=LCD_HEIGHT-BALL_SIZE)
    {
      /* Find new velocities */
      b->dy=-b->dy*b->elasticity;
      /* Calculate friction on other velocity */
      b->dx*=b->friction;
      b->y+=2*b->dy;
    }
  /* Calculate collisions with other balls */
  for(int i=0;i<NUM_BALLS;++i)
    {
      if(balls[i].x-b->x>=BALL_SIZE && 
  pxl_off(oldx, oldy);
  pxl_on(b->x, b->y);
}
signed int rand_range(int min, int max)
{
  return rb->rand()%(max-min)+min;
}
enum plugin_status plugin_start(const void* parameter)
{
  (void)parameter;
  rb->lcd_clear_display();
  rb->lcd_puts(0,0,"RockPhys v0.1-alpha");
  rb->lcd_puts(0,1,"(C) Franklin Wei");
  rb->lcd_update();
  rb->sleep(HZ*2);
  rb->lcd_clear_display();
  rb->lcd_update();
  for(int i=0;i<NUM_BALLS;++i)
    {
      balls[i].x=rand_range(0, LCD_WIDTH-BALL_SIZE);
      balls[i].y=rand_range(0, LCD_HEIGHT-BALL_SIZE);
      balls[i].dx=rand_range(-10, 10);
      balls[i].dy=rand_range(-10, 10);
      balls[i].elasticity=.8;
      balls[i].friction=.95;
    }
  for(;;)
    {
      for(int i=0;i<NUM_BALLS;++i)
	{
	  step(&balls[i]);
	}
      rb->lcd_update();
      rb->yield();
    }
}
