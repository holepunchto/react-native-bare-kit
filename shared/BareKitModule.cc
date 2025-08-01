#include <algorithm>
#include <optional>
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
bare_worklet_start(bare_worklet_t *worklet, const char *filename, const uv_buf_t *source, bare_worklet_finalize_cb finalize, void *finalize_hint, int argc, const char *argv[]);

int
bare_worklet_suspend(bare_worklet_t *worklet, int linger);

int
bare_worklet_resume(bare_worklet_t *worklet);

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

namespace {

struct BareKitWorklet : HostObject {
  bare_worklet_t *worklet;
  bare_ipc_t *ipc;
  bare_ipc_poll_t *poll;

  AsyncCallback<bool, bool> on_poll;

  BareKitWorklet(Runtime &rt, Function &&on_poll, std::shared_ptr<CallInvoker> jsInvoker) : on_poll(rt, std::move(on_poll), jsInvoker) {}
};

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

} // namespace

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
jsi_to_buffer(Runtime &rt, const String &value) {
  auto utf8 = value.utf8(rt);

  return {strdup(utf8.c_str()), utf8.size()};
}

static inline char *
jsi_to_string(Runtime &rt, const String &value) {
  return strdup(value.utf8(rt).c_str());
}

} // namespace

namespace facebook::react {

BareKitModule::BareKitModule(std::shared_ptr<CallInvoker> jsInvoker) : NativeBareKitCxxSpec(std::move(jsInvoker)) {}

Object
BareKitModule::init(Runtime &rt, double memoryLimit, std::optional<String> assets, Function poll) {
  int err;

  auto worklet = std::make_shared<BareKitWorklet>(rt, std::move(poll), jsInvoker_);

  err = bare_worklet_alloc(&worklet->worklet);
  assert(err == 0);

  err = bare_ipc_alloc(&worklet->ipc);
  assert(err == 0);

  err = bare_ipc_poll_alloc(&worklet->poll);
  assert(err == 0);

  bare_ipc_poll_set_data(worklet->poll, worklet.get());

  bare_worklet_options_t options;

  options.memory_limit = static_cast<size_t>(memoryLimit);

  if (assets) {
    options.assets = jsi_to_string(rt, assets.value());
  } else {
    options.assets = nullptr;
  }

  err = bare_worklet_init(worklet->worklet, &options);
  assert(err == 0);

  free(const_cast<char *>(options.assets));

  return Object::createFromHostObject(rt, std::move(worklet));
}

namespace {

static void
bare_kit_module__on_poll(bare_ipc_poll_t *poll, int events) {
  auto worklet = static_cast<BareKitWorklet *>(bare_ipc_poll_get_data(poll));

  worklet->on_poll.call([=](jsi::Runtime &rt, jsi::Function &function) {
    function.call(rt, (events & bare_ipc_readable) != 0, (events & bare_ipc_writable) != 0);
  });
}

} // namespace

void
BareKitModule::update(Runtime &rt, Object handle, bool readable, bool writable) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  int events = 0;

  if (readable) events |= bare_ipc_readable;
  if (writable) events |= bare_ipc_writable;

  if (events) {
    err = bare_ipc_poll_start(worklet->poll, events, bare_kit_module__on_poll);
    assert(err == 0);
  } else {
    err = bare_ipc_poll_stop(worklet->poll);
    assert(err == 0);
  }
}

void
BareKitModule::startFile(Runtime &rt, Object handle, String filename, Array args) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto string = jsi_to_string(rt, filename);

  auto argv = std::vector<const char *>(args.size(rt));

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    argv[i] = jsi_to_string(rt, args.getValueAtIndex(rt, i).getString(rt));
  }

  err = bare_worklet_start(worklet->worklet, string, nullptr, nullptr, nullptr, argv.size(), argv.data());
  assert(err == 0);

  free(string);

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    free(const_cast<char *>(argv[i]));
  }

  err = bare_ipc_init(worklet->ipc, worklet->worklet);
  assert(err == 0);

  err = bare_ipc_poll_init(worklet->poll, worklet->ipc);
  assert(err == 0);
}

