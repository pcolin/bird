#ifndef BASE_PROTO_MESSAGE_CODER_H
#define BASE_PROTO_MESSAGE_CODER_H

#include "Likely.h"
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include <arpa/inet.h>
#include <iostream>
namespace base
{
inline size_t EncodeProtoMessage(const google::protobuf::Message &msg, char **buf, size_t &n)
{
  const std::string &type = msg.GetTypeName();
  int32_t name_len = htonl(static_cast<int32_t>(type.size()));
  size_t len = sizeof(name_len) + type.size() + msg.ByteSize();
  if (unlikely(len > n))
  {
    delete *buf;
    *buf = new char[len];
    n = len;
  }
  memcpy(*buf, reinterpret_cast<char*>(&name_len), sizeof(name_len));
  memcpy(*buf + sizeof(name_len), type.c_str(), type.size());
  if (unlikely(!msg.SerializeToArray(*buf + sizeof(name_len) + type.size(), msg.ByteSize())))
  {
    return 0;
  }
  return len;
}

inline char* EncodeProtoMessage(const google::protobuf::Message &msg, size_t &n)
{
  const std::string &type = msg.GetTypeName();
  int32_t name_len = htonl(static_cast<int32_t>(type.size()));
  n = sizeof(name_len) + type.size() + msg.ByteSize();
  char *buf = new char[n];
  memcpy(buf, reinterpret_cast<char*>(&name_len), sizeof(name_len));
  memcpy(buf + sizeof(name_len), type.c_str(), type.size());
  if (unlikely(!msg.SerializeToArray(buf + sizeof(name_len) + type.size(), msg.ByteSize())))
  {
    delete[] buf;
    n = 0;
    buf = nullptr;
  }
  return buf;
}

inline google::protobuf::Message* DecodeProtoMessage(void *data, size_t n)
{
  const char* buf = static_cast<const char*>(data);
  int32_t name_len = 0;
  memcpy(&name_len, buf, sizeof(name_len));
  name_len = static_cast<int32_t>(ntohl(name_len));
  // std::cout << "len = " << name_len << std::endl;
  std::string type(buf + sizeof(int32_t), name_len);
  // std::cout << "type = " << type << std::endl;
  google::protobuf::Message *msg = nullptr;
  const google::protobuf::Descriptor *descriptor =
    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type);
  if (descriptor)
  {
    const google::protobuf::Message *prototype =
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (prototype)
    {
      msg = prototype->New();
      const size_t head_len = + sizeof(name_len) + name_len;
      if (!msg->ParseFromArray(buf + head_len, n - head_len))
      {
        delete msg;
        msg = nullptr;
      }
    }
    else
    {
      std::cout << "Can't get prototype" << std::endl;
    }
  }
  else
  {
    std::cout << "Can't get descriptor by type " << type << std::endl;
  }
  return msg;
}
}

#endif
