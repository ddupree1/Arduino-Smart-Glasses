#include <Arduino.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); 

const int RXPin = 6, TXPin = 5;
const uint32_t GPSBaud = 9600; //Default baud of NEO-6M is 9600


TinyGPSPlus gps; // the TinyGPS++ object
SoftwareSerial gpsSerial(RXPin, TXPin); // the serial interface to the GPS device

MUIU8G2 mui;

long stop_watch_timer = 0;                      // stop watch timer 1/100 seconds 
long stop_watch_millis = 0;                      // millis() value, when the stop watch was started
uint8_t is_stop_watch_running = 1;          // defines the current state of the stop watch: running or not running
uint8_t gps_connected = 0;
long distance = 0;
unsigned long short_distance = 0;




/* draw the current stop watch value */
uint8_t mui_draw_current_timer(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
      u8g2.setCursor(mui_get_x(ui), mui_get_y(ui));
      u8g2.print(stop_watch_timer/1000);
      u8g2.print(".");
      u8g2.print((stop_watch_timer/10)%100);
  }
  return 0;
}

uint8_t mui_draw_distance(mui_t *ui, uint8_t msg) {
  if(msg == MUIF_MSG_DRAW) {
    u8g2.setCursor(mui_get_x(ui), mui_get_y(ui));
    u8g2.print(distance * (3.6/50000));
  }
  return 0;
}

/* start the stop watch */
uint8_t mui_start_current_timer(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_FORM_START ) {
      is_stop_watch_running = 1;
      stop_watch_millis = millis();
      stop_watch_timer = 0;
      distance = 0;
      short_distance = 0;
  }
  return 0;
}

uint8_t mui_connect_gps(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_FORM_START ) {
      if(gps_connected != 0) {
        mui.gotoForm(3, 0);
      }
  }
  return 0;
}

/* stop the stop watch timer */
uint8_t mui_stop_current_timer(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_FORM_START )
      is_stop_watch_running = 0;
  return 0;
}


muif_t muif_list[] = {
  /* normal text style */
  MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),

  /* custom MUIF callback to draw the timer value */
  MUIF_RO("CT", mui_draw_current_timer),

  MUIF_RO("DI", mui_draw_distance),

  MUIF_RO("CO", mui_connect_gps),
  
  /* custom MUIF callback to start the stop watch timer */
  MUIF_RO("ST", mui_start_current_timer),

  /* custom MUIF callback to end the stop watch timer */
  MUIF_RO("SO", mui_stop_current_timer),

  /* a button for the menu... */
  MUIF_BUTTON("GO", mui_u8g2_btn_goto_wm_fi),
  
  /* MUI_LABEL is used to place fixed text on the screeen */
  MUIF_LABEL(mui_u8g2_draw_text)
};

fds_t fds_data[] = 
MUI_FORM(1)
MUI_STYLE(0)
MUI_XYAT("GO", 65, 35, 2, " Run " )

MUI_FORM(2)
MUI_AUX("CO")
MUI_STYLE(0)
MUI_LABEL(15, 35, " Connecting to GPS ")

MUI_FORM(3)
MUI_AUX("SO")                      // this will stop the stop watch time once this form is entered
MUI_STYLE(0)
MUI_XY("CT", 5, 12)
MUI_XY("DI", 80, 12)
MUI_LABEL(110, 12, "mi")
MUI_XYAT("GO",60, 60, 4, " Start ")     // jump to the second form to start the timer

MUI_FORM(4)
MUI_AUX("ST")                      // this will start the stop watch time once this form is entered
MUI_STYLE(0)
MUI_XY("CT", 5, 12)
MUI_XY("DI", 80, 12)
MUI_LABEL(110, 12, "mi")
MUI_XYAT("GO",60, 60, 3, " Stop ")      // jump to the first form to stop the timer
;

void setup() {
  u8g2.begin(/* menu_select_pin= */ 4, /* menu_next_pin= */ U8X8_PIN_NONE, /* menu_prev_pin= */ U8X8_PIN_NONE, /* menu_up_pin= */ U8X8_PIN_NONE, /* menu_down_pin= */ U8X8_PIN_NONE, /* menu_home_pin= */ 3);
  mui.begin(u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
  gpsSerial.begin(GPSBaud);
  mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
  

}

uint8_t is_redraw = 1;
long milliseconds = 0;

void loop() {

      if(gpsSerial.available() > 0) {
        gps_connected = 1;
        
      }

  /* check whether the menu is active */
  if ( mui.isFormActive() ) {

    /* if so, then draw the menu */
    if ( is_redraw ) {
      u8g2.firstPage();
      do {
          mui.draw();
      } while( u8g2.nextPage() );
      is_redraw = 0;
    }
    
    /* handle events */
    switch(u8g2.getMenuEvent()) {
      case U8X8_MSG_GPIO_MENU_SELECT:
        mui.sendSelect();
        is_redraw = 1;
        break;
      case U8X8_MSG_GPIO_MENU_NEXT:
        mui.nextField();
        is_redraw = 1;
        break;
      case U8X8_MSG_GPIO_MENU_PREV:
        mui.prevField();
        is_redraw = 1;
        break;
    }
    

    /* update the stop watch timer */
    if ( is_stop_watch_running != 0 ) {
      stop_watch_timer = millis() - stop_watch_millis;
      long speed = 15.0;
      if (gps.encode(gpsSerial.read())) {
          if(gps.speed.isValid()) {
            short_distance = gps.speed.mph() * 0.1;
          }
      }   

      distance += short_distance;      
      is_redraw = 1;
    }

    

      
  } else {
      /* the menu should never become inactive, but if so, then restart the menu system */
      mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
  }
}




