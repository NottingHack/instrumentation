drop table if exists members_auth;

create table members_auth 
(
  member_id           int,
  salt                varchar(16),
  passwd              varchar(40),
  last_login          timestamp null,
  primary key (member_id)
) ENGINE = InnoDB; 


