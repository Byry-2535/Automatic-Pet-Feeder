#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[]  = "699cb323-2241-4288-8935-9157e5af8e27";

const char SSID[]               = SECRET_SSID;
const char PASS[]               = SECRET_OPTIONAL_PASS;
const char DEVICE_KEY[]         = SECRET_DEVICE_KEY;

void onFeedHourChange();
void onFeedMinuteChange();
void onFeedNowChange();
void onServoSwitchChange();
void onRevolutionsChange();

int feedHour;
int feedMinute;
bool feedNow = false;
bool servoSwitch;
int userRevolutions = 3;

void initProperties(){
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);

  ArduinoCloud.addProperty(feedHour, READWRITE, ON_CHANGE, onFeedHourChange);
  ArduinoCloud.addProperty(feedMinute, READWRITE, ON_CHANGE, onFeedMinuteChange);
  ArduinoCloud.addProperty(feedNow, READWRITE, ON_CHANGE, onFeedNowChange);
  ArduinoCloud.addProperty(servoSwitch, READWRITE, ON_CHANGE, onServoSwitchChange);
  ArduinoCloud.addProperty(userRevolutions, READWRITE, ON_CHANGE, onRevolutionsChange);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);