#ifndef HARB_PARSER_H
#define HARB_PARSER_H

#include <vector>

#include <city.h>

#include "sparsehash/sparse_hash_set"
#include "rapidjson/reader.h"
#include "rapidjson/filereadstream.h"

#include "ruby_heap_obj.h"

namespace harb {

class Parser {
  struct eqstr {
    bool operator()(const char* s1, const char* s2) const {
      return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }
  };

  template<class T> class CityHasher;
  template<> class CityHasher<const char *> {
  public:
    std::size_t operator()(const char * s) const {
      return CityHash64(s, strlen(s));
    }
  };

  struct HeapDumpHandler {
      bool Null() { return true; }
      bool Bool(bool b);
      bool Int(int i) { return true; }
      bool Uint(unsigned u) { return true; }
      bool Int64(int64_t i) { return true; }
      bool Uint64(uint64_t u) { return true; }
      bool Double(double d) { return true; }
      bool RawNumber(const char* str, rapidjson::SizeType length, bool copy);
      bool String(const char* str, rapidjson::SizeType length, bool copy);
      bool StartObject();
      bool Key(const char* str, rapidjson::SizeType length, bool copy);
      bool EndObject(rapidjson::SizeType memberCount);
      bool StartArray();
      bool EndArray(rapidjson::SizeType elementCount);

      enum State {
        kStart = 0,
        kFinish,
        kInsideObject,
        kFinishObject,
        kAddress,
        kType,
        kReferences,
        kClass,
        kName,
        kMemsize,
        kFrozen,
        kShared,
        kValue,
        kSize,
        kLength,
        kStruct,
        kImemoType,
        kFlags,
        kRoot
      } state_;

      Parser *parser_;
      rapidjson::FileReadStream *stream_;
      RubyHeapObj *obj_;
      size_t obj_start_pos_, obj_end_pos_;
      std::vector<uint64_t> refs_to_;
  };

  typedef CityHasher<const char *> CityHasherString;
  typedef google::sparse_hash_set<const char *, CityHasherString, eqstr> StringSet;

  int32_t heap_obj_count_;
  StringSet intern_strings_;
  HeapDumpHandler handler_;
  FILE *f_;
  char *heap_obj_json_;
  size_t heap_obj_json_size_;

  const char * get_intern_string(const char *str);

public:

  Parser(FILE *f);
  ~Parser();

  RubyHeapObj* create_heap_object(RubyValueType type);

  int32_t get_heap_object_count() { return heap_obj_count_; }

  const char * current_heap_object_json();

  template<typename Func> void parse(Func func) {
    fseeko(f_, 0, SEEK_SET);

    char buf[16384];
    rapidjson::Reader reader;
    rapidjson::FileReadStream frs(f_, buf, sizeof(buf));

    handler_.state_ = HeapDumpHandler::kStart;
    handler_.parser_ = this;
    handler_.stream_ = &frs;
    while (reader.Parse<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseNumbersAsStringsFlag>(frs, handler_)) {
      func(handler_.obj_);
    }

    handler_.state_ = HeapDumpHandler::kFinish;
    if (reader.HasParseError() && reader.GetParseErrorCode() != rapidjson::kParseErrorDocumentEmpty) {
      // TODO: something
    }
  }
};

}

#endif // HARB_PARSER_H
