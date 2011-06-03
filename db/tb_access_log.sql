drop table if exists access_log;

create table access_log 
(
  access_id     int not null auto_increment,
  access_time   timestamp,
  rfid_serial   varchar(50),
  primary key (access_id)
);