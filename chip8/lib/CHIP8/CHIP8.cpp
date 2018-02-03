#include <CHIP8.h>

#define DEBUG 1

CPU::CPU(Adafruit_SSD1306 *display, Keypad *keypad, SpiRAM *memory, I2C_EEPROM *eeprom, void (*tick_callback)(), uint8_t buzzer_pin, uint16_t clock_speed) : 
    display(display),
    keypad(keypad),
    memory(memory),
    eeprom(eeprom),
    tick_callback(tick_callback),
    buzzer_pin(buzzer_pin)
{
    if (clock_speed % 60 != 0) {
        clock_speed = clock_speed - (clock_speed % 60);
    }
    this->clock_speed = clock_speed;
    memory_offset = 0;
    keep_going = true;
}

void CPU::begin() {
    pc = 0x200;
    memset(V, 0, sizeof(V));
    I = 0;
    memset(stack, 0, sizeof(stack));
    sp = 0;
    delay_ctr = 0;
    sound_ctr = 0;
    // for each reset, use to the next 4 KiB in memory
    //memory_offset += 4*1024;
    memory->write_stream(memory_offset, (char*)&hexsprites, 80);

    ticks = 0;
    next_tick = 0;
}

void CPU::load(uint16_t eeprom_offset, uint16_t len) {
    uint8_t buffer[32] = {0};
    uint8_t buf_len = 32;
    uint16_t i=0;
    Serial.println("Loading program:");
    while (i<len) {
        if (len-i < 32) {
            buf_len = len-i;
        }
        buf_len = eeprom->read(eeprom_offset+i, buffer, buf_len);
        for (uint8_t j=0; j<buf_len; j++) {
            if (j%8 == 0 && j>0) {
                Serial.print(F(" "));
            }
            if (j%16 == 0 && j>0) {
                Serial.println();
            }
            print_hex(buffer[j]);
            Serial.print(" ");
        }
        Serial.println();
        memory->write_stream(memory_offset+pc+i, (char*)buffer, buf_len);
        i += buf_len;
        delay(10);
    }
    Serial.print("Done, (");
    Serial.print(i);
    Serial.println(" bytes)");
}

void CPU::run() {
    Serial.println("CPU::run()");
    uint16_t opcode;
    next_tick = (uint32_t)micros() + 1000000/clock_speed;
    while (1) {
        // fetch
        memory->read_stream(memory_offset+pc, (char*)&opcode, 2);  // read 2 bytes from RAM
        opcode = swap_u16(opcode);
        pc += 2;

#ifdef DEBUG
        Serial.print("PC: 0x");
        print_hex(pc-2);
        Serial.print(" OPCODE: ");
        print_hex(opcode);
        Serial.println();
#endif

        // execute
        execute(opcode);

        // house-keeping
        clock_tick();

        // trigger the callback function
        if (tick_callback != NULL) {
            tick_callback();
        }
    }
}

