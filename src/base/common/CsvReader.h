#ifndef BASE_CSV_READER_H
#define BASE_CSV_READER_H

#include <vector>
#include <fstream>
#include <boost/algorithm/string.hpp>
// #include <string>

namespace base
{
class CsvReader
{
  typedef std::vector<std::vector<std::string>> Lines;
  public:
    CsvReader(const std::string& file)
    {
      std::fstream fs(file.c_str(), std::fstream::in);
      if (fs)
      {
        std::string line;
        while (std::getline(fs, line))
        {
          std::vector<std::string> tmp;
          boost::split(tmp, line, boost::is_any_of(","));
          lines_.push_back(std::move(tmp));
        }
      }
      fs.close();
    }

    const Lines& GetLines() const { return lines_; }

  private:
    Lines lines_;
};
}

#endif
