drop table if exists members;

create table members 
(
  member_id     int not null auto_increment,
  name          varchar(50),
  handle        varchar(100),
  primary key (member_id),
  constraint member_name unique (name)
);