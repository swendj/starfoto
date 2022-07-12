/*****************************************************************************
  This is the "starfoto" sketch.

  Module        : main module, starfoto.ino
  Author        : Swen Hopfe (dj)
  Design        : 2019-01-30
  Last modified : 2019-01-31

  It works with ESP32, NEMA17 stepper with 1:26.85 gear,
  stepper driver DRV8825 and a ssd1306 based OLED . It was
  tested with ESP32-ST wired on a WEMOS LOLIN32 board.

 ****************************************************************************/

  #include <Wire.h>

  // oled driver include
  #include "SSD1306Wire.h"
  // #include "SH1106Wire.h"

  // new display object
  SSD1306Wire  display(0x3c, 21, 22);
  // SH1106Wire display(0x3c, 21, 22);

  // pins for stepper control
  const int stepPin = 16;
  const int dirPin = 17;
  const int resPin = 3;
  // pins for keypad
  const int button1 = 2;
  const int button2 = 4;
  const int button3 = 15;

  // menu level
  int mlv = 1;

  // last time stamp for event handling
  long ev_last_millis = 0;

  // last time stamp for stepping (normal operation)
  long last_millis = 0;

  // number of max shots/steps
  const int ns = 20;
  // counter of shots/steps
  int z = 0;

  // Current head position (0-5369 = angle of 0-359°)
  int hpos = 0;
  // decimal degrees of current head position (angle)
  float gpos = 0.000;
  // hpos / 0-pos difference
  int hdiff = 0;

  // start flag for high speed test
  boolean fl_hspeed = false;
  // start flag for normal operation
  boolean fl_normop = false;
  // start flag for turn forward
  boolean fl_forwrd = false;
  // start flag for rewind
  boolean fl_backwrd = false;
  // start flag for stepping to hpos=0
  boolean fl_adj = false;

//-----------------------------------------------------------------

    // main menu
    void main_menu() {

      String hstr;

      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_LEFT);

      hstr = String(hpos);
      if (hstr.length() < 4) hstr = "0" + hstr;
      if (hstr.length() < 4) hstr = "0" + hstr;
      if (hstr.length() < 4) hstr = "0" + hstr;
      hstr = "          # " + hstr + " #";

      switch(mlv) {
        case 1:
          display.drawString(0, 0 , " 1 - 36° vorwärts");
          display.drawString(0, 16, " 2 - Step zurück");
          display.drawString(0, 32, " 3 - Weiter");
          display.drawString(0, 48, hstr);
          break;
        case 2:
          display.drawString(0, 0 , " 1 - Fahre auf 0-P");
          display.drawString(0, 16, " 2 - Nullung");
          display.drawString(0, 32, " 3 - Weiter");
          display.drawString(0, 48, hstr);
          break;
        case 3:
          display.drawString(0, 0 , " 1 - Start!");
          display.drawString(0, 16, " 2 - 360°-Test");
          display.drawString(0, 32, " 3 - Zurück");
          display.drawString(0, 48, hstr);
          break;
        default:
          break;
      }
      display.display();
    }

//-----------------------------------------------------------------

    // single line OLED message
    void message(String msg) {

      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 24, msg);
      display.display();

    }

//-----------------------------------------------------------------

    // do the step
    void stepper(int stepPin) {

      digitalWrite(stepPin,HIGH);
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(500);

    }

//-----------------------------------------------------------------

  void setup() {

      Serial.begin(115200);
      Serial.println();
      Serial.println("Starfoto initialising...");

      // set the control pins to output
      pinMode(stepPin,OUTPUT);
      pinMode(dirPin,OUTPUT);
      pinMode(resPin,OUTPUT);
      // set keypad pins to input
      pinMode(button1,INPUT);
      pinMode(button2,INPUT);
      pinMode(button3,INPUT);
      delay(500);

      // set a particular direction (clockwise)
      digitalWrite(dirPin,LOW);

      // initialise OLED
      digitalWrite(resPin,LOW);
      delay(50);
      digitalWrite(resPin,HIGH);
      delay(50);
      display.init();
      display.flipScreenVertically();
      delay(500);

      // welcome screen
      display.clear();
      display.setFont(ArialMT_Plain_24);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 18, "    Starfoto");
      display.drawString(0, 31, "    -----------");
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 53, "             V1.0");
      display.display();

      Serial.println("Ready to run...");

      delay(3000);

      // main menu
      main_menu();

      // sets timestamps to current system "millis"
      last_millis = millis();
      ev_last_millis = millis();

  }

