/*************************************************************************
  > File Name: MyDataBase.cpp
  > Author: lewin
  > Mail: lilinhan1303@gmail.com
  > Company:  Xiyou Linux Group
  > Created Time: 2015年08月02日 星期日 17时45分16秒
 ************************************************************************/


#include<iostream>
#include<string>
#include<cstdlib>
#include<cerrno>
#include<mysql/mysql.h>
#include"jsoncpp-src-0.5.0/include/json/json.h"
#include<sstream>
#include<stdio.h>


#define HOST "localhost"
#define DATABASE "CloudBackup"
#define USER "root"
#define PASSWD "lewin123"
#define PORT 3306

class MyDataBase {
    public:
        MyDataBase() :res(NULL) {
            mysql_init(&mysql);
            /*连接数据库*/
            if(!mysql_real_connect(&mysql , HOST , USER , PASSWD , DATABASE , 0 , NULL , 0))  {
                std::cout << "连接数据库发生错误!"  << std::endl;
                return;
            }
            std::string name = "lyh";
            std::string sql = "select * from UserInfo = " +name;
            mysql_real_query(&mysql , sql.c_str() , sql.length());
            res = mysql_use_result(&mysql);

            while((row = mysql_fetch_row(res)) != NULL) {
                std::cout << row[0] <<" " << row[1] << "  " << row[2] <<"  "<<row[3]<<std::endl;
            }
            mysql_free_result(res);
            std::cout << "连接数据库成功!" << std::endl;
        }
        ~MyDataBase() {
            std::cout << "exit" << std::endl;
            mysql_close(&mysql);
        }

        /*查询数据库  参数为sql语句*/
        void MySqlQuery(std::string & sql)  {
            std::cout << sql <<std::endl;
            int err = mysql_query(&mysql , sql.c_str());
            if(0 != err){
                std::cout << "Error Making Query:" << mysql_error(&mysql) << std::endl;
            } else {
                res = mysql_use_result(&mysql);
                printf("res &= %p\n",res);
            }
        }

        /*验证账户和密码 返回-1代表登录失败 1为登录成功*/
        int AccountPasswd(std::string UserName , std::string PassWord) {
            std::cout << UserName << "  " << PassWord << std::endl;
            std::string sql = "select * from UserInfo where Account=\"" + UserName+"\";";
         //   MySqlQuery(sql);
            mysql_query(&mysql , sql.c_str());
            res = mysql_use_result(&mysql);
            while((row = mysql_fetch_row(res)) != NULL) {
                //判断UserName和Account是否相等
                if(UserName != row[1] || PassWord != row[2])  {
                    mysql_free_result(res);
                    return -1;
                }
                else {
                    Uid = atoi(row[0]);
                    mysql_free_result(res);
                    return 1;
                }
            }
            mysql_free_result(res);
            return 0;
        }

        /*注册用户  先判断有没有此用户*/
        int Register(std::string UserName , std::string PassWord , std::string Email)  {
            std::string sql = "select * from UserInfo where Account=\""+ UserName+"\" OR Email=\"" + Email + "\";";
         //   MySqlQuery(sql);
            std::cout<<sql<<std::endl;
            mysql_query(&mysql,sql.c_str());
            res = mysql_use_result(&mysql);
            std::cout<<UserName<<"  "<<PassWord<<"  "<<Email<<std::endl;
            int i = 0 ;
            while ( (row = mysql_fetch_row(res)) != NULL)  {
                std::cout<<"开始验证用户注册信息\n";
                if( row[1] == UserName ) {
                    std::cout << "该用户已存在" << std::endl;
                    mysql_free_result(res);
                    return 0;
                }
                else if(row[3] == Email)
                {
                    std::cout<<"邮箱已被占用"<< std::endl;
                    mysql_free_result(res);
                    return 2;
                }
            }
            mysql_free_result(res);
            std::cout<<"注册信息合法\n";
            sql = "insert into UserInfo(Account,Password,Email) values(\"" + UserName +"\",\""+PassWord+"\",\""+Email+"\");";
            if( 0 == mysql_query(&mysql , sql.c_str())) {
                //[未完成]数据成功的log
                return 1;
            }else {
                //[未完成]数据失败的log
                //mysql_close(&mysql);
                return -1;
            }
        }
        //进入目录  在数据库中查找符合条件的目录  并且将该目录下的一级文件或者目录
        std::string DirFiles(std::string dir)  {
            Json::Value root;
            std::string sql = "select * from UserInfo where UserFilePath like " + dir + "$;";
            MySqlQuery(sql);
            int i = 0;
            int num = dir.length();
            std::string temp;
            while((row = mysql_fetch_row(res))) {
                std::string str = row[0];
                int loc = str.find('/' , num);
                if(loc)  {
                    temp = str.substr(num , loc - num + 1);
                }else {
                    temp = str.substr(num - 1 , str.rfind("\0"));
                }
                std::string numstr = std::to_string(i);
                root[numstr] = temp;
            }
            return root.toStyledString();
        }

