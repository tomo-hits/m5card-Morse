#include <Arduino.h>
#include <M5Cardputer.h>

namespace {

constexpr int kToneHz = 700;
constexpr int kWordsPerMinute = 15;
constexpr int kDotMs = 1200 / kWordsPerMinute;

String inputText = "SOS";
String morseText;
bool playing = false;
bool japaneseInput = true;
String romajiBuffer;
String serialText;

struct AsciiMorseEntry {
  char c;
  const char* code;
};

struct WabunMorseEntry {
  uint32_t codepoint;
  const char* code;
};

struct RomajiEntry {
  const char* romaji;
  const char* kana;
};

const AsciiMorseEntry kAsciiMorseTable[] = {
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

const WabunMorseEntry kWabunMorseTable[] = {
    {0x3042, "--.--"},  {0x3044, ".-"},     {0x3046, "..-"},
    {0x3048, "-.---"},  {0x304A, ".-..."},  {0x304B, ".-.."},
    {0x304D, "-.-.."},  {0x304F, "...-"},   {0x3051, "-.--"},
    {0x3053, "----"},   {0x3055, "-.-.-"},  {0x3057, "--.-."},
    {0x3059, "---.-"},  {0x305B, ".---."},  {0x305D, "---."},
    {0x305F, "-."},     {0x3061, "..-."},   {0x3064, ".--."},
    {0x3066, ".-.--"},  {0x3068, "..-.."},  {0x306A, ".-."},
    {0x306B, "-.-."},   {0x306C, "...."},   {0x306D, "--.-"},
    {0x306E, "..--"},   {0x306F, "-..."},   {0x3072, "--..-"},
    {0x3075, "--.."},   {0x3078, "."},      {0x307B, "-.."},
    {0x307E, "-..-"},   {0x307F, "..-.-"},  {0x3080, "-"},
    {0x3081, "-...-"},  {0x3082, "-..-."},  {0x3084, ".--"},
    {0x3086, "-..--"},  {0x3088, "--"},     {0x3089, "..."},
    {0x308A, "--."},    {0x308B, "-.--."},  {0x308C, "---"},
    {0x308D, ".-.-"},   {0x308F, "-.-"},    {0x3090, ".-..-"},
    {0x3091, ".--.."},  {0x3092, ".---"},   {0x3093, ".-.-."},
    {0x309B, ".."},     {0x3099, ".."},     {0x309C, "..--."},
    {0x309A, "..--."},  {0x30FC, ".--.-"},  {0x3001, ".-.-.-"},
    {0x3002, ".-.-.-"}, {0xFF08, "-.--.-"}, {0xFF09, ".-..-."},
};

const WabunMorseEntry kSmallKanaTable[] = {
    {0x3041, "--.--"}, {0x3043, ".-"},    {0x3045, "..-"},
    {0x3047, "-.---"}, {0x3049, ".-..."}, {0x3063, ".--."},
    {0x3083, ".--"},   {0x3085, "-..--"}, {0x3087, "--"},
    {0x308E, "-.-"},   {0x3095, ".-.."},  {0x3096, "-.--"},
};

const WabunMorseEntry kVoicedKanaTable[] = {
    {0x304C, ".-.. .."},    {0x304E, "-.-.. .."},
    {0x3050, "...- .."},    {0x3052, "-.-- .."},
    {0x3054, "---- .."},    {0x3056, "-.-.- .."},
    {0x3058, "--.-. .."},   {0x305A, "---.- .."},
    {0x305C, ".---. .."},   {0x305E, "---. .."},
    {0x3060, "-. .."},      {0x3062, "..-. .."},
    {0x3065, ".--. .."},    {0x3067, ".-.-- .."},
    {0x3069, "..-.. .."},   {0x3070, "-... .."},
    {0x3073, "--..- .."},   {0x3076, "--.. .."},
    {0x3079, ". .."},       {0x307C, "-.. .."},
    {0x3094, "..- .."},     {0x3071, "-... ..--."},
    {0x3074, "--..- ..--."},{0x3077, "--.. ..--."},
    {0x307A, ". ..--."},    {0x307D, "-.. ..--."},
};

const RomajiEntry kRomajiTable[] = {
    {"kyo", "きょ"}, {"kyu", "きゅ"}, {"kya", "きゃ"},
    {"gyo", "ぎょ"}, {"gyu", "ぎゅ"}, {"gya", "ぎゃ"},
    {"sho", "しょ"}, {"shu", "しゅ"}, {"sha", "しゃ"},
    {"syo", "しょ"}, {"syu", "しゅ"}, {"sya", "しゃ"},
    {"jo", "じょ"},  {"ju", "じゅ"},  {"ja", "じゃ"},
    {"jyo", "じょ"}, {"jyu", "じゅ"}, {"jya", "じゃ"},
    {"cho", "ちょ"}, {"chu", "ちゅ"}, {"cha", "ちゃ"},
    {"tyo", "ちょ"}, {"tyu", "ちゅ"}, {"tya", "ちゃ"},
    {"nyo", "にょ"}, {"nyu", "にゅ"}, {"nya", "にゃ"},
    {"hyo", "ひょ"}, {"hyu", "ひゅ"}, {"hya", "ひゃ"},
    {"byo", "びょ"}, {"byu", "びゅ"}, {"bya", "びゃ"},
    {"pyo", "ぴょ"}, {"pyu", "ぴゅ"}, {"pya", "ぴゃ"},
    {"myo", "みょ"}, {"myu", "みゅ"}, {"mya", "みゃ"},
    {"ryo", "りょ"}, {"ryu", "りゅ"}, {"rya", "りゃ"},
    {"fa", "ふぁ"},  {"fi", "ふぃ"},  {"fe", "ふぇ"},  {"fo", "ふぉ"},
    {"va", "ゔぁ"},  {"vi", "ゔぃ"},  {"vu", "ゔ"},    {"ve", "ゔぇ"},
    {"vo", "ゔぉ"},  {"shi", "し"},   {"chi", "ち"},   {"tsu", "つ"},
    {"fu", "ふ"},    {"ji", "じ"},    {"ka", "か"},    {"ki", "き"},
    {"ku", "く"},    {"ke", "け"},    {"ko", "こ"},    {"ga", "が"},
    {"gi", "ぎ"},    {"gu", "ぐ"},    {"ge", "げ"},    {"go", "ご"},
    {"sa", "さ"},    {"si", "し"},    {"su", "す"},    {"se", "せ"},
    {"so", "そ"},    {"za", "ざ"},    {"zi", "じ"},    {"zu", "ず"},
    {"ze", "ぜ"},    {"zo", "ぞ"},    {"ta", "た"},    {"ti", "ち"},
    {"tu", "つ"},    {"te", "て"},    {"to", "と"},    {"da", "だ"},
    {"di", "ぢ"},    {"du", "づ"},    {"de", "で"},    {"do", "ど"},
    {"na", "な"},    {"ni", "に"},    {"nu", "ぬ"},    {"ne", "ね"},
    {"no", "の"},    {"ha", "は"},    {"hi", "ひ"},    {"hu", "ふ"},
    {"he", "へ"},    {"ho", "ほ"},    {"ba", "ば"},    {"bi", "び"},
    {"bu", "ぶ"},    {"be", "べ"},    {"bo", "ぼ"},    {"pa", "ぱ"},
    {"pi", "ぴ"},    {"pu", "ぷ"},    {"pe", "ぺ"},    {"po", "ぽ"},
    {"ma", "ま"},    {"mi", "み"},    {"mu", "む"},    {"me", "め"},
    {"mo", "も"},    {"ya", "や"},    {"yu", "ゆ"},    {"yo", "よ"},
    {"ra", "ら"},    {"ri", "り"},    {"ru", "る"},    {"re", "れ"},
    {"ro", "ろ"},    {"wa", "わ"},    {"wo", "を"},    {"xa", "ぁ"},
    {"xi", "ぃ"},    {"xu", "ぅ"},    {"xe", "ぇ"},    {"xo", "ぉ"},
    {"la", "ぁ"},    {"li", "ぃ"},    {"lu", "ぅ"},    {"le", "ぇ"},
    {"lo", "ぉ"},    {"xtu", "っ"},   {"ltu", "っ"},   {"xya", "ゃ"},
    {"xyu", "ゅ"},   {"xyo", "ょ"},   {"lya", "ゃ"},   {"lyu", "ゅ"},
    {"lyo", "ょ"},   {"a", "あ"},     {"i", "い"},     {"u", "う"},
    {"e", "え"},     {"o", "お"},     {"n", "ん"},
};

const char* findAsciiMorseCode(char c) {
  c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  for (const auto& entry : kAsciiMorseTable) {
    if (entry.c == c) {
      return entry.code;
    }
  }
  return nullptr;
}

const char* findCodepointMorseCode(uint32_t codepoint,
                                   const WabunMorseEntry* table,
                                   size_t tableSize) {
  for (size_t i = 0; i < tableSize; ++i) {
    if (table[i].codepoint == codepoint) {
      return table[i].code;
    }
  }
  return nullptr;
}

uint32_t normalizeKanaCodepoint(uint32_t codepoint) {
  if (codepoint >= 0x30A1 && codepoint <= 0x30F6) {
    return codepoint - 0x60;
  }
  return codepoint;
}

const char* findWabunMorseCode(uint32_t codepoint) {
  codepoint = normalizeKanaCodepoint(codepoint);

  const char* code = findCodepointMorseCode(
      codepoint, kWabunMorseTable,
      sizeof(kWabunMorseTable) / sizeof(kWabunMorseTable[0]));
  if (code != nullptr) {
    return code;
  }

  code = findCodepointMorseCode(
      codepoint, kSmallKanaTable,
      sizeof(kSmallKanaTable) / sizeof(kSmallKanaTable[0]));
  if (code != nullptr) {
    return code;
  }

  return findCodepointMorseCode(
      codepoint, kVoicedKanaTable,
      sizeof(kVoicedKanaTable) / sizeof(kVoicedKanaTable[0]));
}

bool readUtf8Codepoint(const String& text, size_t& index, uint32_t& codepoint,
                       String* glyph = nullptr) {
  if (index >= text.length()) {
    return false;
  }

  const uint8_t first = static_cast<uint8_t>(text[index]);
  size_t length = 1;
  if ((first & 0x80) == 0) {
    codepoint = first;
  } else if ((first & 0xE0) == 0xC0) {
    codepoint = first & 0x1F;
    length = 2;
  } else if ((first & 0xF0) == 0xE0) {
    codepoint = first & 0x0F;
    length = 3;
  } else if ((first & 0xF8) == 0xF0) {
    codepoint = first & 0x07;
    length = 4;
  } else {
    index++;
    return false;
  }

  if (index + length > text.length()) {
    index = text.length();
    return false;
  }

  for (size_t i = 1; i < length; ++i) {
    const uint8_t next = static_cast<uint8_t>(text[index + i]);
    if ((next & 0xC0) != 0x80) {
      index++;
      return false;
    }
    codepoint = (codepoint << 6) | (next & 0x3F);
  }

  if (glyph != nullptr) {
    *glyph = text.substring(index, index + length);
  }
  index += length;
  return true;
}

void appendMorseCode(String& result, const char* code) {
  if (result.length() > 0 && !result.endsWith(" / ")) {
    result += ' ';
  }
  result += code;
}

bool isVowel(char c) {
  return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o';
}

bool isConsonant(char c) {
  return c >= 'a' && c <= 'z' && !isVowel(c);
}

String lowerAscii(const String& text) {
  String result;
  for (size_t i = 0; i < text.length(); ++i) {
    result += static_cast<char>(tolower(static_cast<unsigned char>(text[i])));
  }
  return result;
}

const RomajiEntry* findRomajiPrefix(const String& buffer) {
  const RomajiEntry* best = nullptr;
  size_t bestLength = 0;

  for (const auto& entry : kRomajiTable) {
    const size_t length = strlen(entry.romaji);
    if (length > bestLength && buffer.startsWith(entry.romaji)) {
      best = &entry;
      bestLength = length;
    }
  }

  return best;
}

bool hasRomajiCandidate(const String& buffer) {
  for (const auto& entry : kRomajiTable) {
    if (String(entry.romaji).startsWith(buffer)) {
      return true;
    }
  }
  return false;
}

void processRomajiBuffer(bool force = false) {
  romajiBuffer = lowerAscii(romajiBuffer);

  while (!romajiBuffer.isEmpty()) {
    if (romajiBuffer.length() >= 2 && romajiBuffer[0] == romajiBuffer[1] &&
        isConsonant(romajiBuffer[0]) && romajiBuffer[0] != 'n') {
      inputText += "っ";
      romajiBuffer.remove(0, 1);
      continue;
    }

    if (romajiBuffer.startsWith("n'") || romajiBuffer.startsWith("nn")) {
      inputText += "ん";
      romajiBuffer.remove(0, 2);
      continue;
    }

    const RomajiEntry* entry = findRomajiPrefix(romajiBuffer);
    if (entry != nullptr) {
      if (strcmp(entry->romaji, "n") == 0 && !force &&
          romajiBuffer.length() == 1) {
        break;
      }
      inputText += entry->kana;
      romajiBuffer.remove(0, strlen(entry->romaji));
      continue;
    }

    if (romajiBuffer[0] == 'n') {
      if (force || romajiBuffer.length() >= 2) {
        const char next = romajiBuffer.length() >= 2 ? romajiBuffer[1] : '\0';
        if (next == '\0' || (!isVowel(next) && next != 'y')) {
          inputText += "ん";
          romajiBuffer.remove(0, 1);
          continue;
        }
      }
      break;
    }

    if (!force && hasRomajiCandidate(romajiBuffer)) {
      break;
    }

    inputText += romajiBuffer[0];
    romajiBuffer.remove(0, 1);
  }
}

String toMorse(const String& text) {
  String result;
  bool lastWasSpace = false;

  for (size_t i = 0; i < text.length();) {
    uint32_t codepoint = 0;
    if (!readUtf8Codepoint(text, i, codepoint)) {
      continue;
    }

    if (codepoint <= 0x7F && isspace(static_cast<unsigned char>(codepoint))) {
      if (!lastWasSpace && result.length() > 0) {
        result += " / ";
      }
      lastWasSpace = true;
      continue;
    }

    const char* code = nullptr;
    if (codepoint <= 0x7F) {
      code = findAsciiMorseCode(static_cast<char>(codepoint));
    } else {
      code = findWabunMorseCode(codepoint);
    }

    if (code == nullptr) {
      continue;
    }

    appendMorseCode(result, code);
    lastWasSpace = false;
  }

  return result;
}

void drawWrappedText(const String& text, int32_t x, int32_t y, int32_t width,
                     int32_t lineHeight, uint16_t color, size_t maxChars) {
  M5Cardputer.Display.setTextColor(color, BLACK);

  String line;
  size_t printed = 0;
  for (size_t i = 0; i < text.length() && printed < maxChars;) {
    uint32_t codepoint = 0;
    String glyph;
    if (!readUtf8Codepoint(text, i, codepoint, &glyph)) {
      continue;
    }

    line += glyph;
    if (M5Cardputer.Display.textWidth(line) > width || i >= text.length() ||
        printed == maxChars - 1) {
      if (M5Cardputer.Display.textWidth(line) > width && line.length() > 1) {
        line.remove(line.length() - glyph.length());
        M5Cardputer.Display.drawString(line, x, y);
        y += lineHeight;
        line = glyph;
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
  String displayText = inputText;
  if (!romajiBuffer.isEmpty()) {
    displayText += '[';
    displayText += romajiBuffer;
    displayText += ']';
  }

  auto& display = M5Cardputer.Display;
  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setFont(&fonts::efontJA_12);
  display.setTextColor(GREEN, BLACK);
  display.drawString("Morse Cardputer", 6, 4);
  display.setTextColor(japaneseInput ? CYAN : LIGHTGREY, BLACK);
  display.drawString(japaneseInput ? "JP" : "ABC", display.width() - 30, 4);

  display.setTextColor(WHITE, BLACK);
  display.drawString("TEXT", 6, 24);
  display.drawRoundRect(4, 40, display.width() - 8, 36, 3, DARKGREY);
  drawWrappedText(displayText, 9, 45, display.width() - 18, 14, WHITE, 64);

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
  if (inputText.length() >= 128) {
    return;
  }
  if (c >= 32 && c <= 126) {
    inputText += c;
  }
}

void appendJapaneseInput(char c) {
  if (inputText.length() + romajiBuffer.length() >= 128) {
    return;
  }

  if (isalpha(static_cast<unsigned char>(c)) || c == '\'') {
    romajiBuffer += c;
    processRomajiBuffer(false);
    return;
  }

  processRomajiBuffer(true);
  if (c == '-') {
    inputText += "ー";
  } else if (c == ',' || c == '<') {
    inputText += "、";
  } else if (c == '.' || c == '>') {
    inputText += "。";
  } else if (c == '(' || c == '[') {
    inputText += "（";
  } else if (c == ')' || c == ']') {
    inputText += "）";
  } else if (c >= '0' && c <= '9') {
    inputText += c;
  } else if (c >= 32 && c <= 126) {
    inputText += c;
  }
}

void removeLastInputChar() {
  if (!romajiBuffer.isEmpty()) {
    romajiBuffer.remove(romajiBuffer.length() - 1);
    return;
  }

  if (inputText.isEmpty()) {
    return;
  }

  size_t index = inputText.length() - 1;
  while (index > 0 &&
         (static_cast<uint8_t>(inputText[index]) & 0xC0) == 0x80) {
    index--;
  }
  inputText.remove(index);
}

void handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  bool changed = false;

  if (status.tab) {
    processRomajiBuffer(true);
    japaneseInput = !japaneseInput;
    changed = true;
  }

  for (auto c : status.word) {
    if (japaneseInput) {
      appendJapaneseInput(c);
    } else {
      appendPrintable(c);
    }
    changed = true;
  }

  if (status.space) {
    processRomajiBuffer(true);
    if (!inputText.endsWith(" ")) {
      inputText += ' ';
    }
    changed = true;
  }

  if (status.del && (!romajiBuffer.isEmpty() || inputText.length() > 0)) {
    removeLastInputChar();
    changed = true;
  }

  if (status.enter) {
    processRomajiBuffer(true);
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

void handleSerialInput() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      serialText.trim();
      if (!serialText.isEmpty()) {
        romajiBuffer = "";
        inputText = serialText.substring(0, 128);
        drawScreen("SERIAL TEXT READY");
      }
      serialText = "";
      continue;
    }
    if (serialText.length() < 128) {
      serialText += c;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);

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
  handleSerialInput();
  delay(10);
}
