/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#pragma once

#include <encoding.h>
#include <storage/redis_metadata.h>

namespace redis {

inline constexpr auto kErrorInsufficientLength = "insufficient length while decoding metadata";

enum class SearchSubkeyType : uint8_t {
  // search global metadata
  PREFIXES = 1,

  // field metadata for different types
  TAG_FIELD_META = 64 + 1,

  // field indexing for different types
  TAG_FIELD = 128 + 1,
};

inline std::string ConstructSearchPrefixesSubkey() { return {(char)SearchSubkeyType::PREFIXES}; }

struct SearchPrefixesMetadata {
  std::vector<std::string> prefixes;

  void Encode(std::string *dst) const {
    for (const auto &prefix : prefixes) {
      PutFixed32(dst, prefix.size());
      dst->append(prefix);
    }
  }

  rocksdb::Status Decode(Slice *input) {
    uint32_t size = 0;

    while (GetFixed32(input, &size)) {
      if (input->size() < size) return rocksdb::Status::Corruption(kErrorInsufficientLength);
      prefixes.emplace_back(input->data(), size);
      input->remove_prefix(size);
    }

    return rocksdb::Status::OK();
  }
};

inline std::string ConstructTagFieldMetadataSubkey(std::string_view field_name) {
  std::string res = {(char)SearchSubkeyType::TAG_FIELD_META};
  res.append(field_name);
  return res;
}

struct SearchTagFieldMetadata {
  char separator;
  bool case_sensitive;

  void Encode(std::string *dst) const {
    PutFixed8(dst, separator);
    PutFixed8(dst, case_sensitive);
  }

  rocksdb::Status Decode(Slice *input) {
    if (input->size() < 8 + 8) {
      return rocksdb::Status::Corruption(kErrorInsufficientLength);
    }

    GetFixed8(input, (uint8_t *)&separator);
    GetFixed8(input, (uint8_t *)&case_sensitive);
    return rocksdb::Status::OK();
  }
};

inline std::string ConstructTagFieldSubkey(std::string_view field_name, std::string_view tag, std::string_view key) {
  std::string res = {(char)SearchSubkeyType::TAG_FIELD};
  PutFixed32(&res, field_name.size());
  res.append(field_name);
  PutFixed32(&res, tag.size());
  res.append(tag);
  PutFixed32(&res, key.size());
  res.append(key);
  return res;
}

}  // namespace redis
