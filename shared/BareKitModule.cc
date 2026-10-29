#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <assert.h>
#include <dlfcn.h>

#include <react/bridging/Bridging.h>

#if defined(__ANDROID__)
#include <fbjni/fbjni.h>
#endif

#include "BareKitModule.h"
#include "bare-kit.h"

using namespace facebook::jsi;
using namespace facebook::react;

namespace facebook::react {

namespace {

// Bare Kit is brought along by the application, through bare-kit-swift or
// bare-kit-kotlin, so that the library is shared with any other code of the
// application that runs worklets. It is therefore resolved at runtime rather
// than linked.
#define BARE_KIT_FUNCTIONS(V) \
  V(bare_worklet_create) \
  V(bare_worklet_destroy) \
  V(bare_worklet_on_suspend) \
  V(bare_worklet_on_wakeup) \
  V(bare_worklet_on_idle) \
  V(bare_worklet_on_resume) \
  V(bare_worklet_on_exit) \
  V(bare_worklet_start) \
  V(bare_worklet_suspend) \
  V(bare_worklet_resume) \
  V(bare_worklet_wakeup) \
  V(bare_worklet_terminate) \
  V(bare_ipc_create) \
  V(bare_ipc_end) \
  V(bare_ipc_destroy) \
  V(bare_ipc_read) \
  V(bare_ipc_write) \
  V(bare_ipc_poll_create) \
  V(bare_ipc_poll_destroy) \
  V(bare_ipc_poll_get_data) \
  V(bare_ipc_poll_set_data) \
  V(bare_ipc_poll_start) \
  V(bare_ipc_poll_stop)

static struct {
#define V(name) decltype(&::name) name;
  BARE_KIT_FUNCTIONS(V)
#undef V
} kit;

#if defined(__APPLE__)
static const char *bare_kit_library = "@rpath/BareKit.framework/BareKit";
#else
static const char *bare_kit_library = "libbare-kit.so";
#endif

static void
load(Runtime &rt) {
  static std::string error;
  static std::once_flag loaded;

  std::call_once(loaded, [] {
    void *handle = dlopen(bare_kit_library, RTLD_NOW | RTLD_LOCAL);

    if (handle == nullptr) {
      error = dlerror();

      return;
    }

#define V(name) \
  kit.name = reinterpret_cast<decltype(kit.name)>(dlsym(handle, #name)); \
  if (kit.name == nullptr) { \
    error = "Missing symbol '" #name "'"; \
    return; \
  }
    BARE_KIT_FUNCTIONS(V)
#undef V
  });

  if (!error.empty()) {
    throw JSError(rt, "Bare Kit is not available, so make sure the application includes bare-kit-swift or bare-kit-kotlin (" + error + ")");
  }
}

struct BareKitBuffer : MutableBuffer {
  BareKitBuffer(size_t len) : data_(len) {}

  size_t
  size() const override {
    return data_.size();
  }

  uint8_t *
  data() override {
    return data_.data();
  }

  void
  resize(size_t len) {
    data_.resize(len);
  }

private:
  std::vector<uint8_t> data_;
};

// The lifecycle callbacks of a worklet run on its thread and may still fire
// while it tears down after being destroyed, so they hold on to their own
// state until the worklet exits.
struct BareKitEvents {
  AsyncCallback<int> on_suspend;
  AsyncCallback<int> on_wakeup;
  AsyncCallback<> on_idle;
  AsyncCallback<> on_resume;
};

struct BareKitWorklet : HostObject {
  bare_worklet_t *worklet = nullptr;
  bare_ipc_t *ipc = nullptr;
  bare_ipc_poll_t *poll = nullptr;

  AsyncCallback<> on_terminate;
  AsyncCallback<bool, bool> on_poll;

  std::shared_ptr<BareKitEvents> events;

  bool started = false;
  bool terminated = false;

  std::optional<std::string> id;

  static std::mutex lock;
  static std::map<std::string, BareKitWorklet *> worklets;

  BareKitWorklet(
    Runtime &rt,
    std::optional<String> id,
    size_t memoryLimit,
    Function &&on_terminate,
    Function &&on_poll,
    Function &&on_suspend,
    Function &&on_wakeup,
    Function &&on_idle,
    Function &&on_resume,
    std::shared_ptr<CallInvoker> jsInvoker
  )
      : on_terminate(rt, std::move(on_terminate), jsInvoker),
        on_poll(rt, std::move(on_poll), jsInvoker),
        events(std::make_shared<BareKitEvents>(BareKitEvents{
          AsyncCallback<int>(rt, std::move(on_suspend), jsInvoker),
          AsyncCallback<int>(rt, std::move(on_wakeup), jsInvoker),
          AsyncCallback<>(rt, std::move(on_idle), jsInvoker),
          AsyncCallback<>(rt, std::move(on_resume), jsInvoker),
        })) {
    int err;

    load(rt);

    if (id) this->id = id->utf8(rt);

    bare_worklet_options_t options = {.version = 0, .memory_limit = memoryLimit};

#if defined(__ANDROID__)
    JNIEnv *env = facebook::jni::Environment::current();

    JavaVM *vm;
    err = env->GetJavaVM(&vm);
    assert(err == 0);

    jobject context = nullptr;

    jclass provider = env->FindClass("to/holepunch/bare/kit/react/BareKitProvider");

    if (provider != nullptr) {
      jmethodID get_application_context = env->GetStaticMethodID(provider, "getApplicationContext", "()Landroid/content/Context;");

      if (get_application_context != nullptr) context = env->CallStaticObjectMethod(provider, get_application_context);

      env->DeleteLocalRef(provider);
    }

    if (context == nullptr) {
      env->ExceptionClear();

      throw JSError(rt, "The application context could not be found");
    }

    options.platform.android.jvm = vm;
    options.platform.android.context = context;
#endif

    err = kit.bare_worklet_create(&options, &worklet);

#if defined(__ANDROID__)
    env->DeleteLocalRef(context);
#endif

    if (err < 0) throw JSError(rt, "Worklet could not be created");

    err = kit.bare_ipc_create(worklet, &ipc);
    assert(err == 0);

    err = kit.bare_ipc_poll_create(ipc, &poll);
    assert(err == 0);

    kit.bare_ipc_poll_set_data(poll, this);
  }

  ~BareKitWorklet() {
    terminate();
  }

  void
  start(Runtime &rt, String filename, const void *source, size_t len, Array args) {
    int err;

    if (started || terminated) return;

    // Released once the worklet exits, which it only does once started.
    auto data = new std::shared_ptr<BareKitEvents>(events);

    err = kit.bare_worklet_on_suspend(worklet, on_suspend_, data);
    assert(err == 0);

    err = kit.bare_worklet_on_wakeup(worklet, on_wakeup_, data);
    assert(err == 0);

    err = kit.bare_worklet_on_idle(worklet, on_idle_, data);
    assert(err == 0);

    err = kit.bare_worklet_on_resume(worklet, on_resume_, data);
    assert(err == 0);

    err = kit.bare_worklet_on_exit(worklet, on_exit_, data);
    assert(err == 0);

    auto string = filename.utf8(rt);

    auto argv = std::vector<std::string>(args.size(rt));

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      argv[i] = args.getValueAtIndex(rt, i).getString(rt).utf8(rt);
    }

    auto pointers = std::vector<const char *>(argv.size());

    std::transform(argv.begin(), argv.end(), pointers.begin(), [](const std::string &arg) { return arg.c_str(); });

    err = kit.bare_worklet_start(worklet, string.c_str(), source, len, int(pointers.size()), pointers.data());

    if (err < 0) {
      delete data;

      throw JSError(rt, "Worklet could not be started");
    }

    started = true;

    if (id) {
      std::unique_lock guard(lock);

      auto previous = worklets[id.value()];

      worklets[id.value()] = this;

      if (previous) {
        previous->on_terminate.call([=](Runtime &rt, Function &function) {
          function.call(rt);
        });
      }
    }
  }

  std::optional<Object>
  read(Runtime &rt) {
    if (terminated) return std::nullopt;

    auto buffer = std::make_shared<BareKitBuffer>(BARE_IPC_READ_BUFFER_SIZE);

    int len = kit.bare_ipc_read(ipc, buffer->data(), buffer->size());

    if (len == bare_ipc_would_block) return std::nullopt;

    assert(len >= 0);

    buffer->resize(size_t(len));

    return ArrayBuffer(rt, buffer);
  }

  int
  write(Runtime &rt, Object data, size_t offset, size_t length) {
    if (terminated) return -1;

    auto base = data.getArrayBuffer(rt).data(rt) + offset;

    int len = kit.bare_ipc_write(ipc, base, length);

    if (len == bare_ipc_would_block) return 0;
    if (len == bare_ipc_error) return -1;

    return len;
  }

  void
  end() {
    if (terminated) return;

    kit.bare_ipc_end(ipc);
  }

  void
  update(bool readable, bool writable) {
    int err;

    if (terminated) return;

    int events = 0;

    if (readable) events |= bare_ipc_readable;
    if (writable) events |= bare_ipc_writable;

    if (events) {
      err = kit.bare_ipc_poll_start(poll, events, on_poll_);
      assert(err == 0);
    } else {
      err = kit.bare_ipc_poll_stop(poll);
      assert(err == 0);
    }
  }

  void
  suspend(int linger) {
    if (terminated) return;

    kit.bare_worklet_suspend(worklet, linger);
  }

  void
  resume() {
    if (terminated) return;

    kit.bare_worklet_resume(worklet);
  }

  void
  wakeup(int deadline) {
    if (terminated) return;

    kit.bare_worklet_wakeup(worklet, deadline);
  }

  void
  terminate() {
    if (terminated) return;

    if (id) {
      std::unique_lock guard(lock);

      auto current = worklets[id.value()];

      if (current == this) worklets.erase(id.value());
    }

    terminated = true;

    kit.bare_ipc_poll_destroy(poll);
    kit.bare_ipc_destroy(ipc);
    kit.bare_worklet_destroy(worklet);

    worklet = nullptr;
    ipc = nullptr;
    poll = nullptr;
  }

private:
  static void
  on_poll_(bare_ipc_poll_t *poll, int events) {
    auto worklet = static_cast<BareKitWorklet *>(kit.bare_ipc_poll_get_data(poll));

    worklet->on_poll.call([=](Runtime &rt, Function &function) {
      function.call(rt, (events & bare_ipc_readable) != 0, (events & bare_ipc_writable) != 0);
    });
  }

  static BareKitEvents &
  events_from(void *data) {
    return **static_cast<std::shared_ptr<BareKitEvents> *>(data);
  }

  static void
  on_suspend_(int linger, void *data) {
    events_from(data).on_suspend.call([=](Runtime &rt, Function &function) {
      function.call(rt, linger);
    });
  }

  static void
  on_wakeup_(int deadline, void *data) {
    events_from(data).on_wakeup.call([=](Runtime &rt, Function &function) {
      function.call(rt, deadline);
    });
  }

  static void
  on_idle_(void *data) {
    events_from(data).on_idle.call([=](Runtime &rt, Function &function) {
      function.call(rt);
    });
  }

  static void
  on_resume_(void *data) {
    events_from(data).on_resume.call([=](Runtime &rt, Function &function) {
      function.call(rt);
    });
  }

  static void
  on_exit_(int exit_code, void *data) {
    delete static_cast<std::shared_ptr<BareKitEvents> *>(data);
  }
};

std::mutex BareKitWorklet::lock;

std::map<std::string, BareKitWorklet *> BareKitWorklet::worklets;

} // namespace

BareKitModule::BareKitModule(std::shared_ptr<CallInvoker> jsInvoker) : NativeBareKitCxxSpec(std::move(jsInvoker)) {}

Object
BareKitModule::init(
  Runtime &rt,
  std::optional<String> id,
  double memoryLimit,
  Function on_terminate,
  Function on_poll,
  Function on_suspend,
  Function on_wakeup,
  Function on_idle,
  Function on_resume
) {
  auto worklet = std::make_shared<BareKitWorklet>(
    rt,
    std::move(id),
    size_t(memoryLimit),
    std::move(on_terminate),
    std::move(on_poll),
    std::move(on_suspend),
    std::move(on_wakeup),
    std::move(on_idle),
    std::move(on_resume),
    jsInvoker_
  );

  return Object::createFromHostObject(rt, std::move(worklet));
}

void
BareKitModule::update(Runtime &rt, Object handle, bool readable, bool writable) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->update(readable, writable);
}

void
BareKitModule::startFile(Runtime &rt, Object handle, String filename, Array args) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->start(rt, std::move(filename), nullptr, 0, std::move(args));
}

