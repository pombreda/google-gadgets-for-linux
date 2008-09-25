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

#include "framed_view_decorator_base.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "main_loop_interface.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "button_element.h"
#include "div_element.h"
#include "img_element.h"
#include "label_element.h"
#include "text_frame.h"
#include "menu_interface.h"

namespace ggadget {
static const double kVDFramedBorderWidth = 6;
static const double kVDFramedCaptionMargin = 1;
static const double kVDFramedActionMargin = 1;

class FramedViewDecoratorBase::Impl {
  struct ResizeBorderInfo {
    double x;           // relative x
    double y;           // relative y
    double pin_x;       // relative pin x
    double pin_y;       // relative pin y
    double width;       // pixel width < 0 means relative width = 1.0
    double height;      // pixel height < 0 means relative height = 1.0
    ViewInterface::CursorType cursor;
    ViewInterface::HitTest hittest;
  };

  enum ResizeBorderId {
    RESIZE_LEFT = 0,
    RESIZE_TOP,
    RESIZE_RIGHT,
    RESIZE_BOTTOM,
    RESIZE_TOP_LEFT,
    RESIZE_BOTTOM_LEFT,
    RESIZE_TOP_RIGHT,
    RESIZE_BOTTOM_RIGHT,
    NUMBER_OF_RESIZE_BORDERS
  };

