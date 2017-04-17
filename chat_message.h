#ifndef BOOST_SERVER_CHAT_MESSAGE_H
#define BOOST_SERVER_CHAT_MESSAGE_H

#include <cstdio>
#include <cstdlib>
#include <cstring>

class chat_message {
public:
    enum {
        header_length = 4
    };
    enum {
        id_lenght = 4
    };
    enum {
        max_body_lenght = 512
    };

    chat_message() : body_lenght_(0), id_("0000")
    {}


    const char *data() const
    {
        return data_;
    }

    char *data()
    {
        return data_;
    }

    void data(char ptr[])
    {
        strcpy(data_, ptr);
    }


    char* id()
    {
        return data_ + header_length;
    }
    size_t lenght() const
    {
        return header_length + id_lenght + max_body_lenght;
    }

    const char *body() const
    {
        return data_ + header_length + id_lenght;
    }

    char *body()
    {
        return data_ + header_length + id_lenght;
    }

    void body_lenght(size_t new_len)
    {
        body_lenght_ = new_len;
        if(body_lenght_ > max_body_lenght)
            body_lenght_ = max_body_lenght;
    }

    const size_t body_length() const
    {
        return body_lenght_;
    }

    bool decode_header()
    {
        using namespace std;

        char header[header_length + 1] = "";
        strncat(header, data_, header_length);
        body_lenght_ = (size_t) atoi(header);
        if(body_lenght_ > max_body_lenght)
        {
            body_lenght_ = 0;
            return false;
        }
        return true;
    }

    bool encode_header()
    {
        using namespace std;
        char header[header_length + 1] = "";
        sprintf(header, "%4d", (int) body_lenght_);
        memcpy(data_, header, header_length);
    }

    bool decode_id()
    {
        using namespace std;
        int i = 0;
        for(char* c = data_ + header_length; i < id_lenght; c++)
        {
            id_[i] = *c;
            i++;
        }
        return true;
    }

    bool encode_id()
    {
        using namespace std;
        memcpy(data_+header_length, id_, id_lenght);
        return true;
    }
private:
    char data_[header_length + id_lenght + max_body_lenght];
    size_t body_lenght_;
    char id_[id_lenght + 1];
};

#endif