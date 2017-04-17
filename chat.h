
#ifndef BOOST_SERVER_1_CHAT_H
#define BOOST_SERVER_1_CHAT_H

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include "chat_message.h"
#include "func.h"
#include <cstring>
#include <cstdio>
using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_participant
{
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;

};

typedef boost::shared_ptr<chat_participant> chat_participant_ptr;

class chat_room
{
public:

    void join(chat_participant_ptr participant)
    {
        participants_.insert(participant);
        std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
            boost::bind(&chat_participant::deliver, participant, _1));
    }

    void leave(chat_participant_ptr participant)
    {
        participants_.erase(participant);

        chat_message leave_msg;
        char msg[] = "00350001user disconnected from your channel";
        leave_msg.data(msg);
        leave_msg.encode_header();
        leave_msg.encode_id();
        std::for_each(participants_.begin(), participants_.end(),
            boost::bind(&chat_participant::deliver, _1, leave_msg));

    }

    void deliver(const chat_message& msg, chat_participant_ptr sender)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto iter = participants_.begin(); iter != participants_.end(); iter++)
            if( (*iter) != sender)
            {
                (*iter)->deliver(boost::ref(msg));
            }
    }

private:
    std::set<chat_participant_ptr> participants_;
    chat_message_queue recent_msgs_;
    enum {max_recent_msgs = 100};
};


