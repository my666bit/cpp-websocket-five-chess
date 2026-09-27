
#include"util.hpp"
#ifndef __M_DB_H__
#define __M_DB_H__
#include <mutex>
#include <cassert>


class user_table{
    private:
        MYSQL*_mysql;
        std::mutex _mutex;//互斥锁保护数据库访问操作
    public:
        user_table(const std::string &host,
            const std::string &username,
            const std::string &password,
            const std::string &dbname,
            uint16_t port = 3306){
            _mysql = mysql_util::mysql_create(host,username,password,dbname,port);
            assert(_mysql != NULL);
            }
        ~user_table(){
            mysql_util::mysql_destroy(_mysql);
            _mysql = NULL;
        }




        bool insert(Json::Value &user)//1.注册新增用户
        {
#define INSERT_USER "insert user values(null,'%s',SHA2('%s', 256),1000,0,0);"
            //sprintf(void *buf,char *format,...)


            // Json::Value val;
            // bool ret = select_by_name(user["username"].asCString(),val);
            // if(ret == true){
            //     DBG_LOG("user:%s is already exists",user["username"].asCString());
            //     return false;
            // }



            if(user["password"].asCString() == NULL || user["username"].asCString() == NULL){
                DBG_LOG("INPUT PASSWORD OR USERNAME");
                return false;
            }
            char sql[4096] = {0};

            sprintf(sql,INSERT_USER,user["username"].asCString(),user["password"].asCString());
            bool ret = mysql_util::mysql_exec(_mysql,sql);
            if(ret == false){
                DBG_LOG("insert user info failed!\n");
                return false;
            }
            return true;
        }




        bool login(Json::Value &user)//2.登录验证，并详细返回用户信息
        {
            //以用户名和密码作为查询过滤条件，查询数据表示用户名密码一致，没有信息则用户名密码错误
#define LOGIN_USER "select id,score,total_count,win_count from user where username = '%s' and password = SHA2('%s', 256);"

            char sql[4096] = {0};
            sprintf(sql,LOGIN_USER,user["username"].asCString(),user["password"].asCString());


             MYSQL_RES *res  = NULL;
            {//定义局部作用域管理互斥锁，管理互斥锁
                std::unique_lock<std::mutex> lock(_mutex);

                bool ret = mysql_util::mysql_exec(_mysql,sql);
                if(ret == false){
                    DBG_LOG("user login failed!\n");
                    return false;
                }

                //如果有数据，只能有一条数据
                res = mysql_store_result(_mysql);
                if(res == NULL){
                    DBG_LOG("have no login user info!");
                    return false;
                }
            }

            int row_num = mysql_num_rows(res);
            if(row_num!=1){
                DBG_LOG("the user information queried is not unique!");
                return false;
            }
            MYSQL_ROW row = mysql_fetch_row(res);
            user["id"] = (Json::UInt64)std::stol(row[0]);
            user["score"] = (Json::UInt64)std::stol(row[1]);
            user["total_count"] = std::stoi(row[2]);
            user["win_count"] = std::stoi(row[3]);

            mysql_free_result(res);
            return true;
        }





        bool select_by_name(const std::string &name,Json::Value &user)//3.通过用户名获取用户信息
        {
#define USER_BY_NAME "select id,score,total_count,win_count from user where username = '%s';"
            char sql[4096] = {0};
            sprintf(sql,USER_BY_NAME,name.c_str());
            MYSQL_RES *res  = NULL;
            {//定义局部作用域管理互斥锁，管理互斥锁
                std::unique_lock<std::mutex> lock(_mutex);
                bool ret = mysql_util::mysql_exec(_mysql,sql);
                if(ret == false){
                    DBG_LOG("get user by name failed!\n");
                    return false;
                }

                //如果有数据，只能有一条数据
                res = mysql_store_result(_mysql);
                if(res == NULL){
                    DBG_LOG("have no user info!");
                    return false;
                }
            }
            int row_num = mysql_num_rows(res);
            if(row_num!=1){
                DBG_LOG("the user information queried is not unique!");
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            user["id"] = (Json::UInt64)std::stol(row[0]);
            user["username"] = name;
            user["score"] = (Json::UInt64)std::stol(row[1]);
            user["total_count"] = std::stoi(row[2]);
            user["win_count"] = std::stoi(row[3]);

            mysql_free_result(res);
            return true;
        }





        bool select_by_id(uint64_t id,Json::Value &user)//4.通过id获取用户信息
        {
#define USER_BY_ID "select username,score,total_count,win_count from user where id = %d;"
            char sql[4096] = {0};
            sprintf(sql,USER_BY_ID,id);
            MYSQL_RES *res  = NULL;
            {//定义局部作用域管理互斥锁，管理互斥锁
                std::unique_lock<std::mutex> lock(_mutex);
                bool ret = mysql_util::mysql_exec(_mysql,sql);
                if(ret == false){
                    DBG_LOG("get user by id failed!\n");
                    return false;
                }

                //如果有数据，只能有一条数据
                res = mysql_store_result(_mysql);
                if(res == NULL){
                    DBG_LOG("have no user info!");
                    return false;
                }
            }

            int row_num = mysql_num_rows(res);
            if(row_num!=1){
                DBG_LOG("the user information queried is not unique!");
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            user["id"] = (Json::UInt64)id;
            user["username"] = row[0];
            user["score"] = (Json::UInt64)std::stol(row[1]);
            user["total_count"] = std::stoi(row[2]);
            user["win_count"] = std::stoi(row[3]);

            mysql_free_result(res);
            return true;
        }




        bool win(uint64_t id)//5.胜利时天梯分数增加30分，战⽃场次增加1，胜利场次增加1
        {
#define USER_WIN "update user set score = score+30,total_count = total_count+1,win_count = win_count + 1 where id = %d;"
            char sql[4096] = {0};
            sprintf(sql,USER_WIN,id);
            bool ret = mysql_util::mysql_exec(_mysql,sql);
            if(ret == false)
            {
                DBG_LOG("update win user info failed!\n");
                return false;
            }
            return true;
        }




        bool lose(uint64_t id)//6.失败时天梯分数减少30，战⽃场次增加1，其他不变
        {
#define USER_LOSE "update user set score = score-30,total_count = total_count+1 where id = %d;"
            char sql[4096] = {0};
            sprintf(sql,USER_LOSE,id);
            bool ret = mysql_util::mysql_exec(_mysql,sql);
            if(ret == false)
            {
                DBG_LOG("update lose user info failed!\n");
                return false;
            }
            return true;
        }



};

#endif