drop table if exists transactions;

create table transactions 
(
  transaction_id        int not null auto_increment,
  member_id             int not null,
  transaction_datetime  timestamp default CURRENT_TIMESTAMP,  
  amount                int,
  transaction_type      varchar(  6) not null, -- VEND or MANUAL
  transaction_status    varchar(  8) not null, -- PENDING, COMPLETE or ABORTED
  transaction_desc      varchar(512) null, 
  product_id            int null,
  recorded_by           int null,
  primary key (transaction_id)
) ENGINE = InnoDB; 
