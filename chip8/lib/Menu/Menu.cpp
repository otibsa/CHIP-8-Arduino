#include <Menu.h>

Menu::Menu(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin) :
    cols(cols),
    rows(rows),
    oled(oled),
    keypad(keypad),
    eeprom(eeprom),
    cpu(cpu),
    buzzer_pin(buzzer_pin)
{
    x_step = ASCII_WIDTH/cols;
}

void Menu::begin(uint16_t eeprom_offset, uint16_t item_count) {
    Serial.print(F("Menu::begin(0x"));
    Serial.print(eeprom_offset, HEX);
    Serial.print(F(", "));
    Serial.print(item_count);
    Serial.println(F(")"));
    char c;

    this->eeprom_offset = eeprom_offset;
    this->item_count = item_count;

    memset(buffer, 0, sizeof(buffer));
    stop = false;
    page = 0;
    cursor = 0;
    old_page = 1;
    old_cursor = 1;
    last_page = (item_count-1)/(cols*rows);
    last_cursor = (item_count-1)%(cols*rows);
    oled->clear();
    oled->show();
    oled->set_mode(ASCII_MODE);

    show_info();
    if (item_count == 0) {
        empty_menu();
    }

    while (!stop) {
        Serial.println();
        Serial.print(F("page: "));
        Serial.println(page);
        Serial.print(F("cursor: "));
        Serial.println(cursor);
        if (page != old_page) {
            show_page();
        }
        if (cursor != old_cursor) {
            update_cursor();
            on_cursor_move();
        }
        c = keypad->waitForKey();
        old_page = page;
        old_cursor = cursor;
        switch (c) {
        case '0':
            on_key_0();
            break;
        case '5':
            on_key_5();
            break;
        case 'F':
            on_key_F();
            break;
        case '2':
            move_cursor(c_UP);
            break;
        case '4':
            move_cursor(c_LEFT);
            break;
        case '6':
            move_cursor(c_RIGHT);
            break;
        case '8':
            move_cursor(c_DOWN);
            break;
        }
        if (page == last_page) {
            cursor = MIN(cursor, last_cursor);
        }
    }
    Serial.println(F("------- finished Menu::begin() --------"));
}

void Menu::move_cursor(cursor_move m) {
    switch (m) {
    case c_UP:
        if (cursor < cols) {
            page = MOD(page-1, last_page+1);
            cursor += cols*(rows-1);
        } else {
            cursor -= cols;
        }
        break;
    case c_RIGHT:
        if (cursor == cols*rows -1 || (page==last_page && cursor == last_cursor)) {
            page = MOD(page+1, last_page+1);
            cursor = 0;
        } else {
            cursor++;
        }
        break;
    case c_DOWN:
        if (cursor/cols == rows-1 || (page==last_page && cursor == last_cursor)) {
            page = MOD(page+1, last_page+1);
            cursor = cursor % cols;
        } else {
            if (page == last_page && cursor+cols > last_cursor) {
                cursor = cursor % cols;
            } else {
                cursor += cols;
            }
        }
        break;
    case c_LEFT:
        if (cursor == 0) {
            page = MOD(page-1, last_page+1);
            cursor = cols*rows - 1;
        } else {
            cursor--;
        }
        break;
    }
}

void Menu::update_cursor() {
    oled->show(x_step*(old_cursor%cols), old_cursor/cols, (x_step-1-1)+x_step*(old_cursor%cols), old_cursor/cols, false);
    oled->show(x_step*(cursor%cols), cursor/cols, (x_step-1-1)+x_step*(cursor%cols), cursor/cols, true);
}

void Menu::beep(uint8_t n) {
    for (; n>0; n--) {
        analogWrite(buzzer_pin, 127);
        delay(20);
        analogWrite(buzzer_pin, 0);
        delay(20);
    }
}



////////////////////////////////////////////////////////////////////
// ROM_Menu
////////////////////////////////////////////////////////////////////

ROM_Menu::ROM_Menu(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin, Hex_Editor *hex_editor) :
    Menu(cols, rows, oled, keypad, eeprom, cpu, buzzer_pin),
    hex_editor(hex_editor)
{
    roms = (CHIP8_rom*) buffer;
}

void ROM_Menu::on_key_5() {
    // start ROM
    oled->set_mode(RAW_MODE);
    oled->clear();
    oled->show();
    cpu->begin();
    cpu->load(roms[cursor].address, roms[cursor].size);
    beep(3);
    cpu->run();
    // cpu->run() will probably not return
    oled->clear();
    oled->set_mode(ASCII_MODE);
}

