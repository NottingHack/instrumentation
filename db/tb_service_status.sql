drop table if exists service_status;

create table service_status 
(
  service_name  varchar(256) not null,
  status        int not null,
  status_str    varchar(1024),
  query_time    timestamp default CURRENT_TIMESTAMP,   
  reply_time    timestamp null, 
  description   varchar(2048),
  primary key (service_name)
);