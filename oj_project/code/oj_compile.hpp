#pragma once

#include <iostream>
#include <json/json.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <atomic>

#include "tools.hpp"

//枚举错误码
enum ErrorNo {
    OK = 0,
    PARAM_ERROR,
    INTERNAL_ERROR,
    COMPILE_ERROR,
    RUN_ERROR
};

class Compile {
    public:
        //req : 请求的json对象
        //resp : 出参，返还给调用者的json对象
        static void CompileAndRun(Json::Value req, Json::Value *resp)
        {
            //判断json中的code是否为空，如果为空则表示(认为是)参数错误
            if(req["code"].empty()) {
                (*resp)["error_no"] = PARAM_ERROR;
                (*resp)["reason"] = "Param error";
                return;
            }

            //将代码写到文件中
            std::string code = req["code"].asString();
            std::string file_header = WriteTmpFile(code);
            if(file_header == "") {
                (*resp)["error_no"] = INTERNAL_ERROR;
                (*resp)["reason"] = "write file failed";
                return;
            }

            //编译
            if(!_Compile(file_header)) {
                (*resp)["error_no"] = COMPILE_ERROR;
                std::string reason = "Compile error : ";
                std::string compile_reason;
                FileUtil::ReadFile(CompileErrorPath(file_header), &compile_reason);
                reason += compile_reason;
                (*resp)["reason"] = reason;
                return;
            } 

            //运行
            int ret = Run(file_header);
            if(ret != 0) {
                (*resp)["error_no"] = RUN_ERROR;
                (*resp)["reason"] = "Run error : program  exit by signal " + std::to_string(ret) + "."; 
                return;
            }

            //构造响应
            (*resp)["error_no"] = OK;
            (*resp)["reason"] = "Compile and run successful!";

            std::string stdout_str;
            FileUtil::ReadFile(StdoutPath(file_header).c_str(), &stdout_str);
            (*resp)["stdout"] = stdout_str;

            std::string stderror_str;
            FileUtil::ReadFile(StderrorPath(file_header).c_str(), &stderror_str);
            (*resp)["stderror"] = stderror_str;

            //删除临时文件
            Clean(file_header);
            return;
        }

    private:
        static void Clean(const std::string file_header)
        {
            unlink(SrcPath(file_header).c_str());
            unlink(CompileErrorPath(file_header).c_str());
            unlink(ExePath(file_header).c_str());
            unlink(StdoutPath(file_header).c_str());
            unlink(StderrorPath(file_header).c_str());
        }

        static int Run(const std::string &file_header)
        {
            //创建子进程，子进程进行进程程序替换来运行可执行程序
            pid_t pid = fork();
            if(pid < 0) {
                perror("fork");
                return -1;
            }
            else if(pid == 0) {
                //注册定时器，限制程序运行时间
                alarm(1);

                //限制内存使用
                struct rlimit rlim;                                    
                rlim.rlim_cur = 30000 * 1024;                                                                 
                rlim.rlim_max = RLIM_INFINITY;
                setrlimit(RLIMIT_AS, &rlim);

                //进程程序替换之前先将标准输出和标准错误中的内容都重定向到对应的文件中，用于之后回复响应
                int stdout_fd = open(StdoutPath(file_header).c_str(), O_CREAT | O_WRONLY, 0666);
                if(stdout_fd < 0) {
                    return -2;
                }
                dup2(stdout_fd, 1);

                int stderror_fd = open(StderrorPath(file_header).c_str(), O_CREAT | O_WRONLY, 0666);
                if(stderror_fd < 0) {
                    return -2;
                }
                dup2(stderror_fd, 2);

                execl(ExePath(file_header).c_str(), ExePath(file_header).c_str());
                exit(0);
            }
            else {
                int status;
                waitpid(pid, &status, 0);
                return status & 0x7f;
            }

            return 0;
        }

        static bool _Compile(const std::string file_header)
        {
            //创建子进程
            pid_t pid = fork();
            if(pid < 0) {
                perror("fork");
                return false;
            }
            else if(pid == 0) {
                //子进程进行进程程序替换来编译源码文件
                //替换前先将标准错误重定向为fd, 将标准错误中的内容输出到文件中 
                int fd = open(CompileErrorPath(file_header).c_str(), O_CREAT | O_WRONLY,  0664);
                if(fd < 0) {
                    perror("open");
                    return false;
                }
                dup2(fd, 2);

                execlp("g++", "g++", SrcPath(file_header).c_str(), "-o", ExePath(file_header).c_str(), "-std=c++11", "-D", "CompileOnline", NULL); 
                //如果进程替换失败，直接让子进程退出
                exit(0);
            }
            else {
                //父进程进行进程等待
                waitpid(pid, NULL, 0); 
            }

            //如果g++编译成功，则一定会产生可执行程序，因此在这里判断是否有可执行文件产生从而知道g++是否编译成功
            struct stat st;
            int ret = stat(ExePath(file_header).c_str(), &st);
            if(ret < 0) {
                //可执行程序不存在，意味着编译失败
                return false;
            }

            return true;
        }

        static std::string StdoutPath(const std::string &file_name)
        {
            return "./tmp_file/" + file_name + ".stdout";
        }

        static std::string StderrorPath(const std::string &file_name)
        {
            return "./tmp_file/" + file_name + ".stderror";
        }

        static std::string CompileErrorPath(const std::string &file_name)
        {
            return "./tmp_file/" + file_name + ".compile_error";
        }

        static std::string ExePath(const std::string &file_name)
        {
            return "./tmp_file/" + file_name + ".exe";
        }

        static std::string SrcPath(const std::string &file_name)
        {
            return "./tmp_file/" + file_name + ".cpp";
            //return file_name + ".cpp";
        }

        //写入文件，文件名加上时间戳，来区分不同的请求
        static std::string WriteTmpFile(const std::string &code)
        {
            //线程安全的变量
            static std::atomic_uint id(0);
            //组织文件名称，区分源码文件及可执行程序文件
            //文件头 : tmp_stamp.id
            std::string tmp_file_name = "tmp_" + std::to_string(TimeUtil::GetTimeStampMS()) + "." + std::to_string(id);

            //将code写入文件并加上文件尾来区分不同类型文件
            //tmp_stamp.id.cpp
            FileUtil::WriteFile(SrcPath(tmp_file_name), code);
            return tmp_file_name;
        }
};