void CPU::execute(uint16_t opcode) {
    uint16_t addr = opcode & 0x0FFF;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t kk = opcode & 0x00FF;
    uint8_t tmp[15];

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                // CLS
                display->clear();
                display->show();
            } else if (opcode == 0x00EE) {
                // RET
                sp--;
                pc = stack[sp];
            }
            break;

        case 0x1000:
            // JP addr
            pc = addr;
            break;

        case 0x2000:
            // CALL addr
            stack[sp++] = pc;
            pc = addr;
            break;

        case 0x3000:
            // SE Vx, byte
            if (V[x] == kk) {
                pc += 2;
            }
            break;

        case 0x4000:
            // SNE Vx, byte
            if (V[x] != kk) {
                pc += 2;
            }
            break;

        case 0x5000:
            // SE Vx, Vy
            if (V[x] == V[y]) {
                pc += 2;
            }
            break;

        case 0x6000:
            // LD Vx, byte
            V[x] = kk;
            break;
        
        case 0x7000:
            // ADD Vx, byte
            V[x] += kk;
            break;
        
        case 0x8000:
            switch (opcode & 0xF00F) {
                case 0x8000:
                    // LD Vx, Vy
                    V[x] = V[y];
                    break;
                
                case 0x8001:
                    // OR Vx, Vy
                    V[x] |= V[y];
                    break;

                case 0x8002:
                    // AND Vx, Vy
                    V[x] &= V[y];
                    break;

                case 0x8003:
                    // XOR Vx, Vy
                    V[x] ^= V[y];
                    break;

                case 0x8004:
                    // ADD Vx, Vy
                    V[0xF] = (V[x]+V[y] > 0xFF) ? 1 : 0;
                    V[x] += V[y];
                    break;

                case 0x8005:
                    // SUB Vx, Vy
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    V[x] -= V[y];
                    break;

                case 0x8006:
                    // SHR Vx {, Vy}
                    V[0xF] = V[x] & 0x1;
                    V[x] >>= 1;
                    break;

                case 0x8007:
                    // SUBN Vx, Vy
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    break;

                case 0x800E:
                    // SHL Vx {, Vy}
                    V[0xF] = (V[x] & 0x80) >> 8;
                    V[x] <<= 1;
                    break;

            }
            break;
        
        case 0x9000:
            // SNE Vx, Vy
            if (V[x] != V[y]) {
                pc += 2;
            }
            break;
        
        case 0xA000:
            // LD I, addr
            I = addr;
            break;
        
        case 0xB000:
            // JP V0, addr
            pc = addr + V[0];
            break;
        
        case 0xC000:
            // RND Vx, byte
            V[x] = random(256) & kk;
            break;
        
        case 0xD000:
            // DRW Vx, Vy, nibble
            memory->read_stream(memory_offset+I, (char*)tmp, (opcode & 0x000F));
            V[0xF] = display->drawSprite(V[x], V[y], tmp, (opcode & 0x000F), true);
            //display->show();
            break;
        
        case 0xE000:
            if ((opcode & 0xF0FF) == 0xE09E) {
                // SKP Vx
                if (keypad->isPressed(to_hex(V[x]))) {
                    pc += 2;
                }
            } else if ((opcode & 0xF0FF) == 0xE0A1) {
                // SKNP Vx
                if (!keypad->isPressed(to_hex(V[x]))) {
                    pc += 2;
                }
            }
            break;
        
        case 0xF000:
            switch (opcode & 0xF0FF) {
                case 0xF007:
                    // LD Vx, DT
                    V[x] = delay_ctr;
                    break;

                case 0xF00A:
                    // LD Vx, K
                    V[x] = from_hex(keypad->waitForKey());
                    break;

                case 0xF015:
                    // LD DT, Vx
                    delay_ctr = V[x];
                    break;

                case 0xF018:
                    // LD ST, Vx
                    sound_ctr = V[x];
                    break;

                case 0xF01E:
                    // ADD I, Vx
                    I += V[x];
                    break;

                case 0xF029:
                    // LD F, Vx
                    I = 5*V[x];
                    break;

                case 0xF033:
                    // LD B, Vx
                    // little-endian! for some reason, x << 16 does not work
                    tmp[0] = V[x]%10;               // ones
                    tmp[1] = (V[x]%100) / 10;       // tens
                    tmp[2] = (V[x]%1000) / 100;     // hundreds
                    memory->write_stream(memory_offset+I, (char*)tmp, 3);
                    break;

                case 0xF055:
                    // LD [I], Vx
                    memory->write_stream(memory_offset+I, (char*)V, x+1);
                    break;

                case 0xF065:
                    // LD Vx, [I]
                    memory->read_stream(memory_offset+I, (char*)V, x+1);
                    break;

            }
            break;
    }
}

void CPU::clock_tick() {
    uint32_t now = (uint32_t) micros();
    ticks++;
    if (ticks%50 == 0) {
        //display->show();
    }
    if (ticks % (clock_speed / 60) == 0) {
        // tick the timers
        if (delay_ctr > 0) {
#ifdef DEBUG
            Serial.print(F("DELAY: "));
            Serial.println(delay_ctr);
#endif
            delay_ctr--;
        }
        if (sound_ctr > 0) {
#ifdef DEBUG
            Serial.print(F("SOUND: "));
            Serial.println(sound_ctr);
#endif
            analogWrite(buzzer_pin, 127);
            sound_ctr--;
            if (sound_ctr == 0) {
                analogWrite(buzzer_pin, 0);
            }
        }
    }
    if (next_tick > now) {
        if (next_tick-now < 16384) {
            delayMicroseconds(next_tick - now);
        } else {
            delay((next_tick-now)/1000);
        }
    } else {
        next_tick = now;
    }
    next_tick += 1000000 / clock_speed;
}
