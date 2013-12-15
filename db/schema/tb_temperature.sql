drop table if exists temperature;

create table temperature
(
  name varchar(100),
  dallas_address varchar(16) not null,
  temperature float,
  time timestamp default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  primary key (dallas_address)
);