class chat_session: public chat_participant,
                    public boost::enable_shared_from_this<chat_session>
{

public:

    chat_session(boost::asio::io_service& io_service,   chat_room& room):
            socket_(io_service), room_(room){}

    tcp::socket& socket()
    {
        return socket_;
    }


    void start()
    {
        room_.join(shared_from_this());
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                                boost::bind(
                                        &chat_session::handle_read_header, shared_from_this(),
                                        boost::asio::placeholders::error));
    }

    void deliver(const chat_message& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front().data(),
                                                         write_msgs_.front().lenght()),
                                     boost::bind(&chat_session::handle_write, shared_from_this(),
                                                 boost::asio::placeholders::error));
        }
    }

    void handle_read_header(const boost::system::error_code& error)
    {
        if (!error && read_msg_.decode_header())
        {
            boost::asio::async_read(socket_,
                                    boost::asio::buffer(read_msg_.id(), chat_message::id_lenght),
                                    boost::bind(&chat_session::handle_read_id, shared_from_this(),
                                                boost::asio::placeholders::error));
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    void handle_read_id(const boost::system::error_code& error)
    {
        if (!error && read_msg_.decode_id())
        {
            boost::asio::async_read(socket_,
                                    boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                                    boost::bind(&chat_session::handle_read_body, shared_from_this(),
                                                boost::asio::placeholders::error));
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    void handle_read_body(const boost::system::error_code& error)
    {
        if (!error)
        {
            chat_message encrypt_msg = encrypt(read_msg_);
            room_.deliver(encrypt_msg, shared_from_this());
            boost::asio::async_read(socket_,
                                    boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                                    boost::bind(&chat_session::handle_read_header, shared_from_this(),
                                                boost::asio::placeholders::error));
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    chat_message encrypt(chat_message& msg)
    {
        return msg;
    }

    chat_message decrypt(chat_message& msg)
    {
        return msg;
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
                chat_message decryted_msg = decrypt(write_msgs_.front());
                boost::asio::async_write(socket_,
                                         boost::asio::buffer(decryted_msg.data(),
                                                             decryted_msg.lenght()),
                                         boost::bind(&chat_session::handle_write, shared_from_this(),
                                                     boost::asio::placeholders::error));
            }
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    ~chat_session(){}
private:
    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;

};

typedef boost::shared_ptr<chat_session> chat_session_ptr;

class chat_server
{
public:
    chat_server(boost::asio::io_service& io_service,
                const tcp::endpoint& endpoint)
            : io_service_(io_service),
              acceptor_(io_service, endpoint)
    {
        std::ifstream in("log.txt");

        strcpy(last_id_s_, " ");
        char *buf = new char[1024];
        while(in.getline(buf, 1024))
        {
            char* name = new char[1020];
            int i = 0;
            for(char* c = buf; *c != ' ' && i < 1000;c++, i++)
            {
                name[i] = *c;
            }
            char* pass = new char[1020];
            i++;
            int j = 0;
            for(char* c = buf + i; *c != ' ' && i < 1000;c++, i++, j++)
            {
                pass[j] = *c;
            }
            char* id = new char[1020];
            i++;
            j = 0;
            for(char* c = buf + i; *c != ' ' && i < 1000 && *c != '\0';c++, i++, j++)
            {
                id[j] = *c;
            }
            log_[name] = std::make_pair(pass, id);
            ids_[id] = name;
            if(strcmp(id, last_id_s_) >= 0)
            {
                strcpy(last_id_s_, id);
            }
        }

        last_id_ = atoi(last_id_s_);
        last_id_++;
        std::sprintf(last_id_s_, "%04d", last_id_);

        for(auto iter = ids_.begin(); iter != ids_.end(); iter++)
            std::cout << iter->first << " " << iter->second << std::endl;

        start_accept();
    }

    void start_accept()
    {
        chat_session_ptr new_session(new chat_session(io_service_, room_));
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&chat_server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(chat_session_ptr session,
                       const boost::system::error_code& error)
    {
        if(!error)
        {
            boost::asio::async_read(session->socket(), boost::asio::buffer(header, max_header_lenght),
                boost::bind(&chat_server::handle_header, this,
                            session, boost::asio::placeholders::error));
        }
        else
        {
            start_accept();
        }

    }

    void handle_header(chat_session_ptr session,
                        const boost::system::error_code& error)
    {
        if(!error)
        {
            int lenght = atoi(header);

            boost::asio::async_read(session->socket(), boost::asio::buffer(buf, lenght),
                                    boost::bind(&chat_server::handle_message, this,
                                                session, boost::asio::placeholders::error));
        } else{
            start_accept();
        }
    }

    void handle_message(chat_session_ptr session,
                        const boost::system::error_code& error)
    {
        if(!error) {
            if (substr(buf, max_reg_length, 0, 3) == "reg")
            {
                std::ofstream out("log.txt", std::ios::app);
                int del = 4;
                for(; buf[del] != ' ' && del < 1024; del++);
                int end = del;
                for(; buf[end] != ' ' && end < 1024; end++);
                char* name = new char[1024];
                char* pass = new char[1024];
                name = substr(buf, 1024, 4, del);
                pass = substr(buf, 1024, del, end);
                out << name << " " << pass << " " << last_id_s_;
                ids_[last_id_s_] = name;
                last_id_++;
                strcpy(last_id_s_ ,std::to_string(last_id_).c_str());
                chat_message msg;
                char* message = new char[1024];
                strcpy(message, strcat(name, " "));
                strcmp(message, strcat(message, last_id_s_));
                std::cout<<message << std::endl;
                strcpy(msg.body(), message);
                session->start();
            }
            else
            {
                int del = 4;
                for(; buf[del] != ' ' && del < 1024; del++);
                int end = del;
                for(; buf[end] != ' ' && end < 1024; end++);
                char* name = new char[1024];
                char* pass = new char[1024];
                if(strcmp(log_[name].second, pass) == 0)
                {
                    char* message = new char[20];
                    strcat(message, "00040001");
                    strcat(message, log_[name].first);
                    chat_message msg;
                    strcpy(msg.data(), message);

                    session->deliver(msg);
                    session->start();
                }
            }


            start_accept();
        }else
        {
            start_accept();
        }

    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    chat_room room_;
    std::map<char*, char*> ids_;

    std::map<char*, std::pair<char*, char*> > log_;
    int last_id_;

    enum{max_reg_length = 1024};
    char last_id_s_[max_reg_length];
    enum{max_header_lenght};
    char buf[max_reg_length];
    char header[max_header_lenght];
};
#endif //BOOST_SERVER_1_CHAT_H
