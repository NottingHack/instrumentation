#include "CNHmqtt_irc.h"

using namespace std;

CNHmqtt_irc::CNHmqtt_irc(int argc, char *argv[]) : CNHmqtt(argc, argv)
{
  irc_in = get_str_option("irc", "mqtt_rx", "irc/rx");
  irc_out = get_str_option("irc", "mqtt_tx", "irc/tx");
  ircmsg_mqtt_tx = get_str_option("irc", "msg_mqtt_tx", "nh/ircmsg/tx");
  irc_channel    = get_str_option("irc", "channel", "nottingtest");
  
  slack_in  = get_str_option("slack", "mqtt_rx", "nh/slack/rx");
  slack_out = get_str_option("slack", "mqtt_tx", "nh/slack/tx");
  slack_irc_channel = get_str_option("slack", "irc_channel", "irc");
}

CNHmqtt_irc::~CNHmqtt_irc()
{
  
}

bool CNHmqtt_irc::init()
{
  if (mosq_connect())
    return false;
  
  subscribe(irc_in + "/#");
  subscribe(slack_in + "/#");
  daemonize();
  return true;
}

void CNHmqtt_irc::process_message(string topic, string message)
{ 
  string nick;
  string channel;

  bool is_from_irc   = is_irc_msg  (topic);
  bool is_from_slack = is_slack_msg(topic);

  if (is_from_irc || is_from_slack)
  {
    decode_irc_topic((is_from_irc ? irc_in: slack_in), topic, nick, channel);
    irc_msg msg = irc_msg(message, channel, nick, this, (is_from_irc ? irc_msg::MSGTYPE_IRC : irc_msg::MSGTYPE_SLACK));
    process_irc_message(msg);
  }

  CNHmqtt::process_message(topic, message);
}

// irc_in = base topic, e.g. "nh/irc/rx"
// topic  = topic actaully received, e.g. "nh/irc/rx/nottinghack/daniel1111
bool CNHmqtt_irc::decode_irc_topic(string irc_in, string topic, string &nick, string &channel)
{    
  
  if (irc_in.length() >= topic.length())
  {
    nick="";
    channel="";
    return false;
  }   
 
  // remove irc_in from front
  topic = topic.substr(irc_in.length()+1);
  
  if (topic.find_first_of("/") != string::npos)
  {
    // topic indicates it's a channel chat message
    channel = topic.substr(0, topic.find_first_of("/"));
    nick = topic.substr(topic.find_first_of("/")+1);
  }
  
  if (channel == "pm") // A 'Channel' of pm means it was actaully a private message
    channel = "";

  return true;    
}

int CNHmqtt_irc::irc_reply(string message, irc_msg msg)
{
  if (msg.channel=="")
    return irc_send_nick (message, msg.nick);
  else
    return irc_send_channel (message, msg.channel);
}

int CNHmqtt_irc::irc_send_nick (string message, string nick)
{
  return message_send(irc_out + "/pm/" + nick, message);
}


int CNHmqtt_irc::irc_send_channel (string message, string channel)
{
  return message_send(irc_out + "/" + channel, message);
}

int CNHmqtt_irc::slack_reply(string message, irc_msg msg)
{
  if (msg.channel=="")
    return slack_send_nick (message, msg.nick);
  else
    return slack_send_channel (message, msg.channel);
}

int CNHmqtt_irc::slack_send_nick (string message, string nick)
{
  return message_send(slack_out + "/pm/" + nick, message);
}


int CNHmqtt_irc::slack_send_channel (string message, string channel)
{
  printf("slack_send_channel> CHANNEL = [%s], MESSAGE = [%s]\n", channel.c_str(), message.c_str());
  return message_send(slack_out + "/" + channel, message);
}


bool CNHmqtt_irc::is_irc_msg(string topic)
{
  if (topic.length() > irc_in.length())
    if(topic.substr(0, irc_in.length())==irc_in)
      return true;

  return false;
}

bool CNHmqtt_irc::is_slack_msg(string topic)
{
  if (topic.length() > slack_in.length())
    if(topic.substr(0, slack_in.length())==slack_in)
      return true;

  return false;
}
