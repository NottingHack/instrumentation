drop table if exists light_level;

create table light_level
(
  name varchar(100),
  sensor varchar(30) not null,
  reading int,
  time timestamp default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  primary key (sensor)
);