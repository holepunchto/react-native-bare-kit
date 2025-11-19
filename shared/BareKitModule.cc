#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <react/bridging/Bridging.h>

#include "BareKitModule.h"

using namespace facebook::jsi;
using namespace facebook::react;

extern "C" {
typedef struct uv_buf_t {
  char *base;
  size_t len;
} uv_buf_t;
}

namespace facebook::react {

namespace {

static inline uv_buf_t
jsi_to_buffer(Runtime &rt, const ArrayBuffer &value) {
  return {reinterpret_cast<char *>(value.data(rt)), value.size(rt)};
}

static inline uv_buf_t
jsi_to_buffer(Runtime &rt, const ArrayBuffer &value, size_t offset, size_t length) {
  return {reinterpret_cast<char *>(value.data(rt)) + offset, length};
}

static inline uv_buf_t
jsi_to_buffer_owned(Runtime &rt, const String &value) {
  auto utf8 = value.utf8(rt);

  return {strdup(utf8.c_str()), utf8.size()};
}

static inline char *
jsi_to_string_owned(Runtime &rt, const String &value) {
  return strdup(value.utf8(rt).c_str());
}

} // namespace

} // namespace facebook::react

extern "C" {
typedef struct bare_worklet_s bare_worklet_t;
typedef struct bare_worklet_options_s bare_worklet_options_t;
typedef struct bare_ipc_s bare_ipc_t;
typedef struct bare_ipc_poll_s bare_ipc_poll_t;

typedef void (*bare_worklet_finalize_cb)(bare_worklet_t *, const uv_buf_t *source, void *finalize_hint);
typedef void (*bare_ipc_poll_cb)(bare_ipc_poll_t *, int events);

struct bare_worklet_options_s {
  size_t memory_limit;
  const char *assets;
};

enum {
  bare_ipc_readable = 0x1,
  bare_ipc_writable = 0x2,
};

enum {
  bare_ipc_would_block = -1,
  bare_ipc_error = -2,
};

int
bare_worklet_alloc(bare_worklet_t **result);

int
bare_worklet_init(bare_worklet_t *worklet, const bare_worklet_options_t *options);

void
bare_worklet_destroy(bare_worklet_t *worklet);

void *
bare_worklet_get_data(bare_worklet_t *worklet);

void
bare_worklet_set_data(bare_worklet_t *worklet, void *data);

int
bare_worklet_start(bare_worklet_t *worklet, const char *filename, const uv_buf_t *source, int argc, const char *argv[]);

int
bare_worklet_suspend(bare_worklet_t *worklet, int linger);

int
bare_worklet_resume(bare_worklet_t *worklet);

int
bare_worklet_wakeup(bare_worklet_t *worklet, int deadline);

int
bare_worklet_terminate(bare_worklet_t *worklet);

int
bare_ipc_alloc(bare_ipc_t **result);

int
bare_ipc_init(bare_ipc_t *ipc, bare_worklet_t *worklet);

void
bare_ipc_destroy(bare_ipc_t *ipc);

int
bare_ipc_read(bare_ipc_t *ipc, void **data, size_t *len);

int
bare_ipc_write(bare_ipc_t *ipc, const void *data, size_t len);

int
bare_ipc_poll_alloc(bare_ipc_poll_t **result);

int
bare_ipc_poll_init(bare_ipc_poll_t *poll, bare_ipc_t *ipc);

void
bare_ipc_poll_destroy(bare_ipc_poll_t *poll);

void *
bare_ipc_poll_get_data(bare_ipc_poll_t *poll);

void
bare_ipc_poll_set_data(bare_ipc_poll_t *poll, void *data);

int
bare_ipc_poll_start(bare_ipc_poll_t *poll, int events, bare_ipc_poll_cb cb);

int
bare_ipc_poll_stop(bare_ipc_poll_t *poll);
}

namespace facebook::react {

namespace {

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

private:
  std::vector<uint8_t> data_;
};

struct BareKitWorklet : HostObject {
  bare_worklet_t *worklet = nullptr;
  bare_ipc_t *ipc = nullptr;
  bare_ipc_poll_t *poll = nullptr;

