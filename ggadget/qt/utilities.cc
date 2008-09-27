/*
  Copyright 2008 Google Inc.

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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>
#include <QtGui/QDesktopWidget>

#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/host_utils.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/view_interface.h>
#include <ggadget/xdg/utilities.h>
#include "utilities.h"
#include "utilities_internal.h"

namespace ggadget {
namespace qt {

struct CursorTypeMapping {
  int type;
  Qt::CursorShape qt_type;
};

static const CursorTypeMapping kCursorTypeMappings[] = {
  { ViewInterface::CURSOR_ARROW, Qt::ArrowCursor }, // FIXME
  { ViewInterface::CURSOR_IBEAM, Qt::IBeamCursor },
  { ViewInterface::CURSOR_WAIT, Qt::WaitCursor },
  { ViewInterface::CURSOR_CROSS, Qt::CrossCursor },
  { ViewInterface::CURSOR_UPARROW, Qt::UpArrowCursor },
  { ViewInterface::CURSOR_SIZE, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_SIZENWSE, Qt::SizeFDiagCursor },
  { ViewInterface::CURSOR_SIZENESW, Qt::SizeBDiagCursor },
  { ViewInterface::CURSOR_SIZEWE, Qt::SizeHorCursor },
  { ViewInterface::CURSOR_SIZENS, Qt::SizeVerCursor },
  { ViewInterface::CURSOR_SIZEALL, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_NO, Qt::ForbiddenCursor },
  { ViewInterface::CURSOR_HAND, Qt::OpenHandCursor },
  { ViewInterface::CURSOR_BUSY, Qt::BusyCursor }, // FIXME
  { ViewInterface::CURSOR_HELP, Qt::WhatsThisCursor }
};

Qt::CursorShape GetQtCursorShape(int type) {
  for (size_t i = 0; i < arraysize(kCursorTypeMappings); ++i) {
    if (kCursorTypeMappings[i].type == type)
      return kCursorTypeMappings[i].qt_type;
  }
  return Qt::ArrowCursor;
}

// Check out qt document to get more information about MouseButtons and
// MouseButton
int GetMouseButtons(const Qt::MouseButtons buttons){
  int ret = 0;
  if (buttons & Qt::LeftButton) ret |= MouseEvent::BUTTON_LEFT;
  if (buttons & Qt::RightButton) ret |= MouseEvent::BUTTON_RIGHT;
  if (buttons & Qt::MidButton) ret |= MouseEvent::BUTTON_MIDDLE;
  return ret;
}

int GetMouseButton(const Qt::MouseButton button) {
  if (button == Qt::LeftButton) return MouseEvent::BUTTON_LEFT;
  if (button == Qt::RightButton) return MouseEvent::BUTTON_RIGHT;
  if (button == Qt::MidButton) return MouseEvent::BUTTON_MIDDLE;
  return 0;
}

int GetModifiers(Qt::KeyboardModifiers state) {
  int mod = Event::MOD_NONE;
  if (state & Qt::ShiftModifier) mod |= Event::MOD_SHIFT;
  if (state & Qt::ControlModifier)  mod |= Event::MOD_CONTROL;
  if (state & Qt::AltModifier) mod |= Event::MOD_ALT;
  return mod;
}

struct KeyvalKeyCode {
  Qt::Key qt_key;
  unsigned int key_code;
};

static KeyvalKeyCode keyval_key_code_map[] = {
  { Qt::Key_Cancel,       KeyboardEvent::KEY_CANCEL },
  { Qt::Key_Backspace,    KeyboardEvent::KEY_BACK },
  { Qt::Key_Tab,          KeyboardEvent::KEY_TAB },
  { Qt::Key_Clear,        KeyboardEvent::KEY_CLEAR },
  { Qt::Key_Return,       KeyboardEvent::KEY_RETURN },
  { Qt::Key_Shift,        KeyboardEvent::KEY_SHIFT },
  { Qt::Key_Control,      KeyboardEvent::KEY_CONTROL },
  { Qt::Key_Alt,          KeyboardEvent::KEY_ALT },
  { Qt::Key_Pause,        KeyboardEvent::KEY_PAUSE },
  { Qt::Key_CapsLock,     KeyboardEvent::KEY_CAPITAL },
  { Qt::Key_Escape,       KeyboardEvent::KEY_ESCAPE },
  { Qt::Key_Space,        KeyboardEvent::KEY_SPACE },
  { Qt::Key_PageUp,       KeyboardEvent::KEY_PAGE_UP },
  { Qt::Key_PageDown,     KeyboardEvent::KEY_PAGE_DOWN },
  { Qt::Key_End,          KeyboardEvent::KEY_END },
  { Qt::Key_Home,         KeyboardEvent::KEY_HOME },
  { Qt::Key_Left,         KeyboardEvent::KEY_LEFT },
  { Qt::Key_Up,           KeyboardEvent::KEY_UP },
  { Qt::Key_Right,        KeyboardEvent::KEY_RIGHT },
  { Qt::Key_Down,         KeyboardEvent::KEY_DOWN },
  { Qt::Key_Select,       KeyboardEvent::KEY_SELECT },
  { Qt::Key_Print,        KeyboardEvent::KEY_PRINT },
  { Qt::Key_Execute,      KeyboardEvent::KEY_EXECUTE },
  { Qt::Key_Insert,       KeyboardEvent::KEY_INSERT },
  { Qt::Key_Delete,       KeyboardEvent::KEY_DELETE },
  { Qt::Key_Help,         KeyboardEvent::KEY_HELP },
  { Qt::Key_Menu,         KeyboardEvent::KEY_CONTEXT_MENU },
  { Qt::Key_Exclam,       '1' },
  { Qt::Key_At,           '2' },
  { Qt::Key_NumberSign,   '3' },
  { Qt::Key_Dollar,       '4' },
  { Qt::Key_Percent,      '5' },
  { Qt::Key_AsciiCircum,  '6' },
  { Qt::Key_Ampersand,    '7' },
  { Qt::Key_Asterisk,     '8' },
  { Qt::Key_ParenLeft,    '9' },
  { Qt::Key_ParenRight,   '0' },
  { Qt::Key_Colon,        KeyboardEvent::KEY_COLON },
  { Qt::Key_Semicolon,    KeyboardEvent::KEY_COLON },
  { Qt::Key_Plus,         KeyboardEvent::KEY_PLUS },
  { Qt::Key_Equal,        KeyboardEvent::KEY_PLUS },
  { Qt::Key_Comma,        KeyboardEvent::KEY_COMMA },
  { Qt::Key_Less,         KeyboardEvent::KEY_COMMA },
  { Qt::Key_Minus,        KeyboardEvent::KEY_MINUS },
  { Qt::Key_Underscore,   KeyboardEvent::KEY_MINUS },
  { Qt::Key_Period,       KeyboardEvent::KEY_PERIOD },
  { Qt::Key_Greater,      KeyboardEvent::KEY_PERIOD },
  { Qt::Key_Slash,        KeyboardEvent::KEY_SLASH },
  { Qt::Key_Question,     KeyboardEvent::KEY_SLASH },
  { Qt::Key_Agrave,       KeyboardEvent::KEY_GRAVE },
  { Qt::Key_Egrave,       KeyboardEvent::KEY_GRAVE },
  { Qt::Key_Igrave,       KeyboardEvent::KEY_GRAVE },
  { Qt::Key_Ograve,       KeyboardEvent::KEY_GRAVE },
  { Qt::Key_Dead_Grave,   KeyboardEvent::KEY_GRAVE },
  { Qt::Key_Igrave,       KeyboardEvent::KEY_GRAVE },
  { Qt::Key_AsciiTilde,   KeyboardEvent::KEY_GRAVE },
  { Qt::Key_BracketLeft,  KeyboardEvent::KEY_BRACKET_LEFT },
  { Qt::Key_BraceLeft,    KeyboardEvent::KEY_BRACKET_LEFT },
  { Qt::Key_Backslash,    KeyboardEvent::KEY_BACK_SLASH },
  { Qt::Key_Bar,          KeyboardEvent::KEY_BACK_SLASH },
  { Qt::Key_BracketRight, KeyboardEvent::KEY_BRACKET_RIGHT },
  { Qt::Key_BraceRight,   KeyboardEvent::KEY_BRACKET_RIGHT },
  { Qt::Key_QuoteDbl,     KeyboardEvent::KEY_QUOTE },
  { Qt::Key_Apostrophe,   KeyboardEvent::KEY_QUOTE },
  { Qt::Key_0,            '0' },
  { Qt::Key_1,            '1' },
  { Qt::Key_2,            '2' },
  { Qt::Key_3,            '3' },
  { Qt::Key_4,            '4' },
  { Qt::Key_5,            '5' },
  { Qt::Key_6,            '6' },
  { Qt::Key_7,            '7' },
  { Qt::Key_8,            '8' },
  { Qt::Key_9,            '9' },
  { Qt::Key_0,            '0' },
  { Qt::Key_A,            'A' },
  { Qt::Key_B,            'B' },
  { Qt::Key_C,            'C' },
  { Qt::Key_D,            'D' },
  { Qt::Key_E,            'E' },
  { Qt::Key_F,            'F' },
  { Qt::Key_G,            'G' },
  { Qt::Key_H,            'H' },
  { Qt::Key_I,            'I' },
  { Qt::Key_J,            'J' },
  { Qt::Key_K,            'K' },
  { Qt::Key_L,            'L' },
  { Qt::Key_M,            'M' },
  { Qt::Key_N,            'N' },
  { Qt::Key_O,            'O' },
  { Qt::Key_P,            'P' },
  { Qt::Key_Q,            'Q' },
  { Qt::Key_R,            'R' },
  { Qt::Key_S,            'S' },
  { Qt::Key_T,            'T' },
  { Qt::Key_U,            'U' },
  { Qt::Key_V,            'V' },
  { Qt::Key_W,            'W' },
  { Qt::Key_X,            'X' },
  { Qt::Key_Y,            'Y' },
  { Qt::Key_Z,            'Z' },
#if 0
  { Qt::Key_a,            'A' },
  { Qt::Key_b,            'B' },
  { Qt::Key_c,            'C' },
  { Qt::Key_d,            'D' },
  { Qt::Key_e,            'E' },
  { Qt::Key_f,            'F' },
  { Qt::Key_g,            'G' },
  { Qt::Key_h,            'H' },
  { Qt::Key_i,            'I' },
  { Qt::Key_j,            'J' },
  { Qt::Key_k,            'K' },
  { Qt::Key_l,            'L' },
  { Qt::Key_m,            'M' },
  { Qt::Key_n,            'N' },
  { Qt::Key_o,            'O' },
  { Qt::Key_p,            'P' },
  { Qt::Key_q,            'Q' },
  { Qt::Key_r,            'R' },
  { Qt::Key_s,            'S' },
  { Qt::Key_t,            'T' },
  { Qt::Key_u,            'U' },
  { Qt::Key_v,            'V' },
  { Qt::Key_w,            'W' },
  { Qt::Key_x,            'X' },
  { Qt::Key_y,            'Y' },
  { Qt::Key_z,            'Z' },
  { Qt::Key_KP_0,         KeyboardEvent::KEY_NUMPAD0 },
  { Qt::Key_KP_1,         KeyboardEvent::KEY_NUMPAD1 },
  { Qt::Key_KP_2,         KeyboardEvent::KEY_NUMPAD2 },
  { Qt::Key_KP_3,         KeyboardEvent::KEY_NUMPAD3 },
  { Qt::Key_KP_4,         KeyboardEvent::KEY_NUMPAD4 },
  { Qt::Key_KP_5,         KeyboardEvent::KEY_NUMPAD5 },
  { Qt::Key_KP_6,         KeyboardEvent::KEY_NUMPAD6 },
  { Qt::Key_KP_7,         KeyboardEvent::KEY_NUMPAD7 },
  { Qt::Key_KP_8,         KeyboardEvent::KEY_NUMPAD8 },
  { Qt::Key_KP_9,         KeyboardEvent::KEY_NUMPAD9 },
  { Qt::Key_KP_Add,       KeyboardEvent::KEY_ADD },
  { Qt::Key_KP_Separator, KeyboardEvent::KEY_SEPARATOR },
  { Qt::Key_KP_Subtract,  KeyboardEvent::KEY_SUBTRACT },
  { Qt::Key_KP_Decimal,   KeyboardEvent::KEY_DECIMAL },
#endif
  { Qt::Key_multiply,     KeyboardEvent::KEY_MULTIPLY },
  { Qt::Key_division,     KeyboardEvent::KEY_DIVIDE },
  { Qt::Key_F1,           KeyboardEvent::KEY_F1 },
  { Qt::Key_F2,           KeyboardEvent::KEY_F2 },
  { Qt::Key_F3,           KeyboardEvent::KEY_F3 },
  { Qt::Key_F4,           KeyboardEvent::KEY_F4 },
  { Qt::Key_F5,           KeyboardEvent::KEY_F5 },
  { Qt::Key_F6,           KeyboardEvent::KEY_F6 },
  { Qt::Key_F7,           KeyboardEvent::KEY_F7 },
  { Qt::Key_F8,           KeyboardEvent::KEY_F8 },
  { Qt::Key_F9,           KeyboardEvent::KEY_F9 },
  { Qt::Key_F10,          KeyboardEvent::KEY_F10 },
  { Qt::Key_F11,          KeyboardEvent::KEY_F11 },
  { Qt::Key_F12,          KeyboardEvent::KEY_F12 },
  { Qt::Key_F13,          KeyboardEvent::KEY_F13 },
  { Qt::Key_F14,          KeyboardEvent::KEY_F14 },
  { Qt::Key_F15,          KeyboardEvent::KEY_F15 },
  { Qt::Key_F16,          KeyboardEvent::KEY_F16 },
  { Qt::Key_F17,          KeyboardEvent::KEY_F17 },
  { Qt::Key_F18,          KeyboardEvent::KEY_F18 },
  { Qt::Key_F19,          KeyboardEvent::KEY_F19 },
  { Qt::Key_F20,          KeyboardEvent::KEY_F20 },
  { Qt::Key_F21,          KeyboardEvent::KEY_F21 },
  { Qt::Key_F22,          KeyboardEvent::KEY_F22 },
  { Qt::Key_F23,          KeyboardEvent::KEY_F23 },
  { Qt::Key_F24,          KeyboardEvent::KEY_F24 },
  { Qt::Key_NumLock,      KeyboardEvent::KEY_NUMLOCK },
  { Qt::Key_ScrollLock,   KeyboardEvent::KEY_SCROLL },
};

static inline bool KeyvalCompare(const KeyvalKeyCode &v1,
                                  const KeyvalKeyCode &v2) {
  return v1.qt_key < v2.qt_key;
}
unsigned int GetKeyCode(int qt_key) {
  static bool keycode_map_init = false;
  if (!keycode_map_init) {
    std::sort(keyval_key_code_map,
              keyval_key_code_map + arraysize(keyval_key_code_map),
              KeyvalCompare);
    keycode_map_init = true;
  }

  KeyvalKeyCode key = { static_cast<Qt::Key>(qt_key), 0 };
  const KeyvalKeyCode *pos = std::lower_bound(
      keyval_key_code_map, keyval_key_code_map + arraysize(keyval_key_code_map),
      key, KeyvalCompare);
  ASSERT(pos);
  return pos->qt_key == qt_key ? pos->key_code : 0;
}

static std::string EscapeHtmlText(const std::string str) {
  std::string output;
  std::string::size_type pos;
  for (pos = 0; pos < str.length(); pos++) {
    if (str[pos] == '<')
      output.append("&lt;");
    else if (str[pos] == '>')
      output.append("&gt;");
    else
      output.push_back(str[pos]);
  }
  return output;
}

void ShowGadgetAboutDialog(Gadget *gadget) {
  ASSERT(gadget);

  // About text
  std::string about_text =
      TrimString(gadget->GetManifestInfo(kManifestAboutText));

  if (about_text.empty()) {
    gadget->OnCommand(Gadget::CMD_ABOUT_DIALOG);
    return;
  }

  // Title and Copyright
  std::string title_text;
  std::string copyright_text;
  if (!SplitString(about_text, "\n", &title_text, &about_text)) {
    about_text = title_text;
    title_text = gadget->GetManifestInfo(kManifestName);
  }
  title_text = TrimString(title_text);
  about_text = TrimString(about_text);

  if (!SplitString(about_text, "\n", &copyright_text, &about_text)) {
    about_text = copyright_text;
    copyright_text = gadget->GetManifestInfo(kManifestCopyright);
  }
  copyright_text = TrimString(copyright_text);
  about_text = TrimString(about_text);

  // Remove HTML tags from the text.
  if (ContainsHTML(title_text.c_str()))
    title_text = ExtractTextFromHTML(title_text.c_str());
  if (ContainsHTML(copyright_text.c_str()))
    copyright_text = ExtractTextFromHTML(copyright_text.c_str());
  if (ContainsHTML(about_text.c_str()))
    about_text = ExtractTextFromHTML(about_text.c_str());

  std::string title_copyright = "<b>";
  title_copyright.append(EscapeHtmlText(title_text));
  title_copyright.append("</b><br>");
  title_copyright.append(EscapeHtmlText(copyright_text));

  // Load icon
  std::string icon_name = gadget->GetManifestInfo(kManifestIcon);
  std::string data;
  QPixmap icon;
  if (gadget->GetFileManager()->ReadFile(icon_name.c_str(), &data)) {
    icon.loadFromData(reinterpret_cast<const uchar *>(data.c_str()),
                      static_cast<int>(data.length()));
  }

  QMessageBox box(QMessageBox::NoIcon,
                  QString::fromUtf8(title_text.c_str()),
                  QString::fromUtf8(title_copyright.c_str()),
                  QMessageBox::Ok);
  box.setInformativeText(QString::fromUtf8(about_text.c_str()));

  box.setIconPixmap(icon);
  box.exec();
}

QWidget *NewGadgetDebugConsole(Gadget *gadget, QWidget** widget) {
  DebugConsole *console = new DebugConsole(gadget, widget);
  console->show();
  return console;
}

bool OpenURL(const Gadget *gadget, const char *url) {
  // FIXME: Support launching desktop file.
  return ggadget::xdg::OpenURL(gadget, url);
}

QPixmap GetGadgetIcon(const Gadget *gadget) {
  std::string data;
  QPixmap pixmap;
  if (gadget) {
    std::string icon_name = gadget->GetManifestInfo(kManifestIcon);
    gadget->GetFileManager()->ReadFile(icon_name.c_str(), &data);
  }
  if (data.empty()) {
    FileManagerInterface *file_manager = GetGlobalFileManager();
    if (file_manager)
      file_manager->ReadFile(kGadgetsIcon, &data);
  }
  if (!data.empty()) {
    pixmap.loadFromData(reinterpret_cast<const uchar *>(data.c_str()),
                        static_cast<int>(data.length()));
  }
  return pixmap;
}

void SetGadgetWindowIcon(QWidget *widget, const Gadget *gadget) {
  widget->setWindowIcon(GetGadgetIcon(gadget));
}

QPoint GetPopupPosition(const QRect &rect, const QSize &size) {
  int x, y;
  QDesktopWidget desktop;
  QRect r = desktop.screenGeometry();
  ggadget::GetPopupPosition(rect.x(), rect.y(), rect.width(), rect.height(),
                            size.width(), size.height(),
                            r.width(), r.height(),
                            &x, &y);
  return QPoint(x, y);
}


} // namespace qt
} // namespace ggadget
#include "utilities_internal.moc"
