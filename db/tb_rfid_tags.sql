drop table if exists rfid_tags;

create table rfid_tags 
(
  member_id     int not null,
  rfid_serial   varchar(50),
  primary key (member_id, rfid_serial)
);