drop table if exists access_log;

create table access_log 
(
  access_id     int not null auto_increment,
  access_time   timestamp default CURRENT_TIMESTAMP,
  rfid_serial   varchar(50),
  pin           varchar(50),
  access_result int, -- 10=denied, 20=granted
  member_id     int,
  door_id       int,
  denied_reason varchar(100),
  primary key (access_id)
);