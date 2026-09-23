#include <Keypad.h>
#include <Servo.h>
#include <string.h>

// Quantidade de linhas e colunas do keypad.
const byte ROWS = 4;
const byte COLS = 4;

// Representação das teclas físicas.
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Pinos conectados às linhas do keypad.
byte rowPins[ROWS] = {9, 8, 7, 6};

// Pinos conectados às colunas do keypad.
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(
    makeKeymap(keys),
    rowPins,
    colPins,
    ROWS,
    COLS);

Servo doorServo;

const byte SERVO_PIN = 10;

// VSL-001: credencial armazenada diretamente no firmware.
const char ADMIN_PIN[] = "7319";

// Buffer usado para armazenar o PIN digitado.
char enteredPin[5] = {0};
byte pinLength = 0;

// Buffer usado para os comandos recebidos por UART.
char serialBuffer[32] = {0};
byte serialLength = 0;

bool doorLocked = true;

void lockDoor()
{
  doorLocked = true;

  // 0 graus representa a fechadura trancada.
  doorServo.write(0);

  // LED apagado representa a porta trancada.
  digitalWrite(LED_BUILTIN, LOW);
}

void unlockDoor()
{
  doorLocked = false;
  doorServo.write(90);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println(F("[INFO] Door unlocked"));
}

void unlockTemporarily()
{
  unlockDoor();

  delay(3000);

  lockDoor();
  Serial.println(F("[INFO] Door relocked"));
}

void clearEnteredPin()
{
  memset(enteredPin, 0, sizeof(enteredPin));
  pinLength = 0;
}

void submitPin()
{
  Serial.println();

  if (strcmp(enteredPin, ADMIN_PIN) == 0)
  {
    Serial.println(F("[INFO] Valid administrator PIN"));
    unlockTemporarily();
  }
  else
  {
    Serial.println(F("[WARN] Invalid PIN"));
  }

  clearEnteredPin();
}

void handleKeypad()
{
  char key = keypad.getKey();

  // Nenhuma tecla pressionada.
  if (!key)
  {
    return;
  }

  if (key >= '0' && key <= '9')
  {
    // Evita escrever além do tamanho do buffer.
    if (pinLength < sizeof(enteredPin) - 1)
    {
      enteredPin[pinLength] = key;
      pinLength++;
      enteredPin[pinLength] = '\0';

      // Não mostra o número digitado.
      Serial.print('*');
    }
  }
  else if (key == '*')
  {
    clearEnteredPin();
    Serial.println(F("\n[INFO] PIN entry cleared"));
  }
  else if (key == '#')
  {
    submitPin();
  }
}

void executeSerialCommand()
{
  if (strcmp(serialBuffer, "help") == 0)
  {
    Serial.println(F("Available commands:"));
    Serial.println(F("  help"));
    Serial.println(F("  status"));
  }
  else if (strcmp(serialBuffer, "status") == 0)
  {
    if (doorLocked)
    {
      Serial.println(F("Door status: LOCKED"));
    }
    else
    {
      Serial.println(F("Door status: OPEN"));
    }
  }
  else if (serialLength > 0)
  {
    Serial.println(F("Unknown command"));
  }

  memset(serialBuffer, 0, sizeof(serialBuffer));
  serialLength = 0;
}

void handleSerialConsole()
{
  while (Serial.available() > 0)
  {
    char received = Serial.read();

    // Ignora carriage return.
    if (received == '\r')
    {
      continue;
    }

    // Line feed indica o final do comando.
    if (received == '\n')
    {
      executeSerialCommand();
      continue;
    }

    // Garante que não escreveremos além do buffer.
    if (serialLength < sizeof(serialBuffer) - 1)
    {
      serialBuffer[serialLength] = received;
      serialLength++;
      serialBuffer[serialLength] = '\0';
    }
    else
    {
      Serial.println(F("Command too long"));

      memset(serialBuffer, 0, sizeof(serialBuffer));
      serialLength = 0;
    }
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  doorServo.attach(SERVO_PIN);
  lockDoor();

  // Velocidade da comunicação serial.
  Serial.begin(115200);

  // VSL-003: informações internas expostas.
  Serial.println(F("SmartLock ready"));
}

void loop()
{
  handleKeypad();
  handleSerialConsole();
}