drop table if exists rfid_tags;

create table rfid_tags 
(
  member_id     int not null,
  rfid_serial   varchar(50) not null,
  state         int not null,
  last_used     timestamp,
  primary key (rfid_serial, state)
) ENGINE = InnoDB; 