void ROM_Menu::on_key_0() {
    // edit ROM
    oled->clear();
    oled->show();
    beep(2);
    hex_editor->begin(roms[cursor].address, roms[cursor].size);
    beep(2);
    show_page();
    update_cursor();
}

void ROM_Menu::on_key_F() {
    // upload/input new ROM
    new_rom();
}

void ROM_Menu::new_rom() {
    uint16_t new_rom_start = eeprom_offset;
    uint16_t address;
    uint8_t i;
    uint16_t size = 0;
    char name[9] = {0};
    char c;

    for (i=0; i<item_count; i++) {
        new_rom_start += eeprom->read(new_rom_start, (uint8_t*)&size, 2);
        new_rom_start += eeprom->read(new_rom_start, (uint8_t*)name, 8);
        new_rom_start += size;
    }
    // skipped all saved roms
    address = new_rom_start + 2 + 8;
    beep(5);
    oled->clear();
    oled->drawStringLine(0, "    New CHIP8 ROM        ");
    oled->drawStringLine(2, "1 Input over Serial      ");
    oled->drawStringLine(4, "2 Input over Keypad      ");
    oled->show();
    oled->show(0,0, 24,0, true);
    delay(100);
    while (1) {
        c = keypad->waitForKey();
        if (c == '1' || c == '2') {
            break;
        }
        delay(500);
    }
    oled->clear();
    oled->show();
    if (c == '1') {
        beep(1);
        uint8_t buffer[64];
        uint8_t byte_count;
        bool stopped = false;
        String s;
        // input over Serial
        s = String("SEND ROM");
        Serial.println(s);
        oled->drawStringLine(0, s.c_str());
        oled->show();
        size = 0;
        while(!stopped) {
            byte_count = 0;
            while (byte_count < 64) {
                // receive single bytes until page full or interrupt

                c = keypad->getKey();
                if (c == '0') {
                    stopped = true;
                    break;
                }
                if (Serial.available()) {
                    buffer[byte_count++] = Serial.read();
                }
            }
            if (byte_count) {
                // ACK
                eeprom->write(address, buffer, byte_count);
                beep(1);
                address += byte_count;
                size += byte_count;
            }
        }
        beep(3);
        while(Serial.available()) {
            // flush input
            Serial.read();
        }
        s = String("NAME?");
        Serial.println(s);
        oled->drawStringLine(1, s.c_str());
        oled->show();
        i=0;
        while (i<8) {
            if (Serial.available()) {
                c = Serial.read();
                if (c == '\n' || c == '\r' || c == ' ') {
                    for (uint8_t j=i; j<8; j++) {
                        name[j] = 0;
                    }
                    i=8;
                } else {
                    name[i++] = c;
                }
            }
        }
        size += 32;
        size = size - (size%64) + 64;
        oled->clear();
        s = String("Write ");
        s += String(size);
        s += " B?";
        Serial.println(s);
        oled->drawStringLine(0, s.c_str());
        s = String("AT 0x");
        s += String(new_rom_start+2+8, HEX);
        Serial.println(s);
        oled->drawStringLine(2, s.c_str());
        oled->show();
        while (1) {
            c = keypad->waitForKey();
            if (c == '5') {
                eeprom->write(new_rom_start, (uint8_t*)&size, 2);
                eeprom->write(new_rom_start+2, (uint8_t*)name, 8);
                item_count += 1;
                Serial.print(F("Incrementing rom_count to "));
                Serial.println(item_count);
                eeprom->write(0, item_count);
                delay(10);
                if (eeprom->read(0) != item_count) {
                    Serial.println(F(" BAD"));
                }
                break;
            } else if (c == 'F') {
                break;
            }
        }
    } else {
        beep(2);
        // input over keypad, i.e. hex_editor
    }

    page = 0;
    cursor = 0;
    old_page = 1;
    old_cursor = 1;
    last_page = (item_count-1)/(cols*rows);
    last_cursor = (item_count-1)%(cols*rows);

    oled->clear();
    oled->show();
    show_page();
    // round up size and store at new_rom_start
    // ask for name
}

void ROM_Menu::on_cursor_move() {
    // maybe show index of rom?
}

void ROM_Menu::show_page() {
    uint8_t i;
    uint8_t start_index = page*cols*rows;
    uint8_t count = page==last_page ? last_cursor+1 : cols*rows;
    char name[9] = {0};
    uint16_t address = eeprom_offset;
    uint16_t size;
    
    memset((uint8_t*)roms, 0, sizeof(CHIP8_rom)*cols*rows);

    // skip pages of ROMS
    for (i=0; i<start_index; i++) {
        address += eeprom->read(address, (uint8_t*)&size, 2);
        address += eeprom->read(address, (uint8_t*)name, 8);
        address += size;
    }
    // address now points to ROM number <start_index>
    oled->clear();
    for (i=0; i<count; i++) {
        address += eeprom->read(address, (uint8_t*)&(roms[i].size), 2);
        address += eeprom->read(address, (uint8_t*)name, 8);
        roms[i].address = address;
        address += roms[i].size;

        oled->drawString(x_step*(i%cols), i/cols, name);
    }
    oled->show();
}

