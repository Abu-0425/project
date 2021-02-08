#pragma once 

#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include "tools.hpp"

struct Question
{
  std::string _id; //题目id
  std::string _title; //题目标题
  std::string _star; //题目难易程度
  std::string _path; //题目路径
  std::string _desc; //题目描述
  std::string _header_cpp; //题目的预定义头
  std::string _tail_cpp; //题目的尾（包含测试用例及调用逻辑）
};

class OjModel
{
  public:
    OjModel()
    {
      Load("./oj_data/oj_config.cfg");
    }
    
    ~OjModel()
    {

    }

    //从文件中获取题目信息
    bool Load(const std::string &file_name)
    {
      std::ifstream file(file_name.c_str());
      if(!file.is_open()) {
        std::cout << "config file open failed" << std::endl;
        return false;
      }

      //1.从文件中获取每一行(题目)的信息,并将各项信息保存到结构体中
      //2.把多个Question组织在map中
      std::string line;
      while(std::getline(file, line)) {
        //将读到的每一行信息进行分割，结果放到vector中
        std::vector<std::string> vec;
        StringUtil::Split(line, "\t", &vec);

        Question ques;
        ques._id = vec[0];
        ques._title = vec[1];
        ques._star = vec[2];
        ques._path = vec[3];

        std::string dir = vec[3];
        FileUtil::ReadFile(dir + "/desc.txt", &ques._desc);
        FileUtil::ReadFile(dir + "/header.cpp", &ques._header_cpp);
        FileUtil::ReadFile(dir + "/tail.cpp", &ques._tail_cpp);

        _ques_map[ques._id] = ques;

      }
      file.close();
      return true;
    }

    //获取全部题目
    bool GetAllQuestions(std::vector<Question> *ques)
    {
      //遍历无序的map，将题目信息返还给调用着
      for(const auto &qm : _ques_map) {
        ques->push_back(qm.second);
      }

      //由于map无序，因此还需进行排序
      std::sort(ques->begin(), ques->end(), [](const Question q1, const Question q2){
          //按照题目编号进行升序排序
          return std::stoi(q1._id) < std::stoi(q2._id);
          });
    }

    //获取单个题目
    //id : 所需查找题目的id
    //ques : 保存找到的题目，返还给调用者
    bool GetOneQuestion(const std::string &id, Question *ques)
    {
      auto it = _ques_map.find(id);
      if(it == _ques_map.end()) {
        //未找到该题目
        return false;
      }

      *ques = it->second;
      return true;
    }

  private:
    std::unordered_map<std::string, Question> _ques_map;
};
