#pragma once

#include <iostream>
#include <vector>
#include <ctemplate/template.h>
#include "oj_model.hpp"

class OjView {
    public:
        //渲染所有题目
        static void DrawAllQuestions(std::vector<Question> &questions, std::string *html)
        {
            //创建ctemplate字典
            ctemplate::TemplateDictionary dict ("all_questions");

            //遍历所有题目，每个题目创建一个子字典
            for(const auto &ques : questions) {
                //创建字字典
                ctemplate::TemplateDictionary *sub_dict = dict.AddSectionDictionary("question");

                //void SetValue(const TemplateString variable, const TemplateString value);
                //variable : 预定义的html当中的变量名称
                //value : 替换的值
                sub_dict->SetValue("id", ques._id);
                sub_dict->SetValue("id", ques._id);
                sub_dict->SetValue("title", ques._title);
                sub_dict->SetValue("star", ques._star);
            }
            //填充
            //DO_NOT_STRIP : 逐字逐句的填充到html页面中
            ctemplate::Template *ct = ctemplate::Template::GetTemplate("./template/all_questions.html", ctemplate::DO_NOT_STRIP);
            //渲染
            ct->Expand(html, &dict);
        }

        //渲染单个题目
        static void DrawOneQuestion(const Question &ques, std::string *html)
        {
            ctemplate::TemplateDictionary dict("one_question");
            dict.SetValue("id", ques._id);
            dict.SetValue("title", ques._title);
            dict.SetValue("star", ques._star);
            //dict.SetValue("id", ques._id);
            dict.SetValue("code", ques._header_cpp);
            dict.SetValue("desc", ques._desc);

            //填充 + 渲染
            ctemplate::Template *ct = ctemplate::Template::GetTemplate("./template/question.html", ctemplate::DO_NOT_STRIP);
            ct->Expand(html, &dict);
        }

        //渲染编译/运行结果
        static void DrawCaseResult(const std::string &err_no, const std::string &case_result, const std::string &reason, std::string *html)
        {
            ctemplate::TemplateDictionary dict("case_result");
            dict.SetValue("error_no", err_no);
            dict.SetValue("compile_result", reason);
            dict.SetValue("case_result", case_result);

            ctemplate::Template *ct = ctemplate::Template::GetTemplate("./template/case_result.html", ctemplate::DO_NOT_STRIP);
            ct->Expand(html, &dict);
        }
};