        //新建目录
        int CreateNewDir(std::string DirFiles)  {
            std::ostringstream str;
            str << Uid;

            std::string idstr(str.str());
            std::string sql = "insert into UserFileInfo(Uid ,UserFileSize , UserFilePath) values( "+idstr+",0,"+DirFiles+");";
            if(0 != mysql_query(& mysql,sql.c_str())) {
                return -1;
            }
            return 1;
        }

        //重命名文件或目录
        int RenameFileName(std::string OldName , std::string NewName)  {
            std::string sql = "update UserFileInfo set UserFilePath="+NewName+"where UserFilePath="+OldName+";";
            if(0 != mysql_query(&mysql,sql.c_str()))  {
                return -1;
            }
            return 1;
        }

        //删除目录或文件
        int DeleteFileORDir(std::string File)  {
            std::string sql = "select MD5 from UserFileInfo where UserFilePath="+File+");";
            MySqlQuery(sql);
            row = mysql_fetch_row(res);
            std::string str = row[0];
            sql = "select * from MD5 where MD5="+str+";";
            MySqlQuery(sql);
            int i = 1;
            std::string temp;
            while((row = mysql_fetch_row(res))) {
                if( i%3 == 0 && strcmp(row[0],"1") == 0)  {
                    temp = row[0];
                    sql = "delete from UserFileInfo where UserFilePath="+File+";";
                    mysql_query(&mysql , sql.c_str());
                }
                i++;
            }
            int num = atoi(temp.c_str());
            num--;

            std::ostringstream string;
            string << num;

            std::string ss(string.str());
            sql = "update MD5 set ReferCount="+ss+"where MD5="+str+";";
            if( 0 != mysql_query(&mysql , sql.c_str()))  {
                return -1;
            }else {
                return 1;
            }

        }

        //上传文件
        int UploadFile(std::string UserFilePath , std::string UserFileSize , std::string ServerFilePath , std::string MD5 , int flag)  {
            std::string sql = "select MD5 from UserFileInfo where UserFilePath="+UserFilePath+");";
            MySqlQuery(sql);
            row = mysql_fetch_row(res);
            std::string str = row[0];
            sql = "select * from MD5 where MD5="+str+";";
            MySqlQuery(sql);
            int i = 1;
            while((row = mysql_fetch_row(res))) {
                int temp = atoi(row[0]);
                if( i%3 == 0 && temp >= 1)  {
                    std::ostringstream string;
                    string << temp+1;
                    std::string ss(string.str());
                    sql = "update MD5 set ReferCount="+ss+"where MD5="+str+";";
                    mysql_query(&mysql , sql.c_str());
                }else {
                    std::ostringstream string;
                    string << Uid;
                    std::string uid = string.str();
                    string << flag;
                    std::string f = string.str();

                    std::string sql = "insert into UserFileInfo(Uid , UserFilePath , UserFileSize , ServerFilePath , MD5 , Flag) values("+uid+","+UserFileSize+","+UserFileSize+","+ServerFilePath+","+MD5+","+f+");";
                    if( 0 != mysql_query(&mysql , sql.c_str()))  {
                        return -1;
                    }
                    return 1;
                }
            }
            return 0;
        }
        //下载文件
        MYSQL_RES*   DownloadFile(std::string UserFilePath)  {
            std::string sql = "select * from UserFileInfo where UserFilePath =\"" + UserFilePath+"\";";
            MySqlQuery(sql);
            return res;
        }

        //监控文件
        int MonitorFile(std::string UserFilePath , std::string UserFileSize , std::string ServerFilePath , std::string MD5 , int flag)  {
            int Flag = UploadFile(UserFilePath , UserFileSize , ServerFilePath , MD5 , flag);
            if( 1 == Flag )   {
                return true;
            }
            return false;
        }

        //恢复文件
        std::string RestoreFile(std::string UserFilePath)  {
//            std::string temp = DownloadFile(UserFilePath);
             std::string   temp; //暂时并没有什么卵用
            return temp;
        }
    private:
        MYSQL mysql;  //连接数据库的变量
        MYSQL_RES *res;  //存放查询结果的变量
        MYSQL_ROW row;
        int Uid;  //用户ID
};

