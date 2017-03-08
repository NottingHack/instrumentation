drop table if exists tl_google_notifications;

create table tl_google_notifications 
(
  channel_id          varchar(64),
  tool_id             int unsigned,
  channel_token       varchar(256),
  resource_id         varchar(256),
  channel_created     timestamp default CURRENT_TIMESTAMP,
  channel_expiration  timestamp,
  primary key (channel_id)
) ENGINE = InnoDB; 