void
BareKitModule::startBytes(Runtime &rt, Object handle, String filename, Object source, double offset, double length, Array args) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto base = source.getArrayBuffer(rt).data(rt) + size_t(offset);

  worklet->start(rt, std::move(filename), base, size_t(length), std::move(args));
}

void
BareKitModule::startUTF8(Runtime &rt, Object handle, String filename, String source, Array args) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto utf8 = source.utf8(rt);

  worklet->start(rt, std::move(filename), utf8.data(), utf8.size(), std::move(args));
}

std::optional<Object>
BareKitModule::read(Runtime &rt, Object handle) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  return worklet->read(rt);
}

double
BareKitModule::write(Runtime &rt, Object handle, Object data, double offset, double length) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  return double(worklet->write(rt, std::move(data), size_t(offset), size_t(length)));
}

void
BareKitModule::end(Runtime &rt, Object handle) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->end();
}

void
BareKitModule::suspend(Runtime &rt, Object handle, double linger) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->suspend(int(linger));
}

void
BareKitModule::resume(Runtime &rt, Object handle) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->resume();
}

void
BareKitModule::wakeup(Runtime &rt, Object handle, double deadline) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->wakeup(int(deadline));
}

void
BareKitModule::terminate(Runtime &rt, Object handle) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->terminate();
}

} // namespace facebook::react
