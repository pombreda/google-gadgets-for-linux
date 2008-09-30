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

#include "scriptable_dbus_object.h"

#include <string>
#include <vector>
#include <map>

#include <ggadget/logger.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_function.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace dbus {

// A slot class to call DBusProxy::CallMethod in a simple sync way.
class DBusMethodSlot : public Slot {
  // A helper class to receive return values of a dbus method call.
  class ReturnValueReceiver {
   public:
    DBusProxy::ResultCallback *NewSlot() {
      return ggadget::NewSlot(this, &ReturnValueReceiver::Callback);
    }
    bool Callback(int index, const Variant &value) {
      if (index >= 0) {
        return_values_.push_back(ResultVariant(value));
        return true;
      }
      return false;
    }
    ResultVariant GetReturnValue() const {
      if (return_values_.size() == 0) return ResultVariant(Variant());
      if (return_values_.size() == 1) return return_values_[0];
      return ResultVariant(
          Variant(ScriptableArray::Create(return_values_.begin(),
                                          return_values_.end())));
    }
   private:
    std::vector<ResultVariant> return_values_;
  };

 public:
  DBusMethodSlot(DBusProxy *proxy, const std::string &method,
                 bool has_metadata, int timeout,
                 int argc, Variant::Type *arg_types,
                 int retc, Variant::Type *ret_types)
    : proxy_(proxy),
      method_(method),
      has_metadata_(has_metadata),
      timeout_(timeout),
      argc_(argc),
      arg_types_(arg_types),
      retc_(retc),
      ret_types_(ret_types) {
  }
  virtual ~DBusMethodSlot() {
    delete[] arg_types_;
    delete[] ret_types_;
  }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    ReturnValueReceiver receiver;
    bool ret = proxy_->CallMethod(method_, true, timeout_,
                                  receiver.NewSlot(), argc, argv);
    if (ret)
      return receiver.GetReturnValue();
    return ResultVariant();
  }
  virtual bool HasMetadata() const {
    return true;
  }
  virtual Variant::Type GetReturnType() const {
    if (!has_metadata_) return Variant::TYPE_VARIANT;
    if (retc_ == 0) return Variant::TYPE_VOID;
    if (retc_ == 1) return ret_types_[0];
    return Variant::TYPE_SCRIPTABLE;
  }
  virtual int GetArgCount() const {
    return has_metadata_ ? argc_ : INT_MAX;
  }
  virtual const Variant::Type *GetArgTypes() const {
    return arg_types_;
  }
  virtual bool operator==(const Slot &another) const {
    const DBusMethodSlot *slot = down_cast<const DBusMethodSlot *>(&another);
    return slot && proxy_ == slot->proxy_ && method_ == slot->method_;
  }

 private:
  DBusProxy *proxy_;
  std::string method_;
  bool has_metadata_;
  int timeout_;
  int argc_;
  Variant::Type *arg_types_;
  int retc_;
  Variant::Type *ret_types_;
};

class DBusSignal: public Signal {
 public:
  DBusSignal(int argc, Variant::Type *arg_types)
    : argc_(argc), arg_types_(arg_types), prototype_slot_(NULL) {
  }
  virtual ~DBusSignal() {
    delete[] arg_types_;
    delete prototype_slot_;
  }
  virtual Variant::Type GetReturnType() const {
    return Variant::TYPE_VOID;
  }
  virtual int GetArgCount() const {
    return argc_;
  }
  virtual const Variant::Type *GetArgTypes() const {
    return arg_types_;
  }
  Slot *GetPrototypeSlot() {
    if (!prototype_slot_)
      prototype_slot_ = new SignalSlot(this);
    return prototype_slot_;
  }
  Slot *GetDefaultConnectedSlot() {
    return GetDefaultConnection()->slot();
  }
  bool SetDefaultConnectedSlot(Slot *slot) {
    return GetDefaultConnection()->Reconnect(slot);
  }

 private:
  int argc_;
  Variant::Type *arg_types_;
  SignalSlot *prototype_slot_;
};

class ScriptableDBusObject::Impl {
  // Helper class to receive enumerate result.
  class EnumerateReceiver {
   public:
    Slot1<bool, const std::string &> *NewSlot() {
      return ggadget::NewSlot(this, &EnumerateReceiver::Callback);
    }
    bool Callback(const std::string &value) {
      if (value.length())
        results_.push_back(value);
      return true;
    }
    ScriptableInterface *CreateArray() const {
      return ScriptableArray::Create(results_.begin(), results_.end());
    }

