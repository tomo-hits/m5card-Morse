#include <Arduino.h>
#include <M5Cardputer.h>

namespace {

constexpr int kToneHz = 700;
constexpr int kWordsPerMinute = 15;
constexpr int kDotMs = 1200 / kWordsPerMinute;

String inputText = "SOS";
String morseText;
bool playing = false;

struct MorseEntry {
  char c;
  const char* code;
};

const MorseEntry kMorseTable[] = {
    {'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},
    {'E', "."},     {'F', "..-."},  {'G', "--."},   {'H', "...."},
    {'I', ".."},    {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
    {'M', "--"},    {'N', "-."},    {'O', "---"},   {'P', ".--."},
    {'Q', "--.-"},  {'R', ".-."},   {'S', "..."},   {'T', "-"},
    {'U', "..-"},   {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},  {'0', "-----"}, {'1', ".----"},
    {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
    {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
    {'.', ".-.-.-"},{',', "--..--"},{'?', "..--.."},{'!', "-.-.--"},
    {'/', "-..-."}, {'-', "-....-"},{'=', "-...-"},{'+', ".-.-."},
    {'@', ".--.-."},{'(', "-.--."}, {')', "-.--.-"},{':', "---..."},
};

const char* findMorseCode(char c) {
  c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  for (const auto& entry : kMorseTable) {
    if (entry.c == c) {
      return entry.code;
    }
  }
  return nullptr;
}

String toMorse(const String& text) {
  String result;
  bool lastWasSpace = false;

  for (size_t i = 0; i < text.length(); ++i) {
    const char c = text[i];
    if (isspace(static_cast<unsigned char>(c))) {
      if (!lastWasSpace && result.length() > 0) {
        result += " / ";
      }
      lastWasSpace = true;
      continue;
    }

    const char* code = findMorseCode(c);
    if (code == nullptr) {
      continue;
    }

    if (result.length() > 0 && !result.endsWith(" / ")) {
      result += ' ';
    }
    result += code;
    lastWasSpace = false;
  }

  return result;
}

void drawWrappedText(const String& text, int32_t x, int32_t y, int32_t width,
                     int32_t lineHeight, uint16_t color, size_t maxChars) {
  M5Cardputer.Display.setTextColor(color, BLACK);

  String line;
  size_t printed = 0;
  for (size_t i = 0; i < text.length() && printed < maxChars; ++i) {
    line += text[i];
    if (M5Cardputer.Display.textWidth(line) > width || i == text.length() - 1 ||
        printed == maxChars - 1) {
      if (M5Cardputer.Display.textWidth(line) > width && line.length() > 1) {
        const char overflow = line[line.length() - 1];
        line.remove(line.length() - 1);
        M5Cardputer.Display.drawString(line, x, y);
        y += lineHeight;
        line = overflow;
      } else {
        M5Cardputer.Display.drawString(line, x, y);
        y += lineHeight;
        line = "";
      }
    }
    printed++;
  }
}

void drawScreen(const char* status = "ENTER: play  BKSP: delete") {
  morseText = toMorse(inputText);

  auto& display = M5Cardputer.Display;
  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setFont(&fonts::Font2);
  display.setTextColor(GREEN, BLACK);
  display.drawString("Morse Cardputer", 6, 4);

  display.setTextColor(WHITE, BLACK);
  display.drawString("TEXT", 6, 24);
  display.drawRoundRect(4, 40, display.width() - 8, 36, 3, DARKGREY);
  drawWrappedText(inputText, 9, 45, display.width() - 18, 14, WHITE, 64);

  display.setTextColor(WHITE, BLACK);
  display.drawString("MORSE", 6, 82);
  drawWrappedText(morseText, 9, 100, display.width() - 18, 14, YELLOW, 96);

  display.fillRect(0, display.height() - 14, display.width(), 14, DARKGREY);
  display.setTextColor(playing ? ORANGE : WHITE, DARKGREY);
  display.drawString(status, 6, display.height() - 13);
}

void playSymbol(char symbol) {
  const int duration = (symbol == '-') ? kDotMs * 3 : kDotMs;
  M5Cardputer.Speaker.tone(kToneHz, duration);
  delay(duration);
}

void playMorse() {
  if (morseText.isEmpty()) {
    return;
  }

  playing = true;
  drawScreen("PLAYING...");
  M5Cardputer.Speaker.setVolume(180);

  for (size_t i = 0; i < morseText.length(); ++i) {
    const char symbol = morseText[i];

    if (symbol == '.' || symbol == '-') {
      playSymbol(symbol);
      delay(kDotMs);
    } else if (symbol == '/') {
      delay(kDotMs * 6);
    } else if (symbol == ' ') {
      delay(kDotMs * 2);
    }
  }

  playing = false;
  drawScreen("DONE");
}

void appendPrintable(char c) {
  if (inputText.length() >= 64) {
    return;
  }
  if (c >= 32 && c <= 126) {
    inputText += c;
  }
}

void handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  bool changed = false;

  for (auto c : status.word) {
    appendPrintable(c);
    changed = true;
  }

  if (status.del && inputText.length() > 0) {
    inputText.remove(inputText.length() - 1);
    changed = true;
  }

  if (status.enter) {
    if (inputText.isEmpty()) {
      inputText = "SOS";
      changed = true;
    }
    drawScreen();
    playMorse();
    return;
  }

  if (changed) {
    drawScreen();
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextDatum(top_left);
  M5Cardputer.Speaker.setVolume(180);

  drawScreen("Type text, then press ENTER");
}

void loop() {
  M5Cardputer.update();
  handleKeyboard();
  delay(10);
}
