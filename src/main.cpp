#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
// #include <demos/lv_demos.h>
#include <ui.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string>

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 480;

// Global JSON document to reuse memory (Optimization)
static JsonDocument navDoc;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 8 ];

TFT_eSPI tft = TFT_eSPI();

Preferences preferences;
uint16_t calData[5];
bool calibration_saved = false;
int image_to_display;

#define DEVICE_NAME "BikeNavESP32"
#define SERVICE_UUID "3982587f-ced2-414a-9fe8-d2b53a1e4edd"
#define CHARACTERISTIC_UUID "075a6687-6b8c-4ab7-9b3c-5913fa300734"

BLEServer *pServer = NULL;
BLEAdvertising *pAdvertising = NULL;
BLECharacteristic *pCharacteristic;
lv_obj_t *ui_Battery;
lv_obj_t *ui_Signal;
lv_obj_t *ui_SpeedLimitCont;
lv_obj_t *ui_SpeedLimitValue;
void update_navigation_ui(const char* json_data);
void update_image_component(int maneuver_id);
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p );
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data );
void touch_calibrate();

// --- Call Action Handlers ---
static void ui_event_CallAccept(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        if (pCharacteristic) {
            pCharacteristic->setValue("{\"action\":\"accept\"}");
            pCharacteristic->notify();
            Serial.println("Sent Call Accept action to phone");
        }
        lv_obj_add_flag(ui_CallPanel, LV_OBJ_FLAG_HIDDEN); // Hide panel after action
    }
}

static void ui_event_CallDecline(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        if (pCharacteristic) {
            pCharacteristic->setValue("{\"action\":\"decline\"}");
            pCharacteristic->notify();
            Serial.println("Sent Call Decline action to phone");
        }
        lv_obj_add_flag(ui_CallPanel, LV_OBJ_FLAG_HIDDEN); // Hide panel after action
    }
}

// --- 1. Define the Server Callback Class ---
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("A client has connected.");
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected.");
        // Faster restart for advertising to help phone auto-reconnect
        delay(500); 
        pServer->startAdvertising(); 
    }
};

// Callback class to handle incoming writes from the Central device
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            Serial.print(F("Received Value: "));
            Serial.println(rxValue.c_str());
            update_navigation_ui(rxValue.c_str());
        }
    }
};

void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
}

