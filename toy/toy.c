#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

AbRect paddle = {abRectGetBounds, abRectCheck, {15,1}}; /**< 15x1 paddle >**/
AbRect ball = {abRectGetBounds, abRectCheck, {1,1}}; /**< 1x1 'ball' >**/

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer3 = {               /**< ball >**/
  (AbShape *)&ball,
  {screenWidth/2,(screenHeight/2)+66},     /* right above bottom paddle */
  {0,0}, {0,0},                            /* last & next pos */
  COLOR_RED,
  0
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},          /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer3,
};

Layer layer1 = {		/**< paddle 1 >**/
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
  &layer1,
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
MovLayer ml3 = { &layer3, {1,1}, 0 };
MovLayer ml1 = { &layer1, {1,0}, &ml3 };
MovLayer ml0 = { &layer0, {1,0}, &ml1 };

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

void p1ctrl(u_int sw) {
  if(!(sw & (1<<0))) {
    ml1.velocity.axes[0] = -1;
  }
  else if(!(sw & (1<<1))) {
    ml1.velocity.axes[0] = 1;
  }
  else {
    ml1.velocity.axes[0] = 0;
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


u_int bgColor = COLOR_WHITE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();
  
  layerInit(&layer0);
  layerDraw(&layer0);
  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  u_char width = screenWidth, height = screenHeight;
  char p1score[1];
  char p2score[1];
  p1score[0] = '0';
  p1score[1] = '\0';
  p2score[0] = '0';
  p2score[1] = '\0';
  drawString5x7(1,1, "P1:", COLOR_BLACK, COLOR_WHITE);
  drawString5x7(98,1, "P2:0", COLOR_BLACK, COLOR_WHITE);
  drawString5x7(20,1, p1score, COLOR_BLACK, COLOR_WHITE);
  drawString5x7(122,1, p2score, COLOR_BLACK, COLOR_WHITE);
  
  u_int sw;
  
  for(;;) {
    sw = p2sw_read();

    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      or_sr(0x10);	      /**< CPU OFF */
    }
    
    p1ctrl(sw);
    p2ctrl(sw);

    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0); 
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
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