   private:
    StringVector results_;
  };

 public:
  // A slot to invoke DBusProxy::CallMethod directly.
  // It's a class slot.
  class DBusCallMethodSlot : public Slot {
    class ResultCallbackProxy : public DBusProxy::ResultCallback {
     public:
      explicit ResultCallbackProxy(Slot *callback) : callback_(callback) {}
      virtual ~ResultCallbackProxy() { delete callback_; }
      virtual ResultVariant Call(ScriptableInterface *object,
                                 int argc, const Variant argv[]) const {
        bool result = true;
        callback_->Call(object, argc, argv).v().ConvertToBool(&result);
        return ResultVariant(Variant(result));
      }
      virtual bool operator==(const Slot &another) const {
        return false;
      }
     private:
      Slot *callback_;
    };

   public:
    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      ASSERT(object && object->IsInstanceOf(ScriptableDBusObject::CLASS_ID));
      ScriptableDBusObject *dbus_obj = down_cast<ScriptableDBusObject*>(object);
      if (argc < 4 ||
          argv[0].type() != Variant::TYPE_STRING ||
          argv[1].type() != Variant::TYPE_BOOL ||
          argv[2].type() != Variant::TYPE_INT64 ||
          argv[3].type() != Variant::TYPE_SLOT) {
        DLOG("Argument type mismatch when calling DBusProxy::CallMethod()");
        return ResultVariant(Variant(0));
      }
      int call_id = dbus_obj->impl_->proxy_->CallMethod(
          VariantValue<const std::string &>()(argv[0]), // method name
          VariantValue<bool>()(argv[1]),                // sync
          VariantValue<int>()(argv[2]),                 // timeout
          new ResultCallbackProxy(VariantValue<Slot *>()(argv[3])),
          argc - 4, argv + 4);
      return ResultVariant(Variant(call_id));
    }
    virtual bool HasMetadata() const {
      return true;
    }
    virtual int GetArgCount() const {
      return INT_MAX;
    }
    virtual const Variant::Type *GetArgTypes() const {
      static const Variant::Type arg_types[] = {
        Variant::TYPE_STRING, Variant::TYPE_BOOL,
        Variant::TYPE_INT64, Variant::TYPE_SLOT,
        Variant::TYPE_VOID };
      return arg_types;
    }
    virtual Variant::Type GetReturnType() const {
      return Variant::TYPE_INT64;
    }
    virtual bool operator==(const Slot &another) const {
      // No use.
      return false;
    }
  };

 public:
  Impl(ScriptableDBusObject *owner, DBusProxy* proxy)
    : owner_(owner),
      proxy_(proxy),
      timeout_(-1),
      on_signal_emit_connection_(NULL) {
    ASSERT(proxy);
    on_signal_emit_connection_ =
        proxy->ConnectOnSignalEmit(NewSlot(this, &Impl::EmitSignal));
  }

  ~Impl() {
    on_signal_emit_connection_->Disconnect();
    delete proxy_;
    for (SignalMap::iterator it = signals_.begin(); it != signals_.end(); ++it)
      delete it->second;
    signals_.clear();
  }

  int GetTimeout() {
    return timeout_;
  }

  void SetTimeout(int timeout) {
    if (timeout >= 0)
      timeout_ = timeout;
    else
      timeout_ = -1;
  }

  ScriptableInterface *ListMethods() {
    EnumerateReceiver receiver;
    proxy_->EnumerateMethods(receiver.NewSlot());
    return receiver.CreateArray();
  }

  ScriptableInterface *ListSignals() {
    EnumerateReceiver receiver;
    proxy_->EnumerateSignals(receiver.NewSlot());
    return receiver.CreateArray();
  }

  ScriptableInterface *ListProperties() {
    EnumerateReceiver receiver;
    proxy_->EnumerateProperties(receiver.NewSlot());
    return receiver.CreateArray();
  }

  ScriptableInterface *ListChildren() {
    EnumerateReceiver receiver;
    proxy_->EnumerateChildren(receiver.NewSlot());
    return receiver.CreateArray();
  }

  ScriptableInterface *ListInterfaces() {
    EnumerateReceiver receiver;
    proxy_->EnumerateInterfaces(receiver.NewSlot());
    return receiver.CreateArray();
  }

  ScriptableInterface *GetChild(const std::string &name,
                                const std::string &interface) {
    if (name.empty() || interface.empty()) return NULL;
    DBusProxy *proxy = proxy_->NewChildProxy(name, interface);
    if (proxy)
      return new ScriptableDBusObject(proxy);
    return NULL;
  }

  ScriptableInterface *GetInterface(const std::string &interface) {
    if (interface.empty())
      return NULL;
    DBusProxy *proxy = proxy_->NewInterfaceProxy(interface);
    if (proxy)
      return new ScriptableDBusObject(proxy);
    return NULL;
  }

  ResultVariant DynamicGetter(const std::string &name, bool get_info) {
    DLOG("ScriptableDBusObject::DynamicGetter(%s)", name.c_str());
    // First check if it's a known signal.
    SignalMap::iterator sig_it = signals_.find(name);
    if (sig_it != signals_.end()) {
      if (get_info) {
        return ResultVariant(Variant(sig_it->second->GetPrototypeSlot()));
      }
      return ResultVariant(Variant(sig_it->second->GetDefaultConnectedSlot()));
    }

    // Method has the highest priority.
    int argc = 0;
    Variant::Type *arg_types = NULL;
    int retc = 0;
    Variant::Type *ret_types = NULL;
    if (proxy_->GetMethodInfo(name, &argc, &arg_types, &retc, &ret_types)) {
      DBusMethodSlot *slot =
          new DBusMethodSlot(proxy_, name, true, timeout_,
                             argc, arg_types, retc, ret_types);
      // it's the only way to support dynamic function.
      return ResultVariant(Variant(new ScriptableFunction(slot)));
    }

    // Then try signal.
    if (proxy_->GetSignalInfo(name, &argc, &arg_types)) {
      DBusSignal *signal = new DBusSignal(argc, arg_types);
      signals_[name] = signal;
      if (get_info) {
        return ResultVariant(Variant(signal->GetPrototypeSlot()));
      }
      return ResultVariant(Variant(signal->GetDefaultConnectedSlot()));
    }

    // Then try property.
    Variant::Type prop_type = Variant::TYPE_VOID;
    DBusProxy::PropertyAccess prop_access =
        proxy_->GetPropertyInfo(name, &prop_type);
    if (prop_access != DBusProxy::PROP_UNKNOWN) {
      if (get_info) {
        return ResultVariant(Variant(prop_type));
      }
      // More expansive than just get info.
      if (prop_access & DBusProxy::PROP_READ) {
        return proxy_->GetProperty(name);
      }
      DLOG("Property %s is write only.", name.c_str());
      return ResultVariant();
    }
    // Can't resolve the property.
    DLOG("Can't resolve property name: %s, treat it as a method.",
         name.c_str());

    // Assume it's a method.
    DBusMethodSlot *method_slot =
        new DBusMethodSlot(proxy_, name, false, timeout_, 0, NULL, 0, NULL);
    return ResultVariant(Variant(new ScriptableFunction(method_slot)));
  }

  bool DynamicSetter(const std::string &name, const Variant &value) {
    DLOG("ScriptableDBusObject::DynamicSetter(%s)", name.c_str());
    // First check if it's a known signal.
    SignalMap::iterator sig_it = signals_.find(name);
    if (sig_it != signals_.end()) {
      if (value.type() == Variant::TYPE_SLOT) {
        return sig_it->second->SetDefaultConnectedSlot(
            VariantValue<Slot *>()(value));
      }
      DLOG("Signal property expects a slot.");
      return false;
    }
    // Then try to resolve signal.
    int argc = 0;
    Variant::Type *arg_types = NULL;
    if (proxy_->GetSignalInfo(name, &argc, &arg_types)) {
      DBusSignal *signal = new DBusSignal(argc, arg_types);
      signals_[name] = signal;
      if (value.type() == Variant::TYPE_SLOT) {
        return signal->SetDefaultConnectedSlot(VariantValue<Slot *>()(value));
      }
      DLOG("Signal property expects a slot.");
      return false;
    }
    // Then try property.
    Variant::Type prop_type = Variant::TYPE_VOID;
    DBusProxy::PropertyAccess prop_access =
        proxy_->GetPropertyInfo(name, &prop_type);
    if (prop_access & DBusProxy::PROP_WRITE) {
      return proxy_->SetProperty(name, value);
    }
    // Can't resolve the property.
    DLOG("Can't resolve property name: %s", name.c_str());
    return false;
  }

  void EmitSignal(const std::string &name, int argc, const Variant *argv) {
    SignalMap::iterator sig_it = signals_.find(name);
    if (sig_it != signals_.end()) {
      // Reference owner object before emitting signal to prevent it from
      // being destroyed by garbage collection.
      owner_->Ref();
      sig_it->second->Emit(argc, argv);
      owner_->Unref();
    }
  }

 public:
  static DBusProxy *GetProxy(ScriptableDBusObject *obj) {
    return obj->impl_->proxy_;
  }

  static const DBusProxy *GetConstProxy(ScriptableDBusObject *obj) {
    return obj->impl_->proxy_;
  }

 private:
  ScriptableDBusObject *owner_;
  DBusProxy *proxy_;
  int timeout_;
  Connection *on_signal_emit_connection_;
  typedef std::map<std::string, DBusSignal *> SignalMap;
  SignalMap signals_;
};

