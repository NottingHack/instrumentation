drop table if exists pins;

create table pins 
(
  pin_id       int not null auto_increment,
  pin          varchar(12),
  unlock_text  varchar(100),
  date_added   timestamp default CURRENT_TIMESTAMP,   
  expiry       timestamp NULL default NULL,
  state        int,
  member_id    int default NULL,
  primary key (pin_id)
) ENGINE = InnoDB; 

/*
 pin states:
  10 = Active
  20 = Expired
  30 = Cancelled
  40 = Enroll
*/