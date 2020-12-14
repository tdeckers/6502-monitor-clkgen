const char ADDR[] = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52};
const char DATA[] = {39, 41, 43, 45, 47, 49, 51, 53};
#define CLOCK 2
#define READ_WRITE 3
#define TRIGGER 4

// clock frequency
const int f = 16; // Hz

// 6502 program.
const char PROGRAM[] = "a9 ff 8d 02 60 a2 00 bd 15 80 8d 00 60 e8 e0 0a f0 f3 4c 07 80 c0 f9 a4 b0 99 92 02 f8 80 90 ";
                                 
unsigned int ram[1024]; // RAM: we have 1 MB of ram
const unsigned int RAM_START = 0x0000; // RAM start address

unsigned int rom[1024]; // ROM: we have 1 MB of rom
const unsigned int ROM_START = 0x8000; // ROM start address

// load the program into the rom variable from the PROGRAM variable.
void loadProgram(char * pData, unsigned int m[]) {
  unsigned int idx = 0;
  while(*pData != 0) { // until end of string...
    
    if (*pData == ' ') { // if we hit a space, skip it.
      pData++;
    }

    // convert double char (e.g. 'e2') into unsigned int
    unsigned int code = strtoul(pData, &pData, 16);
    m[idx++] = code;
  }
}

void setup() {
  for (int n = 0; n < 16; n += 1) {
    pinMode(ADDR[n], INPUT);
  }
  for (int n = 0; n < 8; n += 1) {
    pinMode(DATA[n], INPUT);
  }
  pinMode(CLOCK, OUTPUT);
  digitalWrite(CLOCK, LOW);
  pinMode(READ_WRITE, INPUT);
  pinMode(TRIGGER, INPUT_PULLUP);

  // We're now generating the clock.
  //attachInterrupt(digitalPinToInterrupt(CLOCK), onClock, RISING);

  Serial.begin(57600);
  // load program at 
  loadProgram(PROGRAM, rom);
  Serial.print("ROM size: "); Serial.println(sizeof(rom) / sizeof(rom[0]));
  Serial.print("RAM size: "); Serial.println(sizeof(ram) / sizeof(ram[0]));
}

// Ben Eater's code to print out bus data.
void onClock() {
  char output[15];

  unsigned int address = 0;
  for (int n = 0; n < 16; n += 1) {
    int bit = digitalRead(ADDR[n]) ? 1 : 0;
    Serial.print(bit);
    address = (address << 1) + bit;
  }

  Serial.print("   ");

  unsigned int data = 0;
  for (int n = 0; n < 8; n += 1) {
    int bit = digitalRead(DATA[n]) ? 1 : 0;
    Serial.print(bit);
    data = (data << 1) + bit;
  }

  sprintf(output, "   %04x  %c %02x", address, digitalRead(READ_WRITE) ? 'r' : 'W', data);
  Serial.println(output);
}

// write data onto the bus
void write(unsigned int data) {
  for (int n = 0; n < 8; n += 1) {
    pinMode(DATA[n], OUTPUT);
  }
  unsigned int mask = 0b10000000;
  for (int n = 0; n < 8; n += 1) {
    digitalWrite(DATA[n], data & mask ? 1 : 0);
    mask = mask >> 1;
  }
}

void loop() {
  // We can step through code by setting frequency low, and touch TRIGGER to low
  while (digitalRead(TRIGGER) == HIGH) {
    // Do nothing
  }
  digitalWrite(CLOCK, LOW);
  // tADS (address setup time is 30ns).
  __builtin_avr_delay_cycles (1); // on 16Mhz, 1 cycle is 62.5ns

  // Set data pins back to input (in case we wrote last cycle).
  for (int n = 0; n < 8; n += 1) {
    pinMode(DATA[n], INPUT);
  }

  // is the CPU reading?
  if (digitalRead(READ_WRITE) == HIGH) {
    // read address
    unsigned int address = 0;
    for (int n = 0; n < 16; n += 1) {
      int bit = digitalRead(ADDR[n]) ? 1 : 0;
      address = (address << 1) + bit;
    }

    // decode address
    if (address == 0xfffc) {        // init vector 
      write(0x00FF & ROM_START); // low byte
    } else if (address == 0xfffd) { // init vector 
      write(ROM_START >> 8);     // high byte
    } else if ( address >= RAM_START && address <= RAM_START + (sizeof(ram) / sizeof(ram[0]))) {
      // includes stack 0x0100 to 0x01ff
      write(ram[address - RAM_START]);
    } else if ( address >= ROM_START && address <= ROM_START + (sizeof(rom) / sizeof(rom[0]))) {
      write(rom[address - ROM_START]);
    }
    
  }
  delay(1000 / (f * 2));
  digitalWrite(CLOCK, HIGH);
  
  delay(1000 / (f * 4));
  // read bus data in the middle of clock high
  onClock();
  
  // Handle writes to RAM (includes stack).
  if (digitalRead(READ_WRITE) == LOW) {
    // read address
    unsigned int address = 0;
    for (int n = 0; n < 16; n += 1) {
      int bit = digitalRead(ADDR[n]) ? 1 : 0;
      address = (address << 1) + bit;
    }

    // ram writes, includes stack (0x0100-0x01ff)
    if ( address >= RAM_START && address <= RAM_START + (sizeof(ram) / sizeof(ram[0]))) {
      unsigned int data = 0;
      for (int n = 0; n < 8; n += 1) {
        int bit = digitalRead(DATA[n]) ? 1 : 0;      
        data = (data << 1) + bit;
      }      
      ram[address - RAM_START] = data;
    }
  }
  delay(1000 / (f * 4));
}
