#include <assert.h>
#include <jni.h>
#include <memory>
#include <utility>

#include <ReactCommon/JavaTurboModule.h>

#include "BareKitModule.h"

extern "C" {
int
bare_worklet_android_attach_vm(bare_worklet_t *worklet, JavaVM *java_vm);
}

namespace facebook::react {

namespace {

static JavaVM *
currentJavaVm() {
  JNIEnv *env = jni::Environment::current();

  JavaVM *javaVm;
  int err = env->GetJavaVM(&javaVm);
  assert(err == JNI_OK);

  return javaVm;
}

static int
configureAndroidWorklet(bare_worklet_t *worklet, void *data) {
  return bare_worklet_android_attach_vm(worklet, static_cast<JavaVM *>(data));
}

} // namespace

BareKitModule::BareKitModule(std::shared_ptr<CallInvoker> jsInvoker) : BareKitModule(std::move(jsInvoker), configureAndroidWorklet, currentJavaVm()) {}

} // namespace facebook::react
