drop table if exists members;

create table members 
(
  member_id     int not null auto_increment,
  member_number int,
  name          varchar(50),
  email         varchar(100),
  join_date     date,
  handle        varchar(100),
  unlock_text   varchar(95),
  balance       int not null,
  credit_limit  int not null,
  primary key (member_id),
  constraint member_handle unique (handle),
  constraint member_number unique (member_number)
) ENGINE = InnoDB; 