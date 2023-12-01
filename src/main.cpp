#include <Arduino.h>
#include <liquidCrystal_I2C.h>

const uint8_t LCD_NUM_COLS = 20;
const uint8_t LCD_NUM_ROWS = 4;

LiquidCrystal_I2C lcd(0x27, LCD_NUM_COLS, LCD_NUM_ROWS);

enum ConnectionState
{
    Disconnected,
    Connected
};

enum EcgLeadPlacement
{
    Fault,
    Ok
};

ConnectionState connectionState = ConnectionState::Disconnected;
EcgLeadPlacement ecgLeadPLacement = EcgLeadPlacement::Fault;

unsigned long lastConnectionVerificationMillis = 0;

const unsigned long MAX_TIME_WITHOUT_VERIFICATION_MILLIS = 1000;

const byte numChars = 3;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

void recvWithEndMarker()
{
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (Serial.available() > 0 && newData == false)
    {
        rc = Serial.read();

        if (rc != endMarker)
        {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars)
            {
                ndx = numChars - 1;
            }
        }
        else
        {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void writeToLcd(String str, uint8_t rowIdx)
{
    lcd.setCursor(0, rowIdx);

    String paddingRightBuffer = "";

    for (int i = 0; i < LCD_NUM_COLS - str.length(); i++)
    {
        paddingRightBuffer += " ";
    }

    lcd.print(str + paddingRightBuffer);
}

String spacedApart(String left, String right)
{
    int spaceBetween = LCD_NUM_COLS - (left.length() + right.length());

    if (spaceBetween <= 0)
    {
        return left + right;
    }

    String spaceBuffer = "";

    for (int i = 0; i < spaceBetween; i++)
    {
        spaceBuffer += " ";
    }

    return left + spaceBuffer + right;
}

void updateLeadPlacementMessage()
{
    if (ecgLeadPLacement == EcgLeadPlacement::Fault)
    {
        writeToLcd(spacedApart("LEAD PLACEMENT:", "POOR"), 0);
    }

    if (ecgLeadPLacement == EcgLeadPlacement::Ok)
    {
        writeToLcd(spacedApart("LEAD PLACEMENT:", "GOOD"), 0);
    }
}

void updateConnectionMessage()
{
    if (connectionState == ConnectionState::Connected)
    {
        writeToLcd("--Link Established--", 2);
    }

    if (connectionState == ConnectionState::Disconnected)
    {
        writeToLcd("Connect to Computer", 2);
    }
}

void setup()
{
    // initialize the serial communication:
    Serial.begin(57600);
    pinMode(10, INPUT); // Setup for leads off detection LO +
    pinMode(11, INPUT); // Setup for leads off detection LO -

    lcd.init();
    lcd.backlight();

    updateLeadPlacementMessage();
    updateConnectionMessage();

    randomSeed(analogRead(0));
}

void loop()
{
    recvWithEndMarker();

    if (newData == true)
    {
        if (strcmp(receivedChars, "OK") == 0)
        {
            ConnectionState old = connectionState;
            connectionState = ConnectionState::Connected;
            lastConnectionVerificationMillis = millis();

            // Update display only on state change
            if (old == ConnectionState::Disconnected)
            {
                updateConnectionMessage();
            }
        }

        newData = false;
    }

    if (millis() - lastConnectionVerificationMillis > MAX_TIME_WITHOUT_VERIFICATION_MILLIS)
    {
        ConnectionState old = connectionState;
        connectionState = ConnectionState::Disconnected;

        // Update display only on state change
        if (old == ConnectionState::Connected)
        {
            updateConnectionMessage();
        }
    }

    // if ((digitalRead(10) == 1) || (digitalRead(11) == 1))
    // {
    //     Serial.println('!');
    //     delay(1);
    //     return;
    // }

    EcgLeadPlacement old = ecgLeadPLacement;
    ecgLeadPLacement = (digitalRead(10) == 1) || (digitalRead(11) == 1) ? EcgLeadPlacement::Fault : EcgLeadPlacement::Ok;

    // Update display only on state change
    if (ecgLeadPLacement == EcgLeadPlacement::Fault && old == EcgLeadPlacement::Ok)
    {
        updateLeadPlacementMessage();
    }

    // Update display only on state change
    if (ecgLeadPLacement == EcgLeadPlacement::Ok && old == EcgLeadPlacement::Fault)
    {
        updateLeadPlacementMessage();
    }

    Serial.println("(" + String(millis()) + " " + String(analogRead(A0)) + ")");

    // This takes 31/32 milliseconds to carry out and seems to make the code run better
    // TODO try experimenting with making the delay of 1 maybe (1 + 32)
    // writeToLcd(String(random(100)), 3);

    // Wait for a bit to keep serial data from saturating
    delay(20);
}