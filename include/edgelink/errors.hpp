#pragma once

namespace edgelink {

class ArgumentException : public std::invalid_argument {};

class IOException : public std::exception {
  public:
    IOException(const std::string& message) : _message(message) {}
    IOException(const char* message) : _message(message) {}

    // 重载 std::exception 的 what 函数，返回异常的描述信息
    const char* what() const noexcept override { return _message.c_str(); }

  private:
    std::string _message;
};

class BadConfigException : public std::exception {
  public:
    BadConfigException(const std::string& key, const std::string& message) : _key(key), _message(message) {}
    const char* what() const noexcept override { return _message.c_str(); }
    const std::string_view key() const noexcept { return _key.c_str(); }

  private:
    std::string _message;
    std::string _key;
};

}; // namespace edgelink