#ifndef BASE_CODE_CONVERTER_H
#define BASE_CODE_CONVERTER_H

#include <iconv.h>
#include <iostream>
#include <cstring>

namespace base
{
static int32_t CodeConvert(const char *from_charset, const char *to_charset,
    const char *inbuf, int inlen, char *outbuf, int outlen)
{
  iconv_t cd;
  const char **cpin = &inbuf;
  char **pin = const_cast<char **>(cpin);
  char **pout = &outbuf;

  cd = iconv_open(to_charset, from_charset);
  if (cd == 0)
    return -1;
  std::memset(outbuf, 0, outlen);
  size_t inLen = static_cast<size_t>(inlen);
  size_t outLen = static_cast<size_t>(outlen);
  if (iconv(cd, pin, &inLen, pout, &outLen) == size_t(-1))
    return -1;
  iconv_close(cd);
  return 0;
}

inline std::string Utf8ToGB2312(const char *in_utf8, int inlen,
    char *out_gb2312, int outlen)
{
  CodeConvert("utf-8", "gb2312", in_utf8, inlen, out_gb2312, outlen);
  return out_gb2312;
}

inline std::string Utf8ToGB2312(const char *in_utf8, int len)
{
  char out[1024];
  CodeConvert("utf-8", "gb2312", in_utf8, len, out, sizeof(out));
  return out;
}

inline std::string Utf8ToGB2312(const char *in_utf8)
{
  char out[1024];
  CodeConvert("utf-8", "gb2312", in_utf8, strlen(in_utf8), out, sizeof(out));
  return out;
}

inline std::string GB2312ToUtf8(const char *in_gb2312, int inlen,
    char *out_utf8, int outlen)
{
  CodeConvert("gb2312", "utf-8", in_gb2312, inlen, out_utf8, outlen);
  return out_utf8;
}

inline std::string Code_GB2312_TO_Utf8(const char *in_gb2312, int len)
{
  char out[1024];
  CodeConvert("gb2312", "utf-8", in_gb2312, len, out, sizeof(out));
  return out;
}

inline std::string GB2312ToUtf8(const char *in_gb2312)
{
  char out[1024];
  CodeConvert("gb2312", "utf-8", in_gb2312, strlen(in_gb2312), out, sizeof(out));
  return out;
}
}

#endif
