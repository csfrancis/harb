#include "parser.h"

namespace harb {

Parser::Parser(FILE *f) : heap_obj_count_(0), f_(f) {}

const char * Parser::get_intern_string(const char *str) {
  assert(str);
  auto it = intern_strings_.find(str);
  if (it != intern_strings_.end()) {
    return *it;
  }
  const char *dup = strdup(str);
  intern_strings_.insert(dup);
  return dup;
}

RubyHeapObj * Parser::create_heap_object(RubyValueType type) {
  return new RubyHeapObj(NULL, type, ++heap_obj_count_);
}

bool Parser::HeapDumpHandler::StartObject() {
  switch (state_) {
    case kStart:
    case kFinishObject:
      obj_ = parser_->create_heap_object(RUBY_T_NONE);
      state_ = kInsideObject;
      obj_start_pos_ = stream_->Tell() - 1;
      return true;
    default:
      return true;
  }
}

bool Parser::HeapDumpHandler::EndObject(rapidjson::SizeType memberCount __attribute__((unused))) {
  switch (state_) {
    case kInsideObject:
      obj_end_pos_ = stream_->Tell();
      state_ = kFinishObject;
      return true;
    case kFlags:
      state_ = kInsideObject;
      return true;
    default:
      return true;
  }
}

bool Parser::HeapDumpHandler::Key(const char* str, rapidjson::SizeType length, bool copy __attribute__((unused))) {
  switch (state_) {
    case kInsideObject:
      if (strncmp(str, "address", length) == 0) {
        state_ = kAddress;
      } else if (strncmp(str, "type", length) == 0) {
        state_ = kType;
      } else if (strncmp(str, "class", length) == 0) {
        state_ = kClass;
      } else if (strncmp(str, "references", length) == 0) {
        state_ = kReferences;
      } else if (strncmp(str, "value", length) == 0) {
        state_ = kValue;
      } else if (strncmp(str, "memsize", length) == 0) {
        state_ = kMemsize;
      } else if (strncmp(str, "frozen", length) == 0) {
        state_ = kFrozen;
      } else if (strncmp(str, "shared", length) == 0) {
        state_ = kShared;
      } else if (strncmp(str, "size", length) == 0) {
        state_ = kSize;
      } else if (strncmp(str, "length", length) == 0) {
        state_ = kLength;
      } else if (strncmp(str, "flags", length) == 0) {
        state_ = kFlags;
      } else if (strncmp(str, "struct", length) == 0) {
        state_ = kStruct;
      } else if (strncmp(str, "imemo_type", length) == 0) {
        state_ = kStruct;
      } else if (strncmp(str, "root", length) == 0) {
        state_ = kRoot;
      }
      return true;
    default:
      return true;
  }
}

bool Parser::HeapDumpHandler::String(const char* str, rapidjson::SizeType length __attribute__((unused)), bool copy __attribute__((unused))) {
  switch (state_) {
    case kType:
      obj_->flags |= RubyHeapObj::get_value_type(str);
      state_ = kInsideObject;
      return true;
    case kAddress:
      obj_->as.obj.addr = strtoull(str, NULL, 0);
      assert(obj_->as.obj.addr != 0);
      state_ = kInsideObject;
      return true;
    case kClass:
      obj_->as.obj.clazz.addr = strtoull(str, NULL, 0);
      assert(obj_->as.obj.clazz.addr != 0);
      state_ = kInsideObject;
      return true;
    case kReferences:
      {
        uint64_t addr = strtoull(str, NULL, 0);
        assert(addr != 0);
        refs_to_.push_back(addr);
      }
      return true;
    case kValue:
    case kStruct:
    case kImemoType:
      obj_->as.obj.as.value = parser_->get_intern_string(str);
      state_ = kInsideObject;
      return true;
    case kRoot:
      obj_->as.root.name = parser_->get_intern_string(str);
      state_ = kInsideObject;
      return true;
    default:
      return true;
  }
}

bool Parser::HeapDumpHandler::StartArray() {
  if (state_ == kReferences) {
    refs_to_.clear();
  }
  return true;
}

bool Parser::HeapDumpHandler::EndArray(rapidjson::SizeType elementCount) {
  if (state_ == kReferences) {
    size_t i = 0;
    assert(refs_to_.size() == elementCount);
    obj_->refs_to.addr = new uint64_t[refs_to_.size() + 1];
    for (i = 0; i < refs_to_.size(); i++) {
      obj_->refs_to.addr[i] = refs_to_[i];
    }
    obj_->refs_to.addr[i] = 0;
    state_ = kInsideObject;
  }
  return true;
}

bool Parser::HeapDumpHandler::Bool(bool b) {
  uint32_t flag = 0;

  if (b) {
    switch (state_) {
      case kFrozen:
        flag |= RUBY_FL_FROZEN;
        break;
      case kShared:
        flag |= RUBY_FL_SHARED;
        break;
      default:
        break;
    }
  }

  if (flag) {
    obj_->flags |= flag;
    state_ = kInsideObject;
  }

  return true;
}

bool Parser::HeapDumpHandler::RawNumber(const char* str, rapidjson::SizeType length __attribute__((unused)), bool copy __attribute__((unused))) {
  switch (state_) {
    case kMemsize:
      obj_->as.obj.memsize = strtoul(str, NULL, 0);
      state_ = kInsideObject;
      return true;
    case kSize:
    case kLength:
      obj_->as.obj.as.size = strtoul(str, NULL, 0);
      state_ = kInsideObject;
      return true;
    default:
      return true;
  }
}

const char * Parser::current_heap_object_json() {
  // TODO
  return NULL;
}

}
