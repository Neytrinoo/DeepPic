#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "Connection.h"

Connection::Connection(boost::asio::io_context &service, std::function<void(std::shared_ptr<Connection>, std::string &&)> on_read_cb,
                       std::function<void(std::shared_ptr<Connection>)> on_delete_cb) : sock_(service),
                                                                                        onReadCb_(std::move(on_read_cb)),
                                                                                        onDeleteCb_(std::move(on_delete_cb)) {

}


void Connection::afterConnect() {
    read();
}

void Connection::read() {
    std::cerr << "we are in Connection::read" << std::endl;
    boost::asio::async_read(sock_, boost::asio::buffer(readBuf_),
                            boost::bind(&Connection::checkEndOfRead, shared_from_this(), boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred),
                            boost::bind(&Connection::readHandler, shared_from_this(), boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void Connection::write(std::string &command, std::function<void(std::shared_ptr<Connection>)> onWriteCb) {
    for (int i = 0; i < command.length(); ++i) {
        sendBuf_[i] = command[i];
    }
    sock_.async_write_some(boost::asio::buffer(sendBuf_, command.length()),
                           boost::bind(&Connection::writeHandler, shared_from_this(), _1, _2, onWriteCb));
}


void Connection::readHandler(const boost::system::error_code &err, std::size_t bytes_transferred) {
    if (err) {
        if (onDeleteCb_) {
            onDeleteCb_(shared_from_this());
        }
        return;
    }

    std::cerr << "read = " << std::string(readBuf_, readBuf_ + bytes_transferred) << std::endl;
    onReadCb_(shared_from_this(), std::string(readBuf_, readBuf_ + bytes_transferred));
    read();
}


std::size_t Connection::checkEndOfRead(const boost::system::error_code &err, std::size_t bytes_transferred) {
    if (bytes_transferred > 0 &&
        std::string(readBuf_ + bytes_transferred - 1 - std::strlen(END_STR), readBuf_ + bytes_transferred - 1) == std::string(END_STR)) {
        return 0;
    }
    return 1;
}


void Connection::writeHandler(const boost::system::error_code &err, std::size_t bytes_transferred,
                              std::function<void(std::shared_ptr<Connection>)> onWriteCb) {
    if (err) {
        if (onDeleteCb_) {
            onDeleteCb_(shared_from_this());
        }
        return;
    }
    std::cerr << "we are on Connection::writeHandler" << std::endl;

    if (onWriteCb) {
        onWriteCb(shared_from_this());
    }
}

void Connection::stop() {
    sock_.close();
}

boost::asio::ip::tcp::socket &Connection::getSock() {
    return sock_;
}