  static const ResizeBorderInfo kResizeBordersInfo[];
 public:
  Impl(FramedViewDecoratorBase *owner)
    : owner_(owner),
      frame_(new DivElement(NULL, owner, NULL)),
      top_(new ImgElement(frame_, owner, NULL)),
      background_(new ImgElement(frame_, owner, NULL)),
      bottom_(new ImgElement(frame_, owner, NULL)),
      caption_(new LabelElement(frame_, owner, NULL)),
      close_button_(new ButtonElement(frame_, owner, NULL)),
      action_div_(new DivElement(frame_, owner, NULL)),
      resize_border_(new DivElement(NULL, owner, NULL)) {
    frame_->GetChildren()->InsertElement(top_, NULL);
    frame_->GetChildren()->InsertElement(background_, NULL);
    frame_->GetChildren()->InsertElement(bottom_, NULL);
    frame_->GetChildren()->InsertElement(caption_, NULL);
    frame_->GetChildren()->InsertElement(close_button_, NULL);
    frame_->GetChildren()->InsertElement(action_div_, NULL);
    frame_->SetPixelX(0);
    frame_->SetPixelY(0);
    frame_->SetRelativeWidth(1);
    frame_->SetRelativeHeight(1);
    frame_->SetVisible(true);
    owner->InsertDecoratorElement(frame_, true);

    top_->SetSrc(Variant(kVDFramedTop));
    top_->SetStretchMiddle(true);
    top_->SetPixelX(0);
    top_->SetPixelY(0);
    top_->SetRelativeWidth(1);
    top_->SetVisible(true);

    background_->SetSrc(Variant(kVDFramedBackground));
    background_->SetStretchMiddle(true);
    background_->SetPixelX(0);
    background_->SetPixelY(top_->GetSrcHeight());
    background_->SetRelativeWidth(1);
    background_->EnableCanvasCache(true);

    bottom_->SetSrc(Variant(kVDFramedBottom));
    bottom_->SetStretchMiddle(true);
    bottom_->SetPixelX(0);
    bottom_->SetRelativeY(1);
    bottom_->SetRelativePinY(1);
    bottom_->SetRelativeWidth(1);
    bottom_->SetVisible(false);

    // Setup resize borders.
    for (size_t i = 0; i < NUMBER_OF_RESIZE_BORDERS; ++i) {
      BasicElement *elm =
          new BasicElement(resize_border_, owner, NULL, NULL, false);
      const ResizeBorderInfo *info = &kResizeBordersInfo[i];
      elm->SetRelativeX(info->x);
      elm->SetRelativeY(info->y);
      elm->SetRelativePinX(info->pin_x);
      elm->SetRelativePinY(info->pin_y);
      if (info->width > 0)
        elm->SetPixelWidth(info->width);
      else
        elm->SetRelativeWidth(1);
      if (info->height > 0)
        elm->SetPixelHeight(info->height);
      else
        elm->SetRelativeHeight(1);
      elm->SetCursor(info->cursor);
      elm->SetHitTest(info->hittest);
      resize_border_->GetChildren()->InsertElement(elm, NULL);
    }
    resize_border_->SetPixelX(0);
    resize_border_->SetPixelY(0);
    resize_border_->SetRelativeWidth(1);
    resize_border_->SetRelativeHeight(1);
    resize_border_->SetVisible(true);
    resize_border_->SetEnabled(false);
    owner->InsertDecoratorElement(resize_border_, false);

    caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
    caption_->GetTextFrame()->SetWordWrap(false);
    caption_->GetTextFrame()->SetTrimming(
        CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    caption_->SetPixelX(kVDFramedBorderWidth + kVDFramedCaptionMargin);
    caption_->SetPixelY(kVDFramedBorderWidth + kVDFramedCaptionMargin);
    caption_->ConnectOnClickEvent(
        NewSlot(owner, &FramedViewDecoratorBase::OnCaptionClicked));
    caption_->SetEnabled(false);

    close_button_->SetPixelY(kVDFramedBorderWidth);
    close_button_->SetImage(Variant(kVDFramedCloseNormal));
    close_button_->SetOverImage(Variant(kVDFramedCloseOver));
    close_button_->SetDownImage(Variant(kVDFramedCloseDown));
    close_button_->ConnectOnClickEvent(
        NewSlot(owner, &FramedViewDecoratorBase::OnCloseButtonClicked));
    close_button_->Layout();

    action_div_->SetVisible(false);
    action_div_->SetRelativePinX(1);
    action_div_->SetRelativePinY(1);
  }

  void SetShowActionArea(bool show) {
    if (show) {
      bottom_->SetVisible(true);
      action_div_->SetVisible(true);
      background_->SetSrc(Variant(kVDFramedMiddle));
    } else {
      bottom_->SetVisible(false);
      action_div_->SetVisible(false);
      background_->SetSrc(Variant(kVDFramedBackground));
    }
    // Caller shall call UpdateViewSize();
  }

  void LayoutActionArea() {
    Elements *elements = action_div_->GetChildren();
    double width = 0;
    double height = 0;
    size_t count = elements->GetCount();
    for (size_t i = 0; i < count; ++i) {
      BasicElement *elm = elements->GetItemByIndex(i);
      elm->Layout();
      if (elm->IsVisible()) {
        elm->SetPixelY(0);
        elm->SetPixelX(width);
        width += (elm->GetPixelWidth() + kVDFramedActionMargin);
        height = std::max(height, elm->GetPixelHeight());
      }
    }
    action_div_->SetPixelWidth(width);
    action_div_->SetPixelHeight(height);
  }

  void LayoutResizeBorder() {
    resize_border_->SetVisible(
        owner_->GetChildViewResizable() == ViewInterface::RESIZABLE_TRUE);

    if (resize_border_->IsVisible()) {
      Elements *children = resize_border_->GetChildren();
      double left, top, right, bottom;
      bool specified = false;
      View *child = owner_->GetChildView();
      // Uses decoration frame's resize border if frame is visible.
      if (!frame_->IsVisible() && child)
        specified = child->GetResizeBorder(&left, &top, &right, &bottom);

      if (!specified) {
        left = kVDFramedBorderWidth;
        top = kVDFramedBorderWidth;
        right = kVDFramedBorderWidth;
        bottom = kVDFramedBorderWidth;
      }

      children->GetItemByIndex(RESIZE_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_TOP)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_BOTTOM)->SetPixelHeight(bottom);
      children->GetItemByIndex(RESIZE_TOP_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_TOP_LEFT)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_TOP_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_TOP_RIGHT)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_BOTTOM_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_BOTTOM_LEFT)->SetPixelHeight(bottom);
      children->GetItemByIndex(RESIZE_BOTTOM_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_BOTTOM_RIGHT)->SetPixelHeight(bottom);
    }
  }

  void DoLayout() {
    if (frame_->IsVisible()) {
      double width = owner_->GetWidth();
      double height = owner_->GetHeight();
      double caption_width, caption_height;
      double top_height;

      close_button_->SetPixelX(width - kVDFramedBorderWidth -
                               close_button_->GetPixelWidth());
      caption_width = close_button_->GetPixelX() - caption_->GetPixelX() -
          kVDFramedCaptionMargin;
      caption_->SetPixelWidth(caption_width);
      caption_->GetTextFrame()->GetExtents(caption_width, &caption_width,
                                           &caption_height);
      top_height = top_->GetSrcHeight();

      // Only allow displaying two lines of caption.
      if (caption_height > top_height - kVDFramedBorderWidth-
          kVDFramedCaptionMargin * 2) {
        double simple_caption_height;
        caption_->GetTextFrame()->GetSimpleExtents(&caption_width,
                                                   &simple_caption_height);
        caption_height = std::min(simple_caption_height * 2, caption_height);
        top_height = caption_height + kVDFramedBorderWidth +
            kVDFramedCaptionMargin * 2 + 1;
      }

      caption_->SetPixelHeight(caption_height);
      top_->SetPixelHeight(top_height);

      background_->SetPixelY(top_height);
      if (bottom_->IsVisible()) {
        bottom_->SetPixelHeight(action_div_->GetPixelHeight() +
                                kVDFramedBorderWidth +
                                kVDFramedActionMargin * 2);
        background_->SetPixelHeight(height - top_height -
                                    bottom_->GetPixelHeight());
      } else {
        background_->SetPixelHeight(height - top_height);
      }

      if (action_div_->IsVisible()) {
        action_div_->SetPixelX(width - kVDFramedBorderWidth -
                               kVDFramedActionMargin);
        action_div_->SetPixelY(height - kVDFramedBorderWidth -
                               kVDFramedActionMargin);
      }
    }

    LayoutResizeBorder();
  }

 public:
  FramedViewDecoratorBase *owner_;
  DivElement *frame_;
  ImgElement *top_;
  ImgElement *background_;
  ImgElement *bottom_;
  LabelElement *caption_;
  ButtonElement *close_button_;
  DivElement *action_div_;
  DivElement *resize_border_;
};

