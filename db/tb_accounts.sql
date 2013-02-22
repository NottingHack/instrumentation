drop table if exists accounts;

create table accounts 
(
  account_id          int(11) not null auto_increment,
  payment_ref         varchar(18) not null,
  rfid_serial         varchar(50),
  member_id           int(11) not null,
  primary key (account_id),
  unique key (payment_ref)
) ENGINE = InnoDB; 
