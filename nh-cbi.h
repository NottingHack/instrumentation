#pragma once

class ToolsCallbackInterface
{
  public:
    virtual int cbiSendMessage(std::string, std::string) = 0;
};
