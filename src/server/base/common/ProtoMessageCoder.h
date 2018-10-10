#ifndef BASE_PROTO_MESSAGE_CODER_H
#define BASE_PROTO_MESSAGE_CODER_H

#include <arpa/inet.h>
#include <sstream>
#include "Likely.h"
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

namespace base {

inline size_t GetEncodedSize(const google::protobuf::Message& msg) {
  return 5 + msg.GetTypeName().size() + msg.ByteSize();
}

inline bool EncodeProtoMessage(const google::protobuf::Message& msg, char* buf) {
  const std::string& type = msg.GetTypeName();
  assert(type.size() < 256);
  buf[4] = static_cast<char>(type.size());
  memcpy(buf + 5, type.c_str(), type.size());
  return msg.SerializeToArray(buf + 5 + type.size(), msg.ByteSize());
}

inline size_t EncodeProtoMessage(const google::protobuf::Message& msg, char** buf, size_t& n) {
  const std::string& type = msg.GetTypeName();
  assert(type.size() < 256);
  size_t len = 5 + type.size() + msg.ByteSize();
  if (unlikely(len > n)) {
    delete *buf;
    *buf = new char[len];
    n = len;
  }
  (*buf)[4] = static_cast<char>(type.size());
  memcpy(*buf + 5, type.c_str(), type.size());
  if (unlikely(!msg.SerializeToArray(*buf + 5 + type.size(), msg.ByteSize()))) {
    return 0;
  }
  return len;
}

inline char* EncodeProtoMessage(const google::protobuf::Message& msg, size_t& n) {
  const std::string& type = msg.GetTypeName();
  assert(type.size() < 256);
  n = 5 + type.size() + msg.ByteSize();
  char* buf = new char[n];
  buf[4] = static_cast<char>(type.size());
  memcpy(buf + 5, type.c_str(), type.size());
  if (unlikely(!msg.SerializeToArray(buf + 5 + type.size(), msg.ByteSize()))) {
    delete[] buf;
    n = 0;
    buf = nullptr;
  }
  return buf;
}

inline google::protobuf::Message* DecodeProtoMessage(void* data, size_t n) {
  google::protobuf::Message* msg = nullptr;
  const char* buf = static_cast<const char*>(data);
  int32_t name_len = buf[4];
  if (name_len + 5 < n) {
    // std::cout << "len = " << name_len << std::endl;
    std::string type(buf + 5, name_len);
    // std::cout << "type = " << type << std::endl;
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type);
    if (descriptor) {
      const google::protobuf::Message* prototype =
          google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
      if (prototype) {
        msg = prototype->New();
        const size_t head_len = 5 + name_len;
        if (!msg->ParseFromArray(buf + head_len, n - head_len)) {
          delete msg;
          msg = nullptr;
        }
      } else {
        std::cout << "Can't get prototype" << std::endl;
      }
    } else {
      std::cout << "Can't get descriptor by type " << type << std::endl;
    }
  }
  return msg;
}

inline std::string GetString(void* data, size_t n) {
  std::ostringstream oss;
  for (int i = 0; i < n; ++i) {
    char ch = ((char*)data)[i];
    oss << (int)ch  << "(" << ch << ") ";
  }
  return oss.str();
}

} // namespace base

#endif // BASE_PROTO_MESSAGE_CODER_H
