drop table if exists rfid_tags;

create table rfid_tags 
(
  rfid_id             int not null auto_increment,
  member_id           int not null,
  rfid_serial         varchar(50) null,
  rfid_serial_legacy  varchar(50) null,
  state               int not null,
  last_used           timestamp,
  primary key (rfid_id),
  constraint product_rfid_serial unique (rfid_serial),
  constraint product_rfid_serial_legacy unique (rfid_serial_legacy)
) ENGINE = InnoDB; 
