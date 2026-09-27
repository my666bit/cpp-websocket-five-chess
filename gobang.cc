//#include"logger.hpp"
//#include"util.hpp"
#include"db.hpp"
#include"online.hpp"
#include "room.hpp"
#include "session.hpp"
#include "matcher.hpp"
#include "server.hpp"
#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASS "linuxzmy007788"
#define DBNAME "gobang"


void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST,USER,PASS,DBNAME,PORT);
    const char *sql = "insert stu values(null,'小黑',18,53,68,87);";
    bool ret = mysql_util::mysql_exec(mysql,sql);
    if(ret == false){
        return ;
    }

    mysql_util::mysql_destroy(mysql);
}


void json_test(){
    Json::Value root;
    std::string body;
    root["name"] = "lisi";
    root["age"] = 18; 
    root["socre"].append(100);
    root["socre"].append(50);
    root["socre"].append(99);
    json_util::serialize(root,body);
    DBG_LOG("%s",body.c_str());


    Json::Value val;
    json_util::unserialize(body,val);
    std::cout << "反序列化结果:\n"
          << "name:" << val["name"].asString() << "\n"
          << "age:" << val["age"].asInt() << "\n"
          << "socre:" << val["socre"][0].asFloat() << " " << val["socre"][1].asInt()
          << " " << val["socre"][2].asFloat() << "\n";

}


void str_test()
{
    std::string str = "123,234,,,,,345";
    std::vector<std::string> array;
    string_util::split(str,",",array);
    for(auto s:array)
    {
        DBG_LOG("%s",s.c_str());
    }
}


void file_test()
{
    std::string filename = "./makefile";
    std::string body;
    file_util::read(filename,body);

    std::cout<<body<<std::endl;
}


void db_test() //测试user table表     数据管理模块
{
    user_table ut(HOST,USER,PASS,DBNAME,PORT);
    Json::Value user;
    user["username"] = "xiaoming";
    user["password"] = "123123";




    //ut.insert(user);//测试插入的登录用户  数据展示




    bool ret1 = ut.login(user);//验证正确对应的账号密码才可以登录
    if(ret1 == false)
    {
        DBG_LOG("LOGIN FAILED!");
    }




    bool ret2 = ut.select_by_name("xiaoming",user);//通过名称或id来获取用户数据
     if(ret2 == false)
    {
        DBG_LOG("SELECT FAILED!");
    }
    std::string body;
    json_util::serialize(user,body);
    std::cout<<body<<std::endl;



   // bool ret3 = ut.win(1);//输赢的分数验证
    bool ret4 = ut.lose(1);

}


void online_test()
{
    online_manager om;
    wsserver_t::connection_ptr conn;
    uint64_t uid = 2;
    om.enter_game_hall(uid,conn);
    if(om.is_in_game_hall(uid)){
        DBG_LOG("IN GAME HALL");
    }else{
        DBG_LOG("NOT INGAME HALL");
    }

    om.exit_game_hall(uid);
    if(om.is_in_game_hall(uid)){
        DBG_LOG("IN GAME HALL");
    }else{
        DBG_LOG("NOT INGAME HALL");
    }

}

int main()
{

    //日志打印演示
    // INF_LOG("%s-%d","zmy",100);
    // DBG_LOG("%s","xxs");
    // ERR_LOG("%d",888);


    // json_test();
    // mysql_test();
    //str_test();
    //file_test();


    //db_test();

    //online_test();

    //user_table ut(HOST,USER,PASS,DBNAME,PORT);
    //online_manager om;

    //room r(10,&ut,&om);

    //room_manager rm(&ut,&om);
    //room_ptr rp = rm.create_room(10,20);


    //matcher mc(&rm,&ut,&om);


    gobang_server _server(HOST,USER,PASS,DBNAME,PORT);
    _server.start(8085);


    return 0;
}