void
BareKitModule::startBytes(Runtime &rt, Object handle, String filename, Object source, double offset, double length, Array args) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto string = jsi_to_string(rt, filename);

  auto buffer = jsi_to_buffer(rt, source.getArrayBuffer(rt), size_t(offset), size_t(length));

  auto argv = std::vector<const char *>(args.size(rt));

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    argv[i] = jsi_to_string(rt, args.getValueAtIndex(rt, i).getString(rt));
  }

  err = bare_worklet_start(worklet->worklet, string, &buffer, nullptr, nullptr, argv.size(), argv.data());
  assert(err == 0);

  free(string);

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    free(const_cast<char *>(argv[i]));
  }

  err = bare_ipc_init(worklet->ipc, worklet->worklet);
  assert(err == 0);

  err = bare_ipc_poll_init(worklet->poll, worklet->ipc);
  assert(err == 0);
}

namespace {

static void
bare_kit_module__on_finalize(bare_worklet_t *, const uv_buf_t *source, void *) {
  free(source->base);
}

} // namespace

void
BareKitModule::startUTF8(Runtime &rt, Object handle, String filename, String source, Array args) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto string = jsi_to_string(rt, filename);

  auto buffer = jsi_to_buffer(rt, source);

  auto argv = std::vector<const char *>(args.size(rt));

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    argv[i] = jsi_to_string(rt, args.getValueAtIndex(rt, i).getString(rt));
  }

  err = bare_worklet_start(worklet->worklet, string, &buffer, bare_kit_module__on_finalize, nullptr, argv.size(), argv.data());
  assert(err == 0);

  free(string);

  for (size_t i = 0, n = argv.size(); i < n; i++) {
    free(const_cast<char *>(argv[i]));
  }

  err = bare_ipc_init(worklet->ipc, worklet->worklet);
  assert(err == 0);

  err = bare_ipc_poll_init(worklet->poll, worklet->ipc);
  assert(err == 0);
}

std::optional<Object>
BareKitModule::read(Runtime &rt, Object handle) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  uint8_t *data;
  size_t len;
  err = bare_ipc_read(worklet->ipc, reinterpret_cast<void **>(&data), &len);
  assert(err == 0 || err == bare_ipc_would_block);

  if (err == bare_ipc_would_block) {
    return std::nullopt;
  }

  auto buffer = std::make_shared<BareKitBuffer>(len);

  std::copy(data, data + len, buffer->data());

  return ArrayBuffer(rt, buffer);
}

double
BareKitModule::write(Runtime &rt, Object handle, Object data, double offset, double length) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  auto buffer = jsi_to_buffer(rt, data.getArrayBuffer(rt), size_t(offset), size_t(length));

  err = bare_ipc_write(worklet->ipc, buffer.base, buffer.len);
  assert(err >= 0 || err == bare_ipc_would_block);

  if (err == bare_ipc_would_block) {
    return 0;
  }

  return double(err);
}

void
BareKitModule::suspend(Runtime &rt, Object handle, double linger) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  err = bare_worklet_suspend(worklet->worklet, int(linger));
  assert(err == 0);
}

void
BareKitModule::resume(Runtime &rt, Object handle) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  err = bare_worklet_resume(worklet->worklet);
  assert(err == 0);
}

void
BareKitModule::terminate(Runtime &rt, Object handle) {
  int err;

  auto worklet = handle.getHostObject<BareKitWorklet>(rt);

  err = bare_worklet_terminate(worklet->worklet);
  assert(err == 0);

  bare_ipc_poll_destroy(worklet->poll);
  bare_ipc_destroy(worklet->ipc);
  bare_worklet_destroy(worklet->worklet);

  free(worklet->worklet);
  free(worklet->ipc);
  free(worklet->poll);
}

} // namespace facebook::react