const FramedViewDecoratorBase::Impl::ResizeBorderInfo
FramedViewDecoratorBase::Impl::kResizeBordersInfo[] = {
  { 0, 0, 0, 0, kVDFramedBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_LEFT },
  { 0, 0, 0, 0, -1, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_TOP },
  { 1, 0, 1, 0, kVDFramedBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_RIGHT },
  { 0, 1, 0, 1, -1, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_BOTTOM },
  { 0, 0, 0, 0, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_TOPLEFT },
  { 0, 1, 0, 1, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_BOTTOMLEFT },
  { 1, 0, 1, 0, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_TOPRIGHT },
  { 1, 1, 1, 1, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_BOTTOMRIGHT }
};

FramedViewDecoratorBase::FramedViewDecoratorBase(ViewHostInterface *host,
                                                 const char *option_prefix)
  : ViewDecoratorBase(host, option_prefix, false, false),
    impl_(new Impl(this)) {
  GetViewHost()->EnableInputShapeMask(false);
}

FramedViewDecoratorBase::~FramedViewDecoratorBase() {
  delete impl_;
  impl_ = NULL;
}

void FramedViewDecoratorBase::SetCaptionClickable(bool clicked) {
  if (clicked) {
    impl_->caption_->GetTextFrame()->SetColor(Color(0, 0, 1), 1);
    impl_->caption_->GetTextFrame()->SetUnderline(true);
    impl_->caption_->SetEnabled(true);
    impl_->caption_->SetCursor(CURSOR_HAND);
  } else {
    impl_->caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
    impl_->caption_->GetTextFrame()->SetUnderline(false);
    impl_->caption_->SetEnabled(false);
    impl_->caption_->SetCursor(CURSOR_DEFAULT);
  }
}

