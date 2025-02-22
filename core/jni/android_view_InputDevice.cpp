/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/Parcel.h>
#include <input/Input.h>

#include <android_runtime/AndroidRuntime.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>

#include <nativehelper/ScopedLocalRef.h>

#include "android_view_InputDevice.h"
#include "android_view_KeyCharacterMap.h"

#include "core_jni_helpers.h"

namespace android {

static struct {
    jclass clazz;

    jmethodID ctor;
    jmethodID addMotionRange;
} gInputDeviceClassInfo;

jobject android_view_InputDevice_create(JNIEnv* env, const InputDeviceInfo& deviceInfo) {
    ScopedLocalRef<jstring> nameObj(env, env->NewStringUTF(deviceInfo.getDisplayName().c_str()));
    if (!nameObj.get()) {
        return NULL;
    }

    ScopedLocalRef<jstring> descriptorObj(env,
            env->NewStringUTF(deviceInfo.getIdentifier().descriptor.c_str()));
    if (!descriptorObj.get()) {
        return NULL;
    }

    sp<KeyCharacterMap> map = deviceInfo.getKeyCharacterMap();
    if (map != nullptr) {
        Parcel parcel;
        map->writeToParcel(&parcel);
        parcel.setDataPosition(0);
        map = map->readFromParcel(&parcel);
    }

    ScopedLocalRef<jobject> kcmObj(env,
                                   android_view_KeyCharacterMap_create(env, deviceInfo.getId(),
                                                                       map));
    if (!kcmObj.get()) {
        return NULL;
    }

    const InputDeviceIdentifier& ident = deviceInfo.getIdentifier();

    // Not sure why, but JNI is complaining when I pass this through directly.
    jboolean hasMic = deviceInfo.hasMic() ? JNI_TRUE : JNI_FALSE;

    ScopedLocalRef<jobject> inputDeviceObj(env, env->NewObject(gInputDeviceClassInfo.clazz,
                gInputDeviceClassInfo.ctor, deviceInfo.getId(), deviceInfo.getGeneration(),
                deviceInfo.getControllerNumber(), nameObj.get(),
                static_cast<int32_t>(ident.vendor), static_cast<int32_t>(ident.product),
                descriptorObj.get(), deviceInfo.isExternal(), deviceInfo.getSources(),
                deviceInfo.getKeyboardType(), kcmObj.get(), deviceInfo.hasVibrator(),
                hasMic, deviceInfo.hasButtonUnderPad()));

    const std::vector<InputDeviceInfo::MotionRange>& ranges = deviceInfo.getMotionRanges();
    for (const InputDeviceInfo::MotionRange& range: ranges) {
        env->CallVoidMethod(inputDeviceObj.get(), gInputDeviceClassInfo.addMotionRange, range.axis,
                range.source, range.min, range.max, range.flat, range.fuzz, range.resolution);
        if (env->ExceptionCheck()) {
            return NULL;
        }
    }

    return env->NewLocalRef(inputDeviceObj.get());
}


int register_android_view_InputDevice(JNIEnv* env)
{
    gInputDeviceClassInfo.clazz = FindClassOrDie(env, "android/view/InputDevice");
    gInputDeviceClassInfo.clazz = MakeGlobalRefOrDie(env, gInputDeviceClassInfo.clazz);

    gInputDeviceClassInfo.ctor = GetMethodIDOrDie(env, gInputDeviceClassInfo.clazz, "<init>",
            "(IIILjava/lang/String;IILjava/lang/String;ZIILandroid/view/KeyCharacterMap;ZZZ)V");

    gInputDeviceClassInfo.addMotionRange = GetMethodIDOrDie(env, gInputDeviceClassInfo.clazz,
            "addMotionRange", "(IIFFFFF)V");

    return 0;
}

}; // namespace android
