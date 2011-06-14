drop table if exists members;

create table members 
(
  member_id     int not null auto_increment,
  member_number int,
  name          varchar(50),
  handle        varchar(100),
  unlock_text   varchar(95),
  primary key (member_id),
  constraint member_name   unique (handle),
  constraint member_number unique (member_number)
);