void update_navigation_ui(const char* json_data) {
    // 1. Parse the JSON (reusing the static document)
    navDoc.clear();
    DeserializationError error = deserializeJson(navDoc, json_data);

    if (error) {
        Serial.print(F("JSON parsing failed: "));
        Serial.println(error.c_str());
        return;
    }

    // --- A. Top Bar Updates (Clock & Speed) ---
    if (navDoc["clock"].is<const char*>()) {
        const char* clock_val = navDoc["clock"];
        if (strcmp(clock_val, lv_label_get_text(ui_Clock)) != 0) {
            lv_label_set_text(ui_Clock, clock_val);
        }
    }
    
    if (navDoc["speed"].is<const char*>()) {
        const char* new_speed = navDoc["speed"];
        if (new_speed && new_speed[0] != '\0') {
            if (strcmp(new_speed, lv_label_get_text(ui_Speed)) != 0) {
                lv_label_set_text(ui_Speed, new_speed);
            }
        } else {
            lv_label_set_text(ui_Speed, "0");
        }
        
        // Hardcoded Speed Alerts
        int current = atoi(new_speed);
        if (current >= 70) {
            lv_obj_set_style_text_color(ui_Speed, lv_color_hex(0xFF0000), 0); // RED
        } else if (current >= 55) {
            lv_obj_set_style_text_color(ui_Speed, lv_color_hex(0xFFA500), 0); // ORANGE
        } else {
            lv_obj_set_style_text_color(ui_Speed, lv_color_hex(0x007AFF), 0); // BLUE
        }
    }

    // --- B. Health Updates ---
    if (navDoc["bat"].is<int>()) {
        int bat = navDoc["bat"];
        char buf[32];
        snprintf(buf, sizeof(buf), LV_SYMBOL_BATTERY_FULL " %d%%", bat);
        lv_label_set_text(ui_Battery, buf);
    }
    
    if (navDoc["sig"].is<int>()) {
        int sig = navDoc["sig"];
        char buf[32];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %d", sig);
        lv_label_set_text(ui_Signal, buf);
    }

    if (navDoc["call"].is<JsonObject>()) {
        int call_state = navDoc["call"]["st"] | 0; // 0: idle, 1: ringing
        const char* caller_name = navDoc["call"]["nm"];
        
        if (call_state == 1) {
            lv_label_set_text(ui_CallerName, (caller_name && caller_name[0] != '\0') ? caller_name : "Unknown Caller");
            lv_obj_clear_flag(ui_CallPanel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_CallPanel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // --- D. Navigation Updates ---
    bool has_nav = false;
    const char* m_code = nullptr;
    const char* t_str = nullptr;
    const char* d_str = nullptr;
    const char* i_str = nullptr;

    if (navDoc["nav"].is<JsonObject>()) {
        m_code = navDoc["nav"]["m"];
        t_str = navDoc["nav"]["t"];
        d_str = navDoc["nav"]["d"];
        i_str = navDoc["nav"]["i"];
        has_nav = true;
    }

    if (has_nav) {
        if (m_code) update_image_component(atoi(m_code));
        
        if (t_str && strcmp(t_str, lv_label_get_text(ui_ETA)) != 0) {
            lv_label_set_text(ui_ETA, t_str);
        }
        if (d_str && strcmp(d_str, lv_label_get_text(ui_EKM)) != 0) {
            lv_label_set_text(ui_EKM, d_str);
        }
        if (i_str && strcmp(i_str, lv_label_get_text(ui_Instruction)) != 0) {
            lv_label_set_text(ui_Instruction, i_str);
        }
    }
}

void update_image_component(int maneuver_id) {

    // Safety Check 1: Ensure the ID is within the bounds of the array.
    if (maneuver_id >= 0 && maneuver_id < MANEUVER_IMAGE_COUNT) {
        // 1. Get the image pointer directly from the array index
        const lv_img_dsc_t * image_to_display = maneuverImages[maneuver_id];
        // 2. Update the LVGL Image object
        lv_img_set_src(ui_NavDirection, image_to_display);
    } else {
        // Handle error: Maneuver ID out of bounds.
        Serial.printf("Error: Maneuver ID %d is out of array bounds.\n", maneuver_id);
    }
}



void touch_calibrate()
{

  // Try to load saved calibration data
  preferences.begin("touch_cal", false); // Open preferences namespace
  calibration_saved = preferences.getBytes("cal_data", calData, sizeof(calData)) == sizeof(calData);
  preferences.end();

  if (calibration_saved) {
    tft.setTouch(calData);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.println();
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    // Save the new data to non-volatile memory
    preferences.begin("touch_cal", false);
    preferences.putBytes("cal_data", calData, sizeof(calData));
    preferences.end();
  }
}

void setup()
{
    Serial.begin( 115200 );

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    lv_init();


    tft.begin();
    tft.setRotation(0); 
    touch_calibrate();

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 8 );

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register( &indev_drv );



    ui_init();
    
    // Default speed to 0
    lv_label_set_text(ui_Speed, "0");

    // Create Manual Dashboard Components (Health)
    ui_Battery = lv_label_create(ui_Screen1);
    lv_obj_set_style_text_color(ui_Battery, lv_color_hex(0x4CAF50), 0); // GREEN for Battery
    lv_obj_set_style_text_font(ui_Battery, &lv_font_montserrat_14, 0);
    lv_obj_align(ui_Battery, LV_ALIGN_TOP_MID, -20, 25); // Top Center, offset right
    lv_label_set_text(ui_Battery, LV_SYMBOL_BATTERY_FULL " --");

    ui_Signal = lv_label_create(ui_Screen1);
    lv_obj_set_style_text_color(ui_Signal, lv_color_hex(0x2196F3), 0); // BLUE for Signal
    lv_obj_set_style_text_font(ui_Signal, &lv_font_montserrat_14, 0);
    lv_obj_align(ui_Signal, LV_ALIGN_TOP_MID, -28, 45); // Top Center, offset left
    lv_label_set_text(ui_Signal, LV_SYMBOL_WIFI " --");

    // Speed Limit Indicator (Red Circle)
    ui_SpeedLimitCont = lv_obj_create(ui_Screen1);
    lv_obj_set_size(ui_SpeedLimitCont, 45, 45);
    lv_obj_set_style_radius(ui_SpeedLimitCont, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui_SpeedLimitCont, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(ui_SpeedLimitCont, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(ui_SpeedLimitCont, 4, 0);
    lv_obj_align(ui_SpeedLimitCont, LV_ALIGN_CENTER, 80, -60);
    lv_obj_add_flag(ui_SpeedLimitCont, LV_OBJ_FLAG_HIDDEN); // Keep hidden as requested

    ui_SpeedLimitValue = lv_label_create(ui_SpeedLimitCont);
    lv_obj_set_style_text_color(ui_SpeedLimitValue, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui_SpeedLimitValue, &lv_font_montserrat_18, 0);
    lv_obj_center(ui_SpeedLimitValue);
    lv_label_set_text(ui_SpeedLimitValue, "00");

    // Register Call Interaction Events
    lv_obj_add_event_cb(ui_CallAccept, ui_event_CallAccept, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_CallDecline, ui_event_CallDecline, LV_EVENT_ALL, NULL);

    // 1. Create the BLE Device
    BLEDevice::init(DEVICE_NAME);
    BLEDevice::setMTU(512); 

    // 2. Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks()); 

    // 3. Create a Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // 4. Create a Characteristic and set its properties (READ, WRITE, NOTIFY)
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, 
        BLECharacteristic::PROPERTY_WRITE | 
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902()); // Required for Notifications
    pCharacteristic->setCallbacks(new MyCallbacks()); 

    // 5. Start the Service
    pService->start();

    // 6. Start Advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    // Help Android discovery
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    
    pServer->startAdvertising();
    Serial.println("Waiting for BLE connection...");

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); 
    delay( 5 );
}


