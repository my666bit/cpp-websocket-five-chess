#ifndef __M_ROOM_H__
#define __M_ROOM_H__
#include "util.hpp"
#include "logger.hpp"
#include "online.hpp"
#include "db.hpp"

#define BOARD_ROW 15
#define BOARD_COL 15
#define CHESS_WHITE 1
#define CHESS_BLACK 2
typedef enum{GAME_START,GAME_OVER}room_status;

class room{
    private:
        uint64_t _room_id;
        room_status _status;
        int _player_count;
        uint64_t _white_id;
        uint64_t _black_id;


        user_table *_tb_user;//类指针进行表的管理
        online_manager *_online_user;//类指针进行在线用户的管理
        std::vector<std::vector<int>> _board;

    private:
        bool five(int row,int col,int row_off,int col_off,int color){
            //row和col是下棋位置，，，，，row_off和col_off是偏移量，也是方向
            int count = 1;
            int search_row = row + row_off;
            int search_col = col + col_off;
            while(search_row>=0 && search_row<BOARD_ROW &&
                search_col>=0 && search_col<BOARD_COL &&
                _board[search_row][search_col] == color){
                    count++;//同色棋子数量加加

                    //检索位置继续向后偏移
                    search_row += row_off;
                    search_col += col_off;
                }

            search_row = row - row_off;
            search_col = col - col_off;
            while(search_row>=0 && search_row<BOARD_ROW &&
                search_col>=0 && search_col<BOARD_COL &&
                _board[search_row][search_col] == color){
                    count++;//同色棋子数量加加

                    //检索位置继续向后偏移
                    search_row -= row_off;
                    search_col -= col_off;
                }
            return (count>=5);
        }

        uint64_t check_win(int row,int col,int color){//从下棋位置的四个方向上检测是否出现了五个及以上的棋子（横行，纵列，正斜，反斜）
            if(five(row,col,0,1,color) ||
                five(row,col,1,0,color) ||
                five(row,col,-1,1,color) ||
                five(row,col,-1,-1,color)){
                //任意一个方向上出现了true也就是五星连珠，则设置返回值
                return color == CHESS_WHITE ? _white_id : _black_id;
                }
            return 0;/*返回胜利玩家id,否则没有胜利者则返回0*/
        }

    public:
        room(uint64_t room_id,user_table *tb_user,online_manager *online_user)
            :_room_id(room_id),_status(GAME_START),_player_count(0),
            _tb_user(tb_user),_online_user(online_user),
            _board(BOARD_ROW,std::vector<int>(BOARD_COL,0))
            {
                DBG_LOG("%lu 房间创建成功！！！",_room_id);
            }
        ~room()
        {
            DBG_LOG("%lu 房间销毁成功！！！",_room_id);
        }

        uint64_t id(){
            return _room_id;
        }

        room_status status(){
            return _status;
        }

        int player_count(){
            return _player_count;
        }

        void add_white_user(uint64_t uid){
            _white_id = uid;
            _player_count++;
        }

        void add_black_user(uint64_t uid){
            _black_id = uid;
            _player_count++;
        }

        uint64_t get_white_user(){return _white_id;}

        uint64_t get_black_user(){return _black_id;}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //处理下棋动作
        Json::Value handle_chess(Json::Value &req)//请求
        {
            Json::Value json_resp = req;
            //1.当前请求的房间号是否与当前房间号匹配
           
            //2.判断房间内两个玩家是否都在线，任意一个不在线，就是另一方胜利
            int chess_row = req["row"].asInt();
            int chess_col = req["col"].asInt();
            uint64_t cur_uid = req["uid"].asUInt64();
            if(_online_user->is_in_game_room(_white_id) == false){
                json_resp["result"] = true;
                json_resp["reason"] = "运气真好！对方掉线，不战而胜！";
                json_resp["winner"] = (Json::UInt64)_black_id;
                return json_resp; 
            }

            if(_online_user->is_in_game_room(_black_id) == false){
                json_resp["result"] = true;
                json_resp["reason"] = "运气真好！对方掉线，不战而胜！";
                json_resp["winner"] = (Json::UInt64)_white_id;
                return json_resp;
            }
            //3.获取走棋位置，判断当前走棋是否合理（位置是否已经被占用）
            if(_board[chess_row][chess_col]!=0){
                json_resp["result"] = false;
                json_resp["reason"] = "当前位置已经有了其他棋子";
                return json_resp;
            }

            int cur_color = cur_uid == _white_id ? CHESS_WHITE:CHESS_BLACK;
            _board[chess_row][chess_col] = cur_color;
            //4.判断是否有玩家胜利（从当前走棋位置开始判断是否存在五星连珠）
            uint64_t winner_id = check_win(chess_row,chess_col,cur_color);
            if(winner_id!=0){
                //更新数据库用户信息
               json_resp["reason"] = "五星连珠，战无敌！";
            }
            json_resp["result"] = true;
            json_resp["winner"] = (Json::UInt64)winner_id;
            return json_resp;
        }


