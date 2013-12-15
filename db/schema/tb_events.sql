drop table if exists events;

create table events 
(
  event_id      int not null auto_increment,
  event_time    timestamp default CURRENT_TIMESTAMP,  
  event_type    varchar(25),
  event_value   varchar(256) null,
  primary key (event_id)
) ENGINE = InnoDB; 