  AsyncCallback<bool, bool> on_poll;

  bool started = false;
  bool terminated = false;

  std::optional<std::string> id;

  static std::mutex lock;
  static std::map<std::string, BareKitWorklet *> worklets;

  BareKitWorklet(Runtime &rt, std::optional<String> id, int memoryLimit, std::optional<String> assets, Function &&on_poll, std::shared_ptr<CallInvoker> jsInvoker) : on_poll(rt, std::move(on_poll), jsInvoker) {
    int err;

    if (id) this->id = id->utf8(rt);

    err = bare_worklet_alloc(&worklet);
    assert(err == 0);

    err = bare_ipc_alloc(&ipc);
    assert(err == 0);

    err = bare_ipc_poll_alloc(&poll);
    assert(err == 0);

    bare_ipc_poll_set_data(poll, this);

    bare_worklet_options_t options;

    options.memory_limit = static_cast<size_t>(memoryLimit);

    if (assets) {
      options.assets = jsi_to_string_owned(rt, assets.value());
    } else {
      options.assets = nullptr;
    }

    err = bare_worklet_init(worklet, &options);
    assert(err == 0);

    free(const_cast<char *>(options.assets));
  }

  ~BareKitWorklet() {
    terminate();
  }

  void
  start() {
    int err;

    err = bare_ipc_init(ipc, worklet);
    assert(err == 0);

    err = bare_ipc_poll_init(poll, ipc);
    assert(err == 0);

    if (id) {
      std::unique_lock guard(lock);

      auto previous = worklets[id.value()];

      worklets[id.value()] = this;

      if (previous) {
        err = bare_worklet_terminate(previous->worklet);
        assert(err == 0);
      }
    }
  }

  void
  start(Runtime &rt, Object handle, String filename, Array args) {
    int err;

    if (started || terminated) return;

    started = true;

    auto string = jsi_to_string_owned(rt, filename);

    auto argv = std::vector<const char *>(args.size(rt));

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      argv[i] = jsi_to_string_owned(rt, args.getValueAtIndex(rt, i).getString(rt));
    }

    err = bare_worklet_start(worklet, string, nullptr, argv.size(), argv.data());
    assert(err == 0);

    free(string);

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      free(const_cast<char *>(argv[i]));
    }