        //处理聊天动作
        Json::Value handle_chat(Json::Value &req)//请求
        {
            Json::Value json_resp = req;//赋值作用：把客户端原始请求 json 直接拷贝一份当成响应，再追加 / 覆盖字段  ，全新空 json 对象，自己手动一个个添加需要的字段
            //检测房间是否一致
          

            //检测消息中是否包含敏感词
            std::string msg = req["message"].asString();
            size_t pos = msg.find("垃圾");
            if(pos!=std::string::npos){
                json_resp["result"] = false;
                json_resp["reason"] = "消息中包含敏感词，不能发送！";
                return json_resp;
            }

            //广播消息，返回消息
            json_resp["result"] = true;
            return json_resp;
        }


        //处理玩家退出房间动作
        void handle_exit(uint64_t uid)
        {
            //如果是下棋中退出，则对方胜利，否则下棋结束退出，则是正常退出
            Json::Value json_resp;
            if(_status == GAME_START){
                json_resp["optype"] = "put_chess";
                json_resp["result"] = true;
                json_resp["reason"] = "对方掉线，不战而胜！";
                json_resp["room_id"] = (Json::UInt64)_room_id;
                json_resp["uid"] = (Json::UInt64)uid;
                json_resp["row"] = -1;
                json_resp["col"] = -1;
                json_resp["winner"] = (Json::UInt64)(uid == _white_id ? _black_id : _white_id);
                //广播
                broadcast(json_resp);
            }
            
            //房间中玩家数量--
            _player_count--;
            return;

        }


        //总的请求处理函数，在函数内部，区分请求类型，根据不同的请求调用不同的处理函数，得到响应进行广播
        void handle_request(Json::Value &req)
        {
            //1.校验房间号是否匹配
             Json::Value json_resp;
            uint64_t room_id = req["room_id"].asUInt64();
            if(room_id!=_room_id)
            {
                json_resp["optype"] = req["optype"].asString();
                json_resp["result"] = false;
                json_resp["reason"] = "房间号不匹配";
                return broadcast(json_resp);

            }

             //2.根据不同的请求调用不同的处理函数
             if(req["optype"].asString() == "put_chess"){
                json_resp = handle_chess(req);
                if(json_resp["winner"].asUInt64()!=0){
                     uint64_t winner_id = json_resp["winner"].asUInt64();
                     uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                    _tb_user->win(winner_id);
                    _tb_user->lose(loser_id);
                    _status = GAME_OVER;
                }
             }
             else if(req["optype"].asString() == "chat"){
                json_resp = handle_chat(req);
             }else{
                json_resp["optype"] = req["optype"].asString();
                json_resp["result"] = false;
                json_resp["reason"] = "未知请求类型";
             }
             
             return broadcast(json_resp);
        }


