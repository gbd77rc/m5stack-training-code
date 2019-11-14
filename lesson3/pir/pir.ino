#include <M5Stack.h>

// More information about ISR etc can be found here 
// https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

const uint8_t TRIGGER_PIN = 19;  // PIN number for the GPIO - This is the only differnce between the BUTTON and PIR Sketches
volatile int trigger_count = 0;  // Need volatile as the variable could be read as it is being updated by the ISR  

// REMEMBER alway declare a function before using it.
void print_count()
{ 
    Serial.println("ISR Started");
    M5.Lcd.setCursor(0, 0);  
    M5.Lcd.printf("Triggered : %i\r\n", trigger_count);
    //delay(5000);  //Uncomment to see what happens when you try to use Interrupt within an Interrupt
    Serial.println("ISR Finished...");
}

// ISR Callback Function - Must NOT HAVE ANY PARAMETERS and Global/Static
void isr_triggered()
{
    trigger_count++;   // Incrementation Counter
    print_count();
}

// Initialise the sketch - Only called once on start
void setup()
{
    Serial.begin(115200);  // How fast will data to be transmitted to the serial monitor
    M5.begin();
    M5.Lcd.clear();
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);    
    pinMode(TRIGGER_PIN , INPUT);      // Set the TRIGGER_PIN mode to INPUT as we are expecting an external action
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), 
              isr_triggered, RISING);   // Assign the ISR Callback function to PIN. It will fire when voltage goes from 0 to positive
    print_count();
}

void loop()
{
}
