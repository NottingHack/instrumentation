drop table if exists password_reset;

create table password_reset 
(
  reset_id      int not null auto_increment,
  member_id     int,
  pr_key        varchar(40),
  pr_status     varchar(16),  
  pr_created    timestamp default CURRENT_TIMESTAMP,   
  pr_completed  timestamp NULL default NULL,
  primary key (reset_id)
) ENGINE = InnoDB; 
