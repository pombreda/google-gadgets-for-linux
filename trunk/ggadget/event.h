/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_EVENT_H__
#define GGADGET_EVENT_H__

namespace ggadget {

/**
 * Class for holding event information. There are several subclasses for
 * events features in common.
 */
class Event {
 public:
  enum Type {
    EVENT_SIMPLE_RANGE_START = 0,
    EVENT_CANCEL,
    EVENT_CLOSE,
    EVENT_DOCK,
    EVENT_MINIMIZE,
    EVENT_OK,
    EVENT_OPEN,
    EVENT_POPIN,
    EVENT_POPOUT,
    EVENT_RESTORE,
    EVENT_SIZE,
    EVENT_UNDOCK,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_TIMER_TICK,
    EVENT_CHANGE,
    EVENT_SIMPLE_RANGE_END,

    EVENT_MOUSE_RANGE_START = 10000,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DBLCLICK,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_OUT,
    EVENT_MOUSE_OVER,
    EVENT_MOUSE_WHEEL,
    EVENT_MOUSE_RCLICK,
    EVENT_MOUSE_RDBLCLICK,    
    EVENT_MOUSE_RANGE_END,

    EVENT_KEY_RANGE_START = 20000,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_PRESS,
    EVENT_KEY_RANGE_END,

    EVENT_DRAG_RANGE_START = 30000,
    EVENT_DRAG_DROP,
    EVENT_DRAG_OUT,
    EVENT_DRAG_OVER,
    EVENT_DRAG_RANGE_END,

    EVENT_SIZING = 40000,
    EVENT_OPTION_CHANGED,
  };

  explicit Event(Type t) : type_(t) { ASSERT(IsSimpleEvent()); }
  Event(const Event &e) : type_(e.type_) { ASSERT(IsSimpleEvent()); }

  Type GetType() const { return type_; }

  bool IsSimpleEvent() const { return type_ > EVENT_SIMPLE_RANGE_START &&
                                      type_ < EVENT_SIMPLE_RANGE_END; }
  bool IsMouseEvent() const { return type_ > EVENT_MOUSE_RANGE_START &&
                                     type_ < EVENT_MOUSE_RANGE_END; }
  bool IsKeyboardEvent() const { return type_ > EVENT_KEY_RANGE_START &&
                                        type_ < EVENT_KEY_RANGE_END; }
  bool IsDragEvent() const { return type_ > EVENT_DRAG_RANGE_START &&
                                    type_ < EVENT_DRAG_RANGE_END; }

 protected:
  Event(Type t, int dummy) : type_(t) { }

 private:
  Type type_;
};

/**
 * Class representing a mouse event.
 */
class MouseEvent : public Event {
 public:
  enum Button {
    BUTTON_NONE = 0,
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE = 4,
    BUTTON_RIGHT = 2,
    BUTTON_ALL = BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT,
  };

  MouseEvent(Type t, double x, double y, Button button, int wheel_delta)
      : Event(t, 0), x_(x), y_(y), button_(button), wheel_delta_(wheel_delta) {
    ASSERT(IsMouseEvent());
  }

  MouseEvent(const MouseEvent &e)
      : Event(e.GetType(), 0),
        x_(e.x_), y_(e.y_), button_(e.button_), wheel_delta_(e.wheel_delta_) {
    ASSERT(IsMouseEvent());
  }

  double GetX() const { return x_; }
  double GetY() const { return y_; }

  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

  Button GetButton() const { return button_; }
  void SetButton(Button button) { button_ = button; }

  int GetWheelDelta() const { return wheel_delta_; }
  void SetWheelDelta(int wheel_delta) { wheel_delta_ = wheel_delta; }

  static const int kWheelDelta = 120;

 private:
  double x_, y_;
  Button button_;
  int wheel_delta_;
};

/**
 * Class representing a keyboard event.
 */
class KeyboardEvent : public Event {
 public:
  /**
   * Key codes compatible with the Windows version.
   * Most of the following names correspond VK_XXX macros in winuser.h, except
   * for some confusing names.
   *
   * They are only used in @c EVENT_KEY_DOWN and @c EVENT_KEY_UP events.
   * In @c EVENT_KEY_PRESS, the keyCode attribute is the the chararacter code.
   */
  enum KeyCode {
    KEY_CANCEL         = 3,
    KEY_BACK           = 8,
    KEY_TAB            = 9,
    KEY_CLEAR          = 12,
    KEY_RETURN         = 13,
    KEY_SHIFT          = 16,
    KEY_CONTROL        = 17,
    KEY_ALT            = 18,  // VK_MENU in winuser.h.
    KEY_PAUSE          = 19,
    KEY_CAPITAL        = 20,
    KEY_ESCAPE         = 31,
    KEY_SPACE          = 32,
    KEY_PAGE_UP        = 33,  // VK_PRIOR in winuser.h.
    KEY_PAGE_DOWN      = 34,  // VK_NEXT in winuser.h.
    KEY_END            = 35,
    KEY_HOME           = 36,
    KEY_LEFT           = 37,
    KEY_UP             = 38,
    KEY_RIGHT          = 39,
    KEY_DOWN           = 40,
    KEY_SELECT         = 41,
    KEY_PRINT          = 42,
    KEY_EXECUTE        = 43,
    KEY_INSERT         = 45,
    KEY_DELETE         = 46,
    KEY_HELP           = 47,
    // Mixed decimal and hexadecimal to keep consistence with windows.h.
    // 0~9, A-Z and some punctuations are represented in their original ascii.
    KEY_CONTEXT_MENU   = 0x5D,  // VK_APPS in winuser.h.
    KEY_NUMPAD0        = 0x60,
    KEY_NUMPAD1        = 0x61,
    KEY_NUMPAD2        = 0x62,
    KEY_NUMPAD3        = 0x63,
    KEY_NUMPAD4        = 0x64,
    KEY_NUMPAD5        = 0x65,
    KEY_NUMPAD6        = 0x66,
    KEY_NUMPAD7        = 0x67,
    KEY_NUMPAD8        = 0x68,
    KEY_NUMPAD9        = 0x69,
    KEY_MULTIPLY       = 0x6A,
    KEY_ADD            = 0x6B,
    KEY_SEPARATOR      = 0x6C,
    KEY_SUBTRACT       = 0x6D,
    KEY_DECIMAL        = 0x6E,
    KEY_DIVIDE         = 0x6F,
    KEY_F1             = 0x70,
    KEY_F2             = 0x71,
    KEY_F3             = 0x72,
    KEY_F4             = 0x73,
    KEY_F5             = 0x74,
    KEY_F6             = 0x75,
    KEY_F7             = 0x76,
    KEY_F8             = 0x77,
    KEY_F9             = 0x78,
    KEY_F10            = 0x79,
    KEY_F11            = 0x7A,
    KEY_F12            = 0x7B,
    KEY_F13            = 0x7C,
    KEY_F14            = 0x7D,
    KEY_F15            = 0x7E,
    KEY_F16            = 0x7F,
    KEY_F17            = 0x80,
    KEY_F18            = 0x81,
    KEY_F19            = 0x82,
    KEY_F20            = 0x83,
    KEY_F21            = 0x84,
    KEY_F22            = 0x85,
    KEY_F23            = 0x86,
    KEY_F24            = 0x87,
    KEY_NUMLOCK        = 0x90,
    KEY_SCROLL         = 0x91,

