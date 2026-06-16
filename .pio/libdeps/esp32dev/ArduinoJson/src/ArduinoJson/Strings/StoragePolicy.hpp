// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2026, Benoit BLANCHON
// MIT License

#pragma once

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

namespace StringStoragePolicy {

struct Link {};
struct Copy {};
struct LinkOrCopy {
  bool link;
};
}  // namespace StringStoragePolicy

ARDUINOJSON_END_PRIVATE_NAMESPACE
