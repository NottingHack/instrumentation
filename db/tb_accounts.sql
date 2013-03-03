drop table if exists account;

create table account
(
  account_id          int(11) not null auto_increment,
  payment_ref         varchar(18) not null,
  member_id           int(11) not null,
  primary key (account_id),
  unique key (payment_ref)
) ENGINE = InnoDB; 
