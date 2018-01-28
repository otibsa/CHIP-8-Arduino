#include <Menu.h>

Menu::Menu(uint8_t cols, uint8_t rows) : cols(cols), rows(rows) {
}

void Menu::begin() {
    key_5();
}

void Menu::move_cursor(cursor_move m) {
    switch (m) {
    case c_UP:
        Serial.println("c_UP");
        break;
    case c_RIGHT:
        Serial.println("c_RIGHT");
        break;
    case c_DOWN:
        Serial.println("c_DOWN");
        break;
    case c_LEFT:
        Serial.println("c_LEFT");
        break;
    }
}

ROM_Menu::ROM_Menu(uint8_t cols, uint8_t rows) : Menu(cols, rows) {
    roms = (CHIP8_rom*) buffer;
}

void ROM_Menu::key_5() {
    Serial.println("ROM_Menu::key_5()");
}

void ROM_Menu::key_0() {
    Serial.println("ROM_Menu::key_0()");
}

void ROM_Menu::key_F() {
    Serial.println("ROM_Menu::key_F()");
}



Hex_Editor::Hex_Editor(uint8_t cols, uint8_t rows) : Menu(cols, rows) {
}

void Hex_Editor::key_5() {
    Serial.println("Hex_Editor::key_5()");
}

void Hex_Editor::key_0() {
    Serial.println("Hex_Editor::key_0()");
}

void Hex_Editor::key_F() {
    Serial.println("Hex_Editor::key_F()");
}
