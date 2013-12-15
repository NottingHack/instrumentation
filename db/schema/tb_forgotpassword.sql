drop table if exists forgotpassword;

create table forgotpassword 
(
  member_id           int(11) not null,
  request_guid        char(36) not null,
  timestamp           timestamp not null default CURRENT_TIMESTAMP,
  expired             tinyint(1) not null default '0',
  unique key (request_guid)
) ENGINE = InnoDB; 