//-----------------------------------------------------------------

  void loop() {

    String hstr;
    String gstrs;
    char gstr[6];

      // event handler

      if ((millis() - ev_last_millis) > 1600) {

        ev_last_millis = millis();

        if (mlv ==1) {
          if(HIGH == digitalRead(button3)) {
            Serial.println("Menu level2...");
            mlv = 2;
            main_menu();
          } else {
              if(HIGH == digitalRead(button1)) {
                Serial.println("Forward...");
                fl_forwrd = true;
              } else {
                if(HIGH == digitalRead(button2)) {
                  Serial.println("Rewind...");
                  fl_backwrd = true;
                }
              }
          }
        } else { //mlv==1
          if (mlv == 2) {
            if(HIGH == digitalRead(button3)) {
              Serial.println("Menu level3...");
              mlv = 3;
              main_menu();
            } else {
                if(HIGH == digitalRead(button1)) {
                  Serial.println("Adjusting...");
                  fl_adj = true;
                } else {
                  if(HIGH == digitalRead(button2)) {
                    Serial.println("Set Pos=0...");
                    hpos = 0;
                    main_menu();
                  }
                }
            }
          } else { //mlv==2
            if (mlv == 3) {
              if(HIGH == digitalRead(button3)) {
                Serial.println("Menu level1...");
                mlv = 1;
                main_menu();
              } else {
                  if(HIGH == digitalRead(button1)) {
                    Serial.println("Normal operation...");
                    fl_normop = true;
                  } else {
                    if(HIGH == digitalRead(button2)) {
                      Serial.println("High speed test...");
                      fl_hspeed = true;
                      main_menu();
                    }
                  }
              }
            }
          }
        }
      } //if>1600

      //-------------------------------------------------------

      // highspeed test
      // 26.85 * 200 full steps = 5370 times * 1.8 degrees
      // for a full cycle with our 1:26.85 gear
      // and takes us 5370 * 4ms = 5.4 seconds

      if (fl_hspeed) {

        Serial.println("full circle forward...");
        message("360°-Test...");
        digitalWrite(dirPin,LOW); // turn clockwise
        delayMicroseconds(500);

        for(int x = 0; x < 5370; x++) stepper(stepPin);

        fl_hspeed = false;

        // restore main menu
        main_menu();

      }

      //-------------------------------------------------------

      // stepping forward (36°, 1/10 circle)

      if (fl_forwrd) {

        Serial.println("1/10 circle forward...");
        message("1/10 vorwärts...");
        digitalWrite(dirPin,LOW); // turn clockwise
        delayMicroseconds(500);

        for(int x = 0; x < 537; x++) stepper(stepPin);

        hpos += 537;
        if(hpos > 5369) hpos -= 5370;
        fl_forwrd = false;

        // restore main menu
        main_menu();

      }

      //-------------------------------------------------------

      // rewind (27 steps)

      if (fl_backwrd) {

        Serial.println("27 step back...");
        message("Zurück...");
        delay(500);
        digitalWrite(dirPin,HIGH); // rewind
        delayMicroseconds(500);

        for(int x = 0; x < 27; x++) stepper(stepPin);

        hpos -= 27;
        if(hpos < 0) hpos += 5370;
        fl_backwrd = false;

        // restore main menu
        main_menu();

      }

      //-------------------------------------------------------

      // Step to Position hpos = 0 (adjusting)

      if (fl_adj) {

        Serial.println("Step to 0-Pos...");
        message("Step to 0-Pos...");
        delay(500);
        digitalWrite(dirPin,HIGH); // rewind
        delayMicroseconds(500);
        hdiff = hpos; // cause we rewind to zero

        for(int x = 0; x < hdiff; x++) stepper(stepPin);

        hpos -= hdiff;
        fl_adj = false;

        // restore main menu
        main_menu();

      }

      //-------------------------------------------------------

      // normal operation
      // virtual sky turns 360°/24h clockwise
      // stepper without gear runs with 1.8° by step = 200steps/cycle
      // 200steps/24h(24*3600s=86400s) are equal to 1step/432s
      // that means for sky speed we need 1 step every 432 seconds
      // with 1:26.85 gear we need 1 step every 16.089385475 seconds
      // because with gear we only do 0.067039106° by step

      if (fl_normop) {
        if ((millis() - last_millis) > 16089) {

          delayMicroseconds(385);

          last_millis = millis();

          digitalWrite(dirPin,LOW); // turn clockwise

          // if maximum reached do not step

          if (z < ns) {

            stepper(stepPin);

            z++;
            hpos++;

            Serial.println(z);
            Serial.println(hpos);

            display.clear();
            display.setFont(ArialMT_Plain_24);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64,  4 , String(z));
            display.drawString(64, 20 , " -------------");
            display.setFont(ArialMT_Plain_16);
            hstr = String(hpos);
            gpos = ((float)hpos * 360.000) / 5370.000;
            sprintf(gstr, "%.3f", gpos);
            gstrs = String(gstr);
            if (hstr.length() < 4) hstr = "0" + hstr;
            if (hstr.length() < 4) hstr = "0" + hstr;
            if (hstr.length() < 4) hstr = "0" + hstr;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            if (gstrs.length() < 7) gstrs = "0" + gstrs;
            display.drawString(64, 42 , " " + hstr + "  " + gstrs);
            display.drawString(102, 10 , String(ns));
            display.display();
          }
          else {

            fl_normop = false;

            delay(3000);

            // restore main menu
            main_menu();

          }

          // before the next step
          // we have approx 16 seconds to do the camera shot now
          // so we program the camera to this interval
          // all exposure times below this are possible for the current fotograph
          // nevertheless it is not exactly 16s, we will not run into problems
          // because of the fact that there are only some shots to do
          // we expect less than 20 shots only...
        }
      }
  }

//----------------------------------------------------------------------------
