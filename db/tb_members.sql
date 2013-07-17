drop table if exists members;

create table members 
(
  member_id         int not null auto_increment,
  name              varchar(50),
  email             varchar(100),
  join_date         date,
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
  primary key (member_id)
) ENGINE = InnoDB; 