bool FramedViewDecoratorBase::IsCaptionClickable() const {
  return impl_->caption_->IsEnabled();
}

void FramedViewDecoratorBase::SetCaptionWordWrap(bool wrap) {
  impl_->caption_->GetTextFrame()->SetWordWrap(wrap);
  DoLayout();
  UpdateViewSize();
}

bool FramedViewDecoratorBase::IsCaptionWordWrap() const {
  return impl_->caption_->GetTextFrame()->IsWordWrap();
}

void FramedViewDecoratorBase::AddActionElement(BasicElement *element) {
  ASSERT(element);
  if (!impl_->action_div_->IsVisible())
    impl_->SetShowActionArea(true);
  impl_->action_div_->GetChildren()->InsertElement(element, NULL);
  impl_->LayoutActionArea();
  DoLayout();
  UpdateViewSize();
}

void FramedViewDecoratorBase::RemoveActionElements() {
  if (impl_->action_div_->IsVisible())
    impl_->SetShowActionArea(false);
  impl_->action_div_->GetChildren()->RemoveAllElements();
  DoLayout();
  UpdateViewSize();
}

bool FramedViewDecoratorBase::OnAddContextMenuItems(MenuInterface *menu) {
  ViewDecoratorBase::OnAddContextMenuItems(menu);
  // Don't show system menu item.
  return false;
}

void FramedViewDecoratorBase::SetFrameVisible(bool visible) {
  impl_->frame_->SetVisible(visible);
  UpdateViewSize();
}

bool FramedViewDecoratorBase::IsFrameVisible() const {
  return impl_->frame_->IsVisible();
}

void FramedViewDecoratorBase::SetResizable(ResizableMode resizable) {
  ViewDecoratorBase::SetResizable(resizable);
  impl_->LayoutResizeBorder();
}

void FramedViewDecoratorBase::SetCaption(const char *caption) {
  impl_->caption_->GetTextFrame()->SetText(caption);
  ViewDecoratorBase::SetCaption(caption);
}

void FramedViewDecoratorBase::OnChildViewChanged() {
  View *child = GetChildView();
  if (child)
    impl_->caption_->GetTextFrame()->SetText(child->GetCaption());
}

void FramedViewDecoratorBase::DoLayout() {
  // Call through parent's DoLayout();
  ViewDecoratorBase::DoLayout();
  impl_->DoLayout();
}

void FramedViewDecoratorBase::GetMargins(double *left, double *top,
                                         double *right, double *bottom) const {
  if (impl_->frame_->IsVisible()) {
    *left = kVDFramedBorderWidth;
    *top = impl_->background_->GetPixelY();
    *right = kVDFramedBorderWidth;
    *bottom = impl_->bottom_->IsVisible() ? impl_->bottom_->GetPixelHeight() :
        kVDFramedBorderWidth;
  } else {
    *left = *top = *right = *bottom = 0;
  }
}

void FramedViewDecoratorBase::GetMinimumClientExtents(double *width,
                                                      double *height) const {
  *width = 0;
  *height = 0;
  if (impl_->frame_->IsVisible() && impl_->action_div_->IsVisible())
    *width = impl_->action_div_->GetPixelWidth() + kVDFramedActionMargin * 2;
}

void FramedViewDecoratorBase::OnCaptionClicked() {
}

void FramedViewDecoratorBase::OnCloseButtonClicked() {
  PostCloseSignal();
}

} // namespace ggadget
