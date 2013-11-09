
drop table if exists addresses;

create table addresses 
(
  id            int not null auto_increment,
  mac_address   varchar(100),
  last_seen     timestamp,
  ignore_addr   boolean not null default 0,
  comments      varchar(256),
  primary key (id),
  constraint addr unique (mac_address)
);
