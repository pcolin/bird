#include "CsvReader.h"

#include <fstream>
#include <boost/algorithm/string.hpp>

namespace base
{
CsvReader::CsvReader(const std::string& file)
{
  std::fstream fs(file.c_str(), std::fstream::in);
  if (fs)
  {
    std::string line;
    while (std::getline(fs, line))
    {
      std::vector<std::string> tmp;
      boost::split(v, line, boost::is_any_of(","));
      lines_.push_back(std::move(v));
    }

    fs.close();
  }
}
}
