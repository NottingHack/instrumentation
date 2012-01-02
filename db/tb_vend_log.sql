drop table if exists vend_log;

create table vend_log 
(
  vend_tran_id        int not null auto_increment,
  transaction_id      int null default null,  
  rfid_serial         varchar(50),
  member_id           int,
  enq_datetime        timestamp NULL default NULL,
  req_datetime        timestamp NULL default NULL,
  success_datetime    timestamp NULL default NULL,
  cancelled_datetime  timestamp NULL default NULL,
  failed_datetime     timestamp NULL default NULL,
  amount_scaled       int null,
  position            varchar (10) null,
  denied_reason       varchar(100) null,
  primary key (vend_tran_id)
) ENGINE = InnoDB; 