void ROM_Menu::show_info() {
    oled->clear();
    oled->drawStringLine(0, " INSTALLED ROMS");
    oled->drawStringLine(2, "5  START ROM");
    oled->drawStringLine(4, "0  EDIT ROM");
    oled->drawStringLine(6, "F  NEW ROM");
    oled->drawStringLine(8, "            PRESS ANY KEY");
    oled->show();
    oled->show(0,0, 24,0, true);
    keypad->waitForKey();
}

void ROM_Menu::empty_menu() {
    char c;
    oled->clear();
    oled->drawStringLine(0, "     NO ROMS");
    oled->drawStringLine(2, "F  NEW ROM");
    oled->show();
    oled->show(0,0, 24,0, true);

    c = keypad->waitForKey();
    if (c == 'F') {
        new_rom();
    }
}


////////////////////////////////////////////////////////////////////
// Hex_Editor
////////////////////////////////////////////////////////////////////

Hex_Editor::Hex_Editor(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin) :
    Menu(cols, rows, oled, keypad, eeprom, cpu, buzzer_pin)
{
}

void Hex_Editor::on_key_5() {
    // change value in buffer
    uint8_t new_value;
    beep(1);
    new_value = from_hex(keypad->waitForKey());
    oled->drawChar(x_step*(cursor%cols), cursor/cols, to_hex(new_value&0xF));
    oled->show(x_step*(cursor%cols), cursor/cols, x_step*(cursor%cols), cursor/cols);

    new_value <<= 4;
    new_value |= from_hex(keypad->waitForKey());
    oled->drawChar(1+x_step*(cursor%cols), cursor/cols, to_hex(new_value&0xF));
    oled->show(1+x_step*(cursor%cols), cursor/cols, 1+x_step*(cursor%cols), cursor/cols);
    buffer[cursor] = new_value;
    modified_page = true;
    update_cursor();
    beep(1);
}

void Hex_Editor::on_key_0() {
    // save page
    eeprom->write(eeprom_offset+page*cols*rows, buffer, page==last_page ? last_cursor+1 : rows*cols);
    beep(2);
    modified_page = false;
}

void Hex_Editor::on_key_F() {
    // exit Hex_Editor without saving
    stop = true;
}

void Hex_Editor::on_cursor_move() {
    // show current address with offset!
    char addr[4];
    hex_string(HEX_EDITOR_OFFSET+page*cols*rows+cursor, addr);
    oled->drawString(21, 8, addr, 4);
    oled->show(21, 8, 24, 8, true);
}

void Hex_Editor::show_page() {
    uint8_t i;
    uint8_t len = page==last_page ? last_cursor+1 : rows*cols;
    if (modified_page) {
        // save old page
        eeprom->write(eeprom_offset+old_page*cols*rows, buffer, old_page==last_page ? last_cursor+1 : rows*cols);
    }
    modified_page = false;
    oled->clear();
    memset(buffer, 0, len);
    // read 64 B from eeprom
    eeprom->read(eeprom_offset+page*64, buffer, len);
    for (i=0; i<len; i++) {
        oled->drawChar(x_step*(i%cols), i/cols, to_hex(buffer[i]>>4));
        oled->drawChar(1+x_step*(i%cols), i/cols, to_hex(buffer[i]&0xF));

        oled->drawChar(2+x_step*(i%cols), i/cols, ' ');
    }
    oled->show();
}

void Hex_Editor::show_info() {
    oled->clear();
    oled->drawStringLine(0, " HEX EDITOR");
    oled->drawStringLine(2, "5  EDIT BYTE");
    oled->drawStringLine(4, "0  SAVE PAGE");
    oled->drawStringLine(6, "F  EXIT");
    oled->drawStringLine(8, "            PRESS ANY KEY");
    oled->show();
    oled->show(0,0, 24,0, true);
    keypad->waitForKey();
}

void Hex_Editor::empty_menu() {
    oled->clear();
    oled->drawStringLine(0, "     NO MEMORY");
    oled->drawStringLine(8, "            PRESS ANY KEY");
    oled->show();
    oled->show(0,0, 24,0, true);
    keypad->waitForKey();
    stop = true;
}