    KEY_COLON          = 0xBA,  // VK_OEM_1 in winuser.h, ;: in the keyboard.
    KEY_PLUS           = 0xBB,  // =+ in the keyboard.
    KEY_COMMA          = 0xBC,  // ,< in the keyboard.
    KEY_MINUS          = 0xBD,  // -_ in the keyboard.
    KEY_PERIOD         = 0xBE,  // .> in the keyboard.
    KEY_SLASH          = 0xBF,  // VK_OEM_2 in winuser.h, /? in the keyboard.
    KEY_GRAVE          = 0xC0,  // VK_OEM_3 in winuser.h, `~ in the keyboard.
    KEY_BRACKET_LEFT   = 0xDB,  // VK_OEM_4 in winuser.h, [{ in the keyboard.
    KEY_BACK_SLASH     = 0xDC,  // VK_OEM_5 in winuser.h, \| in the keyboard.
    KEY_BRACKET_RIGHT  = 0xDD,  // VK_OEM_6 in winuser.h, ]} in the keyboard.
    KEY_QUOTE          = 0xDE,  // VK_OEM_7 in winuser.h, '" in the keyboard.
  };

  KeyboardEvent(Type t, unsigned int key_code)
      : Event(t, 0), key_code_(key_code) {
    ASSERT(IsKeyboardEvent());
  };

  KeyboardEvent(const KeyboardEvent &e)
      : Event(e.GetType(), 0), key_code_(e.key_code_) {
    ASSERT(IsKeyboardEvent());
  }

  unsigned int GetKeyCode() const { return key_code_; }
  void SetKeyCode(unsigned int key_code) { key_code_ = key_code; }

 private:
  int key_code_;
};

/**
 * Class representing a drag & drop event.
 */
class DragEvent : public Event {
 public:
  DragEvent(Type t, double x, double y, const char **files)
      : Event(t, 0), x_(x), y_(y), files_(files) {
    ASSERT(IsDragEvent());
  }

  DragEvent(const DragEvent &e)
      : Event(e.GetType(), 0), x_(e.x_), y_(e.y_), files_(e.files_) {
    ASSERT(IsDragEvent());
  }

  double GetX() const { return x_; }
  double GetY() const { return y_; }

  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

  const char **GetFiles() const { return files_; }
  void GetFiles(const char **files) { files_ = files; }

 private:
  double x_, y_;
  const char **files_;
};

/**
 * Class representing a sizing event.
 */
class SizingEvent : public Event {
 public:
  SizingEvent(double width, double height)
      : Event(EVENT_SIZING, 0), width_(width), height_(height) {
  }

  SizingEvent(const SizingEvent &e)
      : Event(EVENT_SIZING, 0), width_(e.width_), height_(e.height_) {
    ASSERT(e.GetType() == EVENT_SIZING);
  }

  double GetWidth() const { return width_; }
  double GetHeight() const { return height_; }

  void SetWidth(double width) { width_ = width; }
  void SetHeight(double height) { height_ = height; }

 private:
  double width_, height_;
};

/**
 * Class representing a sizing event.
 */
class OptionChangedEvent : public Event {
 public:
  OptionChangedEvent(const char *property_name)
      : Event(EVENT_OPTION_CHANGED, 0), property_name_(property_name) {
  }

  OptionChangedEvent(const OptionChangedEvent &e)
      : Event(EVENT_OPTION_CHANGED, 0), property_name_(e.property_name_) {
    ASSERT(e.GetType() == EVENT_OPTION_CHANGED);
  }

  const char *GetPropertyName() const { return property_name_.c_str(); }
  void SetPropertyname(
      const char *property_name) { property_name_ = property_name; }

 private:
  std::string property_name_;
};

} // namespace ggadget

#endif // GGADGET_EVENT_H__
