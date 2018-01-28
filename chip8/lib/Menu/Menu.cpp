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
    Serial.print("Menu::begin(0x");
    Serial.print(eeprom_offset, HEX);
    Serial.print(", ");
    Serial.print(item_count);
    Serial.println(")");
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

    while (1) {
        Serial.println();
        Serial.print("page: ");
        Serial.println(page);
        Serial.print("cursor: ");
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
        if (stop) {
            break;
        }
    }
    Serial.println("------- finished Menu::begin() --------");
}

void Menu::move_cursor(cursor_move m) {
    Serial.print("Menu::move_cursor(");
    switch (m) {
    case c_UP:
        Serial.println("c_UP)");
        if (cursor < cols) {
            page = MOD(page-1, last_page+1);
            cursor += cols*(rows-1);
        } else {
            cursor -= cols;
        }
        break;
    case c_RIGHT:
        Serial.println("c_RIGHT)");
        if (cursor == cols*rows -1 || (page==last_page && cursor == last_cursor)) {
            page = MOD(page+1, last_page+1);
            cursor = 0;
        } else {
            cursor++;
        }
        break;
    case c_DOWN:
        Serial.println("c_DOWN)");
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
        Serial.println("c_LEFT)");
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
    Serial.println("ROM_Menu::on_key_5()");
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
    Serial.println("ROM_Menu::on_key_0()");
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
    Serial.println("ROM_Menu::on_key_F()");
    // upload/input new ROM
    beep(5);
}

void ROM_Menu::on_cursor_move() {
    // maybe show index of rom?
}

void ROM_Menu::show_page() {
    uint8_t i;
    uint8_t start_index = page*cols*rows;
    uint8_t count = page==last_page ? last_cursor+1 : cols*rows;
    char name[8];
    uint8_t address = eeprom_offset;
    uint16_t size;

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

        oled->drawString(x_step*(i%cols), i/cols, String(name));
    }
    oled->show();
}



////////////////////////////////////////////////////////////////////
// Hex_Editor
////////////////////////////////////////////////////////////////////

Hex_Editor::Hex_Editor(uint8_t cols, uint8_t rows, Adafruit_SSD1306 *oled, Keypad *keypad, I2C_EEPROM *eeprom, CPU *cpu, uint8_t buzzer_pin) :
    Menu(cols, rows, oled, keypad, eeprom, cpu, buzzer_pin)
{
}

void Hex_Editor::on_key_5() {
    Serial.println("Hex_Editor::on_key_5()");
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
    Serial.println("Hex_Editor::on_key_0()");
    // save page
    eeprom->write(eeprom_offset+page*cols*rows, buffer, page==last_page ? last_cursor+1 : rows*cols);
    beep(2);
    modified_page = false;
}

void Hex_Editor::on_key_F() {
    Serial.println("Hex_Editor::on_key_F()");
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
