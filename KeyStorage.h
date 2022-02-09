/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_VOLD_KEYSTORAGE_H
#define ANDROID_VOLD_KEYSTORAGE_H

#include "KeyBuffer.h"

#include <string>

namespace android {
namespace vold {

enum {
    FBE_ERR_OK = 0,
    FBE_ERR_STORE_KEY = 1,
    FBE_ERR_RETRIEVE_KEY = 2,
    FBE_ERR_RANDOM_KEY = 3,
    FBE_ERR_INSTALL_KEY = 4,
    FBE_ERR_KEYMASTER = 5,
    FBE_ERR_GENERATE_KEY = 6,
    FBE_ERR_ENCRYPT_KEY = 7,
    FBE_ERR_DECRYPT_KEY = 8,
    FBE_ERR_WRITE_VERSION = 9,
    FBE_ERR_WRITE_SEC = 10,
    FBE_ERR_WRITE_STRETCHING = 11,
    FBE_ERR_WRITE_SALT = 12,
    FBE_ERR_WRITE_BLOB = 13,
    FBE_ERR_WRITE_ENCRYPTED_KEY = 14,
    FBE_ERR_READ_VERSION = 15,
    FBE_ERR_READ_SEC = 16,
    FBE_ERR_READ_STRETCHING = 17,
    FBE_ERR_READ_SALT = 18,
    FBE_ERR_READ_BLOB = 19,
    FBE_ERR_READ_ENCRYPTED_KEY = 20,
    FBE_ERR_PATH_MKDIR = 21,
    FBE_ERR_PATH_EXTSTS = 22,
    FBE_ERR_PATH_RENAME = 23,
    FBE_ERR_RANDOM_READ = 24,
    FBE_ERR_GENERATE_APPID = 25,
    FBE_ERR_VERSION_MISMATCH = 26,
    FBE_ERR_ENCRYPT_NO_KEYMASTER = 27,
    FBE_ERR_DECRYPT_NO_KEYMASTER = 28,
};

// Represents the information needed to decrypt a disk encryption key.
// If "token" is nonempty, it is passed in as a required Gatekeeper auth token.
// If "token" and "secret" are nonempty, "secret" is appended to the application-specific
// binary needed to unlock.
// If only "secret" is nonempty, it is used to decrypt in a non-Keymaster process.
class KeyAuthentication {
  public:
    KeyAuthentication(const std::string& t, const std::string& s) : token{t}, secret{s} {};

    bool usesKeymaster() const { return !token.empty() || secret.empty(); };

    const std::string token;
    const std::string secret;
};

extern int err_code;
extern const KeyAuthentication kEmptyAuthentication;

// Checks if path "path" exists.
bool pathExists(const std::string& path);

bool createSecdiscardable(const std::string& path, std::string* hash);
bool readSecdiscardable(const std::string& path, std::string* hash);

// Create a directory at the named path, and store "key" in it,
// in such a way that it can only be retrieved via Keymaster and
// can be securely deleted.
// It's safe to move/rename the directory after creation.
bool storeKey(const std::string& dir, const KeyAuthentication& auth, const KeyBuffer& key);

// Create a directory at the named path, and store "key" in it as storeKey
// This version creates the key in "tmp_path" then atomically renames "tmp_path"
// to "key_path" thereby ensuring that the key is either stored entirely or
// not at all.
bool storeKeyAtomically(const std::string& key_path, const std::string& tmp_path,
                        const KeyAuthentication& auth, const KeyBuffer& key);

// Retrieve the key from the named directory.
bool retrieveKey(const std::string& dir, const KeyAuthentication& auth, KeyBuffer* key,
                 bool keepOld = false);

// Securely destroy the key stored in the named directory and delete the directory.
bool destroyKey(const std::string& dir);

bool runSecdiscardSingle(const std::string& file);
}  // namespace vold
}  // namespace android

#endif
