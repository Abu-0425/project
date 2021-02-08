#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>

//读写文件
class FileUtil {
  public:
    static bool ReadFile(const std::string &file_name, std::string *content)
    {
      //先清空content中的内容
      content->clear();

      std::ifstream file(file_name.c_str());
      if(!file.is_open()) {
        std::cout << "file open failed" << std::endl; 
        return false;
      }

      //将文件中读到的内容追加到content中
      std::string line;
      while(std::getline(file, line)) {
        (*content) += line + "\n";
      }
      file.close();
      return true;
    }

    static bool WriteFile(const std::string &file_name, const std::string &data)
    {
      std::ofstream file(file_name.c_str());

      if(!file.is_open()) {
        std::cout << "file open failed" << std::endl; 
        return false;
      }

      file.write(data.c_str(), data.size());
      file.close();
      return true;
    }
};

//字符串相关
class StringUtil {
  public:
    static void Split(const std::string &input, const std::string &split_char, std::vector<std::string> *output)
    {
      //使用boost库中的split函数来分割字符串
      //is_any_of : 支持多个字符作为分割符
      //token_compress_off : 是否将多个分割字符看成一个(是否压缩分割符), off是不压缩
      boost::split(*output, input, boost::is_any_of(split_char), boost::token_compress_off);
    }
};

//时间相关
class TimeUtil {
  public:
    //获取时间戳，毫秒级别
    static int64_t GetTimeStampMS()
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return tv.tv_sec + tv.tv_usec / 1000;
    }

    //获取时间戳(年月日 时分秒)
    static void GetTimeStamp(std::string *time_stamp) 
    {
      time_t tt;
      time(&tt);

      struct tm *st = localtime(&tt);
      char buf[32] = { 0 };
      snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d %02d:%02d:%02d", st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec);
      time_stamp->assign(buf, strlen(buf));
    }
};

//url解析
class UrlUtil {
  public:
    static std::string UrlEncode(const std::string& str) {
      std::string strTemp = "";
      size_t length = str.length();
      for (size_t i = 0; i < length; i++) {
        if (isalnum((unsigned char)str[i]) || 
            (str[i] == '-') ||
            (str[i] == '_') || 
            (str[i] == '.') || 
            (str[i] == '~'))
          strTemp += str[i];
        else if (str[i] == ' ')
          strTemp += "+";
        else {
          strTemp += '%';
          strTemp += ToHex((unsigned char)str[i] >> 4);
          strTemp += ToHex((unsigned char)str[i] % 16);
        }
      }
      return strTemp;
    }

    static std::string UrlDecode(const std::string& str) {
      std::string strTemp = "";
      size_t length = str.length();
      for (size_t i = 0; i < length; i++)
      {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
          assert(i + 2 < length);
          unsigned char high = FromHex((unsigned char)str[++i]);
          unsigned char low = FromHex((unsigned char)str[++i]);
          strTemp += high*16 + low;
        }
        else strTemp += str[i];
      }
      return strTemp;
    }

    //将Post请求中的body进行解析
    //先切割(boost::split), 在对切割之后的结果进行转码
    static void ParseBody(const std::string& body, 
        std::unordered_map<std::string, std::string>* kv_body) {
      std::vector<std::string> tokens;
      StringUtil::Split(body, "&", &tokens);
      for (const auto& token : tokens) {
        std::vector<std::string> kv;
        StringUtil::Split(token, "=", &kv);
        if (kv.size() != 2) {
          continue;
        }
        //针对获取到的结果(value)进行urldecode
        (*kv_body)[kv[0]] = UrlDecode(kv[1]);
      }
    }
  private:
    static unsigned char ToHex(unsigned char x) { 
      return  x > 9 ? x + 55 : x + 48; 
    }

    static unsigned char FromHex(unsigned char x) { 
      unsigned char y = '\0';
      if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
      else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
      else if (x >= '0' && x <= '9') y = x - '0';
      else assert(0);
      return y;
    }
};

//枚举日志等级
enum LogLevel {
  INFO = 0,
  WRINING,
  ERROR,
  FATAL,
  DEBUG
};

const char *level[] = {"INFO", "WARNING", "ERROR", "FATAL", "DEBUG"};

#define LOG(lev, msg) Log(lev, __FILE__, __LINE__, msg)

//日志格式 : [时间戳 日志等级 文件名:行号] 信息
//如 : [2021-02-07 16:58:47 INFO oj_server.cpp:62] test 

//使用inline可以使file和line更加准确，因为代码会在调用处展开
inline std::ostream& Log(LogLevel lev, const char *file, int line, std::string msg)
{
  std::string log_level = level[lev];
  std::string time_stamp;
  TimeUtil::GetTimeStamp(&time_stamp);
  std::cout << "[" << time_stamp << " " << log_level << " " << file << ":" << line << "] " << msg;

  return std::cout; //便于在调用处直接进行追加打印
}
