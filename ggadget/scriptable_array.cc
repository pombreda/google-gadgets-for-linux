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

#include "scriptable_array.h"

namespace ggadget {

class ScriptableArray::Impl {
 public:
  Impl(ScriptableArray *owner, Variant *array, size_t count)
      : owner_(owner),
        array_(array), count_(count) {
  }

  ScriptableArray *ToArray() { return owner_; }

  ScriptableArray *owner_;
  Variant *array_;
  size_t count_;
};

ScriptableArray::ScriptableArray(Variant *array, size_t count)
    : impl_(new Impl(this, array, count)) {
  RegisterConstant("count", count);
  RegisterMethod("item", NewSlot(this, &ScriptableArray::GetItem));
  // Simulates JavaScript array.
  RegisterConstant("length", count);
  SetArrayHandler(NewSlot(this, &ScriptableArray::GetItem), NULL);
  // Simulates VBArray.
  RegisterMethod("toArray", NewSlot(impl_, &Impl::ToArray));
}

ScriptableArray::~ScriptableArray() {
  delete [] impl_->array_;
  delete impl_;
}

size_t ScriptableArray::GetCount() const {
  return impl_->count_;
}

Variant ScriptableArray::GetItem(size_t index) const {
  return index < impl_->count_ ? impl_->array_[index] : Variant();
}

} // namespace ggadget