ScriptableDBusObject::ScriptableDBusObject(DBusProxy *proxy)
  : impl_(proxy ? new Impl(this, proxy) : NULL) {
}

ScriptableDBusObject::~ScriptableDBusObject() {
  delete impl_;
  impl_ = NULL;
}

void ScriptableDBusObject::DoRegister() {
  if (!impl_) {
    DLOG("Invalid ScriptableDBusObject object.");
    return;
  }
  SetDynamicPropertyHandler(NewSlot(impl_, &Impl::DynamicGetter),
                            NewSlot(impl_, &Impl::DynamicSetter));
}

void ScriptableDBusObject::DoClassRegister() {
  if (!impl_) {
    DLOG("Invalid ScriptableDBusObject object.");
    return;
  }
  RegisterProperty("$name",
                   NewSlot(&DBusProxy::GetName, Impl::GetConstProxy),
                   NULL);
  RegisterProperty("$path",
                   NewSlot(&DBusProxy::GetPath, Impl::GetConstProxy),
                   NULL);
  RegisterProperty("$interface",
                   NewSlot(&DBusProxy::GetInterface, Impl::GetConstProxy),
                   NULL);
  RegisterProperty("$timeout",
                   NewSlot(&Impl::GetTimeout, &ScriptableDBusObject::impl_),
                   NewSlot(&Impl::SetTimeout, &ScriptableDBusObject::impl_));
  RegisterProperty("$methods",
                   NewSlot(&Impl::ListMethods, &ScriptableDBusObject::impl_),
                   NULL);
  RegisterProperty("$signals",
                   NewSlot(&Impl::ListSignals, &ScriptableDBusObject::impl_),
                   NULL);
  RegisterProperty("$properties",
                   NewSlot(&Impl::ListProperties, &ScriptableDBusObject::impl_),
                   NULL);
  RegisterProperty("$children",
                   NewSlot(&Impl::ListChildren, &ScriptableDBusObject::impl_),
                   NULL);
  RegisterProperty("$interfaces",
                   NewSlot(&Impl::ListInterfaces, &ScriptableDBusObject::impl_),
                   NULL);

  RegisterMethod("$callMethod",
                 new Impl::DBusCallMethodSlot());
  RegisterMethod("$cancelMethodCall",
                 NewSlot(&DBusProxy::CancelMethodCall, Impl::GetProxy));
  RegisterMethod("$isMethodCallPending",
                 NewSlot(&DBusProxy::IsMethodCallPending, Impl::GetConstProxy));
  RegisterMethod("$getProperty",
                 NewSlot(&DBusProxy::GetProperty, Impl::GetProxy));
  RegisterMethod("$setProperty",
                 NewSlot(&DBusProxy::SetProperty, Impl::GetProxy));
  RegisterMethod("$getChild",
                 NewSlot(&Impl::GetChild, &ScriptableDBusObject::impl_));
  RegisterMethod("$getInterface",
                 NewSlot(&Impl::GetInterface, &ScriptableDBusObject::impl_));
}

}  // namespace dbus
}  // namespace ggadget