    start();
  }

  void
  start(Runtime &rt, Object handle, String filename, Object source, size_t offset, size_t length, Array args) {
    int err;

    if (started || terminated) return;

    started = true;

    auto string = jsi_to_string_owned(rt, filename);

    auto buffer = jsi_to_buffer(rt, source.getArrayBuffer(rt), offset, length);

    auto argv = std::vector<const char *>(args.size(rt));

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      argv[i] = jsi_to_string_owned(rt, args.getValueAtIndex(rt, i).getString(rt));
    }

    err = bare_worklet_start(worklet, string, &buffer, argv.size(), argv.data());
    assert(err == 0);

    free(string);

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      free(const_cast<char *>(argv[i]));
    }

    start();
  }

  void
  start(Runtime &rt, Object handle, String filename, String source, Array args) {
    int err;

    if (started || terminated) return;

    started = true;

    auto string = jsi_to_string_owned(rt, filename);

    auto buffer = jsi_to_buffer_owned(rt, source);

    auto argv = std::vector<const char *>(args.size(rt));

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      argv[i] = jsi_to_string_owned(rt, args.getValueAtIndex(rt, i).getString(rt));
    }

    err = bare_worklet_start(worklet, string, &buffer, argv.size(), argv.data());
    assert(err == 0);

    free(buffer.base);

    free(string);

    for (size_t i = 0, n = argv.size(); i < n; i++) {
      free(const_cast<char *>(argv[i]));
    }

    start();
  }

  std::optional<Object>
  read(Runtime &rt, Object handle) {
    int err;

    if (!started || terminated) return std::nullopt;

    uint8_t *data;
    size_t len;
    err = bare_ipc_read(ipc, reinterpret_cast<void **>(&data), &len);
    assert(err == 0 || err == bare_ipc_would_block);

    if (err == bare_ipc_would_block) {
      return std::nullopt;
    }

    auto buffer = std::make_shared<BareKitBuffer>(len);

    std::copy(data, data + len, buffer->data());

    return ArrayBuffer(rt, buffer);
  }

  int
  write(Runtime &rt, Object handle, Object data, size_t offset, size_t length) {
    int err;

    auto buffer = jsi_to_buffer(rt, data.getArrayBuffer(rt), offset, length);

    err = bare_ipc_write(ipc, buffer.base, buffer.len);
    assert(err >= 0 || err == bare_ipc_would_block);

    if (err == bare_ipc_would_block) {
      return 0;
    }

    return err;
  }

  void
  update(bool readable, bool writable) {
    int err;

    if (terminated) return;

    int events = 0;

    if (readable) events |= bare_ipc_readable;
    if (writable) events |= bare_ipc_writable;

    if (events) {
      err = bare_ipc_poll_start(poll, events, on_poll_);
      assert(err == 0);
    } else {
      err = bare_ipc_poll_stop(poll);
      assert(err == 0);
    }
  }

  void
  suspend(int linger) {
    int err;

    if (!started || terminated) return;

    err = bare_worklet_suspend(worklet, linger);
    assert(err == 0);
  }

  void
  resume() {
    int err;

    if (!started || terminated) return;

    err = bare_worklet_resume(worklet);
    assert(err == 0);
  }

  void
  wakeup(int deadline) {
    int err;

    if (!started || terminated) return;

    err = bare_worklet_wakeup(worklet, int(deadline));
    assert(err == 0);
  }

  void
  terminate() {
    int err;

    if (terminated) return;

    if (id) {
      std::unique_lock guard(lock);

      auto current = worklets[id.value()];

      if (current == this) worklets.erase(id.value());
    }

    terminated = true;

    if (started) {
      err = bare_worklet_terminate(worklet);
      assert(err == 0);

      bare_ipc_poll_destroy(poll);
      bare_ipc_destroy(ipc);

      free(ipc);
      free(poll);
    }

    bare_worklet_destroy(worklet);

    free(worklet);

    worklet = nullptr;
    ipc = nullptr;
    poll = nullptr;
  }

private:
  static void
  on_poll_(bare_ipc_poll_t *poll, int events) {
    auto worklet = static_cast<BareKitWorklet *>(bare_ipc_poll_get_data(poll));

    worklet->on_poll.call([=](Runtime &rt, Function &function) {
      function.call(rt, (events & bare_ipc_readable) != 0, (events & bare_ipc_writable) != 0);
    });
  }
};

std::mutex BareKitWorklet::lock;

std::map<std::string, BareKitWorklet *> BareKitWorklet::worklets;

} // namespace

BareKitModule::BareKitModule(std::shared_ptr<CallInvoker> jsInvoker) : NativeBareKitCxxSpec(std::move(jsInvoker)) {}

Object
BareKitModule::init(Runtime &rt, std::optional<String> id, double memoryLimit, std::optional<String> assets, Function on_poll) {
  auto worklet = std::make_shared<BareKitWorklet>(rt, std::move(id), int(memoryLimit), std::move(assets), std::move(on_poll), jsInvoker_);

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

  worklet->start(rt, std::move(handle), std::move(filename), std::move(args));
}

void
BareKitModule::startBytes(Runtime &rt, Object handle, String filename, Object source, double offset, double length, Array args) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->start(rt, std::move(handle), std::move(filename), std::move(source), size_t(offset), size_t(length), std::move(args));
}

void
BareKitModule::startUTF8(Runtime &rt, Object handle, String filename, String source, Array args) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  worklet->start(rt, std::move(handle), std::move(filename), std::move(source), std::move(args));
}

std::optional<Object>
BareKitModule::read(Runtime &rt, Object handle) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  return worklet->read(rt, std::move(handle));
}

double
BareKitModule::write(Runtime &rt, Object handle, Object data, double offset, double length) {
  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  return double(worklet->write(rt, std::move(handle), std::move(data), size_t(offset), size_t(length)));
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
