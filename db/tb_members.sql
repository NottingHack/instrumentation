drop table if exists members;

create table members 
(
  member_id         int not null auto_increment,
  member_number     int,
  name              varchar(50),
  email             varchar(100),
  join_date         date,
  handle            varchar(100),
  unlock_text       varchar(95),
  balance           int not null,
  credit_limit      int not null,
  member_status     int default null,
  username          varchar(50),
  account_id        int default null,
  address_1         varchar(100) default null,
  address_2         varchar(100) default null,
  address_city      varchar(100) default null,
  address_postcode  varchar(100) default null,
  contact_number    varchar(20)  default null,
  primary key (member_id),
  constraint member_handle unique (handle),
  constraint member_number unique (member_number)
) ENGINE = InnoDB; 
