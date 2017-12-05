#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdlib.h>
#include "buzzer.h"

AbRect ball = {                 /**< 1x1 'ball' >**/
  abRectGetBounds, abRectCheck,
  {1,1}
};

AbRectOutline paddle = {               /**< 15x1 paddle >**/
  abRectGetBounds, abRectCheck,
  {15,1}
};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer4 = {                /**< ball >**/
  (AbShape *)&ball,
  {screenWidth/2,(screenHeight/2)+64},     /* right above bottom paddle */
  {0,0}, {0,0},                            /* last & next pos */
  COLOR_RED,
  0
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},          /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer4,
};

Layer layer2 = {		/**< paddle 1 >**/
  (AbShape *)&paddle,
  {screenWidth/2, (screenHeight/2)+68},     /* middle bottom */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &fieldLayer,
};

Layer layer0 = {		/**< paddle 2 >**/
  (AbShape *)&paddle,
  {screenWidth/2, (screenHeight/2)-68},     /* middle top */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer2,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml4 = { &layer4, {1,1}, 0 };
MovLayer ml2 = { &layer2, {1,0}, &ml4 };
MovLayer ml0 = { &layer0, {1,0}, &ml2 };

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


/** Code for reading movement switches and movement speed **/
void p1ctrl(u_int sw) {
  if(!(sw & (1<<0))) {
    ml2.velocity.axes[0] = -1;
  }
  else if(!(sw & (1<<1))) {
    ml2.velocity.axes[0] = 1;
  }
  else {
    ml2.velocity.axes[0] = 0;
  }
}

void p2ctrl(u_int sw) {
  if(!(sw & (1<<2))) {
    ml0.velocity.axes[0] = -1;
  }
  else if(!(sw & (1<<3))) {
    ml0.velocity.axes[0] = 1;
  }
  else {
    ml0.velocity.axes[0] = 0;
  }
}

/** Code for detecting ball collisions for each paddle **/
void collision1() {
  if((layer4.pos.axes[1] >= (layer2.pos.axes[1] - 1))
     && (layer4.pos.axes[0] <= (layer2.pos.axes[0] + 15))
     && (layer4.pos.axes[0] >= (layer2.pos.axes[0] - 15))) {
    buzz2();
    layer4.posNext.axes[1] -= 2;
    ml4.velocity.axes[1] = -ml4.velocity.axes[1];
  }
  
  /*else if((layer4.pos.axes[1] <= (layer0.pos.axes[1] + 1))
	  && (layer4.pos.axes[0] >= (layer0.pos.axes[0] + 15))
	  && (layer4.pos.axes[0] <= (layer0.pos.axes[0] - 15))) {
    buzz2();
    layer4.posNext.axes[1] += 2;
    ml4.velocity.axes[1] = -ml4.velocity.axes[1];
    }*/
}

void collision2() {
  if((layer4.pos.axes[1] <= (layer0.pos.axes[1] + 1))
	  && (layer4.pos.axes[0] >= (layer0.pos.axes[0] + 15))
	  && (layer4.pos.axes[0] <= (layer0.pos.axes[0] - 15))) {
    buzz2();
    layer4.posNext.axes[1] += 2;
    ml4.velocity.axes[1] = -ml4.velocity.axes[1];
  }
}

char score1[] = "00";
char score2[] = "00";

/** Code for drawing and updating score **/
void updateScore(int player) {  
  if(player == 1) {
    layer4.posNext.axes[1] += 2;
    
    score1[1] += 1;
    
    drawString5x7(1, 1, "P1:", COLOR_BLACK, COLOR_WHITE);
    drawString5x7(20, 1, score1, COLOR_BLACK, COLOR_WHITE);
  }
  
  else if(player == 2) {    
    score2[1] += 1;

    drawString5x7(90, 1, "P2:", COLOR_BLACK, COLOR_WHITE);
    drawString5x7(110, 1, score2, COLOR_BLACK, COLOR_WHITE);
  }
}

/** Code for detecting a score for each player **/
int point() {
  if(layer4.pos.axes[1] == (screenHeight/2 - 67)) {
    buzz3();
    return 1;
  }
  
  else if(layer4.pos.axes[1] == (screenHeight/2 + 68)) {
    buzz3();
    return 2;
  }
  else
    return 0;
}

void end()
{
  system("pause");
}

u_int bgColor = COLOR_WHITE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field >**/

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main() {
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  shapeInit();
  layerInit(&layer0);
  layerDraw(&layer0);
  layerGetBounds(&fieldLayer, &fieldFence);
  buzzer_init();
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  u_char width = screenWidth, height = screenHeight;  
  u_int sw;
  
  score1[1] -= 1;
  score2[1] -= 1;
  
  updateScore(1);
  updateScore(2);
  
  for(;;) {
    sw = p2sw_read();

    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      or_sr(0x10);	      /**< CPU OFF */
    }
    
    p1ctrl(sw);
    p2ctrl(sw);
    collision1();

    /* disabled for demo
       collision2(); */

    if(point() > 0)
      {
	updateScore(point());
      }

    if(score1[1] == ':')
      {
	score1[0] += 1;
	score1[1] -= 11;
	updateScore(1);
	
	if(score1[0] == '1')
	  {
	    drawString5x7(10, (screenHeight/2), "GAME OVER, P1 WINS", COLOR_RED, COLOR_YELLOW);
	    buzz1();
	    end();
	  }
      }
    
    if(score2[1] == ':')
      {
	score2[0] += 1;
	score2[1] -= 11;
	updateScore(2);
	
	if(score2[0] == '1')
	  {
	    drawString5x7(10, (screenHeight/2), "GAME OVER, P2 WINS", COLOR_RED, COLOR_YELLOW);
	    buzz1();
	    end();
	  }
      }
    
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler() {
  static short count = 0;
  count ++;
  if (count == 5) {
    mlAdvance(&ml0, &fieldFence);
    if (p2sw_read()) {
      redrawScreen = 1;
    }
    count = 0;
  }
}