        //将指定信息广播给房间中所有玩家
        void broadcast(Json::Value &rsp)//响应
        {
            //1.对要响应的信息进行序列化，将json：：value中的数据序列化json格式的字符串
            std::string body;
            json_util::serialize(rsp,body);
            //2.获取房间中所有用户的通信连接
            //3.发送响应信息
            wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
            if(wconn.get()!=nullptr){
                wconn->send(body);
            }


            wsserver_t::connection_ptr bconn = _online_user->get_conn_from_room(_black_id);
            if(bconn.get()!=nullptr){
                bconn->send(body);
            }
            return;
        }      

};



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
using room_ptr = std::shared_ptr<room>;//房间信息的空间使用shard_ptr进行管理
//房间管理类的实现
class room_manager{
    private:
        uint64_t _next_rid;//房间id
        std::mutex _mutex;
        user_table *_tb_user;
        online_manager *_online_user;
        std::unordered_map<uint64_t, room_ptr> _rooms;//建立起房间id与房间信息的映射关系
        std::unordered_map<uint64_t, uint64_t> _users;//房间id与用户id的关联关系管理（先通过用户id找到所在房间id，在查找房间信息）
    public:
        /*初始化房间ID计数器*/
        room_manager(user_table *ut, online_manager *om):
        _next_rid(1), _tb_user(ut), _online_user(om) {
        DBG_LOG("房间管理模块初始化完毕！");
        }
        ~room_manager() { DBG_LOG("房间管理模块即将销毁！"); }
        //为两个⽤⼾创建房间，并返回房间的智能指针管理对象
        room_ptr create_room(uint64_t uid1, uint64_t uid2) {
            //两个⽤⼾在游戏⼤厅中进⾏对战匹配，匹配成功后创建房间
            //1. 校验两个⽤⼾是否都还在游戏⼤厅中，只有都在才需要创建房间。
            if (_online_user->is_in_game_hall(uid1) == false) {
                DBG_LOG("⽤⼾：%lu 不在⼤厅中，创建房间失败!", uid1);
                return room_ptr();//返回空对象
            }
            if (_online_user->is_in_game_hall(uid2) == false) {
                DBG_LOG("⽤⼾：%lu 不在⼤厅中，创建房间失败!", uid2);
                return room_ptr();//返回空对象
            }
            //2. 创建房间，将⽤⼾信息添加到房间中
            std::unique_lock<std::mutex> lock(_mutex);
            room_ptr rp(new room(_next_rid, _tb_user, _online_user));
            rp->add_white_user(uid1);
            rp->add_black_user(uid2);
            //3. 将房间信息管理起来
            _rooms.insert(std::make_pair(_next_rid, rp));
            _users.insert(std::make_pair(uid1, _next_rid));
            _users.insert(std::make_pair(uid2, _next_rid));
            _next_rid++;
            //4. 返回房间信息
            return rp;
        }




        /*通过房间ID获取房间信息*/
        room_ptr get_room_by_rid(uint64_t rid) {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _rooms.find(rid);
            if (it == _rooms.end()) {
                return room_ptr();
            }
            return it->second;
        }




        /*通过⽤⼾ID获取房间信息*/
        room_ptr get_room_by_uid(uint64_t uid) {
            std::unique_lock<std::mutex> lock(_mutex);
            //1. 通过⽤⼾ID获取房间ID
            auto uit = _users.find(uid);
            if (uit == _users.end()) {
                return room_ptr();
            }
            uint64_t rid = uit->second;
            //2. 通过房间ID获取房间信息
            auto rit = _rooms.find(rid);
            if (rit == _rooms.end()) {
                return room_ptr();
            }
            return rit->second;
        }



        /*通过房间ID销毁房间*/
        void remove_room(uint64_t rid) {
            //因为房间信息，是通过shared_ptr在_rooms中进⾏管理，因此只要将shared_ptr从_rooms中移除
            //则shared_ptr计数器==0，外界没有对房间信息进⾏操作保存的情况下就会释放
            //1. 通过房间ID，获取房间信息
            room_ptr rp = get_room_by_rid(rid);
            if (rp.get() == nullptr) {
                return;
            }
            //2. 通过房间信息，获取房间中所有⽤⼾的ID
            uint64_t uid1 = rp->get_white_user();
            uint64_t uid2 = rp->get_black_user();
            //3. 移除房间管理中的⽤⼾信息
            std::unique_lock<std::mutex> lock(_mutex);
            _users.erase(uid1);
            _users.erase(uid2);
            //4. 移除房间管理信息
            _rooms.erase(rid);
        }




        /*删除房间中指定⽤⼾，如果房间中没有⽤⼾了，则销毁房间，⽤⼾连接断开时被调⽤*/
        void remove_room_user(uint64_t uid) {
            room_ptr rp = get_room_by_uid(uid);
            if (rp.get() == nullptr) {
                return;
            }
            //处理房间中玩家退出动作
            rp->handle_exit(uid);
            //房间中没有玩家了，则销毁房间
            if (rp->player_count() == 0) {
                remove_room(rp->id());
            }
            return ;
        }



        };

#endif