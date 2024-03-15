#include <ADS1220_WE.h>
#include <SPI.h>
#include <movingAvg.h>
#define ADS1220_CS_PIN 17    // chip select pin
#define ADS1220_DRDY_PIN 16  // data ready pin
#include "MovingAverage.h"
#include <Ticker.h>
#include <phyphoxBle.h>
Ticker updater;
MovingAverage filter(4);
ADS1220_WE ads = ADS1220_WE(ADS1220_CS_PIN, ADS1220_DRDY_PIN);
void setup() {
  Serial.begin(921600);
  if (!ads.init()) {
    Serial.println("ADS1220 is not connected!");
    while (1)
      ;
  }
  /* The voltages to be measured need to be between negative VREF + 0.2 V and positive
   * VREF -0.2 V if PGA is enabled. For this basic example I disable PGA, to be on the
   * safe side. */
  ads.bypassPGA(true);
  // Configure the ADC with the following settings:
  /* You can choose a gain between 1 (default) and 128 using setGain() if PGA is enabled
       (default). If PGA is disabled you can still choose a gain factor up to 4. If PGA is
       enabled, the amplified voltage shall be between AVSS + 200mV and AVDD - 200mV. Outside
       this range linearity drops. For details check the data sheet, section 8.3.2.1.

       If you apply a single-ended mode (negative AINx = AVSS), PGA must be bypassed. Accordingly,
       the maximum gain is 4. The library does these settings automatically.

       For the measurement of reference voltages / supply voltage PGA will also be bypassed. In
       this case gain is 1.

       The parameters you can choose for setGain() are:
       ADS1220_GAIN_X with X = 1,2,4,8,16,32,64 or 128

       With getGainFactor() you can query the gain. The function returns the effective gain and
       not the gain set in the register. Under certian conditions thes are are different. For
       example, the effective gain is set to 1 when external references are measured.
    */
  ads.setGain(ADS1220_GAIN_1);
  //  ads.getGainFactor(); // returns the effective gain as a byte value
  //  ads.bypassPGA(true); // true disables PGA, false enables PGA
  //  ads.isPGABypassed(); // returns true, if PGA is bypassed

  /* The data rate level with setDataRate(). The data rate itself also depends on the operating
    mode and the clock. If the internal clock is used or an external clock with 4.096 MHz the data
    rates are as follows (per second):

     Level               Normal Mode      Duty-Cycle      Turbo Mode
    ADS1220_DR_LVL_0          20               5               40         (default)
    ADS1220_DR_LVL_1          45              11.25            90
    ADS1220_DR_LVL_2          90              22.5            180
    ADS1220_DR_LVL_3         175              44              350
    ADS1220_DR_LVL_4         330              82.5            660
    ADS1220_DR_LVL_5         600             150             1200
    ADS1220_DR_LVL_6        1000             250             2000

    The higher the data rate, the higher the noise (tables are provided in section 7.1 in the
    data sheet). In single-shot mode the conversion times equal the times in Normal Mode.
 */
  ads.setDataRate(ADS1220_DR_LVL_5);
  /*  You can choose between a continuous and a single-shot (on demand) mode with
      setConversionMode(). Parameters are:
      ADS1220_SINGLE_SHOT (default)
      ADS1220_CONTINUOUS
  */
  ads.setConversionMode(ADS1220_CONTINUOUS);
  /* In order to obtain temperature values, choose enableTemperatureSensor(true); false will
     disable the temperature sensor. As long as the temperature sensor is enabled the ADS1220
     is blocked for this task. To obtain voltage values, you have to switch the sensor off. The
     temperature is queried with getTemperature();
  */
  ads.enableTemperatureSensor(false);  // disable temperature sensor
                                       //  ads.getTemperature(); // returns temperature as float

  /* You can set a filter to reduce 50 and or 60 Hz noise with setFIRFilter(); Parameters:
     ADS1220_NONE       no filter (default)
     ADS1220_50HZ_60HZ  50Hz and 60Hz filter
     ADS1220_50HZ       50Hz filter
     ADS1220_60HZ       60Hz filter
  */
  // ads.setFIRFilter(ADS1220_60HZ);
  /* When data is ready the DRDY pin will turn from HIGH to LOW. In addition, also the DOUT pin
     can be set as a data ready pin. The function is setDrdyMode(), parameters are:
     ADS1220_DRDY        only DRDY pin is indicating data readiness  (default);
     ADS1220_DOUT_DRDY   DRDY and DOUT pin indicate data readiness
  */
  //  ads.setDrdyMode(ADS1220_DOUT_DRDY);
  ads.setCompareChannels(ADS1220_MUX_0_AVSS);
  updater.attach(0.02, update);
  PhyphoxBLE::start("phyphox device");  //Start the BLE server
}

float result = 0.0;
float lastResult = 0.0;
void update() {
  if (lastResult != result) {
    // Serial.println((int)result);
    PhyphoxBLE::write(result);  //Send value to phyphox
    PhyphoxBLE::poll();
    lastResult = result;
  }
}
void loop() {
  if (digitalRead(ADS1220_DRDY_PIN) == LOW) {
    result = ads.getVoltage_mV();
    result = filter.addSample(result);
  }

  // result = ads.getVRef_V();                 // get the reference voltage
  // Serial.print("Reference       [V]: ");
  // Serial.println(result,3);

  // Serial.print("Temperature    [°C]: ");    // get the temperature
  // result = ads.getTemperature();
  // Serial.println(result);
}