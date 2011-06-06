drop table if exists pins;

create table pins 
(
  pin_id       int not null auto_increment,
  pin          varchar(12),
  unlock_text  varchar(100),
  date_added   timestamp default CURRENT_TIMESTAMP,   
  expiry       timestamp NULL default NULL,
  state        int,
  primary key (pin_id),
  constraint pin_u unique (pin, state)
);