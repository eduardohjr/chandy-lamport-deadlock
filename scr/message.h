#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <sstream>


enum class MessageType {
    REQUEST_FORK,
    RELEASE_FORK,
    FORK_GRANTED,
    MARKER,
    SNAPSHOT_DATA
};

struct Message {
    MessageType type;
    int senderId;
    std::string content;

    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

#endif