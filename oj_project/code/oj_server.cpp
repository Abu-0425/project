#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <json/json.h>
#include "httplib.h"
#include "oj_model.hpp"
#include "oj_view.hpp"
#include "oj_compile.hpp"

int main()
{
  
  using namespace httplib;
  OjModel model;
  //1.初始化httplib库中的Server对象
  Server svr;

  //提供三个接口，分别处理三个请求
  //2.1 获取整个题目列表
  svr.Get("/all_questions", [&model](const Request &req, Response &resp){
      std::vector<Question> questions;
      model.GetAllQuestions(&questions);
      //填充渲染html页面
      std::string html;
      OjView::DrawAllQuestions(questions, &html);
      //服务器后台回复响应
      resp.set_content(html, "text/html"); 
      });

  //2.2 获取单个题目    
  //使用正则表达式来区分不同的题目 : \d+可表示一个0到正无穷大的数字
  //浏览器提交的资源路径是 : /question/[试题编号] --> /question/[\d+[0, 正无穷)]
  svr.Get(R"(/question/(\d+))",[&model](const Request &req, Response &resp){
      //获取url中的题目编号，然后获取具体的单个题目
      Question ques;
      model.GetOneQuestion(req.matches[1].str(), &ques); 

      //渲染模板的html文件
      std::string html;
      OjView::DrawOneQuestion(ques, &html);
      resp.set_content(html, "text/html");
      });

  //2.3 编译运行     
  svr.Post(R"(/compile/(\d+))", [&model](const Request &req, Response &resp){
      //获取题目编号及题目内容
      Question ques;
      model.GetOneQuestion(req.matches[1].str(), &ques);

      //解析(decode)用户提交的代码(因为Post方法在提交代码时，是经过urlencode的)
      std::unordered_map<std::string, std::string> body;
      UrlUtil::ParseBody(req.body, &body); 

      std::string user_code = body["code"];
      //构造json对象, 交给编译运行模块
      Json::Value req_json;
      req_json["code"] = user_code + ques._tail_cpp;
      Json::Value resp_json;
      Compile::CompileAndRun(req_json, &resp_json); 

      //编译运行完毕后，所有结果都在resp_json当中
      std::string error_no = resp_json["error_no"].asString();
      std::string case_result = resp_json["stdout"].asString();
      std::string reason = resp_json["reason"].asString();

      //渲染之后给浏览器返回响应
      std::string html;
      OjView::DrawCaseResult(error_no, case_result, reason, &html);
      resp.set_content(html, "text/html");
      });

  LOG(INFO, "Server Start!") << std::endl;
  //设定 http 服务器的根目录
  svr.set_base_dir("./wwwroot");

  svr.listen("0.0.0.0", 8080);

  return 0;
}
